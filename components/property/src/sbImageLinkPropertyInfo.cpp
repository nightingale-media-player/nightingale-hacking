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

#include "sbImageLinkPropertyInfo.h"

#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>
#include <nsITreeView.h>

#include <nsIURI.h>
#include <nsNetUtil.h>
#include <nsServiceManagerUtils.h>
#include <nsUnicharUtils.h>

NS_IMPL_ISUPPORTS_INHERITED3(sbImageLinkPropertyInfo,
                             sbImmutablePropertyInfo,
                             sbIImageLinkPropertyInfo,
                             sbIClickablePropertyInfo,
                             sbITreeViewPropertyInfo)

sbImageLinkPropertyInfo::sbImageLinkPropertyInfo(const nsAString& aPropertyID,
                                                 const nsAString& aDisplayName,
                                                 const nsAString& aLocalizationKey,
                                                 const bool aRemoteReadable,
                                                 const bool aRemoteWritable,
                                                 const bool aUserViewable,
                                                 const bool aUserEditable,
                                                 const nsAString& aUrlPropertyID)
{
  mID = aPropertyID;
  mDisplayName = aDisplayName;
  mLocalizationKey = aLocalizationKey;
  mUserViewable = aUserViewable;
  mUserEditable = aUserEditable;
  mRemoteReadable = aRemoteReadable;
  mRemoteWritable = aRemoteWritable;
  mUrlPropertyID = aUrlPropertyID;
  mType.AssignLiteral("image");
  mSuppressSelect = PR_TRUE;
}

nsresult
sbImageLinkPropertyInfo::Init()
{
  nsresult rv;

  rv = sbImmutablePropertyInfo::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// sbITreeViewPropertyInfo

NS_IMETHODIMP
sbImageLinkPropertyInfo::GetImageSrc(const nsAString& aValue,
                                     nsAString& _retval)
{
  if(!aValue.IsEmpty() &&
     !aValue.LowerCaseEqualsLiteral("default")) {
    _retval = aValue;
    return NS_OK;
  }

  _retval.Truncate();

  return NS_OK;
}

NS_IMETHODIMP
sbImageLinkPropertyInfo::GetProgressMode(const nsAString& aValue,
                                         PRInt32* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsITreeView::PROGRESS_NONE;
  return NS_OK;
}

NS_IMETHODIMP
sbImageLinkPropertyInfo::GetCellValue(const nsAString& aValue,
                                      nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbImageLinkPropertyInfo::GetRowProperties(const nsAString& aValue,
                                          nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbImageLinkPropertyInfo::GetCellProperties(const nsAString& aValue,
                                           nsAString& _retval)
{
  if(aValue.IsEmpty()) {
    _retval.AssignLiteral("image noLink");
    return NS_OK;
  }

  _retval.AssignLiteral("image link");

  return NS_OK;
}

NS_IMETHODIMP
sbImageLinkPropertyInfo::GetColumnType(nsAString& _retval)
{
  _retval.AssignLiteral("text");
  return NS_OK;
}

// sbIClickablePropertyInfo

NS_IMETHODIMP
sbImageLinkPropertyInfo::GetSuppressSelect(bool* aSuppressSelect)
{
  NS_ENSURE_ARG_POINTER(aSuppressSelect);
  *aSuppressSelect = mSuppressSelect;
  return NS_OK;
}

NS_IMETHODIMP
sbImageLinkPropertyInfo::SetSuppressSelect(bool aSuppressSelect)
{
  mSuppressSelect = aSuppressSelect;
  return NS_OK;
}

NS_IMETHODIMP
sbImageLinkPropertyInfo::IsDisabled(const nsAString& aCurrentValue,
                                    bool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbImageLinkPropertyInfo::HitTest(const nsAString& aCurrentValue,
                                 const nsAString& aPart,
                                 PRUint32 aBoxWidth,
                                 PRUint32 aBoxHeight,
                                 PRUint32 aMouseX,
                                 PRUint32 aMouseY,
                                 bool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = aPart.EqualsLiteral("image");
  return NS_OK;
}

NS_IMETHODIMP
sbImageLinkPropertyInfo::GetValueForClick(const nsAString& aCurrentValue,
                                          PRUint32 aBoxWidth,
                                          PRUint32 aBoxHeight,
                                          PRUint32 aMouseX,
                                          PRUint32 aMouseY,
                                          nsAString& _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* boolean onClick (in sbIMediaItem aItem,
                    [optional] in nsISupports aEvent,
                    [optional] in nsISupports aContext); */
NS_IMETHODIMP
sbImageLinkPropertyInfo::OnClick(sbIMediaItem *aItem,
                                 nsISupports *aEvent,
                                 nsISupports *aContext,
                                 bool *_retval NS_OUTPARAM)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;
  return NS_OK;
}

// sbIPropertyInfo

NS_IMETHODIMP
sbImageLinkPropertyInfo::Format(const nsAString& aValue,
                                nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

// sbIImageLinkPropertyInfo

NS_IMETHODIMP
sbImageLinkPropertyInfo::GetUrlProperty(nsAString& _retval)
{
  _retval = mUrlPropertyID;
  
  return NS_OK;
}

NS_IMETHODIMP
sbImageLinkPropertyInfo::GetPreventNavigation(const nsAString& aImageValue,
                                              const nsAString& aUrlValue,
                                              bool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  
  *_retval = aImageValue.IsEmpty() || 
             aUrlValue.IsEmpty();
  
  return NS_OK;
}

