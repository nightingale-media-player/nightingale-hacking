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

#include "sbDatabaseResultStringEnumerator.h"

#include <nsIStringEnumerator.h>

NS_IMPL_ISUPPORTS1(sbDatabaseResultStringEnumerator, nsIStringEnumerator)

// nsIStringEnumerator
nsresult
sbDatabaseResultStringEnumerator::Init()
{
  nsresult rv = mDatabaseResult->GetRowCount(&mLength);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbDatabaseResultStringEnumerator::HasMore(PRBool *_retval)
{
  *_retval = mNextIndex < mLength;
  return NS_OK;
}

NS_IMETHODIMP
sbDatabaseResultStringEnumerator::GetNext(nsAString& _retval)
{
  nsresult rv = mDatabaseResult->GetRowCell(mNextIndex, 0, _retval);
  NS_ENSURE_SUCCESS(rv, rv);
  mNextIndex++;

  return NS_OK;
}

