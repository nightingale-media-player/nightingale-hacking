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

#ifndef __SBHASHKEYS_H__
#define __SBHASHKEYS_H__

#include <pldhash.h>

class sbUint64HashKey : public PLDHashEntryHdr
{
public:
  typedef const PRUint64& KeyType;
  typedef const PRUint64* KeyTypePointer;

  sbUint64HashKey(KeyTypePointer aKey) : mValue(*aKey) { }
  sbUint64HashKey(const sbUint64HashKey& toCopy) : mValue(toCopy.mValue) { }
  ~sbUint64HashKey() { }

  KeyType GetKey() const { return mValue; }
  PRBool KeyEquals(KeyTypePointer aKey) const { return *aKey == mValue; }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey) { return *aKey; }
  enum { ALLOW_MEMMOVE = PR_TRUE };

private:
  const PRUint64 mValue;
};


#endif /* __SBHASHKEYS_H__ */

