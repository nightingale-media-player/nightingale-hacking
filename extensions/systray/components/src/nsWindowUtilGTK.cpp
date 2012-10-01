/* vim: set fileencoding=utf-8 shiftwidth=2 : */
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

#include "nsWindowUtilGTK.h"
#include <nsComponentManagerUtils.h>
#include <nsIInterfaceRequestor.h>
#include <nsIInterfaceRequestorUtils.h>
#include <nsIWebNavigation.h>
#include <nsIDocShellTreeItem.h>
#include <nsIDocShellTreeOwner.h>
#include <nsIXULWindow.h>
#include <nsIWindowUtil.h>
#include <nsIDocShell.h>
#include <nsIBaseWindow.h>

NS_IMPL_ISUPPORTS1(nsWindowUtil, nsIWindowUtil)

nsWindowUtil::nsWindowUtil()
  :mGDKWindow(NULL)
{
}

nsWindowUtil::~nsWindowUtil()
{
  if (mGDKWindow && !gdk_window_is_visible(mGDKWindow)) {
    gdk_window_show_unraised(mGDKWindow);
  }
}

/* readonly attribute boolean hidden; */
NS_IMETHODIMP nsWindowUtil::GetHidden(PRBool *aHidden)
{
  NS_ENSURE_ARG(aHidden);
  if (!mGDKWindow) return NS_ERROR_NOT_INITIALIZED;
  
  gboolean isVisible = gdk_window_is_visible(mGDKWindow);
  *aHidden = isVisible ? PR_FALSE : PR_TRUE;
  return NS_OK;
}

/* void init (in nsIDOMWindow win, in nsIBaseWindow baseWindow); */
NS_IMETHODIMP nsWindowUtil::Init(nsIDOMWindow *aDOMWindow)
{
  #if DEBUG
    NS_WARNING("nsWindowUtil::Init()");
    printf( "nsWindowUtil::Init: %08x\n",
            reinterpret_cast<PRUint32>(aDOMWindow) );
  #endif
  NS_ENSURE_ARG(aDOMWindow);
  if (mDOMWindow) {
    return NS_ERROR_ALREADY_INITIALIZED;
  }
  mDOMWindow = aDOMWindow;
  
  nsresult rv;
  
  nsCOMPtr<nsIDocShell> docShell;
  rv = GetDocShell(getter_AddRefs(docShell));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIBaseWindow> baseWindow( do_QueryInterface(docShell, &rv) );
  NS_ENSURE_SUCCESS(rv, rv);
  nativeWindow theNativeWindow;
  rv = baseWindow->GetParentNativeWindow( &theNativeWindow );
  NS_ENSURE_SUCCESS(rv, rv);

  GdkWindow *gdkWindow = nsnull;
  gdkWindow = reinterpret_cast<GdkWindow*>(theNativeWindow);
  NS_ENSURE_TRUE(gdkWindow, NS_ERROR_UNEXPECTED);
  NS_ENSURE_TRUE(GDK_IS_WINDOW(gdkWindow), NS_ERROR_INVALID_ARG);

  // need to get top level window to hide :/

  mGDKWindow = gdk_window_get_toplevel(gdkWindow);
  NS_ENSURE_TRUE(mGDKWindow, NS_ERROR_UNEXPECTED);

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

/* void minimize (); */
NS_IMETHODIMP nsWindowUtil::Minimize()
{
  if (!mGDKWindow) {
    NS_WARNING("nsWindowUtil::Minimize:: not initialized");
    return NS_ERROR_NOT_INITIALIZED;
  }
  #if DEBUG
    printf( "nsWindowUtil::Minimize(%08x)\n",
            reinterpret_cast<PRUint32>(mGDKWindow) );
  #endif
  NS_ENSURE_TRUE(GDK_IS_WINDOW(mGDKWindow), NS_ERROR_UNEXPECTED);
  #if DEBUG
    printf( "        -> hiding\n");
  #endif
  gdk_window_hide(mGDKWindow);
  return NS_OK;
}

/* void watch (); */
NS_IMETHODIMP nsWindowUtil::Watch()
{
  #if DEBUG
    NS_WARNING("nsWindowUtil::Watch() not implemented");
    printf( "this=%08x cast=%08x\n",
            reinterpret_cast<PRUint32>(this),
            reinterpret_cast<PRUint32>(dynamic_cast<nsIWindowUtil*>(this)) );
  #endif
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void unwatch (); */
NS_IMETHODIMP nsWindowUtil::Unwatch()
{
  #if DEBUG
    NS_WARNING("nsWindowUtil::Unwatch() not implemented");
  #endif
  return NS_ERROR_NOT_IMPLEMENTED;
}

