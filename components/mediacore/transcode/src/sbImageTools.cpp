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

#include "sbImageTools.h"

#include <nsIInputStream.h>

#include <nsCOMPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsWeakReference.h>

#include <imgIContainer.h>
#include <imgIDecoder.h>
#include <imgIDecoderObserver.h>
#include <imgILoad.h>

class HelperLoader : public imgILoad,
                     public imgIDecoderObserver,
                     public nsSupportsWeakReference
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_IMGILOAD
    NS_DECL_IMGIDECODEROBSERVER
    NS_DECL_IMGICONTAINEROBSERVER
    HelperLoader(void);

  private:
    nsCOMPtr<imgIContainer> mContainer;
};

NS_IMPL_ISUPPORTS4 (HelperLoader, imgILoad, imgIDecoderObserver, imgIContainerObserver, nsISupportsWeakReference)

HelperLoader::HelperLoader (void)
{
}

/* Implement imgILoad::image getter */
NS_IMETHODIMP
HelperLoader::GetImage(imgIContainer **aImage)
{
  NS_ENSURE_ARG_POINTER(aImage);

  *aImage = mContainer;
  NS_IF_ADDREF (*aImage);
  return NS_OK;
}

/* Implement imgILoad::image setter */
NS_IMETHODIMP
HelperLoader::SetImage(imgIContainer *aImage)
{
  mContainer = aImage;
  return NS_OK;
}

/* Implement imgILoad::isMultiPartChannel getter */
NS_IMETHODIMP
HelperLoader::GetIsMultiPartChannel(PRBool *aIsMultiPartChannel)
{
  NS_ENSURE_ARG_POINTER(aIsMultiPartChannel);

  *aIsMultiPartChannel = PR_FALSE;
  return NS_OK;
}

/* Implement imgIDecoderObserver::onStartRequest() */
NS_IMETHODIMP
HelperLoader::OnStartRequest(imgIRequest *aRequest)
{
  return NS_OK;
}

/* Implement imgIDecoderObserver::onStartDecode() */
NS_IMETHODIMP
HelperLoader::OnStartDecode(imgIRequest *aRequest)
{
  return NS_OK;
}

/* Implement imgIDecoderObserver::onStartContainer() */
NS_IMETHODIMP
HelperLoader::OnStartContainer(imgIRequest *aRequest, imgIContainer
*aContainer)
{
  return NS_OK;
}

/* Implement imgIDecoderObserver::onStartFrame() */
NS_IMETHODIMP
HelperLoader::OnStartFrame(imgIRequest *aRequest, gfxIImageFrame *aFrame)
{
  return NS_OK;
}

/* Implement imgIDecoderObserver::onDataAvailable() */
NS_IMETHODIMP
HelperLoader::OnDataAvailable(imgIRequest *aRequest, gfxIImageFrame
*aFrame, const nsIntRect * aRect)
{
  return NS_OK;
}

/* Implement imgIDecoderObserver::onStopFrame() */
NS_IMETHODIMP
HelperLoader::OnStopFrame(imgIRequest *aRequest, gfxIImageFrame *aFrame)
{
  return NS_OK;
}

/* Implement imgIDecoderObserver::onStopContainer() */
NS_IMETHODIMP
HelperLoader::OnStopContainer(imgIRequest *aRequest, imgIContainer
*aContainer)
{
  return NS_OK;
}

/* Implement imgIDecoderObserver::onStopDecode() */
NS_IMETHODIMP
HelperLoader::OnStopDecode(imgIRequest *aRequest, nsresult status, const
PRUnichar *statusArg)
{
  return NS_OK;
}

/* Implement imgIDecoderObserver::onStopRequest() */
NS_IMETHODIMP
HelperLoader::OnStopRequest(imgIRequest *aRequest, PRBool aIsLastPart)
{
  return NS_OK;
}

/* implement imgIContainerObserver::frameChanged() */
NS_IMETHODIMP
HelperLoader::FrameChanged(imgIContainer *aContainer,
                           gfxIImageFrame *aFrame, nsIntRect * aDirtyRect)
{
  return NS_OK;
}

/* Just like imgITools::DecodeImageData(), but with workarounds for
   bugs in mozilla components */
/* static */ nsresult
sbImageTools::DecodeImageData(nsIInputStream* aInStr,
                              const nsACString& aMimeType,
                              imgIContainer **aContainer)
{
  NS_ENSURE_ARG_POINTER(aContainer);
  NS_ENSURE_ARG_POINTER(aInStr);

  nsresult rv;
  // Get an image decoder for our media type
  nsCString decoderCID = NS_LITERAL_CSTRING("@mozilla.org/image/decoder;2?type=");
  decoderCID.Append(aMimeType);

  nsCOMPtr<imgIDecoder> decoder = do_CreateInstance(decoderCID.get());
  if (!decoder)
    return NS_ERROR_NOT_AVAILABLE;

  // Init the decoder, we use a small utility class here.
  nsCOMPtr<imgILoad> loader = new HelperLoader();
  if (!loader)
    return NS_ERROR_OUT_OF_MEMORY;

  // If caller provided an existing container, use it.
  if (*aContainer)
    loader->SetImage(*aContainer);

  rv = decoder->Init(loader);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length;
  rv = aInStr->Available(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 written;
  NS_ENSURE_SUCCESS(rv, rv);
  rv = decoder->WriteFrom(aInStr, length, &written);
  NS_ENSURE_SUCCESS(rv, rv);
  if (written != length)
    NS_WARNING("decoder didn't eat all of its vegetables");
  rv = decoder->Flush();
  /* Some decoders don't implement Flush, but that's ok for those decoders... */
  if (rv != NS_ERROR_NOT_IMPLEMENTED) {
    NS_ENSURE_SUCCESS(rv, rv);
  }
  rv = decoder->Close();
  NS_ENSURE_SUCCESS(rv, rv);

  // If caller didn't provide an existing container, return the new one.
  if (!*aContainer)
    loader->GetImage(aContainer);

  return NS_OK;

}

