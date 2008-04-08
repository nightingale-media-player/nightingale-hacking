/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

//------------------------------------------------------------------------------
//
// Songbird prompter.
//
//------------------------------------------------------------------------------

/**
 * \file  sbPrompter.cpp
 * \brief Songbird Prompter Source.
 */

//------------------------------------------------------------------------------
//
// Songbird prompter imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbPrompter.h"

// Mozilla imports.
#include <nsServiceManagerUtils.h>


//------------------------------------------------------------------------------
//
// Songbird prompter nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS3(sbPrompter,
                              sbIPrompter,
                              nsIPromptService,
                              nsIObserver)


//------------------------------------------------------------------------------
//
// Songbird prompter sbIPrompter implementation.
//
//------------------------------------------------------------------------------

/**
 * Open a dialog window.
 * Instead of using the openDialog method, use the nsIWindowWatcher.openWindow
 * method like nsIPromptService does so that the dialog is presented event when
 * no windows are available.
 * \sa nsIDOMWindowInternal.openDialog
 */

NS_IMETHODIMP
sbPrompter::OpenDialog(const nsAString& aUrl,
                       const nsAString& aName,
                       const nsAString& aOptions,
                       nsISupports*     aExtraArgument,
                       nsIDOMWindow**   _retval)
{
  nsresult rv;

  // Get the parent window.
  nsCOMPtr<nsIDOMWindow> parent;
  rv = GetParent(getter_AddRefs(parent));
  NS_ENSURE_SUCCESS(rv, rv);

  // Set up dialog options.  Add the same options that nsIPromptService uses.
  nsAutoString options(aOptions);
  if (!options.IsEmpty())
    options.AppendLiteral(",");
  options.AppendLiteral("centerscreen,chrome,modal,titlebar");

  // Open the dialog.
  return mWindowWatcher->OpenWindow(parent,
                                    NS_ConvertUTF16toUTF8(aUrl).get(),
                                    NS_ConvertUTF16toUTF8(aName).get(),
                                    NS_ConvertUTF16toUTF8(options).get(),
                                    aExtraArgument,
                                    _retval);
}


//
// Getters/setters.
//

/**
 * The parent window type.
 */

NS_IMETHODIMP
sbPrompter::GetParentWindowType(nsAString& aParentWindowType)
{
  aParentWindowType.Assign(mParentWindowType);
  return NS_OK;
}

NS_IMETHODIMP
sbPrompter::SetParentWindowType
              (const nsAString& aParentWindowType)
{
  mParentWindowType.Assign(aParentWindowType);
  return NS_OK;
}


/**
 * If true, wait for a parent window of the configured type to be available
 * before prompting.
 */

NS_IMETHODIMP
sbPrompter::GetWaitForWindow(PRBool* aWaitForWindow)
{
  NS_ENSURE_ARG_POINTER(aWaitForWindow);
  *aWaitForWindow = mWaitForWindow;
  return NS_OK;
}

