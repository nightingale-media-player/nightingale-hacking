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

#include "sbOriginPageImagePropertyInfo.h"
#include "sbStandardOperators.h"

#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>
#include <nsITreeView.h>

NS_IMPL_ISUPPORTS_INHERITED2(sbOriginPageImagePropertyInfo,
                             sbImmutablePropertyInfo,
                             sbIClickablePropertyInfo,
                             sbITreeViewPropertyInfo)

sbOriginPageImagePropertyInfo::sbOriginPageImagePropertyInfo(const nsAString& aPropertyID,
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
sbOriginPageImagePropertyInfo::Init()
{
  nsresult rv;

  rv = sbImmutablePropertyInfo::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// sbITreeViewPropertyInfo

NS_IMETHODIMP
sbOriginPageImagePropertyInfo::GetImageSrc(const nsAString& aValue,
                                           nsAString& _retval)
{
  if(aValue.EqualsLiteral("unknownOrigin")) {
    _retval.Truncate();
    return NS_OK;
  }

  if(aValue.EqualsLiteral("webOrigin")) {
    _retval.Truncate();
    return NS_OK;
  }

  _retval = aValue;

  return NS_OK;
}

NS_IMETHODIMP
sbOriginPageImagePropertyInfo::GetProgressMode(const nsAString& aValue,
                                               PRInt32* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsITreeView::PROGRESS_NONE;
  return NS_OK;
}

NS_IMETHODIMP
sbOriginPageImagePropertyInfo::GetCellValue(const nsAString& aValue,
                                            nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbOriginPageImagePropertyInfo::GetColumnProperties(const nsAString& aValue,
                                                   nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbOriginPageImagePropertyInfo::GetRowProperties(const nsAString& aValue,
                                                nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbOriginPageImagePropertyInfo::GetCellProperties(const nsAString& aValue,
                                                 nsAString& _retval)
{
  if(aValue.EqualsLiteral("unknownOrigin") ||
     aValue.IsEmpty() ||
     aValue.IsVoid()) {
    _retval.AssignLiteral("image unknownOrigin");
    return NS_OK;
  }

  if(aValue.EqualsLiteral("webOrigin")) {
    _retval.AssignLiteral("image webOrigin");
    return NS_OK;
  }

  _retval.AssignLiteral("image");

  return NS_OK;
}

NS_IMETHODIMP
sbOriginPageImagePropertyInfo::GetColumnType(nsAString& _retval)
{
  _retval.AssignLiteral("text");
  return NS_OK;
}

// sbIClickablePropertyInfo

NS_IMETHODIMP
sbOriginPageImagePropertyInfo::GetSuppressSelect(PRBool* aSuppressSelect)
{
  NS_ENSURE_ARG_POINTER(aSuppressSelect);
  *aSuppressSelect = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbOriginPageImagePropertyInfo::IsDisabled(const nsAString& aCurrentValue,
                                          PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbOriginPageImagePropertyInfo::HitTest(const nsAString& aCurrentValue,
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
sbOriginPageImagePropertyInfo::GetValueForClick(const nsAString& aCurrentValue,
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
sbOriginPageImagePropertyInfo::Format(const nsAString& aValue,
                                      nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

