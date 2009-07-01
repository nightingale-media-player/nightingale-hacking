/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#include "sbTranscodeAlbumArt.h"

#include <nsIIOService.h>
#include <nsIURI.h>
#include <nsIResProtocolHandler.h>
#include <nsIFileProtocolHandler.h>
#include <nsIMIMEService.h>
#include <nsIProtocolHandler.h>
#include <nsIBufferedStreams.h>
#include <nsIArray.h>
#include <nsIMutableArray.h>
#include <nsIFileStreams.h>

#include <imgITools.h>
#include <nsComponentManagerUtils.h>
#include <nsTArray.h>

#include <sbProxyUtils.h>
#include <sbStandardProperties.h>
#include <sbProxiedComponentManager.h>
#include <sbTArrayStringEnumerator.h>

#include <sbIJobProgress.h>
#include <sbIFileMetadataService.h>

NS_IMPL_THREADSAFE_ISUPPORTS1(sbTranscodeAlbumArt,
                              sbITranscodeAlbumArt)

sbTranscodeAlbumArt::sbTranscodeAlbumArt()
    : mHasAlbumArt(PR_FALSE),
      mImageHeight(0),
      mImageWidth(0)
{
}

sbTranscodeAlbumArt::~sbTranscodeAlbumArt()
{
}

