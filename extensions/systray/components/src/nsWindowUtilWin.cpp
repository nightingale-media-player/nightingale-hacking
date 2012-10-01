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

#include "nsWindowUtilWin.h"
#include "nsStringGlue.h"
#include "nsIBaseWindow.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDOMAbstractView.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOMDocumentView.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMWindow.h"
#include "nsIInterfaceRequestor.h"
#include "nsIWebNavigation.h"
#include "nsIXULWindow.h"

#define S_PROPINST TEXT("_NS_WINDOWUTIL_INST")
#define S_PROPPROC TEXT("_NS_WINDOWUTIL_PROC")

#define MK_ENSURE_NATIVE(res)                              \
   PR_BEGIN_MACRO                                           \
     NS_ENSURE_TRUE(0 != res || 0 == ::GetLastError(),      \
       ::GetLastError() + MK_ERROR_OFFSET);                 \
   PR_END_MACRO

#define MK_ERROR_OFFSET       (0xD0000000 + (__LINE__ * 0x10000))

NS_IMPL_ISUPPORTS1(nsWindowUtil, nsIWindowUtil)

nsWindowUtil::nsWindowUtil()
{
  mOldProc = NULL;
}

nsWindowUtil::~nsWindowUtil()
{
  if (mOldProc)
  {
    ::RemoveProp(this->mWnd, S_PROPINST);
    ::SetProp(this->mWnd, S_PROPPROC, (HANDLE)this->mOldProc);
  }
}

