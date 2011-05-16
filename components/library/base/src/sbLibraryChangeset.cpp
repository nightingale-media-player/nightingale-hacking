/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2011 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
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

#include <nsIMutableArray.h>
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
{

}

sbPropertyChange::~sbPropertyChange()
{
}

nsresult sbPropertyChange::InitWithValues(PRUint32 aOperation,
                                          const nsAString &aID,
                                          const nsAString &aOldValue,
                                          const nsAString &aNewValue)
{
  mOperation = aOperation;
  mID = aID;
  mOldValue = aOldValue;
  mNewValue = aNewValue;

  return NS_OK;
}

nsresult sbPropertyChange::SetOperation(PRUint32 aOperation)
{
  mOperation = aOperation;
  return NS_OK;
}

nsresult sbPropertyChange::SetID(const nsAString &aID)
{
  mID = aID;
  return NS_OK;
}

nsresult sbPropertyChange::SetOldValue(const nsAString &aOldValue)
{
  mOldValue = aOldValue;
  return NS_OK;
}

nsresult sbPropertyChange::SetNewValue(const nsAString &aNewValue)
{
  mNewValue = aNewValue;
  return NS_OK;
}

/* readonly attribute unsigned long operation; */
NS_IMETHODIMP sbPropertyChange::GetOperation(PRUint32 *aOperation)
{
  NS_ENSURE_ARG_POINTER(aOperation);
  *aOperation = mOperation;

  return NS_OK;
}

/* readonly attribute AString id; */
NS_IMETHODIMP sbPropertyChange::GetId(nsAString & aId)
{
  aId.Assign(mID);

  return NS_OK;
}

/* readonly attribute AString oldValue; */
NS_IMETHODIMP sbPropertyChange::GetOldValue(nsAString & aOldValue)
{
  aOldValue.Assign(mOldValue);

  return NS_OK;
}