NS_IMETHODIMP
sbTranscodeAlbumArt::Init(sbIMediaItem *aItem, nsIArray *aImageFormats)
{
  NS_ENSURE_ARG_POINTER (aItem);
  NS_ENSURE_ARG_POINTER (aImageFormats);

  nsresult rv;
  nsString imageSpec;
  nsCString cImageSpec;

  mImageFormats = aImageFormats;
  mItem = aItem;

  rv = mItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_PRIMARYIMAGEURL),
          imageSpec);
  if (NS_FAILED (rv) || imageSpec.IsEmpty()) {
    mHasAlbumArt = PR_FALSE;
    return NS_OK;
  }

  nsCOMPtr<nsIIOService> ioservice =
      do_ProxiedGetService("@mozilla.org/network/io-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> imageURI;
  rv = ioservice->NewURI(cImageSpec, nsnull, nsnull,
          getter_AddRefs(imageURI));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> proxiedURI;
  rv = SB_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                            NS_GET_IID(nsIURI),
                            imageURI,
                            NS_PROXY_SYNC,
                            getter_AddRefs(proxiedURI));

  PRBool isResource;
  rv = proxiedURI->SchemeIs("resource", &isResource);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isResource) {
    nsCOMPtr<nsIProtocolHandler> resHandler;
    rv = ioservice->GetProtocolHandler("resource", getter_AddRefs(resHandler));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIResProtocolHandler> resourceProtocolHandler =
        do_QueryInterface(resHandler, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIResProtocolHandler> proxiedResourceProtocolHandler;
    rv = SB_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                              NS_GET_IID(nsIResProtocolHandler),
                              resourceProtocolHandler,
                              NS_PROXY_SYNC,
                              getter_AddRefs(proxiedResourceProtocolHandler));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = proxiedResourceProtocolHandler->ResolveURI(imageURI, cImageSpec);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    cImageSpec = NS_LossyConvertUTF16toASCII(imageSpec);
  }

  nsCOMPtr<nsIProtocolHandler> fileHandler;
  rv = ioservice->GetProtocolHandler("file", getter_AddRefs(fileHandler));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIFileProtocolHandler> fileProtocolHandler =
      do_QueryInterface(fileHandler, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIFileProtocolHandler> proxiedFileProtocolHandler;
  rv = SB_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                            NS_GET_IID(nsIFileProtocolHandler),
                            fileProtocolHandler,
                            NS_PROXY_SYNC,
                            getter_AddRefs(proxiedFileProtocolHandler));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> imageFile;
  rv = proxiedFileProtocolHandler->GetFileFromURLSpec(cImageSpec,
          getter_AddRefs(imageFile));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMIMEService> mimeService =
      do_ProxiedGetService("@mozilla.org/mime;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mimeService->GetTypeFromFile(imageFile, mImageMimeType);
  NS_ENSURE_SUCCESS(rv, rv);

  // Load the actual data from the file; we'll reuse this 
  nsCOMPtr<nsIFileInputStream> inputStream =
     do_CreateInstance("@mozilla.org/network/file-input-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = inputStream->Init(imageFile, 0x01, 0600, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIBufferedInputStream> bufferedInputStream =
      do_CreateInstance("@mozilla.org/network/buffered-input-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = bufferedInputStream->Init(inputStream, 1024);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<imgITools> imgTools = do_GetService(
          "@mozilla.org/image/tools;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = imgTools->DecodeImageData(bufferedInputStream, mImageMimeType,
          getter_AddRefs(mImgContainer));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mImgContainer->GetHeight(&mImageHeight);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mImgContainer->GetWidth(&mImageWidth);
  NS_ENSURE_SUCCESS(rv, rv);

  mHasAlbumArt = PR_TRUE;

  return NS_OK;
}

nsresult
sbTranscodeAlbumArt::IsValidSizeForRange(sbIDevCapRange *aRange, PRInt32 aVal,
        PRBool *aIsValid)
{
  NS_ENSURE_ARG_POINTER (aRange);
  NS_ENSURE_ARG_POINTER (aVal);
  NS_ENSURE_ARG_POINTER (aIsValid);

  nsresult rv;
  PRUint32 valueCount;

  rv = aRange->GetValueCount(&valueCount);
  NS_ENSURE_SUCCESS (rv, rv);

  if (valueCount == 0) {
    PRInt32 min, max, step;
    rv = aRange->GetMin(&min);
    NS_ENSURE_SUCCESS (rv, rv);
    rv = aRange->GetMax(&max);
    NS_ENSURE_SUCCESS (rv, rv);
    rv = aRange->GetStep(&step);
    NS_ENSURE_SUCCESS (rv, rv);

    if (min <= aVal && max >= aVal && (step == 0 || aVal % step == 0)) {
      *aIsValid = PR_TRUE;
    }
    else {
      *aIsValid = PR_FALSE;
    }
    return NS_OK;
  }

  for (PRUint32 i = 0; i < valueCount; i++) {
    PRInt32 val;
    rv = aRange->GetValue(i, &val);
    NS_ENSURE_SUCCESS (rv, rv);

    if (val == aVal) {
      *aIsValid = PR_TRUE;
      return NS_OK;
    }
  }

  *aIsValid = PR_FALSE;
  return NS_OK;
}

nsresult
sbTranscodeAlbumArt::IsValidSizeForFormat(sbIImageFormatType *aFormat,
        PRBool *aIsValid)
{
  NS_ENSURE_ARG_POINTER (aFormat);
  NS_ENSURE_ARG_POINTER (aIsValid);

  nsresult rv;
  nsCOMPtr<sbIDevCapRange> widthRange;
  nsCOMPtr<sbIDevCapRange> heightRange;

  rv = aFormat->GetSupportedWidths(getter_AddRefs(widthRange));
  if (NS_SUCCEEDED (rv) && widthRange) {
    rv = aFormat->GetSupportedHeights(getter_AddRefs(heightRange));
    if (NS_SUCCEEDED (rv) && heightRange) {
      // Ok, we have ranges: check if we're within them.
      PRBool validWidth, validHeight;
      rv = IsValidSizeForRange(widthRange, mImageWidth, &validWidth);
      NS_ENSURE_SUCCESS (rv, rv);

      rv = IsValidSizeForRange(heightRange, mImageHeight, &validHeight);
      NS_ENSURE_SUCCESS (rv, rv);

      if (validWidth && validHeight) {
        *aIsValid = PR_TRUE;
        return NS_OK;
      }
    }
  }

  // We don't have ranges, so check for explicit sizes.
  nsCOMPtr<nsIArray> explicitSizes;
  rv = aFormat->GetSupportedExplicitSizes(getter_AddRefs(explicitSizes));
  NS_ENSURE_SUCCESS (rv, rv);

  PRUint32 numSizes;
  rv = explicitSizes->GetLength(&numSizes);
  NS_ENSURE_SUCCESS (rv, rv);

  for (PRUint32 i = 0; i < numSizes; i++) {
    nsCOMPtr<sbIImageSize> size;
    rv = explicitSizes->QueryElementAt(i, NS_GET_IID(sbIImageSize),
            getter_AddRefs(size));
    NS_ENSURE_SUCCESS (rv, rv);

    PRInt32 width, height;
    rv = size->GetWidth(&width);
    NS_ENSURE_SUCCESS (rv, rv);
    rv = size->GetHeight(&height);
    NS_ENSURE_SUCCESS (rv, rv);

    if (mImageWidth == width && mImageHeight == height) {
      *aIsValid = PR_TRUE;
      return NS_OK;
    }
  }

  *aIsValid = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeAlbumArt::GetNeedsAlbumArtConversion(PRBool *aNeedsConversion)
{
  NS_ENSURE_ARG_POINTER (aNeedsConversion);
  NS_ENSURE_STATE (mImageFormats);

  nsresult rv;

  if (!mHasAlbumArt) {
    *aNeedsConversion = PR_FALSE;
    return NS_OK;
  }

  PRUint32 numFormats = 0;
  rv = mImageFormats->GetLength(&numFormats);
  NS_ENSURE_SUCCESS (rv, rv);

  /* If there are zero supported formats, then rather than saying that it's
     not supported, we assume that we just don't have any information about
     what formats are supported for this particular device - so there's no
     point in converting. After all, what would we convert TO? */
  if (numFormats == 0) {
    *aNeedsConversion = PR_FALSE;
    return NS_OK;
  }

  for (PRUint32 i = 0; i < numFormats; i++) {
    nsCOMPtr<sbIImageFormatType> format;
    rv = mImageFormats->QueryElementAt(i, NS_GET_IID(sbIImageFormatType),
            getter_AddRefs(format));
    NS_ENSURE_SUCCESS (rv, rv);

    nsCString formatMimeType;
    rv = format->GetImageFormat(formatMimeType);
    NS_ENSURE_SUCCESS (rv, rv);

    if (formatMimeType == mImageMimeType) {
      // Right format, now let's check sizes...
      PRBool valid;
      rv = IsValidSizeForFormat(format, &valid);
      NS_ENSURE_SUCCESS (rv, rv);
      
      if (valid) {
        *aNeedsConversion = PR_FALSE;
        return NS_OK;
      }
    }
  }

  *aNeedsConversion = PR_TRUE;
  return NS_OK;
}

nsresult
sbTranscodeAlbumArt::GetTargetFormat(
        nsCString aMimeType, PRInt32 *aWidth, PRInt32 *aHeight)
{
  NS_ENSURE_ARG_POINTER (aWidth);
  NS_ENSURE_ARG_POINTER (aHeight);
  NS_ENSURE_STATE (mImageFormats);

  nsresult rv;
  /* We take the first format for which we have an image encoder, and then
     use the first explicit size within that.
     If there are no formats, or no explicit widths, error out. */
  nsCOMPtr<sbIImageFormatType> format;
  rv = mImageFormats->QueryElementAt(0, NS_GET_IID(sbIImageFormatType),
          getter_AddRefs(format));
  NS_ENSURE_SUCCESS (rv, rv);

  nsCOMPtr<nsIArray> explicitSizes;
  rv = format->GetSupportedExplicitSizes(getter_AddRefs(explicitSizes));
  NS_ENSURE_SUCCESS (rv, rv);

  nsCOMPtr<sbIImageSize> size;
  rv = explicitSizes->QueryElementAt(0, NS_GET_IID(sbIImageSize),
          getter_AddRefs(size));
  NS_ENSURE_SUCCESS (rv, rv);

  rv = size->GetWidth(aWidth);
  NS_ENSURE_SUCCESS (rv, rv);

  rv = size->GetHeight(aHeight);
  NS_ENSURE_SUCCESS (rv, rv);

  rv = format->GetImageFormat(aMimeType);
  NS_ENSURE_SUCCESS (rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeAlbumArt::ConvertArt()
{
  NS_ENSURE_STATE (mImageFormats);
  NS_ENSURE_STATE (mItem);

  NS_ASSERTION(!NS_IsMainThread(),
          "ConvertArt must not be called from the main thread");

  nsresult rv;
  nsCString mimeType;
  PRInt32 width, height;

  rv = GetTargetFormat(mimeType, &width, &height);
  NS_ENSURE_SUCCESS (rv, rv);

  nsCOMPtr<imgITools> imgTools = do_GetService(
          "@mozilla.org/image/tools;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInputStream> imageStream;
  rv = imgTools->EncodeScaledImage(mImgContainer, mimeType, width, height,
          getter_AddRefs(imageStream));
  NS_ENSURE_SUCCESS(rv, rv);

  // Ok. We have a replacement image. Now we want to write it to the media item
  // (which points at the copy of the file on the device).
  // TODO: what if we can't write to that (e.g. MTP)? 

  nsCOMPtr<nsIMutableArray> mediaItemArray =
      do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mediaItemArray->AppendElement(mItem, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsTArray<nsString> propArray;
  NS_ENSURE_TRUE(propArray.AppendElement(
              NS_LITERAL_STRING(SB_PROPERTY_PRIMARYIMAGEURL)),
          NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIStringEnumerator> propsToWrite =
    new sbTArrayStringEnumerator(&propArray);
  NS_ENSURE_TRUE(propsToWrite, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<sbIFileMetadataService> metadataService =
      do_GetService("@songbirdnest.com/Songbird/FileMetadataService;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIJobProgress> job;
  rv = metadataService->Write(mediaItemArray, propsToWrite,
          getter_AddRefs(job));
  NS_ENSURE_SUCCESS(rv, rv);

  // The job progress object returnned from the this should only be used
  // from the main thread, so proxy.
  nsCOMPtr<sbIJobProgress> proxiedJob;
  rv = SB_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
          NS_GET_IID(sbIJobProgress),
          job,
          NS_PROXY_SYNC,
          getter_AddRefs(proxiedJob));
  NS_ENSURE_SUCCESS(rv, rv);

  // Wait until the metadata reading completes.  Poll instead of using
  // callbacks because in practice it'll always be very quick, and this is
  // simpler.
  PRBool isRunning = PR_TRUE;
  while (isRunning) {
    // Check if the metadata job is running.
    PRUint16 status;
    rv = proxiedJob->GetStatus(&status);
    NS_ENSURE_SUCCESS(rv, rv);
    isRunning = (status == sbIJobProgress::STATUS_RUNNING);

    // If still running, sleep a bit and check again.
    if (isRunning)
      PR_Sleep(PR_MillisecondsToInterval(100));
    else if (status == sbIJobProgress::STATUS_FAILED)
      return NS_ERROR_FAILURE;
  }

  // Metadata is written, so we're all done. Yay!
  return NS_OK;
}

