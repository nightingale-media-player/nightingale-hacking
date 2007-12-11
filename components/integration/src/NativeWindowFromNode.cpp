/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

/** 
 * \file NativeWindowFromNode.cpp
 * \brief Finds the NativeWindow handle associated with a DOM node - Implementation.
 */
 
  // Abuse the accessibility interfaces to extract a native window handle from a DOM node
  
#include "NativeWindowFromNode.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDOMNode.h"

#include <xpcom/nsCOMPtr.h>
#include "nsIServiceManager.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentView.h"
#include "nsIDOMAbstractView.h"
#include "nsIWebNavigation.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIBaseWindow.h"
#include "nsIWidget.h"

//-----------------------------------------------------------------------------
NATIVEWINDOW NativeWindowFromNode::get(nsISupports *window)
{
  NATIVEWINDOW wnd;
  nsIWidget *widget = NativeWindowFromNode::getWidget(window);
  if (!widget) return NULL;

#ifdef XP_WIN
  wnd = reinterpret_cast<NATIVEWINDOW>(widget->GetNativeData(NS_NATIVE_WIDGET));
#elif defined(XP_MACOSX)
  wnd = reinterpret_cast<NATIVEWINDOW>(widget->GetNativeData(NS_NATIVE_DISPLAY)); 
#elif defined(XP_UNIX)
  wnd = reinterpret_cast<NATIVEWINDOW>(widget->GetNativeData(NS_NATIVE_WIDGET));
#endif

  return wnd;
} // NativeWindowFromNode::get

nsIWidget *NativeWindowFromNode::getWidget(nsISupports *window)
{
  nsCOMPtr<nsIDOMAbstractView> domAbstractView;
  nsCOMPtr<nsIDOMDocumentView> domDocumentView(do_QueryInterface(window));

  //The <window> xul element does not implement nsIDOMDocumentView, 
  //so check for AbstractView if there is no DocumentView.
  if (!domDocumentView) {
    domAbstractView = do_QueryInterface(window);
    if (!domAbstractView) return NULL;
  }
  else {
    domDocumentView->GetDefaultView(getter_AddRefs(domAbstractView)); 
    if (!domAbstractView) return NULL;
  }

  nsCOMPtr<nsIWebNavigation> webNavigation(do_GetInterface(domAbstractView)); 
  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem(do_QueryInterface(webNavigation)); 
  if (!docShellTreeItem) return NULL;

  nsCOMPtr<nsIDocShellTreeOwner> docShellTreeOwner; 
  docShellTreeItem->GetTreeOwner(getter_AddRefs(docShellTreeOwner)); 
  if (!docShellTreeOwner) return NULL;

  nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(docShellTreeOwner); 
  if (!baseWindow) return NULL;

  nsCOMPtr<nsIWidget> widget; 
  baseWindow->GetMainWidget(getter_AddRefs(widget)); 

  return widget;
} // NativeWindowFromNode::get

