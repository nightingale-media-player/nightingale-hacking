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

NS_IMPL_ISUPPORTS_INHERITED1(sbRatingPropertyInfo, sbNumberPropertyInfo,
                                                   sbIClickablePropertyInfo)
#define STAR_WIDTH 16
#define MAX_RATING 5

sbRatingPropertyInfo::sbRatingPropertyInfo()
{
  MOZ_COUNT_CTOR(sbRatingPropertyInfo);
}

sbRatingPropertyInfo::~sbRatingPropertyInfo()
{
  MOZ_COUNT_DTOR(sbRatingPropertyInfo);
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
  if (aMouseX < 7) {
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

NS_IMETHODIMP
sbRatingPropertyInfo::GetDisplayPropertiesForValue(const nsAString& aValue,
                                                   nsAString& _retval)
{
  nsString property;
  property.AssignLiteral("rating");
  property.Append(aValue);

  _retval = property;
  return NS_OK;
}

