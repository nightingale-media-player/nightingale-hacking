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

#ifndef __SBLOCALDATABASEGUIDARRAYLENGTHCACHE_H__
#define __SBLOCALDATABASEGUIDARRAYLENGTHCACHE_H__

#include <sbILocalDatabaseGUIDArray.h>

#include <nsDataHashtable.h>
#include <nsAutoLock.h>

#include <set>
#include <map>

class sbLocalDatabaseGUIDArrayLengthCache :
    public sbILocalDatabaseGUIDArrayLengthCache
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILOCALDATABASEGUIDARRAYLENGTHCACHE

  sbLocalDatabaseGUIDArrayLengthCache();

protected:
  virtual ~sbLocalDatabaseGUIDArrayLengthCache();

  PRLock* mLock;

  nsDataHashtable<nsStringHashKey, PRUint32> mCachedLengths;
  nsDataHashtable<nsStringHashKey, PRUint32> mCachedNonNullLengths;
};

#endif
