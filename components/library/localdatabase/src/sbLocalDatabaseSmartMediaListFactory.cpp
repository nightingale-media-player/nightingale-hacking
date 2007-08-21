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
#include <sbIPropertyArray.h>

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsComponentManagerUtils.h>
#include "sbLocalDatabaseCID.h"
#include "sbLocalDatabaseSmartMediaList.h"
#include <sbStandardProperties.h>
#include <sbPropertiesCID.h>

#define SB_SMART_MEDIALIST_FACTORY_TYPE "smart"
#define SB_SMART_MEDIALIST_METRICS_TYPE "smart"

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

  // Get the guid of the media list used to store the query result of
  // the smart media list
  nsAutoString dataGuid;
  rv = aInner->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_STORAGEGUID),
                           dataGuid);

  // If the dataGuid is not set, then this must be the first time we are
  // instantiating this list
  if (dataGuid.IsEmpty()) {

    // Create the simple media list used to store the query result and store the
    // guid as a property of the list
    nsCOMPtr<sbILibrary> library;
    rv = aInner->GetLibrary(getter_AddRefs(library));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMutablePropertyArray> properties =
      do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = properties->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                                    NS_LITERAL_STRING("1"));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaList> dataList;
    rv = library->CreateMediaList(NS_LITERAL_STRING("simple"),
                                  properties,
                                  getter_AddRefs(dataList));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString guid;
    rv = dataList->GetGuid(guid);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aInner->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_STORAGEGUID),
                             guid);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsRefPtr<sbLocalDatabaseSmartMediaList> newSmartList(
    new sbLocalDatabaseSmartMediaList());
  NS_ENSURE_TRUE(newSmartList, NS_ERROR_OUT_OF_MEMORY);

  rv = newSmartList->Init(aInner);
  NS_ENSURE_SUCCESS(rv, rv);


  // Get customType so we don't overwrite it.  Grrrr.
  nsAutoString customType;
  rv = newSmartList->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CUSTOMTYPE), customType );
  if (customType.IsEmpty()) {
    // Set new customType for use by metrics.
    rv = newSmartList->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_CUSTOMTYPE),
                                   NS_LITERAL_STRING(SB_SMART_MEDIALIST_METRICS_TYPE));
  }

  NS_ADDREF(*_retval = newSmartList);
  return NS_OK;
}
