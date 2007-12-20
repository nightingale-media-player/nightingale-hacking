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

#define SB_STRING_BUNDLE_CHROME_URL "chrome://songbird/locale/songbird.properties"

#include "sbDownloadButtonPropertyInfo.h"
#include "sbStandardOperators.h"

#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>
#include <nsIStringBundle.h>
#include <nsITreeView.h>
#include <nsServiceManagerUtils.h>

NS_IMPL_ISUPPORTS_INHERITED2(sbDownloadButtonPropertyInfo,
                             sbImmutablePropertyInfo,
                             sbIClickablePropertyInfo,
                             sbITreeViewPropertyInfo)

/*
 * The value of download button properties is formatted as follows:
 *
 * <mode>|<total size>|<current size>
 *
 * mode can be: 0 = none, 1 = new, 2 = starting, 3 = downloading, 4 = paused,
                5 = complete, 6 = failed
 * total size and current size are in btyes.
 *
 */
sbDownloadButtonPropertyInfo::sbDownloadButtonPropertyInfo(const nsAString& aPropertyID,
                                                           const nsAString& aDisplayName,
                                                           const nsAString& aLabel,
                                                           const nsAString& aRetryLabel,
                                                           const PRBool aRemoteReadable,
                                                           const PRBool aRemoteWritable,
                                                           const PRBool aUserViewable,
                                                           const PRBool aUserEditable)
{
  mID = aPropertyID;
  mDisplayName = aDisplayName;
  mLabel = aLabel;
  mRetryLabel = aRetryLabel;
  mUserViewable = aUserViewable;
  mUserEditable = aUserEditable;
  mRemoteReadable = aRemoteReadable;
  mRemoteWritable = aRemoteWritable;
  mType.AssignLiteral("downloadbutton");
}

nsresult
sbDownloadButtonPropertyInfo::Init()
{
  nsresult rv;

  rv = sbImmutablePropertyInfo::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// sbITreeViewPropertyInfo

NS_IMETHODIMP
sbDownloadButtonPropertyInfo::GetImageSrc(const nsAString& aValue,
                                          nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbDownloadButtonPropertyInfo::GetProgressMode(const nsAString& aValue,
                                              PRInt32* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  sbDownloadButtonPropertyValue value(aValue);

  switch(value.GetMode()) {
    case sbDownloadButtonPropertyValue::eNone:
    case sbDownloadButtonPropertyValue::eNew:
    case sbDownloadButtonPropertyValue::eComplete:
    case sbDownloadButtonPropertyValue::eFailed:
      *_retval = nsITreeView::PROGRESS_NONE;
      break;
    case sbDownloadButtonPropertyValue::eStarting:
      *_retval = nsITreeView::PROGRESS_UNDETERMINED;
      break;
    case sbDownloadButtonPropertyValue::eDownloading:
    case sbDownloadButtonPropertyValue::ePaused:
      *_retval = nsITreeView::PROGRESS_NORMAL;
      break;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDownloadButtonPropertyInfo::GetCellValue(const nsAString& aValue,
                                           nsAString& _retval)
{
  sbDownloadButtonPropertyValue value(aValue);

  switch(value.GetMode()) {
    case sbDownloadButtonPropertyValue::eDownloading:
    case sbDownloadButtonPropertyValue::ePaused:
      if (value.GetTotal() > 0) {
        PRFloat64 progress =
          ((PRFloat64) value.GetCurrent() / (PRFloat64) value.GetTotal()) * 100;
        _retval.AppendInt((PRUint32) progress);
      }
      break;
    default:
      _retval.Truncate();
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDownloadButtonPropertyInfo::GetColumnProperties(const nsAString& aValue,
                                                  nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbDownloadButtonPropertyInfo::GetRowProperties(const nsAString& aValue,
                                               nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbDownloadButtonPropertyInfo::GetCellProperties(const nsAString& aValue,
                                                nsAString& _retval)
{
  sbDownloadButtonPropertyValue value(aValue);

  switch(value.GetMode()) {
    case sbDownloadButtonPropertyValue::eNew:
      _retval.AssignLiteral("button");
      break;
    case sbDownloadButtonPropertyValue::eStarting:
      _retval.AssignLiteral("progressNotStarted");
      break;
    case sbDownloadButtonPropertyValue::eComplete:
      _retval.AssignLiteral("progressCompleted");
      break;
    case sbDownloadButtonPropertyValue::ePaused:
      _retval.AssignLiteral("progressPaused");
      break;
    case sbDownloadButtonPropertyValue::eFailed:
      _retval.AssignLiteral("button progressFailed");
      break;
    default:
      _retval.Truncate();
  }

  _retval.AppendLiteral(" downloadbutton");

  return NS_OK;
}

NS_IMETHODIMP
sbDownloadButtonPropertyInfo::GetColumnType(nsAString& _retval)
{
  _retval.AssignLiteral("progressmeter");
  return NS_OK;
}

// sbIClickablePropertyInfo

NS_IMETHODIMP
sbDownloadButtonPropertyInfo::GetSuppressSelect(PRBool* aSuppressSelect)
{
  NS_ENSURE_ARG_POINTER(aSuppressSelect);
  *aSuppressSelect = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbDownloadButtonPropertyInfo::IsDisabled(const nsAString& aCurrentValue,
                                         PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbDownloadButtonPropertyInfo::HitTest(const nsAString& aCurrentValue,
                                      const nsAString& aPart,
                                      PRUint32 aBoxWidth,
                                      PRUint32 aBoxHeight,
                                      PRUint32 aMouseX,
                                      PRUint32 aMouseY,
                                      PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = aPart.EqualsLiteral("text");
  return NS_OK;
}

NS_IMETHODIMP
sbDownloadButtonPropertyInfo::GetValueForClick(const nsAString& aCurrentValue,
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
sbDownloadButtonPropertyInfo::Format(const nsAString& aValue,
                                     nsAString& _retval)
{
  sbDownloadButtonPropertyValue value(aValue);

  switch(value.GetMode()) {
    case sbDownloadButtonPropertyValue::eNew:
      _retval = mLabel;
      break;
    case sbDownloadButtonPropertyValue::eFailed:
      _retval = mRetryLabel;
      break;
    default:
      _retval.Truncate();
  }

  return NS_OK;
}

