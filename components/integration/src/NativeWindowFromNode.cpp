/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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

/** 
 * \file NativeWindowFromNode.cpp
 * \brief Finds the NativeWindow handle associated with a DOM node - Implementation.
 */
 
  // Abuse the accessibility interfaces to extract a native window handle from a DOM node
  
#include "NativeWindowFromNode.h"
#include "nsIAccessibleDocument.h"
#include "nsIAccessNode.h"
#include "nsIAccessible.h"
#include "nsIDOMNode.h"

#ifdef XP_MACOSX
#include "nsIWidget.h"
#endif

#include "nsIAccessibilityService.h"
#include <xpcom/nsCOMPtr.h>
#include "nsIServiceManager.h"

//-----------------------------------------------------------------------------
NATIVEWINDOW NativeWindowFromNode::get(nsISupports *window)
{
  NATIVEWINDOW wnd = NULL;
  
  nsIDOMNode *node = NULL;
  if (window) window->QueryInterface(NS_GET_IID(nsIDOMNode), (void **)&node);
  if (!node) return NULL;

  nsCOMPtr<nsIAccessibilityService> accService = do_GetService("@mozilla.org/accessibilityService;1");

  nsCOMPtr<nsIAccessible> accessible;
  accService->GetAccessibleFor(node, getter_AddRefs(accessible));
  if (!accessible) 
  {
    NS_RELEASE(node);
    return NULL;
  }

  nsIAccessNode *accnode = NULL;
  accessible->QueryInterface(NS_GET_IID(nsIAccessNode), (void **)&accnode);
  if (!accnode) 
  {
    NS_RELEASE(node);
    return NULL;
  }
  
  nsIAccessibleDocument *accdocument = NULL;
  accnode->GetAccessibleDocument(&accdocument);
  if (!accdocument) {
    NS_RELEASE(accnode);
    NS_RELEASE(node);
    return NULL;
  }


#ifdef XP_WIN
  accdocument->GetWindowHandle((void **)&wnd);
  while (GetWindowLong(wnd, GWL_STYLE) & WS_CHILD) wnd = GetParent(wnd);
#elif defined(XP_MACOSX)
  // accdocument->GetWindowHandle will give us a void* to the mac nsWindow widget.
  void *temp = NULL;
  accdocument->GetWindowHandle(&temp);
  nsIWidget *windowWidget = reinterpret_cast<nsIWidget*>(temp);

  // Once we have the nsWindow, we can request the actual window pointer
  wnd = reinterpret_cast<NATIVEWINDOW>(windowWidget->GetNativeData(NS_NATIVE_DISPLAY)); 
#endif

  return wnd; 
} // NativeWindowFromNode::get

