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

#ifndef __SBLOCALDATABASEMEDIALISTBASE_H__
#define __SBLOCALDATABASEMEDIALISTBASE_H__

#include "sbLocalDatabaseMediaItem.h"
#include "sbLocalDatabaseMediaListListener.h"
#include <nsIClassInfo.h>
#include <nsIStringEnumerator.h>
#include <sbIMediaList.h>

#include <nsClassHashtable.h>
#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
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
  typedef nsTArray<nsString> sbStringArray;
  typedef nsClassHashtable<nsStringHashKey, sbStringArray> sbStringArrayHash;

public:
  NS_DECL_ISUPPORTS_INHERITED

  NS_FORWARD_SBILOCALDATABASERESOURCEPROPERTY(sbLocalDatabaseMediaItem::)
  NS_FORWARD_SBILOCALDATABASEMEDIAITEM(sbLocalDatabaseMediaItem::)
  NS_FORWARD_SBILIBRARYRESOURCE(sbLocalDatabaseMediaItem::)
  NS_FORWARD_SBIMEDIAITEM(sbLocalDatabaseMediaItem::)

  NS_DECL_SBIMEDIALIST

  sbLocalDatabaseMediaListBase();
  virtual ~sbLocalDatabaseMediaListBase();

  nsresult Init(sbLocalDatabaseLibrary* aLibrary,
                const nsAString& aGuid,
                PRBool aOwnsLibrary = PR_TRUE);

  already_AddRefed<sbLocalDatabaseLibrary> GetNativeLibrary();

  nsresult AddListener(sbIMediaListListener* aListener,
                       PRBool aOwnsWeak = PR_FALSE,
                       PRUint32 aFlags = 0) {
    return AddListener(aListener, aOwnsWeak, aFlags, nsnull);
  }

protected:
  NS_IMETHOD GetDefaultSortProperty(nsAString& aProperty) = 0;

  nsresult MakeStandardQuery(sbIDatabaseQuery** _retval);

  nsresult GetFilteredPropertiesForNewItem(sbIPropertyArray* aProperties,
                                           sbIPropertyArray** _retval);

private:

  // This callback is meant to be used with an sbStringArrayHash.
  // aUserData should be a sbILocalDatabaseGUIDArray pointer.
  static PLDHashOperator PR_CALLBACK
    AddFilterToGUIDArrayCallback(nsStringHashKey::KeyType aKey,
                                 sbStringArray* aEntry,
                                 void* aUserData);

  nsresult EnumerateAllItemsInternal(sbIMediaListEnumerationListener* aEnumerationListener);

  nsresult EnumerateItemsByPropertyInternal(const nsAString& aName,
                                            nsIStringEnumerator* aValueEnum,
                                            sbIMediaListEnumerationListener* aEnumerationListener);

  nsresult EnumerateItemsByPropertiesInternal(sbStringArrayHash* aPropertiesHash,
                                              sbIMediaListEnumerationListener* aEnumerationListener);

  // Called for the enumeration methods.
  nsresult EnumerateItemsInternal(sbGUIDArrayEnumerator* aEnumerator,
                                  sbIMediaListEnumerationListener* aListener);

protected:

  // The mFullArray is a cached version of the full contents of the media
  // list this instance represents.
  nsCOMPtr<sbILocalDatabaseGUIDArray> mFullArray;

  // A monitor for changes to the media list.
  PRMonitor* mFullArrayMonitor;

  PRBool mLockedEnumerationActive;

private:
  PRInt32 mBatchCount;
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
  sbAutoBatchHelper(sbLocalDatabaseMediaListBase* aList)
  : mList(aList)
  {
    NS_ASSERTION(aList, "Null pointer!");
    mList->BeginUpdateBatch();
  }

  ~sbAutoBatchHelper()
  {
    mList->EndUpdateBatch();
  }

private:
  // Not meant to be implemented. This makes it a compiler error to
  // attempt to create an object on the heap.
  static void* operator new(size_t /*size*/) CPP_THROW_NEW {return 0;}
  static void operator delete(void* /*memory*/) { }

  sbLocalDatabaseMediaListBase* mList;
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
