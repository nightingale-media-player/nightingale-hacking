/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.getnightingale.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END NIGHTINGALE GPL
 */


#include "sbStatusPropertyInfo.h"
#include "sbStatusPropertyValue.h"
#include "sbStandardOperators.h"

#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>
#include <nsIStringBundle.h>
#include <nsITreeView.h>
#include <nsServiceManagerUtils.h>


NS_IMPL_ISUPPORTS_INHERITED1(sbStatusPropertyInfo,
                             sbImmutablePropertyInfo,
                             sbITreeViewPropertyInfo)

/*
 * The value of status properties is formatted as follows:
 *
 * <mode>|<current value>
 *
 * mode can be: 0 = none, 1 = ripping, 2 = complete, 3 = failed
 * current value: 0 to 100
 *
 */
sbStatusPropertyInfo::sbStatusPropertyInfo(const nsAString& aPropertyID,
                                           const nsAString& aDisplayName,
                                           const nsAString& aLocalizationKey,
                                           const nsAString& aLabel,
                                           const nsAString& aCompletedLabel,
                                           const nsAString& aFailedLabel,
                                           const PRBool aRemoteReadable,
                                           const PRBool aRemoteWritable,
                                           const PRBool aUserViewable,
                                           const PRBool aUserEditable)
{
  // sbImmutablePropertyInfo initialization
  mID = aPropertyID;
  mDisplayName = aDisplayName;
  mLocalizationKey = aLocalizationKey;
  mLabel = aLabel;
  mCompletedLabel = aCompletedLabel;
  mFailedLabel = aFailedLabel;
  mUserViewable = aUserViewable;
  mUserEditable = aUserEditable;
  mRemoteReadable = aRemoteReadable;
  mRemoteWritable = aRemoteWritable;
  mType.AssignLiteral("status");
}

nsresult
sbStatusPropertyInfo::Init()
{
  nsresult rv;

  rv = sbImmutablePropertyInfo::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// sbITreeViewPropertyInfo

NS_IMETHODIMP
sbStatusPropertyInfo::GetImageSrc(const nsAString& aValue,
                                  nsAString& retval)
{
  retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbStatusPropertyInfo::GetProgressMode(const nsAString& aValue,
                                      PRInt32* retval)
{
  NS_ENSURE_ARG_POINTER(retval);

  sbStatusPropertyValue value(aValue);

  switch(value.GetMode()) {
    case sbStatusPropertyValue::eNone:
    case sbStatusPropertyValue::eComplete:
    case sbStatusPropertyValue::eFailed:
      *retval = nsITreeView::PROGRESS_NONE;
      break;
    case sbStatusPropertyValue::eRipping:
     *retval = nsITreeView::PROGRESS_NORMAL;
      break;
    default:
      NS_WARNING("Unexpected sbStatusPropertyValue mode");
      *retval = nsITreeView::PROGRESS_UNDETERMINED;
      break;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbStatusPropertyInfo::GetCellValue(const nsAString& aValue,
                                   nsAString& retval)
{
  sbStatusPropertyValue value(aValue);
  retval.Truncate();

  if (value.GetMode() == sbStatusPropertyValue::eRipping) {
    retval.AppendInt(value.GetCurrent());
  }

  return NS_OK;
}

NS_IMETHODIMP
sbStatusPropertyInfo::GetColumnProperties(const nsAString& aValue,
                                          nsAString& retval)
{
  retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbStatusPropertyInfo::GetRowProperties(const nsAString& aValue,
                                       nsAString& retval)
{
  retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbStatusPropertyInfo::GetCellProperties(const nsAString& aValue,
                                        nsAString& retval)
{
  sbStatusPropertyValue value(aValue);

  switch(value.GetMode()) {
    case sbStatusPropertyValue::eComplete:
      retval.AssignLiteral("progressCompleted ");
      break;
    case sbStatusPropertyValue::eFailed:
      retval.AssignLiteral("progressFailed ");
      break;
    default:
      retval.Truncate();
  }

  retval.AppendLiteral("status");

  return NS_OK;
}

NS_IMETHODIMP
sbStatusPropertyInfo::GetColumnType(nsAString& retval)
{
  retval.AssignLiteral("progressmeter");
  return NS_OK;
}

// sbIPropertyInfo

NS_IMETHODIMP
sbStatusPropertyInfo::Format(const nsAString& aValue,
                             nsAString& retval)
{
  sbStatusPropertyValue value(aValue);

  switch(value.GetMode()) {
    case sbStatusPropertyValue::eNone:
      retval = mLabel;
      break;
    case sbStatusPropertyValue::eComplete:
      retval = mCompletedLabel;
      break;
    case sbStatusPropertyValue::eFailed:
      retval = mFailedLabel;
      break;
    default:
      retval.Truncate();
  }

  return NS_OK;
}

