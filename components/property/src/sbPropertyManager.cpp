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

#include "sbDatetimePropertyInfo.h"
#include "sbNumberPropertyInfo.h"
#include "sbTextPropertyInfo.h"
#include "sbURIPropertyInfo.h"

struct sbStaticProperty {
  const char* mName;
  const char* mColumn;
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
    "http://songbirdnest.com/data/1.0#created",
      "created",
      PR_UINT32_MAX,
  },
  {
    "http://songbirdnest.com/data/1.0#updated",
      "updated",
      PR_UINT32_MAX - 1,
  },
  {
    "http://songbirdnest.com/data/1.0#contentUrl",
      "content_url",
      PR_UINT32_MAX - 2,
  },
  {
    "http://songbirdnest.com/data/1.0#contentMimeType",
      "content_mime_type",
      PR_UINT32_MAX - 3,
  },
  {
    "http://songbirdnest.com/data/1.0#contentLength",
      "content_length",
      PR_UINT32_MAX - 4,
  }
};

NS_IMPL_THREADSAFE_ISUPPORTS2(sbPropertyManager, 
                              sbIPropertyManager,
                              nsIObserver)

sbPropertyManager::sbPropertyManager()
{
  PRBool success = mPropInfoHashtable.Init();
  NS_ASSERTION(success, 
    "sbPropertyManager::mPropInfoHashtable failed to initialize!");
}

sbPropertyManager::~sbPropertyManager()
{
  mPropInfoHashtable.Clear();
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

    // Now we load our libraries.
    rv = CreateSystemProperties();
    NS_ENSURE_SUCCESS(rv, rv);
  }

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

  return (success == PR_TRUE) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP sbPropertyManager::GetPropertyInfo(const nsAString & aName, sbIPropertyInfo **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  PRBool success = PR_FALSE;
  *_retval = nsnull;

  if((success = mPropInfoHashtable.Get(aName, _retval))) {
    NS_ADDREF(*_retval);
  }

  return (success == PR_TRUE) ? NS_OK : NS_ERROR_NOT_AVAILABLE;
}

NS_METHOD sbPropertyManager::CreateSystemProperties()
{
  nsresult rv;
  
  sbDatetimePropertyInfo *datetimeProperty = nsnull;
  sbNumberPropertyInfo *numberProperty = nsnull;
  sbTextPropertyInfo *textProperty = nsnull;
  sbURIPropertyInfo *uriProperty = nsnull;

  //Track name
  textProperty = new sbTextPropertyInfo();
  NS_ENSURE_TRUE(textProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = textProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#trackName"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbITextPropertyInfo *, textProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  textProperty = nsnull;

  //Album name
  textProperty = new sbTextPropertyInfo();
  NS_ENSURE_TRUE(textProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = textProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#albumName"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbITextPropertyInfo *, textProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  textProperty = nsnull;

  //Artist name
  textProperty = new sbTextPropertyInfo();
  NS_ENSURE_TRUE(textProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = textProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#artistName"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbITextPropertyInfo *, textProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  textProperty = nsnull;

  //Duration (in milliseconds)
  //datetimeProperty = new sbDatetimePropertyInfo();
  //TODO

  //Genre
  textProperty = new sbTextPropertyInfo();
  NS_ENSURE_TRUE(textProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = textProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#genre"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbITextPropertyInfo *, textProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  textProperty = nsnull;

  //Track number
  numberProperty = new sbNumberPropertyInfo();
  NS_ENSURE_TRUE(numberProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = numberProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#track"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = numberProperty->SetMinValue(1);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbINumberPropertyInfo *, numberProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  numberProperty = nsnull;

  //Year
  numberProperty = new sbNumberPropertyInfo();
  NS_ENSURE_TRUE(numberProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = numberProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#year"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = numberProperty->SetMinValue(1877);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbINumberPropertyInfo *, numberProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  numberProperty = nsnull;

  //Disc Number
  numberProperty = new sbNumberPropertyInfo();
  NS_ENSURE_TRUE(numberProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = numberProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#trackName"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbINumberPropertyInfo *, numberProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  numberProperty = nsnull;

  //Total Discs
  numberProperty = new sbNumberPropertyInfo();
  NS_ENSURE_TRUE(numberProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = numberProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#trackName"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbINumberPropertyInfo *, numberProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  numberProperty = nsnull;

  //Total tracks
  numberProperty = new sbNumberPropertyInfo();
  NS_ENSURE_TRUE(numberProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = numberProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#trackName"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbINumberPropertyInfo *, numberProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  numberProperty = nsnull;

  //Is part of a compilation
  numberProperty = new sbNumberPropertyInfo();
  NS_ENSURE_TRUE(numberProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = numberProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#trackName"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbINumberPropertyInfo *, numberProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  numberProperty = nsnull;

  //Producer(s)
  textProperty = new sbTextPropertyInfo();
  NS_ENSURE_TRUE(textProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = textProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#trackName"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbITextPropertyInfo *, textProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  textProperty = nsnull;
  
  //Composer(s)
  textProperty = new sbTextPropertyInfo();
  NS_ENSURE_TRUE(textProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = textProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#trackName"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbITextPropertyInfo *, textProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  textProperty = nsnull;

  //Lyricist(s)
  textProperty = new sbTextPropertyInfo();
  NS_ENSURE_TRUE(textProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = textProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#trackName"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbITextPropertyInfo *, textProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  textProperty = nsnull;

  //Lyrics
  textProperty = new sbTextPropertyInfo();
  NS_ENSURE_TRUE(textProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = textProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#trackName"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbITextPropertyInfo *, textProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  textProperty = nsnull;

  //Record Label
  textProperty = new sbTextPropertyInfo();
  NS_ENSURE_TRUE(textProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = textProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#trackName"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbITextPropertyInfo *, textProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  textProperty = nsnull;

  //Album art url
  uriProperty = new sbURIPropertyInfo();
  NS_ENSURE_TRUE(uriProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = uriProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#trackName"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbIURIPropertyInfo *, uriProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  uriProperty = nsnull;

  //Last time played
  //datetimeProperty = new sbDatetimePropertyInfo();
  //TODO

  //Number of times played
  numberProperty = new sbNumberPropertyInfo();
  NS_ENSURE_TRUE(numberProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = numberProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#trackName"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbINumberPropertyInfo *, numberProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  numberProperty = nsnull;

  //Number of times song was skipped
  numberProperty = new sbNumberPropertyInfo();
  NS_ENSURE_TRUE(numberProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = numberProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#trackName"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbINumberPropertyInfo *, numberProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  numberProperty = nsnull;

  //Rating
  numberProperty = new sbNumberPropertyInfo();
  NS_ENSURE_TRUE(numberProperty, NS_ERROR_OUT_OF_MEMORY);
  rv = numberProperty->SetName(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#trackName"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(SB_IPROPERTYINFO_CAST(sbINumberPropertyInfo *, numberProperty));
  NS_ENSURE_SUCCESS(rv, rv);
  numberProperty = nsnull;

  return NS_OK;
}
