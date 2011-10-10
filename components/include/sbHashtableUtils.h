/* vim: set sw=2 :miv */
/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

#ifndef SBHASHTABLEUTILS_H_
#define SBHASHTABLEUTILS_H_

#include <nsDataHashtable.h>

/**
 * Enumerate function to copy hash elements
 */
template <class T>
PLDHashOperator HashCopierEnumerator(typename T::KeyType aKey,
                                     typename T::DataType aData,
                                     void* userArg)
{
  NS_ASSERTION(userArg, "ArrayBuilder passed a null arg");
  typename T::Hashtable * table = reinterpret_cast<typename T::Hashtable*>(userArg);

  PRBool added = table->Put(aKey, aData);
  NS_ENSURE_TRUE(added, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}

/**
 * Copies one hash table to another
 * Mozilla hash table classes do not have proper traits so users must provide a
 * traits class representing the hashtable class, key and data types.
 * T must be a traits class containing Hashtable, KeyType, and DataType entries
 * describing the hashable
 * Example:
 *  struct MyHashtableTraits
 *  {
 *    typedef nsHashtable<TheKeyType, TheDataType> Hashtable;
 *    typedef TheKeyType KeyType;
 *    typedef TheDataType DataType;
 *  };
 */
template <class T>
nsresult sbCopyHashtable(typename T::Hashtable const & aSource,
                         typename T::Hashtable & aDest)
{
  aSource.EnumerateRead(&HashCopierEnumerator<T>,
                        &aDest);

  return NS_OK;
}


#endif /* SBHASHTABLEUTILS_H_ */
