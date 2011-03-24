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

#include "sbDeviceLibrarySyncDiff.h"

#include <nsIClassInfoImpl.h>
#include <nsIMutableArray.h>

#include <sbIMediaListView.h>
#include <sbIDevice.h>
#include <sbIDeviceLibrary.h>
#include <sbIDeviceManager.h>
#include <sbILibraryChangeset.h>
#include <sbILibraryManager.h>

#include <nsArrayUtils.h>
#include <nsTArray.h>
#include <nsISupportsUtils.h>

#include <sbLibraryChangeset.h>
#include <sbLibraryUtils.h>
#include <sbStandardProperties.h>
#include <sbStringUtils.h>
#include <sbDebugUtils.h>

// Library enumeration listener base class to collect items to sync
class SyncEnumListenerBase:
    public sbIMediaListEnumerationListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER

  SyncEnumListenerBase() :
    mMediaTypes(0),
    mIsDrop(PR_FALSE),
    mPlaylistMatchingUsesOriginGUID(PR_FALSE)
  {
  }

  virtual nsresult Init(PRBool aIsDrop,
                        PRBool aPlaylistMatchingUsesOriginGUID,
                        sbILibrary *aMainLibrary,
                        sbILibrary *aDeviceLibrary);

  virtual nsresult ProcessItem(sbIMediaList *aMediaList,
                               sbIMediaItem *aMediaItem) = 0;

  void SetMediaTypes(PRUint32 aMediaTypes) {
    mMediaTypes = aMediaTypes;
  }

  nsresult Finish();

protected:
  virtual ~SyncEnumListenerBase() {}

  nsresult AddChange(PRUint32 aChangeType,
                     sbIMediaItem *aSrcItem,
                     sbIMediaItem *aDstItem);

  nsresult CreatePropertyChangesForItemAdded(sbIMediaItem *aSourceItem,
                                             nsIArray **aPropertyChanges);
  nsresult CreatePropertyChangesForItemModified(
                                sbIMediaItem *aSourceItem,
                                sbIMediaItem *aDestinationItem,
                                nsIArray **aPropertyChanges);

  nsresult GetTimeProperty(sbIMediaItem *aMediaItem,
                           nsString aPropertyName,
                           PRInt64 *_result);

  nsresult GetMatchingPlaylist(sbILibrary      *aLibrary,
                               sbIMediaList    *aList,
                               sbIMediaList   **aMatchingList);

  virtual nsresult GetMatchingPlaylistByOriginGUID(
        sbILibrary *aLibrary,
        sbIMediaList *aList,
        sbIMediaList **aMatchingList) = 0;

  bool HasCorrectContentType(sbIMediaItem *aItem);
  bool ListIsMixed(sbIMediaList *aList);
  bool ListHasCorrectContentType(sbIMediaList *aList);

  PRUint32 mMediaTypes;
  PRBool   mIsDrop;
  PRBool   mPlaylistMatchingUsesOriginGUID;

  nsTHashtable<nsStringHashKey> mSeenMediaItems;

  nsCOMPtr<sbILibrary> mMainLibrary;
  nsCOMPtr<sbILibrary> mDeviceLibrary;

  nsCOMPtr<nsIMutableArray> mLibraryChanges;

public:
  // The changeset we created
  nsRefPtr<sbLibraryChangeset> mChangeset;

};

NS_IMPL_THREADSAFE_ISUPPORTS1(SyncEnumListenerBase,
                              sbIMediaListEnumerationListener)

nsresult
SyncEnumListenerBase::Init(PRBool aIsDrop,
                           PRBool aPlaylistMatchingUsesOriginGUID,
                           sbILibrary *aMainLibrary,
                           sbILibrary *aDeviceLibrary)
{
  nsresult rv;

  mIsDrop = aIsDrop;
  mPlaylistMatchingUsesOriginGUID = aPlaylistMatchingUsesOriginGUID;

  mMainLibrary = aMainLibrary;
  mDeviceLibrary = aDeviceLibrary;

  NS_NEWXPCOM(mChangeset, sbLibraryChangeset);
  NS_ENSURE_TRUE(mChangeset, NS_ERROR_OUT_OF_MEMORY);

  rv = mChangeset->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  mLibraryChanges = do_CreateInstance(
          "@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mSeenMediaItems.Init();

  return NS_OK;
}

nsresult
SyncEnumListenerBase::Finish()
{
  nsresult rv;

  rv = mChangeset->SetChanges(mLibraryChanges);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
SyncEnumListenerBase::OnEnumerationBegin(sbIMediaList *aMediaList,
                                         PRUint16 *_retval)
{
  *_retval = sbIMediaListEnumerationListener::CONTINUE;

  return NS_OK;
}

NS_IMETHODIMP
SyncEnumListenerBase::OnEnumeratedItem(sbIMediaList *aMediaList,
                                       sbIMediaItem *aMediaItem,
                                       PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  // First: is this something we've already added? Ignore if so.
  nsString itemId;
  rv = aMediaItem->GetGuid(itemId);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mSeenMediaItems.GetEntry(itemId)) {
    return NS_OK;
  }
  nsStringHashKey *key = mSeenMediaItems.PutEntry(itemId);
  NS_ENSURE_TRUE(key, NS_ERROR_OUT_OF_MEMORY);

  // Also ignore hidden items.
  nsString hidden;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                               hidden);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!hidden.EqualsLiteral("1")) {
    rv = ProcessItem(aMediaList, aMediaItem);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  *_retval = sbIMediaListEnumerationListener::CONTINUE;

  return NS_OK;
}

