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

#include <prlock.h>

/**
 * This class can be used for stack-based locking when the overhead of the
 * nsAutoLock deadlock detection is too great (e.g. for classes that have many
 * instances created and deleted in rapid succession).
 *
 * If you don't expect to have hundreds of instances running around then please
 * favor nsAutoLock over this in order to take advantage of deadlock detection.
 *
 * DON'T use this class with locks created with nsAutoLock::NewLock!
 */
class sbSimpleAutoLock
{
public:
  sbSimpleAutoLock(PRLock* aLock)
  : mLock(aLock)
  {
    NS_ASSERTION(aLock, "Null lock!");
    PR_Lock(aLock);
  }

  ~sbSimpleAutoLock()
  {
    PR_Unlock(mLock);
  }

private:
  // Prevent copying and assigmnent
  sbSimpleAutoLock(void) {}
  sbSimpleAutoLock(sbSimpleAutoLock& /*aOther*/) {}
  sbSimpleAutoLock& operator =(sbSimpleAutoLock& /*aOther*/) { return *this; }

  // Prevent heap usage
  static void* operator new(size_t /*aSize*/) CPP_THROW_NEW { return nsnull; }
  static void operator delete(void* /*aMemory*/) {}

  PRLock* mLock;
};
