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

#include "sbTextPropertyInfo.h"
#include <nsAutoLock.h>

NS_IMPL_THREADSAFE_ISUPPORTS1(sbPropertyInfo, sbIPropertyInfo)

sbPropertyInfo::sbPropertyInfo()
: mNullSort(sbIPropertyInfo::SORT_NULL_SMALL)
, mNameLock(nsnull)
, mTypeLock(nsnull)
, mDisplayKeyLock(nsnull)
, mDisplayUsingSimpleTypeLock(nsnull)
, mDisplayUsingXBLWidgetLock(nsnull)
, mUnitsLock(nsnull)
{
  mNameLock = PR_NewLock();
  NS_ASSERTION(mNameLock, 
    "sbPropertyInfo::mNameLock failed to create lock!");

  mTypeLock = PR_NewLock();
  NS_ASSERTION(mTypeLock, 
    "sbPropertyInfo::mTypeLock failed to create lock!");

  mDisplayKeyLock = PR_NewLock();
  NS_ASSERTION(mDisplayKeyLock, 
    "sbPropertyInfo::mDisplayKeyLock failed to create lock!");

  mDisplayUsingSimpleTypeLock = PR_NewLock();
  NS_ASSERTION(mDisplayUsingSimpleTypeLock, 
    "sbPropertyInfo::mDisplayUsingSimpleTypeLock failed to create lock!");

  mDisplayUsingXBLWidgetLock = PR_NewLock();
  NS_ASSERTION(mDisplayUsingXBLWidgetLock, 
    "sbPropertyInfo::mDisplayUsingXBLWidgetLock failed to create lock!");

  mUnitsLock = PR_NewLock();
  NS_ASSERTION(mUnitsLock, 
    "sbPropertyInfo::mUnitsLock failed to create lock!");

}

sbPropertyInfo::~sbPropertyInfo()
{
  if(mNameLock) {
    PR_DestroyLock(mNameLock);
  }

  if(mTypeLock) {
    PR_DestroyLock(mTypeLock);
  }

  if(mDisplayKeyLock) {
    PR_DestroyLock(mDisplayKeyLock);
  }

  if(mDisplayUsingSimpleTypeLock) {
    PR_DestroyLock(mDisplayUsingSimpleTypeLock);
  }

  if(mDisplayUsingXBLWidgetLock) {
    PR_DestroyLock(mDisplayUsingXBLWidgetLock);
  }

  if(mUnitsLock) {
    PR_DestroyLock(mUnitsLock);
  }
}

NS_IMETHODIMP sbPropertyInfo::SetNullSort(PRUint32 aNullSort)
{
  mNullSort = aNullSort;
  return NS_OK; 
}
NS_IMETHODIMP sbPropertyInfo::GetNullSort(PRUint32 *aNullSort)
{
  NS_ENSURE_ARG_POINTER(aNullSort);
  *aNullSort = mNullSort;
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetName(nsAString & aName)
{
  nsAutoLock lock(mNameLock);
  aName = mName;
  return NS_OK;
}
NS_IMETHODIMP sbPropertyInfo::SetName(const nsAString &aName)
{
  nsAutoLock lock(mNameLock);
  
  if(mName.IsEmpty()) {
    mName = aName;
    return NS_OK;
  }

  return NS_ERROR_ALREADY_INITIALIZED;
}

NS_IMETHODIMP sbPropertyInfo::GetType(nsAString & aType)
{
  nsAutoLock lock(mTypeLock);
  aType = mType;
  return NS_OK;
}
NS_IMETHODIMP sbPropertyInfo::SetType(const nsAString &aType)
{
  nsAutoLock lock(mTypeLock);

  if(mType.IsEmpty()) {
    mType = aType;
    return NS_OK;
  }

  return NS_ERROR_ALREADY_INITIALIZED;
}

NS_IMETHODIMP sbPropertyInfo::GetDisplayKey(nsAString & aDisplayKey)
{
  nsAutoLock lock(mDisplayKeyLock);
  aDisplayKey = mDisplayKey;
  return NS_OK;
}
NS_IMETHODIMP sbPropertyInfo::SetDisplayKey(const nsAString &aKey)
{
  nsAutoLock lock(mDisplayKeyLock);

  if(mDisplayKey.IsEmpty()) {
    mDisplayKey = aKey;
    return NS_OK;
  }

  return NS_ERROR_ALREADY_INITIALIZED;
}

NS_IMETHODIMP sbPropertyInfo::GetDisplayUsingSimpleType(nsAString & aDisplayUsingSimpleType)
{
  nsAutoLock lock(mDisplayUsingSimpleTypeLock);
  aDisplayUsingSimpleType = mDisplayUsingSimpleType;
  return NS_OK;
}
NS_IMETHODIMP sbPropertyInfo::SetDisplayUsingSimpleType(const nsAString &aDisplayUsingSimpleType)
{
  nsAutoLock lock(mDisplayUsingSimpleTypeLock);

  if(mDisplayUsingSimpleType.IsEmpty()) {
    mDisplayUsingSimpleType = aDisplayUsingSimpleType;
    return NS_OK;
  }

  return NS_ERROR_ALREADY_INITIALIZED;
}

NS_IMETHODIMP sbPropertyInfo::GetDisplayUsingXBLWidget(nsIURI * *aDisplayUsingXBLWidget)
{
  NS_ENSURE_ARG_POINTER(aDisplayUsingXBLWidget);
  *aDisplayUsingXBLWidget = nsnull;
  
  nsAutoLock lock(mDisplayUsingXBLWidgetLock);
  if(mDisplayUsingXBLWidget) {
    
  }
    
  return NS_OK;
}
NS_IMETHODIMP sbPropertyInfo::SetDisplayUsingXBLWidget(nsIURI *aDisplayUsingXBLWidget)
{
  nsAutoLock lock(mDisplayUsingXBLWidgetLock);

  if(!mDisplayUsingXBLWidget) {
    PRBool isChromeURI = PR_FALSE;
    nsresult rv = aDisplayUsingXBLWidget->SchemeIs("chrome", &isChromeURI);
    NS_ENSURE_SUCCESS(rv, rv);

    if(isChromeURI) {
      mDisplayUsingXBLWidget = aDisplayUsingXBLWidget;
      return NS_OK;
    }
    
    return NS_ERROR_INVALID_ARG;
  }

  return NS_ERROR_ALREADY_INITIALIZED;
}

NS_IMETHODIMP sbPropertyInfo::GetUnits(nsAString & aUnits)
{
  nsAutoLock lock(mUnitsLock);
  aUnits = mUnits;
  return NS_OK;
}
NS_IMETHODIMP sbPropertyInfo::SetUnits(const nsAString & aUnits)
{
  nsAutoLock lock(mUnitsLock);

  if(mUnits.IsEmpty()) {
    mUnits = aUnits;
    return NS_OK;
  }

  return NS_ERROR_ALREADY_INITIALIZED;
}

NS_IMETHODIMP sbPropertyInfo::Validate(const nsAString & aValue, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbPropertyInfo::Format(const nsAString & aValue, nsAString & _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbPropertyInfo::MakeSortable(const nsAString & aValue, nsAString & _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
