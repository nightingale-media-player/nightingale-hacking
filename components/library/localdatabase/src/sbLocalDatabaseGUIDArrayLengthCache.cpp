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

#include "sbLocalDatabaseGUIDArrayLengthCache.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(sbLocalDatabaseGUIDArrayLengthCache,
        sbILocalDatabaseGUIDArrayLengthCache);

sbLocalDatabaseGUIDArrayLengthCache::sbLocalDatabaseGUIDArrayLengthCache()
  : mLock("sbLocalDatabaseGUIDArrayLengthCache::mLock")
{
  mCachedLengths.Init();
  mCachedNonNullLengths.Init();
}

sbLocalDatabaseGUIDArrayLengthCache::~sbLocalDatabaseGUIDArrayLengthCache()
{
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArrayLengthCache::AddCachedLength(const nsAString &aKey,
                                                     PRUint32 aLength)
{
  mozilla::MutexAutoLock lock(mLock);

  NS_ENSURE_TRUE(mCachedLengths.Put(aKey, aLength), NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArrayLengthCache::GetCachedLength(const nsAString &aKey,
                                                     PRUint32 *aLength)
{
  NS_ENSURE_ARG_POINTER(aLength);

  *aLength = 0;

  mozilla::MutexAutoLock lock(mLock);

  if (!mCachedLengths.Get(aKey, aLength))
    return NS_ERROR_NOT_AVAILABLE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArrayLengthCache::RemoveCachedLength(const nsAString &aKey)
{
  mozilla::MutexAutoLock lock(mLock);

  // We don't care if it wasn't there
  mCachedLengths.Remove(aKey);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArrayLengthCache::AddCachedNonNullLength(
        const nsAString &aKey,
        PRUint32 aLength)
{
  mozilla::MutexAutoLock lock(mLock);

  NS_ENSURE_TRUE(mCachedNonNullLengths.Put(aKey, aLength),
          NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArrayLengthCache::GetCachedNonNullLength(
        const nsAString &aKey,
        PRUint32 *aLength)
{
  NS_ENSURE_ARG_POINTER(aLength);

  *aLength = 0;

  mozilla::MutexAutoLock lock(mLock);

  if (!mCachedNonNullLengths.Get(aKey, aLength))
    return NS_ERROR_NOT_AVAILABLE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArrayLengthCache::RemoveCachedNonNullLength(
        const nsAString &aKey)
{
  mozilla::MutexAutoLock lock(mLock);

  // We don't care if it wasn't there
  mCachedNonNullLengths.Remove(aKey);

  return NS_OK;
}

