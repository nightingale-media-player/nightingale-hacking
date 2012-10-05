/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird prompter.
//
//------------------------------------------------------------------------------
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
#include <mozilla/Mutex.h>
#include <nsComponentManagerUtils.h>
#include <nsIDOMWindow.h>
#include <nsPIPromptService.h>
#include <nsServiceManagerUtils.h>
#include <sbThreadUtils.h>


//------------------------------------------------------------------------------
//
// Songbird prompter defs.
//
//------------------------------------------------------------------------------

#define kPromptURL "chrome://global/content/commonDialog.xul"
#define kHTMLPromptURL "chrome://global/content/commonDialog.xul?useHTML"
static const char kQuestionIconClass[] = "question-icon";
static const char kAlertIconClass[] = "alert-icon";


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
 * Open a dialog window with the chrome URL specified by aUrl and parent
 * specified by aParent.  The window name is specified by aName and the window
 * options are specified by aOptions.  Additional window arguments may be
 * provided in aExtraArguments.
 *
 * \param aParent             Parent window.
 * \param aUrl                URL of window chrome.
 * \param aName               Window name.
 * \param aOptions            Window options.
 * \param aExtraArguments     Extra window arguments.
 *
 * When called on the main-thread, return NS_ERROR_NOT_AVAILABLE if window of
 * configured type is not available and configured to wait for window.
 */

NS_IMETHODIMP
sbPrompter::OpenDialog(nsIDOMWindow*    aParent,
                       const nsAString& aUrl,
                       const nsAString& aName,
                       const nsAString& aOptions,
                       nsISupports*     aExtraArgument,
                       nsIDOMWindow**   _retval)
{
  return OpenDialogImpl(aParent,aUrl,aName,aOptions,aExtraArgument,_retval);
}

