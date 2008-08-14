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

#include <gfxIImageFrame.h>
#include <imgIContainer.h>
#include <imgITools.h>
#include <nsIBinaryInputStream.h>
#include <nsIClipboard.h>
#include <nsIImage.h>
#include <nsIInputStream.h>
#include <nsIInterfaceRequestor.h>
#include <nsIInterfaceRequestorUtils.h>
#include <nsIStringStream.h>
#include <nsISupportsPrimitives.h>
#include <nsITransferable.h>
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
  
  nsCOMPtr<nsIClipboard> nsClipboard = do_GetService("@mozilla.org/widget/clipboard;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsITransferable> xferable = do_CreateInstance("@mozilla.org/widget/transferable;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  const int imageTypesLen = 3;
  const char *imageTypes[3] = { "image/png",
                                "image/jpg",
                                "image/jpeg" };

  for (int iIndex = 0; iIndex < imageTypesLen; iIndex++) {
    rv = xferable->AddDataFlavor(imageTypes[iIndex]);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  PRBool clipboardHasData = PR_FALSE;
  rv = nsClipboard->HasDataMatchingFlavors(imageTypes,
                                           imageTypesLen,
                                           nsIClipboard::kGlobalClipboard,
                                           &clipboardHasData);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (clipboardHasData) {
    rv = nsClipboard->GetData(xferable, nsIClipboard::kGlobalClipboard);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<nsISupports> clipboardData;
    char* clipboardDataFlavor = 0;
    PRUint32 clipboardDataLen = 0;

    rv = xferable->GetAnyTransferData(&clipboardDataFlavor,
                                      getter_AddRefs(clipboardData),
                                      &clipboardDataLen);
    NS_ENSURE_SUCCESS(rv, rv);
    
    if(strcmp(clipboardDataFlavor, "image/jpg")) {
      aMimeType.AssignLiteral(clipboardDataFlavor);
    }
    else {
      aMimeType.AssignLiteral("image/jpeg");
    }
    
    
    nsCOMPtr<nsIInputStream> clipboardDataStream = do_QueryInterface(clipboardData,
                                                                     &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    PRUint32 dataSize;
    rv = clipboardDataStream->Available(&dataSize);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<nsIBinaryInputStream> imageDataStream = do_CreateInstance(kBinaryInputStream,
                                                                       &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = imageDataStream->SetInputStream(clipboardDataStream);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = imageDataStream->ReadByteArray(dataSize, aData);
    NS_ENSURE_SUCCESS(rv, rv);
    *aDataLen = dataSize;
  } else {
    *aDataLen = 0;
    *aData = nsnull;
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
  nsCOMPtr<imgIContainer> container;
  rv = imgtool->DecodeImageData(stream, aMimeType, getter_AddRefs(container));
  NS_ENSURE_SUCCESS(rv, rv);

  // Grab first frame of the image
  nsCOMPtr<gfxIImageFrame> imgFrame;
  rv = container->GetCurrentFrame(getter_AddRefs(imgFrame));
  NS_ENSURE_SUCCESS(rv, rv);

  // Change to nsIImage so we can pass to the clipboard
  nsCOMPtr<nsIImage> image(do_GetInterface(imgFrame, &rv));
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
