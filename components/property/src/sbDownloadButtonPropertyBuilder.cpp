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

#include "sbDownloadButtonPropertyBuilder.h"
#include "sbDownloadButtonPropertyInfo.h"

#include <nsAutoPtr.h>

#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>
#include <nsIStringBundle.h>

#include "sbStandardProperties.h"

NS_IMPL_ISUPPORTS_INHERITED2(sbDownloadButtonPropertyBuilder,
                             sbAbstractPropertyBuilder,
                             sbISimpleButtonPropertyBuilder,
                             sbIDownloadButtonPropertyBuilder)

nsresult
sbDownloadButtonPropertyBuilder::Init()
{
  nsresult rv;
  rv = sbAbstractPropertyBuilder::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetPropertyID(NS_LITERAL_STRING(SB_PROPERTY_DOWNLOADBUTTON));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetDisplayNameKey(NS_LITERAL_STRING("property.download_button"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetLabelKey(NS_LITERAL_STRING("property.download_button"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetRetryLabelKey(NS_LITERAL_STRING("property.download_button_retry"));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;  
}

NS_IMETHODIMP
sbDownloadButtonPropertyBuilder::Get(sbIPropertyInfo** _retval)
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
  
  nsString retryLabel;
  if (!mRetryLabelKey.IsEmpty()) {
    rv = GetStringFromName(mBundle, mRetryLabelKey, retryLabel);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    retryLabel = mRetryLabel;
  }

  nsRefPtr<sbDownloadButtonPropertyInfo> pi =
    new sbDownloadButtonPropertyInfo(mPropertyID,
                                     displayName,
                                     mDisplayNameKey,
                                     label,
                                     retryLabel,
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
sbDownloadButtonPropertyBuilder::GetLabel(nsAString& aLabel)
{
  aLabel = mLabel;
  return NS_OK;
}
NS_IMETHODIMP
sbDownloadButtonPropertyBuilder::SetLabel(const nsAString& aLabel)
{
  mLabel = aLabel;
  return NS_OK;
}

NS_IMETHODIMP
sbDownloadButtonPropertyBuilder::GetRetryLabel(nsAString& aRetryLabel)
{
  aRetryLabel = mRetryLabel;
  return NS_OK;
}
NS_IMETHODIMP
sbDownloadButtonPropertyBuilder::SetRetryLabel(const nsAString& aRetryLabel)
{
  mRetryLabel = aRetryLabel;
  return NS_OK;
}

NS_IMETHODIMP
sbDownloadButtonPropertyBuilder::GetLabelKey(nsAString& aLabelKey)
{
  aLabelKey = mLabelKey;
  return NS_OK;
}
NS_IMETHODIMP
sbDownloadButtonPropertyBuilder::SetLabelKey(const nsAString& aLabelKey)
{
  mLabelKey = aLabelKey;
  return NS_OK;
}

NS_IMETHODIMP
sbDownloadButtonPropertyBuilder::GetRetryLabelKey(nsAString& aRetryLabelKey)
{
  aRetryLabelKey = mRetryLabelKey;
  return NS_OK;
}
NS_IMETHODIMP
sbDownloadButtonPropertyBuilder::SetRetryLabelKey(const nsAString& aRetryLabelKey)
{
  mRetryLabelKey = aRetryLabelKey;
  return NS_OK;
}