nsresult
sbPrompter::OpenDialogImpl(nsIDOMWindow*    aParent,
                       const nsAString& aUrl,
                       const nsAString& aName,
                       const nsAString& aOptions,
                       nsISupports*     aExtraArgument,
                       nsIDOMWindow**   _retval)
{
  nsresult rv;

  // If not on main thread, proxy to it.
  if (!NS_IsMainThread()) {
    nsRefPtr<sbRunnable_<nsresult>> job =
      new sbRunnableMethod6_<nsresult,sbPrompter,
        nsIDOMWindow*,const nsAString&,const nsAString&,const nsAString&,
        nsISupports*,nsIDOMWindow**>(
          *this,&sbPrompter::OpenDialogImpl,
          aParent,aUrl,aName,aOptions,aExtraArgument,_retval);

    // Call proxied prompter until a window is available.
    while (1) {
      NS_DispatchToMainThread(job);
      rv = job->Wait();
      if (rv != NS_ERROR_NOT_AVAILABLE)
        NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
      if (NS_SUCCEEDED(rv))
        break;

      // Wait for a window to be available.
      rv = mSBWindowWatcher->WaitForWindow(mParentWindowType);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
  }

  // Get the parent window.
  nsCOMPtr<nsIDOMWindow> parent = aParent;
  if (!parent) {
    rv = GetParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
  }

  // If configured to wait for the desired window and the window is not
  // available, return a not available error indication.
  if (mWaitForWindow && !mParentWindowType.IsEmpty() && !parent)
    return NS_ERROR_NOT_AVAILABLE;

  // Set up dialog options.  Add the same options that nsIPromptService uses.
  nsAutoString options(aOptions);
  if (!options.IsEmpty())
    options.AppendLiteral(",");
  options.AppendLiteral("centerscreen,chrome,modal,titlebar");

  // Open the dialog.
  rv = mWindowWatcher->OpenWindow(parent,
                                  NS_ConvertUTF16toUTF8(aUrl).get(),
                                  NS_ConvertUTF16toUTF8(aName).get(),
                                  NS_ConvertUTF16toUTF8(options).get(),
                                  aExtraArgument,
                                  _retval);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

  return NS_OK;
}


/**
 * Open a window with the chrome URL specified by aUrl and parent specified
 * by aParent. The window name is specified by aName and the window options are
 * specified by aOptions. Additional window arguments may be provided in
 * aExtraArguments.
 *
 * There are no default options for openWindow.
 *
 * \param aParent             Parent window.
 * \param aUrl                URL of window chrome.
 * \param aName               Window name.
 * \param aOptions            Window options.
 * \param aExtraArguments     Extra window arguments.
 */
NS_IMETHODIMP
sbPrompter::OpenWindow(nsIDOMWindow*    aParent,
                       const nsAString& aUrl,
                       const nsAString& aName,
                       const nsAString& aOptions,
                       nsISupports*     aExtraArgument,
                       nsIDOMWindow**   _retval)
{
  return OpenWindowImpl(aParent,aUrl,aName,aOptions,aExtraArgument,_retval);
}

nsresult
sbPrompter::OpenWindowImpl(nsIDOMWindow*    aParent,
                       const nsAString& aUrl,
                       const nsAString& aName,
                       const nsAString& aOptions,
                       nsISupports*     aExtraArgument,
                       nsIDOMWindow**   _retval)
{
  nsresult rv;

  // If not on main thread, proxy to it.
  if (!NS_IsMainThread()) {
    nsRefPtr<sbRunnable_<nsresult>> job =
      new sbRunnableMethod6_<nsresult,sbPrompter,
        nsIDOMWindow*,const nsAString&,const nsAString&,const nsAString&,
        nsISupports*,nsIDOMWindow**>(
          *this,&sbPrompter::OpenWindowImpl,
          aParent,aUrl,aName,aOptions,aExtraArgument,_retval);

    // Call proxied prompter until a window is available.
    while (1) {
      NS_DispatchToMainThread(job);
      rv = job->Wait();
      if (rv != NS_ERROR_NOT_AVAILABLE)
        NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
      if (NS_SUCCEEDED(rv))
        break;

      // Wait for a window to be available.
      rv = mSBWindowWatcher->WaitForWindow(mParentWindowType);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
  }

  // Get the parent window.
  nsCOMPtr<nsIDOMWindow> parent = aParent;
  if (!parent) {
    rv = GetParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
  }

  // If configured to wait for the desired window and the window is not
  // available, return a not available error indication.
  if (mWaitForWindow && !mParentWindowType.IsEmpty() && !parent)
    return NS_ERROR_NOT_AVAILABLE;

  // Open the dialog.
  rv = mWindowWatcher->OpenWindow(parent,
                                  NS_ConvertUTF16toUTF8(aUrl).get(),
                                  NS_ConvertUTF16toUTF8(aName).get(),
                                  NS_ConvertUTF16toUTF8(aOptions).get(),
                                  aExtraArgument,
                                  _retval);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

  return NS_OK;
}


/**
 * Cancel and close the current prompter window.
 */

NS_IMETHODIMP
sbPrompter::Cancel()
{
  return CancelImpl();
}

nsresult
sbPrompter::CancelImpl()
{
  nsresult rv;

  // If not on main thread, proxy to it.
  if (!NS_IsMainThread()) {
    nsRefPtr<sbRunnable_<nsresult>> job =
      new sbRunnableMethod0_<nsresult,sbPrompter>(
      *this,&sbPrompter::CancelImpl);
    NS_DispatchToMainThread(job);
    rv = job->Wait();
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  // If there's a current prompter window, close it.
  if (mCurrentWindow) {
    nsCOMPtr<nsIDOMWindowInternal>
      window = do_QueryInterface(mCurrentWindow, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = window->Close();
    NS_ENSURE_SUCCESS(rv, rv);
    mCurrentWindow = nsnull;
  }

  return NS_OK;
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
  mozilla::MutexAutoLock autoLock(mPrompterLock);
  aParentWindowType.Assign(mParentWindowType);
  return NS_OK;
}

NS_IMETHODIMP
sbPrompter::SetParentWindowType(const nsAString& aParentWindowType)
{
  mozilla::MutexAutoLock autoLock(mPrompterLock);
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
  mozilla::MutexAutoLock autoLock(mPrompterLock);
  *aWaitForWindow = mWaitForWindow;
  return NS_OK;
}

NS_IMETHODIMP
sbPrompter::SetWaitForWindow(PRBool aWaitForWindow)
{
  mozilla::MutexAutoLock autoLock(mPrompterLock);
  mWaitForWindow = aWaitForWindow;
  return NS_OK;
}


/**
 * If true, render prompt text as HTML.
 */

NS_IMETHODIMP
sbPrompter::GetRenderHTML(PRBool* aRenderHTML)
{
  NS_ENSURE_ARG_POINTER(aRenderHTML);
  mozilla::MutexAutoLock autoLock(mPrompterLock);
  *aRenderHTML = mRenderHTML;
  return NS_OK;
}

NS_IMETHODIMP
sbPrompter::SetRenderHTML(PRBool aRenderHTML)
{
  mozilla::MutexAutoLock autoLock(mPrompterLock);
  mRenderHTML = aRenderHTML;
  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Songbird prompter nsIPromptService implementation.
//
//------------------------------------------------------------------------------

/**
 * Forward Alert.
 *
 * When called on the main-thread, return NS_ERROR_NOT_AVAILABLE if window of
 * configured type is not available and configured to wait for window.
 */

NS_IMETHODIMP
sbPrompter::Alert(nsIDOMWindow*    aParent,
                  const PRUnichar* aDialogTitle,
                  const PRUnichar* aText)
{
  return AlertImpl(aParent,aDialogTitle,aText);
}

nsresult
sbPrompter::AlertImpl(nsIDOMWindow*    aParent,
                  const PRUnichar* aDialogTitle,
                  const PRUnichar* aText)
{
  nsresult rv;

  // If not on main thread, proxy to it.
  if (!NS_IsMainThread()) {
    nsRefPtr<sbRunnable_<nsresult>> job =
      new sbRunnableMethod3_<nsresult,sbPrompter,
        nsIDOMWindow*,const PRUnichar*,const PRUnichar*>(
          *this,&sbPrompter::AlertImpl,aParent,aDialogTitle,aText);
    while (1) {
      // Call the proxied prompter.
      rv = prompter->Alert(aParent, aDialogTitle, aText);
      if (rv != NS_ERROR_NOT_AVAILABLE)
        NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
      if (NS_SUCCEEDED(rv))
        break;

      // Wait for a window to be available.
      rv = mSBWindowWatcher->WaitForWindow(mParentWindowType);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
  }

  // Get the parent window.
  nsCOMPtr<nsIDOMWindow> parent = aParent;
  if (!parent) {
    rv = GetParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
  }

  // If configured to wait for the desired window and the window is not
  // available, return a not available error indication.
  if (mWaitForWindow && !mParentWindowType.IsEmpty() && !parent)
    return NS_ERROR_NOT_AVAILABLE;

  // Set up the dialog parameter block.
  nsCOMPtr<nsIDialogParamBlock>
    paramBlock = do_CreateInstance(NS_DIALOGPARAMBLOCK_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = paramBlock->SetInt(nsPIPromptService::eNumberButtons, 1);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = paramBlock->SetString(nsPIPromptService::eMsg, aText);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = paramBlock->SetString(nsPIPromptService::eDialogTitle, aDialogTitle);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = paramBlock->SetString(nsPIPromptService::eIconClass,
                             NS_ConvertASCIItoUTF16(kAlertIconClass).get());
  NS_ENSURE_SUCCESS(rv, rv);

  // Present the dialog.
  rv = PresentPrompterDialog(parent, paramBlock);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Forward AlertCheck.
 *
 * When called on the main-thread, return NS_ERROR_NOT_AVAILABLE if window of
 * configured type is not available and configured to wait for window.
 */

NS_IMETHODIMP
sbPrompter::AlertCheck(nsIDOMWindow*    aParent,
                       const PRUnichar* aDialogTitle,
                       const PRUnichar* aText,
                       const PRUnichar* aCheckMsg,
                       PRBool*          aCheckState)
{
  return AlertCheckImpl(aParent,aDialogTitle,aText,aCheckMsg,aCheckState);
}

nsresult
sbPrompter::AlertCheckImpl(nsIDOMWindow*    aParent,
                       const PRUnichar* aDialogTitle,
                       const PRUnichar* aText,
                       const PRUnichar* aCheckMsg,
                       bool*          aCheckState)
{
  nsresult rv;

  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aCheckState);

  // If not on main thread, proxy to it.
  if (!NS_IsMainThread()) {
    nsRefPtr<sbRunnable_<nsresult>> job =
      new sbRunnableMethod5_<nsresult,sbPrompter,
        nsIDOMWindow*,const PRUnichar*,const PRUnichar*,
        const PRUnichar*,bool*>(
          *this,&sbPrompter::AlertCheckImpl,
          aParent,aDialogTitle,aText,aCheckMsg,aCheckState);

    // Call proxied prompter until a window is available.
    while (1) {
      NS_DispatchToMainThread(job);
      rv = job->Wait();
      if (rv != NS_ERROR_NOT_AVAILABLE)
        NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
      if (NS_SUCCEEDED(rv))
        break;

      // Wait for a window to be available.
      rv = mSBWindowWatcher->WaitForWindow(mParentWindowType);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
  }

  // Get the parent window.
  nsCOMPtr<nsIDOMWindow> parent = aParent;
  if (!parent) {
    rv = GetParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
  }

  // If configured to wait for the desired window and the window is not
  // available, return a not available error indication.
  if (mWaitForWindow && !mParentWindowType.IsEmpty() && !parent)
    return NS_ERROR_NOT_AVAILABLE;

  // Set up the dialog parameter block.
  nsCOMPtr<nsIDialogParamBlock>
    paramBlock = do_CreateInstance(NS_DIALOGPARAMBLOCK_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = paramBlock->SetInt(nsPIPromptService::eNumberButtons, 1);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = paramBlock->SetString(nsPIPromptService::eMsg, aText);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = paramBlock->SetString(nsPIPromptService::eDialogTitle, aDialogTitle);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = paramBlock->SetString(nsPIPromptService::eCheckboxMsg, aCheckMsg);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = paramBlock->SetString(nsPIPromptService::eIconClass,
                             NS_ConvertASCIItoUTF16(kAlertIconClass).get());
  NS_ENSURE_SUCCESS(rv, rv);

  // Present the dialog.
  rv = PresentPrompterDialog(parent, paramBlock);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the results.
  PRInt32 checkState;
  rv = paramBlock->GetInt(nsPIPromptService::eCheckboxState, &checkState);
  NS_ENSURE_SUCCESS(rv, rv);
  *aCheckState = checkState ? PR_TRUE : PR_FALSE;

  return NS_OK;
}


/**
 * Forward Confirm.
 *
 * When called on the main-thread, return NS_ERROR_NOT_AVAILABLE if window of
 * configured type is not available and configured to wait for window.
 */

NS_IMETHODIMP
sbPrompter::Confirm(nsIDOMWindow*    aParent,
                    const PRUnichar* aDialogTitle,
                    const PRUnichar* aText,
                    PRBool*          _retval)
{
  return ConfirmImpl(aParent,aDialogTitle,aText,_retval);
}

nsresult
sbPrompter::ConfirmImpl(nsIDOMWindow*    aParent,
                    const PRUnichar* aDialogTitle,
                    const PRUnichar* aText,
                    bool*          _retval)
{
  nsresult rv;

  // Validate arguments.
  NS_ENSURE_ARG_POINTER(_retval);

  // If not on main thread, proxy to it.
  if (!NS_IsMainThread()) {
    nsRefPtr<sbRunnable_<nsresult>> job =
      new sbRunnableMethod4_<nsresult,sbPrompter,
        nsIDOMWindow*,const PRUnichar*,const PRUnichar*,bool*>(
          *this,&sbPrompter::ConfirmImpl,aParent,aDialogTitle,aText,_retval);

    // Call proxied prompter until a window is available.
    while (1) {
      NS_DispatchToMainThread(job);
      rv = job->Wait();
      if (rv != NS_ERROR_NOT_AVAILABLE)
        NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
      if (NS_SUCCEEDED(rv))
        break;

      // Wait for a window to be available.
      rv = mSBWindowWatcher->WaitForWindow(mParentWindowType);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
  }

  // Get the parent window.
  nsCOMPtr<nsIDOMWindow> parent = aParent;
  if (!parent) {
    rv = GetParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
  }

  // If configured to wait for the desired window and the window is not
  // available, return a not available error indication.
  if (mWaitForWindow && !mParentWindowType.IsEmpty() && !parent)
    return NS_ERROR_NOT_AVAILABLE;

  // Set up the dialog parameter block.
  nsCOMPtr<nsIDialogParamBlock>
    paramBlock = do_CreateInstance(NS_DIALOGPARAMBLOCK_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = paramBlock->SetInt(nsPIPromptService::eNumberButtons, 2);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = paramBlock->SetString(nsPIPromptService::eMsg, aText);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = paramBlock->SetString(nsPIPromptService::eDialogTitle, aDialogTitle);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = paramBlock->SetString(nsPIPromptService::eIconClass,
                             NS_ConvertASCIItoUTF16(kQuestionIconClass).get());
  NS_ENSURE_SUCCESS(rv, rv);

  // Present the dialog.
  rv = PresentPrompterDialog(parent, paramBlock);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the results.
  PRInt32 buttonPressed = 0;
  rv = paramBlock->GetInt(nsPIPromptService::eButtonPressed, &buttonPressed);
  NS_ENSURE_SUCCESS(rv, rv);
  *_retval = buttonPressed ? PR_FALSE : PR_TRUE;

  return NS_OK;
}


/**
 * Forward ConfirmCheck.
 *
 * When called on the main-thread, return NS_ERROR_NOT_AVAILABLE if window of
 * configured type is not available and configured to wait for window.
 */

NS_IMETHODIMP
sbPrompter::ConfirmCheck(nsIDOMWindow*    aParent,
                         const PRUnichar* aDialogTitle,
                         const PRUnichar* aText,
                         const PRUnichar* aCheckMsg,
                         PRBool*          aCheckState,
                         PRBool*          _retval)
{
  return ConfirmCheckImpl(aParent,aDialogTitle,aText,aCheckMsg,aCheckState,_retval);
}

nsresult
sbPrompter::ConfirmCheckImpl(nsIDOMWindow*    aParent,
                         const PRUnichar* aDialogTitle,
                         const PRUnichar* aText,
                         const PRUnichar* aCheckMsg,
                         bool*          aCheckState,
                         bool*          _retval)
{
  nsresult rv;

  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aCheckState);
  NS_ENSURE_ARG_POINTER(_retval);

  // If not on main thread, proxy to it.
  if (!NS_IsMainThread()) {
    // Get a main thread proxy.
    nsCOMPtr<sbIPrompter> prompter;
    rv = GetProxiedPrompter(getter_AddRefs(prompter));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    // Call proxied prompter until a window is available.
    while (1) {
      // Call the proxied prompter.
      rv = prompter->ConfirmCheck(aParent,
                                  aDialogTitle,
                                  aText,
                                  aCheckMsg,
                                  aCheckState,
                                  _retval);
      if (rv != NS_ERROR_NOT_AVAILABLE)
        NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
      if (NS_SUCCEEDED(rv))
        break;

      // Wait for a window to be available.
      rv = mSBWindowWatcher->WaitForWindow(mParentWindowType);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
  }

  // Get the parent window.
  nsCOMPtr<nsIDOMWindow> parent = aParent;
  if (!parent) {
    rv = GetParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
  }

  // If configured to wait for the desired window and the window is not
  // available, return a not available error indication.
  if (mWaitForWindow && !mParentWindowType.IsEmpty() && !parent)
    return NS_ERROR_NOT_AVAILABLE;

  // Set up the dialog parameter block.
  nsCOMPtr<nsIDialogParamBlock>
    paramBlock = do_CreateInstance(NS_DIALOGPARAMBLOCK_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = paramBlock->SetInt(nsPIPromptService::eNumberButtons, 2);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = paramBlock->SetString(nsPIPromptService::eMsg, aText);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = paramBlock->SetString(nsPIPromptService::eDialogTitle, aDialogTitle);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = paramBlock->SetString(nsPIPromptService::eCheckboxMsg, aCheckMsg);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = paramBlock->SetString(nsPIPromptService::eIconClass,
                             NS_ConvertASCIItoUTF16(kQuestionIconClass).get());
  NS_ENSURE_SUCCESS(rv, rv);

  // Present the dialog.
  rv = PresentPrompterDialog(parent, paramBlock);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the button pressed results.
  PRInt32 buttonPressed = 0;
  rv = paramBlock->GetInt(nsPIPromptService::eButtonPressed, &buttonPressed);
  NS_ENSURE_SUCCESS(rv, rv);
  *_retval = buttonPressed ? PR_FALSE : PR_TRUE;

  // Get the check state results.
  PRInt32 checkState;
  rv = paramBlock->GetInt(nsPIPromptService::eCheckboxState, &checkState);
  NS_ENSURE_SUCCESS(rv, rv);
  *aCheckState = checkState ? PR_TRUE : PR_FALSE;

  return NS_OK;
}


/**
 * Forward ConfirmEx.
 *
 * When called on the main-thread, return NS_ERROR_NOT_AVAILABLE if window of
 * configured type is not available and configured to wait for window.
 *XXXeps add HTML support
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
  return ConfirmExImpl(aParent,aDialogTitle,aText,aButtonFlags,
      aButton0Title,aButton1Title,aButton2Title,
      aCheckMsg,aCheckState,_retval);
}

nsresult
sbPrompter::ConfirmExImpl(nsIDOMWindow*    aParent,
                      const PRUnichar* aDialogTitle,
                      const PRUnichar* aText,
                      PRUint32         aButtonFlags,
                      const PRUnichar* aButton0Title,
                      const PRUnichar* aButton1Title,
                      const PRUnichar* aButton2Title,
                      const PRUnichar* aCheckMsg,
                      bool*          aCheckState,
                      PRInt32*         _retval)
{
  nsresult rv;

  // If not on main thread, proxy to it.
  if (!NS_IsMainThread()) {
    nsRefPtr<sbRunnable_<nsresult>> job =
      new sbRunnableMethod10_<nsresult,sbPrompter,
        nsIDOMWindow*,const PRUnichar*,const PRUnichar*,PRUint32,
        const PRUnichar*,const PRUnichar*,const PRUnichar*,const PRUnichar*,
        bool*,PRInt32*>(
          *this,&sbPrompter::ConfirmExImpl,aParent,aDialogTitle,aText,
          aButtonFlags,aButton0Title,aButton1Title,aButton2Title,
          aCheckMsg,aCheckState,_retval);

    // Call proxied prompter until a window is available.
    while (1) {
      NS_DispatchToMainThread(job);
      rv = job->Wait();
      if (rv != NS_ERROR_NOT_AVAILABLE)
        NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
      if (NS_SUCCEEDED(rv))
        break;

      // Wait for a window to be available.
      rv = mSBWindowWatcher->WaitForWindow(mParentWindowType);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
  }

  // Get the parent window.
  nsCOMPtr<nsIDOMWindow> parent = aParent;
  if (!parent) {
    rv = GetParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
  }

  // If configured to wait for the desired window and the window is not
  // available, return a not available error indication.
  if (mWaitForWindow && !mParentWindowType.IsEmpty() && !parent)
    return NS_ERROR_NOT_AVAILABLE;

  // Forward method call.
  rv = mPromptService->ConfirmEx(parent,
                                 aDialogTitle,
                                 aText,
                                 aButtonFlags,
                                 aButton0Title,
                                 aButton1Title,
                                 aButton2Title,
                                 aCheckMsg,
                                 aCheckState,
                                 _retval);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

  return NS_OK;
}


/**
 * Forward Prompt.
 *
 * When called on the main-thread, return NS_ERROR_NOT_AVAILABLE if window of
 * configured type is not available and configured to wait for window.
 *XXXeps add HTML support
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
  return PromptImpl(aParent,aDialogTitle,aText,aValue,aCheckMsg,aCheckState,_retval);
}

nsresult
sbPrompter::PromptImpl(nsIDOMWindow*    aParent,
                   const PRUnichar* aDialogTitle,
                   const PRUnichar* aText,
                   PRUnichar**      aValue,
                   const PRUnichar* aCheckMsg,
                   bool*          aCheckState,
                   bool*          _retval)
{
  nsresult rv;

  // If not on main thread, proxy to it.
  if (!NS_IsMainThread()) {
    nsRefPtr<sbRunnable_<nsresult>> job =
      new sbRunnableMethod7_<nsresult,sbPrompter,
        nsIDOMWindow*,const PRUnichar*,const PRUnichar*,PRUnichar**,
        const PRUnichar*,bool*,bool*>(
          *this,&sbPrompter::PromptImpl,aParent,aDialogTitle,aText,
          aValue,aCheckMsg,aCheckState,_retval);

    // Call proxied prompter until a window is available.
    while (1) {
      NS_DispatchToMainThread(job);
      rv = job->Wait();
      if (rv != NS_ERROR_NOT_AVAILABLE)
        NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
      if (NS_SUCCEEDED(rv))
        break;

      // Wait for a window to be available.
      rv = mSBWindowWatcher->WaitForWindow(mParentWindowType);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
  }

  // Get the parent window.
  nsCOMPtr<nsIDOMWindow> parent = aParent;
  if (!parent) {
    rv = GetParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
  }

  // If configured to wait for the desired window and the window is not
  // available, return a not available error indication.
  if (mWaitForWindow && !mParentWindowType.IsEmpty() && !parent)
    return NS_ERROR_NOT_AVAILABLE;

  // Forward method call.
  rv = mPromptService->Prompt(parent,
                              aDialogTitle,
                              aText,
                              aValue,
                              aCheckMsg,
                              aCheckState,
                              _retval);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

  return NS_OK;
}


/**
 * Forward PromptUsernameAndPassword.
 *
 * When called on the main-thread, return NS_ERROR_NOT_AVAILABLE if window of
 * configured type is not available and configured to wait for window.
 *XXXeps add HTML support
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
  return PromptUsernameAndPasswordImpl(aParent,aDialogTitle,aText,
            aUsername,aPassword,aCheckMsg,aCheckState,_retval);
}

nsresult
sbPrompter::PromptUsernameAndPasswordImpl(nsIDOMWindow*    aParent,
                                      const PRUnichar* aDialogTitle,
                                      const PRUnichar* aText,
                                      PRUnichar**      aUsername,
                                      PRUnichar**      aPassword,
                                      const PRUnichar* aCheckMsg,
                                      bool*          aCheckState,
                                      bool*          _retval)
{
  nsresult rv;

  // If not on main thread, proxy to it.
  if (!NS_IsMainThread()) {
    nsRefPtr<sbRunnable_<nsresult>> job =
      new sbRunnableMethod8_<nsresult,sbPrompter,
        nsIDOMWindow*,const PRUnichar*,const PRUnichar*,
        PRUnichar**,PRUnichar**,
        const PRUnichar*,bool*,bool*>(
          *this,&sbPrompter::PromptUsernameAndPasswordImpl,
          aParent,aDialogTitle,aText,
          aUsername,aPassword,aCheckMsg,aCheckState,_retval);

    // Call proxied prompter until a window is available.
    while (1) {
      NS_DispatchToMainThread(job);
      rv = job->Wait();
      if (rv != NS_ERROR_NOT_AVAILABLE)
        NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
      if (NS_SUCCEEDED(rv))
        break;

      // Wait for a window to be available.
      rv = mSBWindowWatcher->WaitForWindow(mParentWindowType);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
  }

  // Get the parent window.
  nsCOMPtr<nsIDOMWindow> parent = aParent;
  if (!parent) {
    rv = GetParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
  }

  // If configured to wait for the desired window and the window is not
  // available, return a not available error indication.
  if (mWaitForWindow && !mParentWindowType.IsEmpty() && !parent)
    return NS_ERROR_NOT_AVAILABLE;

  // Forward method call.
  rv = mPromptService->PromptUsernameAndPassword(parent,
                                                 aDialogTitle,
                                                 aText,
                                                 aUsername,
                                                 aPassword,
                                                 aCheckMsg,
                                                 aCheckState,
                                                 _retval);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

  return NS_OK;
}


/**
 * Forward PromptPassword.
 *
 * When called on the main-thread, return NS_ERROR_NOT_AVAILABLE if window of
 * configured type is not available and configured to wait for window.
 *XXXeps add HTML support
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
  return PromptPasswordImpl(
      aParent,aDialogTitle,aText,aPassword,aCheckMsg,aCheckState,_retval);
}

nsresult
sbPrompter::PromptPasswordImpl(nsIDOMWindow*    aParent,
                           const PRUnichar* aDialogTitle,
                           const PRUnichar* aText,
                           PRUnichar**      aPassword,
                           const PRUnichar* aCheckMsg,
                           bool*          aCheckState,
                           bool*          _retval)
{
  nsresult rv;

  // If not on main thread, proxy to it.
  if (!NS_IsMainThread()) {
    nsRefPtr<sbRunnable_<nsresult>> job =
      new sbRunnableMethod7_<nsresult,sbPrompter,
        nsIDOMWindow*,const PRUnichar*,const PRUnichar*,PRUnichar**,
        const PRUnichar*,bool*,bool*>(
          *this,&sbPrompter::PromptPasswordImpl,
          aParent,aDialogTitle,aText,
          aPassword,aCheckMsg,aCheckState,_retval);

    // Call proxied prompter until a window is available.
    while (1) {
      // Call the proxied prompter.
      NS_DispatchToMainThread(job);
      rv = job->Wait();
      if (rv != NS_ERROR_NOT_AVAILABLE)
        NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
      if (NS_SUCCEEDED(rv))
        break;

      // Wait for a window to be available.
      rv = mSBWindowWatcher->WaitForWindow(mParentWindowType);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
  }

  // Get the parent window.
  nsCOMPtr<nsIDOMWindow> parent = aParent;
  if (!parent) {
    rv = GetParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
  }

  // If configured to wait for the desired window and the window is not
  // available, return a not available error indication.
  if (mWaitForWindow && !mParentWindowType.IsEmpty() && !parent)
    return NS_ERROR_NOT_AVAILABLE;

  // Forward method call.
  rv = mPromptService->PromptPassword(parent,
                                      aDialogTitle,
                                      aText,
                                      aPassword,
                                      aCheckMsg,
                                      aCheckState,
                                      _retval);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

  return NS_OK;
}


/**
 * Forward Select.
 *
 * When called on the main-thread, return NS_ERROR_NOT_AVAILABLE if window of
 * configured type is not available and configured to wait for window.
 *XXXeps add HTML support
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
  return SelectImpl(aParent,aDialogTitle,aText,
      aCount,aSelectList,aOutSelection,_retval);
}

nsresult
sbPrompter::SelectImpl(nsIDOMWindow*     aParent,
                   const PRUnichar*  aDialogTitle,
                   const PRUnichar*  aText,
                   PRUint32          aCount,
                   const PRUnichar** aSelectList,
                   PRInt32*          aOutSelection,
                   PRBool*           _retval)
{
nsresult rv;

  // If not on main thread, proxy to it.
  if (!NS_IsMainThread()) {
    nsRefPtr<sbRunnable_<nsresult>> job =
      new sbRunnableMethod7_<nsresult,sbPrompter,
        nsIDOMWindow*,const PRUnichar*,const PRUnichar*,PRUint32,
        const PRUnichar**,PRInt32*,bool*>(
          *this,&sbPrompter::SelectImpl,
          aParent,aDialogTitle,aText,aCount,
          aSelectList,aOutSelection,_retval);

    // Call proxied prompter until a window is available.
    while (1) {
      NS_DispatchToMainThread(job);
      rv = job->Wait();
      if (rv != NS_ERROR_NOT_AVAILABLE)
        NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
      if (NS_SUCCEEDED(rv))
        break;

      // Wait for a window to be available.
      rv = mSBWindowWatcher->WaitForWindow(mParentWindowType);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
  }

  // Get the parent window.
  nsCOMPtr<nsIDOMWindow> parent = aParent;
  if (!parent) {
    rv = GetParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
  }

  // If configured to wait for the desired window and the window is not
  // available, return a not available error indication.
  if (mWaitForWindow && !mParentWindowType.IsEmpty() && !parent)
    return NS_ERROR_NOT_AVAILABLE;

  // Forward method call.
  rv = mPromptService->Select(parent,
                              aDialogTitle,
                              aText,
                              aCount,
                              aSelectList,
                              aOutSelection,
                              _retval);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

  return NS_OK;
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
  nsresult rv;

  // Dispatch processing of the event.
  if (!strcmp(aTopic, "sbPrompter::InitOnMainThread")) {
    rv = InitOnMainThread();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Songbird prompter services.
//
//------------------------------------------------------------------------------

/**
 * Construct a Songbird prompter object.
 */

sbPrompter::sbPrompter() :
  mPrompterLock("sbPrompter.mPrompterLock"),
  mRenderHTML(PR_FALSE)
{
}


/**
 * Destroy a Songbird prompter object.
 */

sbPrompter::~sbPrompter()
{
  // Dispose of prompter lock.
  //pretty sure this is automatic?
}


/**
 * Initialize the prompter services.
 */

nsresult
sbPrompter::Init()
{
  nsresult rv;	

  // Set defaults.
  {
    mozilla::MutexAutoLock autoLock(mPrompterLock);
    mWaitForWindow = PR_FALSE;
  }

  // Perform initialization on main thread.  If not already on main thread,
  // create a main thread proxy to this nsIObserver interface and initialize on
  // the main thread through it.
  if (NS_IsMainThread()) {
    rv = InitOnMainThread();
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    // Create the job:
    nsRefPtr<sbRunnable_<nsresult>> job = new sbRunnableMethod0_<nsresult,sbPrompter>(
      *this,
      &sbPrompter::InitOnMainThread);
    NS_DispatchToMainThread(job);
    rv = job->Wait();
    NS_ENSURE_SUCCESS(rv,rv);
  }

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Internal Songbird prompter services.
//
//------------------------------------------------------------------------------

/**
 * Initialize the prompter services on the main thread.
 */

nsresult
sbPrompter::InitOnMainThread()
{
  nsresult rv;

  // Get the window watcher service.
  mWindowWatcher = do_GetService("@mozilla.org/embedcomp/window-watcher;1",
                                 &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the Songbird window watcher service.
  mSBWindowWatcher =
    do_GetService("@songbirdnest.com/Songbird/window-watcher;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the prompt service.
  mPromptService = do_GetService("@mozilla.org/embedcomp/prompt-service;1",
                                 &rv);
  NS_ENSURE_SUCCESS(rv, rv);

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

  // Operate under lock.
  mozilla::MutexAutoLock autoLock(mPrompterLock);

  // If the Songbird window watcher is shutting down, don't wait for a window.
  {
    PRBool isShuttingDown;
    rv = mSBWindowWatcher->GetIsShuttingDown(&isShuttingDown);
    NS_ENSURE_SUCCESS(rv, rv);
    if (isShuttingDown)
      mWaitForWindow = PR_FALSE;
  }

  // Get the parent.
  if (mParentWindowType.Length() > 0) {
    rv = mSBWindowWatcher->GetWindow(mParentWindowType, getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // If no window of the configured type is available and we're not waiting for
  // one, use the currently active window as the parent.
  if (!parent && !mWaitForWindow) {
    rv = mWindowWatcher->GetActiveWindow(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Return results.
  NS_IF_ADDREF(*aParent = parent);

  return NS_OK;
}


/**
 * Return this prompter proxied to the main thread in aPrompter.
 *
 * \param aPrompter             Returned proxied prompter.
 */

nsresult
sbPrompter::GetProxiedPrompter(sbIPrompter** aPrompter)
{
  // Validate arguments.
  NS_ASSERTION(aPrompter, "aPrompter is null");

  // Function variables.
  nsresult rv;

  // Create a main thread proxy for the prompter.
  nsCOMPtr<nsIProxyObjectManager>
    proxyObjectManager = do_GetService("@mozilla.org/xpcomproxy;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = proxyObjectManager->GetProxyForObject
                             (NS_PROXY_TO_MAIN_THREAD,
                              NS_GET_IID(sbIPrompter),
                              NS_ISUPPORTS_CAST(sbIPrompter*, this),
                              nsIProxyObjectManager::INVOKE_SYNC |
                              nsIProxyObjectManager::FORCE_PROXY_CREATION,
                              (void**) aPrompter);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Present the prompter dialog using the parent window specified by aParent and
 * dialog parameter block specified by aParamBlock.
 *
 * \param aParent               Parent window.
 * \param aParamBlock           Prompter dialog parameter block.
 */

nsresult
sbPrompter::PresentPrompterDialog(nsIDOMWindow*        aParent,
                                  nsIDialogParamBlock* aParamBlock)
{
  nsresult rv;

  // Get the prompter dialog URI spec.
  nsAutoString dialogURISpec;
  if (mRenderHTML)
    dialogURISpec.Assign(NS_LITERAL_STRING(kHTMLPromptURL));
  else
    dialogURISpec.Assign(NS_LITERAL_STRING(kPromptURL));

  // Open the prompter dialog.
  nsCOMPtr<nsISupports> extraArgument = do_QueryInterface(aParamBlock, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = OpenDialog(aParent,
                  dialogURISpec,
                  NS_LITERAL_STRING("_blank"),
                  NS_LITERAL_STRING(""),
                  extraArgument,
                  getter_AddRefs(mCurrentWindow));
  NS_ENSURE_SUCCESS(rv, rv);

  // Prompter dialog window is no longer open.
  mCurrentWindow = nsnull;

  return NS_OK;
}

