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

#include "sbTArrayStringEnumerator.h"

NS_IMPL_ISUPPORTS1(sbTArrayStringEnumerator, nsIStringEnumerator)

sbTArrayStringEnumerator::sbTArrayStringEnumerator(nsTArray<nsString>* aStringArray) :
  mNextIndex(0)
{
  mStringArray.InsertElementsAt(0, *aStringArray);
};

NS_IMETHODIMP
sbTArrayStringEnumerator::HasMore(PRBool *_retval)
{
  *_retval = mNextIndex < mStringArray.Length();
  return NS_OK;
};

NS_IMETHODIMP
sbTArrayStringEnumerator::GetNext(nsAString& _retval)
{
  if (mNextIndex < mStringArray.Length()) {
    _retval = mStringArray[mNextIndex];
    mNextIndex++;
    return NS_OK;
  }
  else {
    return NS_ERROR_NOT_AVAILABLE;
  }
};


