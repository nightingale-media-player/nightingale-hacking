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
#include <sbHashtableUtils.h>
#include <sbStringUtils.h>

NS_IMPL_ISUPPORTS_INHERITED1(sbImageLabelLinkPropertyInfo,
                             sbImageLinkPropertyInfo,
                             sbIImageLabelLinkPropertyInfo)

sbImageLabelLinkPropertyInfo::sbImageLabelLinkPropertyInfo()
  : sbImageLinkPropertyInfo(nsString(),
                            nsString(),
                            nsString(),
                            PR_FALSE,
                            PR_FALSE,
                            PR_TRUE,
                            PR_TRUE,
                            nsString()),
    mImages(nsnull),
    mLabels(nsnull),
    mClickHandlers(nsnull)
{
  mType.AssignLiteral("image");
}

sbImageLabelLinkPropertyInfo::~sbImageLabelLinkPropertyInfo()
{
  delete mImages;
  delete mLabels;
  delete mClickHandlers;
}

nsresult
sbImageLabelLinkPropertyInfo::Init()
{
  mImages = new nsClassHashtable<nsCStringHashKey, nsCString>();
  NS_ENSURE_TRUE(mImages, NS_ERROR_OUT_OF_MEMORY);
  mLabels = new nsClassHashtable<nsCStringHashKey, nsString>();
  NS_ENSURE_TRUE(mLabels, NS_ERROR_OUT_OF_MEMORY);
  mClickHandlers = new nsTHashtable<nsISupportsHashKey>();
  NS_ENSURE_TRUE(mClickHandlers, NS_ERROR_OUT_OF_MEMORY);
  mImages->Init();
  mLabels->Init();
  mClickHandlers->Init();

  nsresult rv = sbImageLinkPropertyInfo::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbImageLabelLinkPropertyInfo::Init(ImageMap_t *&aImages,
                                   LabelMap_t *&aLabels,
                                   InterfaceSet_t *&aClickHandlers)
{
  mImages = aImages;
  aImages = nsnull;
  mLabels = aLabels;
  aLabels = nsnull;
  mClickHandlers = aClickHandlers;
  aClickHandlers = nsnull;

  nsresult rv = sbImageLinkPropertyInfo::Init();
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

/***** sbIImageLinkPropertyInfo */

NS_IMETHODIMP
sbImageLabelLinkPropertyInfo::GetUrlProperty(nsAString& _retval)
{
  _retval = mUrlPropertyID;

  return NS_OK;
}

NS_IMETHODIMP
sbImageLabelLinkPropertyInfo::GetPreventNavigation(const nsAString& aImageValue,
                                                   const nsAString& aUrlValue,
                                                   PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = aImageValue.IsEmpty() || aUrlValue.IsEmpty();

  return NS_OK;
}

/***** sbIImageLabelLinkPropertyInfo */

/* void addImage (in ACString aKey, in ACString aImageUrl); */
NS_IMETHODIMP
sbImageLabelLinkPropertyInfo::AddImage(const nsACString & aKey,
                                       const nsACString & aImageUrl)
{
  NS_ENSURE_TRUE(mImages, NS_ERROR_NOT_INITIALIZED);

  if (mImages->Get(aKey, nsnull)) {
    NS_WARNING("found an existing image key for %s", aKey.BeginReading());
    return NS_OK;
  }

  PRBool success = mImages->Put(aKey, new nsCString(aImageUrl));
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  return NS_OK;
}

/* void addLabel (in ACString aKey, in AString aLabel); */
NS_IMETHODIMP
sbImageLabelLinkPropertyInfo::AddLabel(const nsACString & aKey,
                                       const nsAString & aLabel)
{
  NS_ENSURE_TRUE(mLabels, NS_ERROR_NOT_INITIALIZED);

  if (mLabels->Get(aKey, nsnull)) {
    NS_WARNING("found an existing label key for %s", aKey.BeginReading());
    return NS_OK;
  }

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

/***** sbIClickablePropertyInfo */
NS_IMETHODIMP
sbImageLabelLinkPropertyInfo::HitTest(const nsAString& aCurrentValue,
                                      const nsAString& aPart,
                                      PRUint32 aBoxWidth,
                                      PRUint32 aBoxHeight,
                                      PRUint32 aMouseX,
                                      PRUint32 aMouseY,
                                      PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = aPart.EqualsLiteral("image") ||
             aPart.EqualsLiteral("text");
  return NS_OK;
}

NS_IMETHODIMP
sbImageLabelLinkPropertyInfo::OnClick(sbIMediaItem *aItem,
                                      nsISupports *aEvent,
                                      nsISupports *aContext,
                                      PRBool *_retval NS_OUTPARAM)
{
  NS_ENSURE_ARG_POINTER(aItem);
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;

  nsresult rv;
  
  if (!mClickHandlers) {
    return NS_OK;
  }

  // copy the handlers to an auxilary array to handle removing them in the call
  nsCOMPtr<nsIMutableArray> handlers =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1");
  NS_ENSURE_TRUE(handlers, NS_ERROR_OUT_OF_MEMORY);
  sbCopyHashtableToArray(*mClickHandlers, handlers);

  nsCOMPtr<nsISimpleEnumerator> enumerator;
  rv = handlers->Enumerate(getter_AddRefs(enumerator));
  NS_ENSURE_SUCCESS(rv, rv);
  
  PRBool hasMore, result;

  while (NS_SUCCEEDED(rv = enumerator->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> supports;
    rv = enumerator->GetNext(getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<sbIClickablePropertyCallback> callback =
      do_QueryInterface(supports);
    if (!callback) {
      NS_WARNING("found a onClick callback that is "
                 "not a sbIClickablePropertyCallback");
      continue;
    }
    rv = callback->OnClick(this,
                           aItem,
                           aEvent,
                           aContext,
                           &result);
    if (NS_SUCCEEDED(rv) && result) {
      *_retval = PR_TRUE;
    }
  }
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
