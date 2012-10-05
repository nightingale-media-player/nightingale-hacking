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

#ifndef __SB_PROMPTER_H__
#define __SB_PROMPTER_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird prompter.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbPrompter.h
 * \brief Songbird Prompter Definitions.
 */

// Songbird imports.
#include <sbIPrompter.h>
#include <sbIWindowWatcher.h>

// Mozilla imports.
#include <mozilla/Mutex.h>
#include <nsCOMPtr.h>
#include <nsIDialogParamBlock.h>
#include <nsIObserver.h>
#include <nsIWindowWatcher.h>
#include <nsStringAPI.h>


/**
 * This class implements the Songbird prompter interface.
 */

class sbPrompter : public sbIPrompter,
                   public nsIObserver
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // Inherited interfaces.
  //

  NS_DECL_ISUPPORTS
  NS_DECL_SBIPROMPTER
  NS_DECL_NSIPROMPTSERVICE
  NS_DECL_NSIOBSERVER


  //
  // Songbird prompter services.
  //

  sbPrompter();

  virtual ~sbPrompter();

  nsresult Init();

  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mPrompterLock               Prompter lock.
  // mWindowWatcher              Window watcher service.
  // mSBWindowWatcher            Songbird window watcher service.
  // mPromptService              Prompt service.
  // mParentWindowType           Parent window type.
  // mWaitForWindow              If true, wait for parent window type.
  // mRenderHTML                 If true, render prompt text as HTML.
  // mCurrentWindow              Currently open prompter dialog window.
  //
  // The following fields must only be accessed under the prompter lock:
  //   mParentWindowType
  //   mWaitForWindow
  //   mRenderHTML
  //

  mozilla::Mutex*	            mPrompterLock;
  nsCOMPtr<nsIWindowWatcher>    mWindowWatcher;
  nsCOMPtr<sbIWindowWatcher>    mSBWindowWatcher;
  nsCOMPtr<nsIPromptService>    mPromptService;
  nsString                      mParentWindowType;
  bool                        mWaitForWindow;
  bool                        mRenderHTML;
  nsCOMPtr<nsIDOMWindow>        mCurrentWindow;


  //
  // Internal Songbird prompter services.
  //

  nsresult InitOnMainThread();

  nsresult GetParent(nsIDOMWindow** aParent);

  nsresult PresentPrompterDialog(nsIDOMWindow*        aParent,
                                 nsIDialogParamBlock* aParamBlock);

  nsresult
  sbPrompter::OpenDialogImpl(nsIDOMWindow*    aParent,
                         const nsAString& aUrl,
                         const nsAString& aName,
                         const nsAString& aOptions,
                         nsISupports*     aExtraArgument,
                         nsIDOMWindow**   _retval);
  nsresult
  sbPrompter::OpenWindowImpl(nsIDOMWindow*    aParent,
                         const nsAString& aUrl,
                         const nsAString& aName,
                         const nsAString& aOptions,
                         nsISupports*     aExtraArgument,
                         nsIDOMWindow**   _retval);
  nsresult
  sbPrompter::CancelImpl();

  nsresult
  sbPrompter::AlertImpl(nsIDOMWindow*    aParent,
                    const PRUnichar* aDialogTitle,
                    const PRUnichar* aText);

  nsresult
  sbPrompter::AlertCheckImpl(nsIDOMWindow*    aParent,
                         const PRUnichar* aDialogTitle,
                         const PRUnichar* aText,
                         const PRUnichar* aCheckMsg,
                         bool*          aCheckState);

  nsresult
  sbPrompter::ConfirmImpl(nsIDOMWindow*    aParent,
                      const PRUnichar* aDialogTitle,
                      const PRUnichar* aText,
                      bool*          _retval);

  nsresult
  sbPrompter::ConfirmCheckImpl(nsIDOMWindow*    aParent,
                           const PRUnichar* aDialogTitle,
                           const PRUnichar* aText,
                           const PRUnichar* aCheckMsg,
                           bool*          aCheckState,
                           bool*          _retval);
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
                        PRInt32*         _retval);

  nsresult
  sbPrompter::PromptImpl(nsIDOMWindow*    aParent,
                     const PRUnichar* aDialogTitle,
                     const PRUnichar* aText,
                     PRUnichar**      aValue,
                     const PRUnichar* aCheckMsg,
                     bool*          aCheckState,
                     bool*          _retval);

  nsresult
  sbPrompter::PromptUsernameAndPasswordImpl(nsIDOMWindow*    aParent,
                                        const PRUnichar* aDialogTitle,
                                        const PRUnichar* aText,
                                        PRUnichar**      aUsername,
                                        PRUnichar**      aPassword,
                                        const PRUnichar* aCheckMsg,
                                        bool*          aCheckState,
                                        bool*          _retval);

  nsresult
  sbPrompter::PromptPasswordImpl(nsIDOMWindow*    aParent,
                             const PRUnichar* aDialogTitle,
                             const PRUnichar* aText,
                             PRUnichar**      aPassword,
                             const PRUnichar* aCheckMsg,
                             bool*          aCheckState,
                             bool*          _retval);

  nsresult
  sbPrompter::SelectImpl(nsIDOMWindow*     aParent,
                     const PRUnichar*  aDialogTitle,
                     const PRUnichar*  aText,
                     PRUint32          aCount,
                     const PRUnichar** aSelectList,
                     PRInt32*          aOutSelection,
                     bool*           _retval);

};


//
// Songbird prompter component defs.
//

// contract ID defined in sbIPrompter.idl
#define SONGBIRD_PROMPTER_CLASSNAME "Songbird Prompter"
#define SONGBIRD_PROMPTER_CID \
  /* {fa7ec5bd-7cab-4a63-a970-7bc4e83ee891} */ \
  { \
    0xfa7ec5bd, \
    0x7cab, \
    0x4a63, \
    { 0xa9, 0x70, 0x7b, 0xc4, 0xe8, 0x3e, 0xe8, 0x91 } \
  }


#endif // __SB_PROMPTER_H__