NS_IMETHODIMP
SyncEnumListenerBase::OnEnumerationEnd(sbIMediaList *aMediaList,
                                       nsresult aStatusCode)
{
  return NS_OK;
}

bool
SyncEnumListenerBase::ListIsMixed(sbIMediaList *aList)
{
  nsresult rv;

  PRUint16 listType;
  rv = aList->GetListContentType(&listType);
  NS_ENSURE_SUCCESS(rv, false);

  return listType ==
          (sbIMediaList::CONTENTTYPE_AUDIO | sbIMediaList::CONTENTTYPE_VIDEO);
}

bool
SyncEnumListenerBase::ListHasCorrectContentType(sbIMediaList *aList)
{
  nsresult rv;

  PRUint16 listType;
  rv = aList->GetListContentType(&listType);
  NS_ENSURE_SUCCESS(rv, false);

  if (listType ==
          (sbIMediaList::CONTENTTYPE_AUDIO | sbIMediaList::CONTENTTYPE_VIDEO))
  {
    // Special case. A mixed content playlist is always considered to be of the
    // correct type. Mixed content playlists are crazy-shit.
    return true;
  }
  else if ((listType == sbIMediaList::CONTENTTYPE_AUDIO &&
            mMediaTypes & sbIDeviceLibrarySyncDiff::SYNC_TYPE_AUDIO) ||
           (listType == sbIMediaList::CONTENTTYPE_VIDEO &&
            mMediaTypes & sbIDeviceLibrarySyncDiff::SYNC_TYPE_VIDEO))
  {
    return true;
  }

  return false;
}

bool
SyncEnumListenerBase::HasCorrectContentType(sbIMediaItem *aItem)
{
  nsresult rv;

  nsString contentType;
  rv = aItem->GetContentType(contentType);
  NS_ENSURE_SUCCESS(rv, false);

  if ((contentType.EqualsLiteral("audio") &&
       mMediaTypes & sbIDeviceLibrarySyncDiff::SYNC_TYPE_AUDIO) ||
      (contentType.EqualsLiteral("video") &&
       mMediaTypes & sbIDeviceLibrarySyncDiff::SYNC_TYPE_VIDEO))
    return true;
  return false;
}

