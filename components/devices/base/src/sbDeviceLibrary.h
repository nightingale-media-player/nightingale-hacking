/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2011 POTI, Inc.
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

#ifndef __SBDEVICELIBRARY_H__
#define __SBDEVICELIBRARY_H__

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsInterfaceHashtable.h>
#include <nsIClassInfo.h>
#include <nsISimpleEnumerator.h>

#include <sbIDeviceEventListener.h>
#include <sbIDeviceLibrary.h>
#include <sbILibrary.h>
#include <sbIMediaListListener.h>
#include <sbILocalDatabaseSimpleMediaList.h>
#include <sbIPropertyArray.h>

#include <nsIPrefBranch.h>

#include "sbDeviceLibraryHelpers.h"

class sbDeviceLibrarySyncSettings;
struct PRMonitor;

// These are the methods from sbLibrary that we're going to
// override in sbDeviceLibrary.
#define SB_DECL_SBILIBRARY_OVERRIDES  \
  NS_IMETHOD CreateMediaItem(nsIURI *aContentUri, sbIPropertyArray *aProperties, PRBool aAllowDuplicates, sbIMediaItem **_retval); \
  NS_IMETHOD CreateMediaItemIfNotExist(nsIURI *aContentUri, sbIPropertyArray *aProperties, sbIMediaItem **aResultItem, PRBool *_retval); \
  NS_IMETHOD CreateMediaList(const nsAString & aType, sbIPropertyArray *aProperties, sbIMediaList **_retval);  \
  NS_IMETHOD GetDevice(sbIDevice * *aDevice); \
  NS_IMETHOD ClearItems(); \
  NS_IMETHOD ClearItemsByType(const nsAString &aContentType);

// Use this macro to declare functions that forward the behavior of this
// interface to another object in a safe way.
#define SB_FORWARD_SAFE_SBILIBRARY(_to) \
  NS_IMETHOD GetSupportsForeignMediaItems(PRBool *aSupportsForeignMediaItems) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetSupportsForeignMediaItems(aSupportsForeignMediaItems); } \
  NS_IMETHOD GetCreationParameters(nsIPropertyBag2 * *aCreationParameters) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetCreationParameters(aCreationParameters); } \
  NS_IMETHOD GetFactory(sbILibraryFactory * *aFactory) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetFactory(aFactory); } \
  NS_IMETHOD ContainsItemWithSameIdentity(sbIMediaItem* aMediaItem, PRBool* _retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->ContainsItemWithSameIdentity(aMediaItem, _retval); } \
  NS_IMETHOD GetItemsWithSameIdentity(sbIMediaItem* aMediaItem, nsIArray** _retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetItemsWithSameIdentity(aMediaItem, _retval); } \
  NS_IMETHOD Resolve(nsIURI *aUri, nsIChannel **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Resolve(aUri, _retval); } \
  NS_IMETHOD CopyMediaList(const nsAString & aType, sbIMediaList *aSource, PRBool aDontCopyContent, sbIMediaList **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->CopyMediaList(aType, aSource, aDontCopyContent, _retval); } \
  NS_IMETHOD GetMediaItem(const nsAString & aGuid, sbIMediaItem **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetMediaItem(aGuid, _retval); } \
  NS_IMETHOD GetDuplicate(sbIMediaItem *aMediaItem, sbIMediaItem **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetDuplicate(aMediaItem, _retval); } \
  NS_IMETHOD GetMediaListTypes(nsIStringEnumerator * *aMediaListTypes) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetMediaListTypes(aMediaListTypes); } \
  NS_IMETHOD RegisterMediaListFactory(sbIMediaListFactory *aFactory) { return !_to ? NS_ERROR_NULL_POINTER : _to->RegisterMediaListFactory(aFactory); } \
  NS_IMETHOD Optimize(PRBool aAnalyzeOnly) { return !_to ? NS_ERROR_NULL_POINTER : _to->Optimize(aAnalyzeOnly); } \
  NS_IMETHOD Flush(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->Flush(); } \
  NS_IMETHOD BatchCreateMediaItems(nsIArray *aURIArray, nsIArray *aPropertyArrayArray, PRBool aAllowDuplicates, nsIArray **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->BatchCreateMediaItems(aURIArray, aPropertyArrayArray, aAllowDuplicates, _retval); } \
  NS_IMETHOD BatchCreateMediaItemsIfNotExist(nsIArray *aURIArray, nsIArray *aPropertyArrayArray, nsIArray **aResultItemArray, nsIArray **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->BatchCreateMediaItemsIfNotExist(aURIArray, aPropertyArrayArray, aResultItemArray, _retval); } \
  NS_IMETHOD BatchCreateMediaItemsAsync(sbIBatchCreateMediaItemsListener *aListener, nsIArray *aURIArray, nsIArray *aPropertyArrayArray, PRBool aAllowDuplicates) { return !_to ? NS_ERROR_NULL_POINTER : _to->BatchCreateMediaItemsAsync(aListener, aURIArray, aPropertyArrayArray, aAllowDuplicates); }

