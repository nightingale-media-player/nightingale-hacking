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

#include "sbRatingPropertyInfo.h"

#include <sbIPropertyManager.h>
#include <nsITreeView.h>

NS_IMPL_ISUPPORTS_INHERITED2(sbRatingPropertyInfo,
                             sbImmutablePropertyInfo,
                             sbIClickablePropertyInfo,
                             sbITreeViewPropertyInfo)

#define STAR_WIDTH 16
#define MAX_RATING 5
#define ZERO_HIT_WIDTH 7

sbRatingPropertyInfo::sbRatingPropertyInfo(const nsAString& aPropertyName,
                                           const nsAString& aDisplayName)
{
  mName = aPropertyName;
  mDisplayName = aDisplayName;
  mUserViewable = PR_TRUE;
  mType.AssignLiteral("rating");
}

nsresult
sbRatingPropertyInfo::Init()
{
  nsresult rv;

  rv = sbImmutablePropertyInfo::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// sbITreeViewPropertyInfo

NS_IMETHODIMP
sbRatingPropertyInfo::GetImageSrc(const nsAString& aValue,
                                  nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbRatingPropertyInfo::GetProgressMode(const nsAString& aValue,
                                      PRInt32* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsITreeView::PROGRESS_NONE;
  return NS_OK;
}

NS_IMETHODIMP
sbRatingPropertyInfo::GetCellValue(const nsAString& aValue,
                                   nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbRatingPropertyInfo::GetColumnProperties(const nsAString& aValue,
                                          nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbRatingPropertyInfo::GetRowProperties(const nsAString& aValue,
                                       nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbRatingPropertyInfo::GetCellProperties(const nsAString& aValue,
                                        nsAString& _retval)
{
  _retval.AssignLiteral("rating rating");
  _retval.Append(aValue);
  return NS_OK;
}

NS_IMETHODIMP
sbRatingPropertyInfo::GetColumnType(nsAString& _retval)
{
  _retval.AssignLiteral("text");
  return NS_OK;
}

// sbIClickablePropertyInfo

NS_IMETHODIMP
sbRatingPropertyInfo::GetSuppressSelect(PRBool* aSuppressSelect)
{
  NS_ENSURE_ARG_POINTER(aSuppressSelect);
  *aSuppressSelect = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbRatingPropertyInfo::IsDisabled(const nsAString& aCurrentValue,
                                 PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbRatingPropertyInfo::HitTest(const nsAString& aCurrentValue,
                              const nsAString& aPart,
                              PRUint32 aBoxWidth,
                              PRUint32 aBoxHeight,
                              PRUint32 aMouseX,
                              PRUint32 aMouseY,
                              PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbRatingPropertyInfo::GetValueForClick(const nsAString& aCurrentValue,
                                       PRUint32 aBoxWidth,
                                       PRUint32 aBoxHeight,
                                       PRUint32 aMouseX,
                                       PRUint32 aMouseY,
                                       nsAString& _retval)
{

  PRUint32 rating;

  // Magical number that allows the user to click to the left of the first
  // star to set the rating to zero
  if (aMouseX < ZERO_HIT_WIDTH) {
    rating = 0;
  }
  else {
    rating = (aMouseX / STAR_WIDTH) + 1;

    if (rating > MAX_RATING) {
      rating = MAX_RATING;
    }
  }

  nsString ratingStr;
  ratingStr.AppendInt(rating);

  _retval = ratingStr;
  return NS_OK;
}

// sbIPropertyInfo

NS_IMETHODIMP
sbRatingPropertyInfo::Format(const nsAString& aValue,
                             nsAString& _retval)
{
  _retval = aValue;
  return NS_OK;
}

