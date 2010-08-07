/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
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
 *=END SONGBIRD GPL
 */

#include "sbImageLabelLinkPropertyInfo.h"

#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>
#include <nsITreeView.h>

#include <nsIURI.h>
#include <nsNetUtil.h>
#include <nsServiceManagerUtils.h>
#include <nsUnicharUtils.h>
#include <sbStringUtils.h>

sbImageLabelLinkPropertyInfo::sbImageLabelLinkPropertyInfo(ImageMap_t *&aImages,
                                                           LabelMap_t *&aLabels)
  : sbImageLinkPropertyInfo(nsString(),
                            nsString(),
                            nsString(),
                            PR_FALSE,
                            PR_FALSE,
                            PR_TRUE,
                            PR_TRUE,
                            nsString())
{
  mImages = aImages;
  aImages = nsnull;
  mLabels = aLabels;
  aLabels = nsnull;
  mType.AssignLiteral("image");
  mSuppressSelect = PR_TRUE;
}

nsresult
sbImageLabelLinkPropertyInfo::Init()
{
  nsresult rv;

  rv = sbImageLinkPropertyInfo::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbImageLabelLinkPropertyInfo::SetPropertyID(const nsAString& aPropertyID)
{
  mID = aPropertyID;
  return NS_OK;
}

NS_IMETHODIMP
sbImageLabelLinkPropertyInfo::SetDisplayName(const nsAString& aDisplayName)
{
  mDisplayName = aDisplayName;
  return NS_OK;
}

NS_IMETHODIMP
sbImageLabelLinkPropertyInfo::SetLocalizationKey(const nsAString& aLocalizationKey)
{
  mLocalizationKey = aLocalizationKey;
  return NS_OK;
}

NS_IMETHODIMP
sbImageLabelLinkPropertyInfo::SetRemoteReadable(PRBool aRemoteReadable)
{
  mRemoteReadable = aRemoteReadable;
  return NS_OK;
}

NS_IMETHODIMP
sbImageLabelLinkPropertyInfo::SetRemoteWritable(PRBool aRemoteWritable)
{
  mRemoteWritable = aRemoteWritable;
  return NS_OK;
}

NS_IMETHODIMP
sbImageLabelLinkPropertyInfo::SetUserViewable(PRBool aUserViewable)
{
  mUserViewable = aUserViewable;
  return NS_OK;
}

NS_IMETHODIMP
sbImageLabelLinkPropertyInfo::SetUserEditable(PRBool aUserEditable)
{
  mUserEditable = aUserEditable;
  return NS_OK;
}

NS_IMETHODIMP
sbImageLabelLinkPropertyInfo::SetUrlPropertyID(const nsAString& aUrlPropertyID)
{
  mUrlPropertyID = aUrlPropertyID;
  return NS_OK;
}

/***** sbITreeViewPropertyInfo */

NS_IMETHODIMP
sbImageLabelLinkPropertyInfo::GetImageSrc(const nsAString& aValue,
                                          nsAString& _retval)
{
  NS_ENSURE_TRUE(mImages, NS_ERROR_NOT_INITIALIZED);

  NS_LossyConvertUTF16toASCII key(aValue);
  nsCString *result;

  // look for the key as given
  if (mImages->Get(key, &result)) {
    CopyASCIItoUTF16(*result, _retval);
    return NS_OK;
  }
  // look for a default value
  if (mImages->Get(nsCString(), &result)) {
    CopyASCIItoUTF16(*result, _retval);
    return NS_OK;
  }
  // nothing at all; give an empty string back
  _retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbImageLabelLinkPropertyInfo::GetCellProperties(const nsAString& aValue,
                                                nsAString& _retval)
{
  if(aValue.IsEmpty()) {
    _retval.AssignLiteral("image label noLink");
    return NS_OK;
  }

  _retval.AssignLiteral("image label link");

  return NS_OK;
}

/***** sbIPropertyInfo */

NS_IMETHODIMP
sbImageLabelLinkPropertyInfo::Format(const nsAString& aValue,
                                     nsAString& _retval)
{
  NS_ENSURE_TRUE(mLabels, NS_ERROR_NOT_INITIALIZED);

  NS_LossyConvertUTF16toASCII key(aValue);
  nsString *result;

  // look for the key as given
  if (mLabels->Get(key, &result)) {
    _retval.Assign(*result);
    return NS_OK;
  }
  // look for a default value
  if (mLabels->Get(nsCString(), &result)) {
    _retval.Assign(*result);
    return NS_OK;
  }
  // nothing at all; give an empty string back
  _retval.Truncate();
  return NS_OK;
}
