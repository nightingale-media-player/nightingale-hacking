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

#include "sbLocalDatabaseMediaListViewState.h"

#include <sbILibraryConstraints.h>
#include <sbIPropertyArray.h>

#include <nsIClassInfoImpl.h>
#include <nsIObjectInputStream.h>
#include <nsIObjectOutputStream.h>
#include <nsIMutableArray.h>
#include <nsIProgrammingLanguage.h>
#include <nsArrayUtils.h>
#include <nsComponentManagerUtils.h>
#include <nsMemory.h>

static NS_DEFINE_CID(kMediaListViewStateCID,
                     SB_LOCALDATABASE_MEDIALISTVIEWSTATE_CID);

NS_IMPL_ISUPPORTS4(sbLocalDatabaseMediaListViewState,
                   sbILocalDatabaseMediaListViewState,
                   sbIMediaListViewState,
                   nsISerializable,
                   nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER4(sbLocalDatabaseMediaListViewState,
                             sbILocalDatabaseMediaListViewState,
                             sbIMediaListViewState,
                             nsISerializable,
                             nsIClassInfo)

sbLocalDatabaseMediaListViewState::sbLocalDatabaseMediaListViewState() :
  mInitialized(PR_FALSE)
{
}

sbLocalDatabaseMediaListViewState::sbLocalDatabaseMediaListViewState(sbIMutablePropertyArray* aSort,
                                                                     sbILibraryConstraint* aSearch,
                                                                     sbILibraryConstraint* aFilter,
                                                                     sbLocalDatabaseCascadeFilterSetState* aFilterSet,
                                                                     sbLocalDatabaseTreeViewState* aTreeViewState) :
  mInitialized(PR_TRUE),
  mSort(aSort),
  mSearch(aSearch),
  mFilter(aFilter),
  mFilterSet(aFilterSet),
  mTreeViewState(aTreeViewState)
{
  NS_ASSERTION(aSort, "aSort is null");
}

// sbILocalDatabaseMediaListViewState
NS_IMETHODIMP
sbLocalDatabaseMediaListViewState::GetSort(sbIMutablePropertyArray** aSort)
{
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(aSort);

  NS_ADDREF(*aSort = mSort);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewState::GetSearch(sbILibraryConstraint** aSearch)
{
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(aSearch);

  NS_IF_ADDREF(*aSearch = mSearch);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewState::GetFilter(sbILibraryConstraint** aFilter)
{
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(aFilter);

  NS_IF_ADDREF(*aFilter = mFilter);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewState::GetFilterSet(sbLocalDatabaseCascadeFilterSetState** aFilterSet)
{
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(aFilterSet);

  NS_IF_ADDREF(*aFilterSet = mFilterSet);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewState::GetTreeViewState(sbLocalDatabaseTreeViewState** aTreeViewState)
{
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(aTreeViewState);

  NS_IF_ADDREF(*aTreeViewState = mTreeViewState);
  return NS_OK;
}

// sbIMediaListViewState
NS_IMETHODIMP
sbLocalDatabaseMediaListViewState::ToString(nsAString& aString)
{
  NS_ENSURE_STATE(mInitialized);

  nsresult rv;
  nsString buff;
  nsString temp;

  buff.AppendLiteral("sort: ");
  rv = mSort->ToString(temp);
  NS_ENSURE_SUCCESS(rv, rv);
  buff.Append(temp);

  buff.AppendLiteral(" search: ");
  if (mSearch) {
  rv = mSearch->ToString(temp);
  NS_ENSURE_SUCCESS(rv, rv);
  buff.Append(temp);
  }
  else {
    buff.AppendLiteral("null");
  }

  buff.AppendLiteral(" filter: ");
  if (mFilter) {
  rv = mFilter->ToString(temp);
  NS_ENSURE_SUCCESS(rv, rv);
  buff.Append(temp);
  }
  else {
    buff.AppendLiteral("null");
  }

  buff.AppendLiteral(" filterSet: [");
  if (mFilterSet) {
    rv = mFilterSet->ToString(temp);
    NS_ENSURE_SUCCESS(rv, rv);
    buff.Append(temp);
  }
  buff.AppendLiteral("]");

  buff.AppendLiteral(" treeView: [");
  if (mTreeViewState) {
    rv = mTreeViewState->ToString(temp);
    NS_ENSURE_SUCCESS(rv, rv);
    buff.Append(temp);
  }
  buff.AppendLiteral("]");

  aString = buff;

  return NS_OK;
}

// nsISerializable
NS_IMETHODIMP
sbLocalDatabaseMediaListViewState::Read(nsIObjectInputStream* aStream)
{
  NS_ENSURE_ARG_POINTER(aStream);

  nsresult rv;

  nsCOMPtr<nsISupports> supports;

  rv = aStream->ReadObject(PR_TRUE, getter_AddRefs(supports));  
  NS_ENSURE_SUCCESS(rv, rv);
  mSort = do_QueryInterface(supports, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasSearch;
  rv = aStream->ReadBoolean(&hasSearch);
  NS_ENSURE_SUCCESS(rv, rv);

  if (hasSearch) {
  rv = aStream->ReadObject(PR_TRUE, getter_AddRefs(supports));
  NS_ENSURE_SUCCESS(rv, rv);
  mSearch = do_QueryInterface(supports, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  }

  PRBool hasFilter;
  rv = aStream->ReadBoolean(&hasFilter);
  NS_ENSURE_SUCCESS(rv, rv);

  if (hasFilter) {
  rv = aStream->ReadObject(PR_TRUE, getter_AddRefs(supports));
  NS_ENSURE_SUCCESS(rv, rv);
  mFilter = do_QueryInterface(supports, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  }

  PRBool hasFilterSet;
  rv = aStream->ReadBoolean(&hasFilterSet);
  NS_ENSURE_SUCCESS(rv, rv);

  if (hasFilterSet) {
    mFilterSet = new sbLocalDatabaseCascadeFilterSetState();
    NS_ENSURE_TRUE(mFilterSet, NS_ERROR_OUT_OF_MEMORY);

    rv = mFilterSet->Read(aStream);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRBool hasTreeViewState;
  rv = aStream->ReadBoolean(&hasTreeViewState);
  NS_ENSURE_SUCCESS(rv, rv);

  if (hasTreeViewState) {
    mTreeViewState = new sbLocalDatabaseTreeViewState();
    NS_ENSURE_TRUE(mTreeViewState, NS_ERROR_OUT_OF_MEMORY);

    rv = mTreeViewState->Init();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mTreeViewState->Read(aStream);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mInitialized = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewState::Write(nsIObjectOutputStream* aStream)
{
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(aStream);

  nsresult rv;

  rv = aStream->WriteObject(mSort, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mSearch) {
    rv = aStream->WriteBoolean(PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
  rv = aStream->WriteObject(mSearch, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = aStream->WriteBoolean(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mFilter) {
    rv = aStream->WriteBoolean(PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
  rv = aStream->WriteObject(mFilter, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = aStream->WriteBoolean(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mFilterSet) {
    rv = aStream->WriteBoolean(PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mFilterSet->Write(aStream);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = aStream->WriteBoolean(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mTreeViewState) {
    rv = aStream->WriteBoolean(PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mTreeViewState->Write(aStream);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = aStream->WriteBoolean(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

// nsIClassInfo
NS_IMETHODIMP
sbLocalDatabaseMediaListViewState::GetInterfaces(PRUint32* count, nsIID*** array)
{
  return NS_CI_INTERFACE_GETTER_NAME(sbLocalDatabaseMediaListViewState)(count, array);
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewState::GetHelperForLanguage(PRUint32 language,
                                                        nsISupports** _retval)
{
  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewState::GetContractID(char** aContractID)
{
  *aContractID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewState::GetClassDescription(char** aClassDescription)
{
  *aClassDescription = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewState::GetClassID(nsCID** aClassID)
{
  *aClassID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewState::GetImplementationLanguage(PRUint32* aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewState::GetFlags(PRUint32 *aFlags)
{
  *aFlags = 0;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewState::GetClassIDNoAlloc(nsCID* aClassIDNoAlloc)
{
  NS_ENSURE_ARG_POINTER(aClassIDNoAlloc);
  *aClassIDNoAlloc = kMediaListViewStateCID;
  return NS_OK;
}