/* readonly attribute AString newValue; */
NS_IMETHODIMP sbPropertyChange::GetNewValue(nsAString & aNewValue)
{
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
, mTimestamp(0)
{
}

sbLibraryChange::~sbLibraryChange()
{
}


nsresult sbLibraryChange::InitWithValues(PRUint32 aOperation,
                                         PRUint64 aTimestamp,
                                         sbIMediaItem *aSourceItem,
                                         sbIMediaItem *aDestinationItem,
                                         nsIArray *aProperties,
                                         nsIArray *aListItems)
{
  mOperation = aOperation;
  mTimestamp = aTimestamp;
  mSourceItem = aSourceItem;
  mDestinationItem = aDestinationItem;
  mProperties = aProperties;
  mListItems = aListItems;
  return NS_OK;
}

nsresult sbLibraryChange::SetOperation(PRUint32 aOperation)
{
  mOperation = aOperation;

  return NS_OK;
}

nsresult sbLibraryChange::SetTimestamp(PRUint64 aTimestamp)
{
  mTimestamp = aTimestamp;

  return NS_OK;
}

nsresult sbLibraryChange::SetListItems(nsIArray *aListItems)
{
  mListItems = aListItems;

  return NS_OK;
}

nsresult sbLibraryChange::SetItems(sbIMediaItem *aSourceItem,
                                   sbIMediaItem *aDestinationItem)
{
  NS_ENSURE_ARG_POINTER(aSourceItem);

  mSourceItem = aSourceItem;
  mDestinationItem = aDestinationItem ? aDestinationItem : aSourceItem;

  return NS_OK;
}

nsresult sbLibraryChange::SetProperties(nsIArray *aProperties)
{
  NS_ENSURE_ARG_POINTER(aProperties);

  mProperties = aProperties;

  return NS_OK;
}

/* readonly attribute unsigned long operation; */
NS_IMETHODIMP sbLibraryChange::GetOperation(PRUint32 *aOperation)
{
  NS_ENSURE_ARG_POINTER(aOperation);

  *aOperation = mOperation;

  return NS_OK;
}

/* readonly attribute unsigned long long timestamp; */
NS_IMETHODIMP sbLibraryChange::GetTimestamp(PRUint64 *aTimestamp)
{
  NS_ENSURE_ARG_POINTER(aTimestamp);

  *aTimestamp = mTimestamp;

  return NS_OK;
}

/* readonly attribute sbIMediaItem sourceItem; */
NS_IMETHODIMP sbLibraryChange::GetSourceItem(sbIMediaItem * *aItem)
{
  NS_ENSURE_ARG_POINTER(aItem);
  NS_IF_ADDREF(*aItem = mSourceItem);

  return *aItem ? NS_OK : NS_ERROR_NOT_AVAILABLE;
}

/* readonly attribute sbIMediaItem destinationItem; */
NS_IMETHODIMP sbLibraryChange::GetDestinationItem(sbIMediaItem * *aItem)
{
  NS_ENSURE_ARG_POINTER(aItem);

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

/* readonly attribute nsIArray listItems; */
NS_IMETHODIMP sbLibraryChange::GetListItems(nsIArray **aListItems)
{
  NS_ENSURE_ARG_POINTER(aListItems);

  PRBool isList;
  nsresult rv = GetItemIsList(&isList);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!isList)
    return NS_ERROR_FAILURE;

  NS_IF_ADDREF(*aListItems = mListItems);

  return NS_OK;
}

/* readonly attribute nsIArray properties; */
NS_IMETHODIMP sbLibraryChange::GetProperties(nsIArray * *aProperties)
{
  NS_ENSURE_ARG_POINTER(aProperties);

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
{
}

sbLibraryChangeset::~sbLibraryChangeset()
{
}

nsresult sbLibraryChangeset::InitWithValues(nsIArray *aSourceLists,
                                            sbIMediaList *aDestinationList,
                                            nsIArray *aChanges)
{
  NS_ENSURE_ARG_POINTER(aSourceLists);
  NS_ENSURE_ARG_POINTER(aDestinationList);
  NS_ENSURE_ARG_POINTER(aChanges);


  mSourceLists = aSourceLists;
  mDestinationList = aDestinationList;
  mChanges = aChanges;

  return NS_OK;
}

nsresult sbLibraryChangeset::SetSourceLists(nsIArray *aSourceLists)
{
  NS_ENSURE_ARG_POINTER(aSourceLists);

  mSourceLists = aSourceLists;

  return NS_OK;
}

nsresult sbLibraryChangeset::SetDestinationList(sbIMediaList *aDestinationList)
{
  NS_ENSURE_ARG_POINTER(aDestinationList);

  mDestinationList = aDestinationList;

  return NS_OK;
}

NS_IMETHODIMP sbLibraryChangeset::SetChanges(nsIArray *aChanges)
{
  NS_ENSURE_ARG_POINTER(aChanges);

  mChanges = aChanges;

  return NS_OK;
}

/* readonly attribute sbIMediaList sourceList; */
NS_IMETHODIMP sbLibraryChangeset::GetSourceLists(nsIArray * *aSourceLists)
{
  NS_ENSURE_ARG_POINTER(aSourceLists);

  NS_IF_ADDREF(*aSourceLists = mSourceLists);

  return *aSourceLists ? NS_OK : NS_ERROR_NOT_AVAILABLE;
}

/* readonly attribute sbIMediaList destinationList; */
NS_IMETHODIMP sbLibraryChangeset::GetDestinationList(sbIMediaList * *aDestinationList)
{
  NS_ENSURE_ARG_POINTER(aDestinationList);

  NS_IF_ADDREF(*aDestinationList = mDestinationList);

  return *aDestinationList ? NS_OK : NS_ERROR_NOT_AVAILABLE;
}

/* readonly attribute nsIArray changes; */
NS_IMETHODIMP sbLibraryChangeset::GetChanges(nsIArray * *aChanges)
{
  NS_ENSURE_ARG_POINTER(aChanges);

  NS_IF_ADDREF(*aChanges = mChanges);

  return *aChanges ? NS_OK : NS_ERROR_NOT_AVAILABLE;
}
