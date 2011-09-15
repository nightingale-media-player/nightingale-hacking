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

#include "sbClipboardHelper.h"

#include <imgIContainer.h>
#include <imgITools.h>
#include <nsIBinaryInputStream.h>
#include <nsIClipboard.h>
#include <nsIInputStream.h>
#include <nsIInterfaceRequestor.h>
#include <nsIInterfaceRequestorUtils.h>
#include <nsIMIMEService.h>
#include <nsIStringStream.h>
#include <nsISupportsPrimitives.h>
#include <nsITransferable.h>
#include <nsIURL.h>
#include <nsNetUtil.h>
#include <nsStringAPI.h>

#include "nsWidgetsCID.h"

#define kBinaryInputStream "@mozilla.org/binaryinputstream;1"

NS_IMPL_ISUPPORTS1(sbClipboardHelper, sbIClipboardHelper)

sbClipboardHelper::sbClipboardHelper()
{
  /* member initializers and constructor code */
}

sbClipboardHelper::~sbClipboardHelper()
{
  /* destructor code */
}

NS_IMETHODIMP
sbClipboardHelper::CopyImageFromClipboard(nsAString  &aMimeType,
                                          PRUint32   *aDataLen,
                                          PRUint8    **aData)
{
  nsresult rv;
  
  *aDataLen = 0;
  
  nsCOMPtr<nsIClipboard> nsClipboard =
                         do_GetService("@mozilla.org/widget/clipboard;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsITransferable> xferable =
                  do_CreateInstance("@mozilla.org/widget/transferable;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  /*
   * Load up the flavors we support
   */
  const char *flavorTypes[] = { kPNGImageMime,
                                kJPEGImageMime,
                                "image/jpeg",
                                kGIFImageMime,
                                kFileMime,
                                kUnicodeMime };
  const int flavorTypesLen = NS_ARRAY_LENGTH(flavorTypes);
  for (int iIndex = 0; iIndex < flavorTypesLen; iIndex++) {
    rv = xferable->AddDataFlavor(flavorTypes[iIndex]);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  // Check the clipboard if it has one of those flavors
  PRBool clipboardHasData = PR_FALSE;
  rv = nsClipboard->HasDataMatchingFlavors(flavorTypes,
                                           flavorTypesLen,
                                           nsIClipboard::kGlobalClipboard,
                                           &clipboardHasData);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (clipboardHasData) {
    // Grab the data as a nsITransferable
    rv = nsClipboard->GetData(xferable, nsIClipboard::kGlobalClipboard);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<nsISupports> clipboardData;
    char *clipboardDataFlavor = 0;
    PRUint32 clipboardDataLen = 0;

    rv = xferable->GetAnyTransferData(&clipboardDataFlavor,
                                      getter_AddRefs(clipboardData),
                                      &clipboardDataLen);
    NS_ENSURE_SUCCESS(rv, rv);

    // Once we get a stream we can grab the data
    nsCOMPtr<nsIInputStream> dataStream;
    
    // First check if this is a file/url flavor
    if (!strcmp(clipboardDataFlavor, kUnicodeMime) ||
        !strcmp(clipboardDataFlavor, kFileMime)) {

      // This is a file we are copying from
      nsCOMPtr<nsILocalFile> imageFile;
      if (!strcmp(clipboardDataFlavor, kUnicodeMime)) {
        nsAutoString url;
        nsCOMPtr<nsISupportsString> stringData(do_QueryInterface(clipboardData));
        if (stringData) {
          stringData->GetData(url);
        }
        
        if (url.IsEmpty()) {
          // Nothing to work with so abort
          return NS_OK;
        }
        
        imageFile = do_CreateInstance("@mozilla.org/file/local;1", &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = imageFile->InitWithPath(url);
        if (NS_FAILED(rv)) {
          return NS_OK;
        }
      } else {
        // it already is a file :)
        imageFile = do_QueryInterface(clipboardData);
        if (!imageFile) {
          return NS_OK;
        }
      }
      
      // Get the mime type from the file
      nsCOMPtr<nsIMIMEService> mimeService =
                                do_CreateInstance("@mozilla.org/mime;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      
      nsCAutoString mimeType;
      rv = mimeService->GetTypeFromFile(imageFile, mimeType);
      NS_ENSURE_SUCCESS(rv, rv);
      
      aMimeType.Assign(NS_ConvertUTF8toUTF16(mimeType));

      // Now set up an input stream for the file
      nsCOMPtr<nsIFileInputStream> fileStream =
            do_CreateInstance("@mozilla.org/network/file-input-stream;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      
      rv = fileStream->Init(imageFile, 0x01, 0600, 0);
      NS_ENSURE_SUCCESS(rv, rv);
      
      dataStream = do_QueryInterface(fileStream, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      // Assume it is a image then
      // Here we have to force the image/jpg mime type as image/jpeg because
      // the windows clipboard does not like image/jpg. The other systems work
      // either way.
      if(strcmp(clipboardDataFlavor, kJPEGImageMime)) {
        aMimeType.AssignLiteral(clipboardDataFlavor);
      }
      else {
        aMimeType.AssignLiteral("image/jpeg");
      }
      
      dataStream = do_QueryInterface(clipboardData, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // Now read in the data from the stream
    PRUint32 dataSize;
    rv = dataStream->Available(&dataSize);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<nsIBinaryInputStream> imageDataStream =
                                    do_CreateInstance(kBinaryInputStream, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = imageDataStream->SetInputStream(dataStream);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = imageDataStream->ReadByteArray(dataSize, aData);
    NS_ENSURE_SUCCESS(rv, rv);
    *aDataLen = dataSize;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbClipboardHelper::PasteImageToClipboard(const nsACString &aMimeType,
                                         const PRUint8    *aData,
                                         PRUint32         aDataLen)
{
  nsresult rv;
  
  nsCOMPtr<imgITools> imgtool = do_CreateInstance("@mozilla.org/image/tools;1",
                                                  &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Convert the image data to a stream so we can use the imagITools
  nsCOMPtr<nsIStringInputStream> stream = do_CreateInstance("@mozilla.org/io/string-input-stream;1",
                                                             &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  stream->ShareData(reinterpret_cast<const char*>(aData), aDataLen);

  // decode image
  nsCOMPtr<imgIContainer> image;
  rv = imgtool->DecodeImageData(stream, aMimeType, getter_AddRefs(image));
  NS_ENSURE_SUCCESS(rv, rv);

  // Now create a Transferable so we can copy to the clipboard
  nsCOMPtr<nsITransferable> xferable = do_CreateInstance("@mozilla.org/widget/transferable;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupportsInterfacePointer>
    imgPtr(do_CreateInstance(NS_SUPPORTS_INTERFACE_POINTER_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = imgPtr->SetData(image);
  NS_ENSURE_SUCCESS(rv, rv);

  // copy the image data onto the transferable
  rv = xferable->SetTransferData(kNativeImageMime,
                                 imgPtr,
                                 sizeof(nsISupportsInterfacePointer*));
  NS_ENSURE_SUCCESS(rv, rv);

  // get clipboard
  nsCOMPtr<nsIClipboard> clipboard(do_GetService("@mozilla.org/widget/clipboard;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  // check whether the system supports the selection clipboard or not.
  PRBool selectionSupported;
  rv = clipboard->SupportsSelectionClipboard(&selectionSupported);
  NS_ENSURE_SUCCESS(rv, rv);

  // put the transferable on the clipboard
  if (selectionSupported) {
    rv = clipboard->SetData(xferable, nsnull, nsIClipboard::kSelectionClipboard);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return clipboard->SetData(xferable, nsnull, nsIClipboard::kGlobalClipboard);
}

