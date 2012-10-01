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

#ifndef ULONG_PTR
#define ULONG_PTR DWORD
#endif

#ifndef nsSystemTrayService_h__
#define nsSystemTrayService_h__

// needed for QueueUserAPC
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT  0x0400

#include <windows.h>
#include <shellapi.h>

#include "nsISystemTrayService.h"
#include "imgIDecoderObserver.h"
#include "imgILoad.h"
#include "nsIDOMEventListener.h"
#include "nsCOMPtr.h"
#include "nsDataHashtable.h"

class nsIDOMWindow;
class nsIURI;
class nsIXULWindow;

// {F53FCEFA-2F53-40cf-BBAF-2F5896A56E82}
#define NS_SYSTEMTRAYSERVICE_CID                      \
  { 0xf53fcefa, 0x2f53, 0x40cf,                       \
  { 0xbb, 0xaf, 0x2f, 0x58, 0x96, 0xa5, 0x6e, 0x82 } }

#define NS_SYSTEMTRAYSERVICE_CONTRACTID \
  "@mozilla.org/system-tray-service;1"

#define NS_SYSTEMTRAYSERVICE_CLASSNAME "System Tray Service"

class nsIDOMAbstractView;
class nsIDOMDocumentEvent;
class nsIDOMElement;
class nsIDOMEvent;
class nsIDOMEventTarget;
class nsIURI;

class nsSystemTrayService : public nsISystemTrayService,
  public imgILoad, public imgIDecoderObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISYSTEMTRAYSERVICE
  NS_DECL_IMGICONTAINEROBSERVER
  NS_DECL_IMGIDECODEROBSERVER
  NS_DECL_IMGILOAD

  nsSystemTrayService();

  nsresult Init();

private:
  ~nsSystemTrayService();

protected:
  nsresult AddTrayIcon(HWND hwnd, nsIURI* iconURI, const nsAString& iconId,
    nsIDOMElement* targetElement);
  nsresult GetIconForWnd(HWND hwnd, HICON& result);
  nsresult GetIconForURI(nsIURI* iconURI, HICON& result);
  nsresult CreateListenerWindow(HWND* listenerWindow,
    nsIDOMElement* targetElement);
  nsresult GetDOMWindow(nsIXULWindow* xulWindow, nsIDOMWindow** _retval);
  
  static nsresult CreateEvent(nsIDOMDocumentEvent* documentEvent,
    const nsAString& typeArg, nsIDOMEvent** _retval);
  static nsresult CreateMouseEvent(nsIDOMDocumentEvent* documentEvent,
    const nsAString& typeArg, nsIDOMAbstractView* viewArg, PRInt32 detailArg,
    PRInt32 screenXArg, PRInt32 screenYArg, PRInt32 clientXArg,
    PRInt32 clientYArg, PRBool ctrlKeyArg, PRBool altKeyArg,
    PRBool shiftKeyArg, PRBool metkeyArg, PRUint16 buttonArg,
    nsIDOMEvent** _retval);

  static LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

  /* additional members */
  static ATOM s_wndClass;
  nsDataHashtable<nsStringHashKey, NOTIFYICONDATAW> mIconDataMap;
  nsCOMPtr<imgIContainer> mImage;
  
  /* window property constants */
  static const TCHAR* S_PROPINST;
  static const TCHAR* S_PROPPROC;
};

#endif // nsSystemTrayService_h__
