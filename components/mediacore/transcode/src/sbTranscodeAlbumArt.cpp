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

#include <prio.h>

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
#include <nsISeekableStream.h>
#include <nsIBinaryInputStream.h>

#include <imgITools.h>
#include <imgIEncoder.h>
#include <nsComponentManagerUtils.h>
#include <nsTArray.h>
#include <nsThreadUtils.h>

#include <sbStandardProperties.h>
#include <sbProxiedComponentManager.h>
#include <sbTArrayStringEnumerator.h>
#include <sbMemoryUtils.h>

#include <sbIJobProgress.h>
#include <sbIFileMetadataService.h>
#include <sbIAlbumArtService.h>

#include "sbImageTools.h"

#define BUFFER_CHUNK_SIZE 1024 

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

  cImageSpec = NS_LossyConvertUTF16toASCII(imageSpec);
  nsCOMPtr<nsIURI> imageURI;
  rv = ioservice->NewURI(cImageSpec, nsnull, nsnull,
          getter_AddRefs(imageURI));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIThread> target;
  rv = NS_GetMainThread(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> proxiedURI;
  rv = do_GetProxyForObject(target,
                            NS_GET_IID(nsIURI),
                            imageURI,
                            NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                            getter_AddRefs(proxiedURI));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isResource;
  rv = proxiedURI->SchemeIs("resource", &isResource);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isResource) {
    nsCOMPtr<nsIProtocolHandler> resHandler;
    rv = ioservice->GetProtocolHandler("resource", getter_AddRefs(resHandler));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIResProtocolHandler> proxiedResourceProtocolHandler;
    rv = do_GetProxyForObject(target,
                              NS_GET_IID(nsIResProtocolHandler),
                              resHandler,
                              NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                              getter_AddRefs(proxiedResourceProtocolHandler));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = proxiedResourceProtocolHandler->ResolveURI(imageURI, cImageSpec);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIProtocolHandler> fileHandler;
  rv = ioservice->GetProtocolHandler("file", getter_AddRefs(fileHandler));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFileProtocolHandler> proxiedFileProtocolHandler;
  rv = do_GetProxyForObject(target,
                            NS_GET_IID(nsIFileProtocolHandler),
                            fileHandler,
                            NS_PROXY_SYNC | NS_PROXY_ALWAYS,
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
  mInputStream = do_CreateInstance(
      "@mozilla.org/network/file-input-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mInputStream->Init(imageFile, PR_RDONLY, 0, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIBufferedInputStream> bufferedInputStream =
      do_CreateInstance("@mozilla.org/network/buffered-input-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = bufferedInputStream->Init(mInputStream, BUFFER_CHUNK_SIZE);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = sbImageTools::DecodeImageData(bufferedInputStream,
                                     mImageMimeType,
                                     getter_AddRefs(mImgContainer));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mImgContainer->GetHeight(&mImageHeight);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mImgContainer->GetWidth(&mImageWidth);
  NS_ENSURE_SUCCESS(rv, rv);

  mHasAlbumArt = PR_TRUE;

  // Reset the stream so we can reuse it later if required
  nsCOMPtr<nsISeekableStream> seekableStream =
      do_QueryInterface(mInputStream, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = seekableStream->Seek(nsISeekableStream::NS_SEEK_SET, 0);
  NS_ENSURE_SUCCESS(rv, rv);

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

static nsresult HaveEncoderForFormat(nsCString mimeType, PRBool *haveEncoder)
{
  nsresult rv;
  nsCString encoderCID = NS_LITERAL_CSTRING(
          "@mozilla.org/image/encoder;2?type=");
  encoderCID.Append(mimeType);

  nsCOMPtr<imgIEncoder> encoder = do_CreateInstance(encoderCID.get(), &rv);
  if (NS_SUCCEEDED(rv)) {
    *haveEncoder = PR_TRUE;
  }
  else {
    *haveEncoder = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeAlbumArt::GetTargetFormat(
        nsACString & aMimeType, PRInt32 *aWidth, PRInt32 *aHeight)
{
  NS_ENSURE_ARG_POINTER (aWidth);
  NS_ENSURE_ARG_POINTER (aHeight);
  NS_ENSURE_STATE (mImageFormats);

  nsresult rv;
  /* Figure out the target format as follows:

     1. If the input width and height are both supported for some format, choose
        the first such format.
     2. Otherwise, choose the first format that we have an encoder for. 
        Then, select the smallest explicitly-listed size larger than the
        input image (unless there are none, in which case use the largest
        explicit size).

     Note: if the input aspect ratio is not the same as the output aspect ratio,
           we do not correctly maintain the image aspect ratio by putting black
           bars/etc. around it. TODO!
   */

  // 1: exact size match.
  PRUint32 numFormats = 0;
  rv = mImageFormats->GetLength(&numFormats);
  NS_ENSURE_SUCCESS (rv, rv);
  for (PRUint32 i = 0; i < numFormats; i++) {
    nsCOMPtr<sbIImageFormatType> format;
    rv = mImageFormats->QueryElementAt(i, NS_GET_IID(sbIImageFormatType),
                                       getter_AddRefs(format));
    NS_ENSURE_SUCCESS (rv, rv);

    nsCString formatMimeType;
    rv = format->GetImageFormat(formatMimeType);
    NS_ENSURE_SUCCESS (rv, rv);

    PRBool haveEncoder;
    rv = HaveEncoderForFormat(formatMimeType, &haveEncoder);
    NS_ENSURE_SUCCESS(rv, rv);

    // Skip this if we don't have an encoder for it.
    if (!haveEncoder) {
      continue;
    }

    PRBool valid;
    rv = IsValidSizeForFormat(format, &valid);
    NS_ENSURE_SUCCESS (rv, rv);
      
    if (valid) {
      aMimeType = formatMimeType;
      *aWidth = mImageWidth;
      *aHeight = mImageHeight;
      return NS_OK;
    }
  }

  // 2: resize required.
  nsCOMPtr<sbIImageFormatType> chosenformat;
  for (PRUint32 i = 0; i < numFormats; i++) {
    nsCOMPtr<sbIImageFormatType> format;
    rv = mImageFormats->QueryElementAt(i, NS_GET_IID(sbIImageFormatType),
            getter_AddRefs(format));
    NS_ENSURE_SUCCESS (rv, rv);

    nsCString formatMimeType;
    rv = format->GetImageFormat(formatMimeType);
    NS_ENSURE_SUCCESS (rv, rv);

    PRBool haveEncoder;
    rv = HaveEncoderForFormat(formatMimeType, &haveEncoder);
    NS_ENSURE_SUCCESS(rv, rv);

    if (haveEncoder) {
      chosenformat = format;
      break;
    }
  }

  if (!chosenformat)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIArray> explicitSizes;
  rv = chosenformat->GetSupportedExplicitSizes(getter_AddRefs(explicitSizes));
  NS_ENSURE_SUCCESS (rv, rv);

  rv = chosenformat->GetImageFormat(aMimeType);
  NS_ENSURE_SUCCESS (rv, rv);

  PRInt32 bestWidth = 0;
  PRInt32 bestHeight = 0;
  PRInt32 bestSmallerWidth = 0;
  PRInt32 bestSmallerHeight = 0;

  PRUint32 numSizes = 0;
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

    if (width >= mImageWidth && height >= mImageHeight &&
        (bestWidth == 0 || bestHeight == 0 || 
         width < bestWidth || height < bestHeight))
    {
      bestWidth = width;
      bestHeight = height;
    }
    else if (width < mImageWidth && height < mImageHeight &&
             (width > bestSmallerWidth || height > bestSmallerHeight))
    {
      bestSmallerWidth = width;
      bestSmallerHeight = height;
    }
  }

  if (bestWidth && bestHeight) {
    *aWidth = bestWidth;
    *aHeight = bestHeight;
    return NS_OK;
  }
  else if (bestSmallerWidth && bestSmallerHeight) {
    *aWidth = bestSmallerWidth;
    *aHeight = bestSmallerHeight;
    return NS_OK;
  }
  else {
    return NS_ERROR_FAILURE;
  }
}

NS_IMETHODIMP
sbTranscodeAlbumArt::GetTranscodedArt(nsIInputStream **aImageStream)
{
  NS_ENSURE_ARG_POINTER(aImageStream);

  PRBool needsConversion = PR_FALSE;
  nsresult rv = GetNeedsAlbumArtConversion(&needsConversion);
  NS_ENSURE_SUCCESS(rv, rv);
  if (needsConversion) {
    nsresult rv;
    nsCString mimeType;
    PRInt32 width, height;

    rv = GetTargetFormat(mimeType, &width, &height);
    NS_ENSURE_SUCCESS (rv, rv);

    nsCOMPtr<imgITools> imgTools = do_ProxiedGetService(
            "@mozilla.org/image/tools;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = imgTools->EncodeScaledImage(mImgContainer, mimeType, width, height,
                                     aImageStream);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }
  else {
    NS_IF_ADDREF(*aImageStream = mInputStream);
    return NS_OK;
  }
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

  nsCOMPtr<imgITools> imgTools = do_ProxiedGetService(
          "@mozilla.org/image/tools;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInputStream> imageStream;
  rv = imgTools->EncodeScaledImage(mImgContainer, mimeType, width, height,
          getter_AddRefs(imageStream));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIBinaryInputStream> binaryStream =
             do_CreateInstance("@mozilla.org/binaryinputstream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = binaryStream->SetInputStream(imageStream);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 imageDataLen;
  rv = imageStream->Available(&imageDataLen);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint8 *imageData;
  rv = binaryStream->ReadByteArray(imageDataLen, &imageData);
  NS_ENSURE_SUCCESS(rv, rv);

  sbAutoNSMemPtr imageDataDestroy(imageData);

  nsCOMPtr<sbIAlbumArtService> albumArtService = do_ProxiedGetService(
              SB_ALBUMARTSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> cacheURI;
  rv = albumArtService->CacheImage(mimeType,
                                   imageData,
                                   imageDataLen,
                                   getter_AddRefs(cacheURI));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString imageURISpec;
  rv = cacheURI->GetSpec(imageURISpec);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_PRIMARYIMAGEURL),
                          NS_ConvertUTF8toUTF16(imageURISpec));
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

  nsCOMPtr<sbIFileMetadataService> metadataService = do_ProxiedGetService(
	 "@songbirdnest.com/Songbird/FileMetadataService;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIJobProgress> job;
  rv = metadataService->Write(mediaItemArray, propsToWrite,
          getter_AddRefs(job));
  NS_ENSURE_SUCCESS(rv, rv);

  // The job progress object returnned from the this should only be used
  // from the main thread, so proxy that too.
  nsCOMPtr<nsIThread> target;
  rv = NS_GetMainThread(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIJobProgress> proxiedJob;
  rv = do_GetProxyForObject(target,
                            NS_GET_IID(sbIJobProgress),
                            job,
                            NS_PROXY_SYNC | NS_PROXY_ALWAYS,
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