NS_IMETHODIMP
sbPrompter::SetWaitForWindow(PRBool aWaitForWindow)
{
  mWaitForWindow = aWaitForWindow;
  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Songbird prompter nsIPromptService implementation.
//
//------------------------------------------------------------------------------

/**
 * Forward Alert.
 */

NS_IMETHODIMP
sbPrompter::Alert(nsIDOMWindow*    aParent,
                  const PRUnichar* aDialogTitle,
                  const PRUnichar* aText)
{
  nsresult rv;

  // Get the parent window.
  nsCOMPtr<nsIDOMWindow> parent = aParent;
  if (!parent) {
    rv = GetParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Forward method call.
  return mPromptService->Alert(parent, aDialogTitle, aText);
}


/**
 * Forward AlertCheck.
 */

NS_IMETHODIMP
sbPrompter::AlertCheck(nsIDOMWindow*    aParent,
                       const PRUnichar* aDialogTitle,
                       const PRUnichar* aText,
                       const PRUnichar* aCheckMsg,
                       PRBool*          aCheckState)
{
  nsresult rv;

  // Get the parent window.
  nsCOMPtr<nsIDOMWindow> parent = aParent;
  if (!parent) {
    rv = GetParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Forward method call.
  return mPromptService->AlertCheck(parent,
                                    aDialogTitle,
                                    aText,
                                    aCheckMsg,
                                    aCheckState);
}


/**
 * Forward Confirm.
 */

NS_IMETHODIMP
sbPrompter::Confirm(nsIDOMWindow*    aParent,
                    const PRUnichar* aDialogTitle,
                    const PRUnichar* aText,
                    PRBool*          _retval)
{
  nsresult rv;

  // Get the parent window.
  nsCOMPtr<nsIDOMWindow> parent = aParent;
  if (!parent) {
    rv = GetParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Forward method call.
  return mPromptService->Confirm(parent,
                                 aDialogTitle,
                                 aText,
                                 _retval);
}


/**
 * Forward ConfirmCheck.
 */

NS_IMETHODIMP
sbPrompter::ConfirmCheck(nsIDOMWindow*    aParent,
                         const PRUnichar* aDialogTitle,
                         const PRUnichar* aText,
                         const PRUnichar* aCheckMsg,
                         PRBool*          aCheckState,
                         PRBool*          _retval)
{
  nsresult rv;

  // Get the parent window.
  nsCOMPtr<nsIDOMWindow> parent = aParent;
  if (!parent) {
    rv = GetParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Forward method call.
  return mPromptService->ConfirmCheck(parent,
                                      aDialogTitle,
                                      aText,
                                      aCheckMsg,
                                      aCheckState,
                                      _retval);
}


/**
 * Forward ConfirmEx.
 */

NS_IMETHODIMP
sbPrompter::ConfirmEx(nsIDOMWindow*    aParent,
                      const PRUnichar* aDialogTitle,
                      const PRUnichar* aText,
                      PRUint32         aButtonFlags,
                      const PRUnichar* aButton0Title,
                      const PRUnichar* aButton1Title,
                      const PRUnichar* aButton2Title,
                      const PRUnichar* aCheckMsg,
                      PRBool*          aCheckState,
                      PRInt32*         _retval)
{
  nsresult rv;

  // Get the parent window.
  nsCOMPtr<nsIDOMWindow> parent = aParent;
  if (!parent) {
    rv = GetParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Forward method call.
  return mPromptService->ConfirmEx(parent,
                                   aDialogTitle,
                                   aText,
                                   aButtonFlags,
                                   aButton0Title,
                                   aButton1Title,
                                   aButton2Title,
                                   aCheckMsg,
                                   aCheckState,
                                   _retval);
}


/**
 * Forward Prompt.
 */

NS_IMETHODIMP
sbPrompter::Prompt(nsIDOMWindow*    aParent,
                   const PRUnichar* aDialogTitle,
                   const PRUnichar* aText,
                   PRUnichar**      aValue, 
                   const PRUnichar* aCheckMsg,
                   PRBool*          aCheckState,
                   PRBool*          _retval)
{
  nsresult rv;

  // Get the parent window.
  nsCOMPtr<nsIDOMWindow> parent = aParent;
  if (!parent) {
    rv = GetParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Forward method call.
  return mPromptService->Prompt(parent,
                                aDialogTitle,
                                aText,
                                aValue,
                                aCheckMsg,
                                aCheckState,
                                _retval);
}


/**
 * Forward PromptUsernameAndPassword.
 */

NS_IMETHODIMP
sbPrompter::PromptUsernameAndPassword(nsIDOMWindow*    aParent,
                                      const PRUnichar* aDialogTitle,
                                      const PRUnichar* aText,
                                      PRUnichar**      aUsername,
                                      PRUnichar**      aPassword,
                                      const PRUnichar* aCheckMsg,
                                      PRBool*          aCheckState,
                                      PRBool*          _retval)
{
  nsresult rv;

  // Get the parent window.
  nsCOMPtr<nsIDOMWindow> parent = aParent;
  if (!parent) {
    rv = GetParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Forward method call.
  return mPromptService->PromptUsernameAndPassword(parent,
                                                   aDialogTitle,
                                                   aText,
                                                   aUsername,
                                                   aPassword,
                                                   aCheckMsg,
                                                   aCheckState,
                                                   _retval);
}


/**
 * Forward PromptPassword.
 */

NS_IMETHODIMP
sbPrompter::PromptPassword(nsIDOMWindow*    aParent,
                           const PRUnichar* aDialogTitle,
                           const PRUnichar* aText,
                           PRUnichar**      aPassword,
                           const PRUnichar* aCheckMsg,
                           PRBool*          aCheckState,
                           PRBool*          _retval)
{
  nsresult rv;

  // Get the parent window.
  nsCOMPtr<nsIDOMWindow> parent = aParent;
  if (!parent) {
    rv = GetParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Forward method call.
  return mPromptService->PromptPassword(parent,
                                        aDialogTitle,
                                        aText,
                                        aPassword,
                                        aCheckMsg,
                                        aCheckState,
                                        _retval);
}


/**
 * Forward Select.
 */

NS_IMETHODIMP
sbPrompter::Select(nsIDOMWindow*     aParent,
                   const PRUnichar*  aDialogTitle,
                   const PRUnichar*  aText,
                   PRUint32          aCount,
                   const PRUnichar** aSelectList,
                   PRInt32*          aOutSelection,
                   PRBool*           _retval)
{
  nsresult rv;

  // Get the parent window.
  nsCOMPtr<nsIDOMWindow> parent = aParent;
  if (!parent) {
    rv = GetParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Forward method call.
  return mPromptService->Select(parent,
                                aDialogTitle,
                                aText,
                                aCount,
                                aSelectList,
                                aOutSelection,
                                _retval);
}


//------------------------------------------------------------------------------
//
// Songbird prompter nsIObserver implementation.
//
//------------------------------------------------------------------------------

/**
 * Observe will be called when there is a notification for the
 * topic |aTopic|.  This assumes that the object implementing
 * this interface has been registered with an observer service
 * such as the nsIObserverService. 
 *
 * If you expect multiple topics/subjects, the impl is 
 * responsible for filtering.
 *
 * You should not modify, add, remove, or enumerate 
 * notifications in the implemention of observe. 
 *
 * @param aSubject : Notification specific interface pointer.
 * @param aTopic   : The notification topic or subject.
 * @param aData    : Notification specific wide string.
 *                    subject event.
 */

NS_IMETHODIMP
sbPrompter::Observe(nsISupports*     aSubject,
                    const char*      aTopic,
                    const PRUnichar* aData)
{
  /*XXXeps handle quit application and other events. */
  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Internal Songbird prompter services.
//
//------------------------------------------------------------------------------

/**
 * Initialize the prompter services.
 */

nsresult
sbPrompter::Init()
{
  nsresult rv;

  // Get the window mediator service.
  mWindowMediator = do_GetService("@mozilla.org/appshell/window-mediator;1",
                                  &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the window watcher service.
  mWindowWatcher = do_GetService("@mozilla.org/embedcomp/window-watcher;1",
                                 &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the prompt service.
  mPromptService = do_GetService("@mozilla.org/embedcomp/prompt-service;1",
                                 &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set defaults.
  mParentWindowType = NS_LITERAL_STRING("Songbird:Main");
  mWaitForWindow = PR_FALSE;

  return NS_OK;
}


/**
 * Get the parent window and return it in aParent.
 *
 * \aParent Parent window.  May be null if no window is available and the
 *          instance is not set to wait for one.
 */

nsresult
sbPrompter::GetParent(nsIDOMWindow** aParent)
{
  nsCOMPtr<nsIDOMWindow> parent;
  nsresult rv;

  // Get the parent.
  nsCOMPtr<nsIDOMWindowInternal> _parent;
  rv = mWindowMediator->GetMostRecentWindow(mParentWindowType.get(),
                                            getter_AddRefs(_parent));
  NS_ENSURE_SUCCESS(rv, rv);
  if (_parent) {
    parent = do_QueryInterface(_parent, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // If no window of the configured type is available and we're not waiting for
  // one, use the currently active window as the parent.
  if (!parent && !mWaitForWindow) {
    rv = mWindowWatcher->GetActiveWindow(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Return results.
  if (parent)
    NS_ADDREF(*aParent = parent);
  else
    *aParent = nsnull;

  return NS_OK;
}

