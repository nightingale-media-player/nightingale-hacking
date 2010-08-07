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

#include "sbRatingPropertyInfo.h"

#include <nsAutoPtr.h>
#include <sbIPropertyManager.h>
#include <nsITreeView.h>

NS_IMPL_ISUPPORTS_INHERITED2(sbRatingPropertyInfo,
                             sbImmutablePropertyInfo,
                             sbIClickablePropertyInfo,
                             sbITreeViewPropertyInfo)

#define STAR_WIDTH 14
#define MAX_RATING 5
#define ZERO_HIT_WIDTH 10

sbRatingPropertyInfo::sbRatingPropertyInfo(const nsAString& aPropertyID,
                                           const nsAString& aDisplayName,
                                           const nsAString& aLocalizationKey,
                                           const PRBool aRemoteReadable,
                                           const PRBool aRemoteWritable,
                                           const PRBool aUserViewable,
                                           const PRBool aUserEditable)
{
  mID = aPropertyID;
  mDisplayName = aDisplayName;
  mLocalizationKey = aLocalizationKey;
  mUserViewable = aUserViewable;
  mUserEditable = aUserEditable;
  mRemoteReadable = aRemoteReadable;
  mRemoteWritable = aRemoteWritable;
  mType.AssignLiteral("rating");
  mSuppressSelect = PR_TRUE;
}

nsresult
sbRatingPropertyInfo::InitializeOperators()
{
  nsresult rv;
  nsAutoString op;
  nsRefPtr<sbPropertyOperator> propOp;

  rv = sbImmutablePropertyInfo::GetOPERATOR_EQUALS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.rating.equal"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbImmutablePropertyInfo::GetOPERATOR_NOTEQUALS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.rating.notequal"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbImmutablePropertyInfo::GetOPERATOR_GREATER(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp =  new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.rating.greater"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbImmutablePropertyInfo::GetOPERATOR_GREATEREQUAL(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.rating.greaterequal"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbImmutablePropertyInfo::GetOPERATOR_LESS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.rating.less"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbImmutablePropertyInfo::GetOPERATOR_LESSEQUAL(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.rating.lessequal"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbImmutablePropertyInfo::GetOPERATOR_BETWEEN(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.rating.between"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbRatingPropertyInfo::Init()
{
  nsresult rv;

  rv = sbImmutablePropertyInfo::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = InitializeOperators();
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
  *aSuppressSelect = mSuppressSelect;
  return NS_OK;
}

NS_IMETHODIMP
sbRatingPropertyInfo::SetSuppressSelect(PRBool aSuppressSelect)
{
  mSuppressSelect = aSuppressSelect;
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
    rating = ((aMouseX - ZERO_HIT_WIDTH) / STAR_WIDTH) + 1;

    if (rating > MAX_RATING) {
      rating = MAX_RATING;
    }
  }

  nsString ratingStr;
  ratingStr.AppendInt(rating);

  if (ratingStr.Equals(aCurrentValue) || ratingStr.EqualsLiteral("0")) {
    ratingStr.SetIsVoid(PR_TRUE);
  }

  _retval = ratingStr;
  return NS_OK;
}

/* boolean onClick (in sbIMediaItem aItem,
                    [optional] in nsISupports aEvent,
                    [optional] in nsISupports aContext); */
NS_IMETHODIMP
sbRatingPropertyInfo::OnClick(sbIMediaItem *aItem,
                              nsISupports *aEvent,
                              nsISupports *aContext,
                              PRBool *_retval NS_OUTPARAM)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;
  return NS_OK;
}

// sbIPropertyInfo

NS_IMETHODIMP
sbRatingPropertyInfo::Format(const nsAString& aValue,
                             nsAString& _retval)
{
  // XXXsteve Pretend that zeros are nulls.  This should be removed when
  // bug 8033 is fixed.
  if (aValue.EqualsLiteral("0")) {
    _retval.SetIsVoid(PR_TRUE);
  }
  else {
    _retval = aValue;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbRatingPropertyInfo::Validate(const nsAString& aValue, PRBool* _retval)
{
  // Value can only be 1 through MAX_RATING or null
  *_retval = PR_TRUE;
  if (aValue.IsVoid()) {
    return NS_OK;
  }

  nsresult rv;
  PRUint32 rating = aValue.ToInteger(&rv);
  if (NS_SUCCEEDED(rv) && rating >= 0 && rating <= MAX_RATING) {
    return NS_OK;
  }

  *_retval = PR_FALSE;
  return NS_OK;
}
