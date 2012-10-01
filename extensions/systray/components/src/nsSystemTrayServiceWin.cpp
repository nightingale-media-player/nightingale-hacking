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
 * The Original Code is MinimizeToTray.
 *
 * The Initial Developer of the Original Code are
 * Mark Yen and Brad Peterson.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Mark Yen <mook.moz+cvs.mozilla.org@gmail.com>, Original author
 *   Brad Peterson <b_peterson@yahoo.com>, Original author
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

#include "nsStringGlue.h"
#include "nsSystemTrayServiceWin.h"
#include "gfxImageSurface.h"
#include "imgIContainer.h"
#include "imgIDecoder.h"
#include "nsAutoPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsMemory.h"
#include "nsIBaseWindow.h"
#include "nsIBufferedStreams.h"
#include "nsIChannel.h"
#include "nsIContent.h"
#include "nsIDocShell.h"
#include "nsIDocument.h"
#include "nsIDOMAbstractView.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOMDocumentView.h"
#include "nsIDOMDOMImplementation.h"
#include "nsIDOMDocumentType.h"
#include "nsIDOMElement.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMWindow.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIIOService.h"
#include "nsIPresShell.h"
#include "nsIWidget.h"
#include "nsIWindowUtil.h"
#include "nsIURI.h"
#include "nsIXULDocument.h"
#include "nsIXULWindow.h"
#include "nsServiceManagerUtils.h"

#include "nsIImageToBitmap.h"

#include <tchar.h>

#pragma comment(lib, "shell32.lib")

const TCHAR* nsSystemTrayService::S_PROPINST =
  TEXT("_SYSTEMTRAYSERVICE_INST");
const TCHAR* nsSystemTrayService::S_PROPPROC =
  TEXT("_SYSTEMTRAYSERVICE_PROC");

ATOM nsSystemTrayService::s_wndClass = NULL;

#define MK_ERROR_OFFSET       (0xE0000000 + (__LINE__ * 0x10000))

#define MK_ENSURE_NATIVE(res)                              \
  PR_BEGIN_MACRO                                           \
    NS_ENSURE_TRUE(0 != res || 0 == ::GetLastError(),      \
      ::GetLastError() + MK_ERROR_OFFSET);                 \
  PR_END_MACRO

// this can be WM_USER + anything
#define WM_TRAYICON (WM_USER + 0x17b6)

NS_IMPL_ISUPPORTS4(nsSystemTrayService, nsISystemTrayService,
  imgILoad, imgIContainerObserver, imgIDecoderObserver)

nsSystemTrayService::nsSystemTrayService()
{
  /* member initializers and constructor code */
  mIconDataMap.Init();
}

nsSystemTrayService::~nsSystemTrayService()
{
  BOOL windowClassUnregistered = ::UnregisterClass(
    (LPCTSTR)nsSystemTrayService::s_wndClass,
    ::GetModuleHandle(NULL));
  if (windowClassUnregistered)
    nsSystemTrayService::s_wndClass = NULL;
}

