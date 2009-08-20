/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#ifndef SBARRAYUTILS_H_
#define SBARRAYUTILS_H_

#include <nsComponentManagerUtils.h>
#include <nsIMutableArray.h>

#include <sbArray.h>

template <class T>
nsresult sbCOMArrayTonsIArray(T & aCOMArray, nsIArray ** aOutArray)
{
  nsresult rv;
  nsCOMPtr<nsIMutableArray> outArray =
    do_CreateInstance(SB_THREADSAFE_ARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 const length = aCOMArray.Count();
  for (PRInt32 index = 0; index < length; ++index) {
    rv = outArray->AppendElement(aCOMArray.ObjectAt(index), PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = CallQueryInterface(outArray, aOutArray);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
#endif /* SBARRAYUTILS_H_ */
