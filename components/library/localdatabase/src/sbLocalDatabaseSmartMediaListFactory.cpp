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

#include "sbLocalDatabaseSmartMediaListFactory.h"

#include <sbIDatabaseQuery.h>
#include <sbILibrary.h>
#include <sbILocalDatabaseLibrary.h>
#include <sbIMediaList.h>
#include <sbIMediaItem.h>

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include "sbLocalDatabaseCID.h"
#include "sbLocalDatabaseSmartMediaList.h"

#define SB_SMART_MEDIALIST_FACTORY_TYPE "smart"

NS_IMPL_ISUPPORTS1(sbLocalDatabaseSmartMediaListFactory, 
                   sbIMediaListFactory)

/**
 * See sbIMediaListFactory.idl
 */
NS_IMETHODIMP
sbLocalDatabaseSmartMediaListFactory::GetType(nsAString& aType)
{
  aType.AssignLiteral(SB_SMART_MEDIALIST_FACTORY_TYPE);
  return NS_OK;
}

/**
 * See sbIMediaListFactory.idl
 */
NS_IMETHODIMP
sbLocalDatabaseSmartMediaListFactory::GetContractID(nsACString& aContractID)
{
  aContractID.AssignLiteral(SB_LOCALDATABASE_SMARTMEDIALISTFACTORY_CONTRACTID);
  return NS_OK;
}

/**
 * See sbIMediaListFactory.idl
 */
NS_IMETHODIMP
sbLocalDatabaseSmartMediaListFactory::CreateMediaList(sbIMediaItem* aInner,
                                                      sbIMediaList** _retval)
{
  NS_ENSURE_ARG_POINTER(aInner);
  NS_ENSURE_ARG_POINTER(_retval);
  
  nsresult rv;
  nsAutoString dataGuid;

  // Get the guid of the media list used to store the query result of
  // the smart media list
  rv = aInner->GetProperty(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#storageGuid"),
                        dataGuid);

  // If the dataGuid is not set, then this must be the first time we are
  // instantiating this list
  if (dataGuid.Equals(EmptyString())) {

    // Create the simple media list used to store the query result and store the
    // guid as a property of the list
    nsCOMPtr<sbILibrary> library;
    rv = aInner->GetLibrary(getter_AddRefs(library));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaList> dataList;
    rv = library->CreateMediaList(NS_LITERAL_STRING("simple"), 
                                  getter_AddRefs(dataList));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString guid;
    rv = dataList->GetGuid(guid);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aInner->
      SetProperty(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#storageGuid"), 
                  guid);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<sbILocalDatabaseSmartMediaList> newSmartList = 
    new sbLocalDatabaseSmartMediaList();

  rv = newSmartList->Init(aInner);
  NS_ENSURE_SUCCESS(rv, rv);
  
  *_retval = newSmartList;
  NS_ADDREF(*_retval);

  return NS_OK;
}
