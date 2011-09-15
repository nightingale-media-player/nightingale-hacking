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

#include "sbLocalDatabaseDiffingService.h"

#include <vector>
#include <algorithm>

#include "sbLocalDatabaseCID.h"

#include <nsIAppStartupNotifier.h>
#include <nsICategoryManager.h>
#include <nsIClassInfoImpl.h>
#include <nsIURI.h>
#include <nsIMutableArray.h>
#include <nsIObserverService.h>
#include <nsIProgrammingLanguage.h>
#include <nsIStringEnumerator.h>

#include <sbIMediaListView.h>

#include <nsArrayUtils.h>
#include <nsComponentManagerUtils.h>
#include <nsDataHashtable.h>
#include <nsHashKeys.h>
#include <nsTHashtable.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsTArray.h>
#include <nsXPCOMCID.h>

#include <sbIndex.h>
#include <sbLibraryChangeset.h>
#include <sbLibraryUtils.h>
#include <sbPropertiesCID.h>
#include <sbStandardProperties.h>
#include <sbStringUtils.h>

static nsID const NULL_GUID = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0 } };

#ifdef PR_LOGGING
  static PRLogModuleInfo* gsbLocalDatabaseDiffingLog = nsnull;
# define TRACE(args) \
    PR_BEGIN_MACRO \
      if (!gsbLocalDatabaseDiffingLog) \
         gsbLocalDatabaseDiffingLog = PR_NewLogModule("sbLocalDatabaseDiffingService"); \
      PR_LOG(gsbLocalDatabaseDiffingLog, PR_LOG_DEBUG, args); \
    PR_END_MACRO
# define LOG(args) \
    PR_BEGIN_MACRO \
      if (!gsbLocalDatabaseDiffingLog) \
        gsbLocalDatabaseDiffingLog = PR_NewLogModule("sbLocalDatabaseDiffingService"); \
      PR_LOG(gsbLocalDatabaseDiffingLog, PR_LOG_WARN, args); \
    PR_END_MACRO
void LogMediaItem(char const * aMessage, sbIMediaItem * aMediaItem)
{
  nsCOMPtr<nsIURI> uri;
  aMediaItem->GetContentSrc(getter_AddRefs(uri));
  nsString sourceGUID;
  aMediaItem->GetGuid(sourceGUID);
  nsString sourceOriginGUID;
  aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID),
                          sourceOriginGUID);
  nsCString spec;
  if (uri) {
    uri->GetSpec(spec);
  }
  LOG(("%s\n  URL=%s\n  ID=%s\n  Origin=%s",
       aMessage,
       spec.BeginReading(),
       NS_LossyConvertUTF16toASCII(sourceGUID).BeginReading(),
       NS_LossyConvertUTF16toASCII(sourceOriginGUID).BeginReading()));
}
#else
#  define TRACE(args) PR_BEGIN_MACRO /* nothing */ PR_END_MACRO
#  define LOG(args) PR_BEGIN_MACRO /* nothing */ PR_END_MACRO
inline
void LogMediaItem(char const *, sbIMediaItem *)
{

}
#endif

nsString sbGUIDToString(nsID const & aID)
{
  char guidBuffer[NSID_LENGTH];
  aID.ToProvidedString(guidBuffer);
  guidBuffer[NSID_LENGTH - 2] = '\0';
  nsString retval;
  retval.AssignLiteral(guidBuffer + 1);
  return retval;
}

nsID GetItemGUID(sbIMediaItem * aItem)
{
  nsID returnedID;
  nsString itemGUID;
  nsresult rv = aItem->GetGuid(itemGUID);
  NS_ENSURE_SUCCESS(rv, NULL_GUID);
  PRBool success =
    returnedID.Parse(NS_LossyConvertUTF16toASCII(itemGUID).BeginReading());
  NS_ENSURE_TRUE(success, NULL_GUID);

  return returnedID;
}

nsID GetGUIDProperty(sbIMediaItem * aItem, nsAString const & aProperty)
{
  NS_ASSERTION(aItem, "aItem is null");
  NS_ASSERTION(!aProperty.IsEmpty(), "aProperty is empty");
  nsID returnedID;
  nsString guid;
  nsresult rv = aItem->GetProperty(aProperty, guid);
  if (rv == NS_ERROR_NOT_AVAILABLE || guid.IsEmpty()) {
    return NULL_GUID;
  }
  NS_ENSURE_SUCCESS(rv, NULL_GUID);
  PRBool success =
    returnedID.Parse(NS_LossyConvertUTF16toASCII(guid).BeginReading());
  NS_ENSURE_TRUE(success, NULL_GUID);

  return returnedID;
}

/**
 * This class serves to enumerate and collect the ID and origin ID of
 * items
 */
