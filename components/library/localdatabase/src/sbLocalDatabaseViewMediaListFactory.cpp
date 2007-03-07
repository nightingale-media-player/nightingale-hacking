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
#include "sbLocalDatabaseViewMediaList.h"
#include "sbLocalDatabaseLibrary.h"
#include "sbLocalDatabaseCID.h"

#include <nsComponentManagerUtils.h>
#include <sbISQLBuilder.h>
#include <sbSQLBuilderCID.h>
#include <sbIMediaList.h>
#include <sbIDatabaseQuery.h>
#include <DatabaseQuery.h>
#include <sbIDatabaseResult.h>

NS_IMPL_ISUPPORTS1(sbLocalDatabaseViewMediaListFactory, sbIMediaListFactory)

sbLocalDatabaseViewMediaListFactory::sbLocalDatabaseViewMediaListFactory()
{
}

sbLocalDatabaseViewMediaListFactory::~sbLocalDatabaseViewMediaListFactory()
{
}

NS_IMETHODIMP
sbLocalDatabaseViewMediaListFactory::Init()
{
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseViewMediaListFactory::GetLibrary(sbILibrary** aLibrary)
{
  *aLibrary = mLibrary;
  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseViewMediaListFactory::SetLibrary(sbILibrary* aLibrary)
{
  mLibrary = aLibrary;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseViewMediaListFactory::GetNameKey(nsAString& aNameKey)
{
  aNameKey.AssignLiteral("view");
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseViewMediaListFactory::CreateMediaList(sbIMediaList** _retval)
{
  nsresult rv;

  // TODO: actually create a new thing in the db

  sbLocalDatabaseViewMediaList* list =
    new sbLocalDatabaseViewMediaList(mLibrary, EmptyString());
  NS_ENSURE_TRUE(list, NS_ERROR_OUT_OF_MEMORY);

  rv = list->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = list);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseViewMediaListFactory::InstantiateMediaList(const nsAString& aGuid,
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

  if (!contractId.EqualsLiteral(SB_LOCALDATABASE_VIEWMEDIALISTFACTORY_CONTRACTID)) {
    return NS_ERROR_NO_INTERFACE;
  }

  sbLocalDatabaseViewMediaList* list =
    new sbLocalDatabaseViewMediaList(mLibrary, aGuid);
  NS_ENSURE_TRUE(list, NS_ERROR_OUT_OF_MEMORY);

  rv = list->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = list);
  return NS_OK;
}