NS_IMETHODIMP
nsSystemTrayService::ShowIcon(const nsAString& aIconId,
                              nsIURI* imageURI,
                              nsIDOMWindow* aDOMWindow)
{
  nsresult rv;

  NOTIFYICONDATAW notifyIconData;
  if (mIconDataMap.Get(aIconId, &notifyIconData))
    // Already have an entry so just change the image
  {
    HICON hicon;
    rv = GetIconForURI(imageURI, hicon);
    NS_ENSURE_SUCCESS(rv, rv);

    notifyIconData.hIcon = hicon;
    MK_ENSURE_NATIVE(::Shell_NotifyIcon(NIM_MODIFY,
      &notifyIconData));

    return NS_OK;
  }

  // first make sure that we can find the element associated with the
  // <trayicon> element
  nsCOMPtr<nsIDOMDocument> document;
  rv = aDOMWindow->GetDocument(getter_AddRefs(document));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMElement> iconElement;
  rv = document->GetElementById(aIconId, getter_AddRefs(iconElement));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIWindowUtil>
    windowUtil(do_CreateInstance("@mozilla.org/window-util;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = windowUtil->Init(aDOMWindow);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDocShell> docShell;
  rv = windowUtil->GetDocShell(getter_AddRefs(docShell));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIBaseWindow> baseWindow(do_QueryInterface(docShell, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  nativeWindow theNativeWindow;
  rv = baseWindow->GetParentNativeWindow( &theNativeWindow );
  NS_ENSURE_SUCCESS(rv, rv);

  HWND hWnd = reinterpret_cast<HWND>(theNativeWindow);
  NS_ENSURE_TRUE(hWnd, NS_ERROR_UNEXPECTED);

  // yay, we succeeded so we can add the icon
  rv = AddTrayIcon(hWnd, imageURI, aIconId, iconElement);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsSystemTrayService::HideIcon(const nsAString& iconId)
{
  NOTIFYICONDATAW notifyIconData;
  if (!mIconDataMap.Get(iconId, &notifyIconData))
    // can't find the icon data
    return NS_ERROR_NOT_AVAILABLE;

  if (notifyIconData.hWnd)
    ::DestroyWindow(notifyIconData.hWnd);

  Shell_NotifyIcon(NIM_DELETE, &notifyIconData);

  mIconDataMap.Remove(iconId);

  return NS_OK;
}

nsresult
nsSystemTrayService::AddTrayIcon(HWND hwnd, nsIURI* iconURI,
                                 const nsAString& iconId,
                                 nsIDOMElement* targetElement)
{
  nsresult rv;

  NOTIFYICONDATAW notifyIconData;
  memset(&notifyIconData, 0, sizeof(NOTIFYICONDATAW));
  notifyIconData.cbSize = sizeof(NOTIFYICONDATAW);
  notifyIconData.uCallbackMessage = WM_TRAYICON;
  notifyIconData.uID = 1;
  notifyIconData.uFlags = NIF_MESSAGE | NIF_ICON;

  HICON hicon = NULL;
  if (iconURI)
  {
    rv = GetIconForURI(iconURI, hicon);
    // ignore errors, we have fallback
    //NS_ENSURE_SUCCESS(rv, rv);
  }

  if (NS_FAILED(rv) || !hicon)
  {
    // just use the window icon
    rv = GetIconForWnd(hwnd, hicon);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  notifyIconData.hIcon = hicon;
  
  HWND listenerWindow;
  rv = CreateListenerWindow(&listenerWindow, targetElement);
  NS_ENSURE_SUCCESS(rv, rv);

  //::SetTimer(listenerWindow, 1, 100, NULL);

// add the icon
  notifyIconData.hWnd = listenerWindow;
  MK_ENSURE_NATIVE(Shell_NotifyIcon(NIM_ADD, &notifyIconData));
  
  mIconDataMap.Put(iconId, notifyIconData);
  
  return NS_OK;
}


NS_IMETHODIMP
nsSystemTrayService::SetTitle(const nsAString& iconId, const nsAString& title)
{
  NOTIFYICONDATAW notifyIconData;
  if (!mIconDataMap.Get(iconId, &notifyIconData))
    return NS_ERROR_NOT_AVAILABLE;

  notifyIconData.uFlags = NIF_TIP;
  PRUint32 length = title.Length();
  if (length > 64)
    length = 64;

  wcsncpy(notifyIconData.szTip, PromiseFlatString(title).get(), length);

  MK_ENSURE_NATIVE(Shell_NotifyIcon(NIM_MODIFY, &notifyIconData));

  return NS_OK;
}

nsresult
nsSystemTrayService::GetIconForURI(nsIURI* iconURI, HICON& result)
{
  nsresult rv;

  // get a channel to read the data from
  nsCOMPtr<nsIIOService>
    ioService(do_GetService("@mozilla.org/network/io-service;1", &rv));
  nsCOMPtr<nsIChannel> channel;
  rv = ioService->NewChannelFromURI(iconURI, getter_AddRefs(channel));
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef DEBUG
  {
    nsCString spec;
    rv = iconURI->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);
    printf("icon URI: %s\n", spec.get());
  }
#endif

  nsCOMPtr<nsIInputStream> inputStream;
  rv = channel->Open(getter_AddRefs(inputStream));
  NS_ENSURE_SUCCESS(rv, rv);

  // get the source MIME type
  nsCAutoString sourceMimeType;
  rv = channel->GetContentType(sourceMimeType);
  NS_ENSURE_SUCCESS(rv, rv);

  // get the decoder
  nsCAutoString decoderContractId =
    NS_LITERAL_CSTRING("@mozilla.org/image/decoder;2?type=");
  decoderContractId.Append(sourceMimeType);
  nsCOMPtr<imgIDecoder>
    decoder(do_CreateInstance(decoderContractId.get(), &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  // decode into our image container
  rv = decoder->Init(this);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 available;
  rv = inputStream->Available(&available);
  NS_ENSURE_SUCCESS(rv, rv);

  // need to wrap the stream so that the decoder can ReadSegments
  nsCOMPtr<nsIBufferedInputStream> bufferedStream(
    do_CreateInstance("@mozilla.org/network/buffered-input-stream;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = bufferedStream->Init(inputStream, 4096);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 written;
  rv = decoder->WriteFrom(bufferedStream, available, &written);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = decoder->Close();
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<gfxImageSurface> imageFrame;
  rv = mImage->CopyCurrentFrame(getter_AddRefs(imageFrame));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!imageFrame)
    return NS_ERROR_NOT_AVAILABLE;

  nsCOMPtr<nsIImageToBitmap> imageToBitmap(
    do_GetService("@mozilla.org/widget/image-to-win32-hbitmap;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  return imageToBitmap->ConvertImageToIcon(imageFrame, result);
}

nsresult
nsSystemTrayService::GetIconForWnd(HWND hwnd, HICON& result)
{
  result = (HICON)::SendMessage(hwnd, WM_GETICON, ICON_SMALL, NULL);
  if (!result) {
    // can't find icon; try GetClassLong
    result = (HICON)::GetClassLongPtr(hwnd, GCLP_HICONSM);
  }
  if (!result) {
    // still no icon - use generic windows icon
    result = ::LoadIcon(NULL, IDI_WINLOGO);
  }
  if (!result)
    return NS_ERROR_FAILURE;
  return NS_OK;
}

nsresult
nsSystemTrayService::CreateListenerWindow(HWND* listenerWindow,
                                          nsIDOMElement* targetElement)
{
  ::SetLastError(0);
  HINSTANCE hInst = ::GetModuleHandle(NULL);
  MK_ENSURE_NATIVE(hInst);

  if (!nsSystemTrayService::s_wndClass) {
    WNDCLASS wndClassDef;
    wndClassDef.style          = CS_NOCLOSE | CS_GLOBALCLASS;
    wndClassDef.lpfnWndProc    = nsSystemTrayService::WindowProc;
    wndClassDef.cbClsExtra     = 0;
    wndClassDef.cbWndExtra     = 0;
    wndClassDef.hInstance      = hInst;
    wndClassDef.hIcon          = NULL;
    wndClassDef.hCursor        = NULL;
    wndClassDef.hbrBackground  = NULL;
    wndClassDef.lpszMenuName   = NULL;
    wndClassDef.lpszClassName  = TEXT("MinimizeToTray:MessageWindowClass");

    nsSystemTrayService::s_wndClass = ::RegisterClass(&wndClassDef);
    MK_ENSURE_NATIVE(nsSystemTrayService::s_wndClass);
  }
  
  *listenerWindow =
    ::CreateWindow(
    (LPCTSTR)nsSystemTrayService::s_wndClass,                //class
    TEXT("MinimizeToTray:MessageWindow"), //caption
    WS_MINIMIZE,                          //style
    CW_USEDEFAULT ,                       //x
    CW_USEDEFAULT ,                       //y
    CW_USEDEFAULT,                        //width
    CW_USEDEFAULT,                        //height
    ::GetDesktopWindow(),                 //parent
    NULL,                                 //menu
    hInst,                                //hInst
    NULL);                                //param
  
  if (!*listenerWindow) {
    if (::UnregisterClass((LPCTSTR)nsSystemTrayService::s_wndClass, hInst))
      nsSystemTrayService::s_wndClass = NULL;
    MK_ENSURE_NATIVE(*listenerWindow);
  }
  
  MK_ENSURE_NATIVE(::SetProp(
    *listenerWindow,
    S_PROPINST,
    (HANDLE) targetElement)
  );
  
  return NS_OK;
}

// a little helper class to automatically manage our reentrancy counter
// if we reenter WindowProc very bad things happen
class AutoReentryBlocker
{
public:
  AutoReentryBlocker(PRUint32* counter) { mCounter = counter; (*mCounter)++; }
  ~AutoReentryBlocker() { (*mCounter)--; }

protected:
  PRUint32* mCounter;
};

static PRUint32 numberOfCallsIntoWindowProc = 0;

LRESULT CALLBACK
nsSystemTrayService::WindowProc(HWND hwnd,
                                UINT uMsg,
                                WPARAM wParam,
                                LPARAM lParam)
{
  if (numberOfCallsIntoWindowProc > 0)
    // don't reenter this function ever or we could crash (if the popup
    // frame is still being destroyed)
    return FALSE;

  AutoReentryBlocker blocker(&numberOfCallsIntoWindowProc);

  bool handled = true;
  
  PRUint32 message = 0;
  PRUint32 mask = 0;
  POINT mousePos;

  PRUint16 button;
  PRInt32 detail;
  nsAutoString typeArg;

  switch(uMsg) {
    case WM_TRAYICON:
      break;
    case WM_CREATE:
      return 0;
    case WM_NCCREATE:
      return true;
    case WM_ACTIVATE:
      if (LOWORD(wParam) == WA_INACTIVE)
      {
        // if we are activating the popup menu then it will hide itself after
        // command selection, so we just ignore this
        // I don't suppose that using the Window class name is the cleanest
        // way to do this, but it's hopefully a temporary hack anyway until
        // the issues with popups are solved
        // (see https://bugzilla.mozilla.org/show_bug.cgi?id=279703) and I
        // can't see a way to get at the HWND of the popup to remember it
        TCHAR classname[256];
        HWND hNewWnd = (HWND) lParam;
        ::GetClassName(hNewWnd, classname, 256);
        if (_tcscmp(classname, _T("MozillaDropShadowWindowClass")) == 0)
          return FALSE;

        typeArg = NS_LITERAL_STRING("blur");
        detail = -1;
        break;
      }
    default:
      handled = false;
  }
  
  // convert the Win32 event into a DOM event
  // screaming to be encapsulated somewhere else (utility class?)
  // only supporting click events for the time being

  nsCOMPtr<nsIDOMElement> targetElement;
  nsCOMPtr<nsIDOMEventTarget> eventTarget;
  if (handled)
  {
    targetElement = (nsIDOMElement *) GetProp(hwnd, S_PROPINST);
    eventTarget = do_QueryInterface(targetElement);
  }

  if (!eventTarget)
    handled = false;

  nsAutoString syntheticEvent;

  if (uMsg == WM_TRAYICON && handled) {
    switch (lParam) {
      case WM_LBUTTONDOWN:
        button = 0;
        detail = 1;
        typeArg = NS_LITERAL_STRING("mousedown");
        break;
      case WM_RBUTTONDOWN:
        button = 2;
        detail = 1;
        typeArg = NS_LITERAL_STRING("mousedown");
        break;
      case WM_MBUTTONDOWN:
        button = 1;
        detail = 1;
        typeArg = NS_LITERAL_STRING("mousedown");
        break;
      case WM_LBUTTONUP:
        button = 0;
        detail = 1;
        typeArg = NS_LITERAL_STRING("mouseup");
        syntheticEvent = NS_LITERAL_STRING("click");
        break;
      case WM_RBUTTONUP:
        button = 2;
        detail = 1;
        typeArg = NS_LITERAL_STRING("mouseup");
        syntheticEvent = NS_LITERAL_STRING("click");
        break;
      case WM_MBUTTONUP:
        button = 1;
        detail = 1;
        typeArg = NS_LITERAL_STRING("mouseup");
        syntheticEvent = NS_LITERAL_STRING("click");
        break;
      case WM_CONTEXTMENU:
        button = 2;
        detail = 1;
        typeArg = NS_LITERAL_STRING("contextmenu");
        break;
      case WM_LBUTTONDBLCLK:
        button = 0;
        detail = 2;
        typeArg = NS_LITERAL_STRING("click");
        break;
      case WM_MBUTTONDBLCLK:
        button = 1;
        detail = 2;
        typeArg = NS_LITERAL_STRING("click");
        break;
      case WM_RBUTTONDBLCLK:
        button = 2;
        detail = 2;
        typeArg = NS_LITERAL_STRING("click");
        break;
      default:
        handled = false;
        break;
    }
  }
  
  bool shiftArg = PR_FALSE;
  bool ctrlArg = PR_FALSE;
  bool altArg = PR_FALSE;

  if (handled) {
    GetCursorPos(&mousePos);
    // check modifier key states
    if (::GetKeyState(VK_CONTROL) & 0x8000)
      ctrlArg = PR_TRUE;
    if (::GetKeyState(VK_MENU) & 0x8000)
      altArg = PR_TRUE;
    if (::GetKeyState(VK_SHIFT) & 0x8000)
      shiftArg = PR_TRUE;
   
    // set the hidden window to foreground so the popup
    // menu can go away when we lose focus
    ::SetForegroundWindow(hwnd);
   
    POINT clientPos;
    clientPos.x = mousePos.x;
    clientPos.y = mousePos.y;
    
    nsresult rv;
    nsCOMPtr<nsIDOMElement> element(do_QueryInterface(eventTarget, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMDocument> document;
    rv = element->GetOwnerDocument(getter_AddRefs(document));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMDocumentEvent>
      documentEvent(do_QueryInterface(document, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMDocumentView>
      documentView(do_QueryInterface(document, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMAbstractView> abstractView;
    rv = documentView->GetDefaultView(getter_AddRefs(abstractView));
    NS_ENSURE_SUCCESS(rv, rv);

    bool ret = PR_TRUE;
    nsCOMPtr<nsIDOMEvent> event;
    if (detail == -1)
      // not a mouse event
      rv = CreateEvent(documentEvent, typeArg, getter_AddRefs(event));
    else
      rv = CreateMouseEvent(documentEvent, typeArg, abstractView, detail,
        mousePos.x, mousePos.y, clientPos.x, clientPos.y, ctrlArg, altArg,
        shiftArg, PR_FALSE, button, getter_AddRefs(event));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = eventTarget->DispatchEvent(event, &ret);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!syntheticEvent.IsEmpty())
    {
      rv = CreateMouseEvent(documentEvent, syntheticEvent, abstractView, detail,
        mousePos.x, mousePos.y, clientPos.x, clientPos.y, ctrlArg, altArg,
        shiftArg, PR_FALSE, button, getter_AddRefs(event));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = eventTarget->DispatchEvent(event, &ret);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    if (!ret)
    {
      // handler called preventDefault, so we're done
      PostMessage(hwnd, WM_NULL, 0, 0);
      return FALSE;
    }
  }
  
  return ::CallWindowProc(
    DefWindowProc,
    hwnd,
    uMsg,
    wParam,
    lParam);
}

nsresult
nsSystemTrayService::CreateEvent(nsIDOMDocumentEvent* documentEvent,
                                 const nsAString& typeArg,
                                 nsIDOMEvent** _retval)
{
  nsresult rv;
  nsCOMPtr<nsIDOMEvent> event;
  rv = documentEvent->CreateEvent(NS_LITERAL_STRING("events"),
    getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = event->InitEvent(typeArg, PR_FALSE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = event;
  NS_ADDREF(*_retval);
  return NS_OK;
}

nsresult
nsSystemTrayService::CreateMouseEvent(nsIDOMDocumentEvent* documentEvent,
                                      const nsAString& typeArg,
                                      nsIDOMAbstractView* viewArg,
                                      PRInt32 detailArg,
                                      PRInt32 screenXArg, PRInt32 screenYArg,
                                      PRInt32 clientXArg, PRInt32 clientYArg,
                                      bool ctrlKeyArg, bool altKeyArg,
                                      bool shiftKeyArg, bool metaKeyArg,
                                      PRUint16 buttonArg,
                                      nsIDOMEvent** _retval)
{
  nsresult rv;
  nsCOMPtr<nsIDOMEvent> event;
  rv = documentEvent->CreateEvent(NS_LITERAL_STRING("mouseevent"), getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMMouseEvent> mouseEvent(do_QueryInterface(event, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mouseEvent->InitMouseEvent(typeArg, PR_FALSE, PR_TRUE, viewArg,
    detailArg, screenXArg, screenYArg, clientXArg, clientYArg, ctrlKeyArg, altKeyArg,
    shiftKeyArg, metaKeyArg, buttonArg, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(mouseEvent, _retval);
}

nsresult
nsSystemTrayService::GetDOMWindow(nsIXULWindow* xulWindow,
                                  nsIDOMWindow** _retval)
{
  nsresult rv;
  nsCOMPtr<nsIDocShell> docShell;
  rv = xulWindow->GetDocShell(getter_AddRefs(docShell));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInterfaceRequestor> requestor(do_QueryInterface(docShell, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = requestor->GetInterface(NS_GET_IID(nsIDOMWindow), (void **) _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* attribute imgIContainer image; */
NS_IMETHODIMP
nsSystemTrayService::GetImage(imgIContainer * *aImage)
{
  *aImage = mImage;
  NS_IF_ADDREF(*aImage);
  return NS_OK;
}

NS_IMETHODIMP
nsSystemTrayService::SetImage(imgIContainer * aImage)
{
  mImage = aImage;
  return NS_OK;
}

/* readonly attribute bool isMultiPartChannel; */
NS_IMETHODIMP
nsSystemTrayService::GetIsMultiPartChannel(bool *aIsMultiPartChannel)
{
  *aIsMultiPartChannel = PR_FALSE;
  return NS_OK;
}

/* void onStartRequest(in imgIRequest aRequest); */
NS_IMETHODIMP
nsSystemTrayService::OnStartRequest(imgIRequest* aRequest)
{
  return NS_OK;
}

/* void onStartDecode (in imgIRequest aRequest); */
NS_IMETHODIMP
nsSystemTrayService::OnStartDecode(imgIRequest *aRequest)
{
  return NS_OK;
}

/* void onStartContainer (in imgIRequest aRequest,
  in imgIContainer aContainer); */
NS_IMETHODIMP
nsSystemTrayService::OnStartContainer(imgIRequest *aRequest,
                                      imgIContainer *aContainer)
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
                                     bool aCurrentFrame,
                                     const nsIntRect * aRect)
{
  return NS_OK;
}

/* void onStopFrame (in imgIRequest aRequest, in unsigned long aFrame); */
NS_IMETHODIMP
nsSystemTrayService::OnStopFrame(imgIRequest *aRequest,
                                 PRUint32 aFrame)
{
  return NS_OK;
}

/* void onStopContainer (in imgIRequest aRequest,
  in imgIContainer aContainer); */
NS_IMETHODIMP
nsSystemTrayService::OnStopContainer(imgIRequest *aRequest,
                                     imgIContainer *aContainer)
{
  return NS_OK;
}

/* void onStopDecode (in imgIRequest aRequest, in nsresult status,
  in wstring statusArg); */
NS_IMETHODIMP
nsSystemTrayService::OnStopDecode(imgIRequest *aRequest,
                                  nsresult status, const PRUnichar *statusArg)
{
  return NS_OK;
}

/* void onStopRequest(in imgIRequest aRequest, in boolean aIsLastPart); */
NS_IMETHODIMP
nsSystemTrayService::OnStopRequest(imgIRequest* aRequest,
                                   bool aIsLastPart)
{
  return NS_OK;
}

/* [noscript] void frameChanged (in imgIContainer aContainer,
  in nsIntRect aDirtyRect); */
NS_IMETHODIMP
nsSystemTrayService::FrameChanged(imgIContainer *aContainer,
                                  nsIntRect * aDirtyRect)
{
  return NS_OK;
}