class sbLDBDSEnumerator :
  public sbIMediaListEnumerationListener
{
// Public types
public:

  struct ItemInfo
  {
    static bool CompareID(ItemInfo const & aLeft, ItemInfo const & aRight)
    {
      return aLeft.mID.Equals(aRight.mID) != PR_FALSE;
    }
    enum Action
    {
      ACTION_NONE,
      ACTION_NEW,
      ACTION_DELETE,
      ACTION_UPDATE,
      ACTION_MOVE,
    };
    ItemInfo() : mID(NULL_GUID),
                 mOriginID(NULL_GUID),
                 mAction(ACTION_NONE),
                 mPosition(0)
    {
    }
    nsID mID;
    nsID mOriginID;
    Action mAction;
    PRUint32 mPosition;
  };
  static bool lessThan(nsID const & aLeftID, nsID const & aRightID)
  {
    if (aLeftID.m0 < aRightID.m0) {
      return true;
    }
    else if (aLeftID.m0 > aRightID.m0) {
      return false;
    }

    if (aLeftID.m1 < aRightID.m1) {
      return true;
    }
    else if (aLeftID.m1 > aRightID.m1) {
      return false;
    }

    if (aLeftID.m2 < aRightID.m2) {
      return true;
    }
    else if (aLeftID.m2 > aRightID.m2) {
      return false;
    }

    if (aLeftID.m2 < aRightID.m2) {
      for (PRUint32 index = 0; index < 8; ++index) {
        if (aLeftID.m3[index] < aRightID.m3[index]) {
          return true;
        }
        else if (aLeftID.m3[index] > aRightID.m3[index]) {
          return false;
        }
      }
    }
    return false;
  }
  class IDCompare;
  class OriginIDCompare;
  // We use a vector to conserve memory, since stl based sets and maps are
  // tree based. Sorting the vector gives us comparable speed without the
  // overhead
  typedef std::vector<ItemInfo> ItemInfos;
  typedef nsID const & (*IDExtractorFunc)(ItemInfos::const_iterator);
  typedef sbIndex<nsID, ItemInfos::iterator, IDCompare> IDIndex;
  typedef sbIndex<nsID, ItemInfos::iterator, OriginIDCompare> OriginIDIndex;
  class IDCompare
  {
  public:
    bool operator()(ItemInfos::const_iterator aLeft, nsID const & aID) const
    {
      return lessThan(aLeft->mID, aID);
    }
    bool operator()(nsID const & aID, ItemInfos::const_iterator aRight) const
    {
      return lessThan(aID, aRight->mID);
    }
    bool operator()(ItemInfos::const_iterator aLeft,
                    ItemInfos::const_iterator aRight) const
    {
      return lessThan(aLeft->mID, aRight->mID);
    }
    bool operator()(ItemInfos::iterator aLeft, nsID const & aID) const
    {
      return lessThan(aLeft->mID, aID);
    }
    bool operator()(nsID const & aID, ItemInfos::iterator aRight) const
    {
      return lessThan(aID, aRight->mID);
    }
    bool operator()(ItemInfos::iterator aLeft,
                    ItemInfos::iterator aRight) const
    {
      return lessThan(aLeft->mID, aRight->mID);
    }
  };
  class OriginIDCompare
  {
  public:
    bool operator()(ItemInfos::const_iterator aLeft, nsID const & aID) const
    {
      return lessThan(aLeft->mOriginID, aID);
    }
    bool operator()(nsID const & aID, ItemInfos::const_iterator aRight) const
    {
      return lessThan(aID, aRight->mOriginID);
    }
    bool operator()(ItemInfos::const_iterator aLeft,
                    ItemInfos::const_iterator aRight) const
    {
      return lessThan(aLeft->mOriginID, aRight->mOriginID);
    }

    bool operator()(ItemInfos::iterator aLeft, nsID const & aID) const
    {
      return lessThan(aLeft->mOriginID, aID);
    }
    bool operator()(nsID const & aID, ItemInfos::iterator aRight) const
    {
      return lessThan(aID, aRight->mOriginID);
    }
    bool operator()(ItemInfos::iterator aLeft,
                    ItemInfos::iterator aRight) const
    {
      return lessThan(aLeft->mOriginID, aRight->mOriginID);
    }
  };

public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER

  enum Index {
    INDEX_ID,
    INDEX_ORIGIN,
    INDEXES
  };
  typedef ItemInfos::const_iterator const_iterator;
  typedef ItemInfos::iterator iterator;
  typedef IDIndex::const_iterator ConstIDIterator;
  typedef IDIndex::iterator IDIterator;
  typedef IDIndex::const_iterator ConstOriginIterator;
  typedef IDIndex::iterator OriginIterator;

  sbLDBDSEnumerator();

  void Sort()
  {
    mIDIndex.Build(mItemInfos.begin(),
                   mItemInfos.end());
    mOriginIndex.Build(mItemInfos.begin(),
                       mItemInfos.end());
  }
  IDIterator FindByID(nsID const & aID) {
    return mIDIndex.find(aID);
  }
  ConstIDIterator FindByID(nsID const & aID) const {
    return mIDIndex.find(aID);
  }
  OriginIterator FindByOrigin(nsID const & aID) {
    return mOriginIndex.find(aID);
  }
  ConstOriginIterator FindByOrigin(nsID const & aID) const {
    return mOriginIndex.find(aID);
  }
  const_iterator begin() const
  {
    return mItemInfos.begin();
  }
  iterator begin()
  {
    return mItemInfos.begin();
  }
  const_iterator end() const
  {
    return mItemInfos.end();
  }
  iterator end()
  {
    return mItemInfos.end();
  }

  ConstIDIterator IDBegin() const
  {
    return mIDIndex.begin();
  }
  IDIterator IDBegin()
  {
    return mIDIndex.begin();
  }
  ConstIDIterator IDEnd() const
  {
    return mIDIndex.end();
  }
  IDIterator IDEnd()
  {
    return mIDIndex.end();
  }

  ConstOriginIterator OriginBegin() const
  {
    return mOriginIndex.begin();
  }
  OriginIterator OriginBegin()
  {
    return mOriginIndex.begin();
  }
  ConstOriginIterator OriginEnd() const
  {
    return mOriginIndex.end();
  }
  OriginIterator OriginEnd()
  {
    return mOriginIndex.end();
  }
protected:
  ~sbLDBDSEnumerator();

private:
  ItemInfos mItemInfos;
  IDIndex mIDIndex;
  OriginIDIndex mOriginIndex;
  PRUint32 mItemIndex;
};

//-----------------------------------------------------------------------------
// sbLocalDatabaseDiffingServiceComparator
//-----------------------------------------------------------------------------
NS_IMPL_THREADSAFE_ISUPPORTS1(sbLDBDSEnumerator,
                              sbIMediaListEnumerationListener)

sbLDBDSEnumerator::sbLDBDSEnumerator() :
  mItemIndex(0)
{
}

sbLDBDSEnumerator::~sbLDBDSEnumerator()
{
}

/* unsigned short onEnumerationBegin (in sbIMediaList aMediaList); */
NS_IMETHODIMP
sbLDBDSEnumerator::OnEnumerationBegin(sbIMediaList *aMediaList,
                                      PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(_retval);

  LOG(("Scanning for items"));
  mItemIndex = 0;
  PRUint32 length;
  nsresult rv = aMediaList->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  mItemInfos.reserve(length);

  *_retval = sbIMediaListEnumerationListener::CONTINUE;

  return NS_OK;
}

