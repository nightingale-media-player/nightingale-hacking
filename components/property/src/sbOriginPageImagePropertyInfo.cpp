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

#include <nsIURI.h>
#include <nsNetUtil.h>
#include <nsServiceManagerUtils.h>
#include <nsUnicharUtils.h>

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

  nsCOMPtr<nsIFaviconService> faviconService = 
    do_GetService("@mozilla.org/browser/favicon-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mFaviconService = faviconService;

  rv = sbImmutablePropertyInfo::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// sbITreeViewPropertyInfo

NS_IMETHODIMP
sbOriginPageImagePropertyInfo::GetImageSrc(const nsAString& aValue,
                                           nsAString& _retval)
{
  if(aValue.IsEmpty() ||
     aValue.IsVoid() ||
     aValue.EqualsLiteral("unknownOrigin") ||
     aValue.EqualsLiteral("webOrigin")) {
    _retval.Truncate();
    return NS_OK;
  }

  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aValue);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> imageUri;
  rv = mFaviconService->GetFaviconForPage(uri, getter_AddRefs(imageUri));
  
  if(rv == NS_ERROR_NOT_AVAILABLE) {
    _retval.Truncate();
    return NS_OK;
  }

  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString spec;
  rv = imageUri->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);
  
  NS_NAMED_LITERAL_CSTRING(mozAnnoFavicon, "moz-anno:favicon:");
  if(!StringBeginsWith(spec, mozAnnoFavicon)) {
    _retval = NS_ConvertUTF8toUTF16(spec);
    return NS_OK;
  }

  spec.Cut(0, mozAnnoFavicon.Length());
  NS_WARNING(spec.get());
  _retval = NS_ConvertUTF8toUTF16(spec);

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

  if(aValue.EqualsLiteral("webOrigin") ||
     StringBeginsWith(aValue, NS_LITERAL_STRING("http://"), CaseInsensitiveCompare) ||
     StringBeginsWith(aValue, NS_LITERAL_STRING("https://"), CaseInsensitiveCompare) ||
     StringBeginsWith(aValue, NS_LITERAL_STRING("ftp://"), CaseInsensitiveCompare)) {
    
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

