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

#include "sbLocalDatabaseViewMediaListFactory.h"

#include <sbIMediaList.h>
#include <sbILibrary.h>
#include <sbILocalDatabaseLibrary.h>

#include <nsAutoPtr.h>
#include "sbLocalDatabaseViewMediaList.h"
#include "sbLocalDatabaseCID.h"

#define SB_VIEW_MEDIALIST_FACTORY_TYPE "view"

NS_IMPL_ISUPPORTS1(sbLocalDatabaseViewMediaListFactory, sbIMediaListFactory)

NS_IMETHODIMP
sbLocalDatabaseViewMediaListFactory::GetType(nsAString& aType)
{
  aType.AssignLiteral(SB_VIEW_MEDIALIST_FACTORY_TYPE);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseViewMediaListFactory::GetContractID(nsACString& aContractID)
{
  aContractID.AssignLiteral(SB_LOCALDATABASE_VIEWMEDIALISTFACTORY_CONTRACTID);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseViewMediaListFactory::CreateMediaList(sbIMediaItem* aInner,
                                                     sbIMediaList** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbILibrary> library;
  nsresult rv = aInner->GetLibrary(getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);

  // This should always succeed because no one else will be making instances of
  // this class.
  nsCOMPtr<sbILocalDatabaseLibrary> localLibrary =
    do_QueryInterface(library, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString guid;
  rv = aInner->GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoPtr<sbLocalDatabaseViewMediaList>
    newMediaList(new sbLocalDatabaseViewMediaList());
  NS_ENSURE_TRUE(newMediaList, NS_ERROR_OUT_OF_MEMORY);

  rv = newMediaList->Init(localLibrary, guid);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = newMediaList.forget());
  return NS_OK;
}