/* unsigned short onEnumeratedItem (in sbIMediaList aMediaList, in sbIMediaItem aMediaItem); */
NS_IMETHODIMP
sbLDBDSEnumerator::OnEnumeratedItem(sbIMediaList *aMediaList,
                                    sbIMediaItem *aMediaItem,
                                    PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  ItemInfo info;
  info.mID = GetItemGUID(aMediaItem);
  NS_ENSURE_TRUE(!info.mID.Equals(NULL_GUID), NS_ERROR_FAILURE);

  nsString originID;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID),
                               originID);
  if (rv != NS_ERROR_NOT_AVAILABLE) {
    NS_ENSURE_SUCCESS(rv, rv);
  }
  PRBool parsed = PR_FALSE;
  nsID originGUID;
  if (!originID.IsEmpty()) {
   parsed = originGUID.Parse(
                          NS_LossyConvertUTF16toASCII(originID).BeginReading());
  }
  if (parsed) {
    info.mOriginID = originGUID;
  }
  info.mPosition = mItemIndex++;
#ifdef PR_LOGGING
  nsCOMPtr<nsIURI> uri;
  aMediaItem->GetContentSrc(getter_AddRefs(uri));
  nsCString spec;
  if (uri) {
    uri->GetSpec(spec);
  }
  LOG(("  URL=%s\n  ID=%s\n  Origin=%s",
       spec.BeginReading(),
       NS_LossyConvertUTF16toASCII(sbGUIDToString(info.mID)).BeginReading(),
       NS_LossyConvertUTF16toASCII(sbGUIDToString(info.mOriginID)).BeginReading()));
#endif
  mItemInfos.push_back(info);
  *_retval = sbIMediaListEnumerationListener::CONTINUE;

  return NS_OK;
}

/* void onEnumerationEnd (in sbIMediaList aMediaList, in nsresult aStatusCode); */
NS_IMETHODIMP
sbLDBDSEnumerator::OnEnumerationEnd(sbIMediaList *aMediaList,
                                    nsresult aStatusCode)
{
  LOG(("Scanning complete, sorting"));
  Sort();
  return NS_OK;
}

NS_IMPL_THREADSAFE_ADDREF(sbLocalDatabaseDiffingService)
NS_IMPL_THREADSAFE_RELEASE(sbLocalDatabaseDiffingService)

NS_INTERFACE_MAP_BEGIN(sbLocalDatabaseDiffingService)
  NS_IMPL_QUERY_CLASSINFO(sbLocalDatabaseDiffingService)
  NS_INTERFACE_MAP_ENTRY(sbILibraryDiffingService)
  NS_INTERFACE_MAP_ENTRY(nsIClassInfo)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, sbILibraryDiffingService)
NS_INTERFACE_MAP_END

NS_IMPL_CI_INTERFACE_GETTER1(sbLocalDatabaseDiffingService,
                             sbILibraryDiffingService)

NS_DECL_CLASSINFO(sbLocalDatabaseDiffingService)
NS_IMPL_THREADSAFE_CI(sbLocalDatabaseDiffingService)

sbLocalDatabaseDiffingService::sbLocalDatabaseDiffingService()
{
}

sbLocalDatabaseDiffingService::~sbLocalDatabaseDiffingService()
{
}