nsresult
SyncEnumListenerBase::CreatePropertyChangesForItemModified(
                                sbIMediaItem *aSourceItem,
                                sbIMediaItem *aDestinationItem,
                                nsIArray **aPropertyChanges)
{
  nsresult rv;

  nsCOMPtr<sbIPropertyArray> sourceProperties;
  nsCOMPtr<sbIPropertyArray> destinationProperties;

  rv = aSourceItem->GetProperties(nsnull,
                                  getter_AddRefs(sourceProperties));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aDestinationItem->GetProperties(nsnull,
                                       getter_AddRefs(destinationProperties));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMutableArray> propertyChanges =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 sourceLength;
  rv = sourceProperties->GetLength(&sourceLength);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 destinationLength;
  rv = destinationProperties->GetLength(&destinationLength);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIProperty> property;
  nsTHashtable<nsStringHashKey> sourcePropertyNamesFoundInDestination;

  PRBool success = sourcePropertyNamesFoundInDestination.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  // These properties are excluded from checking since they
  // are automatically maintained and do not reflect actual
  // metadata differences in the items.
  nsTHashtable<nsStringHashKey> propertyExclusionList;
  success = propertyExclusionList.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsStringHashKey* successHashkey =
    propertyExclusionList.PutEntry(NS_LITERAL_STRING(SB_PROPERTY_CREATED));
  NS_ENSURE_TRUE(successHashkey, NS_ERROR_OUT_OF_MEMORY);

  successHashkey =
    propertyExclusionList.PutEntry(NS_LITERAL_STRING(SB_PROPERTY_UPDATED));
  NS_ENSURE_TRUE(successHashkey, NS_ERROR_OUT_OF_MEMORY);

  successHashkey =
    propertyExclusionList.PutEntry(NS_LITERAL_STRING(SB_PROPERTY_GUID));
  NS_ENSURE_TRUE(successHashkey, NS_ERROR_OUT_OF_MEMORY);

  successHashkey =
    propertyExclusionList.PutEntry(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID));
  NS_ENSURE_TRUE(successHashkey, NS_ERROR_OUT_OF_MEMORY);

  successHashkey =
    propertyExclusionList.PutEntry(NS_LITERAL_STRING(SB_PROPERTY_ORIGINLIBRARYGUID));
  NS_ENSURE_TRUE(successHashkey, NS_ERROR_OUT_OF_MEMORY);

  successHashkey =
    propertyExclusionList.PutEntry(NS_LITERAL_STRING(SB_PROPERTY_ORIGINURL));
  NS_ENSURE_TRUE(successHashkey, NS_ERROR_OUT_OF_MEMORY);

  // Sadly we can't use content length because on a device the length may be
  // different
  successHashkey =
    propertyExclusionList.PutEntry(NS_LITERAL_STRING(SB_PROPERTY_CONTENTLENGTH));
  NS_ENSURE_TRUE(successHashkey, NS_ERROR_OUT_OF_MEMORY);

  nsString propertyId;
  nsString propertyValue;
  nsString propertyDestinationValue;

  // First, we process the sourceProperties.
  for(PRUint32 current = 0; current < sourceLength; ++current)
  {
    rv = sourceProperties->GetPropertyAt(current, getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = property->GetId(propertyId);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = property->GetValue(propertyValue);
    NS_ENSURE_SUCCESS(rv, rv);

    if(propertyExclusionList.GetEntry(propertyId)) {
      continue;
    }

    rv = destinationProperties->GetPropertyValue(propertyId,
                                                 propertyDestinationValue);
    // Property has been added.
    if(rv == NS_ERROR_NOT_AVAILABLE) {
      nsRefPtr<sbPropertyChange> propertyChange;
      NS_NEWXPCOM(propertyChange, sbPropertyChange);
      NS_ENSURE_TRUE(propertyChange, NS_ERROR_OUT_OF_MEMORY);

      rv = propertyChange->InitWithValues(sbIChangeOperation::ADDED,
                                          propertyId,
                                          EmptyString(),
                                          propertyValue);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsISupports> element =
        do_QueryInterface(NS_ISUPPORTS_CAST(sbIPropertyChange *, propertyChange), &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = propertyChanges->AppendElement(element,
                                          PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {

      NS_ENSURE_SUCCESS(rv, rv);

      // Didn't fail, it should be present in both source and destination.
      successHashkey = sourcePropertyNamesFoundInDestination.PutEntry(propertyId);
      NS_ENSURE_TRUE(successHashkey, NS_ERROR_OUT_OF_MEMORY);

      if (propertyId.EqualsLiteral(SB_PROPERTY_CONTENTURL)) {
        nsString originURL;
        rv = destinationProperties->GetPropertyValue(
                                       NS_LITERAL_STRING(SB_PROPERTY_ORIGINURL),
                                       originURL);
        if (NS_SUCCEEDED(rv) && !originURL.IsEmpty()) {
          if (propertyValue.Equals(originURL)) {
            continue;
          }
        }
      }
      // Check if the duration is the same in seconds, that's good enough
      else if (propertyId.EqualsLiteral(SB_PROPERTY_DURATION)) {
        PRUint64 const sourceDuration = nsString_ToUint64(propertyValue, &rv);
        if (NS_SUCCEEDED(rv)) {
          PRUint64 const destDuration =
            nsString_ToUint64(propertyDestinationValue, &rv);
          // If the duration was parsed and the difference less than a second
          // then treat it as unchanged
          if (NS_SUCCEEDED(rv)) {
            // MSVC has no llabs(), so do it this way.
            PRInt64 durationDiff = sourceDuration - destDuration;
            if ((durationDiff < 0 && -durationDiff < PR_USEC_PER_SEC) ||
                durationDiff < PR_USEC_PER_SEC)
              continue;
          }
        }
      }
      // Property values are the same, nothing changed,
      // continue onto the next property.
      else if(propertyValue.Equals(propertyDestinationValue)) {
        continue;
      }

      nsRefPtr<sbPropertyChange> propertyChange;
      NS_NEWXPCOM(propertyChange, sbPropertyChange);
      NS_ENSURE_TRUE(propertyChange, NS_ERROR_OUT_OF_MEMORY);

      rv = propertyChange->InitWithValues(sbIChangeOperation::MODIFIED,
                                          propertyId,
                                          propertyDestinationValue,
                                          propertyValue);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsISupports> element =
        do_QueryInterface(NS_ISUPPORTS_CAST(sbIPropertyChange *, propertyChange), &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = propertyChanges->AppendElement(element,
                                          PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // Second, we process the destinationProperties.
  // This will enable us to determine which properties were removed
  // from the source.
  for(PRUint32 current = 0; current < destinationLength; ++current) {
    rv = destinationProperties->GetPropertyAt(current,
                                              getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = property->GetId(propertyId);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = property->GetValue(propertyDestinationValue);
    NS_ENSURE_SUCCESS(rv, rv);

    if(propertyExclusionList.GetEntry(propertyId)) {
      continue;
    }

    if(!sourcePropertyNamesFoundInDestination.GetEntry(propertyId)) {

      // We couldn't find the property in the source properties, this means
      // the property must've been removed.
      nsRefPtr<sbPropertyChange> propertyChange;
      NS_NEWXPCOM(propertyChange, sbPropertyChange);
      NS_ENSURE_TRUE(propertyChange, NS_ERROR_OUT_OF_MEMORY);

      rv = propertyChange->InitWithValues(sbIChangeOperation::DELETED,
                                          propertyId,
                                          propertyDestinationValue,
                                          EmptyString());
      NS_ENSURE_SUCCESS(rv, rv);

      rv = propertyChanges->AppendElement(NS_ISUPPORTS_CAST(sbIPropertyChange *, propertyChange),
                                          PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  PRUint32 propertyChangesCount = 0;
  rv = propertyChanges->GetLength(&propertyChangesCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (propertyChangesCount == 0) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  return CallQueryInterface(propertyChanges.get(), aPropertyChanges);
}

nsresult
SyncEnumListenerBase::CreatePropertyChangesForItemAdded(
                                sbIMediaItem *aSourceItem,
                                nsIArray **aPropertyChanges)
{
  nsCOMPtr<sbIPropertyArray> properties;
  nsresult rv = aSourceItem->GetProperties(nsnull, getter_AddRefs(properties));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMutableArray> propertyChanges =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 propertyCount = 0;
  rv = properties->GetLength(&propertyCount);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString strPropertyID;
  nsString strPropertyValue;
  nsCOMPtr<sbIProperty> property;

  for(PRUint32 current = 0; current < propertyCount; ++current) {

    rv = properties->GetPropertyAt(current, getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = property->GetId(strPropertyID);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = property->GetValue(strPropertyValue);
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<sbPropertyChange> propertyChange;
    NS_NEWXPCOM(propertyChange, sbPropertyChange);
    NS_ENSURE_TRUE(propertyChange, NS_ERROR_OUT_OF_MEMORY);

    rv = propertyChange->InitWithValues(sbIChangeOperation::ADDED,
                                        strPropertyID,
                                        EmptyString(),
                                        strPropertyValue);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsISupports> element =
      do_QueryInterface(NS_ISUPPORTS_CAST(sbIPropertyChange *, propertyChange), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = propertyChanges->AppendElement(element,
                                        PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return CallQueryInterface(propertyChanges.get(), aPropertyChanges);
}

nsresult
SyncEnumListenerBase::AddChange(PRUint32 aChangeType,
                                sbIMediaItem *aSrcItem,
                                sbIMediaItem *aDstItem)
{
  nsresult rv;

  nsRefPtr<sbLibraryChange> libraryChange;
  NS_NEWXPCOM(libraryChange, sbLibraryChange);
  NS_ENSURE_TRUE(libraryChange, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIArray> propertyChanges;

  if (aChangeType == sbIChangeOperation::ADDED) {
    rv = CreatePropertyChangesForItemAdded(aSrcItem,
                                           getter_AddRefs(propertyChanges));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (aChangeType == sbIChangeOperation::MODIFIED) {
    rv = CreatePropertyChangesForItemModified(aSrcItem,
                                              aDstItem,
                                              getter_AddRefs(propertyChanges));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    PR_NOT_REACHED("Wrong change type");
  }

  rv = libraryChange->InitWithValues(aChangeType,
                                     0,
                                     aSrcItem,
                                     aDstItem,
                                     propertyChanges,
                                     nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupports> element =
      do_QueryInterface(
              NS_ISUPPORTS_CAST(sbILibraryChange *, libraryChange), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mLibraryChanges->AppendElement(element, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
SyncEnumListenerBase::GetTimeProperty(sbIMediaItem *aMediaItem,
                                      nsString aPropertyName,
                                      PRInt64 *_result)
{
  nsAutoString str;
  nsresult rv = aMediaItem->GetProperty(aPropertyName, str);
  NS_ENSURE_SUCCESS(rv, rv);

  *_result = nsString_ToInt64(str, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
SyncEnumListenerBase::GetMatchingPlaylist(sbILibrary      *aLibrary,
                                          sbIMediaList    *aList,
                                          sbIMediaList   **aMatchingList)
{
  // See if the list aList has a matching list in the library. The list is
  // not from the library (e.g. main library list, device library)
  nsresult rv;

  if (mPlaylistMatchingUsesOriginGUID) {
    rv = GetMatchingPlaylistByOriginGUID(aLibrary, aList, aMatchingList);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    // We don't maintain this info for this device. For these device types
    // (such as MSC), we check for a matching list name.
    nsString listName;
    rv = aList->GetName(listName);
    NS_ENSURE_SUCCESS(rv, rv);

    // The list name is internally stored as the MEDIALISTNAME property.
    nsCOMPtr<nsIArray> items;
    rv = aLibrary->GetItemsByProperty(
            NS_LITERAL_STRING(SB_PROPERTY_MEDIALISTNAME),
            listName,
            getter_AddRefs(items));
    if (rv == NS_ERROR_NOT_AVAILABLE) {
      *aMatchingList = NULL;
      return NS_OK;
    }

    // If we get multiple matches, we just pick the first, arbitrarily.
    nsCOMPtr<sbIMediaList> item = do_QueryElementAt(items, 0, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    item.forget(aMatchingList);
  }

  return NS_OK;
}

class SyncExportEnumListener : public SyncEnumListenerBase
{
public:

  SyncExportEnumListener() {}

  virtual nsresult ProcessItem(sbIMediaList *aMediaList,
                               sbIMediaItem *aMediaItem);

protected:
  nsresult GetItemWithOriginGUID(sbILibrary    *aDeviceLibrary,
                                 nsString       aItemID,
                                 sbIMediaItem **aMediaItem);

  virtual nsresult GetMatchingPlaylistByOriginGUID(
        sbILibrary *aLibrary,
        sbIMediaList *aList,
        sbIMediaList **aMatchingList);

private:
  virtual ~SyncExportEnumListener() { }

public:
  nsTArray<nsCOMPtr<sbIMediaList> > mMixedContentPlaylists;
};

/* Look for an item in the device library with an origin item GUID matching
   aItemID */
nsresult
SyncExportEnumListener::GetItemWithOriginGUID(sbILibrary    *aDeviceLibrary,
                                              nsString       aItemID,
                                              sbIMediaItem **aMediaItem)
{
  nsresult rv;

  nsCOMPtr<nsIArray> items;
  rv = aDeviceLibrary->GetItemsByProperty(
          NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID),
          aItemID,
          getter_AddRefs(items));
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    *aMediaItem = NULL;
    return NS_OK;
  }
  
  // We shouldn't ever get multiple matches here. If we do, warn, and just
  // return the first.
  PRUint32 count;
  rv = items->GetLength(&count);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_WARN_IF_FALSE(count < 2, "Multiple OriginGUID matches");
  NS_ASSERTION(count == 1, "GetItemsByProperty returned bogus array");

  nsCOMPtr<sbIMediaItem> item = do_QueryElementAt(items, 0, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  item.forget(aMediaItem);

  return NS_OK;
}

nsresult
SyncExportEnumListener::GetMatchingPlaylistByOriginGUID(
        sbILibrary *aLibrary,
        sbIMediaList *aList,
        sbIMediaList **aMatchingList)
{
  nsresult rv;

  // This is for a device type (such as MTP) where we maintain more
  // information about playlists, including origin item/library GUIDs. So,
  // we can check using that.
  nsString listId;
  rv = aList->GetGuid(listId);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> matchingItem;
  rv = GetItemWithOriginGUID(aLibrary, listId, getter_AddRefs(matchingItem));
  NS_ENSURE_SUCCESS(rv, rv);

  if (matchingItem) {
    rv = CallQueryInterface(matchingItem.get(), aMatchingList);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
SyncExportEnumListener::ProcessItem(sbIMediaList *aMediaList,
                                    sbIMediaItem *aMediaItem)
{
  nsresult rv;

  nsCOMPtr<sbIMediaList> itemAsList = do_QueryInterface(aMediaItem, &rv);
  if (NS_SUCCEEDED(rv)) {
    if (!ListHasCorrectContentType(itemAsList)) {
      return NS_OK;
    }

    if (ListIsMixed(itemAsList)) {
      // We have to do some absurd things with mixed-content playlists; keep
      // track of them.
      nsCOMPtr<sbIMediaList>* element =
          mMixedContentPlaylists.AppendElement(itemAsList);
      NS_ENSURE_TRUE(element, NS_ERROR_OUT_OF_MEMORY);
    }

    // We have a list. We only need to sync the list itself. If we're syncing
    // all content we'll enumerate the items anyway. If we're syncing individual
    // playlists, we'll explicitly enumerate each of those.
    nsCOMPtr<sbIMediaList> matchingPlaylist;
    rv = GetMatchingPlaylist(mDeviceLibrary,
                             itemAsList,
                             getter_AddRefs(matchingPlaylist));
    NS_ENSURE_SUCCESS(rv, rv);

    if (matchingPlaylist) {
      // Check sync time vs. last modified time.
      PRInt64 itemLastModifiedTime;
      PRInt64 lastSyncTime;
      rv = aMediaItem->GetUpdated(&itemLastModifiedTime);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = GetTimeProperty(mDeviceLibrary,
                           NS_LITERAL_STRING(SB_PROPERTY_LAST_SYNC_TIME),
                           &lastSyncTime);

      if (NS_SUCCEEDED(rv) && itemLastModifiedTime > lastSyncTime) {
        // Ok, we have a match - clobber the target item.
        nsCOMPtr<sbIMediaItem> matchingItem = do_QueryInterface(
                matchingPlaylist, &rv);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = AddChange(sbIChangeOperation::MODIFIED,
                       aMediaItem,
                       matchingItem);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      // Otherwise, the playlist hasn't been modified since the last sync, or
      // we couldn't find the last sync time; don't do anything.
    }
    else {
      // No match found; add a new item.
      rv = AddChange(sbIChangeOperation::ADDED,
                     aMediaItem,
                     NULL);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else {
    // Normal media item.
    // Second: is it of the correct type?
    if (!HasCorrectContentType(aMediaItem)) {
      return NS_OK;
    }

    // Now we get to the core of the sync logic!
    // Does the device library have an item with origin GUID equal to this
    // item's guid?
    nsString itemId;
    rv = aMediaItem->GetGuid(itemId);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaItem> destMediaItem;
    rv = GetItemWithOriginGUID(mDeviceLibrary,
                               itemId,
                               getter_AddRefs(destMediaItem));
    PRBool hasOriginMatch = NS_SUCCEEDED(rv) && destMediaItem;

    if (mIsDrop) {
      // Drag-and drop case is simple: if we have a matching origin item,
      // overwrite. Otherwise, add a new one.
      if (hasOriginMatch) {
        rv = AddChange(sbIChangeOperation::MODIFIED,
                       aMediaItem,
                       destMediaItem);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        rv = AddChange(sbIChangeOperation::ADDED,
                       aMediaItem,
                       NULL);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    else {
      // Sync case
      if (hasOriginMatch) {
        PRInt64 itemLastModifiedTime;
        PRInt64 lastSyncTime;
        rv = aMediaItem->GetUpdated(&itemLastModifiedTime);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = GetTimeProperty(mDeviceLibrary,
                             NS_LITERAL_STRING(SB_PROPERTY_LAST_SYNC_TIME),
                             &lastSyncTime);

        if (NS_SUCCEEDED(rv) && itemLastModifiedTime > lastSyncTime) {
          // Ok. We want to overwrite the destination media item.
          rv = AddChange(sbIChangeOperation::MODIFIED,
                         aMediaItem,
                         destMediaItem);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
      else {
        // We don't have a definite (OriginGUID based) match, how about an
        // identity (hash) based match?
        PRBool hasMatch;
        rv = mDeviceLibrary->ContainsItemWithSameIdentity(aMediaItem,
                                                          &hasMatch);
        NS_ENSURE_SUCCESS(rv, rv);

        if (!hasMatch) {
          // Ok, no match - appears to be an all-new file. Copy it as a new item
          // to the destination library.
          rv = AddChange(sbIChangeOperation::ADDED,
                         aMediaItem,
                         NULL);
          NS_ENSURE_SUCCESS(rv, rv);
        }
        // Otherwise: looks like it's probably a dupe, but we're not sure, so...
        // do nothing!
      }
    }
  }

  return NS_OK;
}

// Library enumeration listener to collect items to import from a device
class SyncImportEnumListener : public SyncEnumListenerBase
{
public:

  SyncImportEnumListener() {}

  virtual nsresult ProcessItem(sbIMediaList *aMediaList,
                               sbIMediaItem *aMediaItem);
protected:
  virtual ~SyncImportEnumListener() { }

  nsresult IsFromMainLibrary(sbIMediaItem *aMediaItem,
                             PRBool       *aFromMainLibrary);
  nsresult IsInMainLibrary(sbIMediaItem *aMediaItem,
                           PRBool       *aFromMainLibrary);

  virtual nsresult GetMatchingPlaylistByOriginGUID(
        sbILibrary *aLibrary,
        sbIMediaList *aList,
        sbIMediaList **aMatchingList);
};

nsresult
SyncImportEnumListener::IsFromMainLibrary(sbIMediaItem *aMediaItem,
                                          PRBool       *aFromMainLibrary)
{
  // Determine if we know this is from the main library. Must have an origin
  // item GUID (which need not point to a currently-existing item!), and have an
  // origin library GUID matching the main library.
  nsresult rv;

  nsString originItemGUID;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID),
                               originItemGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  if (originItemGUID.IsVoid()) {
    *aFromMainLibrary = PR_FALSE;
    return NS_OK;
  }

  nsString originLibraryGUID;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINLIBRARYGUID),
                               originLibraryGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString mainLibraryGUID;
  rv = mMainLibrary->GetGuid(mainLibraryGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  *aFromMainLibrary = originLibraryGUID.Equals(mainLibraryGUID);
  return NS_OK;
}

nsresult
SyncImportEnumListener::IsInMainLibrary(sbIMediaItem *aMediaItem,
                                        PRBool       *aInMainLibrary)
{
  // Is the origin item actually IN the main library? This is assumed to only
  // be called if we already know that the origin points at the main library.
  nsresult rv;

  nsString originItemGUID;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID),
                               originItemGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> item;
  rv = mMainLibrary->GetMediaItem(originItemGUID, getter_AddRefs(item));

  *aInMainLibrary = NS_SUCCEEDED(rv) && item;

  return NS_OK;
}

nsresult
SyncImportEnumListener::GetMatchingPlaylistByOriginGUID(
        sbILibrary *aLibrary,
        sbIMediaList *aList,
        sbIMediaList **aMatchingList)
{
  nsresult rv;

  // Ensure the origin library matches aLibrary
  nsString originLibraryGUID;
  rv = aList->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINLIBRARYGUID),
                          originLibraryGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString libraryGUID;
  rv = aLibrary->GetGuid(libraryGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!libraryGUID.Equals(originLibraryGUID)) {
    *aMatchingList = nsnull;
    return NS_OK;
  }

  // Get the origin item ID, which we'll look up in aLibrary
  nsString originItemGUID;
  rv = aList->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID),
                          originItemGUID);
  NS_ENSURE_SUCCESS(rv, rv);


  nsCOMPtr<sbIMediaItem> matchingItem;
  rv = aLibrary->GetMediaItem(originItemGUID, getter_AddRefs(matchingItem));
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(matchingItem.get(), aMatchingList);
}

nsresult
SyncImportEnumListener::ProcessItem(sbIMediaList *aMediaList,
                                    sbIMediaItem *aMediaItem)
{
  nsresult rv;

  nsCOMPtr<sbIMediaList> itemAsList = do_QueryInterface(aMediaItem, &rv);
  if (NS_SUCCEEDED(rv)) {
    if (!ListHasCorrectContentType(itemAsList)) {
      return NS_OK;
    }

    nsCOMPtr<sbIMediaList> matchingPlaylist;
    rv = GetMatchingPlaylist(mMainLibrary,
                             itemAsList,
                             getter_AddRefs(matchingPlaylist));
    NS_ENSURE_SUCCESS(rv, rv);

    if (matchingPlaylist) {
      nsString playlistType;
      rv = matchingPlaylist->GetType(playlistType);
      NS_ENSURE_SUCCESS(rv, rv);

      // We only import changes for simple playlists.
      if (playlistType.EqualsLiteral("simple")) {
        // Check timestamps...
        PRInt64 itemLastModifiedTime;
        PRInt64 lastSyncTime;
        rv = matchingPlaylist->GetUpdated(&itemLastModifiedTime);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = GetTimeProperty(mDeviceLibrary,
                             NS_LITERAL_STRING(SB_PROPERTY_LAST_SYNC_TIME),
                             &lastSyncTime);
        NS_ENSURE_SUCCESS(rv, rv);

        if (itemLastModifiedTime < lastSyncTime) {
          // Main library item hasn't been modified since the last sync;
          // overwrite the version in the main library.
          // Note that this happens even if the device-side playlist is
          // unmodified (we don't know if it has been changed), so every sync
          // will result in these playlists being overwritten (and their
          // last-modified time updated)
          nsCOMPtr<sbIMediaItem> matchingItem =
              do_QueryInterface(matchingPlaylist, &rv);
          NS_ENSURE_SUCCESS(rv, rv);
          rv = AddChange(sbIChangeOperation::MODIFIED,
                         aMediaItem,
                         matchingItem);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
    }
    else {
      // New playlist; add it to the main library
      rv = AddChange(sbIChangeOperation::ADDED,
                     aMediaItem,
                     NULL);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else {
    // Normal media item.
    // Is it of the correct type?
    if (!HasCorrectContentType(aMediaItem)) {
      return NS_OK;
    }

    // If it came from the main library (according to origin item and library
    // guids), we do not wish to re-import it (even if the linked item is no
    // longer in the main library!).
    PRBool isFromMainLibrary;
    rv = IsFromMainLibrary(aMediaItem, &isFromMainLibrary);
    NS_ENSURE_SUCCESS(rv, rv);

    if (mIsDrop) {
      // Drag-and-drop case.
      if (isFromMainLibrary) {
        // Ok, it's from the main library. Is it still IN the main library?
        PRBool isInMainLibrary;
        rv = IsInMainLibrary(aMediaItem, &isInMainLibrary);
        NS_ENSURE_SUCCESS(rv, rv);

        if (!isInMainLibrary) {
          // It's no longer in the main library, but the user dragged it there,
          // so take that to mean "re-add this"
          rv = AddChange(sbIChangeOperation::ADDED,
                         aMediaItem,
                         NULL);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
    }
    else {
      // Sync case.
      if (!isFromMainLibrary) {
        // Didn't come from Songbird (at least not from this profile on this
        // machine). Is there something that _looks_ like the same item? If
        // there is, we don't want to import it.
        PRBool hasMatch;
        rv = mMainLibrary->ContainsItemWithSameIdentity(aMediaItem, &hasMatch);
        NS_ENSURE_SUCCESS(rv, rv);

        if (!hasMatch) {
          // It looks like an all-new item not present in the main library. Time
          // to actually import it!
          rv = AddChange(sbIChangeOperation::ADDED,
                         aMediaItem,
                         NULL);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
    }
  }

  return NS_OK;
}

NS_IMPL_ISUPPORTS1(sbDeviceLibrarySyncDiff, sbIDeviceLibrarySyncDiff)

sbDeviceLibrarySyncDiff::sbDeviceLibrarySyncDiff()
{
  SB_PRLOG_SETUP(sbDeviceLibrarySyncDiff);
}

sbDeviceLibrarySyncDiff::~sbDeviceLibrarySyncDiff()
{
}


NS_IMETHODIMP
sbDeviceLibrarySyncDiff::GenerateSyncLists(
        PRUint32 aMode,
        PRBool   aPlaylistMatchingUsesOriginGUID,
        PRUint32 aMediaTypes,
        sbILibrary *aSourceLibrary,
        sbILibrary *aDestLibrary,
        nsIArray *aSourceLists,
        sbILibraryChangeset **aExportChangeset NS_OUTPARAM,
        sbILibraryChangeset **aImportChangeset NS_OUTPARAM)
{
  NS_ENSURE_ARG_POINTER(aSourceLibrary);
  NS_ENSURE_ARG_POINTER(aDestLibrary);
  NS_ENSURE_ARG_POINTER(aExportChangeset);
  NS_ENSURE_ARG_POINTER(aImportChangeset);

  if (aMode & sbIDeviceLibrarySyncDiff::SYNC_FLAG_EXPORT_PLAYLISTS) {
    NS_ENSURE_ARG_POINTER(aSourceLists);
  }

  nsresult rv;

  nsRefPtr<SyncExportEnumListener> exportListener =
      new SyncExportEnumListener();
  NS_ENSURE_TRUE(exportListener, NS_ERROR_OUT_OF_MEMORY);
  rv = exportListener->Init(PR_FALSE,
                            aPlaylistMatchingUsesOriginGUID,
                            aSourceLibrary,
                            aDestLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  const PRUint32 mixedMediaTypes = sbIDeviceLibrarySyncDiff::SYNC_TYPE_AUDIO |
                                   sbIDeviceLibrarySyncDiff::SYNC_TYPE_VIDEO;

  if (aMode & sbIDeviceLibrarySyncDiff::SYNC_FLAG_EXPORT_ALL) {
    // Enumerate all items to find the ones we need to sync (based on media
    // type, and what's already on the device). This includes playlists.
    // This will find all the playlists (including mixed-content ones!), as well
    // as media items, that match the desired media type. However, it won't find
    // items that are in a mixed-content playlist, but not of the right type.
    exportListener->SetMediaTypes(aMediaTypes);

    rv = aSourceLibrary->EnumerateAllItems(
            exportListener,
            sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
    NS_ENSURE_SUCCESS(rv, rv);

    // If we have any mixed-content playlists, but are not syncing all media
    // types, now do a 2nd pass over those, but with the media types set to all.
    if (aMediaTypes != mixedMediaTypes) {
      exportListener->SetMediaTypes(mixedMediaTypes);
      PRInt32 const mixedListCount =
          exportListener->mMixedContentPlaylists.Length();
      for (PRInt32 i = 0; i < mixedListCount; i++) {
        rv = exportListener->mMixedContentPlaylists[i]->EnumerateAllItems(
                exportListener,
                sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }
  else if (aMode & sbIDeviceLibrarySyncDiff::SYNC_FLAG_EXPORT_PLAYLISTS) {
    // We're not syncing everything, just some specific playlists.
    // Items found in multiple playlists (or multiple times in a single
    // playlist) are dealt with internally.

    // We'll only be asked to export playlists of the correct type. We want
    // to export ALL media in these playlists though - this means that we'll
    // sync video files from a mixed-content playlist.
    exportListener->SetMediaTypes(mixedMediaTypes);

    PRUint32 sourceListLength;
    rv = aSourceLists->GetLength(&sourceListLength);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaList> list;
    for (PRUint32 i = 0; i < sourceListLength; i++) {
      list = do_QueryElementAt(aSourceLists, i, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      // Enumerate the list into our hashmap
      list->EnumerateAllItems(exportListener,
                              sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
      NS_ENSURE_SUCCESS(rv, rv);

      // And add the list itself.
      rv = exportListener->ProcessItem(aSourceLibrary, list);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  rv = exportListener->Finish();
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<SyncImportEnumListener> importListener =
      new SyncImportEnumListener();
  NS_ENSURE_TRUE(importListener, NS_ERROR_OUT_OF_MEMORY);
  rv = importListener->Init(PR_FALSE,
                            aPlaylistMatchingUsesOriginGUID,
                            aSourceLibrary,
                            aDestLibrary);
  NS_ENSURE_SUCCESS(rv, rv);
  importListener->SetMediaTypes(aMediaTypes);

  if (aMode & sbIDeviceLibrarySyncDiff::SYNC_FLAG_IMPORT) {
    // We always import everything (not just select playlists) if this is
    // enabled.
    rv = aDestLibrary->EnumerateAllItems(
            importListener,
            sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
    NS_ENSURE_SUCCESS(rv, rv);

  }

  rv = importListener->Finish();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_IF_ADDREF(*aExportChangeset = exportListener->mChangeset);
  NS_IF_ADDREF(*aImportChangeset = importListener->mChangeset);

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrarySyncDiff::GenerateDropLists(
        PRBool   aPlaylistMatchingUsesOriginGUID,
        sbILibrary *aSourceLibrary,
        sbILibrary *aDestLibrary,
        sbIMediaList *aSourceList,
        nsIArray *aSourceItems,
        sbILibraryChangeset **aChangeset NS_OUTPARAM)
{
  NS_ENSURE_ARG_POINTER (aDestLibrary);
  NS_ENSURE_ARG_POINTER (aChangeset);

  if (!aSourceList)
    NS_ENSURE_ARG_POINTER(aSourceItems);

  nsresult rv;

  // Figure out if we're going to a device or from one.
  bool toDevice = false;
  nsCOMPtr<sbIDeviceManager2> deviceManager = do_GetService(
          "@songbirdnest.com/Songbird/DeviceManager;2", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDevice> device;
  rv = deviceManager->GetDeviceForItem(aDestLibrary, getter_AddRefs(device));
  if (NS_SUCCEEDED(rv) && device) {
    toDevice = true;
  }

  const PRUint32 allMediaTypes = sbIDeviceLibrarySyncDiff::SYNC_TYPE_AUDIO |
                                 sbIDeviceLibrarySyncDiff::SYNC_TYPE_VIDEO;

  nsRefPtr<SyncEnumListenerBase> listener;

  if (toDevice) {
    listener = new SyncExportEnumListener();
    NS_ENSURE_TRUE(listener, NS_ERROR_OUT_OF_MEMORY);
  }
  else {
    listener = new SyncImportEnumListener();
    NS_ENSURE_TRUE(listener, NS_ERROR_OUT_OF_MEMORY);
  }

  rv = listener->Init(PR_TRUE,
                      aPlaylistMatchingUsesOriginGUID,
                      aSourceLibrary,
                      aDestLibrary);
  NS_ENSURE_SUCCESS(rv, rv);
  listener->SetMediaTypes(allMediaTypes);

  if (aSourceList) {
    // Enumerate the list into our hashmap
    aSourceList->EnumerateAllItems(listener,
                                   sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
    NS_ENSURE_SUCCESS(rv, rv);

    // And add the list itself.
    rv = listener->ProcessItem(aSourceLibrary, aSourceList);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    PRUint32 itemsLen;
    rv = aSourceItems->GetLength(&itemsLen);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 i = 0; i < itemsLen; i++) {
      nsCOMPtr<sbIMediaItem> item = do_QueryElementAt(aSourceItems, i, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = listener->ProcessItem(aSourceLibrary, item);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  rv = listener->Finish();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aChangeset = listener->mChangeset);

  return NS_OK;
}

