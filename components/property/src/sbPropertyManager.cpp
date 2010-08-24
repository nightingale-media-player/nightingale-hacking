/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#include "sbPropertyManager.h"
#include "sbPropertiesCID.h"

#include <nsICategoryManager.h>
#include <nsIGenericFactory.h>
#include <nsIObserverService.h>
#include <nsIStringBundle.h>

#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>

#include "sbBooleanPropertyInfo.h"
#include "sbDatetimePropertyInfo.h"
#include "sbDurationPropertyInfo.h"
#include "sbNumberPropertyInfo.h"
#include "sbStandardProperties.h"
#include "sbDummyProperties.h"
#include "sbTrackTypeTextPropertyInfo.h"
#include "sbTextPropertyInfo.h"
#include "sbURIPropertyInfo.h"
#include "sbImagePropertyInfo.h"
#include "sbDummyPlaylistPropertyInfo.h"
#include "sbDummyContentTypePropertyInfo.h"
#include "sbDownloadButtonPropertyBuilder.h"
#include "sbStatusPropertyBuilder.h"
#include "sbSimpleButtonPropertyBuilder.h"
#include "sbRatingPropertyBuilder.h"
#include "sbOriginPageImagePropertyBuilder.h"
#include "sbImageLinkPropertyInfo.h"
#include "sbDurationPropertyUnitConverter.h"
#include "sbStoragePropertyUnitConverter.h"
#include "sbFrequencyPropertyUnitConverter.h"
#include "sbBitratePropertyUnitConverter.h"
#include <sbLockUtils.h>
#include <sbTArrayStringEnumerator.h>
#include <sbIPropertyBuilder.h>

#ifdef DEBUG
#include <prprf.h>
#endif

/*
 *  * To log this module, set the following environment variable:
 *   *   NSPR_LOG_MODULES=sbPropMan:5
 *    */
#include <prlog.h>
#ifdef PR_LOGGING
static PRLogModuleInfo* gPropManLog = nsnull;
#endif

#define LOG(args) PR_LOG(gPropManLog, PR_LOG_WARN, args)
#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif


#if !defined(SB_STRING_BUNDLE_CHROME_URL)
  #define SB_STRING_BUNDLE_CHROME_URL "chrome://songbird/locale/songbird.properties"
#endif


const struct {
  const char* propName;
  const char* types;
} sFilterListPickerProperties[] = {
  { SB_PROPERTY_ALBUMNAME,       "audio video" },
  { SB_PROPERTY_ALBUMARTISTNAME, "audio video" },
  { SB_PROPERTY_ARTISTNAME,      "audio video" },
  { SB_PROPERTY_GENRE,           "audio video" },
  { SB_PROPERTY_YEAR,            "audio video" },
  { SB_PROPERTY_RATING,          "audio video" },
  { SB_PROPERTY_COMPOSERNAME,    "audio      " },
  { SB_PROPERTY_BITRATE,         "audio      " },
  { SB_PROPERTY_SAMPLERATE,      "audio      " },
  { SB_PROPERTY_BPM,             "audio      " },
  { SB_PROPERTY_SHOWNAME,        "      video" },
};

NS_IMPL_THREADSAFE_ISUPPORTS1(sbPropertyManager,
                              sbIPropertyManager)

sbPropertyManager::sbPropertyManager()
: mPropIDsLock(nsnull)
{
#ifdef PR_LOGGING
  if (!gPropManLog) {
    gPropManLog = PR_NewLogModule("sbPropMan");
  }
#endif

  PRBool success = mPropInfoHashtable.Init(100);
  NS_ASSERTION(success,
    "sbPropertyManager::mPropInfoHashtable failed to initialize!");

  success = mPropDependencyMap.Init(100);
  NS_ASSERTION(success,
    "sbPropertyManager::mPropInfoHashtable failed to initialize!");

  mPropIDsLock = PR_NewLock();
  NS_ASSERTION(mPropIDsLock,
    "sbPropertyManager::mPropIDsLock failed to create lock!");
}

sbPropertyManager::~sbPropertyManager()
{
  mPropInfoHashtable.Clear();
  mPropDependencyMap.Clear();

  if(mPropIDsLock) {
    PR_DestroyLock(mPropIDsLock);
  }
}

