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

#include "sbTArrayStringEnumerator.h"

NS_IMPL_ISUPPORTS1(sbTArrayStringEnumerator, nsIStringEnumerator)

sbTArrayStringEnumerator * 
sbTArrayStringEnumerator::New(const sbStringArray & aStringArray) {
  return new sbTArrayStringEnumerator(&aStringArray);
}

sbTArrayStringEnumerator::sbTArrayStringEnumerator(const sbStringArray* aStringArray) :
  mNextIndex(0)
{
  mStringArray.InsertElementsAt(0, *aStringArray);
};

sbTArrayStringEnumerator::sbTArrayStringEnumerator(const sbCStringArray* aStringArray) :
  mNextIndex(0)
{
  for ( PRUint32 index = 0; index < aStringArray->Length(); index ++ ) {
    mStringArray.AppendElement(NS_ConvertUTF8toUTF16((*aStringArray)[index]));
  }
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

NS_IMPL_ISUPPORTS1(sbTArrayCStringEnumerator, nsIUTF8StringEnumerator)

sbTArrayCStringEnumerator::sbTArrayCStringEnumerator(const sbStringArray* aStringArray) :
  mNextIndex(0)
{
  for ( PRUint32 index = 0; index < aStringArray->Length(); index ++ ) {
    mCStringArray.AppendElement(NS_ConvertUTF16toUTF8((*aStringArray)[index]));
  }
};

sbTArrayCStringEnumerator::sbTArrayCStringEnumerator(const sbCStringArray* aCStringArray) :
  mNextIndex(0)
{
  mCStringArray.InsertElementsAt(0, *aCStringArray);
};

NS_IMETHODIMP
sbTArrayCStringEnumerator::HasMore(PRBool *_retval)
{
  *_retval = mNextIndex < mCStringArray.Length();
  return NS_OK;
};

NS_IMETHODIMP
sbTArrayCStringEnumerator::GetNext(nsACString& _retval)
{
  if (mNextIndex < mCStringArray.Length()) {
    _retval = mCStringArray[mNextIndex];
    mNextIndex++;
    return NS_OK;
  }
  else {
    return NS_ERROR_NOT_AVAILABLE;
  }
};


