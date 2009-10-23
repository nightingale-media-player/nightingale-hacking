/* vim: set sw=2 :miv */
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

#include "sbMediaListEnumSingleItemHelper.h"

#include <sbIMediaItem.h>
#include <sbIMediaList.h>

NS_IMPL_ISUPPORTS1(sbMediaListEnumSingleItemHelper,
                   sbIMediaListEnumerationListener)

sbMediaListEnumSingleItemHelper* sbMediaListEnumSingleItemHelper::New()
{
  return new sbMediaListEnumSingleItemHelper();
}

sbMediaListEnumSingleItemHelper::sbMediaListEnumSingleItemHelper()
{
  /* member initializers and constructor code */
}

sbMediaListEnumSingleItemHelper::~sbMediaListEnumSingleItemHelper()
{
  /* destructor code */
}

already_AddRefed<sbIMediaItem> sbMediaListEnumSingleItemHelper::GetItem()
{
  return nsCOMPtr<sbIMediaItem>(mItem).forget();
}

already_AddRefed<sbIMediaList> sbMediaListEnumSingleItemHelper::GetList()
{
  return nsCOMPtr<sbIMediaList>(mList).forget();
}

/* unsigned short onEnumerationBegin (in sbIMediaList aMediaList); */
NS_IMETHODIMP
sbMediaListEnumSingleItemHelper::OnEnumerationBegin(sbIMediaList *aMediaList,
                                                    PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  mItem = nsnull;
  mList = nsnull;
  *_retval = sbIMediaListEnumerationListener::CONTINUE;
  return NS_OK;
}

/* unsigned short onEnumeratedItem (in sbIMediaList aMediaList, in sbIMediaItem aMediaItem); */
NS_IMETHODIMP
sbMediaListEnumSingleItemHelper::OnEnumeratedItem(sbIMediaList *aMediaList,
                                                  sbIMediaItem *aMediaItem,
                                                  PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  mItem = aMediaItem;
  mList = aMediaList;
  *_retval = sbIMediaListEnumerationListener::CANCEL;
  return NS_OK;
}

/* void onEnumerationEnd (in sbIMediaList aMediaList, in nsresult aStatusCode); */
NS_IMETHODIMP
sbMediaListEnumSingleItemHelper::OnEnumerationEnd(sbIMediaList *aMediaList,
                                                  nsresult aStatusCode)
{
  return NS_OK;
}
