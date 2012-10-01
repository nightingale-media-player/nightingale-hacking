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

#ifndef __SBLOCALDATABASEMEDIALISTBASE_H__
#define __SBLOCALDATABASEMEDIALISTBASE_H__

#include "sbLocalDatabaseMediaItem.h"
#include "sbLocalDatabaseMediaListListener.h"
#include <nsIClassInfo.h>
#include <nsIStringEnumerator.h>
#include <sbILocalDatabaseGUIDArray.h>
#include <sbIMediaList.h>

#include <nsClassHashtable.h>
#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <nsTHashtable.h>
#include <prmon.h>
#include <prlock.h>

// Macros to help early returns within loops.
#define SB_CONTINUE_IF_FALSE(_expr)                                            \
  PR_BEGIN_MACRO                                                               \
    if (!(_expr)) {                                                            \
      NS_WARNING("SB_CONTINUE_IF_FALSE triggered");                            \
      continue;                                                                \
    }                                                                          \
  PR_END_MACRO

#define SB_CONTINUE_IF_FAILED(_rv)                                             \
  SB_CONTINUE_IF_FALSE(NS_SUCCEEDED(_rv))

// This macro is for derived classes that want to ensure safe access to
// mFullArray.
#define SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE()                       \
  PR_BEGIN_MACRO                                                               \
    nsAutoMonitor mon(mFullArrayMonitor);                                      \
    if (mLockedEnumerationActive) {                                            \
      NS_ERROR("Operation not permitted during a locked enumeration");         \
      return NS_ERROR_FAILURE;                                                 \
    }                                                                          \
  PR_END_MACRO

#define SB_ASYNC_NOTIFICATION_ITEMS  50

class nsIArray;
class nsIMutableArray;
class nsIStringEnumerator;
class nsStringHashKey;
class sbGUIDArrayEnumerator;
class sbIDatabaseQuery;
class sbILocalDatabaseGUIDArray;
class sbILocalDatabaseLibrary;
class sbIMediaItem;
class sbIMediaListEnumerationListener;

