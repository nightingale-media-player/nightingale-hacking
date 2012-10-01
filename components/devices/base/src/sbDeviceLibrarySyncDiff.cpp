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

//    Sync2 core implementation.
// 
// The 'Sync2' behaviour is driven by changesets (sbILibraryChangeset) created
// by the classes in this file. These describe the changes to be made to the
// target library. The changeset interfaces are somewhat limited - we've only
// added to the interface where needed to implement the current desired
// behaviour, which is somewhat constrained by the existing UI - e.g. there is
// no mechanism currently to select multiple playlists simultaneously, so drag
// and drop of multiple playlists is not currently implemented here. This sort
// of thing could be added fairly easily if desired.
//
// The changesets can be applied to devices via the sbIDevice.importFromDevice
// and sbIDevice.exportToDevice methods. In certain cases callers may wish to
// directly examine the contents of the changeset also.
//
// The public functions here are implemented in sbDeviceLibrarySyncDiff, the
// implementation of sbIDeviceLibrarySyncDiff. This drives the collection of
// objects to add/modify/etc. The bulk of the decision logic exists in the
// enumeration listeners (SyncDiffExportListener or SyncDiffImportListener); see
// those if you wish to change behaviour of what we do with a particular item.
//
// Comments here try to describe what behaviour we're implementing, but for
// information about _why_ we've chosen the particular behaviour at hand, the
// reader should instead see:
//   http://wiki.songbirdnest.com/Releases/Ratatat/Device_Import_and_Sync
// In particular, the four attached images on that page show the decision in
// a relatively easily understood form.


// Library enumeration listener base class to collect items to sync
// This implements a variety of useful methods and basic functionality that is
// common to both export and import, but delegates all the actual
// decision-making about what to do for any given item to pure-virtual methods
// implemented in subclasses for import/export.
class SyncEnumListenerBase:
    public sbIMediaListEnumerationListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER

  enum DropAction
  {
    NOT_DROP,
    DROP_TRACKS,
    DROP_LIST
  };
  // Mode for enumeration - handle all items, only non-list items, or only
  // list items.
  enum HandleMode {
    HANDLE_ALL,
    HANDLE_ITEMS,
    HANDLE_LISTS
  };

  // Type of change. This is used to indicate what action for sync behaviour
  // is desired for a single item.
  enum ChangeType {
    CHANGE_NONE,    // Item has been ignored.
    CHANGE_ADD,     // Item should be added to the destination
    CHANGE_CLOBBER, // Item should be used to replace the existing destination
    CHANGE_RETAIN   // Existing destination item should be used
  };


  SyncEnumListenerBase() :
    mMediaTypes(0),
    mDropAction(NOT_DROP),
    mHandleMode(HANDLE_ALL)
  {
  }

  virtual nsresult Init(DropAction aDropAction,
                        sbILibrary *aMainLibrary,
                        sbILibrary *aDeviceLibrary);

  // Process any item (list or non-list), and act on it appropriately. This
  // will add any desired changes to the computed changeset object.
  virtual nsresult ProcessItem(sbIMediaList *aMediaList,
                               sbIMediaItem *aMediaItem) = 0;

  // Process a non-list item, determining what to do with it. Does not modify
  // any state. This allows a caller to determine the behaviour desired without
  // necessarily adding that action to the changeset.
  virtual nsresult SelectChangeForItem(sbIMediaItem *aMediaItem,
                                       ChangeType *aChangeType,
                                       sbIMediaItem **aDestMediaItem) = 0;

  // Process a list item, determining what to do with it. Does not modify
  // any state. As above, allows a caller to determine desired behaviour.
  virtual nsresult SelectChangeForList(sbIMediaList *aMediaList,
                                       ChangeType *aChangeType,
                                       sbIMediaList **aDestMediaList) = 0;

  // Add a change (sbILibraryChange) to the changeset we're computing.
  // If aSrcItem is a list, aListItems should contain the items we're adding to
  // the list. See the documentation for sbILibraryChange.listItems for details
  // on the semantics of this.
  nsresult AddChange(PRUint32 aChangeType,
                     sbIMediaItem *aSrcItem,
                     sbIMediaItem *aDstItem,
                     nsIArray *aListItems = NULL);

  // Add a list change. Computes the items that need to go into the destination
  // list according to the semantics of sbILibraryChange.listItems.
  nsresult AddListChange(PRUint32 aChangeType,
                         sbIMediaList *aSrcList,
                         sbIMediaList *aDstList);

  // Set the media types to process. aMediaType is a bitfield, so multiple
  // types may be selected. Any item NOT of a type indicated by a call to this
  // function will be ignored - that is, the SelectChangeFor*() functions will
  // return CHANGE_NONE for all such items.
  void SetMediaTypes(PRUint32 aMediaTypes) {
    mMediaTypes = aMediaTypes;
  }

  // Set the mode for enumerating over the library - whether we're processing
  // lists, non-lists, or both.
  void SetHandleMode(HandleMode aMode) {
    mHandleMode = aMode;
  }

  // Call to finish processing. This will create the result mChangeset member.
  // After calling this, no other functions should be called on this object
  // (note that this constraint is not currently checked internally).
  nsresult Finish();