/*static*/
NS_METHOD sbLocalDatabaseDiffingService::RegisterSelf(
                          nsIComponentManager* aCompMgr,
                          nsIFile* aPath,
                          const char* aLoaderStr,
                          const char* aType,
                          const nsModuleComponentInfo *aInfo)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> categoryManager =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = categoryManager->AddCategoryEntry(APPSTARTUP_CATEGORY,
                                         SB_LOCALDATABASE_DIFFINGSERVICE_DESCRIPTION,
                                         "service," SB_LOCALDATABASE_DIFFINGSERVICE_CONTRACTID,
                                         PR_TRUE,
                                         PR_TRUE,
                                         nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

template <class T, class M>
T FindNextUsable(T aIter, T aEnd, M aMember)
{
  if (aIter != aEnd) {
    nsID const id = (**aIter).*aMember;
    while (aIter != aEnd &&
           (*aIter)->mAction ==
             sbLDBDSEnumerator::ItemInfo::ACTION_UPDATE &&
           ((**aIter).*aMember).Equals(id)) {
      ++aIter;
    }
    if (aIter != aEnd &&
        (!((**aIter).*aMember).Equals(id) ||
          ((*aIter)->mAction == sbLDBDSEnumerator::ItemInfo::ACTION_UPDATE))) {
      return aEnd;
    }
  }
  return aIter;
}

static void
MarkLists(sbLDBDSEnumerator * aSrc, sbLDBDSEnumerator * aDest)
{
  LOG(("Marking source and destination"));
  sbLDBDSEnumerator::iterator const srcEnd = aSrc->end();
  sbLDBDSEnumerator::OriginIterator const destOriginEnd = aDest->OriginEnd();
  sbLDBDSEnumerator::IDIterator const destIDEnd = aDest->IDEnd();
  for (sbLDBDSEnumerator::iterator srcIter = aSrc->begin();
       srcIter != srcEnd;
       ++srcIter) {
    // Find the first non-marked destination item with the origin ID of our
    // source's ID
    sbLDBDSEnumerator::OriginIterator destOriginIter =
      aDest->FindByOrigin(srcIter->mID);
    destOriginIter = FindNextUsable(destOriginIter,
                                    destOriginEnd,
                                    &sbLDBDSEnumerator::ItemInfo::mOriginID);

    // If found then we're mark the destination as an update and get it's ID
    nsID foundDestID;
    bool found = false;
    if (destOriginIter != destOriginEnd) {
      (*destOriginIter)->mAction = sbLDBDSEnumerator::ItemInfo::ACTION_UPDATE;
      foundDestID = (*destOriginIter)->mID;
      found = true;
    }
    // Not found, so look at other copy traits
    else {
      // If the destination and source have the same ID then obviously they
      // are copies
      sbLDBDSEnumerator::IDIterator destIDIter = aDest->FindByID(srcIter->mID);
      destIDIter = FindNextUsable(destIDIter,
                                  destIDEnd,
                                  &sbLDBDSEnumerator::ItemInfo::mOriginID);
      // Not found so see if a destination item's ID is the same as our
      // origin, if it is set
      if (destIDIter == destIDEnd && !srcIter->mOriginID.Equals(NULL_GUID)) {
        destIDIter = aDest->FindByID(srcIter->mOriginID);
        destIDIter = FindNextUsable(destIDIter,
                                    destIDEnd,
                                    &sbLDBDSEnumerator::ItemInfo::mID);
      }
      // if we found one then mark it and set the found flag
      if (destIDIter != destIDEnd) {
        (*destIDIter)->mAction = sbLDBDSEnumerator::ItemInfo::ACTION_UPDATE;
        foundDestID = (*destIDIter)->mID;
        found = true;
      }
    }
    // If we found a match in the destination then we need to update the
    // source, setting it's mAction and setting the origin back to the
    // to the destination
    if (found) {
      // Origin in source becomes destination GUID
      srcIter->mOriginID = foundDestID;
      srcIter->mAction = sbLDBDSEnumerator::ItemInfo::ACTION_UPDATE;
      LOG(("  Updating src=%s, dest=%s",
          NS_LossyConvertUTF16toASCII(
                                   sbGUIDToString(srcIter->mID)).BeginReading(),
          NS_LossyConvertUTF16toASCII(
                                sbGUIDToString(foundDestID)).BeginReading()));
    }
    // Not found, mark the source as new
    else {
      LOG(("  New src=%s",
          NS_LossyConvertUTF16toASCII(
                                 sbGUIDToString(srcIter->mID)).BeginReading()));
      srcIter->mAction = sbLDBDSEnumerator::ItemInfo::ACTION_NEW;
    }
  }
}

nsresult
sbLocalDatabaseDiffingService::CreateChanges(sbIMediaList * aSrcList,
                                             sbIMediaList * aDestList,
                                             sbLDBDSEnumerator * aSrcEnum,
                                             sbLDBDSEnumerator * aDestEnum,
                                             nsIArray ** aChanges)
{
  NS_ENSURE_ARG_POINTER(aSrcList);
  NS_ENSURE_ARG_POINTER(aDestList);
  NS_ENSURE_ARG_POINTER(aChanges);

  nsresult rv;

  LOG(("Creating changes"));
  nsCOMPtr<nsIMutableArray> libraryChanges =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> srcItem;
  nsCOMPtr<sbIMediaItem> destItem;
  nsCOMPtr<sbILibraryChange> libraryChange;

  // For each item in the source, verify presence in destination library.
  sbLDBDSEnumerator::const_iterator const srcEnd = aSrcEnum->end();
  for(sbLDBDSEnumerator::const_iterator srcIter = aSrcEnum->begin();
      srcIter != srcEnd;
      ++srcIter) {

    rv = aSrcList->GetItemByGuid(sbGUIDToString(srcIter->mID),
                                 getter_AddRefs(srcItem));
    if (NS_FAILED(rv) || !srcItem) {
      continue;
    }

    switch (srcIter->mAction) {
      case sbLDBDSEnumerator::ItemInfo::ACTION_NEW: {
#ifdef PR_LOGGING
        nsCOMPtr<nsIURI> uri;
        srcItem->GetContentSrc(getter_AddRefs(uri));
        nsCString spec;
        if (uri) {
          uri->GetSpec(spec);
        }
        LOG(("  New %s-%s",
             NS_LossyConvertUTF16toASCII(
                                   sbGUIDToString(srcIter->mID)).BeginReading(),
             spec.BeginReading()));
#endif
        rv = CreateItemAddedLibraryChange(srcItem,
                                          getter_AddRefs(libraryChange));
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      }
      case sbLDBDSEnumerator::ItemInfo::ACTION_UPDATE: {
        // The origin is the destination ID as updated in MarkLists
        rv = aDestList->GetItemByGuid(sbGUIDToString(srcIter->mOriginID),
                                      getter_AddRefs(destItem));
        if (NS_FAILED(rv) || !destItem) {
          continue;
        }
#ifdef PR_LOGGING
        nsCOMPtr<nsIURI> uri;
        srcItem->GetContentSrc(getter_AddRefs(uri));
        nsCString srcSpec;
        if (uri) {
          uri->GetSpec(srcSpec);
        }
        destItem->GetContentSrc(getter_AddRefs(uri));
        nsCString destSpec;
        if (uri) {
          uri->GetSpec(destSpec);
        }
        LOG(("  Updating"));
        LOG(("    %s-%s",
             NS_LossyConvertUTF16toASCII(
                                   sbGUIDToString(srcIter->mID)).BeginReading(),
             srcSpec.BeginReading()));
        LOG(("    %s-%s",
             NS_LossyConvertUTF16toASCII(
                             sbGUIDToString(srcIter->mOriginID)).BeginReading(),
             destSpec.BeginReading()));
#endif
        rv = CreateLibraryChangeFromItems(srcItem,
                                          destItem,
                                          getter_AddRefs(libraryChange));
        // Handle no changes error
        if (rv == NS_ERROR_NOT_AVAILABLE) {
          continue;
        }
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      }
      default: {
        NS_WARNING("Unexpected action in diffing source, skipping");
        break;
      }
    }
    if (libraryChange) {
      rv = libraryChanges->AppendElement(libraryChange, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  sbLDBDSEnumerator::const_iterator const destEnd = aDestEnum->end();
  // For each item in the destination
  for(sbLDBDSEnumerator::const_iterator destIter =
        aDestEnum->begin();
        destIter != destEnd;
        destIter++) {

    // Add delete change if not marked as used
    if (destIter->mAction ==
          sbLDBDSEnumerator::ItemInfo::ACTION_NONE) {
      rv = aDestList->GetItemByGuid(sbGUIDToString(destIter->mID),
                                    getter_AddRefs(destItem));
      // If we can't find it now, just skip it
      if (rv == NS_ERROR_NOT_AVAILABLE || !destItem) {
        continue;
      }
      NS_ENSURE_SUCCESS(rv, rv);

#ifdef PR_LOGGING
        nsCOMPtr<nsIURI> uri;
        destItem->GetContentSrc(getter_AddRefs(uri));
        nsCString spec;
        if (uri) {
          uri->GetSpec(spec);
        }
        LOG(("  Delete %s-%s",
             NS_LossyConvertUTF16toASCII(
                                   sbGUIDToString(destIter->mID)).BeginReading(),
             spec.BeginReading()));
#endif
      rv = CreateItemDeletedLibraryChange(destItem,
                                          getter_AddRefs(libraryChange));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = libraryChanges->AppendElement(libraryChange, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  rv = CallQueryInterface(libraryChanges.get(), aChanges);
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}

nsresult
sbLocalDatabaseDiffingService::Init()
{
  nsresult rv;

  mPropertyManager = do_CreateInstance(SB_PROPERTYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseDiffingService::GetPropertyIDs(nsIStringEnumerator **aPropertyIDs)
{
  NS_ENSURE_ARG_POINTER(aPropertyIDs);
  NS_ENSURE_STATE(mPropertyManager);

  nsCOMPtr<nsIStringEnumerator> ids;

  nsresult rv = mPropertyManager->GetPropertyIDs(getter_AddRefs(ids));
  NS_ENSURE_SUCCESS(rv, rv);

  ids.forget(aPropertyIDs);

  return NS_OK;
}

nsresult
sbLocalDatabaseDiffingService::CreateLibraryChangeFromItems(
                                sbIMediaItem *aSourceItem,
                                sbIMediaItem *aDestinationItem,
                                sbILibraryChange **aLibraryChange)
{
  NS_ENSURE_ARG_POINTER(aSourceItem);
  NS_ENSURE_ARG_POINTER(aDestinationItem);
  NS_ENSURE_ARG_POINTER(aLibraryChange);

  nsCOMPtr<sbIPropertyArray> sourceProperties;
  nsCOMPtr<sbIPropertyArray> destinationProperties;

  nsresult rv = aSourceItem->GetProperties(nsnull,
                                           getter_AddRefs(sourceProperties));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aDestinationItem->GetProperties(nsnull,
                                       getter_AddRefs(destinationProperties));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> propertyChanges;
  rv = CreatePropertyChangesFromProperties(sourceProperties,
                                           destinationProperties,
                                           getter_AddRefs(propertyChanges));
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbLibraryChange> libraryChange;
  NS_NEWXPCOM(libraryChange, sbLibraryChange);
  NS_ENSURE_TRUE(libraryChange, NS_ERROR_OUT_OF_MEMORY);

  rv = libraryChange->InitWithValues(sbIChangeOperation::MODIFIED,
                                     0,
                                     aSourceItem,
                                     aDestinationItem,
                                     propertyChanges,
                                     nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(libraryChange.get(), aLibraryChange);
}

nsresult
sbLocalDatabaseDiffingService::CreateItemAddedLibraryChange(
                                sbIMediaItem *aSourceItem,
                                sbILibraryChange **aLibraryChange)
{
  NS_ENSURE_ARG_POINTER(aSourceItem);
  NS_ENSURE_ARG_POINTER(aLibraryChange);

  nsRefPtr<sbLibraryChange> libraryChange;
  NS_NEWXPCOM(libraryChange, sbLibraryChange);
  NS_ENSURE_TRUE(libraryChange, NS_ERROR_OUT_OF_MEMORY);

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

  rv = libraryChange->InitWithValues(sbIChangeOperation::ADDED,
                                     0,
                                     aSourceItem,
                                     nsnull,
                                     propertyChanges,
                                     nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(libraryChange.get(), aLibraryChange);
}

nsresult
sbLocalDatabaseDiffingService::CreateItemMovedLibraryChange(sbIMediaItem *aSourceItem,
                                                            PRUint32 aItemOrdinal,
                                                            sbILibraryChange **aLibraryChange)
{
  NS_ENSURE_ARG_POINTER(aSourceItem);
  NS_ENSURE_ARG_POINTER(aLibraryChange);

  nsresult rv = NS_ERROR_UNEXPECTED;

  nsRefPtr<sbLibraryChange> libraryChange;
  NS_NEWXPCOM(libraryChange, sbLibraryChange);
  NS_ENSURE_TRUE(libraryChange, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIMutableArray> propertyChanges =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbPropertyChange> propertyChange;
  NS_NEWXPCOM(propertyChange, sbPropertyChange);
  NS_ENSURE_TRUE(propertyChange, NS_ERROR_OUT_OF_MEMORY);

  nsString strPropertyValue;
  strPropertyValue.AppendInt(aItemOrdinal);

  rv = propertyChange->InitWithValues(sbIChangeOperation::MODIFIED,
                                      NS_LITERAL_STRING(SB_PROPERTY_ORDINAL),
                                      EmptyString(),
                                      strPropertyValue);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupports> element =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbIPropertyChange *, propertyChange), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propertyChanges->AppendElement(element,
                                      PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = libraryChange->InitWithValues(sbIChangeOperation::MOVED,
                                     0,
                                     aSourceItem,
                                     nsnull,
                                     propertyChanges,
                                     nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(libraryChange.get(), aLibraryChange);
}

nsresult
sbLocalDatabaseDiffingService::CreateItemDeletedLibraryChange(sbIMediaItem *aDestinationItem,
                                                              sbILibraryChange **aLibraryChange)
{
  NS_ENSURE_ARG_POINTER(aDestinationItem);
  NS_ENSURE_ARG_POINTER(aLibraryChange);

  nsRefPtr<sbLibraryChange> libraryChange;
  NS_NEWXPCOM(libraryChange, sbLibraryChange);
  NS_ENSURE_TRUE(libraryChange, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = libraryChange->InitWithValues(sbIChangeOperation::DELETED,
                                              0,
                                              nsnull,
                                              aDestinationItem,
                                              nsnull,
                                              nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(libraryChange.get(), aLibraryChange);
}

nsresult
sbLocalDatabaseDiffingService::CreatePropertyChangesFromProperties(
                                sbIPropertyArray *aSourceProperties,
                                sbIPropertyArray *aDestinationProperties,
                                nsIArray **aPropertyChanges)
{
  NS_ENSURE_ARG_POINTER(aSourceProperties);
  NS_ENSURE_ARG_POINTER(aDestinationProperties);
  NS_ENSURE_ARG_POINTER(aPropertyChanges);

  nsresult rv;
  nsCOMPtr<nsIMutableArray> propertyChanges =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 sourceLength;
  rv = aSourceProperties->GetLength(&sourceLength);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 destinationLength;
  rv = aDestinationProperties->GetLength(&destinationLength);

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
    rv = aSourceProperties->GetPropertyAt(current, getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = property->GetId(propertyId);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = property->GetValue(propertyValue);
    NS_ENSURE_SUCCESS(rv, rv);

    if(propertyExclusionList.GetEntry(propertyId)) {
      continue;
    }

    rv = aDestinationProperties->GetPropertyValue(propertyId,
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
        rv = aDestinationProperties->GetPropertyValue(
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
      if(propertyValue.Equals(propertyDestinationValue)) {
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
      rv = propertyChanges->AppendElement(element,
                                          PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // Second, we process the destinationProperties.
  // This will enable us to determine which properties were removed
  // from the source.
  for(PRUint32 current = 0; current < destinationLength; ++current) {
    rv = aDestinationProperties->GetPropertyAt(current, getter_AddRefs(property));
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
sbLocalDatabaseDiffingService::CreateLibraryChangesetFromLists(
                                sbIMediaList *aSourceList,
                                sbIMediaList *aDestinationList,
                                sbILibraryChangeset **aLibraryChangeset)
{
  NS_ENSURE_ARG_POINTER(aSourceList);
  NS_ENSURE_ARG_POINTER(aDestinationList);
  NS_ENSURE_ARG_POINTER(aLibraryChangeset);

  nsRefPtr<sbLibraryChangeset> libraryChangeset;
  NS_NEWXPCOM(libraryChangeset, sbLibraryChangeset);
  NS_ENSURE_TRUE(libraryChangeset, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv;

  nsRefPtr<sbLDBDSEnumerator> sourceEnum;
  NS_NEWXPCOM(sourceEnum, sbLDBDSEnumerator);
  NS_ENSURE_TRUE(sourceEnum, NS_ERROR_OUT_OF_MEMORY);

  nsRefPtr<sbLDBDSEnumerator> destinationEnum;
  NS_NEWXPCOM(destinationEnum, sbLDBDSEnumerator);
  NS_ENSURE_TRUE(destinationEnum, NS_ERROR_OUT_OF_MEMORY);

  rv = aSourceList->EnumerateAllItems(sourceEnum,
    sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aDestinationList->EnumerateAllItems(destinationEnum,
    sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  MarkLists(sourceEnum, destinationEnum);

  nsCOMPtr<nsIArray> libraryChanges;
  rv = CreateChanges(aSourceList,
                     aDestinationList,
                     sourceEnum,
                     destinationEnum,
                     getter_AddRefs(libraryChanges));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMutableArray> libraryChangesMutable =
    do_QueryInterface(libraryChanges, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  // Ensure that all items present will be in the correct order
  // by explicitly including a move operation for each item present
  // the source. sourceEnum items are in playlist order
  PRInt32 listIndex = 0;
  sbLDBDSEnumerator::const_iterator const sourceEnd = sourceEnum->end();
  for (sbLDBDSEnumerator::const_iterator sourceIter = sourceEnum->begin();
       sourceIter != sourceEnd;
       ++sourceIter) {
    // Skip delete ones.
    if (sourceIter->mAction == sbLDBDSEnumerator::ItemInfo::ACTION_DELETE) {
      continue;
    }
    nsCOMPtr<sbIMediaItem> sourceItem;
    rv = aSourceList->GetItemByIndex(sourceIter->mPosition,
                                     getter_AddRefs(sourceItem));
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<sbILibraryChange> libraryChange;
      rv = CreateItemMovedLibraryChange(sourceItem,
                                        listIndex++,
                                        getter_AddRefs(libraryChange));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = libraryChangesMutable->AppendElement(libraryChange, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // That's it, we should have a valid changeset.
  nsCOMPtr<nsIMutableArray> sources =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sources->AppendElement(aSourceList, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = libraryChangeset->InitWithValues(sources,
                                        aDestinationList,
                                        libraryChanges);
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(libraryChangeset.get(), aLibraryChangeset);
}

nsresult
sbLocalDatabaseDiffingService::CreateLibraryChangesetFromLibraries(
                                sbILibrary *aSourceLibrary,
                                sbILibrary *aDestinationLibrary,
                                sbILibraryChangeset **aLibraryChangeset)
{
  NS_ENSURE_ARG_POINTER(aSourceLibrary);
  NS_ENSURE_ARG_POINTER(aDestinationLibrary);
  NS_ENSURE_ARG_POINTER(aLibraryChangeset);

  NS_NAMED_LITERAL_STRING(ORIGIN_ID, SB_PROPERTY_ORIGINITEMGUID);

  nsRefPtr<sbLibraryChangeset> libraryChangeset;
  NS_NEWXPCOM(libraryChangeset, sbLibraryChangeset);
  NS_ENSURE_TRUE(libraryChangeset, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv;

  nsRefPtr<sbLDBDSEnumerator> sourceEnum;
  NS_NEWXPCOM(sourceEnum, sbLDBDSEnumerator);
  NS_ENSURE_TRUE(sourceEnum, NS_ERROR_OUT_OF_MEMORY);

  nsRefPtr<sbLDBDSEnumerator> destinationEnum;
  NS_NEWXPCOM(destinationEnum, sbLDBDSEnumerator);
  NS_ENSURE_TRUE(destinationEnum, NS_ERROR_OUT_OF_MEMORY);

  rv = aSourceLibrary->EnumerateAllItems(sourceEnum,
                                         sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aDestinationLibrary->EnumerateAllItems(destinationEnum,
                                              sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  MarkLists(sourceEnum, destinationEnum);

  nsCOMPtr<nsIArray> libraryChanges;
  rv = CreateChanges(aSourceLibrary,
                     aDestinationLibrary,
                     sourceEnum,
                     destinationEnum,
                     getter_AddRefs(libraryChanges));
  NS_ENSURE_SUCCESS(rv, rv);

  // That's it, we should have a valid changeset.
  nsCOMPtr<nsIMutableArray> sources =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sources->AppendElement(aSourceLibrary, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = libraryChangeset->InitWithValues(sources,
                                        aDestinationLibrary,
                                        libraryChanges);
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(libraryChangeset.get(), aLibraryChangeset);
}

inline
nsresult AddUniqueItem(nsTHashtable<nsIDHashKey> & aItems, sbIMediaItem * aItem)
{
  nsID const & itemID = GetItemGUID(aItem);
  NS_ENSURE_SUCCESS(!itemID.Equals(NULL_GUID), NS_ERROR_FAILURE);

  // Already in the list, done.
  if (!aItems.GetEntry(itemID)) {
    nsIDHashKey *hashKey = aItems.PutEntry(itemID);
    NS_ENSURE_TRUE(hashKey, NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

struct EnumeratorArgs
{
  sbLDBDSEnumerator * mDestinationEnum;
  sbLocalDatabaseDiffingService * mDiffService;
  nsIMutableArray * mLibraryChanges;
  sbILibrary * mSourceLibrary;
  sbILibrary * mDestinationLibrary;
};

PLDHashOperator
sbLocalDatabaseDiffingService::Enumerator(nsIDHashKey* aEntry, void* userArg)
{
  nsresult rv;

  EnumeratorArgs * args = static_cast<EnumeratorArgs*>(userArg);

  nsCOMPtr<sbIMediaItem> sourceItem;

  rv = args->mSourceLibrary->GetItemByGuid(
                                  sbGUIDToString(aEntry->GetKey()),
                                  getter_AddRefs(sourceItem));
  NS_ENSURE_SUCCESS(rv, PL_DHASH_NEXT);

  nsCOMPtr<sbILibraryChange> libraryChange;

  sbLDBDSEnumerator::OriginIterator iter =
    args->mDestinationEnum->FindByOrigin(aEntry->GetKey());
  nsCOMPtr<sbIMediaItem> destinationItem;
  if (iter != args->mDestinationEnum->OriginEnd()) {
    (*iter)->mAction = sbLDBDSEnumerator::ItemInfo::ACTION_UPDATE;
    args->mDestinationLibrary->GetItemByGuid(
                                    sbGUIDToString((*iter)->mID),
                                    getter_AddRefs(destinationItem));
    NS_ENSURE_SUCCESS(rv, PL_DHASH_NEXT);
  }
  else {
    sbLDBDSEnumerator::IDIterator idIter =
      args->mDestinationEnum->FindByID(aEntry->GetKey());
    if (idIter == args->mDestinationEnum->IDEnd()) {
      nsID originID = GetGUIDProperty(
                                 sourceItem,
                                 NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID));
      if (!originID.Equals(NULL_GUID)) {
        idIter = args->mDestinationEnum->FindByID(originID);
      }
    }
    if (idIter != args->mDestinationEnum->IDEnd()) {
      (*idIter)->mAction = sbLDBDSEnumerator::ItemInfo::ACTION_UPDATE;
      args->mDestinationLibrary->GetItemByGuid(
                                      sbGUIDToString((*idIter)->mID),
                                      getter_AddRefs(destinationItem));
      NS_ENSURE_SUCCESS(rv, PL_DHASH_NEXT);
    }
  }
  if (destinationItem) {
    // Do not update playlist
    nsString isList;
    rv = sourceItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ISLIST),
                                 isList);
    NS_ENSURE_SUCCESS(rv, PL_DHASH_NEXT);

    if (isList.EqualsLiteral("1"))
      return PL_DHASH_NEXT;

    LogMediaItem("Source Item", sourceItem);
    LogMediaItem("Destination item", destinationItem);
    rv = args->mDiffService->CreateLibraryChangeFromItems(
                                                 sourceItem,
                                                 destinationItem,
                                                 getter_AddRefs(libraryChange));

    // Item did not change.
    if (rv == NS_ERROR_NOT_AVAILABLE) {
      return PL_DHASH_NEXT;
    }
    NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  }
  else {
    LogMediaItem("New item", sourceItem);
    rv = args->mDiffService->CreateItemAddedLibraryChange(
                                                 sourceItem,
                                                 getter_AddRefs(libraryChange));
    NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);
  }
  rv = args->mLibraryChanges->AppendElement(libraryChange, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);
  return PL_DHASH_NEXT;
}

nsresult
sbLocalDatabaseDiffingService::CreateLibraryChangesetFromListsToLibrary(
                                  nsIArray *aSourceLists,
                                  sbILibrary *aDestinationLibrary,
                                  sbILibraryChangeset **aLibraryChangeset)
{
  NS_ENSURE_ARG_POINTER(aSourceLists);
  NS_ENSURE_ARG_POINTER(aDestinationLibrary);
  NS_ENSURE_ARG_POINTER(aLibraryChangeset);

  // First, we build a hashtable of all the unique GUIDs.
  PRUint32 sourcesLength = 0;
  nsresult rv = aSourceLists->GetLength(&sourcesLength);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> sourceItem;
  nsCOMPtr<sbIMediaList> sourceList;
  nsCOMPtr<sbILibrary> sourceLibrary;

  nsTHashtable<nsIDHashKey> sourceItemIDs;
  sourceItemIDs.Init();
  for(PRUint32 currentSource = 0;
      currentSource < sourcesLength;
      ++currentSource) {

    sourceItem = do_QueryElementAt(aSourceLists, currentSource, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // TODO: XXX a bit of a hack, we're assuming all lists are from the same
    // library
    if (currentSource == 0) {
      rv = sourceItem->GetLibrary(getter_AddRefs(sourceLibrary));
      NS_ENSURE_SUCCESS(rv, rv);
    }
#ifdef DEBUG
    // Ensure that we're seeing items from the same library
    else {
      nsCOMPtr<sbILibrary> testLibrary;
      PRBool sameLibrary = PR_FALSE;
      rv = sourceItem->GetLibrary(getter_AddRefs(testLibrary));
      if (NS_SUCCEEDED(rv)) {

        rv = testLibrary->Equals(sourceLibrary, &sameLibrary);
        sameLibrary = sameLibrary && NS_SUCCEEDED(rv);
      }
      NS_ASSERTION(sameLibrary,
                   "Source cannot contain items from different libraries");
    }
#endif
    sourceList = do_QueryInterface(sourceItem, &rv);
    // Not media list.
    if (NS_FAILED(rv)) {
      rv = AddUniqueItem(sourceItemIDs, sourceItem);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      PRUint32 sourceLength = 0;
      rv = sourceList->GetLength(&sourceLength);
      NS_ENSURE_SUCCESS(rv, rv);

      for(PRUint32 currentItem = 0; currentItem < sourceLength; ++currentItem) {
        rv = sourceList->GetItemByIndex(currentItem,
                                        getter_AddRefs(sourceItem));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = AddUniqueItem(sourceItemIDs,
                           sourceItem);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      // Add source list to the unique item list if it's not a library.
      nsCOMPtr<sbILibrary> sourceListIsLibrary = do_QueryInterface(sourceList);
      if (!sourceListIsLibrary) {
        rv = AddUniqueItem(sourceItemIDs,
                           sourceList);
        NS_ENSURE_SUCCESS(rv, rv);
     }
    }
  }

  nsRefPtr<sbLibraryChangeset> libraryChangeset;
  NS_NEWXPCOM(libraryChangeset, sbLibraryChangeset);
  NS_ENSURE_TRUE(libraryChangeset, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIMutableArray> libraryChanges =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbLDBDSEnumerator> destinationEnum;
  NS_NEWXPCOM(destinationEnum, sbLDBDSEnumerator);
  NS_ENSURE_TRUE(destinationEnum, NS_ERROR_OUT_OF_MEMORY);

  rv = aDestinationLibrary->EnumerateAllItems(destinationEnum,
    sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create a unique list of media items in the source lists
  EnumeratorArgs args = {
    destinationEnum,
    this,
    libraryChanges,
    sourceLibrary,
    aDestinationLibrary
  };
  sourceItemIDs.EnumerateEntries(Enumerator, &args);

  // For each item in the destination
  sbLDBDSEnumerator::const_iterator const end = destinationEnum->end();
  for (sbLDBDSEnumerator::const_iterator iter = destinationEnum->begin();
       iter != end;
       ++iter) {
    if (iter->mAction == sbLDBDSEnumerator::ItemInfo::ACTION_NONE) {
      // Not present in source, indicate that the item was removed from the
      // source.
      nsCOMPtr<sbIMediaItem> destItem;
      rv = aDestinationLibrary->GetItemByGuid(sbGUIDToString(iter->mID),
                                              getter_AddRefs(destItem));
      // If we can't find it now, just skip it
      if (rv == NS_ERROR_NOT_AVAILABLE || !destItem) {
        continue;
      }
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<sbILibraryChange> libraryChange;
      rv = CreateItemDeletedLibraryChange(destItem, getter_AddRefs(libraryChange));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = libraryChanges->AppendElement(libraryChange, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // That's it, we should have a valid changeset.
  rv = libraryChangeset->InitWithValues(aSourceLists,
                                        aDestinationLibrary,
                                        libraryChanges);
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(libraryChangeset.get(), aLibraryChangeset);
}

/* sbILibraryChangeset createChangeset (in sbIMediaList aSource, in sbIMediaList aDestination); */
NS_IMETHODIMP sbLocalDatabaseDiffingService::CreateChangeset(sbIMediaList *aSource,
                                                             sbIMediaList *aDestination,
                                                             sbILibraryChangeset **_retval)
{
  NS_ENSURE_ARG_POINTER(aSource);
  NS_ENSURE_ARG_POINTER(aDestination);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbILibrary> sourceLibrary = do_QueryInterface(aSource);
  nsCOMPtr<sbILibrary> destinationLibrary = do_QueryInterface(aDestination);

  nsresult rv = NS_ERROR_FAILURE;
  nsCOMPtr<sbILibraryChangeset> changeset;

  if(sourceLibrary && destinationLibrary) {

    rv = CreateLibraryChangesetFromLibraries(sourceLibrary,
                                             destinationLibrary,
                                             getter_AddRefs(changeset));

  }
  else {
    rv = CreateLibraryChangesetFromLists(aSource,
                                         aDestination,
                                         getter_AddRefs(changeset));
  }

  NS_ENSURE_SUCCESS(rv, rv);
  changeset.forget(_retval);

  return NS_OK;
}

/* sbILibraryChangeset createMultiChangeset (in nsIArray aSources, in sbIMediaList aDestination); */
NS_IMETHODIMP sbLocalDatabaseDiffingService::CreateMultiChangeset(nsIArray *aSources,
                                                                  sbIMediaList *aDestination,
                                                                  sbILibraryChangeset **_retval)
{
  NS_ENSURE_ARG_POINTER(aSources);
  NS_ENSURE_ARG_POINTER(aDestination);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv = NS_ERROR_UNEXPECTED;

  // Destination must be a library.
  nsCOMPtr<sbILibrary> destinationLibrary =
    do_QueryInterface(aDestination);
  NS_ENSURE_TRUE(destinationLibrary, NS_ERROR_INVALID_ARG);

  nsCOMPtr<sbILibraryChangeset> changeset;
  rv = CreateLibraryChangesetFromListsToLibrary(aSources,
                                                destinationLibrary,
                                                getter_AddRefs(changeset));
  NS_ENSURE_SUCCESS(rv, rv);

  changeset.forget(_retval);

  return NS_OK;
}

/* AString createChangesetAsync (in sbIMediaList aSource, in sbIMediaList aDestination, [optional] in nsIObserver aObserver); */
NS_IMETHODIMP sbLocalDatabaseDiffingService::CreateChangesetAsync(sbIMediaList *aSource,
                                                                  sbIMediaList *aDestination,
                                                                  nsIObserver *aObserver,
                                                                  nsAString & _retval)
{
  NS_ENSURE_ARG_POINTER(aSource);
  NS_ENSURE_ARG_POINTER(aDestination);

  return NS_ERROR_NOT_IMPLEMENTED;
}

/* sbILibraryChangeset createMultiChangesetAsync (in nsIArray aSources, in sbIMediaList aDestination, [optional] in nsIObserver aObserver); */
NS_IMETHODIMP sbLocalDatabaseDiffingService::CreateMultiChangesetAsync(nsIArray *aSources,
                                                                       sbIMediaList *aDestination,
                                                                       nsIObserver *aObserver,
                                                                       sbILibraryChangeset **_retval)
{
  NS_ENSURE_ARG_POINTER(aSources);
  NS_ENSURE_ARG_POINTER(aDestination);
  NS_ENSURE_ARG_POINTER(_retval);

  return NS_ERROR_NOT_IMPLEMENTED;
}

/* sbILibraryChangeset getChangeset (in AString aChangesetCookie); */
NS_IMETHODIMP sbLocalDatabaseDiffingService::GetChangeset(const nsAString & aChangesetCookie,
                                                          sbILibraryChangeset **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  return NS_ERROR_NOT_IMPLEMENTED;
}

