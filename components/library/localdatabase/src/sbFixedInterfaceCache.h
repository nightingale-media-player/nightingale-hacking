/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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
#ifndef SBFIXEDINTERFACECACHE_H_
#define SBFIXEDINTERFACECACHE_H_

#include <utility>

#include <nsAutoLock.h>
#include <nsTArray.h>

/**
 * This class provides a fixed size cache for interface pointers.
 * It holds an owning reference to the pointer, but hands out
 * raw pointers that much be AddRef'd by the caller if they
 * wish to hold on to it.
 * The cache is populated from the end to make it easy to
 * search the most recently added entry.
 * The class now sports a hash implementation so lookups are fast and
 * this makes the cache much more scalable and negates the power of 2
 * requirement on the size.
 */
template <class KeyClass, class Interface>
class sbFixedInterfaceCache
{
public:
  typedef typename KeyClass::KeyType KeyType;

  /**
   * Initializes the key array and hash to the cache size.
   * NOTE: The size must be a power of 2
   */
  sbFixedInterfaceCache(PRUint32 aSize) : mKeys(aSize),
                                          mCurrent(aSize),
                                          mSize(aSize)
  {
    NS_ASSERTION(aSize, "sbFixedInterfaceCache must have a size > 0");
    // Fill out array so that we don't have to worry about using append later
    mKeys.AppendElements(mSize);
    mCacheMap.Init(aSize);
  }
  /**
   * Releases references to the bags we're holding
   */
  ~sbFixedInterfaceCache()
  {
    for (PRUint32 index = 0; index < mSize; ++index)
    {
      Interface * obj = nsnull;
      mCacheMap.Get(mKeys[index], &obj);
      NS_IF_RELEASE(obj);
    }
  }
  /**
   * Replaces or adds the interface pointer for aKey. The
   * previous interface pointer is released if there is one.
   */
  void Put(KeyType aKey, Interface * aValue)
  {
    NS_IF_ADDREF(aValue);
    if (mCurrent == 0)
      mCurrent = mSize - 1;
    else
      --mCurrent;
    // Clean up the old entry
    Interface * oldObject = nsnull;
    nsString const & oldKey = mKeys[mCurrent];
    if (!oldKey.IsEmpty() && mCacheMap.Get(oldKey, &oldObject) && oldObject) {
      NS_RELEASE(oldObject);
      mCacheMap.Remove(oldKey);
    }
    // Save off the new one
    mKeys[mCurrent] = aKey;
    mCacheMap.Put(aKey, aValue);
  }
  /**
   * Returns the interface pointer for aKey. If it's not found
   * then nsnull is returned. The pointer returned is not
   * addref'd so if you want to keep it around be sure to do
   * it youself
   */
  Interface * Get(KeyType aKey) const
  {
    Interface * obj = nsnull;
    // If this fails, obj is null which is what we want to return in that case
    mCacheMap.Get(aKey, &obj);
    return obj;
  }
private:
  // Holds the list of keys and is used for aging entries out
  // nsStringHashKey has sucky traits so we have to hard code string here
  nsTArray<nsString> mKeys;
  // Provides a fast lookup non-owning map
  nsDataHashtable<KeyClass, Interface*> mCacheMap;
  PRUint32 mCurrent;
  PRUint32 const mSize;
};

#endif /* SBFIXEDINTERFACECACHE_H_ */
