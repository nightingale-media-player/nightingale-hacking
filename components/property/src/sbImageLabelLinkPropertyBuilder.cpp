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

#include "sbImageLabelLinkPropertyBuilder.h"

#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>
#include <nsITreeView.h>

#include <nsAutoPtr.h>
#include <nsIURI.h>
#include <nsNetUtil.h>
#include <nsServiceManagerUtils.h>
#include <nsUnicharUtils.h>
#include <sbStringUtils.h>

#include "sbImageLabelLinkPropertyInfo.h"

NS_IMPL_ISUPPORTS_INHERITED1(sbImageLabelLinkPropertyBuilder, \
                             sbAbstractPropertyBuilder, \
                             sbIImageLabelLinkPropertyBuilder)

sbImageLabelLinkPropertyBuilder::sbImageLabelLinkPropertyBuilder()
  : mImages(nsnull),
    mLabels(nsnull)
{
}

sbImageLabelLinkPropertyBuilder::~sbImageLabelLinkPropertyBuilder()
{
  delete mImages;
  delete mLabels;
}

nsresult
sbImageLabelLinkPropertyBuilder::Init()
{
  nsresult rv;

  mImages = new nsClassHashtable<nsCStringHashKey, nsCString>();
  NS_ENSURE_TRUE(mImages, NS_ERROR_OUT_OF_MEMORY);
  mLabels = new nsClassHashtable<nsCStringHashKey, nsString>();
  NS_ENSURE_TRUE(mLabels, NS_ERROR_OUT_OF_MEMORY);
  mImages->Init();
  mLabels->Init();
  rv = sbAbstractPropertyBuilder::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/***** sbIPropertyBuilder */

NS_IMETHODIMP
sbImageLabelLinkPropertyBuilder::Get(sbIPropertyInfo** _retval)
{
  nsresult rv;
  nsRefPtr<sbImageLabelLinkPropertyInfo> info =
    new sbImageLabelLinkPropertyInfo(mImages, mLabels);
  rv = info->Init();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = info->SetPropertyID(mPropertyID);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = info->SetDisplayName(mDisplayName);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = info->SetLocalizationKey(mDisplayNameKey);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = info->SetRemoteReadable(mRemoteReadable);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = info->SetRemoteWritable(mRemoteWritable);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = info->SetUserViewable(mUserViewable);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = info->SetUserEditable(mUserEditable);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return CallQueryInterface(info.get(), _retval);
}

/***** sbIImageLabelLinkPropertyBuilder */

/* void addImage (in ACString aKey, in ACString aImageUrl); */
NS_IMETHODIMP
sbImageLabelLinkPropertyBuilder::AddImage(const nsACString & aKey,
                                          const nsACString & aImageUrl)
{
  NS_ENSURE_TRUE(mImages, NS_ERROR_NOT_INITIALIZED);
  PRBool success = mImages->Put(aKey, new nsCString(aImageUrl));
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  return NS_OK;
}

/* ACString getImage (in ACString aKey); */
NS_IMETHODIMP
sbImageLabelLinkPropertyBuilder::GetImage(const nsACString & aKey,
                                          nsACString & _retval NS_OUTPARAM)
{
  NS_ENSURE_TRUE(mImages, NS_ERROR_NOT_INITIALIZED);

  nsCString key(aKey), *result;

  // look for the key as given
  if (mImages->Get(aKey, &result)) {
    _retval.Assign(*result);
    return NS_OK;
  }
  // look for a default value
  if (mImages->Get(nsCString(), &result)) {
    _retval.Assign(*result);
    return NS_OK;
  }
  // nothing at all; give an empty string back
  _retval.Truncate();
  return NS_OK;
}

/* void addLabel (in ACString aKey, in AString aLabel); */
NS_IMETHODIMP
sbImageLabelLinkPropertyBuilder::AddLabel(const nsACString & aKey,
                                          const nsAString & aLabel)
{
  NS_ENSURE_TRUE(mLabels, NS_ERROR_NOT_INITIALIZED);

  nsString value(aLabel);

  if (StringBeginsWith(aLabel, NS_LITERAL_STRING("&")) &&
      StringEndsWith(aLabel, NS_LITERAL_STRING(";")))
  {
    // localize the string
    value = SBLocalizedString(Substring(aLabel, 1, aLabel.Length() - 2));
  }

  PRBool success = mLabels->Put(aKey, new nsString(value));
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  return NS_OK;
}

/* AString getLabel (in ACString aKey); */
NS_IMETHODIMP
sbImageLabelLinkPropertyBuilder::GetLabel(const nsACString & aKey,
                                          nsAString & _retval NS_OUTPARAM)
{
  NS_ENSURE_TRUE(mLabels, NS_ERROR_NOT_INITIALIZED);

  nsString *result;

  // look for the key as given
  if (mLabels->Get(aKey, &result)) {
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
