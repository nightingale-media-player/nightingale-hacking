/*
//
// BEGIN NIGHTINGALE GPL
// 
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

/** 
 * \file  sbLibraryChangeset.cpp
 * \brief sbLibraryChangeset Implementation.
 */
#include "sbLibraryChangeset.h"

#include <nsIClassInfoImpl.h>
#include <nsIProgrammingLanguage.h>
#include <sbIMediaItem.h>
#include <sbIMediaList.h>

#include <nsAutoLock.h>
#include <nsMemory.h>

//-----------------------------------------------------------------------------
// sbPropertyChange
//-----------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ADDREF(sbPropertyChange)
NS_IMPL_THREADSAFE_RELEASE(sbPropertyChange)

NS_INTERFACE_MAP_BEGIN(sbPropertyChange)
  NS_IMPL_QUERY_CLASSINFO(sbPropertyChange)
  //XXXAus: static_cast does not work in this case, 
  //reinterpret_cast to nsISupports is necessary
  if ( aIID.Equals(NS_GET_IID(sbPropertyChange)) )
    foundInterface = reinterpret_cast<nsISupports*>(this);
  else
  NS_INTERFACE_MAP_ENTRY(sbIPropertyChange)
  NS_INTERFACE_MAP_ENTRY(sbIChangeOperation)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, sbIPropertyChange)
NS_INTERFACE_MAP_END

NS_IMPL_CI_INTERFACE_GETTER2(sbPropertyChange,
                             sbIPropertyChange,
                             sbIChangeOperation)

NS_DECL_CLASSINFO(sbPropertyChange)
NS_IMPL_THREADSAFE_CI(sbPropertyChange)

sbPropertyChange::sbPropertyChange()
: mOperation(sbIChangeOperation::UNKNOWN)
, mOperationLock(nsnull)
, mIDLock(nsnull)
, mOldValueLock(nsnull)
, mNewValueLock(nsnull)
{

}

sbPropertyChange::~sbPropertyChange()
{
  if(mOperationLock) {
    nsAutoLock::DestroyLock(mOperationLock);
  }
  if(mIDLock) {
    nsAutoLock::DestroyLock(mIDLock);
  }
  if(mOldValueLock) {
    nsAutoLock::DestroyLock(mOldValueLock);
  }
  if(mNewValueLock) {
    nsAutoLock::DestroyLock(mNewValueLock);
  }
}

