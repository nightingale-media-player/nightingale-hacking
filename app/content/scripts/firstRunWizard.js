/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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

/**
* \file  firstRunWizard.js
* \brief Javascript source for the first-run wizard dialog.
*/

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// First-run wizard dialog.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// First-run wizard dialog defs.
//
//------------------------------------------------------------------------------

// Component manager defs.
if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;


//------------------------------------------------------------------------------
//
// First-run wizard dialog services.
//
//------------------------------------------------------------------------------

var firstRunWizard = {
  //
  // First-run wizard fields.
  //
  //   _restartWizard           True if the wizard needs to be restarted.
  //

  _restartWizard: false,


  //----------------------------------------------------------------------------
  //
  // Event handling services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle an unload event.
   */

  doUnload: function firstRunWizard_doUnload() {
    // Indicate that the wizard is complete and whether it should be restarted.
    window.arguments[0].onComplete(this._restartWizard);
  },


  /**
   * Handle a wizard finish event.
   */

  doFinish: function firstRunWizard_doFinish() {
    // Indicate that the wizard is complete and should not be restarted.
    window.arguments[0].onComplete(false);
  },


  /**
   * Handle a wizard cancel event.
   */

  doCancel: function firstRunWizard_doCancel() {
    // Indicate that the wizard is complete and should not be restarted.
    window.arguments[0].onComplete(false);
  },


  /**
   * Handle a wizard page show event.
   */

  doPageShow: function firstRunWizard_doPageShow() {
    // Update the UI.
    this._update();
  },


  /**
   * Handle a EULA page accept changed event.
   */

  doEULAAcceptChanged: function firstRunWizard_doEULAAcceptChanged() {
    // Update the UI.
    this._update();
  },


  /**
   * Handle the key press event specified by aEvent.
   *
   * \param aEvent              Key press event.
   */

  doKeyPress: function firstRunWizard_doKeyPress(aEvent) {
    // Get the pressed key code.
    var keyCode = aEvent.keyCode;

    // Get the current page.
    var wizardElem = document.getElementById("first_run_wizard");
    var currentPage = wizardElem.currentPage;

    // If the current page is the EULA page, don't allow the wizard to process
    // navigation key presses.
    if ((currentPage.id == "first_run_eula_page") &&
        ((keyCode == Ci.nsIDOMKeyEvent.DOM_VK_ESCAPE) ||
         (keyCode == Ci.nsIDOMKeyEvent.DOM_VK_ENTER) ||
         (keyCode == Ci.nsIDOMKeyEvent.DOM_VK_RETURN))) {
      aEvent.stopPropagation();
      aEvent.preventDefault();
    }
  },


  /**
   * Handle the page hide event specified by aEvent.
   *
   * \param aEvent              Page hide event.
   */

  doPageHide: function firstRunWizard_doPageHide(aEvent) {
    hiddenPage = aEvent.target;
    switch (hiddenPage.id) {
      case "first_run_locale" :
        // Cancel any locale switch in progress.
        hiddenPage.cancelSwitchLocale();
        break;

      default :
        break;
    }
  },


  /**
   * Handle the locale switch complete event.
   */

  doLocaleSwitchComplete:
    function firstRunWizard_doLocaleSwitchComplete() {
    // Get the first run locale element.
    var firstRunLocaleElem = document.getElementById("first_run_locale");

    // If the locale switch succeeded, restart.  Otherwise, advance through the
    // wizard without switching locales.
    if (firstRunLocaleElem.localeSwitchSucceeded) {
      if (firstRunLocaleElem.appRestartRequired) {
        restartApp();
      } else {
        this._restartWizard = true;
        document.defaultView.close();
      }
    } else {
      var wizardElem = document.getElementById("first_run_wizard");
      wizardElem.canAdvance = true;
      wizardElem.advance();
    }
  },


  //----------------------------------------------------------------------------
  //
  // Internal services.
  //
  //----------------------------------------------------------------------------

  /*
   * Update the UI.
   */

  _update: function firstRunWizard__update() {
    // Get the current wizard page.
    var wizardElem = document.getElementById("first_run_wizard");
    var currentPage = wizardElem.currentPage;

    // Hide or show the back button.
    var backButton = wizardElem.getButton("back");
    if (currentPage.getAttribute("hideback") == "true")
      backButton.hidden = true;
    else
      backButton.hidden = false;

    // Hide or show the cancel button.
    var cancelButton = wizardElem.getButton("cancel");
    if (currentPage.getAttribute("hidecancel") == "true")
      cancelButton.hidden = true;
    else
      cancelButton.hidden = false;

    // Disable the continue button on the EULA page while EULA is not accepted.
    if (currentPage.id == "first_run_eula_page") {
      var firstRunEULAElem = document.getElementById("first_run_eula");
      wizardElem.canAdvance = firstRunEULAElem.accepted;
    }

    // If showing the first-run locale page and a locale switch is required,
    // switch the locale.  Otherwise, skip the first-run locale page.
    if (currentPage.id == "first_run_locale_page") {
      var firstRunLocaleElem = document.getElementById("first_run_locale");
      if (firstRunLocaleElem.localeSwitchRequired) {
        // Switch the locale, but don't allow advancing.
        wizardElem.canAdvance = false;
        firstRunLocaleElem.switchLocale();
      } else {
        wizardElem.canAdvance = true;
        wizardElem.advance();
      }
    }

    // Focus the next or finish buttons on all but the EULA page.
    if (currentPage.id != "first_run_eula_page") {
      // Focus the finish button if it's not hidden.  Otherwise, focus the next
      // button.
      var finishButton = wizardElem.getButton("finish");
      var nextButton = wizardElem.getButton("next");
      if (!finishButton.hidden)
        finishButton.focus();
      else if (!nextButton.hidden)
        nextButton.focus();
    }
  }
};

