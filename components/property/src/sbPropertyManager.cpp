/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
// http://songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
//
// Software distributed under the License is distributed
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
// END SONGBIRD GPL
//
*/

#include "sbPropertyManager.h"
#include "sbPropertiesCID.h"

#include <nsAutoLock.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsICategoryManager.h>
#include <nsIGenericFactory.h>
#include <nsIObserverService.h>
#include <nsAutoPtr.h>

#include "sbDatetimePropertyInfo.h"
#include "sbNumberPropertyInfo.h"
#include "sbTextPropertyInfo.h"
#include "sbURIPropertyInfo.h"

#include <sbTArrayStringEnumerator.h>

struct sbStaticProperty {
  const wchar_t* mName;
  const wchar_t* mColumn;
  PRUint32    mID;
};

enum {
  sbPropCreated = 0,
  sbPropUpdated,
  sbPropContentUrl,
  sbPropMimeType,
  sbPropContentLength,
};

static sbStaticProperty kStaticProperties[] = {
  {
    NS_L("http://songbirdnest.com/data/1.0#created"),
    NS_L("created"),
    PR_UINT32_MAX,
  },
  {
    NS_L("http://songbirdnest.com/data/1.0#updated"),
    NS_L("updated"),
    PR_UINT32_MAX - 1,
  },
  {
    NS_L("http://songbirdnest.com/data/1.0#contentUrl"),
    NS_L("content_url"),
    PR_UINT32_MAX - 2,
  },
  {
    NS_L("http://songbirdnest.com/data/1.0#contentMimeType"),
    NS_L("content_mime_type"),
    PR_UINT32_MAX - 3,
  },
  {
    NS_L("http://songbirdnest.com/data/1.0#contentLength"),
    NS_L("content_length"),
    PR_UINT32_MAX - 4,
  }
};

NS_IMPL_THREADSAFE_ISUPPORTS2(sbPropertyManager, 
                              sbIPropertyManager,
                              nsIObserver)

sbPropertyManager::sbPropertyManager()
: mPropNamesLock(nsnull)
{
  PRBool success = mPropInfoHashtable.Init(32);
  NS_ASSERTION(success, 
    "sbPropertyManager::mPropInfoHashtable failed to initialize!");

  mPropNamesLock = PR_NewLock();
  NS_ASSERTION(mPropNamesLock,
    "sbPropertyManager::mPropNamesLock failed to create lock!");
}

sbPropertyManager::~sbPropertyManager()
{
  mPropInfoHashtable.Clear();

  if(mPropNamesLock) {
    PR_DestroyLock(mPropNamesLock);
  }
}

/*static*/ NS_METHOD sbPropertyManager::RegisterSelf(nsIComponentManager* aCompMgr,
                                                     nsIFile* aPath,
                                                     const char* aLoaderStr,
                                                     const char* aType,
                                                     const nsModuleComponentInfo *aInfo)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> categoryManager =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString contractId("service,");
  contractId.Append(SB_PROPERTYMANAGER_CONTRACTID);

  rv = categoryManager->AddCategoryEntry("app-startup",
    SB_PROPERTYMANAGER_DESCRIPTION,
    contractId.get(),
    PR_TRUE, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}

NS_METHOD sbPropertyManager::Init()
{
  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService = 
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this, "app-startup", PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
}

