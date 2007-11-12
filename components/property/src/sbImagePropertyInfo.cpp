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

#include "sbImagePropertyInfo.h"
#include "sbStandardOperators.h"

#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>
#include <nsITreeView.h>

NS_IMPL_ISUPPORTS_INHERITED2(sbImagePropertyInfo,
                             sbImmutablePropertyInfo,
                             sbIClickablePropertyInfo,
                             sbITreeViewPropertyInfo)

sbImagePropertyInfo::sbImagePropertyInfo(const nsAString& aPropertyID,
                                         const nsAString& aDisplayName,
                                         const PRBool aRemoteReadable,
                                         const PRBool aRemoteWritable,
                                         const PRBool aUserViewable,
                                         const PRBool aUserEditable)
{
  mID = aPropertyID;
  mDisplayName = aDisplayName;
  mUserViewable = aUserViewable;
  mUserEditable = aUserEditable;
  mRemoteReadable = aRemoteReadable;
  mRemoteWritable = aRemoteWritable;
  mType.AssignLiteral("image");
}

nsresult
sbImagePropertyInfo::Init()
{
  nsresult rv;

  rv = sbImmutablePropertyInfo::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// sbITreeViewPropertyInfo

NS_IMETHODIMP
sbImagePropertyInfo::GetImageSrc(const nsAString& aValue,
                                 nsAString& _retval)
{
  _retval = aValue;
  return NS_OK;
}

NS_IMETHODIMP
sbImagePropertyInfo::GetProgressMode(const nsAString& aValue,
                                     PRInt32* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsITreeView::PROGRESS_NONE;
  return NS_OK;
}

NS_IMETHODIMP
sbImagePropertyInfo::GetCellValue(const nsAString& aValue,
                                  nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbImagePropertyInfo::GetColumnProperties(const nsAString& aValue,
                                         nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbImagePropertyInfo::GetRowProperties(const nsAString& aValue,
                                      nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbImagePropertyInfo::GetCellProperties(const nsAString& aValue,
                                       nsAString& _retval)
{
  _retval.AssignLiteral("image");
  return NS_OK;
}

NS_IMETHODIMP
sbImagePropertyInfo::GetColumnType(nsAString& _retval)
{
  _retval.AssignLiteral("text");
  return NS_OK;
}

// sbIClickablePropertyInfo

NS_IMETHODIMP
sbImagePropertyInfo::GetSuppressSelect(PRBool* aSuppressSelect)
{
  NS_ENSURE_ARG_POINTER(aSuppressSelect);
  *aSuppressSelect = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbImagePropertyInfo::IsDisabled(const nsAString& aCurrentValue,
                                PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbImagePropertyInfo::HitTest(const nsAString& aCurrentValue,
                             const nsAString& aPart,
                             PRUint32 aBoxWidth,
                             PRUint32 aBoxHeight,
                             PRUint32 aMouseX,
                             PRUint32 aMouseY,
                             PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = aPart.EqualsLiteral("image");
  return NS_OK;
}

NS_IMETHODIMP
sbImagePropertyInfo::GetValueForClick(const nsAString& aCurrentValue,
                                      PRUint32 aBoxWidth,
                                      PRUint32 aBoxHeight,
                                      PRUint32 aMouseX,
                                      PRUint32 aMouseY,
                                      nsAString& _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

// sbIPropertyInfo

NS_IMETHODIMP
sbImagePropertyInfo::Format(const nsAString& aValue,
                            nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