NS_METHOD sbPropertyManager::Init()
{
  nsresult rv;

  rv = CreateSystemProperties();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = RegisterFilterListPickerProperties();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIObserverService> obs =
    do_GetService("@mozilla.org/observer-service;1");
  if (obs) {
    obs->NotifyObservers(nsnull, SB_PROPERTY_MANAGER_READY_CATEGORY, nsnull);
  }

  nsCOMPtr<nsIObserver> startupNotifier =
    do_CreateInstance("@mozilla.org/embedcomp/appstartup-notifier;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  startupNotifier->Observe(nsnull, SB_PROPERTY_MANAGER_READY_CATEGORY, nsnull);

  return NS_OK;
}

NS_IMETHODIMP sbPropertyManager::GetPropertyIDs(nsIStringEnumerator * *aPropertyIDs)
{
  NS_ENSURE_ARG_POINTER(aPropertyIDs);

  PR_Lock(mPropIDsLock);
  *aPropertyIDs = new sbTArrayStringEnumerator(&mPropIDs);
  PR_Unlock(mPropIDsLock);

  NS_ENSURE_TRUE(*aPropertyIDs, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*aPropertyIDs);

  return NS_OK;
}

NS_IMETHODIMP sbPropertyManager::AddPropertyInfo(sbIPropertyInfo *aPropertyInfo)
{
  NS_ENSURE_ARG_POINTER(aPropertyInfo);

  nsresult rv;
  nsAutoString id;
  PRBool success = PR_FALSE;

  rv = aPropertyInfo->GetId(id);
  NS_ENSURE_SUCCESS(rv, rv);

  success = mPropInfoHashtable.Put(id, aPropertyInfo);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  PR_Lock(mPropIDsLock);
  mPropIDs.AppendElement(id);
  mPropDependencyMap.Clear();
  PR_Unlock(mPropIDsLock);

  return NS_OK;
}

NS_IMETHODIMP sbPropertyManager::GetPropertyInfo(const nsAString & aID,
                                                 sbIPropertyInfo **_retval)
{
  LOG(( "sbPropertyManager::GetPropertyInfo(%s)",
        NS_LossyConvertUTF16toASCII(aID).get() ));

  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsnull;

  if(mPropInfoHashtable.Get(aID, _retval)) {
    return NS_OK;
  }
  else {
    //Create default property (text) for new property id encountered.
    nsresult rv;
    nsRefPtr<sbTextPropertyInfo> textProperty;

    textProperty = new sbTextPropertyInfo();
    NS_ENSURE_TRUE(textProperty, NS_ERROR_OUT_OF_MEMORY);

    rv = textProperty->Init();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = textProperty->SetId(aID);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIPropertyInfo> propInfo = do_QueryInterface(NS_ISUPPORTS_CAST(sbITextPropertyInfo*, textProperty), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = AddPropertyInfo(propInfo);
    NS_ENSURE_SUCCESS(rv, rv);

    //This is the only safe way to hand off the instance because the hash table
    //may have changed and returning the instance pointer above may yield a
    //stale pointer and cause a crash.
    if(mPropInfoHashtable.Get(aID, _retval)) {
      return NS_OK;
    }
  }

  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP sbPropertyManager::HasProperty(const nsAString &aID,
                                             PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  if(mPropInfoHashtable.Get(aID, nsnull))
    *_retval = PR_TRUE;
  else
    *_retval = PR_FALSE;
  return NS_OK;
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

  nsAutoString value;
  nsresult rv = aBundle->GetStringFromName(aName.BeginReading(),
                                           getter_Copies(value));
  if (NS_SUCCEEDED(rv)) {
    _retval.Assign(value);
  }
  else {
    _retval.Truncate();
#ifdef DEBUG
    char* message = PR_smprintf("sbPropertyManager: '%s' not found in bundle",
                                NS_LossyConvertUTF16toASCII(aName).get());
    NS_WARNING(message);
    PR_smprintf_free(message);
#endif
  }

  return NS_OK;
}

NS_IMETHODIMP sbPropertyManager::GetDependentProperties(const nsAString & aId,
                                                        sbIPropertyArray** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  nsresult rv = NS_OK;
  PRBool success = PR_FALSE;

  sbSimpleAutoLock lock(mPropIDsLock);

  // Lazily init a map like: { propID: [props, that, use, propID], ... }
  if (mPropDependencyMap.Count() == 0) {
    nsCOMPtr<sbIMutablePropertyArray> deps;

    // First create an empty array for every known property
    for (PRUint32 i=0; i < mPropIDs.Length(); i++) {
      deps = do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = deps->SetStrict(PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);

      success = mPropDependencyMap.Put(mPropIDs.ElementAt(i), deps);
      NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
    }

    // Now populate the dependency arrays using the property infos
    nsCOMPtr<sbIPropertyInfo> propertyInfo;
    nsCOMPtr<sbIPropertyArray> secondarySort;
    nsCOMPtr<sbIPropertyArray> currentDeps;
    for (PRUint32 i=0; i < mPropIDs.Length(); i++) {
      nsString dependentID = mPropIDs.ElementAt(i);
      rv = GetPropertyInfo(dependentID, getter_AddRefs(propertyInfo));
      NS_ENSURE_SUCCESS(rv, rv);

      secondarySort = nsnull;
      rv = propertyInfo->GetSecondarySort(getter_AddRefs(secondarySort));

      if (NS_SUCCEEDED(rv) && secondarySort) {
        PRUint32 propertyCount;
        rv = secondarySort->GetLength(&propertyCount);
        NS_ENSURE_SUCCESS(rv, rv);

        // Map prop id to all dependent ids
        for (PRUint32 j=0; j < propertyCount; j++) {
          nsCOMPtr<sbIProperty> property;
          rv = secondarySort->GetPropertyAt(j, getter_AddRefs(property));
          NS_ENSURE_SUCCESS(rv, rv);
          nsString propertyID;
          rv = property->GetId(propertyID);
          NS_ENSURE_SUCCESS(rv, rv);

          success = mPropDependencyMap.Get(propertyID, getter_AddRefs(currentDeps));
          NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
          deps = do_QueryInterface(currentDeps, &rv);
          NS_ENSURE_SUCCESS(rv, rv);

          rv = deps->AppendProperty(dependentID, EmptyString());
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
    }
  }

  // Use the map to return all dependent properties for aID
  success = mPropDependencyMap.Get(aId, _retval);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  return NS_OK;
}

NS_METHOD sbPropertyManager::CreateSystemProperties()
{
  nsresult rv;
  // If you add or remove properties accessible to remote pages, please update
  // the documentation in sbILibraryResource.idl as well, thanks!

  nsCOMPtr<nsIStringBundle> stringBundle;
  rv = CreateBundle(SB_STRING_BUNDLE_CHROME_URL, getter_AddRefs(stringBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  //Ordinal
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_ORDINAL),
                      NS_LITERAL_STRING("property.ordinal"),
                      stringBundle, PR_TRUE, PR_FALSE, 0, PR_FALSE, 0, PR_FALSE,
                      PR_FALSE, PR_FALSE, NULL);
  NS_ENSURE_SUCCESS(rv, rv);

  //Guid (internal use only)
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_GUID), EmptyString(),
                    stringBundle, PR_FALSE, PR_FALSE, 0, PR_FALSE, PR_FALSE,
                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Storage Guid (internal use only)
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_STORAGEGUID), EmptyString(),
                    stringBundle, PR_FALSE, PR_FALSE, 0, PR_FALSE, PR_FALSE,
                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Outer Guid - the reverse of storage guid (internal use only)
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_OUTERGUID), EmptyString(),
                    stringBundle, PR_FALSE, PR_FALSE, 0, PR_FALSE, PR_FALSE,
                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //List Type
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_LISTTYPE), EmptyString(),
                      stringBundle, PR_FALSE, PR_FALSE, 0, PR_FALSE,
                      0, PR_FALSE, PR_TRUE, PR_FALSE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  //Is List
  rv = RegisterBoolean(NS_LITERAL_STRING(SB_PROPERTY_ISLIST), EmptyString(),
                       stringBundle, PR_FALSE, PR_FALSE, PR_FALSE,
                       PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Is Read Only (internal use)
  rv = RegisterBoolean(NS_LITERAL_STRING(SB_PROPERTY_ISREADONLY), EmptyString(),
                       stringBundle, PR_FALSE, PR_FALSE, PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Is Content Read Only (internal use)
  rv = RegisterBoolean(NS_LITERAL_STRING(SB_PROPERTY_ISCONTENTREADONLY), EmptyString(),
                       stringBundle, PR_FALSE, PR_FALSE, PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Only Custom Media Pages (internal use only)
  rv = RegisterBoolean(NS_LITERAL_STRING(SB_PROPERTY_ONLY_CUSTOM_MEDIAPAGES),
                       EmptyString(), stringBundle, PR_FALSE, PR_FALSE,
                       PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Date created
  rv = RegisterDateTime(NS_LITERAL_STRING(SB_PROPERTY_CREATED),
                        NS_LITERAL_STRING("property.date_created"),
                        sbIDatetimePropertyInfo::TIMETYPE_DATETIME,
                        stringBundle,
                        PR_TRUE,
                        PR_FALSE,
                        PR_FALSE,
                        PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Date updated
  rv = RegisterDateTime(NS_LITERAL_STRING(SB_PROPERTY_UPDATED),
                        NS_LITERAL_STRING("property.date_updated"),
                        sbIDatetimePropertyInfo::TIMETYPE_DATETIME,
                        stringBundle,
                        PR_TRUE,
                        PR_FALSE,
                        PR_FALSE,
                        PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Content URL
  rv = RegisterURI(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
                   NS_LITERAL_STRING("property.content_url"),
                   stringBundle,
                   PR_TRUE,
                   PR_FALSE,
                   PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Content Type
  rv = RegisterDummy(new sbDummyContentTypePropertyInfo(),
                     NS_LITERAL_STRING(SB_PROPERTY_CONTENTTYPE),
                     NS_LITERAL_STRING("property.content_type"),
                     stringBundle);
  NS_ENSURE_SUCCESS(rv, rv);

  //Content Length (-1, can't determine.)
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_CONTENTLENGTH),
                      NS_LITERAL_STRING("property.content_length"),
                      stringBundle,
                      PR_TRUE,
                      PR_FALSE,
                      -1, PR_TRUE,
                      0, PR_FALSE,
                      PR_FALSE,
                      PR_FALSE,
                      new sbStoragePropertyUnitConverter());
  NS_ENSURE_SUCCESS(rv, rv);

  // Hash (empty string if can't determine or cannot generate).
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_HASH),
                    NS_LITERAL_STRING("property.content_hash"),
                    stringBundle,
                    PR_FALSE,
                    PR_FALSE,
                    sbIPropertyInfo::SORT_NULL_BIG, PR_TRUE,
                    PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Track name
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME),
                    NS_LITERAL_STRING("property.track_name"),
                    stringBundle, PR_TRUE, PR_TRUE,
                    sbIPropertyInfo::SORT_NULL_BIG, PR_TRUE,
                    PR_TRUE, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Album name
  nsCOMPtr<sbIMutablePropertyArray> albumSecondarySort =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = albumSecondarySort->SetStrict(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Sorting by album will sort by album->disc no->track no->track name
  rv = albumSecondarySort->AppendProperty(
                             NS_LITERAL_STRING(SB_PROPERTY_DISCNUMBER),
                             NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = albumSecondarySort->AppendProperty(
                             NS_LITERAL_STRING(SB_PROPERTY_TRACKNUMBER),
                             NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = albumSecondarySort->AppendProperty(
                             NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME),
                             NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME),
                    NS_LITERAL_STRING("property.album_name"),
                    stringBundle, PR_TRUE, PR_TRUE,
                    sbIPropertyInfo::SORT_NULL_BIG, PR_TRUE,
                    PR_TRUE, PR_TRUE, PR_FALSE, albumSecondarySort);
  NS_ENSURE_SUCCESS(rv, rv);

  //Artist name
  nsCOMPtr<sbIMutablePropertyArray> artistSecondarySort =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = artistSecondarySort->SetStrict(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Sorting by artist will sort by artist->album->disc no->track no->track name
  rv = artistSecondarySort->AppendProperty(
                              NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME),
                              NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = artistSecondarySort->AppendProperty(
                              NS_LITERAL_STRING(SB_PROPERTY_DISCNUMBER),
                              NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = artistSecondarySort->AppendProperty(
                              NS_LITERAL_STRING(SB_PROPERTY_TRACKNUMBER),
                              NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = artistSecondarySort->AppendProperty(
                              NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME),
                              NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_ARTISTNAME),
                    NS_LITERAL_STRING("property.artist_name"),
                    stringBundle, PR_TRUE, PR_TRUE,
                    sbIPropertyInfo::SORT_NULL_BIG, PR_TRUE,
                    PR_TRUE, PR_TRUE, PR_FALSE, artistSecondarySort);
  NS_ENSURE_SUCCESS(rv, rv);

  //Track type
  rv = RegisterTrackTypeText(NS_LITERAL_STRING(SB_PROPERTY_TRACKTYPE),
                             NS_LITERAL_STRING("property.track_type"),
                             stringBundle, PR_TRUE, PR_TRUE,
                             sbIPropertyInfo::SORT_NULL_BIG, PR_TRUE,
                             PR_TRUE, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Duration (in usecs)
  rv = RegisterDuration(NS_LITERAL_STRING(SB_PROPERTY_DURATION),
                        NS_LITERAL_STRING("property.duration"),
                        stringBundle, PR_TRUE, PR_FALSE, PR_TRUE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Genre

  nsCOMPtr<sbIMutablePropertyArray> genreSecondarySort =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = genreSecondarySort->SetStrict(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Sorting by genre will sort by genre->artist->album->disc no->track no->track name
  rv = genreSecondarySort->AppendProperty(
                             NS_LITERAL_STRING(SB_PROPERTY_ARTISTNAME),
                             NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = genreSecondarySort->AppendProperty(
                             NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME),
                             NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = genreSecondarySort->AppendProperty(
                             NS_LITERAL_STRING(SB_PROPERTY_DISCNUMBER),
                             NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = genreSecondarySort->AppendProperty(
                             NS_LITERAL_STRING(SB_PROPERTY_TRACKNUMBER),
                             NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = genreSecondarySort->AppendProperty(
                             NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME),
                             NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_GENRE),
                    NS_LITERAL_STRING("property.genre"),
                    stringBundle, PR_TRUE, PR_TRUE, 0, PR_FALSE,
                    PR_TRUE, PR_TRUE, PR_FALSE, genreSecondarySort);
  NS_ENSURE_SUCCESS(rv, rv);

  //Year

  nsCOMPtr<sbIMutablePropertyArray> yearSecondarySort =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = yearSecondarySort->SetStrict(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Sorting by year will sort by year->artist->album->disc no->track no->track name

  rv = yearSecondarySort->AppendProperty(
                            NS_LITERAL_STRING(SB_PROPERTY_ARTISTNAME),
                            NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = yearSecondarySort->AppendProperty(
                            NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME),
                            NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = yearSecondarySort->AppendProperty(
                            NS_LITERAL_STRING(SB_PROPERTY_DISCNUMBER),
                            NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = yearSecondarySort->AppendProperty(
                            NS_LITERAL_STRING(SB_PROPERTY_TRACKNUMBER),
                            NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = yearSecondarySort->AppendProperty(
                            NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME),
                            NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_YEAR),
                      NS_LITERAL_STRING("property.year"),
                      stringBundle, PR_TRUE, PR_TRUE, 0, PR_FALSE, 9999, PR_TRUE,
                      PR_TRUE, PR_TRUE, NULL, yearSecondarySort);
  NS_ENSURE_SUCCESS(rv, rv);

  //Track number
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_TRACKNUMBER),
                      NS_LITERAL_STRING("property.track_no"),
                      stringBundle, PR_TRUE, PR_TRUE, 1, PR_TRUE, 9999, PR_TRUE,
                      PR_TRUE, PR_TRUE, NULL);
  NS_ENSURE_SUCCESS(rv, rv);

  //Disc Number
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_DISCNUMBER),
                      NS_LITERAL_STRING("property.disc_no"),
                      stringBundle, PR_TRUE, PR_TRUE, 1, PR_TRUE, 999, PR_TRUE,
                      PR_TRUE, PR_TRUE, NULL);
  NS_ENSURE_SUCCESS(rv, rv);

  //Total Discs
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_TOTALDISCS),
                      NS_LITERAL_STRING("property.total_discs"),
                      stringBundle, PR_FALSE, PR_TRUE, 1, PR_TRUE, 999, PR_TRUE,
                      PR_TRUE, PR_TRUE, NULL);
  NS_ENSURE_SUCCESS(rv, rv);

  //Total tracks
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_TOTALTRACKS),
                      NS_LITERAL_STRING("property.total_tracks"),
                      stringBundle, PR_FALSE, PR_TRUE, 1, PR_TRUE, 9999, PR_TRUE,
                      PR_TRUE, PR_TRUE, NULL);
  NS_ENSURE_SUCCESS(rv, rv);

  //Is part of a compilation
  rv = RegisterBoolean(NS_LITERAL_STRING(SB_PROPERTY_ISPARTOFCOMPILATION),
                       NS_LITERAL_STRING("property.is_part_of_compilation"),
                       stringBundle, PR_FALSE, PR_TRUE,
                       PR_TRUE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Producer(s)
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_PRODUCERNAME),
                    NS_LITERAL_STRING("property.producer"),
                    stringBundle, PR_FALSE, PR_TRUE, 0, PR_FALSE,
                    PR_TRUE, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Composer(s)
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_COMPOSERNAME),
                    NS_LITERAL_STRING("property.composer"),
                    stringBundle, PR_TRUE, PR_TRUE, 0, PR_FALSE,
                    PR_TRUE, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Conductor(s)
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_CONDUCTORNAME),
                    NS_LITERAL_STRING("property.conductor"),
                    stringBundle, PR_FALSE, PR_TRUE, 0, PR_FALSE,
                    PR_TRUE, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Lyricist(s)
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_LYRICISTNAME),
                    NS_LITERAL_STRING("property.lyricist"),
                    stringBundle, PR_FALSE, PR_TRUE, 0, PR_FALSE,
                    PR_TRUE, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Lyrics
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_LYRICS),
                    NS_LITERAL_STRING("property.lyrics"),
                    stringBundle, PR_FALSE, PR_TRUE, 0, PR_FALSE,
                    PR_TRUE, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Record Label
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_RECORDLABELNAME),
                    NS_LITERAL_STRING("property.record_label_name"),
                    stringBundle, PR_FALSE, PR_TRUE, 0, PR_FALSE,
                    PR_TRUE, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Primary image url, see bug #11618 for an explanation as to why this
  //property is not user viewable.
  rv = RegisterImage(NS_LITERAL_STRING(SB_PROPERTY_PRIMARYIMAGEURL),
                     NS_LITERAL_STRING("property.primary_image_url"),
                     stringBundle, PR_FALSE, PR_FALSE, PR_TRUE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Last played time
  rv = RegisterDateTime(NS_LITERAL_STRING(SB_PROPERTY_LASTPLAYTIME),
                        NS_LITERAL_STRING("property.last_play_time"),
                        sbIDatetimePropertyInfo::TIMETYPE_DATETIME,
                        stringBundle, PR_TRUE, PR_FALSE, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Number of times played
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_PLAYCOUNT),
                      NS_LITERAL_STRING("property.play_count"),
                      stringBundle, PR_TRUE, PR_FALSE, 0, PR_FALSE, 0, PR_FALSE,
                      PR_TRUE, PR_FALSE, NULL);
  NS_ENSURE_SUCCESS(rv, rv);

  //Playback position on last stop
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_LASTPLAYPOSITION),
                      nsString(),
                      stringBundle, PR_FALSE, PR_FALSE, 0, PR_FALSE, 0, PR_FALSE,
                      PR_FALSE, PR_FALSE, NULL);
  NS_ENSURE_SUCCESS(rv, rv);

  //Last skipped time
  rv = RegisterDateTime(NS_LITERAL_STRING(SB_PROPERTY_LASTSKIPTIME),
                        NS_LITERAL_STRING("property.last_skip_time"),
                        sbIDatetimePropertyInfo::TIMETYPE_DATETIME,
                        stringBundle, PR_TRUE, PR_FALSE, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Number of times song was skipped
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_SKIPCOUNT),
                      NS_LITERAL_STRING("property.skip_count"),
                      stringBundle, PR_TRUE, PR_FALSE, 0, PR_FALSE, 0, PR_FALSE,
                      PR_TRUE, PR_FALSE, NULL);
  NS_ENSURE_SUCCESS(rv, rv);

  //Bitrate
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_BITRATE),
                      NS_LITERAL_STRING("property.bitrate"),
                      stringBundle, PR_TRUE, PR_FALSE, 0, PR_FALSE, 0, PR_FALSE,
                      PR_TRUE, PR_TRUE, new sbBitratePropertyUnitConverter());
  NS_ENSURE_SUCCESS(rv, rv);

  //Channels
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_CHANNELS),
                      NS_LITERAL_STRING("property.channels"),
                      stringBundle, PR_TRUE, PR_FALSE, 0, PR_FALSE, 0, PR_FALSE,
                      PR_TRUE, PR_TRUE, NULL);
  NS_ENSURE_SUCCESS(rv, rv);

  //Samplerate
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_SAMPLERATE),
                      NS_LITERAL_STRING("property.samplerate"),
                      stringBundle, PR_TRUE, PR_FALSE, 0, PR_FALSE, 0, PR_FALSE,
                      PR_TRUE, PR_TRUE, new sbFrequencyPropertyUnitConverter());
  NS_ENSURE_SUCCESS(rv, rv);

  //BPM
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_BPM),
                      NS_LITERAL_STRING("property.bpm"),
                      stringBundle, PR_TRUE, PR_FALSE, 0, PR_FALSE, 999, PR_TRUE,
                      PR_TRUE, PR_TRUE, NULL);
  NS_ENSURE_SUCCESS(rv, rv);

  //Key
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_KEY),
                    NS_LITERAL_STRING("property.key"),
                    stringBundle, PR_FALSE, PR_TRUE, 0, PR_FALSE,
                    PR_TRUE, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Language
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_LANGUAGE),
                    NS_LITERAL_STRING("property.language"),
                    stringBundle, PR_FALSE, PR_TRUE, 0, PR_FALSE,
                    PR_TRUE, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Comment
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_COMMENT),
                    NS_LITERAL_STRING("property.comment"),
                    stringBundle, PR_TRUE, PR_TRUE, 0, PR_FALSE,
                    PR_TRUE, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Copyright
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_COPYRIGHT),
                    NS_LITERAL_STRING("property.copyright"),
                    stringBundle, PR_FALSE, PR_TRUE, 0, PR_FALSE,
                    PR_TRUE, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Copyright URL
  rv = RegisterURI(NS_LITERAL_STRING(SB_PROPERTY_COPYRIGHTURL),
                   NS_LITERAL_STRING("property.copyright_url"),
                   stringBundle, PR_FALSE, PR_FALSE, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Subtitle
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_SUBTITLE),
                    NS_LITERAL_STRING("property.subtitle"),
                    stringBundle, PR_FALSE, PR_TRUE, 0, PR_FALSE,
                    PR_TRUE, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Metadata UUID
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_METADATAUUID),
                    NS_LITERAL_STRING("property.metadata_uuid"),
                    stringBundle, PR_FALSE, PR_FALSE, 0, PR_FALSE,
                    PR_TRUE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  //SoftwareVendor
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_SOFTWAREVENDOR),
                    NS_LITERAL_STRING("property.vendor"),
                    stringBundle, PR_FALSE, PR_TRUE, 0, PR_FALSE,
                    PR_TRUE, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Origin URL
  rv = RegisterURI(NS_LITERAL_STRING(SB_PROPERTY_ORIGINURL),
                   NS_LITERAL_STRING("property.origin_url"),
                   stringBundle, PR_FALSE, PR_FALSE, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Origin Library Guid (internal use only)
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_ORIGINLIBRARYGUID), EmptyString(),
                    stringBundle, PR_FALSE, PR_FALSE, 0, PR_FALSE, PR_FALSE,
                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Origin Item Guid (internal use only)
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID), EmptyString(),
                    stringBundle, PR_FALSE, PR_FALSE, 0, PR_FALSE, PR_FALSE,
                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Download destination
  rv = RegisterURI(NS_LITERAL_STRING(SB_PROPERTY_DESTINATION),
                   NS_LITERAL_STRING("property.destination"),
                   stringBundle, PR_FALSE, PR_FALSE, PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Download Status Target (internal use only)
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_DOWNLOAD_STATUS_TARGET),
                    EmptyString(),
                    stringBundle, PR_FALSE, PR_FALSE,
                    0, PR_TRUE, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Hidden
  rv = RegisterBoolean(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                       NS_LITERAL_STRING("property.hidden"),
                       stringBundle, PR_FALSE, PR_FALSE,
                       PR_TRUE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Display columns
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_COLUMNSPEC),
                    NS_LITERAL_STRING("property.column_spec"),
                    stringBundle, PR_FALSE, PR_FALSE, 0, PR_FALSE,
                    PR_TRUE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Display column widths
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_DEFAULTCOLUMNSPEC),
                    NS_LITERAL_STRING("property.default_column_spec"),
                    stringBundle, PR_FALSE, PR_FALSE, 0, PR_FALSE,
                    PR_TRUE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Origin page
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_ORIGINPAGE),
                    NS_LITERAL_STRING("property.origin_page"),
                    stringBundle, PR_FALSE, PR_FALSE, 0, PR_FALSE,
                    PR_TRUE, PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Artist detail uri
  rv = RegisterURI(NS_LITERAL_STRING(SB_PROPERTY_ARTISTDETAILURL),
                   NS_LITERAL_STRING("property.artist_detail_url"),
                   stringBundle, PR_FALSE, PR_TRUE, PR_TRUE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  //album detail uri
  rv = RegisterURI(NS_LITERAL_STRING(SB_PROPERTY_ALBUMDETAILURL),
                   NS_LITERAL_STRING("property.album_detail_url"),
                   stringBundle, PR_FALSE, PR_TRUE, PR_TRUE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Origin page title
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_ORIGINPAGETITLE),
                    NS_LITERAL_STRING("property.origin_pagetitle"),
                    stringBundle, PR_FALSE, PR_FALSE, 0, PR_FALSE,
                    PR_TRUE, PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Custom type (used for css and metrics reporting)
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_CUSTOMTYPE), EmptyString(),
                    stringBundle, PR_FALSE, PR_FALSE, 0, PR_FALSE,
                    PR_TRUE, PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Is sortable (for lists)
  rv = RegisterBoolean(NS_LITERAL_STRING(SB_PROPERTY_ISSORTABLE), EmptyString(),
                       stringBundle, PR_FALSE, PR_FALSE, PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Download button
  nsRefPtr<sbDownloadButtonPropertyBuilder> dbBuilder =
    new sbDownloadButtonPropertyBuilder();
  NS_ENSURE_TRUE(dbBuilder, NS_ERROR_OUT_OF_MEMORY);

  rv = dbBuilder->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  dbBuilder->SetUserViewable(PR_FALSE);

  rv = dbBuilder->SetRemoteReadable(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dbBuilder->SetRemoteWritable(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPropertyInfo> propertyInfo;
  rv = dbBuilder->Get(getter_AddRefs(propertyInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(propertyInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  // Download details
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_DOWNLOAD_DETAILS),
                    NS_LITERAL_STRING("property.download_details"),
                    stringBundle, PR_FALSE, PR_FALSE, 0, PR_FALSE,
                    PR_TRUE, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Rating
  nsRefPtr<sbRatingPropertyBuilder> rBuilder = new sbRatingPropertyBuilder();
  NS_ENSURE_TRUE(rBuilder, NS_ERROR_OUT_OF_MEMORY);

  rv = rBuilder->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rBuilder->SetUserViewable(PR_TRUE);

  rv = rBuilder->SetPropertyID(NS_LITERAL_STRING(SB_PROPERTY_RATING));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = rBuilder->SetDisplayNameKey(NS_LITERAL_STRING("property.rating"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = rBuilder->SetRemoteReadable(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = rBuilder->SetRemoteWritable(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = rBuilder->Get(getter_AddRefs(propertyInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(propertyInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  // Status Property
  nsRefPtr<sbStatusPropertyBuilder> statusBuilder =
    new sbStatusPropertyBuilder();
  NS_ENSURE_TRUE(statusBuilder, NS_ERROR_OUT_OF_MEMORY);

  rv = statusBuilder->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  statusBuilder->SetUserViewable(PR_FALSE);

  rv = statusBuilder->SetPropertyID(NS_LITERAL_STRING(SB_PROPERTY_CDRIP_STATUS));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statusBuilder->SetDisplayNameKey(NS_LITERAL_STRING("property.cdrip_status"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statusBuilder->SetLabelKey(NS_LITERAL_STRING("property.cdrip_status"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statusBuilder->SetCompletedLabelKey(NS_LITERAL_STRING("property.cdrip_completed"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statusBuilder->SetFailedLabelKey(NS_LITERAL_STRING("property.cdrip_failed"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statusBuilder->SetRemoteReadable(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statusBuilder->SetRemoteWritable(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statusBuilder->Get(getter_AddRefs(propertyInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(propertyInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  // CD disc hash.
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_CDDISCHASH),
                    EmptyString(),
                    stringBundle,
                    PR_FALSE,
                    PR_FALSE,
                    0,
                    PR_FALSE,
                    PR_FALSE,
                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Go To Source
  nsRefPtr<sbOriginPageImagePropertyBuilder> iBuilder = new sbOriginPageImagePropertyBuilder();
  NS_ENSURE_TRUE(iBuilder, NS_ERROR_OUT_OF_MEMORY);

  rv = iBuilder->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  iBuilder->SetUserViewable(PR_FALSE);

  rv = iBuilder->SetPropertyID(NS_LITERAL_STRING(SB_PROPERTY_ORIGINPAGEIMAGE));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = iBuilder->SetDisplayNameKey(NS_LITERAL_STRING("property.origin_page_image"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = iBuilder->SetRemoteReadable(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = iBuilder->SetRemoteWritable(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = iBuilder->SetUserEditable(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = iBuilder->Get(getter_AddRefs(propertyInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(propertyInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  // Artist detail image link
  rv = RegisterImageLink(NS_LITERAL_STRING(SB_PROPERTY_ARTISTDETAIL),
                         NS_LITERAL_STRING("property.artist_detail_image"),
                         stringBundle,
                         PR_FALSE, PR_TRUE, PR_TRUE, PR_TRUE,
                         NS_LITERAL_STRING(SB_PROPERTY_ARTISTDETAILURL));

  // Album detail image link
  rv = RegisterImageLink(NS_LITERAL_STRING(SB_PROPERTY_ALBUMDETAIL),
                         NS_LITERAL_STRING("property.album_detail_image"),
                         stringBundle,
                         PR_FALSE, PR_TRUE, PR_TRUE, PR_TRUE,
                         NS_LITERAL_STRING(SB_PROPERTY_ALBUMDETAILURL));

  //Remote API scope URL
  rv = RegisterURI(NS_LITERAL_STRING(SB_PROPERTY_RAPISCOPEURL),
                   NS_LITERAL_STRING("property.rapi_scope_url"),
                   stringBundle, PR_FALSE, PR_FALSE, PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Remote API SiteID
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_RAPISITEID),
                    NS_LITERAL_STRING("property.rapi_site_id"),
                    stringBundle, PR_FALSE, PR_FALSE, 0, PR_FALSE,
                    PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // MediaListName
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_MEDIALISTNAME),
                    NS_LITERAL_STRING("property.media_list_name"),
                    stringBundle, PR_FALSE, PR_TRUE, 0, PR_FALSE,
                    PR_TRUE, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Disable downloading
  rv = RegisterBoolean(NS_LITERAL_STRING(SB_PROPERTY_DISABLE_DOWNLOAD),
                       EmptyString(),
                       stringBundle, PR_FALSE, PR_FALSE,
                       PR_TRUE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Enable auto-download
  rv = RegisterBoolean(NS_LITERAL_STRING(SB_PROPERTY_ENABLE_AUTO_DOWNLOAD),
                       EmptyString(),
                       stringBundle, PR_FALSE, PR_FALSE,
                       PR_TRUE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Exclude from playback history?
  rv = RegisterBoolean(NS_LITERAL_STRING(SB_PROPERTY_EXCLUDE_FROM_HISTORY),
                       EmptyString(), stringBundle, PR_FALSE, PR_FALSE,
                       PR_TRUE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Transfer policy
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_TRANSFER_POLICY),
                    EmptyString(),
                    stringBundle, PR_FALSE, PR_FALSE, 0, PR_FALSE,
                    PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Default MediaPage URL
  rv = RegisterURI(NS_LITERAL_STRING(SB_PROPERTY_DEFAULT_MEDIAPAGE_URL),
                   NS_LITERAL_STRING("property.default_mediapage_url"),
                   stringBundle,
                   PR_FALSE,
                   PR_FALSE,
                   PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Availability
  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_AVAILABILITY),
                      NS_LITERAL_STRING("property.availability"),
                      stringBundle,
                      PR_FALSE, PR_FALSE,
                      0, PR_FALSE, 0, PR_FALSE,
                      PR_FALSE, PR_FALSE, NULL);

  // Album/Artist Name

  nsCOMPtr<sbIMutablePropertyArray> albumArtistSecondarySort =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = albumArtistSecondarySort->SetStrict(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Sorting by album/artist will sort by album/artist->album->disc no->track no->track name
  rv = albumArtistSecondarySort->AppendProperty(
                                   NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME),
                                   NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = albumArtistSecondarySort->AppendProperty(
                                   NS_LITERAL_STRING(SB_PROPERTY_DISCNUMBER),
                                   NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = albumArtistSecondarySort->AppendProperty(
                                   NS_LITERAL_STRING(SB_PROPERTY_TRACKNUMBER),
                                   NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = albumArtistSecondarySort->AppendProperty(
                                   NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME),
                                   NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_ALBUMARTISTNAME),
                    NS_LITERAL_STRING("property.albumartistname"),
                    stringBundle, PR_TRUE, PR_TRUE, 0, PR_FALSE,
                    PR_TRUE, PR_TRUE, PR_FALSE, albumArtistSecondarySort);
  NS_ENSURE_SUCCESS(rv, rv);

  //Peristent ID of the object on the device (internal use only)
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_DEVICE_PERSISTENT_ID),
                    EmptyString(),
                    stringBundle, PR_FALSE, PR_FALSE, 0, PR_FALSE, PR_FALSE,
                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Playcount when last sync'd (internal use only)
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_LAST_SYNC_PLAYCOUNT),
                    EmptyString(),
                    stringBundle, PR_FALSE, PR_FALSE, 0, PR_FALSE, PR_FALSE,
                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Skip count when last sync'd (internal use only)
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_LAST_SYNC_SKIPCOUNT),
                    EmptyString(),
                    stringBundle, PR_FALSE, PR_FALSE, 0, PR_FALSE, PR_FALSE,
                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Smart medialist state (internal use)
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_SMARTMEDIALIST_STATE),
                    EmptyString(),
                    stringBundle, PR_FALSE, PR_FALSE, 0, PR_FALSE, PR_FALSE,
                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Playlist dummy property for smart medialists internal use
  rv = RegisterDummy(new sbDummyPlaylistPropertyInfo(),
                     NS_LITERAL_STRING(SB_DUMMYPROPERTY_SMARTMEDIALIST_PLAYLIST),
                     NS_LITERAL_STRING("property.dummy.playlist"),
                     stringBundle);
  NS_ENSURE_SUCCESS(rv, rv);

  //Created first-run smart playlists (internal use)
  rv = RegisterBoolean(NS_LITERAL_STRING(SB_PROPERTY_CREATED_FIRSTRUN_SMARTPLAYLISTS), EmptyString(),
                       stringBundle, PR_FALSE, PR_FALSE, PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Download media list Guid (internal use only)
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_DOWNLOAD_MEDIALIST_GUID),
                    EmptyString(),
                    stringBundle, PR_FALSE, PR_FALSE, 0, PR_FALSE, PR_FALSE,
                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Play queue media list Guid (internal use only)
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_PLAYQUEUE_MEDIALIST_GUID),
                    EmptyString(),
                    stringBundle, PR_FALSE, PR_FALSE, 0, PR_FALSE, PR_FALSE,
                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Denote that the library is a device library (internal use)
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_DEVICE_LIBRARY_GUID),
                    EmptyString(),
                    stringBundle, PR_FALSE, PR_FALSE, 0, PR_FALSE,
                    PR_TRUE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  // iTunes Guid (for import/export from/to iTunes)
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_ITUNES_GUID),
                    EmptyString(),
                    stringBundle, PR_FALSE, PR_FALSE, 0, PR_FALSE,
                    PR_TRUE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Should Rip (for cd devices)
  rv = RegisterBoolean(NS_LITERAL_STRING(SB_PROPERTY_SHOULDRIP),
                       NS_LITERAL_STRING("property.shouldrip"),
                       stringBundle,
                       PR_FALSE, PR_TRUE, PR_FALSE, PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Whether a media file is DRM protected
  rv = RegisterBoolean(NS_LITERAL_STRING(SB_PROPERTY_ISDRMPROTECTED),
                       EmptyString(), stringBundle, PR_FALSE, PR_FALSE,
                       PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // If true, don't write metadata to contents (use with libraries and media
  // lists)
  rv = RegisterBoolean(NS_LITERAL_STRING(SB_PROPERTY_DONT_WRITE_METADATA),
                       EmptyString(),
                       stringBundle,
                       PR_FALSE,
                       PR_FALSE,
                       PR_FALSE,
                       PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // video properties
  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_KEYWORDS),
                    NS_LITERAL_STRING("property.keywords"),
                    stringBundle, PR_TRUE, PR_TRUE, 0, PR_FALSE,
                    PR_TRUE, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_DESCRIPTION),
                    NS_LITERAL_STRING("property.description"),
                    stringBundle, PR_TRUE, PR_TRUE, 0, PR_FALSE,
                    PR_TRUE, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = RegisterText(NS_LITERAL_STRING(SB_PROPERTY_SHOWNAME),
                    NS_LITERAL_STRING("property.showName"),
                    stringBundle, PR_TRUE, PR_TRUE, 0, PR_FALSE,
                    PR_TRUE, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_EPISODENUMBER),
                      NS_LITERAL_STRING("property.episodeNumber"),
                      stringBundle, PR_TRUE, PR_TRUE, 1, PR_TRUE, 0,
                      PR_FALSE, PR_TRUE, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = RegisterNumber(NS_LITERAL_STRING(SB_PROPERTY_SEASONNUMBER),
                      NS_LITERAL_STRING("property.seasonNumber"),
                      stringBundle, PR_TRUE, PR_TRUE, 1, PR_TRUE, 0,
                      PR_FALSE, PR_TRUE, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  // Playlist URL
  rv = RegisterURI(NS_LITERAL_STRING(SB_PROPERTY_PLAYLISTURL),
                   NS_LITERAL_STRING("property.playlist_url"),
                   stringBundle,
                   PR_FALSE,
                   PR_FALSE,
                   PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Attempted to fetch artwork remotely (internal use)
  rv = RegisterBoolean
         (NS_LITERAL_STRING(SB_PROPERTY_ATTEMPTED_REMOTE_ART_FETCH),
          EmptyString(),
          stringBundle,
          PR_FALSE,
          PR_FALSE,
          PR_FALSE,
          PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbPropertyManager::RegisterFilterListPickerProperties()
{
  nsresult rv;

  nsCOMPtr<nsICategoryManager> cm =
    do_GetService("@mozilla.org/categorymanager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(sFilterListPickerProperties); i++) {
    // due to mozbug 364864 this never gets persisted in compreg.dat :(
    rv = cm->AddCategoryEntry("filter-list-picker-properties",
                              sFilterListPickerProperties[i].propName,
                              sFilterListPickerProperties[i].types,
                              PR_FALSE,
                              PR_TRUE,
                              nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbPropertyManager::RegisterText(const nsAString& aPropertyID,
                                const nsAString& aDisplayKey,
                                nsIStringBundle* aStringBundle,
                                PRBool aUserViewable,
                                PRBool aUserEditable,
                                PRUint32 aNullSort,
                                PRBool aHasNullSort,
                                PRBool aRemoteReadable,
                                PRBool aRemoteWritable,
                                PRBool aCompressWhitespace,
                                sbIPropertyArray* aSecondarySort)
{
  NS_ASSERTION(aStringBundle, "aStringBundle is null");

  nsRefPtr<sbTextPropertyInfo> textProperty(new sbTextPropertyInfo());
  NS_ENSURE_TRUE(textProperty, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = textProperty->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = textProperty->SetId(aPropertyID);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aDisplayKey.IsEmpty()) {
    nsAutoString displayValue;
    rv = GetStringFromName(aStringBundle, aDisplayKey, displayValue);
    if(NS_SUCCEEDED(rv)) {
      rv = textProperty->SetDisplayName(displayValue);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = textProperty->SetLocalizationKey(aDisplayKey);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (aHasNullSort) {
    rv = textProperty->SetNullSort(aNullSort);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = textProperty->SetUserViewable(aUserViewable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = textProperty->SetUserEditable(aUserEditable);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aSecondarySort) {
    rv = textProperty->SetSecondarySort(aSecondarySort);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (!aCompressWhitespace) {
    rv = textProperty->SetNoCompressWhitespace(!aCompressWhitespace);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<sbIPropertyInfo> propInfo =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbITextPropertyInfo*, textProperty), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetRemoteAccess(propInfo, aRemoteReadable, aRemoteWritable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(propInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbPropertyManager::RegisterTrackTypeText(const nsAString& aPropertyID,
                                         const nsAString& aDisplayKey,
                                         nsIStringBundle* aStringBundle,
                                         PRBool aUserViewable,
                                         PRBool aUserEditable,
                                         PRUint32 aNullSort,
                                         PRBool aHasNullSort,
                                         PRBool aRemoteReadable,
                                         PRBool aRemoteWritable,
                                         PRBool aCompressWhitespace,
                                         sbIPropertyArray* aSecondarySort)
{
  NS_ASSERTION(aStringBundle, "aStringBundle is null");

  nsRefPtr<sbTrackTypeTextPropertyInfo>
    typeProperty(new sbTrackTypeTextPropertyInfo());
  NS_ENSURE_TRUE(typeProperty, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = typeProperty->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = typeProperty->SetId(aPropertyID);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aDisplayKey.IsEmpty()) {
    nsAutoString displayValue;
    rv = GetStringFromName(aStringBundle, aDisplayKey, displayValue);
    if(NS_SUCCEEDED(rv)) {
      rv = typeProperty->SetDisplayName(displayValue);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = typeProperty->SetLocalizationKey(aDisplayKey);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (aHasNullSort) {
    rv = typeProperty->SetNullSort(aNullSort);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = typeProperty->SetUserViewable(aUserViewable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = typeProperty->SetUserEditable(aUserEditable);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aSecondarySort) {
    rv = typeProperty->SetSecondarySort(aSecondarySort);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (!aCompressWhitespace) {
    rv = typeProperty->SetNoCompressWhitespace(!aCompressWhitespace);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<sbIPropertyInfo> propInfo =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbITextPropertyInfo*, typeProperty), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetRemoteAccess(propInfo, aRemoteReadable, aRemoteWritable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(propInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbPropertyManager::RegisterDateTime(const nsAString& aPropertyID,
                                    const nsAString& aDisplayKey,
                                    PRInt32 aType,
                                    nsIStringBundle* aStringBundle,
                                    PRBool aUserViewable,
                                    PRBool aUserEditable,
                                    PRBool aRemoteReadable,
                                    PRBool aRemoteWritable)
{
  NS_ASSERTION(aStringBundle, "aStringBundle is null");

  nsRefPtr<sbDatetimePropertyInfo>
    datetimeProperty(new sbDatetimePropertyInfo());
  NS_ENSURE_TRUE(datetimeProperty, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = datetimeProperty->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = datetimeProperty->SetId(aPropertyID);
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

    rv = datetimeProperty->SetLocalizationKey(aDisplayKey);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = datetimeProperty->SetUserViewable(aUserViewable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = datetimeProperty->SetUserEditable(aUserEditable);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPropertyInfo> propInfo =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbIDatetimePropertyInfo*, datetimeProperty), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetRemoteAccess(propInfo, aRemoteReadable, aRemoteWritable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(propInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbPropertyManager::RegisterDuration(const nsAString& aPropertyID,
                                    const nsAString& aDisplayKey,
                                    nsIStringBundle* aStringBundle,
                                    PRBool aUserViewable,
                                    PRBool aUserEditable,
                                    PRBool aRemoteReadable,
                                    PRBool aRemoteWritable)
{
  NS_ASSERTION(aStringBundle, "aStringBundle is null");

  nsRefPtr<sbDurationPropertyInfo>
    durationProperty(new sbDurationPropertyInfo());
  NS_ENSURE_TRUE(durationProperty, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = durationProperty->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = durationProperty->SetId(aPropertyID);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aDisplayKey.IsEmpty()) {
    nsAutoString displayValue;
    rv = GetStringFromName(aStringBundle, aDisplayKey, displayValue);
    if(NS_SUCCEEDED(rv)) {
      rv = durationProperty->SetDisplayName(displayValue);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = durationProperty->SetLocalizationKey(aDisplayKey);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = durationProperty->SetUserViewable(aUserViewable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = durationProperty->SetUserEditable(aUserEditable);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbIPropertyUnitConverter> propConverter = new sbDurationPropertyUnitConverter();
  rv = durationProperty->SetUnitConverter(propConverter);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPropertyInfo> propInfo =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbIDurationPropertyInfo*, durationProperty), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetRemoteAccess(propInfo, aRemoteReadable, aRemoteWritable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(propInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbPropertyManager::RegisterURI(const nsAString& aPropertyID,
                               const nsAString& aDisplayKey,
                               nsIStringBundle* aStringBundle,
                               PRBool aUserViewable,
                               PRBool aUserEditable,
                               PRBool aRemoteReadable,
                               PRBool aRemoteWritable)
{
  NS_ASSERTION(aStringBundle, "aStringBundle is null");

  nsRefPtr<sbURIPropertyInfo> uriProperty(new sbURIPropertyInfo());
  NS_ENSURE_TRUE(uriProperty, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = uriProperty->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = uriProperty->SetId(aPropertyID);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aDisplayKey.IsEmpty()) {
    nsAutoString displayValue;
    rv = GetStringFromName(aStringBundle, aDisplayKey, displayValue);
    if(NS_SUCCEEDED(rv)) {
      rv = uriProperty->SetDisplayName(displayValue);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = uriProperty->SetLocalizationKey(aDisplayKey);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = uriProperty->SetUserViewable(aUserViewable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = uriProperty->SetUserEditable(aUserEditable);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPropertyInfo> propInfo =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbIURIPropertyInfo*, uriProperty), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetRemoteAccess(propInfo, aRemoteReadable, aRemoteWritable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(propInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbPropertyManager::RegisterImage(const nsAString& aPropertyID,
                                 const nsAString& aDisplayKey,
                                 nsIStringBundle* aStringBundle,
                                 PRBool aUserViewable,
                                 PRBool aUserEditable,
                                 PRBool aRemoteReadable,
                                 PRBool aRemoteWritable)
{
  NS_ASSERTION(aStringBundle, "aStringBundle is null");
  nsresult rv;

  // translate the display name
  nsAutoString displayName(aDisplayKey);
  if (!aDisplayKey.IsEmpty()) {
    rv = GetStringFromName(aStringBundle, aDisplayKey, displayName);
  }

  // create the image property
  nsRefPtr<sbImagePropertyInfo> imageProperty(
      new sbImagePropertyInfo(aPropertyID, displayName, aDisplayKey,
        aRemoteReadable, aRemoteWritable, aUserViewable,
        aUserEditable));
  NS_ENSURE_TRUE(imageProperty, NS_ERROR_OUT_OF_MEMORY);

  rv = AddPropertyInfo(imageProperty);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbPropertyManager::RegisterImageLink(const nsAString& aPropertyID,
                                     const nsAString& aDisplayKey,
                                     nsIStringBundle* aStringBundle,
                                     PRBool aUserViewable,
                                     PRBool aUserEditable,
                                     PRBool aRemoteReadable,
                                     PRBool aRemoteWritable,
                                     const nsAString& aUrlPropertyID)
{
  NS_ASSERTION(aStringBundle, "aStringBundle is null");
  nsresult rv;

  // translate the display name
  nsAutoString displayName(aDisplayKey);
  if (!aDisplayKey.IsEmpty()) {
    rv = GetStringFromName(aStringBundle, aDisplayKey, displayName);
  }

  // create the image property
  nsRefPtr<sbImageLinkPropertyInfo> imageLinkProperty(
      new sbImageLinkPropertyInfo(aPropertyID, displayName, aDisplayKey,
        aRemoteReadable, aRemoteWritable, aUserViewable,
        aUserEditable, aUrlPropertyID));
  NS_ENSURE_TRUE(imageLinkProperty, NS_ERROR_OUT_OF_MEMORY);

  rv = AddPropertyInfo(imageLinkProperty);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbPropertyManager::RegisterNumber(const nsAString& aPropertyID,
                                  const nsAString& aDisplayKey,
                                  nsIStringBundle* aStringBundle,
                                  PRBool aUserViewable,
                                  PRBool aUserEditable,
                                  PRInt32 aMinValue,
                                  PRBool aHasMinValue,
                                  PRInt32 aMaxValue,
                                  PRBool aHasMaxValue,
                                  PRBool aRemoteReadable,
                                  PRBool aRemoteWritable,
                                  sbIPropertyUnitConverter *aUnitConverter,
                                  sbIPropertyArray* aSecondarySort)
{
  NS_ASSERTION(aStringBundle, "aStringBundle is null");

  nsRefPtr<sbNumberPropertyInfo> numberProperty(new sbNumberPropertyInfo());
  NS_ENSURE_TRUE(numberProperty, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = numberProperty->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = numberProperty->SetId(aPropertyID);
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

    rv = numberProperty->SetLocalizationKey(aDisplayKey);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = numberProperty->SetUserViewable(aUserViewable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = numberProperty->SetUserEditable(aUserEditable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = numberProperty->SetUnitConverter(aUnitConverter);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aSecondarySort) {
    rv = numberProperty->SetSecondarySort(aSecondarySort);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<sbIPropertyInfo> propInfo =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbINumberPropertyInfo*, numberProperty), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetRemoteAccess(propInfo, aRemoteReadable, aRemoteWritable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(propInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbPropertyManager::RegisterBoolean(const nsAString &aPropertyID,
                                   const nsAString &aDisplayKey,
                                   nsIStringBundle* aStringBundle,
                                   PRBool aUserViewable,
                                   PRBool aUserEditable,
                                   PRBool aRemoteReadable,
                                   PRBool aRemoteWritable,
                                   PRBool aShouldSuppress)
{
  LOG(( "sbPropertyManager::RegisterBoolean(%s)",
        NS_LossyConvertUTF16toASCII(aPropertyID).get() ));
  NS_ASSERTION(aStringBundle, "aStringBundle is null");

  nsRefPtr<sbBooleanPropertyInfo> booleanProperty(new sbBooleanPropertyInfo());
  NS_ENSURE_TRUE(booleanProperty, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = booleanProperty->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = booleanProperty->SetId(aPropertyID);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aDisplayKey.IsEmpty()) {
    nsAutoString displayValue;
    rv = GetStringFromName(aStringBundle, aDisplayKey, displayValue);
    if(NS_SUCCEEDED(rv)) {
      rv = booleanProperty->SetDisplayName(displayValue);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = booleanProperty->SetLocalizationKey(aDisplayKey);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = booleanProperty->SetUserViewable(aUserViewable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = booleanProperty->SetUserEditable(aUserEditable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = booleanProperty->SetSuppressSelect(aShouldSuppress);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPropertyInfo> propInfo =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbIBooleanPropertyInfo*, booleanProperty), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetRemoteAccess(propInfo, aRemoteReadable, aRemoteWritable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(propInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbPropertyManager::RegisterDummy(sbDummyPropertyInfo *dummyProperty,
                                 const nsAString &aPropertyID,
                                 const nsAString &aDisplayKey,
                                 nsIStringBundle* aStringBundle)
{
  nsresult rv = dummyProperty->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dummyProperty->SetId(aPropertyID);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aDisplayKey.IsEmpty()) {
    nsAutoString displayValue;
    rv = GetStringFromName(aStringBundle, aDisplayKey, displayValue);
    if(NS_SUCCEEDED(rv)) {
      rv = dummyProperty->SetDisplayName(displayValue);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = dummyProperty->SetLocalizationKey(aDisplayKey);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<sbIPropertyInfo> propInfo = do_QueryInterface(NS_ISUPPORTS_CAST(sbIDummyPropertyInfo*, dummyProperty), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPropertyInfo(propInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbPropertyManager::SetRemoteAccess(sbIPropertyInfo* aProperty,
                                   PRBool aRemoteReadable,
                                   PRBool aRemoteWritable)
{
  nsresult rv = NS_OK;

  rv = aProperty->SetRemoteReadable(aRemoteReadable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aProperty->SetRemoteWritable(aRemoteWritable);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
