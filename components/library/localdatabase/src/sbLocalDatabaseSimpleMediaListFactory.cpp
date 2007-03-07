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

#include "sbLocalDatabaseSimpleMediaListFactory.h"
#include "sbLocalDatabaseSimpleMediaList.h"
#include "sbLocalDatabaseLibrary.h"
#include "sbLocalDatabaseCID.h"

NS_IMPL_ISUPPORTS1(sbLocalDatabaseSimpleMediaListFactory, sbIMediaListFactory)

sbLocalDatabaseSimpleMediaListFactory::sbLocalDatabaseSimpleMediaListFactory()
{
}

sbLocalDatabaseSimpleMediaListFactory::~sbLocalDatabaseSimpleMediaListFactory()
{
}

nsresult
sbLocalDatabaseSimpleMediaListFactory::Init()
{
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaListFactory::GetLibrary(sbILibrary** aLibrary)
{
  *aLibrary = mLibrary;
  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseSimpleMediaListFactory::SetLibrary(sbILibrary* aLibrary)
{
  mLibrary = aLibrary;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaListFactory::GetNameKey(nsAString& aNameKey)
{
  aNameKey.AssignLiteral("simple");
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaListFactory::CreateMediaList(sbIMediaList** _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaListFactory::InstantiateMediaList(const nsAString& aGuid,
                                                            sbIMediaList** _retval)
{
  nsresult rv;

  /*
   * Make sure the supplied media list guid can be created by this factory
   * by looking its factory contract id up in the database
   */
  sbLocalDatabaseLibrary* library =
    NS_STATIC_CAST(sbLocalDatabaseLibrary*, mLibrary.get());

  nsCAutoString contractId;
  rv = library->GetContractIdForGuid(aGuid, contractId);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!contractId.EqualsLiteral(SB_LOCALDATABASE_SIMPLEMEDIALISTFACTORY_CONTRACTID)) {
    return NS_ERROR_NO_INTERFACE;
  }

  sbLocalDatabaseSimpleMediaList* list =
    new sbLocalDatabaseSimpleMediaList(mLibrary, aGuid);
  NS_ENSURE_TRUE(list, NS_ERROR_OUT_OF_MEMORY);

  rv = list->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = list);
  return NS_OK;
}

