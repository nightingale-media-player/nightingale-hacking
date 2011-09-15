/* vim: fileencoding=utf-8 shiftwidth=2 */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is toolkit/components/systray
 *
 * The Initial Developer of the Original Code is
 * Mook <mook.moz+cvs.mozilla.org@gmail.com>.
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Brad Peterson <b_peterson@yahoo.com>
 *   Daniel Glazman <daniel.glazman@disruptive-innovations.com>
 *   Matthew Gertner <matthew@allpeers.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsSystemTrayIconBase.h"
// headers for SetImageFromURI
#include "nsServiceManagerUtils.h"
#include "nsIInterfaceRequestorUtils.h"
#include "imgILoader.h"
#include "imgIRequest.h"
#include "nsNetError.h" // for NS_BINDING_ABORTED
// needed for event dispatching
#include "nsIDOMNode.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOMDocument.h"
#include "nsStringGlue.h"
#include "nsIDOMDocumentView.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMAbstractView.h"

// debugging
#if DEBUG
#include <cstdio>
#endif

NS_IMPL_ISUPPORTS2(nsSystemTrayIconBase,
                   imgIDecoderObserver,
                   imgIContainerObserver)

nsSystemTrayIconBase::~nsSystemTrayIconBase()
{
  if (mRequest) {
    mRequest->Cancel(NS_BINDING_ABORTED);
  }
}

nsresult nsSystemTrayIconBase::SetImageFromURI(nsIURI* aURI)
{
  nsresult rv;
  
  nsCOMPtr<imgILoader> imgloader =
    do_GetService("@mozilla.org/image/loader;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (mRequest) {
    mRequest->Cancel(NS_BINDING_ABORTED);
  }
  rv = imgloader->LoadImage(aURI, aURI, aURI, nsnull, this, nsnull,
                            nsIRequest::LOAD_BACKGROUND, nsnull, nsnull,
                            getter_AddRefs(mRequest));
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

nsresult nsSystemTrayIconBase::SetEventTarget(nsIDOMEventTarget *aEventTarget)
{
  mEventTarget = aEventTarget;
  return NS_OK;
}

void nsSystemTrayIconBase::DispatchEvent(const nsAString& aType,
                                         PRUint16 aButton, PRInt32 aDetail,
                                         PRBool aCtrlKey, PRBool aAltKey,
                                         PRBool aShiftKey, PRBool aMetaKey)
{
  nsresult rv;

  // first, create an event...
  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(mEventTarget, &rv));
  if (NS_FAILED(rv)) return;
  
  nsCOMPtr<nsIDOMDocument> doc;
  rv = node->GetOwnerDocument(getter_AddRefs(doc));
  if (NS_FAILED(rv)) return;
  
  nsCOMPtr<nsIDOMDocumentEvent> docEvent(do_QueryInterface(doc, &rv));
  if (NS_FAILED(rv)) return;
  
  nsCOMPtr<nsIDOMEvent> event;
  rv = docEvent->CreateEvent(NS_LITERAL_STRING("mouseevent"),
                             getter_AddRefs(event));
  if (NS_FAILED(rv)) return;
  
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent(do_QueryInterface(event, &rv));
  if (NS_FAILED(rv)) return;
  
  // get the view the event occurred on
  nsCOMPtr<nsIDOMDocumentView> documentView(do_QueryInterface(doc, &rv));
  if (NS_FAILED(rv)) return;
  
  nsCOMPtr<nsIDOMAbstractView> view;
  rv = documentView->GetDefaultView(getter_AddRefs(view));
  if (NS_FAILED(rv)) return;
  
  // figure out where to position the popup
  nsPoint position = GetPopupPosition();
  
  // initialize the event
  // TODO: give useful arguments here
  rv = mouseEvent->InitMouseEvent(aType,
                                  PR_FALSE, PR_TRUE, view, aDetail,
                                  position.x, position.y, position.x, position.y,
                                  aCtrlKey, aAltKey, aShiftKey, aMetaKey,
                                  aButton, nsnull);
  if (NS_FAILED(rv)) return;
  
  // and dispatch it. (yes, return value is ignored; we can't do anything)
  PRBool result;
  mEventTarget->DispatchEvent(event, &result);
}

///// imgIDecoderObserver
/* void onStartRequest (in imgIRequest aRequest); */
NS_IMETHODIMP nsSystemTrayIconBase::OnStartRequest(imgIRequest *aRequest)
{
  return NS_OK;
}

/* void onStartDecode (in imgIRequest aRequest); */
NS_IMETHODIMP nsSystemTrayIconBase::OnStartDecode(imgIRequest *aRequest)
{
  return NS_OK;
}

/* void onStartContainer (in imgIRequest aRequest, in imgIContainer aContainer); */
NS_IMETHODIMP nsSystemTrayIconBase::OnStartContainer(imgIRequest *aRequest, imgIContainer *aContainer)
{
  return NS_OK;
}

/* void onStartFrame (in imgIRequest aRequest, in unsigned long aFrame); */
NS_IMETHODIMP
nsSystemTrayService::OnStartFrame(imgIRequest *aRequest,
                                  PRUint32 aFrame)
{
  return NS_OK;
}

/* [noscript] void onDataAvailable (in imgIRequest aRequest,
  in boolean aCurrentFrame, [const] in nsIntRect aRect); */
NS_IMETHODIMP
nsSystemTrayService::OnDataAvailable(imgIRequest *aRequest,
                                     PRBool aCurrentFrame,
                                     const nsIntRect * aRect)
{
  return NS_OK;
}

/* void onStopFrame (in imgIRequest aRequest, in unsigned long aFrame); */
NS_IMETHODIMP
nsSystemTrayService::OnStopFrame(imgIRequest *aRequest,
                                 PRUint32 aFrame)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void onStopContainer (in imgIRequest aRequest, in imgIContainer aContainer); */
NS_IMETHODIMP nsSystemTrayIconBase::OnStopContainer(imgIRequest *aRequest, imgIContainer *aContainer)
{
  return NS_OK;
}

/* void onStopDecode (in imgIRequest aRequest, in nsresult status, in wstring statusArg); */
NS_IMETHODIMP nsSystemTrayIconBase::OnStopDecode(imgIRequest *aRequest, nsresult status, const PRUnichar *statusArg)
{
  return NS_OK;
}

/* void onStopRequest (in imgIRequest aRequest, in boolean aIsLastPart); */
NS_IMETHODIMP nsSystemTrayIconBase::OnStopRequest(imgIRequest *aRequest, PRBool aIsLastPart)
{
  return NS_OK;
}

///// imgIContainerObserver
/* [noscript] void frameChanged (in imgIContainer aContainer,
  in nsIntRect aDirtyRect); */
NS_IMETHODIMP
nsSystemTrayService::FrameChanged(imgIContainer *aContainer,
                                  nsIntRect * aDirtyRect)
{
  return NS_OK;
}