protected:
  virtual ~SyncEnumListenerBase() {}

  // Helper function to create array of sbIPropertyChange objects for an
  // item being added to the destination.
  nsresult CreatePropertyChangesForItemAdded(sbIMediaItem *aSourceItem,
                                             nsIArray **aPropertyChanges);
  // Helper function to create array of sbIPropertyChange objects for an
  // item being modified.
  nsresult CreatePropertyChangesForItemModified(
                                sbIMediaItem *aSourceItem,
                                sbIMediaItem *aDestinationItem,
                                nsIArray **aPropertyChanges);

  // Get a property on a media item and convert to a 64 bit integer; this is
  // useful for our time properties such as the last modified time
  // (SB_PROPERTY_UPDATED).
  nsresult GetTimeProperty(sbIMediaItem *aMediaItem,
                           nsString aPropertyName,
                           PRInt64 *_result);

  // Get a playlist from aLibrary that matches aList. Returns null in
  // aMatchingList is not found. Matching is done based on GUID, but since
  // origin GUIDs are one-way, we need to implement this differently for export
  // and import.
  virtual nsresult GetMatchingPlaylist(
        sbILibrary *aLibrary,
        sbIMediaList *aList,
        sbIMediaList **aMatchingList) = 0;

  // Returns true if the item aItem has a content type that we're currently
  // processing.
  bool HasCorrectContentType(sbIMediaItem *aItem);
  // Returns true is aList is a mixed-content (audio and video) playlist.
  bool ListIsMixed(sbIMediaList *aList);
  // Returns true if we should process aList based on the current settings
  // for content types to process.
  bool ListHasCorrectContentType(sbIMediaList *aList);

  bool IsDrop() const
  {
    return mDropAction != NOT_DROP;
  }
  PRUint32   mMediaTypes;
  DropAction mDropAction;
  HandleMode mHandleMode;

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

// Enumerator used to determine which items to add to a destination playlist
// when adding or clobbering a list. This builds an array (in mListItems) that
// matches the semantics of sbILibraryChange.listItems.
class ListAddingEnumerationListener:
    public sbIMediaListEnumerationListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER

  // Create an enumeration listener. aMainListener is the import or export
  // listener used to determine whether the source item, destination item,
  // or neither is added to aListItems. The caller should then call
  // sourceList.enumerateAllItems() with this listener.
  ListAddingEnumerationListener(SyncEnumListenerBase *aMainListener,
                                nsIMutableArray *aListItems):
      mMainListener(aMainListener),
      mListItems(aListItems)
  {
  };

  nsCOMPtr<SyncEnumListenerBase> mMainListener;
  nsCOMPtr<nsIMutableArray> mListItems;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(ListAddingEnumerationListener,
                              sbIMediaListEnumerationListener)

NS_IMETHODIMP
ListAddingEnumerationListener::OnEnumerationBegin(sbIMediaList *aMediaList,
                                                  PRUint16 *_retval)
{
  *_retval = sbIMediaListEnumerationListener::CONTINUE;

  return NS_OK;
}

