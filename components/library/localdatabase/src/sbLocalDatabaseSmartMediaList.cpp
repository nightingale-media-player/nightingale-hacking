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

#include "sbLocalDatabaseSmartMediaList.h"

#include <sbIDatabaseQuery.h>
#include <sbIDatabaseResult.h>
#include <sbISQLBuilder.h>

#include <sbILibrary.h>

#include "sbLocalDatabaseCID.h"

#include <nsAutoLock.h>
#include <nsCOMPtr.h>
#include <nsComponentManagerUtils.h>
#include <sbStandardProperties.h>
#include <sbSQLBuilderCID.h>

//==============================================================================
// sbLocalDatabaseSmartMediaListCondition
//==============================================================================
NS_IMPL_ISUPPORTS1(sbLocalDatabaseSmartMediaListCondition, 
                   sbILocalDatabaseSmartMediaListCondition)

sbLocalDatabaseSmartMediaListCondition::sbLocalDatabaseSmartMediaListCondition()
{
}

sbLocalDatabaseSmartMediaListCondition::~sbLocalDatabaseSmartMediaListCondition()
{
}

NS_IMETHODIMP sbLocalDatabaseSmartMediaListCondition::GetPropertyName(nsAString & aPropertyName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP sbLocalDatabaseSmartMediaListCondition::SetPropertyName(const nsAString & aPropertyName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbLocalDatabaseSmartMediaListCondition::GetOperator(nsAString & aOperator)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP sbLocalDatabaseSmartMediaListCondition::SetOperator(const nsAString & aOperator)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbLocalDatabaseSmartMediaListCondition::GetLeftValue(nsAString & aLeftValue)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP sbLocalDatabaseSmartMediaListCondition::SetLeftValue(const nsAString & aLeftValue)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbLocalDatabaseSmartMediaListCondition::GetRightValue(nsAString & aRightValue)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP sbLocalDatabaseSmartMediaListCondition::SetRightValue(const nsAString & aRightValue)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbLocalDatabaseSmartMediaListCondition::GetLimit(PRBool *aLimit)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP sbLocalDatabaseSmartMediaListCondition::SetLimit(PRBool aLimit)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


//==============================================================================
// sbLocalDatabaseSmartMediaList
//==============================================================================


NS_IMPL_THREADSAFE_ISUPPORTS5(sbLocalDatabaseSmartMediaList,
                              nsISupportsWeakReference,
                              sbILibraryResource,
                              sbIMediaItem,
                              sbIMediaList,
                              sbILocalDatabaseSmartMediaList)

sbLocalDatabaseSmartMediaList::sbLocalDatabaseSmartMediaList()
: mInnerLock(nsnull)
{
}

sbLocalDatabaseSmartMediaList::~sbLocalDatabaseSmartMediaList()
{
  if(mInnerLock) {
    PR_DestroyLock(mInnerLock);
  }
}

NS_IMETHODIMP sbLocalDatabaseSmartMediaList::Init(sbIMediaItem *aItem)
{
  NS_ENSURE_ARG_POINTER(aItem);
  
  mInnerLock = PR_NewLock();
  NS_ENSURE_TRUE(mInnerLock, NS_ERROR_OUT_OF_MEMORY);

  mItem = aItem;

  nsAutoString storageGuid;
  nsresult rv = mItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_STORAGEGUID), 
                          storageGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibrary> library;
  rv = mItem->GetLibrary(getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> mediaItem;
  rv = library->GetMediaItem(storageGuid, getter_AddRefs(mediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaList> dataList = do_QueryInterface(mediaItem, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mList = dataList;
  
  return NS_OK;
}

NS_IMETHODIMP sbLocalDatabaseSmartMediaList::GetType(nsAString& aType)
{
  aType.Assign(NS_LITERAL_STRING("smart"));
  return NS_OK;
}

NS_IMETHODIMP sbLocalDatabaseSmartMediaList::GetCurrentQuery(nsAString & aCurrentQuery)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbLocalDatabaseSmartMediaList::GetConditionCount(PRInt32 *aConditionCount)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbLocalDatabaseSmartMediaList::GetItemLimit(PRInt32 *aItemLimit)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP sbLocalDatabaseSmartMediaList::SetItemLimit(PRInt32 aItemLimit)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbLocalDatabaseSmartMediaList::GetRandomSelection(PRBool *aRandomSelection)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP sbLocalDatabaseSmartMediaList::SetRandomSelection(PRBool aRandomSelection)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbLocalDatabaseSmartMediaList::AddCondition(sbILocalDatabaseSmartMediaListCondition *aCondition, 
                                                          PRInt32 *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbLocalDatabaseSmartMediaList::RemoveCondition(PRInt32 aConditionIndex)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbLocalDatabaseSmartMediaList::GetConditionAt(PRInt32 aConditionIndex, 
                                                            sbILocalDatabaseSmartMediaListCondition **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbLocalDatabaseSmartMediaList::Rebuild()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