nsresult sbPropertyChange::Init()
{
  mOperationLock = nsAutoLock::NewLock("sbPropertyChange::mOperationLock");
  NS_ENSURE_TRUE(mOperationLock, NS_ERROR_OUT_OF_MEMORY);

  mIDLock = nsAutoLock::NewLock("sbPropertyChange::mIDLock");
  NS_ENSURE_TRUE(mIDLock, NS_ERROR_OUT_OF_MEMORY);

  mOldValueLock = nsAutoLock::NewLock("sbPropertyChange::mOldValueLock");
  NS_ENSURE_TRUE(mOldValueLock, NS_ERROR_OUT_OF_MEMORY);

  mNewValueLock = nsAutoLock::NewLock("sbPropertyChange::mNewValueLock");
  NS_ENSURE_TRUE(mNewValueLock, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

nsresult sbPropertyChange::InitWithValues(PRUint32 aOperation,
                                          const nsAString &aID,
                                          const nsAString &aOldValue, 
                                          const nsAString &aNewValue)
{
  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  {
    nsAutoLock lock(mOperationLock);
    mOperation = aOperation;
  }

  {
    nsAutoLock lock(mIDLock);
    mID = aID;
  }

  {
    nsAutoLock lock(mOldValueLock);
    mOldValue = aOldValue;
  }

  {
    nsAutoLock lock(mNewValueLock);
    mNewValue = aNewValue;
  }

  return NS_OK;
}

nsresult sbPropertyChange::SetOperation(PRUint32 aOperation)
{
  nsAutoLock lock(mOperationLock);
  mOperation = aOperation;
  return NS_OK;
}

nsresult sbPropertyChange::SetID(const nsAString &aID)
{
  nsAutoLock lock(mIDLock);
  mID = aID;
  return NS_OK;
}

nsresult sbPropertyChange::SetOldValue(const nsAString &aOldValue)
{
  nsAutoLock lock(mOldValueLock);
  mOldValue = aOldValue;
  return NS_OK;
}

nsresult sbPropertyChange::SetNewValue(const nsAString &aNewValue)
{
  nsAutoLock lock(mNewValueLock);
  mNewValue = aNewValue;
  return NS_OK;
}

/* readonly attribute unsigned long operation; */
NS_IMETHODIMP sbPropertyChange::GetOperation(PRUint32 *aOperation)
{
  NS_ENSURE_ARG_POINTER(aOperation);
  
  nsAutoLock lock(mOperationLock);
  *aOperation = mOperation;
  
  return NS_OK;
}

/* readonly attribute AString id; */
NS_IMETHODIMP sbPropertyChange::GetId(nsAString & aId)
{
  nsAutoLock lock(mIDLock);
  aId.Assign(mID);

  return NS_OK;
}

/* readonly attribute AString oldValue; */
NS_IMETHODIMP sbPropertyChange::GetOldValue(nsAString & aOldValue)
{
  nsAutoLock lock(mOldValueLock);
  aOldValue.Assign(mOldValue);

  return NS_OK;
}

/* readonly attribute AString newValue; */
NS_IMETHODIMP sbPropertyChange::GetNewValue(nsAString & aNewValue)
{
  nsAutoLock lock(mNewValueLock);
  aNewValue.Assign(mNewValue);
  
  return NS_OK;
}


//-----------------------------------------------------------------------------
// sbLibraryChange
//-----------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ADDREF(sbLibraryChange)
NS_IMPL_THREADSAFE_RELEASE(sbLibraryChange)

NS_INTERFACE_MAP_BEGIN(sbLibraryChange)
  NS_IMPL_QUERY_CLASSINFO(sbLibraryChange)
  //XXXAus: static_cast does not work in this case, 
  //reinterpret_cast to nsISupports is necessary
  if ( aIID.Equals(NS_GET_IID(sbLibraryChange)) )
    foundInterface = reinterpret_cast<nsISupports*>(this);
  else
  NS_INTERFACE_MAP_ENTRY(sbILibraryChange)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, sbILibraryChange)
NS_INTERFACE_MAP_END

NS_IMPL_CI_INTERFACE_GETTER2(sbLibraryChange,
                             sbILibraryChange,
                             sbIChangeOperation)

NS_DECL_CLASSINFO(sbLibraryChange)
NS_IMPL_THREADSAFE_CI(sbLibraryChange)

sbLibraryChange::sbLibraryChange()
: mOperation(sbIChangeOperation::UNKNOWN)
, mOperationLock(nsnull)
, mTimestamp(0)
, mTimestampLock(nsnull)
, mItemLock(nsnull)
, mPropertiesLock(nsnull)
{
}

sbLibraryChange::~sbLibraryChange()
{
  if(mOperationLock) {
    nsAutoLock::DestroyLock(mOperationLock);
  }
  if(mTimestampLock) {
    nsAutoLock::DestroyLock(mTimestampLock);
  }
  if(mItemLock) {
    nsAutoLock::DestroyLock(mItemLock);
  }
  if(mPropertiesLock) {
    nsAutoLock::DestroyLock(mPropertiesLock);
  }
}

nsresult sbLibraryChange::Init()
{
  mOperationLock = nsAutoLock::NewLock("sbLibraryChange::mOperationLock");
  NS_ENSURE_TRUE(mOperationLock, NS_ERROR_OUT_OF_MEMORY);

  mTimestampLock = nsAutoLock::NewLock("sbLibraryChange::mOperationLock");
  NS_ENSURE_TRUE(mTimestampLock, NS_ERROR_OUT_OF_MEMORY);

  mItemLock = nsAutoLock::NewLock("sbLibraryChange::mItemLock");
  NS_ENSURE_TRUE(mItemLock, NS_ERROR_OUT_OF_MEMORY);

  mPropertiesLock = nsAutoLock::NewLock("sbLibraryChange::mPropertiesLock");
  NS_ENSURE_TRUE(mPropertiesLock, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

nsresult sbLibraryChange::InitWithValues(PRUint32 aOperation,
                                         PRUint64 aTimestamp, 
                                         sbIMediaItem *aSourceItem,
                                         sbIMediaItem *aDestinationItem,
                                         nsIArray *aProperties)
{
  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);
  
  {
    nsAutoLock lock(mOperationLock);
    mOperation = aOperation;
  }
  
  {
    nsAutoLock lock(mTimestampLock);
    mTimestamp = aTimestamp;
  }

  if(aSourceItem) {
    nsAutoLock lock(mItemLock);
    mSourceItem = aSourceItem;
  }

  if(aDestinationItem) {
    nsAutoLock lock(mItemLock);
    mDestinationItem = aDestinationItem;
  }

  if(aProperties)
  {
    nsAutoLock lock(mPropertiesLock);
    mProperties = aProperties;
  }
  
  return NS_OK;
}

nsresult sbLibraryChange::SetOperation(PRUint32 aOperation)
{
  nsAutoLock lock(mOperationLock);
  mOperation = aOperation;

  return NS_OK;
}

nsresult sbLibraryChange::SetTimestamp(PRUint64 aTimestamp)
{
  nsAutoLock lock(mTimestampLock);
  mTimestamp = aTimestamp;

  return NS_OK;
}

nsresult sbLibraryChange::SetItems(sbIMediaItem *aSourceItem,
                                   sbIMediaItem *aDestinationItem)
{
  NS_ENSURE_ARG_POINTER(aSourceItem);

  {
    nsAutoLock lock(mItemLock);
    mSourceItem = aSourceItem;
    mDestinationItem = aDestinationItem ? aDestinationItem : aSourceItem;
  }

  return NS_OK;
}

nsresult sbLibraryChange::SetProperties(nsIArray *aProperties)
{
  NS_ENSURE_ARG_POINTER(aProperties);

  nsAutoLock lock(mPropertiesLock);
  mProperties = aProperties;

  return NS_OK;
}

/* readonly attribute unsigned long operation; */
NS_IMETHODIMP sbLibraryChange::GetOperation(PRUint32 *aOperation)
{
  NS_ENSURE_ARG_POINTER(aOperation);

  nsAutoLock lock(mOperationLock);
  *aOperation = mOperation;

  return NS_OK;
}

/* readonly attribute unsigned long long timestamp; */
NS_IMETHODIMP sbLibraryChange::GetTimestamp(PRUint64 *aTimestamp)
{
  NS_ENSURE_ARG_POINTER(aTimestamp);

  nsAutoLock lock(mTimestampLock);
  *aTimestamp = mTimestamp;

  return NS_OK;
}

/* readonly attribute sbIMediaItem sourceItem; */
NS_IMETHODIMP sbLibraryChange::GetSourceItem(sbIMediaItem * *aItem)
{
  NS_ENSURE_ARG_POINTER(aItem);
  
  nsAutoLock lock(mItemLock);
  NS_IF_ADDREF(*aItem = mSourceItem);

  return *aItem ? NS_OK : NS_ERROR_NOT_AVAILABLE;
}

/* readonly attribute sbIMediaItem destinationItem; */
NS_IMETHODIMP sbLibraryChange::GetDestinationItem(sbIMediaItem * *aItem)
{
  NS_ENSURE_ARG_POINTER(aItem);

  nsAutoLock lock(mItemLock);
  NS_IF_ADDREF(*aItem = mDestinationItem);

  return *aItem ? NS_OK : NS_ERROR_NOT_AVAILABLE;
}

/* readonly attribute boolean itemIsList; */
NS_IMETHODIMP sbLibraryChange::GetItemIsList(PRBool *aItemIsList)
{
  NS_ENSURE_ARG_POINTER(aItemIsList);
  
  nsresult rv;
  nsCOMPtr<sbIMediaList> mediaList;
  if (mSourceItem) {
    mediaList = do_QueryInterface(mSourceItem, &rv);
  } else {
    // no source, use dest
    mediaList = do_QueryInterface(mDestinationItem, &rv);
  }
  
  if(rv == NS_ERROR_NO_INTERFACE) {
    *aItemIsList = PR_FALSE;
    return NS_OK;
  }
  else if(mediaList != 0) {
    *aItemIsList = PR_TRUE;
    return NS_OK;
  }

  return NS_ERROR_UNEXPECTED;
}

/* readonly attribute nsIArray properties; */
NS_IMETHODIMP sbLibraryChange::GetProperties(nsIArray * *aProperties)
{
  NS_ENSURE_ARG_POINTER(aProperties);

  nsAutoLock lock(mPropertiesLock);
  NS_IF_ADDREF(*aProperties = mProperties);

  return *aProperties ? NS_OK : NS_ERROR_NOT_AVAILABLE;
}

//-----------------------------------------------------------------------------
// sbLibraryChangeset
//-----------------------------------------------------------------------------


NS_IMPL_THREADSAFE_ADDREF(sbLibraryChangeset)
NS_IMPL_THREADSAFE_RELEASE(sbLibraryChangeset)

NS_INTERFACE_MAP_BEGIN(sbLibraryChangeset)
  NS_IMPL_QUERY_CLASSINFO(sbLibraryChangeset)
  //XXXAus: static_cast does not work in this case, 
  //reinterpret_cast to nsISupports is necessary
  if ( aIID.Equals(NS_GET_IID(sbLibraryChangeset)) )
    foundInterface = reinterpret_cast<nsISupports*>(this);
  else
  NS_INTERFACE_MAP_ENTRY(sbILibraryChangeset)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, sbILibraryChangeset)
NS_INTERFACE_MAP_END

NS_IMPL_CI_INTERFACE_GETTER1(sbLibraryChangeset,
                             sbILibraryChangeset)

NS_DECL_CLASSINFO(sbLibraryChangeset)
NS_IMPL_THREADSAFE_CI(sbLibraryChangeset)

sbLibraryChangeset::sbLibraryChangeset()
: mSourceListsLock(nsnull)
, mDestinationListLock(nsnull)
, mChangesLock(nsnull)
{
}

sbLibraryChangeset::~sbLibraryChangeset()
{
  if(mSourceListsLock) {
    nsAutoLock::DestroyLock(mSourceListsLock);
  }
  if(mDestinationListLock) {
    nsAutoLock::DestroyLock(mDestinationListLock);
  }
  if(mChangesLock) {
    nsAutoLock::DestroyLock(mChangesLock);
  }
}

nsresult sbLibraryChangeset::Init() 
{
  mSourceListsLock = nsAutoLock::NewLock("sbLibraryChangeset::mSourceListsLock");
  NS_ENSURE_TRUE(mSourceListsLock, NS_ERROR_OUT_OF_MEMORY);

  mDestinationListLock = nsAutoLock::NewLock("sbLibraryChangeset::mDestinationListLock");
  NS_ENSURE_TRUE(mDestinationListLock, NS_ERROR_OUT_OF_MEMORY);

  mChangesLock = nsAutoLock::NewLock("sbLibraryChangeset::mChangesLock");
  NS_ENSURE_TRUE(mChangesLock, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

nsresult sbLibraryChangeset::InitWithValues(nsIArray *aSourceLists,
                                            sbIMediaList *aDestinationList,
                                            nsIArray *aChanges)
{
  NS_ENSURE_ARG_POINTER(aSourceLists);
  NS_ENSURE_ARG_POINTER(aDestinationList);
  NS_ENSURE_ARG_POINTER(aChanges);

  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  {
    nsAutoLock lock(mSourceListsLock);
    mSourceLists = aSourceLists;
  }

  {
    nsAutoLock lock(mDestinationListLock);
    mDestinationList = aDestinationList;
  }
  
  {
    nsAutoLock lock(mChangesLock);
    mChanges = aChanges;
  }
  
  return NS_OK;
}

nsresult sbLibraryChangeset::SetSourceLists(nsIArray *aSourceLists)
{
  NS_ENSURE_ARG_POINTER(aSourceLists);

  nsAutoLock lock(mSourceListsLock);
  mSourceLists = aSourceLists;

  return NS_OK;
}

nsresult sbLibraryChangeset::SetDestinationList(sbIMediaList *aDestinationList)
{
  NS_ENSURE_ARG_POINTER(aDestinationList);

  nsAutoLock lock(mDestinationListLock);
  mDestinationList = aDestinationList;

  return NS_OK;
}

nsresult sbLibraryChangeset::SetChanges(nsIArray *aChanges)
{
  NS_ENSURE_ARG_POINTER(aChanges);

  nsAutoLock lock(mChangesLock);
  mChanges = aChanges;

  return NS_OK;
}

/* readonly attribute sbIMediaList sourceList; */
NS_IMETHODIMP sbLibraryChangeset::GetSourceLists(nsIArray * *aSourceLists)
{
  NS_ENSURE_ARG_POINTER(aSourceLists);

  nsAutoLock lock(mSourceListsLock);
  NS_IF_ADDREF(*aSourceLists = mSourceLists);

  return *aSourceLists ? NS_OK : NS_ERROR_NOT_AVAILABLE;
}

/* readonly attribute sbIMediaList destinationList; */
NS_IMETHODIMP sbLibraryChangeset::GetDestinationList(sbIMediaList * *aDestinationList)
{
  NS_ENSURE_ARG_POINTER(aDestinationList);

  nsAutoLock lock(mDestinationListLock);
  NS_IF_ADDREF(*aDestinationList = mDestinationList);

  return *aDestinationList ? NS_OK : NS_ERROR_NOT_AVAILABLE;
}

/* readonly attribute nsIArray changes; */
NS_IMETHODIMP sbLibraryChangeset::GetChanges(nsIArray * *aChanges)
{
  NS_ENSURE_ARG_POINTER(aChanges);

  nsAutoLock lock(mChangesLock);
  NS_IF_ADDREF(*aChanges = mChanges);

  return *aChanges ? NS_OK : NS_ERROR_NOT_AVAILABLE;
}