NS_IMETHODIMP
sbPropertyManager::Observe(nsISupports* aSubject,
                           const char* aTopic,
                           const PRUnichar *aData)
{
  if (strcmp(aTopic, "app-startup") == 0) {
    nsresult rv;
    nsCOMPtr<nsIObserverService> observerService = 
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);

    // We have to remove ourselves or we will stay alive until app shutdown.
    if (NS_SUCCEEDED(rv)) {
      observerService->RemoveObserver(this, "app-startup");
    }

    // Now we create the system properties.
    rv = CreateSystemProperties();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP sbPropertyManager::GetPropertyNames(nsIStringEnumerator * *aPropertyNames)
{
  NS_ENSURE_ARG_POINTER(aPropertyNames);

  PR_Lock(mPropNamesLock);
  *aPropertyNames = new sbTArrayStringEnumerator(&mPropNames);
  PR_Unlock(mPropNamesLock);

  NS_ENSURE_TRUE(*aPropertyNames, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*aPropertyNames);

  return NS_OK;
}

NS_IMETHODIMP sbPropertyManager::AddPropertyInfo(sbIPropertyInfo *aPropertyInfo)
{
  NS_ENSURE_ARG_POINTER(aPropertyInfo);

  nsresult rv;
  nsAutoString name;
  PRBool success = PR_FALSE;

  rv = aPropertyInfo->GetName(name);
  NS_ENSURE_SUCCESS(rv, rv);

  success = mPropInfoHashtable.Put(name, aPropertyInfo);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  PR_Lock(mPropNamesLock);
  mPropNames.AppendElement(name);
  PR_Unlock(mPropNamesLock);

  return NS_OK;
}

NS_IMETHODIMP sbPropertyManager::GetPropertyInfo(const nsAString & aName, sbIPropertyInfo **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsnull;
  
  if(mPropInfoHashtable.Get(aName, _retval)) {
    NS_ADDREF(*_retval);
    return NS_OK;
  }
  else {
    //Create default property (text) for new property name encountered.
    nsresult rv;
    nsAutoPtr<sbTextPropertyInfo> textProperty;

    textProperty = new sbTextPropertyInfo();
    NS_ENSURE_TRUE(textProperty, NS_ERROR_OUT_OF_MEMORY);
    rv = textProperty->SetName(aName);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbITextPropertyInfo *, textProperty));
    NS_ENSURE_SUCCESS(rv, rv);
    textProperty.forget();

    //This is the only safe way to hand off the instance because the hash table
    //may have changed and returning the instance pointer above may yield a 
    //stale pointer and cause a crash.
    if(mPropInfoHashtable.Get(aName, _retval)) {
      NS_ADDREF(*_retval);
      return NS_OK;
    }
  }

  return NS_ERROR_NOT_AVAILABLE;
}