// These are the methods from sbLibrary that we're going to
// override in sbDeviceLibrary.
#define SB_DECL_SBIMEDIALIST_OVERRIDES \
  NS_IMETHOD Add(sbIMediaItem *aMediaItem); \
  NS_IMETHOD AddItem(sbIMediaItem *aMediaItem, sbIMediaItem ** aNewMediaItem); \
  NS_IMETHOD AddAll(sbIMediaList *aMediaList); \
  NS_IMETHOD AddSome(nsISimpleEnumerator *aMediaItems); \
  NS_IMETHOD AddSomeAsync(nsISimpleEnumerator *aMediaItems, sbIMediaListAsyncListener *aListener); \
  NS_IMETHOD Clear(void);

#define SB_FORWARD_SAFE_SBIMEDIALIST(_to) \
  NS_IMETHOD GetName(nsAString & aName) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetName(aName); } \
  NS_IMETHOD SetName(const nsAString & aName) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetName(aName); } \
  NS_IMETHOD GetType(nsAString & aType) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetType(aType); } \
  NS_IMETHOD GetLength(PRUint32 *aLength) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetLength(aLength); } \
  NS_IMETHOD GetIsEmpty(PRBool *aIsEmpty) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetIsEmpty(aIsEmpty); } \
  NS_IMETHOD GetUserEditableContent(PRBool *aUserEditableContent) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetUserEditableContent(aUserEditableContent); } \
  NS_IMETHOD GetItemByGuid(const nsAString & aGuid, sbIMediaItem **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetItemByGuid(aGuid, _retval); } \
  NS_IMETHOD GetItemByIndex(PRUint32 aIndex, sbIMediaItem **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetItemByIndex(aIndex, _retval); } \
  NS_IMETHOD GetListContentType(PRUint16 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetListContentType(_retval); } \
  NS_IMETHOD EnumerateAllItems(sbIMediaListEnumerationListener *aEnumerationListener, PRUint16 aEnumerationType) { return !_to ? NS_ERROR_NULL_POINTER : _to->EnumerateAllItems(aEnumerationListener, aEnumerationType); } \
  NS_IMETHOD EnumerateItemsByProperty(const nsAString & aPropertyID, const nsAString & aPropertyValue, sbIMediaListEnumerationListener *aEnumerationListener, PRUint16 aEnumerationType) { return !_to ? NS_ERROR_NULL_POINTER : _to->EnumerateItemsByProperty(aPropertyID, aPropertyValue, aEnumerationListener, aEnumerationType); } \
  NS_IMETHOD EnumerateItemsByProperties(sbIPropertyArray *aProperties, sbIMediaListEnumerationListener *aEnumerationListener, PRUint16 aEnumerationType) { return !_to ? NS_ERROR_NULL_POINTER : _to->EnumerateItemsByProperties(aProperties, aEnumerationListener, aEnumerationType); } \
  NS_IMETHOD GetItemsByProperty(const nsAString & aPropertyID, const nsAString & aPropertyValue, nsIArray **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetItemsByProperty(aPropertyID, aPropertyValue, _retval); } \
  NS_IMETHOD GetItemCountByProperty(const nsAString & aPropertyID, const nsAString & aPropertyValue, PRUint32 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetItemCountByProperty(aPropertyID, aPropertyValue, _retval); } \
  NS_IMETHOD GetItemsByProperties(sbIPropertyArray *aProperties, nsIArray **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetItemsByProperties(aProperties, _retval); } \
  NS_IMETHOD IndexOf(sbIMediaItem *aMediaItem, PRUint32 aStartFrom, PRUint32 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->IndexOf(aMediaItem, aStartFrom, _retval); } \
  NS_IMETHOD LastIndexOf(sbIMediaItem *aMediaItem, PRUint32 aStartFrom, PRUint32 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->LastIndexOf(aMediaItem, aStartFrom, _retval); } \
  NS_IMETHOD Contains(sbIMediaItem *aMediaItem, PRBool *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Contains(aMediaItem, _retval); } \
  NS_IMETHOD Remove(sbIMediaItem *aMediaItem) { return !_to ? NS_ERROR_NULL_POINTER : _to->Remove(aMediaItem); } \
  NS_IMETHOD RemoveByIndex(PRUint32 aIndex) { return !_to ? NS_ERROR_NULL_POINTER : _to->RemoveByIndex(aIndex); } \
  NS_IMETHOD RemoveSome(nsISimpleEnumerator *aMediaItems) { return !_to ? NS_ERROR_NULL_POINTER : _to->RemoveSome(aMediaItems); } \
  NS_IMETHOD AddListener(sbIMediaListListener *aListener, PRBool aOwnsWeak, PRUint32 aFlags, sbIPropertyArray *aPropertyFilter) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddListener(aListener, aOwnsWeak, aFlags, aPropertyFilter); } \
  NS_IMETHOD RemoveListener(sbIMediaListListener *aListener) { return !_to ? NS_ERROR_NULL_POINTER : _to->RemoveListener(aListener); } \
  NS_IMETHOD CreateView(sbIMediaListViewState *aState, sbIMediaListView **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->CreateView(aState, _retval); } \
  NS_IMETHOD RunInBatchMode(sbIMediaListBatchCallback *aCallback, nsISupports *aUserData) { return !_to ? NS_ERROR_NULL_POINTER : _to->RunInBatchMode(aCallback, aUserData); } \
  NS_IMETHOD GetDistinctValuesForProperty(const nsAString & aPropertyID, nsIStringEnumerator **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetDistinctValuesForProperty(aPropertyID, _retval); }