class sbLocalDatabaseMediaListBase : public sbLocalDatabaseMediaItem,
                                     public sbLocalDatabaseMediaListListener,
                                     public sbIMediaList
{
  friend class sbAutoBatchHelper;

  typedef nsTArray<nsString> sbStringArray;
  typedef nsClassHashtable<nsStringHashKey, sbStringArray> sbStringArrayHash;

public:
  NS_DECL_ISUPPORTS_INHERITED

  NS_FORWARD_SBILOCALDATABASEMEDIAITEM(sbLocalDatabaseMediaItem::)
  NS_FORWARD_SBILIBRARYRESOURCE(sbLocalDatabaseMediaItem::)
  NS_FORWARD_SBIMEDIAITEM(sbLocalDatabaseMediaItem::)

  NS_DECL_SBIMEDIALIST

  sbLocalDatabaseMediaListBase();
  virtual ~sbLocalDatabaseMediaListBase();

  nsresult Init(sbLocalDatabaseLibrary* aLibrary,
                const nsAString& aGuid,
                bool aOwnsLibrary = PR_TRUE);

  already_AddRefed<sbLocalDatabaseLibrary> GetNativeLibrary();

  sbILocalDatabaseGUIDArray * GetArray() {
    NS_ASSERTION(mFullArray, "mArray is null!");
    return mFullArray;
  }

  nsresult AddListener(sbIMediaListListener* aListener,
                       bool aOwnsWeak = PR_FALSE,
                       PRUint32 aFlags = 0) {
    return AddListener(aListener, aOwnsWeak, aFlags, nsnull);
  }

  void SetCachedListContentType(PRUint16 aContentType) {
    mListContentType = aContentType;
  }

  // These aren't meant to be called directly. Use sbAutoBatchHelper
  // to avoid the risk of leaving a batch in progress
  void BeginUpdateBatch() {
    sbLocalDatabaseMediaListListener::NotifyListenersBatchBegin(this);
  }
  void EndUpdateBatch() {
    sbLocalDatabaseMediaListListener::NotifyListenersBatchEnd(this);
  }

protected:
  NS_IMETHOD GetDefaultSortProperty(nsAString& aProperty) = 0;

  nsresult MakeStandardQuery(sbIDatabaseQuery** _retval);

  nsresult GetFilteredPropertiesForNewItem(sbIPropertyArray* aProperties,
                                           sbIPropertyArray** _retval);

  // Set SB_PROPERTY_ORIGINLIBRARYGUID and SB_PROPERTY_ORIGINITEMGUID, and
  // set SB_PROPERTY_ORIGIN_IS_IN_MAIN_LIBRARY to true in the given
  // property array if aSourceItem is in the main library.  Otherwise, set
  // these properties only if they are not already set, and remove
  // SB_PROPERTY_ORIGIN_IS_IN_MAIN_LIBRARY if it is false.  aProperties
  // should be properties for a new item that is being created by copying
  // aSourceItem.
  nsresult GetOriginProperties(sbIMediaItem *             aSourceItem,
                               sbIMutablePropertyArray *  aProperties);

private:

  // Remove any values for the aProperty from aPropertyArray
  static nsresult RemoveProperty(sbIMutablePropertyArray * aPropertyArray,
                                 const nsAString &         aProperty);

  // This callback is meant to be used with an sbStringArrayHash.
  // aUserData should be a sbILocalDatabaseGUIDArray pointer.
  static PLDHashOperator PR_CALLBACK
    AddFilterToGUIDArrayCallback(nsStringHashKey::KeyType aKey,
                                 sbStringArray* aEntry,
                                 void* aUserData);

  nsresult EnumerateAllItemsInternal(sbIMediaListEnumerationListener* aEnumerationListener);

  nsresult EnumerateItemsByPropertyInternal(const nsAString& aID,
                                            nsIStringEnumerator* aValueEnum,
                                            sbIMediaListEnumerationListener* aEnumerationListener);
  /**
   * Enumerates by properties using the previous filter that was set
   */
  nsresult EnumerateItemsByPropertiesInternal(sbStringArrayHash* aPropertiesHash,
                                              sbIMediaListEnumerationListener* aEnumerationListener);

  // Called for the enumeration methods.
  nsresult EnumerateItemsInternal(sbGUIDArrayEnumerator* aEnumerator,
                                  sbIMediaListEnumerationListener* aListener);

protected:
  void SetArray(sbILocalDatabaseGUIDArray * aArray);
  // A monitor for changes to the media list.
  PRMonitor* mFullArrayMonitor;

  // Cached list content type
  PRUint16 mListContentType;

  bool mLockedEnumerationActive;
  
  // The mFilteredProperties hash table caches the property ids
  // that we always want to filter out of the property arrays that
  // are used to create media items or set multiple properties
  // on a library resource.
  nsTHashtable<nsStringHashKey> mFilteredProperties;

private:
  // The mFullArray is a cached version of the full contents of the media
  // list this instance represents.
  nsCOMPtr<sbILocalDatabaseGUIDArray> mFullArray;
};

/**
 * \class sbAutoBatchHelper
 *
 * \brief Simple class to make sure we notify listeners that a batch operation
 *        has completed every time they are notified that a batch operation
 *        has begun.
 */
class sbAutoBatchHelper
{
public:
  sbAutoBatchHelper(sbLocalDatabaseMediaListBase& aList)
  : mList(aList)
  {
    mList.BeginUpdateBatch();
  }

  ~sbAutoBatchHelper()
  {
    mList.EndUpdateBatch();
  }

private:
  // Not meant to be implemented. This makes it a compiler error to
  // attempt to create an object on the heap.
  static void* operator new(size_t /*size*/) CPP_THROW_NEW;
  static void operator delete(void* /*memory*/);

  sbLocalDatabaseMediaListBase& mList;
};

class sbGUIDArrayValueEnumerator : public nsIStringEnumerator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTRINGENUMERATOR

  sbGUIDArrayValueEnumerator(sbILocalDatabaseGUIDArray* aArray);

  ~sbGUIDArrayValueEnumerator();

private:
  nsCOMPtr<sbILocalDatabaseGUIDArray> mArray;
  PRUint32 mLength;
  PRUint32 mNextIndex;
};

#endif /* __SBLOCALDATABASEMEDIALISTBASE_H__ */
