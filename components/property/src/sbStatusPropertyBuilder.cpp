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

#include "sbStatusPropertyBuilder.h"
#include "sbStatusPropertyInfo.h"

#include <nsAutoPtr.h>

#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>
#include <nsIStringBundle.h>

NS_IMPL_ISUPPORTS_INHERITED1(sbStatusPropertyBuilder,
                             sbAbstractPropertyBuilder,
                             sbIStatusPropertyBuilder)

NS_IMETHODIMP
sbStatusPropertyBuilder::Get(sbIPropertyInfo** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_STATE(!mPropertyID.IsEmpty());

  nsString displayName;
  nsresult rv = GetFinalDisplayName(displayName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString label;
  if (!mLabelKey.IsEmpty()) {
    rv = GetStringFromName(mBundle, mLabelKey, label);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    label = mLabel;
  }

  nsString completedLabel;
  if (!mCompletedLabelKey.IsEmpty()) {
    rv = GetStringFromName(mBundle, mCompletedLabelKey, completedLabel);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    completedLabel = mCompletedLabel;
  }

  nsString failedLabel;
  if (!mFailedLabelKey.IsEmpty()) {
    rv = GetStringFromName(mBundle, mFailedLabelKey, failedLabel);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    failedLabel = mFailedLabel;
  }

  nsRefPtr<sbStatusPropertyInfo> pi =
    new sbStatusPropertyInfo(mPropertyID,
                             displayName,
                             mDisplayNameKey,
                             label,
                             completedLabel,
                             failedLabel,
                             mRemoteReadable,
                             mRemoteWritable,
                             mUserViewable,
                             mUserEditable);
  NS_ENSURE_TRUE(pi, NS_ERROR_OUT_OF_MEMORY);

  rv = pi->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = pi);
  return NS_OK;
}

NS_IMETHODIMP
sbStatusPropertyBuilder::GetLabel(nsAString& aLabel)
{
  aLabel = mLabel;
  return NS_OK;
}
NS_IMETHODIMP
sbStatusPropertyBuilder::SetLabel(const nsAString& aLabel)
{
  mLabel = aLabel;
  return NS_OK;
}

NS_IMETHODIMP
sbStatusPropertyBuilder::GetCompletedLabel(nsAString& aCompletedLabel)
{
  aCompletedLabel = mCompletedLabel;
  return NS_OK;
}
NS_IMETHODIMP
sbStatusPropertyBuilder::SetCompletedLabel(const nsAString& aCompletedLabel)
{
  mCompletedLabel = aCompletedLabel;
  return NS_OK;
}

NS_IMETHODIMP
sbStatusPropertyBuilder::GetFailedLabel(nsAString& aFailedLabel)
{
  aFailedLabel = mFailedLabel;
  return NS_OK;
}
NS_IMETHODIMP
sbStatusPropertyBuilder::SetFailedLabel(const nsAString& aFailedLabel)
{
  mFailedLabel = aFailedLabel;
  return NS_OK;
}

NS_IMETHODIMP
sbStatusPropertyBuilder::GetLabelKey(nsAString& aLabelKey)
{
  aLabelKey = mLabelKey;
  return NS_OK;
}
NS_IMETHODIMP
sbStatusPropertyBuilder::SetLabelKey(const nsAString& aLabelKey)
{
  mLabelKey = aLabelKey;
  return NS_OK;
}

NS_IMETHODIMP
sbStatusPropertyBuilder::GetCompletedLabelKey(nsAString& aCompletedLabelKey)
{
  aCompletedLabelKey = mCompletedLabelKey;
  return NS_OK;
}
NS_IMETHODIMP
sbStatusPropertyBuilder::SetCompletedLabelKey(const nsAString& aCompletedLabelKey)
{
  mCompletedLabelKey = aCompletedLabelKey;
  return NS_OK;
}

NS_IMETHODIMP
sbStatusPropertyBuilder::GetFailedLabelKey(nsAString& aFailedLabelKey)
{
  aFailedLabelKey = mFailedLabelKey;
  return NS_OK;
}
NS_IMETHODIMP
sbStatusPropertyBuilder::SetFailedLabelKey(const nsAString& aFailedLabelKey)
{
  mFailedLabelKey = aFailedLabelKey;
  return NS_OK;
}