/* void init (in nsIDOMWindow win); */
NS_IMETHODIMP
nsWindowUtil::Init(nsIDOMWindow *win)
{
  nsresult rv;
  mDOMWindow = win;

  nsCOMPtr<nsIDocShell> docShell;
  rv = GetDocShell(getter_AddRefs(docShell));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIBaseWindow> baseWindow(do_QueryInterface(docShell, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nativeWindow theNativeWindow;
  rv = baseWindow->GetParentNativeWindow( &theNativeWindow );
  NS_ENSURE_SUCCESS(rv, rv);

  mWnd = reinterpret_cast<HWND>(theNativeWindow);
  NS_ENSURE_TRUE(mWnd, NS_ERROR_UNEXPECTED);

  return NS_OK;
}

/* readonly attribute nsIXULWindow xulWindow */
NS_IMETHODIMP
nsWindowUtil::GetXulWindow(nsIXULWindow **_retval)
{
  NS_ENSURE_STATE(mDOMWindow);

  nsresult rv;
  nsCOMPtr<nsIInterfaceRequestor>
    requestor(do_QueryInterface(mDOMWindow, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIWebNavigation> nav;
  rv = requestor->GetInterface(NS_GET_IID(nsIWebNavigation),
    getter_AddRefs(nav));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDocShellTreeItem> treeItem(do_QueryInterface(nav, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
  rv = treeItem->GetTreeOwner(getter_AddRefs(treeOwner));
  NS_ENSURE_SUCCESS(rv, rv);

  requestor = do_QueryInterface(treeOwner, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return requestor->GetInterface(NS_GET_IID(nsIXULWindow), (void **) _retval);
}

/* readonly attribute nsIDocShell docShell */
NS_IMETHODIMP
nsWindowUtil::GetDocShell(nsIDocShell **_retval)
{
  nsresult rv;
  nsCOMPtr<nsIXULWindow> xulWindow;
  rv = GetXulWindow(getter_AddRefs(xulWindow));
  NS_ENSURE_SUCCESS(rv, rv);

  return xulWindow->GetDocShell(_retval);
}

/* void minimize(); */
NS_IMETHODIMP
nsWindowUtil::Minimize()
{
  ::ShowWindow(mWnd, SW_MINIMIZE);
  return NS_OK;
}

/* readonly attribute boolean hidden; */
NS_IMETHODIMP
nsWindowUtil::GetHidden(bool* _retval)
{
  NS_ENSURE_STATE(mWnd);

  if (::IsWindowVisible(mWnd))
    *_retval = PR_FALSE;
  else
    *_retval = PR_TRUE;

  return NS_OK;
}

/* void watch() */
NS_IMETHODIMP
nsWindowUtil::Watch()
{
  NS_ENSURE_STATE(mWnd);

  // subclass the window (need to intercept WM_TRAYICON)
  ::SetLastError(0);
  
  if (::GetProp(mWnd, S_PROPINST))
     return NS_ERROR_ALREADY_INITIALIZED;
  MK_ENSURE_NATIVE(::SetProp(mWnd, S_PROPINST, (HANDLE)this));
  mOldProc = (WNDPROC)::GetWindowLongPtr(mWnd, GWLP_WNDPROC);
  ::SetLastError(0);
  MK_ENSURE_NATIVE(::SetWindowLongPtr(
      mWnd,
      GWLP_WNDPROC,
      (LONG_PTR)nsWindowUtil::WindowProc
    ));

  return NS_OK;
}

NS_IMETHODIMP
nsWindowUtil::Unwatch()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

LRESULT CALLBACK
nsWindowUtil::WindowProc(HWND hwnd,
                         UINT uMsg,
                         WPARAM wParam,
                         LPARAM lParam)
{
  bool handled = true;
  WNDPROC proc;
  nsWindowUtil* self =
    (nsWindowUtil*)GetProp(hwnd, S_PROPINST);

  if (self && self->mOldProc && self->mDOMWindow) {
    proc = self->mOldProc;
  } else {
    // property not found - we don't exist anymore
    // use the original window proc
    proc = (WNDPROC)GetProp(hwnd, S_PROPPROC);
    if (!proc)
      // can't find the right process... skip over to the default
      // (this will, at the minimum, totally break the app)
      proc = DefWindowProc;
    handled = false;
  }
 
  /*
   * For this next section, including all the switches, we are simply
   * collecting messages we feel are important to collect.  Once we collect
   * all the windows messages that may be important to us, then these messages
   * are dispatched as DOM events to the window.
   */

  nsAutoString typeArg;

  POINT clientPos = { 0, 0};
  POINT screenPos = { 0, 0};

  if (handled) {
    switch (uMsg) {
      case WM_NCLBUTTONDOWN:
        switch(wParam) {
          // these are the min, max, and close buttons for
          case HTMINBUTTON:
            typeArg = NS_LITERAL_STRING("minimizing");
            break;
          case HTMAXBUTTON:
            typeArg = NS_LITERAL_STRING("maximizing");
            break;
          case HTCLOSE:
            typeArg = NS_LITERAL_STRING("closing");
            break;
          default:
            handled = false;
        }
        clientPos.x = screenPos.x = LOWORD(lParam);
        clientPos.y = screenPos.y = LOWORD(lParam);
        ::ClientToScreen(hwnd, &screenPos);
        break;
      case WM_ACTIVATE:
        switch (LOWORD(wParam)) {
          case WA_ACTIVE:
          case WA_CLICKACTIVE:
            // window is being activated
            typeArg = NS_LITERAL_STRING("activating");
            break;
          default:
            handled = false;
        }
        break;
      case WM_SIZE:
        switch (wParam) {
          case SIZE_MINIMIZED:
            typeArg = NS_LITERAL_STRING("minimizing");
            break;
          default:
            // for some reason this message is received with wParam ==
            // SIZE_MAXIMIZED when the browser is shutting down, and calling
            // Restore() results in the maximized window being "restored"
            // (returned to non-maximized) size. This size is preserved
            // after restarting, which is annoying, so I'm commenting it out
            // until someone can prove that this is actually useful.
            //
            // we're resizing, but not minimized, so some part must be visible
            //typeArg = NS_LITERAL_STRING("activating");
            break;
        }
        break;
      case WM_SYSCOMMAND:
        switch (wParam) {
          case SC_CLOSE:
            //The user has clicked on the top left window icon and selected close...
            //Or the user typed Alt+F4.
            typeArg = NS_LITERAL_STRING("closing");
            break;
          default:
            handled = false;
        }
    		break;
      default:
        handled = false;
        break;
    }
  }
  
  bool ctrlArg = PR_FALSE;
  bool altArg = PR_FALSE;
  bool shiftArg = PR_FALSE;

  if (handled) {
    // check modifier key states
    if (::GetKeyState(VK_CONTROL) & 0x8000)
      ctrlArg = PR_TRUE;
    if (::GetKeyState(VK_MENU) & 0x8000)
      altArg = PR_TRUE;
    if (::GetKeyState(VK_SHIFT) & 0x8000)
      shiftArg = PR_TRUE;
    
    // dispatch the event
    bool ret = TRUE;
    nsresult rv;
    
    nsCOMPtr<nsIDOMEventTarget> eventTarget(do_QueryInterface(self->mDOMWindow, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMDocument> document;
    rv = self->mDOMWindow->GetDocument(getter_AddRefs(document));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMDocumentEvent> documentEvent(do_QueryInterface(document, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMDocumentView> documentView(do_QueryInterface(document, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMAbstractView> abstractView;
    rv = documentView->GetDefaultView(getter_AddRefs(abstractView));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMEvent> event;
    rv = documentEvent->CreateEvent(NS_LITERAL_STRING("mouseevent"), getter_AddRefs(event));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMMouseEvent> mouseEvent(do_QueryInterface(event, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mouseEvent->InitMouseEvent(typeArg, PR_TRUE, PR_TRUE, abstractView,
      1, screenPos.x, screenPos.y, clientPos.x, clientPos.y, ctrlArg, altArg,
      shiftArg, PR_FALSE, 0, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = eventTarget->DispatchEvent(mouseEvent, &ret);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!ret)
      // event was hooked
      return FALSE;
  }
  
  return CallWindowProc(proc, hwnd, uMsg, wParam, lParam);
}
