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
 * \file WindowCloak.cpp
 * \brief Songbird Window Cloaker Object Implementation.
 */

#include "WindowCloak.h"

#include <dom/nsIDOMWindow.h>
#include <dom/nsIScriptGlobalObject.h>
#include <docshell/nsIDocShell.h>
#include <docshell/nsIDocShellTreeItem.h>
#include <docshell/nsIDocShellTreeOwner.h>
#include <webbrwsr/nsIEmbeddingSiteWindow.h>
#include <xpcom/nsCOMPtr.h>
#include <xpcom/nsIInterfaceRequestorUtils.h>

sbWindowCloak::~sbWindowCloak()
{
  // This will delete all of the sbCloakInfo objects that are still laying
  // around.
  if (mCloakedWindows.IsInitialized())
    mCloakedWindows.Clear();
}

NS_IMPL_ISUPPORTS1(sbWindowCloak, sbIWindowCloak)

NS_IMETHODIMP
sbWindowCloak::Cloak(nsIDOMWindow* aDOMWindow)
{
  NS_ENSURE_ARG_POINTER(aDOMWindow);
  return SetVisibility(aDOMWindow, PR_FALSE);
}

NS_IMETHODIMP
sbWindowCloak::Uncloak(nsIDOMWindow* aDOMWindow)
{
  NS_ENSURE_ARG_POINTER(aDOMWindow);
  return SetVisibility(aDOMWindow, PR_TRUE);
}

NS_IMETHODIMP
sbWindowCloak::IsCloaked(nsIDOMWindow* aDOMWindow,
                         PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(aDOMWindow);

  // Return early if nothing has been added to our hashtable.
  if (!mCloakedWindows.IsInitialized()) {
    *_retval = PR_FALSE;
    return NS_OK;
  }

  nsresult rv;
  nsCOMPtr<nsIScriptGlobalObject> scriptGlobalObj =
    do_QueryInterface(aDOMWindow, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIDocShell* docShell = scriptGlobalObj->GetDocShell();
  NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocShellTreeItem> treeItem = do_QueryInterface(docShell, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
  rv = treeItem->GetTreeOwner(getter_AddRefs(treeOwner));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIEmbeddingSiteWindow> embedWindow =
    do_GetInterface(treeOwner, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  sbCloakInfo* cloakInfo = nsnull;
  mCloakedWindows.Get(embedWindow, &cloakInfo);

  *_retval = cloakInfo && !cloakInfo->mVisible ? PR_TRUE : PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbWindowCloak::SetVisibility(nsIDOMWindow* aDOMWindow,
                             PRBool aVisible)
{
  // XXXben Someday we could just ask the window if it is visible rather than
  //        trying to maintain a list ourselves.

  // Make sure the hashtable is initialized.
  if (!mCloakedWindows.IsInitialized())
    NS_ENSURE_TRUE(mCloakedWindows.Init(), NS_ERROR_FAILURE);

  nsresult rv;
  nsCOMPtr<nsIScriptGlobalObject> scriptGlobalObj =
    do_QueryInterface(aDOMWindow, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIDocShell* docShell = scriptGlobalObj->GetDocShell();
  NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocShellTreeItem> treeItem = do_QueryInterface(docShell, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
  rv = treeItem->GetTreeOwner(getter_AddRefs(treeOwner));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIEmbeddingSiteWindow> embedWindow =
    do_GetInterface(treeOwner, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // See if we have previously cloaked this window.
  sbCloakInfo* cloakInfo = nsnull;
  PRBool alreadyHashedWindow = mCloakedWindows.Get(embedWindow, &cloakInfo);

  if (!cloakInfo) {
    // This is the first time we have seen this window so assume it is already
    // visible. Return early if there is nothing to do.
    if (aVisible)
      return NS_OK;

    // Otherwise make a new sbCloakInfo structure to hold state info.
    NS_NEWXPCOM(cloakInfo, sbCloakInfo);
    NS_ENSURE_TRUE(cloakInfo, NS_ERROR_OUT_OF_MEMORY);

    // Add it to our hashtable here so that the memory will be freed on
    // shutdown.
    if (!mCloakedWindows.Put(embedWindow, cloakInfo))
      return NS_ERROR_FAILURE;

    // Set the defaults.
    cloakInfo->mVisible = PR_TRUE;
  }

  // Return early if there's nothing to do.
  if (cloakInfo->mVisible == aVisible)
    return NS_OK;

  rv = embedWindow->SetVisibility(aVisible);
  NS_ENSURE_SUCCESS(rv, rv);

  // Everything has succeeded so update the sbCloakInfo with the new state.
  cloakInfo->mVisible = aVisible;

  return NS_OK;
}
