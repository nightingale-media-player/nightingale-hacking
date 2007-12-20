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

#ifndef __SBLIBRARYUTILS_H__
#define __SBLIBRARYUTILS_H__

#include <nsCOMPtr.h>
#include <pratom.h>

class sbLibraryBatchHelper
{
public:
  sbLibraryBatchHelper() :
    mDepth(0)
  {
    MOZ_COUNT_CTOR(sbLibraryBatchHelper);
  }

  ~sbLibraryBatchHelper()
  {
    MOZ_COUNT_DTOR(sbLibraryBatchHelper);
  }

  void Begin()
  {
    NS_ASSERTION(mDepth >= 0, "Illegal batch depth, mismatched calls!");
    PR_AtomicIncrement(&mDepth);
  }

  void End()
  {
    PRInt32 depth = PR_AtomicDecrement(&mDepth);
    NS_ASSERTION(depth >= 0, "Illegal batch depth, mismatched calls!");
  }

  PRUint32 Depth()
  {
    return (PRUint32) mDepth;
  }

  PRBool IsActive()
  {
    return mDepth > 0;
  }

private:
  PRInt32 mDepth;
};

#endif // __SBLIBRARYUTILS_H__