NS_METHOD sbPropertyManager::CreateSystemProperties()
{
  nsresult rv;
  
  nsAutoPtr<sbDatetimePropertyInfo> datetimeProperty;
  nsAutoPtr<sbNumberPropertyInfo> numberProperty;
  nsAutoPtr<sbTextPropertyInfo> textProperty;
  nsAutoPtr<sbURIPropertyInfo> uriProperty;

  //Storage Guid
  textProperty = new sbTextPropertyInfo();
  NS_ENSURE_TRUE(textProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = textProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#storageGuid"));
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbITextPropertyInfo *, textProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  textProperty.forget();

  //Date created
  datetimeProperty = new sbDatetimePropertyInfo();
  NS_ENSURE_TRUE(datetimeProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = datetimeProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#created"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = datetimeProperty->SetTimeType(sbIDatetimePropertyInfo::TIMETYPE_DATETIME);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbIDatetimePropertyInfo *, datetimeProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  datetimeProperty.forget();

  //Date updated
  datetimeProperty = new sbDatetimePropertyInfo();
  NS_ENSURE_TRUE(datetimeProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = datetimeProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#updated"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = datetimeProperty->SetTimeType(sbIDatetimePropertyInfo::TIMETYPE_DATETIME);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbIDatetimePropertyInfo *, datetimeProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  datetimeProperty.forget();
  
  //Content URL
  uriProperty = new sbURIPropertyInfo();
  NS_ENSURE_TRUE(uriProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = uriProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#contentUrl"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbIURIPropertyInfo *, uriProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  uriProperty.forget();

  //Content Mime Type
  textProperty = new sbTextPropertyInfo();
  NS_ENSURE_TRUE(textProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = textProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#contentMimeType"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbITextPropertyInfo *, textProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  textProperty.forget();
  
  //Content Length (-1, can't determine.)
  numberProperty = new sbNumberPropertyInfo();
  NS_ENSURE_TRUE(numberProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = numberProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#contentLength"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = numberProperty->SetMinValue(-1);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbINumberPropertyInfo *, numberProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  numberProperty.forget();

  //Track name
  textProperty = new sbTextPropertyInfo();
  NS_ENSURE_TRUE(textProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = textProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#trackName"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbITextPropertyInfo *, textProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  textProperty.forget();

  //Album name
  textProperty = new sbTextPropertyInfo();
  NS_ENSURE_TRUE(textProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = textProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#albumName"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbITextPropertyInfo *, textProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  textProperty.forget();

  //Artist name
  textProperty = new sbTextPropertyInfo();
  NS_ENSURE_TRUE(textProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = textProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#artistName"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbITextPropertyInfo *, textProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  textProperty.forget();

  //Duration (in usecs)
  datetimeProperty = new sbDatetimePropertyInfo();
  NS_ENSURE_TRUE(datetimeProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = datetimeProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#duration"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = datetimeProperty->SetTimeType(sbIDatetimePropertyInfo::TIMETYPE_DURATION);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbIDatetimePropertyInfo *, datetimeProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  datetimeProperty.forget();

  //Genre
  textProperty = new sbTextPropertyInfo();
  NS_ENSURE_TRUE(textProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = textProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#genre"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbITextPropertyInfo *, textProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  textProperty.forget();

  //Track number
  numberProperty = new sbNumberPropertyInfo();
  NS_ENSURE_TRUE(numberProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = numberProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#track"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = numberProperty->SetMinValue(1);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbINumberPropertyInfo *, numberProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  numberProperty.forget();

  //Year
  numberProperty = new sbNumberPropertyInfo();
  NS_ENSURE_TRUE(numberProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = numberProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#year"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = numberProperty->SetMinValue(1877);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbINumberPropertyInfo *, numberProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  numberProperty.forget();

  //Disc Number
  numberProperty = new sbNumberPropertyInfo();
  NS_ENSURE_TRUE(numberProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = numberProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#discNumber"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbINumberPropertyInfo *, numberProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  numberProperty.forget();

  //Total Discs
  numberProperty = new sbNumberPropertyInfo();
  NS_ENSURE_TRUE(numberProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = numberProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#totalDiscs"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbINumberPropertyInfo *, numberProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  numberProperty.forget();

  //Total tracks
  numberProperty = new sbNumberPropertyInfo();
  NS_ENSURE_TRUE(numberProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = numberProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#totalTracks"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbINumberPropertyInfo *, numberProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  numberProperty.forget();

  //Is part of a compilation
  numberProperty = new sbNumberPropertyInfo();
  NS_ENSURE_TRUE(numberProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = numberProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#isPartOfCompilation"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = numberProperty->SetMinValue(0);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = numberProperty->SetMaxValue(1);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbINumberPropertyInfo *, numberProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  numberProperty.forget();

  //Producer(s)
  textProperty = new sbTextPropertyInfo();
  NS_ENSURE_TRUE(textProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = textProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#producerName"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbITextPropertyInfo *, textProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  textProperty.forget();
  
  //Composer(s)
  textProperty = new sbTextPropertyInfo();
  NS_ENSURE_TRUE(textProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = textProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#composerName"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbITextPropertyInfo *, textProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  textProperty.forget();

  //Lyricist(s)
  textProperty = new sbTextPropertyInfo();
  NS_ENSURE_TRUE(textProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = textProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#lyricistName"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbITextPropertyInfo *, textProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  textProperty.forget();

  //Lyrics
  textProperty = new sbTextPropertyInfo();
  NS_ENSURE_TRUE(textProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = textProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#lyrics"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbITextPropertyInfo *, textProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  textProperty.forget();

  //Record Label
  textProperty = new sbTextPropertyInfo();
  NS_ENSURE_TRUE(textProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = textProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#recordLabelName"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbITextPropertyInfo *, textProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  textProperty.forget();

  //Album art url
  uriProperty = new sbURIPropertyInfo();
  NS_ENSURE_TRUE(uriProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = uriProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#albumArtUrl"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbIURIPropertyInfo *, uriProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  uriProperty.forget();

  //Last played time
  datetimeProperty = new sbDatetimePropertyInfo();
  NS_ENSURE_TRUE(datetimeProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = datetimeProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#lastPlayTime"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = datetimeProperty->SetTimeType(sbIDatetimePropertyInfo::TIMETYPE_DATETIME);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbIDatetimePropertyInfo *, datetimeProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  datetimeProperty.forget();

  //Number of times played
  numberProperty = new sbNumberPropertyInfo();
  NS_ENSURE_TRUE(numberProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = numberProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#playCount"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbINumberPropertyInfo *, numberProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  numberProperty.forget();

  //Number of times song was skipped
  numberProperty = new sbNumberPropertyInfo();
  NS_ENSURE_TRUE(numberProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = numberProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#skipCount"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbINumberPropertyInfo *, numberProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  numberProperty.forget();

  //Rating
  numberProperty = new sbNumberPropertyInfo();
  NS_ENSURE_TRUE(numberProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = numberProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#rating"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = numberProperty->SetMinValue(0);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = numberProperty->SetMaxValue(100);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbINumberPropertyInfo *, numberProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  numberProperty.forget();

  return NS_OK;
}
