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
#include <nsIStringBundle.h>

#include "sbDatetimePropertyInfo.h"
#include "sbNumberPropertyInfo.h"
#include "sbProgressPropertyInfo.h"
#include "sbStandardProperties.h"
#include "sbTextPropertyInfo.h"
#include "sbURIPropertyInfo.h"

#include <sbTArrayStringEnumerator.h>

#if !defined(SB_STRING_BUNDLE_CHROME_URL)
  #define SB_STRING_BUNDLE_CHROME_URL "chrome://songbird/locale/songbird.properties"
#endif

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

  return NS_OK;
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

NS_IMETHODIMP sbPropertyManager::CreateBundle(const char *aURLSpec, 
                                              nsIStringBundle **_retval)
{
  NS_ENSURE_ARG_POINTER(aURLSpec);
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsnull;
  
  nsresult rv;
  nsCOMPtr<nsIStringBundleService> stringBundleService = 
    do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIStringBundle> stringBundle; 
  rv = stringBundleService->CreateBundle(aURLSpec, 
                                         getter_AddRefs(stringBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = stringBundle;
  NS_ADDREF(*_retval);

  return NS_OK;
}

NS_IMETHODIMP sbPropertyManager::GetStringFromName(nsIStringBundle *aBundle, 
                                                   const nsAString & aName, 
                                                   nsAString & _retval)
{
  NS_ENSURE_ARG_POINTER(aBundle);

  PRUnichar *value;
  nsresult rv = aBundle->GetStringFromName(aName.BeginReading(), &value);
  NS_ENSURE_SUCCESS(rv, rv);
  
  _retval.Assign(value);
  NS_Free(value);

  return NS_OK;
}

NS_METHOD sbPropertyManager::CreateSystemProperties()
{
  nsresult rv;

  nsCOMPtr<nsIStringBundle> stringBundle;
  rv = CreateBundle(SB_STRING_BUNDLE_CHROME_URL, getter_AddRefs(stringBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  //Ordinal
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_ORDINAL), 
                      NS_LITERAL_STRING("property.ordinal"),
                      stringBundle, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Storage Guid (internal use only)
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_STORAGEGUID), EmptyString(),
                    stringBundle, PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Is List (internal use only)
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_ISLIST), EmptyString(),
    stringBundle, PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Date created
  rv = RegisterDateTime(NS_LITERAL_STRING(SB_PROPERTY_CREATED),
                        NS_LITERAL_STRING("property.date_created"),
                        sbIDatetimePropertyInfo::TIMETYPE_DATETIME,
                        stringBundle, 
                        PR_TRUE, 
                        PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Date updated
  rv = RegisterDateTime(NS_LITERAL_STRING(SB_PROPERTY_UPDATED),
                        NS_LITERAL_STRING("property.date_updated"),
                        sbIDatetimePropertyInfo::TIMETYPE_DATETIME,
                        stringBundle,
                        PR_TRUE,
                        PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Content URL
  rv = RegisterURI(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
                   NS_LITERAL_STRING("property.content_url"),
                   stringBundle,
                   PR_TRUE,
                   PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Content Mime Type
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_CONTENTMIMETYPE),
                    NS_LITERAL_STRING("property.content_mime_type"),
                    stringBundle,
                    PR_TRUE,
                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Content Length (-1, can't determine.)
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_CONTENTLENGTH),
                      NS_LITERAL_STRING("property.content_length"),
                      stringBundle,
                      PR_TRUE,
                      PR_FALSE,
                      -1, 
                      PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Track name
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME),
                    NS_LITERAL_STRING("property.track_name"),
                    stringBundle, PR_TRUE, PR_TRUE, 
                    sbIPropertyInfo::SORT_NULL_BIG, PR_TRUE);

  //Album name
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME),
                    NS_LITERAL_STRING("property.album_name"),
                    stringBundle, PR_TRUE, PR_TRUE, 
                    sbIPropertyInfo::SORT_NULL_BIG, PR_TRUE);

  //Artist name
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_ARTISTNAME),
                    NS_LITERAL_STRING("property.artist_name"),
                    stringBundle, PR_TRUE, PR_TRUE, 
                    sbIPropertyInfo::SORT_NULL_BIG, PR_TRUE);

  //Duration (in usecs)
  rv = RegisterDateTime(NS_LITERAL_STRING(SB_PROPERTY_DURATION),
                        NS_LITERAL_STRING("property.duration"),
                        sbIDatetimePropertyInfo::TIMETYPE_DURATION,
                        stringBundle, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Genre
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_GENRE),
                    NS_LITERAL_STRING("property.genre"),
                    stringBundle, PR_TRUE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Track number
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_TRACK),
                      NS_LITERAL_STRING("property.track_no"),
                      stringBundle, PR_TRUE, PR_TRUE, 1, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Year
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_YEAR),
                      NS_LITERAL_STRING("property.year"),
                      stringBundle, PR_TRUE, PR_TRUE, 1877, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Disc Number
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_DISCNUMBER),
                      NS_LITERAL_STRING("property.disc_no"),
                      stringBundle, PR_TRUE, PR_TRUE, 1, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Total Discs
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_TOTALDISCS),
                      NS_LITERAL_STRING("property.total_discs"),
                      stringBundle, PR_TRUE, PR_TRUE, 1, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Total tracks
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_TOTALTRACKS),
                      NS_LITERAL_STRING("property.total_tracks"),
                      stringBundle, PR_TRUE, PR_TRUE, 1, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Is part of a compilation
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_ISPARTOFCOMPILATION),
                      NS_LITERAL_STRING("property.is_part_of_compilation"),
                      stringBundle, PR_TRUE, PR_TRUE, 0, PR_TRUE, 1, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Producer(s)
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_PRODUCERNAME),
                    NS_LITERAL_STRING("property.producer"),
                    stringBundle, PR_TRUE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Composer(s)
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_COMPOSERNAME),
                    NS_LITERAL_STRING("property.composer"),
                    stringBundle, PR_TRUE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Lyricist(s)
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_LYRICISTNAME),
                    NS_LITERAL_STRING("property.lyricist"),
                    stringBundle, PR_TRUE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Lyrics
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_LYRICS),
                    NS_LITERAL_STRING("property.lyrics"),
                    stringBundle, PR_TRUE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Record Label
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_RECORDLABELNAME),
                    NS_LITERAL_STRING("property.record_label_name"),
                    stringBundle, PR_TRUE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Album art url
  rv = RegisterURI(NS_LITERAL_STRING(SB_PROPERTY_ALBUMARTURL),
                   NS_LITERAL_STRING("property.album_art_url"),
                   stringBundle, PR_TRUE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Last played time
  rv = RegisterDateTime(NS_LITERAL_STRING(SB_PROPERTY_LASTPLAYTIME),
                        NS_LITERAL_STRING("property.last_play_time"),
                        sbIDatetimePropertyInfo::TIMETYPE_DATETIME,
                        stringBundle, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Number of times played
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_PLAYCOUNT),
                      NS_LITERAL_STRING("property.play_count"),
                      stringBundle, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Number of times song was skipped
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_SKIPCOUNT),
                      NS_LITERAL_STRING("property.skip_count"),
                      stringBundle, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Rating
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_RATING),
                      NS_LITERAL_STRING("property.rating"),
                      stringBundle, PR_TRUE, PR_TRUE, 
                      0, PR_TRUE, 100, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Origin URL
  rv = RegisterURI(NS_LITERAL_STRING(SB_PROPERTY_ORIGINURL),
                   NS_LITERAL_STRING("property.origin_url"),
                   stringBundle, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Hidden
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                      NS_LITERAL_STRING("property.hidden"),
                      stringBundle, PR_FALSE, PR_FALSE, 
                      0, PR_TRUE, 1, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Progress Value
  rv = RegisterProgress(NS_LITERAL_STRING(SB_PROPERTY_PROGRESSVALUE),
                        NS_LITERAL_STRING("property.progressValue"),
                        NS_LITERAL_STRING(SB_PROPERTY_PROGRESSMODE),
                        NS_LITERAL_STRING("property.progressMode"),
                        stringBundle, PR_TRUE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Display columns
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_COLUMNSPEC),
                    NS_LITERAL_STRING("property.column_spec"),
                    stringBundle, PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Display column widths
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_DEFAULTCOLUMNSPEC),
                    NS_LITERAL_STRING("property.default_column_spec"),
                    stringBundle, PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbPropertyManager::RegisterText(const nsAString& aPropertyName,
                                const nsAString& aDisplayKey,
                                nsIStringBundle* aStringBundle,
                                PRBool aUserViewable,
                                PRBool aUserEditable,
                                PRUint32 aNullSort,
                                PRBool aHasNullSort)
{
  nsAutoPtr<sbTextPropertyInfo> textProperty(new sbTextPropertyInfo());
  NS_ENSURE_TRUE(textProperty, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = textProperty->SetName(aPropertyName);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aDisplayKey.IsEmpty()) {
    nsCOMPtr<nsIStringBundle> stringBundle;
    rv = CreateBundle(SB_STRING_BUNDLE_CHROME_URL, getter_AddRefs(stringBundle));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString displayValue;
    rv = GetStringFromName(stringBundle, aDisplayKey, displayValue);
    if(NS_SUCCEEDED(rv)) {
      rv = textProperty->SetDisplayName(displayValue);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  if (aHasNullSort) {
    rv = textProperty->SetNullSort(aNullSort);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = textProperty->SetUserViewable(aUserViewable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = textProperty->SetUserEditable(aUserEditable);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPropertyInfo> propInfo =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbITextPropertyInfo*, textProperty), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(propInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  textProperty.forget();
  return NS_OK;
}

nsresult
sbPropertyManager::RegisterDateTime(const nsAString& aPropertyName,
                                    const nsAString& aDisplayKey,
                                    PRInt32 aType,
                                    nsIStringBundle* aStringBundle,
                                    PRBool aUserViewable,
                                    PRBool aUserEditable)
{
  nsAutoPtr<sbDatetimePropertyInfo>
    datetimeProperty(new sbDatetimePropertyInfo());
  NS_ENSURE_TRUE(datetimeProperty, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = datetimeProperty->SetName(aPropertyName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = datetimeProperty->SetTimeType(aType);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aDisplayKey.IsEmpty()) {
    nsAutoString displayValue;
    rv = GetStringFromName(aStringBundle, aDisplayKey, displayValue);
    if(NS_SUCCEEDED(rv)) {
      rv = datetimeProperty->SetDisplayName(displayValue);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  rv = datetimeProperty->SetUserViewable(aUserViewable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = datetimeProperty->SetUserEditable(aUserEditable);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPropertyInfo> propInfo =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbIDatetimePropertyInfo*, datetimeProperty), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(propInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  datetimeProperty.forget();
  return NS_OK;
}

nsresult
sbPropertyManager::RegisterURI(const nsAString& aPropertyName,
                               const nsAString& aDisplayKey,
                               nsIStringBundle* aStringBundle,
                               PRBool aUserViewable,
                               PRBool aUserEditable)
{
  nsAutoPtr<sbURIPropertyInfo> uriProperty(new sbURIPropertyInfo());
  NS_ENSURE_TRUE(uriProperty, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = uriProperty->SetName(aPropertyName);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aDisplayKey.IsEmpty()) {
    nsAutoString displayValue;
    rv = GetStringFromName(aStringBundle, aDisplayKey, displayValue);
    if(NS_SUCCEEDED(rv)) {
      rv = uriProperty->SetDisplayName(displayValue);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  rv = uriProperty->SetUserViewable(aUserViewable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = uriProperty->SetUserEditable(aUserEditable);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPropertyInfo> propInfo =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbIURIPropertyInfo*, uriProperty), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(propInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  uriProperty.forget();
  return NS_OK;
}

nsresult
sbPropertyManager::RegisterNumber(const nsAString& aPropertyName,
                                  const nsAString& aDisplayKey,
                                  nsIStringBundle* aStringBundle,
                                  PRBool aUserViewable,
                                  PRBool aUserEditable,
                                  PRInt32 aMinValue,
                                  PRBool aHasMinValue,
                                  PRInt32 aMaxValue,
                                  PRBool aHasMaxValue)
{
  nsAutoPtr<sbNumberPropertyInfo> numberProperty(new sbNumberPropertyInfo());
  NS_ENSURE_TRUE(numberProperty, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = numberProperty->SetName(aPropertyName);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aHasMinValue) {
    rv = numberProperty->SetMinValue(aMinValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (aHasMaxValue) {
    rv = numberProperty->SetMaxValue(aMaxValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (!aDisplayKey.IsEmpty()) {
    nsAutoString displayValue;
    rv = GetStringFromName(aStringBundle, aDisplayKey, displayValue);
    if(NS_SUCCEEDED(rv)) {
      rv = numberProperty->SetDisplayName(displayValue);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  rv = numberProperty->SetUserViewable(aUserViewable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = numberProperty->SetUserEditable(aUserEditable);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPropertyInfo> propInfo =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbINumberPropertyInfo*, numberProperty), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(propInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  numberProperty.forget();
  return NS_OK;
}

nsresult
sbPropertyManager::RegisterProgress(const nsAString& aValuePropertyName,
                                    const nsAString& aValueDisplayKey,
                                    const nsAString& aModePropertyName,
                                    const nsAString& aModeDisplayKey,
                                    nsIStringBundle* aStringBundle, 
                                    PRBool aUserViewable,
                                    PRBool aUserEditable)
{
  nsresult rv = RegisterNumber(aModePropertyName, aModeDisplayKey,
                               aStringBundle, PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoPtr<sbProgressPropertyInfo>
    progressProperty(new sbProgressPropertyInfo());
  NS_ENSURE_TRUE(progressProperty, NS_ERROR_OUT_OF_MEMORY);

  rv = progressProperty->SetName(aValuePropertyName);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aValueDisplayKey.IsEmpty()) {
    nsAutoString displayValue;
    rv = GetStringFromName(aStringBundle, aValueDisplayKey, displayValue);
    if(NS_SUCCEEDED(rv)) {
      rv = progressProperty->SetDisplayName(displayValue);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  rv = progressProperty->SetDisplayUsingSimpleType(NS_LITERAL_STRING("progressmeter"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = progressProperty->SetModePropertyName(aModePropertyName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = progressProperty->SetUserViewable(aUserViewable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = progressProperty->SetUserEditable(aUserEditable);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPropertyInfo> propInfo =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbIProgressPropertyInfo*, progressProperty), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(propInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  progressProperty.forget();
  return NS_OK;
}

nsresult
sbPropertyManager::RegisterCheckbox(const nsAString& aPropertyName,
                                    const nsAString& aDisplayKey,
                                    nsIStringBundle* aStringBundle,
                                    PRBool aUserViewable,
                                    PRBool aUserEditable)
{
  nsAutoPtr<sbTextPropertyInfo> textProperty(new sbTextPropertyInfo());
  NS_ENSURE_TRUE(textProperty, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = textProperty->SetName(aPropertyName);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aDisplayKey.IsEmpty()) {
    nsCOMPtr<nsIStringBundle> stringBundle;
    rv = CreateBundle(SB_STRING_BUNDLE_CHROME_URL, getter_AddRefs(stringBundle));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString displayValue;
    rv = GetStringFromName(stringBundle, aDisplayKey, displayValue);
    if(NS_SUCCEEDED(rv)) {
      rv = textProperty->SetDisplayName(displayValue);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  rv = textProperty->SetDisplayUsingSimpleType(NS_LITERAL_STRING("checkbox"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = textProperty->SetUserViewable(aUserViewable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = textProperty->SetUserEditable(aUserEditable);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPropertyInfo> propInfo =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbITextPropertyInfo*, textProperty), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(propInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  textProperty.forget();
  return NS_OK;
}