NS_IMETHODIMP
ListAddingEnumerationListener::OnEnumeratedItem(sbIMediaList *aMediaList,
                                                sbIMediaItem *aMediaItem,
                                                PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  SyncEnumListenerBase::ChangeType changeType;
  nsCOMPtr<sbIMediaItem> destItem;
  rv = mMainListener->SelectChangeForItem(aMediaItem,
                                          &changeType, 
                                          getter_AddRefs(destItem));
  NS_ENSURE_SUCCESS(rv, rv);

  switch(changeType) {
    case SyncEnumListenerBase::CHANGE_ADD:
    case SyncEnumListenerBase::CHANGE_CLOBBER:
      // For both of these, use the source item
      rv = mListItems->AppendElement(aMediaItem, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    case SyncEnumListenerBase::CHANGE_RETAIN:
      // For this one, use the (existing) destination item
      rv = mListItems->AppendElement(destItem, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    default:
      // And this means it was an item we chose not to transfer at all.
      break;
  }

  *_retval = sbIMediaListEnumerationListener::CONTINUE;

  return NS_OK;
}

NS_IMETHODIMP
ListAddingEnumerationListener::OnEnumerationEnd(sbIMediaList *aMediaList,
                                                nsresult aStatusCode)
{
  return NS_OK;
}

nsresult
SyncEnumListenerBase::Init(DropAction aDropAction,
                           sbILibrary *aMainLibrary,
                           sbILibrary *aDeviceLibrary)
{
  nsresult rv;

  mDropAction = aDropAction;

  mMainLibrary = aMainLibrary;
  mDeviceLibrary = aDeviceLibrary;

  NS_NEWXPCOM(mChangeset, sbLibraryChangeset);
  NS_ENSURE_TRUE(mChangeset, NS_ERROR_OUT_OF_MEMORY);

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

  nsCOMPtr<sbIMediaList> itemAsList = do_QueryInterface(aMediaItem, &rv);
  bool isList = NS_SUCCEEDED(rv);

  // Skip things that aren't what we're trying to handle currently.
  if ((mHandleMode == HANDLE_LISTS && !isList) ||
      (mHandleMode == HANDLE_ITEMS && isList))
  {
    *_retval = sbIMediaListEnumerationListener::CONTINUE;
    return NS_OK;
  }

  // First: is this something we've already added? Ignore if so.
  nsString itemId;
  rv = aMediaItem->GetGuid(itemId);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mSeenMediaItems.GetEntry(itemId)) {
    *_retval = sbIMediaListEnumerationListener::CONTINUE;
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
    // Finally, we've decided we're actually going to process an item - so go
    // ahead. This will add the appropriate change (if any) to the changeset.
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

  // Content URL for source and destination will never bee the same
  successHashkey =
    propertyExclusionList.PutEntry(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL));
  NS_ENSURE_TRUE(successHashkey, NS_ERROR_OUT_OF_MEMORY);

  // Sadly we can't use content length because on a device the length may be
  // different
  successHashkey =
    propertyExclusionList.PutEntry(NS_LITERAL_STRING(SB_PROPERTY_CONTENTLENGTH));
  NS_ENSURE_TRUE(successHashkey, NS_ERROR_OUT_OF_MEMORY);

  successHashkey =
    propertyExclusionList.PutEntry(NS_LITERAL_STRING(SB_PROPERTY_ORIGIN_IS_IN_MAIN_LIBRARY));
  NS_ENSURE_TRUE(successHashkey, NS_ERROR_OUT_OF_MEMORY);

  // Playlist URL will never be the same for ML and DL
  successHashkey =
    propertyExclusionList.PutEntry(NS_LITERAL_STRING(SB_PROPERTY_PLAYLISTURL));
  NS_ENSURE_TRUE(successHashkey, NS_ERROR_OUT_OF_MEMORY);

  // The availability property is just so we don't display items on the
  // the device that haven't been copied so we can ignore this
  successHashkey =
    propertyExclusionList.PutEntry(NS_LITERAL_STRING(SB_PROPERTY_AVAILABILITY));
  NS_ENSURE_TRUE(successHashkey, NS_ERROR_OUT_OF_MEMORY);

  // Column spec property may be different or absent between ML and DL and should
  // be ignored
  successHashkey =
    propertyExclusionList.PutEntry(NS_LITERAL_STRING(SB_PROPERTY_COLUMNSPEC));
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
      // content type defaults to audio if not present
      if (propertyId.Equals(NS_LITERAL_STRING(SB_PROPERTY_CONTENTTYPE)) &&
          propertyValue.Equals(NS_LITERAL_STRING("audio"))) {
        continue;
      }
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

      // Check if the duration is the same in seconds, that's good enough
      if (propertyId.EqualsLiteral(SB_PROPERTY_DURATION)) {
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
SyncEnumListenerBase::AddListChange(PRUint32 aChangeType,
                                    sbIMediaList *aSrcList,
                                    sbIMediaList *aDstList)
{
  nsresult rv;

  nsCOMPtr<nsIMutableArray> listItems =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);

  // Now we want to enumerate aSrcList, and for each item, determine a
  // corresponding item (or null). If we determine any item, add it to
  // listItems.
  nsRefPtr<ListAddingEnumerationListener> listener =
      new ListAddingEnumerationListener(this, listItems);

  aSrcList->EnumerateAllItems(listener,
                              sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> srcItem = do_QueryInterface(aSrcList, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> dstItem;
  if (aDstList) {
    dstItem = do_QueryInterface(aDstList, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = AddChange(aChangeType,
                 srcItem,
                 dstItem,
                 listItems);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
SyncEnumListenerBase::AddChange(PRUint32 aChangeType,
                                sbIMediaItem *aSrcItem,
                                sbIMediaItem *aDstItem,
                                nsIArray *aListItems)
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
                                     aListItems);
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

class SyncExportEnumListener : public SyncEnumListenerBase
{
public:

  SyncExportEnumListener() {}

  virtual nsresult ProcessItem(sbIMediaList *aMediaList,
                               sbIMediaItem *aMediaItem);

  virtual nsresult SelectChangeForItem(sbIMediaItem *aMediaItem,
                                       ChangeType *aChangeType,
                                       sbIMediaItem **aDestMediaItem);
  virtual nsresult SelectChangeForList(sbIMediaList *aMediaList,
                                       ChangeType *aChangeType,
                                       sbIMediaList **aDestMediaList);

protected:
  nsresult GetItemWithOriginGUID(sbILibrary    *aDeviceLibrary,
                                 nsString       aItemID,
                                 sbIMediaItem **aMediaItem);

  virtual nsresult GetMatchingPlaylist(
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
SyncExportEnumListener::GetMatchingPlaylist(
        sbILibrary *aLibrary,
        sbIMediaList *aList,
        sbIMediaList **aMatchingList)
{
  nsresult rv;

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
    if (ListIsMixed(itemAsList)) {
      // We have to do some absurd things with mixed-content playlists; keep
      // track of them.
      nsCOMPtr<sbIMediaList>* element =
          mMixedContentPlaylists.AppendElement(itemAsList);
      NS_ENSURE_TRUE(element, NS_ERROR_OUT_OF_MEMORY);
    }

    ChangeType changeType = CHANGE_NONE;
    nsCOMPtr<sbIMediaList> destList;
    rv = SelectChangeForList(itemAsList, &changeType, getter_AddRefs(destList));
    NS_ENSURE_SUCCESS(rv, rv);

    switch (changeType) {
      case CHANGE_ADD:
        rv = AddListChange(sbIChangeOperation::ADDED,
                           itemAsList,
                           NULL);
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      case CHANGE_CLOBBER:
        rv = AddListChange(sbIChangeOperation::MODIFIED,
                           itemAsList,
                           destList);
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      default:
        // Others we do nothing with
        break;
    }
  }
  else {
    // Normal media item.
    ChangeType changeType = CHANGE_NONE;
    nsCOMPtr<sbIMediaItem> destItem;
    rv = SelectChangeForItem(aMediaItem, &changeType, getter_AddRefs(destItem));
    NS_ENSURE_SUCCESS(rv, rv);

    switch (changeType) {
      case CHANGE_ADD:
        rv = AddChange(sbIChangeOperation::ADDED,
                       aMediaItem,
                       NULL);
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      case CHANGE_CLOBBER:
        rv = AddChange(sbIChangeOperation::MODIFIED,
                       aMediaItem,
                       destItem);
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      default:
        // Others we do nothing with
        break;
    }
  }

  return NS_OK;
}

nsresult
SyncExportEnumListener::SelectChangeForList(sbIMediaList *aMediaList,
                                            ChangeType *aChangeType,
                                            sbIMediaList **aDestMediaList)
{
  nsresult rv;

  if (!ListHasCorrectContentType(aMediaList)) {
    *aChangeType = CHANGE_NONE;
    return NS_OK;
  }

  // We have a list. We only need to sync the list itself. If we're syncing
  // all content we'll enumerate the items anyway. If we're syncing individual
  // playlists, we'll explicitly enumerate each of those.
  nsCOMPtr<sbIMediaList> matchingPlaylist;
  rv = GetMatchingPlaylist(mDeviceLibrary,
                           aMediaList,
                           getter_AddRefs(matchingPlaylist));
  NS_ENSURE_SUCCESS(rv, rv);

  if (matchingPlaylist) {
    // Check sync time vs. last modified time.
    PRInt64 itemLastModifiedTime;
    PRInt64 lastSyncTime;
    rv = aMediaList->GetUpdated(&itemLastModifiedTime);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = GetTimeProperty(mDeviceLibrary,
                         NS_LITERAL_STRING(SB_PROPERTY_LAST_SYNC_TIME),
                         &lastSyncTime);

    if (NS_SUCCEEDED(rv) && itemLastModifiedTime > lastSyncTime) {
      // Ok, we have a match - clobber the target item.
      *aChangeType = CHANGE_CLOBBER;
    }
    else {
      // Otherwise, the playlist hasn't been modified since the last sync, or
      // we couldn't find the last sync time; don't do anything.
      *aChangeType = CHANGE_RETAIN;
    }

    matchingPlaylist.forget(aDestMediaList);
  }
  else {
    // No match found; add a new item.
    *aChangeType = CHANGE_ADD;
  }
  return NS_OK;
}


nsresult
SyncExportEnumListener::SelectChangeForItem(sbIMediaItem *aMediaItem,
                                            ChangeType *aChangeType,
                                            sbIMediaItem **aDestMediaItem)
{
  nsresult rv;

  if (!HasCorrectContentType(aMediaItem)) {
    *aChangeType = CHANGE_NONE;
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

  if (mDropAction == DROP_TRACKS) {
    // Drag-and drop case is simple: if we have a matching origin item,
    // overwrite. Otherwise, add a new one.
    if (hasOriginMatch) {
      *aChangeType = CHANGE_CLOBBER;
      destMediaItem.forget(aDestMediaItem);
      return NS_OK;
    }
    else {
      *aChangeType = CHANGE_ADD;
      return NS_OK;
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
        *aChangeType = CHANGE_CLOBBER;
        destMediaItem.forget(aDestMediaItem);

        return NS_OK;
      }
      else {
        // No changes needed; we just point at the existing matched item.
        *aChangeType = CHANGE_RETAIN;
        destMediaItem.forget(aDestMediaItem);

        return NS_OK;
      }
    }
    else {
      // We don't have a definite (OriginGUID based) match, how about an
      // identity (hash) based match?
      nsCOMPtr<nsIArray> matchedItems;
      rv = mDeviceLibrary->GetItemsWithSameIdentity(
              aMediaItem,
              getter_AddRefs(matchedItems));
      NS_ENSURE_SUCCESS(rv, rv);

      PRUint32 matchedItemsLength;
      rv = matchedItems->GetLength(&matchedItemsLength);
      NS_ENSURE_SUCCESS(rv, rv);

      if (matchedItemsLength == 0) {
        // Ok, no match - appears to be an all-new file. Copy it as a new item
        // to the destination library.
        *aChangeType = CHANGE_ADD;
        return NS_OK;
      }
      else {
        // Otherwise: looks like it's probably a dupe, but we're not sure, so...
        // do nothing! Just point at the first thing we matched.
        *aChangeType = CHANGE_RETAIN;

        nsCOMPtr<sbIMediaItem> matchItem = do_QueryElementAt(
                matchedItems, 0, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        matchItem.forget(aDestMediaItem);
        return NS_OK;
      }
    }
  }
}

// Library enumeration listener to collect items to import from a device
class SyncImportEnumListener : public SyncEnumListenerBase
{
public:

  SyncImportEnumListener() {}

  virtual nsresult ProcessItem(sbIMediaList *aMediaList,
                               sbIMediaItem *aMediaItem);
  virtual nsresult SelectChangeForItem(sbIMediaItem *aMediaItem,
                                       ChangeType *aChangeType,
                                       sbIMediaItem **aDestMediaItem);
  virtual nsresult SelectChangeForList(sbIMediaList *aMediaList,
                                       ChangeType *aChangeType,
                                       sbIMediaList **aDestMediaList);

protected:
  virtual ~SyncImportEnumListener() { }

  nsresult IsFromMainLibrary(sbIMediaItem *aMediaItem,
                             PRBool       *aFromMainLibrary);
  nsresult GetItemInMainLibrary(sbIMediaItem *aMediaItem,
                                sbIMediaItem **aMainLibraryItem);
  nsresult GetSimplePlaylistWithSameName(sbILibrary *aLibrary,
                                         sbIMediaList *aList,
                                         sbIMediaList **aMatchingList);

  virtual nsresult GetMatchingPlaylist(
        sbILibrary *aLibrary,
        sbIMediaList *aList,
        sbIMediaList **aMatchingList);
};

nsresult
SyncImportEnumListener::GetSimplePlaylistWithSameName(
        sbILibrary *aLibrary,
        sbIMediaList *aList,
        sbIMediaList **aMatchingList)

{
  nsresult rv;

  // Find a list with type 'simple' and a name matching aList's name from
  // library 'aLibrary'. There might be multiple matches; in that case just
  // return the first.
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
    return NS_OK;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 itemsLength;
  rv = items->GetLength(&itemsLength);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < itemsLength; i++) {
    nsCOMPtr<sbIMediaList> list = do_QueryElementAt(items, 0, &rv);
    if (NS_FAILED(rv))
      continue;

    nsString playlistType;
    rv = list->GetType(playlistType);
    NS_ENSURE_SUCCESS(rv, rv);

    if (playlistType.EqualsLiteral("simple")) {
      // Found one; return it. We don't check to see if there might be multiple
      // matches.
      list.forget(aMatchingList);
      return NS_OK;
    }
  }

  // Not found
  *aMatchingList = NULL;
  return NS_OK;
}

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
SyncImportEnumListener::GetItemInMainLibrary(sbIMediaItem *aMediaItem,
                                             sbIMediaItem **aMainLibraryItem)
{
  // Is the origin item actually IN the main library? This is assumed to only
  // be called if we already know that the origin points at the main library.
  // Returns the object if found.
  nsresult rv;

  nsString originItemGUID;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID),
                               originItemGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> item;
  rv = mMainLibrary->GetMediaItem(originItemGUID, getter_AddRefs(item));

  if (NS_SUCCEEDED(rv) && item) {
    item.forget(aMainLibraryItem);
  }

  return NS_OK;
}

nsresult
SyncImportEnumListener::GetMatchingPlaylist(sbILibrary *aLibrary,
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
  // Possible we might not find the original item return null then
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    *aMatchingList = nsnull;
    return NS_OK;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(matchingItem.get(), aMatchingList);
}

nsresult
SyncImportEnumListener::SelectChangeForList(sbIMediaList *aMediaList,
                                            ChangeType *aChangeType,
                                            sbIMediaList **aDestMediaList)
{
  nsresult rv;

  if (!ListHasCorrectContentType(aMediaList)) {
    *aChangeType = CHANGE_NONE;
    return NS_OK;
  }

  nsCOMPtr<sbIMediaList> matchingPlaylist;
  rv = GetMatchingPlaylist(mMainLibrary,
                           aMediaList,
                           getter_AddRefs(matchingPlaylist));
  NS_ENSURE_SUCCESS(rv, rv);

  if (matchingPlaylist) {
    nsString playlistType;
    rv = matchingPlaylist->GetType(playlistType);
    NS_ENSURE_SUCCESS(rv, rv);

    // We only import changes for simple playlists.
    if (playlistType.EqualsLiteral("simple")) {
      if (IsDrop()) {
        *aChangeType = CHANGE_CLOBBER;
        matchingPlaylist.forget(aDestMediaList);

        return NS_OK;
      }
      else {
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
          *aChangeType = CHANGE_CLOBBER;
          matchingPlaylist.forget(aDestMediaList);

          return NS_OK;
        }
        else {
          *aChangeType = CHANGE_RETAIN;
          matchingPlaylist.forget(aDestMediaList);

          return NS_OK;
        }
      }
    }
    else {
      // Matching list is a smart playlist; we only do anything with these
      // for d&d.
      if (IsDrop()) {
        nsCOMPtr<sbIMediaList> matchingSimplePlaylist;
        rv = GetSimplePlaylistWithSameName(
                mMainLibrary,
                aMediaList,
                getter_AddRefs(matchingSimplePlaylist));
        NS_ENSURE_SUCCESS(rv, rv);

        if (matchingSimplePlaylist) {
          *aChangeType = CHANGE_CLOBBER;
          matchingSimplePlaylist.forget(aDestMediaList);

          return NS_OK;
        }
        else {
          *aChangeType = CHANGE_ADD;
          return NS_OK;
        }
      }
      else {
        *aChangeType = CHANGE_RETAIN;
        matchingPlaylist.forget(aDestMediaList);
        return NS_OK;
      }
    }
  }
  else {
    // New playlist; add it to the main library
    *aChangeType = CHANGE_ADD;
    return NS_OK;
  }
}

nsresult
SyncImportEnumListener::SelectChangeForItem(sbIMediaItem *aMediaItem,
                                            ChangeType *aChangeType,
                                            sbIMediaItem **aDestMediaItem)
{
  nsresult rv;

  // Is it of the correct type?
  if (!HasCorrectContentType(aMediaItem)) {
    *aChangeType = CHANGE_NONE;
    return NS_OK;
  }

  // If it came from the main library (according to origin item and library
  // guids), we do not wish to re-import it (even if the linked item is no
  // longer in the main library!).
  PRBool isFromMainLibrary;
  rv = IsFromMainLibrary(aMediaItem, &isFromMainLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  if (IsDrop()) {
    // Drag-and-drop case.
    if (isFromMainLibrary) {
      // Ok, it's from the main library. Is it still IN the main library?
      nsCOMPtr<sbIMediaItem> mainLibItem;
      rv = GetItemInMainLibrary(aMediaItem, getter_AddRefs(mainLibItem));
      NS_ENSURE_SUCCESS(rv, rv);

      if (!mainLibItem) {
        // It's no longer in the main library, but the user dragged it there,
        // so take that to mean "re-add this"
        *aChangeType = CHANGE_ADD;
        return NS_OK;
      }
      else {
        *aChangeType = CHANGE_RETAIN;
        mainLibItem.forget(aDestMediaItem);
        return NS_OK;
      }
    }
    else {
      // Not from the main library; add it.
      *aChangeType = CHANGE_ADD;
      return NS_OK;
    }
  }
  else {
    // Sync case.
    if (!isFromMainLibrary) {
      // Didn't come from Songbird (at least not from this profile on this
      // machine). Is there something that _looks_ like the same item? If
      // there is, we don't want to import it.
      nsCOMPtr<nsIArray> matchedItems;
      rv = mMainLibrary->GetItemsWithSameIdentity(
              aMediaItem,
              getter_AddRefs(matchedItems));
      NS_ENSURE_SUCCESS(rv, rv);

      PRUint32 matchedItemsLength;
      rv = matchedItems->GetLength(&matchedItemsLength);
      NS_ENSURE_SUCCESS(rv, rv);

      if (matchedItemsLength == 0) {
        // It looks like an all-new item not present in the main library. Time
        // to actually import it!
        *aChangeType = CHANGE_ADD;
        return NS_OK;
      }
      else {
        // Point at the object we matched (might be several, that's ok...)
        *aChangeType = CHANGE_RETAIN;

        nsCOMPtr<sbIMediaItem> matchItem = do_QueryElementAt(
                matchedItems, 0, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        matchItem.forget(aDestMediaItem);
        return NS_OK;
      }
    }
    else {
      // It came from the main library. Do nothing (even if it's not IN the main
      // library any more). However, point at the main library item, if present.
      nsCOMPtr<sbIMediaItem> mainLibItem;
      rv = GetItemInMainLibrary(aMediaItem, getter_AddRefs(mainLibItem));
      NS_ENSURE_SUCCESS(rv, rv);

      if (mainLibItem) {
        *aChangeType = CHANGE_RETAIN;
        mainLibItem.forget(aDestMediaItem);
        return NS_OK;
      }
      else {
        *aChangeType = CHANGE_NONE;
        return NS_OK;
      }
    }
  }
}

nsresult
SyncImportEnumListener::ProcessItem(sbIMediaList *aMediaList,
                                    sbIMediaItem *aMediaItem)
{
  nsresult rv;

  nsCOMPtr<sbIMediaList> itemAsList = do_QueryInterface(aMediaItem, &rv);
  if (NS_SUCCEEDED(rv)) {
    ChangeType changeType = CHANGE_NONE;
    nsCOMPtr<sbIMediaList> destList;
    rv = SelectChangeForList(itemAsList, &changeType, getter_AddRefs(destList));
    NS_ENSURE_SUCCESS(rv, rv);

    switch (changeType) {
      case CHANGE_ADD:
        rv = AddListChange(sbIChangeOperation::ADDED,
                           itemAsList,
                           NULL);
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      case CHANGE_CLOBBER:
        rv = AddListChange(sbIChangeOperation::MODIFIED,
                           itemAsList,
                           destList);
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      default:
        // Others we do nothing with
        break;
    }
  }
  else {
    // Normal media item.
    ChangeType changeType = CHANGE_NONE;
    nsCOMPtr<sbIMediaItem> destItem;
    rv = SelectChangeForItem(aMediaItem, &changeType, getter_AddRefs(destItem));
    NS_ENSURE_SUCCESS(rv, rv);

    switch (changeType) {
      case CHANGE_ADD:
        rv = AddChange(sbIChangeOperation::ADDED,
                       aMediaItem,
                       NULL);
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      case CHANGE_CLOBBER:
        rv = AddChange(sbIChangeOperation::MODIFIED,
                       aMediaItem,
                       destItem);
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      default:
        // Others we do nothing with
        break;
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
        PRUint32 aMediaTypesToExportAll,
        PRUint32 aMediaTypesToImportAll,
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

  nsresult rv;

  // We first determine which (if any) items we need to export, depending on
  // settings.

  nsRefPtr<SyncExportEnumListener> exportListener =
      new SyncExportEnumListener();
  NS_ENSURE_TRUE(exportListener, NS_ERROR_OUT_OF_MEMORY);
  rv = exportListener->Init(SyncEnumListenerBase::NOT_DROP,
                            aSourceLibrary,
                            aDestLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  const PRUint32 mixedMediaTypes = sbIDeviceLibrarySyncDiff::SYNC_TYPE_AUDIO |
                                   sbIDeviceLibrarySyncDiff::SYNC_TYPE_VIDEO;

  if (aMediaTypesToExportAll) {
    // Enumerate all items to find the ones we need to sync (based on media
    // type, and what's already on the device). This includes playlists.
    // This will find all the playlists (including mixed-content ones!), as well
    // as media items, that match the desired media type. However, it won't find
    // items that are in a mixed-content playlist, but not of the right type.
    exportListener->SetMediaTypes(aMediaTypesToExportAll);

    // Handle all non-list items.
    exportListener->SetHandleMode(SyncEnumListenerBase::HANDLE_ITEMS);
    rv = aSourceLibrary->EnumerateAllItems(
            exportListener,
            sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
    NS_ENSURE_SUCCESS(rv, rv);

    // Handle all lists - this might need to refer to items above, so we have
    // to do it separately.
    exportListener->SetHandleMode(SyncEnumListenerBase::HANDLE_LISTS);
    rv = aSourceLibrary->EnumerateAllItems(
            exportListener,
            sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
    NS_ENSURE_SUCCESS(rv, rv);

    // If we have any mixed-content playlists, but are not syncing all media
    // types, now do a 2nd pass over those, but with the media types set to all.
    if (aMediaTypesToExportAll != mixedMediaTypes) {
      exportListener->SetHandleMode(SyncEnumListenerBase::HANDLE_ITEMS);
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

  if (aSourceLists) {
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

      // We only want to enumerate the items in the list if we're going to
      // export the list itself - so we need to determine that first.
      SyncEnumListenerBase::ChangeType changeType;
      nsCOMPtr<sbIMediaList> destList;
      rv = exportListener->SelectChangeForList(list,
                                               &changeType,
                                               getter_AddRefs(destList));
      NS_ENSURE_SUCCESS(rv, rv);

      // If we're actually importing the playlist, enumerate everything in it.
      if (changeType == SyncEnumListenerBase::CHANGE_ADD ||
          changeType == SyncEnumListenerBase::CHANGE_CLOBBER) {
        list->EnumerateAllItems(exportListener,
                                sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
        NS_ENSURE_SUCCESS(rv, rv);

        // And add the list itself.
        if (changeType == SyncEnumListenerBase::CHANGE_ADD) {
          rv = exportListener->AddListChange(sbIChangeOperation::ADDED,
                                             list,
                                             NULL);
          NS_ENSURE_SUCCESS(rv, rv);
        }
        else {
          rv = exportListener->AddListChange(sbIChangeOperation::MODIFIED,
                                             list,
                                             destList);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
    }
  }

  rv = exportListener->Finish();
  NS_ENSURE_SUCCESS(rv, rv);

  // All done with export. Now import; this is a little simpler as we don't
  // have as many configuration options available.
  nsRefPtr<SyncImportEnumListener> importListener =
      new SyncImportEnumListener();
  NS_ENSURE_TRUE(importListener, NS_ERROR_OUT_OF_MEMORY);
  rv = importListener->Init(SyncEnumListenerBase::NOT_DROP,
                            aSourceLibrary,
                            aDestLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aMediaTypesToImportAll) {
    // We always import everything (not just select playlists) if this is
    // enabled. As for export, this needs to handle the items before the lists,
    // so that exporting a list can refer to the correct items contained within
    // the list.
    importListener->SetMediaTypes(aMediaTypesToImportAll);
    importListener->SetHandleMode(SyncEnumListenerBase::HANDLE_ITEMS);
    rv = aDestLibrary->EnumerateAllItems(
            importListener,
            sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
    NS_ENSURE_SUCCESS(rv, rv);

    importListener->SetHandleMode(SyncEnumListenerBase::HANDLE_LISTS);
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
        sbILibrary *aSourceLibrary,
        sbILibrary *aDestLibrary,
        sbIMediaList *aSourceList,
        nsIArray *aSourceItems,
        nsIArray **aDestItems NS_OUTPARAM,
        sbILibraryChangeset **aChangeset NS_OUTPARAM)
{
  NS_ENSURE_ARG_POINTER (aDestLibrary);
  NS_ENSURE_ARG_POINTER (aChangeset);

  if (!aSourceList)
    NS_ENSURE_ARG_POINTER(aSourceItems);

  nsresult rv;

  // Figure out if we're going to a device or from one. We might think that
  // checking sbILibrary.device for non-null would work - but what we actually
  // get here is the internal library itself, not the device library that wraps
  // that. Only the device library implements the 'device' attribute.
  bool toDevice = false;
  nsCOMPtr<sbIDeviceManager2> deviceManager = do_GetService(
          "@songbirdnest.com/Songbird/DeviceManager;2", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDevice> device;
  rv = deviceManager->GetDeviceForItem(aDestLibrary, getter_AddRefs(device));
  if (NS_SUCCEEDED(rv) && device) {
    toDevice = true;
  }

  // Drag and drop always works for both audio and video.
  const PRUint32 allMediaTypes = sbIDeviceLibrarySyncDiff::SYNC_TYPE_AUDIO |
                                 sbIDeviceLibrarySyncDiff::SYNC_TYPE_VIDEO;

  nsCOMPtr<nsIMutableArray> destItems = do_CreateInstance(
            "@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<SyncEnumListenerBase> listener;

  // Determine if we're droping tracks or a list
  const SyncEnumListenerBase::DropAction dropAction =
    aSourceList ? SyncEnumListenerBase::DROP_LIST :
                  SyncEnumListenerBase::DROP_TRACKS;

  // Figure out whether this is to a device (even if it's from a device as well)
  // or from a device to a non-device. The third possibility (no devices
  // involved at all) is not checked for; the callers must ensure that either
  // source or destination is a device.
  if (toDevice) {
    listener = new SyncExportEnumListener();
    NS_ENSURE_TRUE(listener, NS_ERROR_OUT_OF_MEMORY);
    rv = listener->Init(dropAction,
                        aSourceLibrary,
                        aDestLibrary);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    listener = new SyncImportEnumListener();
    NS_ENSURE_TRUE(listener, NS_ERROR_OUT_OF_MEMORY);
    rv = listener->Init(dropAction,
                        aDestLibrary,
                        aSourceLibrary);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  listener->SetMediaTypes(allMediaTypes);

  if (aSourceList) {
    // Enumerate the list into our hashmap. We can do this before deciding
    // what to do to the playlist itself because D&D of a playlist ALWAYS
    // results in transfer of the list (the only difference is whether it's
    // adding a new list or clobbering an existing one)
    aSourceList->EnumerateAllItems(listener,
                                   sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
    NS_ENSURE_SUCCESS(rv, rv);

    // And add the list itself.
    rv = listener->ProcessItem(aSourceLibrary, aSourceList);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    // Transferring an array of items, not a media list.
    // For this, we just individually check each item. Note that this handles
    // only non-list items.
    PRUint32 itemsLen;
    rv = aSourceItems->GetLength(&itemsLen);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 i = 0; i < itemsLen; i++) {
      nsCOMPtr<sbIMediaItem> item = do_QueryElementAt(aSourceItems, i, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = listener->ProcessItem(aSourceLibrary, item);
      NS_ENSURE_SUCCESS(rv, rv);

      SyncEnumListenerBase::ChangeType changeType;
      nsCOMPtr<sbIMediaItem> destItem;
      rv = listener->SelectChangeForItem(item,
                                         &changeType, 
                                         getter_AddRefs(destItem));
      NS_ENSURE_SUCCESS(rv, rv);

      switch(changeType) {
        case SyncEnumListenerBase::CHANGE_ADD:
        case SyncEnumListenerBase::CHANGE_CLOBBER:
          // For both of these, use the source item
          rv = destItems->AppendElement(item, PR_FALSE);
          NS_ENSURE_SUCCESS(rv, rv);
          break;
        case SyncEnumListenerBase::CHANGE_RETAIN:
          // For this one, use the (existing) destination item
          rv = destItems->AppendElement(destItem, PR_FALSE);
          NS_ENSURE_SUCCESS(rv, rv);
          break;
        default:
          // And this means it was an item we chose not to transfer at all.
          break;
      }
    }
  }

  rv = listener->Finish();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aChangeset = listener->mChangeset);
  NS_ADDREF(*aDestItems = destItems);

  return NS_OK;
}

