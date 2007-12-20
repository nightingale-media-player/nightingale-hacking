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

#ifndef __SBDATABASERESULTSTRINGENUMERATOR_H__
#define __SBDATABASERESULTSTRINGENUMERATOR_H__

#include <nsIStringEnumerator.h>
#include <nsCOMPtr.h>
#include <sbIDatabaseResult.h>

class sbDatabaseResultStringEnumerator : public nsIStringEnumerator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTRINGENUMERATOR

  sbDatabaseResultStringEnumerator(sbIDatabaseResult* aDatabaseResult) :
    mDatabaseResult(aDatabaseResult),
    mNextIndex(0),
    mLength(0)
  {
  }

  nsresult Init();
private:
  nsCOMPtr<sbIDatabaseResult> mDatabaseResult;
  PRUint32 mNextIndex;
  PRUint32 mLength;
};

#endif /* __SBDATABASERESULTSTRINGENUMERATOR_H__ */

