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

#include "sbSimpleButtonPropertyInfo.h"
#include "sbStandardOperators.h"

#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>
#include <nsIStringBundle.h>
#include <nsITreeView.h>

NS_IMPL_ISUPPORTS_INHERITED2(sbSimpleButtonPropertyInfo,
                             sbImmutablePropertyInfo,
                             sbIClickablePropertyInfo,
                             sbITreeViewPropertyInfo)

/*
 * The value of simple button properties is formatted as follows:
 *
 * <label>[|<disable state>]
 *
 * A disable state of 1 means the button is disabled, otherwise it is enabled
 *
 */
sbSimpleButtonPropertyInfo::sbSimpleButtonPropertyInfo(const nsAString& aPropertyName,
                                                       const nsAString& aDisplayName,
                                                       PRBool aHasLabel,
                                                       const nsAString& aLabel) :
  sbImmutablePropertyInfo()
{
  mName = aPropertyName;
  mDisplayName = aDisplayName;
  mHasLabel = aHasLabel;
  mLabel = aLabel;
  mUserViewable = PR_TRUE;
  mType.AssignLiteral("button");
}

nsresult
sbSimpleButtonPropertyInfo::Init()
{
  nsresult rv;

  rv = sbImmutablePropertyInfo::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// sbITreeViewPropertyInfo

NS_IMETHODIMP
sbSimpleButtonPropertyInfo::GetImageSrc(const nsAString& aValue,
                                        nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbSimpleButtonPropertyInfo::GetProgressMode(const nsAString& aValue,
                                            PRInt32* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsITreeView::PROGRESS_NONE;
  return NS_OK;
}

NS_IMETHODIMP
sbSimpleButtonPropertyInfo::GetCellValue(const nsAString& aValue,
                                         nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbSimpleButtonPropertyInfo::GetColumnProperties(const nsAString& aValue,
                                                nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbSimpleButtonPropertyInfo::GetRowProperties(const nsAString& aValue,
                                             nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbSimpleButtonPropertyInfo::GetCellProperties(const nsAString& aValue,
                                              nsAString& _retval)
{
  _retval.AssignLiteral("button");
  return NS_OK;
}

NS_IMETHODIMP
sbSimpleButtonPropertyInfo::GetColumnType(nsAString& _retval)
{
  _retval.AssignLiteral("text");
  return NS_OK;
}

// sbIClickablePropertyInfo

NS_IMETHODIMP
sbSimpleButtonPropertyInfo::GetSuppressSelect(PRBool* aSuppressSelect)
{
  NS_ENSURE_ARG_POINTER(aSuppressSelect);
  *aSuppressSelect = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbSimpleButtonPropertyInfo::IsDisabled(const nsAString& aCurrentValue,
                                       PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  PRInt32 pos = aCurrentValue.FindChar('|');
  if (pos >= 0) {
    *_retval = Substring(aCurrentValue, pos + 1).EqualsLiteral("1");
  }
  else {
    *_retval = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbSimpleButtonPropertyInfo::HitTest(const nsAString& aCurrentValue,
                                    const nsAString& aPart,
                                    PRUint32 aBoxWidth,
                                    PRUint32 aBoxHeight,
                                    PRUint32 aMouseX,
                                    PRUint32 aMouseY,
                                    PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  PRBool isDisabled;
  nsresult rv = IsDisabled(aCurrentValue, &isDisabled);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isDisabled) {
    *_retval = PR_FALSE;
  }
  else {
    *_retval = aPart.EqualsLiteral("text");
  }
  return NS_OK;
}

NS_IMETHODIMP
sbSimpleButtonPropertyInfo::GetValueForClick(const nsAString& aCurrentValue,
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
sbSimpleButtonPropertyInfo::Format(const nsAString& aValue,
                                   nsAString& _retval)
{
  if (mHasLabel) {
    _retval = mLabel;
  }
  else {
    PRInt32 pos = aValue.FindChar('|');
    if (pos >= 0) {
      _retval = Substring(aValue, 0, pos);
    }
    else {
      _retval = aValue;
    }
  }

  return NS_OK;
}

