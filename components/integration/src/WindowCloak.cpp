/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2006 POTI, Inc.
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
#include <xpcom/nsCOMPtr.h>

#ifdef XP_MACOSX

#include <dom/nsIDOMWindowInternal.h>
#define OUTER_SPACE_X -32000
#define OUTER_SPACE_Y -32000

#else /* XP_MACOSX */

#include <docshell/nsIDocShell.h>
#include <dom/nsIScriptGlobalObject.h>
#include <widget/nsIBaseWindow.h>
#include <widget/nsIWidget.h>

#endif /* XP_MACOSX */

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

  sbCloakInfo* cloakInfo = nsnull;
  mCloakedWindows.Get(aDOMWindow, &cloakInfo);

  *_retval = cloakInfo && !cloakInfo->mVisible ? PR_TRUE : PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbWindowCloak::SetVisibility(nsIDOMWindow* aDOMWindow,
                             PRBool aVisible)
{
  // XXXben Someday we could just ask the window if it is visible rather than
  //        trying to maintain a list ourselves. Sadly the sbCloakInfo structs
  //        are necessary until we rid ourselves of QuickTime (see below).
  //        After that, though, we should remove all this mess.

  // Make sure the hashtable is initialized.
  if (!mCloakedWindows.IsInitialized())
    NS_ENSURE_TRUE(mCloakedWindows.Init(), NS_ERROR_FAILURE);

  // See if we have previously cloaked this window.
  sbCloakInfo* cloakInfo = nsnull;
  PRBool alreadyHashedWindow = mCloakedWindows.Get(aDOMWindow, &cloakInfo);

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
    if (!mCloakedWindows.Put(aDOMWindow, cloakInfo))
      return NS_ERROR_FAILURE;

    // Set the defaults.
    cloakInfo->mVisible = PR_TRUE;
#ifdef XP_MACOSX
    cloakInfo->mLastX = -1;
    cloakInfo->mLastY = -1;
#endif
  }

  // Return early if there's nothing to do.
  if (cloakInfo->mVisible == aVisible)
    return NS_OK;

  nsresult rv;

#ifdef XP_MACOSX
  // Currently the QuickTime plugin refuses to play anything if the parent
  // window is hidden, so we're just going to kick the window out by the
  // XULRunner hidden window. Remove this as soon as QuickTime is gone!
  nsCOMPtr<nsIDOMWindowInternal> window = do_QueryInterface(aDOMWindow, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (aVisible) {
    NS_ASSERTION(cloakInfo->mLastX >= 0 && cloakInfo->mLastY >= 0,
                 "Trying to uncloak a window without previous position!");

    rv = window->MoveTo(cloakInfo->mLastX, cloakInfo->mLastY);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    PRInt32 currentX;
    rv = window->GetScreenX(&currentX);
    NS_ENSURE_SUCCESS(rv, rv);
    
    PRInt32 currentY;
    rv = window->GetScreenY(&currentY);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = window->MoveTo(OUTER_SPACE_X, OUTER_SPACE_Y);
    NS_ENSURE_SUCCESS(rv, rv);
    
    cloakInfo->mLastX = currentX;
    cloakInfo->mLastY = currentY;
  }
#else /* XP_MACOSX */
  nsCOMPtr<nsIScriptGlobalObject> scriptGlobalObj =
    do_QueryInterface(aDOMWindow, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIBaseWindow> baseWindow =
    do_QueryInterface(scriptGlobalObj->GetDocShell(), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIWidget> mainWidget;
  rv = baseWindow->GetMainWidget(getter_AddRefs(mainWidget));
  NS_ENSURE_SUCCESS(rv, rv);

  // Make sure this is a toplevel window.
  nsCOMPtr<nsIWidget> tempWidget = mainWidget->GetParent();
  while (tempWidget) {
    mainWidget = tempWidget;
    tempWidget = tempWidget->GetParent();
  }
  rv = mainWidget->Show(aVisible);
  NS_ENSURE_SUCCESS(rv, rv);
#endif /* XP_MACOSX */

  // Everything has succeeded so update the sbCloakInfo with the new state.
  cloakInfo->mVisible = aVisible;

  return NS_OK;
}