#define SB_FORWARD_SAFE_SBIMEDIAITEM_MINUS_OVERRIDES(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetIsMutable(PRBool *aIsMutable) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetIsMutable(aIsMutable); } \
  NS_SCRIPTABLE NS_IMETHOD GetItemController(sbIMediaItemController **aMediaItemController) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetItemController(aMediaItemController); } \
  NS_SCRIPTABLE NS_IMETHOD GetMediaCreated(PRInt64 *aMediaCreated) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetMediaCreated(aMediaCreated); } \
  NS_SCRIPTABLE NS_IMETHOD SetMediaCreated(PRInt64 aMediaCreated) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetMediaCreated(aMediaCreated); } \
  NS_SCRIPTABLE NS_IMETHOD GetMediaUpdated(PRInt64 *aMediaUpdated) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetMediaUpdated(aMediaUpdated); } \
  NS_SCRIPTABLE NS_IMETHOD SetMediaUpdated(PRInt64 aMediaUpdated) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetMediaUpdated(aMediaUpdated); } \
  NS_SCRIPTABLE NS_IMETHOD GetContentSrc(nsIURI * *aContentSrc) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetContentSrc(aContentSrc); } \
  NS_SCRIPTABLE NS_IMETHOD SetContentSrc(nsIURI * aContentSrc) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetContentSrc(aContentSrc); } \
  NS_SCRIPTABLE NS_IMETHOD GetContentLength(PRInt64 *aContentLength) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetContentLength(aContentLength); } \
  NS_SCRIPTABLE NS_IMETHOD SetContentLength(PRInt64 aContentLength) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetContentLength(aContentLength); } \
  NS_SCRIPTABLE NS_IMETHOD GetContentType(nsAString & aContentType) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetContentType(aContentType); } \
  NS_SCRIPTABLE NS_IMETHOD SetContentType(const nsAString & aContentType) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetContentType(aContentType); } \
  NS_SCRIPTABLE NS_IMETHOD TestIsURIAvailable(nsIObserver *aObserver) { return !_to ? NS_ERROR_NULL_POINTER : _to->TestIsURIAvailable(aObserver); } \
  NS_SCRIPTABLE NS_IMETHOD OpenInputStreamAsync(nsIStreamListener *aListener, nsISupports *aContext, nsIChannel **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->OpenInputStreamAsync(aListener, aContext, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD OpenInputStream(nsIInputStream **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->OpenInputStream(_retval); } \
  NS_SCRIPTABLE NS_IMETHOD OpenOutputStream(nsIOutputStream **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->OpenOutputStream(_retval); } \
  NS_SCRIPTABLE NS_IMETHOD ToString(nsAString & _retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->ToString(_retval); }

class sbDeviceLibrary : public sbIDeviceLibrary,
                        public sbIMediaListListener,
                        public sbILocalDatabaseMediaListCopyListener,
                        public sbIDeviceEventListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO

  NS_DECL_SBIDEVICELIBRARY
  NS_DECL_SBIMEDIALISTLISTENER
  NS_DECL_SBILOCALDATABASEMEDIALISTCOPYLISTENER
  NS_DECL_SBIDEVICEEVENTLISTENER

  sbDeviceLibrary(sbIDevice* aDevice);
  virtual ~sbDeviceLibrary();

  NS_FORWARD_SAFE_SBILIBRARYRESOURCE(mDeviceLibrary)
  SB_FORWARD_SAFE_SBIMEDIALIST(mDeviceLibrary)
  SB_FORWARD_SAFE_SBILIBRARY(mDeviceLibrary)
  SB_FORWARD_SAFE_SBIMEDIAITEM_MINUS_OVERRIDES(mDeviceLibrary)

  NS_IMETHODIMP GetLibrary(sbILibrary** _retval) {
    NS_ENSURE_ARG_POINTER(_retval);
    *_retval = this;
    NS_ADDREF(*_retval);
    return NS_OK;
  }

  SB_DECL_SBILIBRARY_OVERRIDES
  SB_DECL_SBIMEDIALIST_OVERRIDES

  static nsresult GetDefaultDeviceLibraryDatabaseFile
                    (const nsAString& aDeviceIdentifier,
                     nsIFile**        aDBFile);

private:
  /**
   * \brief This callback adds the enumerated listeners to an nsCOMArray.
   *
   * \param aKey      - An nsISupport pointer to a listener for the key.
   * \param aEntry    - An sbIDeviceLibrary entry.
   * \param aUserData - An nsCOMArray to hold the enumerated pointers.
   *
   * \return PL_DHASH_NEXT on success
   * \return PL_DHASH_STOP on failure
   */
  static PLDHashOperator PR_CALLBACK
    AddListenersToCOMArrayCallback(nsISupportsHashKey::KeyType aKey,
                                   sbIDeviceLibraryListener* aEntry,
                                   void* aUserData);

  /**
   * \brief Return in aPrefKey the device preference key for the management type
   *        preference.
   *
   * \param aContentType - Content type for the preference key.
   * \param aPrefKey - Returned device preference key for the management type
   *                   preference.
   */
  nsresult GetMgmtTypePrefKey(PRUint32 aContentType, nsAString& aPrefKey);

  /**
   * \brief Return in aPrefKey the device preference key for the list of sync
   *        playlists.
   *
   * \param aContentType - Content type for the preference key.
   * \param aPrefKey - Returned device preference key for the list of sync
   *                   playlists.
   */
  nsresult GetSyncListsPrefKey(PRUint32 aContentType, nsAString& aPrefKey);

  /**
   * \brief Return in aPrefKey the device preference key for the sync root
   *        folder.
   *
   * \param aContentType - Content type for the preference key.
   * \param aPrefKey - Returned device preference key for the sync root folder.
   */
  nsresult GetSyncRootPrefKey(PRUint32 aContentType, nsAString& aPrefKey);

  /**
   * \brief Create a library for a device instance.
   *
   * Creating a library provides you with storage for all data relating
   * to media items present on the device. After creating a library you will
   * typically want to register it so that it may be shown to the user.
   *
   * When a library is created, two listeners are added to it. One listener
   * takes care of advising the sbIDeviceLibrary interface instance when items
   * need to be transferred to it, deleted from it or updated because data
   * relating to those items have changed.
   *
   * The second listener is responsible for detecting when items are transferred
   * from the devices library to another library.
   *
   * \param aDeviceDatabaseURI Optional URI for the database.
   * \sa RemoveDeviceLibrary, RegisterDeviceLibrary, UnregisterDeviceLibrary
   */
  nsresult CreateDeviceLibrary(const nsAString &aDeviceIdentifier,
                               nsIURI *aDeviceDatabaseURI);

  /**
   * \brief Register the device library with the library manager.
   *
   * Registering the device library with the library manager enables the user
   * to view the library. It becomes accessible to others programmatically as
   * well through the library manager.
   *
   */
  nsresult RegisterDeviceLibrary();

  /**
   * \brief Unregister a device library with the library manager.
   *
   * Unregister a device library with the library manager will make the library
   * vanish from the list of libraries and block out access to it programatically
   * as well.
   *
   * A device should always unregister the device library when the application
   * shuts down, when the device is disconnected suddenly and when the user
   * ejects the device.
   *
   */
  nsresult UnregisterDeviceLibrary();

  /**
   * \brief Update the listeners for the main library to account for changes to
   *        the sync settings.
   */
  nsresult UpdateMainLibraryListeners(
                                  sbIDeviceLibrarySyncSettings * aSyncSettings);

  /**
   * \brief Update the library is read-only property based upon the device
   *        management type preference.
   */
  nsresult UpdateIsReadOnly();

  /**
   * \brief Return true if the device is configured to auto sync and syncing
   *        all.
   *
   * \param aIsMgmtTypeSyncAll Set to true if the device is configured to
   *                           auto sync and syncing all.
   */
  nsresult GetIsMgmtTypeSyncAll(PRBool* aIsMgmtTypeSyncAll);

  /**
   * \brief Return true if the device is configured to auto sync and syncing
   *        playlist.
   *
   * \param aIsMgmtTypeSyncList Set to true if the device is configured to
   *                            auto sync and syncing playlist.
   */
  nsresult GetIsMgmtTypeSyncList(PRBool* aIsMgmtTypeSyncList);

  /**
   * \brief Sets the sync management settings but does not lock our lock.
   *
   * \param aSyncSettings the settings to be set
   */
  nsresult SetSyncSettingsNoLock(sbIDeviceLibrarySyncSettings * aSyncSettings);

  sbDeviceLibrarySyncSettings *  CreateSyncSettings();

  /**
   * \brief library for this device, location is specified by the
   *        aDeviceDatabaseURI param for CreateDeviceLibrary or the default
   *        location under the users profile.
   */
  nsCOMPtr<sbILibrary> mDeviceLibrary;

  /**
   * \brief the device this library belongs to
   */
  nsCOMPtr<sbIDevice> mDevice;

  /**
   * \brief the main library updating listener
   */
  nsRefPtr<sbLibraryUpdateListener> mMainLibraryListener;

  /**
   * \brief the main library updating listener property filter
   */
  nsCOMPtr<sbIMutablePropertyArray> mMainLibraryListenerFilter;

  /**
   * \brief A list of listeners.
   */
  nsInterfaceHashtable<nsISupportsHashKey, sbIDeviceLibraryListener> mListeners;

  /**
   * \brief The sync mode has been changed or not.
   */
  nsRefPtr<sbDeviceLibrarySyncSettings> mCurrentSyncSettings;
  nsRefPtr<sbDeviceLibrarySyncSettings> mTempSyncSettings;
  PRMonitor* mMonitor;
};

#define PREF_IMAGESYNC_ENABLED "imagesync.enabled"
#define PREF_SYNC_PREFIX    "library."
#define PREF_SYNC_BRANCH    ".sync."
#define PREF_SYNC_MODE      "syncMode"
#define PREF_SYNC_MGMTTYPE  "mgmtType"
#define PREF_SYNC_LISTS     "playlists"
#define PREF_SYNC_ROOT      "root"
#define PREF_SYNC_FOLDER    "syncFolder"

extern char const * const gMediaType[];

#endif /* __SBDEVICELIBRARY_H__ */
