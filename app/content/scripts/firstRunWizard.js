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
// First-run wizard dialog imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
Components.utils.import("resource://app/jsmodules/DOMUtils.jsm");


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
  //   _initialized             True if these services have been initialized.
  //   _wizardElem              Wizard element.
  //   _domEventListenerSet     Set of DOM event listeners.
  //   _restartWizard           True if the wizard needs to be restarted.
  //   _savedSettings           True if settings have been saved.
  //   _postFinish              True if wizard is in the post-finish pages.
  //   _markFirstRunComplete    True if first-run should be marked as complete
  //                            when wizard exits.
  //

  _initialized: false,
  _wizardElem: null,
  _domEventListenerSet: null,
  _restartWizard: false,
  _savedSettings: false,
  _postFinish: false,
  _markFirstRunComplete: false,


  //----------------------------------------------------------------------------
  //
  // Event handling services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle a load event.
   */

  doLoad: function firstRunWizard_doLoad() {
    // Initialize the services.
    this._initialize();
  },


  /**
   * Handle an unload event.
   */

  doUnload: function firstRunWizard_doUnload() {
    // Indicate that the first-run checks have been made.
    if (this._markFirstRunComplete)
      Application.prefs.setValue("songbird.firstrun.check.0.3", true);

    // Restart application as specified.
    if (this._wizardElem.getAttribute("restartapp") == "true")
      restartApp();

    // Indicate that the wizard is complete and whether it should be restarted.
    window.arguments[0].onComplete(this._restartWizard);

    // Finalize the services.
    this._finalize();
  },


  /**
   * Handle a wizard finish event.
   */

  doFinish: function firstRunWizard_doFinish() {
    // Save wizard settings.
    this._saveSettings();

    // Advance to post-finish pages if specified.  Return false to prevent
    // finish.
    var currentPage = this._wizardElem.currentPage;
    if (currentPage.hasAttribute("postfinish")) {
      // Indicate post-finish state before switching pages.
      this._postFinish = true;

      // Switch to post-finish pages.
      var postFinishPageID = currentPage.getAttribute("postfinish");
      this._wizardElem.goTo(postFinishPageID);

      // Cancel finish.
      return false;
    }
  },


  /**
   * Handle a wizard cancel event.
   */

  doCancel: function firstRunWizard_doCancel() {
  },


  /**
   * Handle a wizard quit event.
   */

  doQuit: function firstRunWizard_doQuit() {
    // Quit application.
    quitApp();
  },


  /**
   * Handle a wizard page show event.
   */

  doPageShow: function firstRunWizard_doPageShow() {
    // Initialize the services.  Page show can occur before load.
    this._initialize();

    // Update the UI.
    this.update();
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
    var currentPage = this._wizardElem.currentPage;

    // If the cancel button is disabled or hidden, block the escape key.
    if (keyCode == Ci.nsIDOMKeyEvent.DOM_VK_ESCAPE) {
      var cancelButton = this._wizardElem.getButton("cancel");
      var hideWizardButton =
            cancelButton.getAttribute("hidewizardbutton") == "true";
      if (cancelButton.disabled || cancelButton.hidden || hideWizardButton) {
        aEvent.stopPropagation();
        aEvent.preventDefault();
      }
    }

    // If the next and finished buttons are disabled or hidden, block the enter
    // and return keys.
    if ((keyCode == Ci.nsIDOMKeyEvent.DOM_VK_ENTER) ||
        (keyCode == Ci.nsIDOMKeyEvent.DOM_VK_RETURN)) {
      var nextButton = this._wizardElem.getButton("next");
      var hideNextButton =
            nextButton.getAttribute("hidewizardbutton") == "true";
      var finishButton = this._wizardElem.getButton("finish");
      var hideFinishButton =
            finishButton.getAttribute("hidewizardbutton") == "true";
      if ((nextButton.disabled || nextButton.hidden || hideNextButton) &&
          (finishButton.disabled || finishButton.hidden || hideFinishButton)) {
        aEvent.stopPropagation();
        aEvent.preventDefault();
      }
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
    //XXXeps should restart like the first-run add-ons installation does
    if (firstRunLocaleElem.localeSwitchSucceeded) {
      if (firstRunLocaleElem.appRestartRequired) {
        restartApp();
      } else {
        this._restartWizard = true;
        document.defaultView.close();
      }
    } else {
      this._wizardElem.canAdvance = true;
      this._wizardElem.advance();
    }
  },


  //----------------------------------------------------------------------------
  //
  // UI services.
  //
  //----------------------------------------------------------------------------

  /**
   * Update the UI.
   */

  update: function firstRunWizard_update() {
    // Get the current wizard page.
    var currentPage = this._wizardElem.currentPage;

    // Set to mark first-run complete if showing welcome page.
    if (currentPage.id == "first_run_welcome_page")
      this._markFirstRunComplete = true;

    // Update the buttons.
    this._updateButtons();

    // If showing the first-run locale page and a locale switch is required,
    // switch the locale.  Otherwise, skip the first-run locale page.
    if (currentPage.id == "first_run_locale_page") {
      var firstRunLocaleElem = document.getElementById("first_run_locale");
      if (firstRunLocaleElem.localeSwitchRequired) {
        // Switch the locale, but don't allow advancing.
        this._wizardElem.canAdvance = false;
        firstRunLocaleElem.switchLocale();
      } else {
        this._wizardElem.canAdvance = true;
        this._wizardElem.advance();
      }
    }

    // Focus the next or finish buttons unless they're disabled or hidden.
    var finishButton = this._wizardElem.getButton("finish");
    var hideFinishButton =
          finishButton.getAttribute("hidewizardbutton") == "true";
    var nextButton = this._wizardElem.getButton("next");
    var hideNextButton =
          nextButton.getAttribute("hidewizardbutton") == "true";
    if (!finishButton.hidden && !finishButton.disabled && !hideFinishButton)
      finishButton.focus();
    else if (!nextButton.hidden && !nextButton.disbled && !hideNextButton)
      nextButton.focus();
  },


  //----------------------------------------------------------------------------
  //
  // Internal services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the first-run wizard dialog services.
   */

  _initialize: function firstRunWizard__initialize() {
    // Do nothing if already initialized.
    if (this._initialized)
      return;

    // Get the wizard element.
    this._wizardElem = document.getElementById("first_run_wizard");

    // Create a DOM event listener set.
    this._domEventListenerSet = new DOMEventListenerSet();

    // Listen for quit button events.  These don't bubble to attribute based
    // handlers.
    var _this = this;
    var func = function(aEvent) { return _this.doQuit(aEvent); }
    this._domEventListenerSet.add(this._wizardElem, "extra1", func, false);

    // Services are now initialized.
    this._initialized = true;
  },


  /**
   * Finalize the first-run wizard dialog services.
   */

  _finalize: function firstRunWizard__finalize() {
    // Remove DOM event listeners.
    if (this._domEventListenerSet)
      this._domEventListenerSet.removeAll();
    this._domEventListenerSet = null;
  },


  /**
   * Update the wizard buttons.
   */

  _updateButtons: function firstRunWizard__updateButtons() {
    // Get the current wizard page.
    var currentPage = this._wizardElem.currentPage;

    // Get the button hide and show settings.
    var hideBackButton = currentPage.getAttribute("hideback") == "true";
    var hideCancelButton = currentPage.getAttribute("hidecancel") == "true";
    var hideNextButton = currentPage.getAttribute("hidenext") == "true";
    var hideFinishButton = currentPage.getAttribute("hidefinish") == "true";
    var showQuitButton = currentPage.getAttribute("showquit") == "true";

    // Always hide navigation buttons on post-finish pages.
    if (this._postFinish) {
      hideBackButton = true;
      hideNextButton = true;
      hideFinishButton = true;
    }

    // Update the buttons.
    this._setHideButton("back", hideBackButton);
    this._setHideButton("cancel", hideCancelButton);
    this._setHideButton("next", hideNextButton);
    this._setHideButton("finish", hideFinishButton);
    this._setShowButton("extra1", showQuitButton);

    // Set the quit button label.
    var quitButton = this._wizardElem.getButton("extra1");
    quitButton.label = SBString("first_run.quit");
  },


  /**
   * Set the wizard button specified by aButtonID to be hidden as specified by
   * aHide.
   *
   * \param aButtonID           ID of button to set hidden.
   * \param aHide               If true, button should be hidden.
   */

  _setHideButton: function firstRunWizard__setHideButton(aButtonID, aHide) {
    // Hide the button if specified to do so.  Use a "hidewizardbutton"
    // attribute with CSS to avoid conflicts with the wizard widget's use of the
    // button "hidden" attribute.
    var button = this._wizardElem.getButton(aButtonID);
    if (aHide)
      button.setAttribute("hidewizardbutton", "true");
    else
      button.removeAttribute("hidewizardbutton");
  },


  /**
   * Set the wizard button specified by aButtonID to be shown as specified by
   * aShow.
   *
   * \param aButtonID           ID of button to set show.
   * \param aShow               If true, button should be shown.
   */

  _setShowButton: function firstRunWizard__setShowButton(aButtonID, aShow) {
    // Show button if specified to do so.
    var button = this._wizardElem.getButton(aButtonID);
    button.hidden = !aShow;
  },


  /**
   * Save settings from all wizard pages.
   */

  _saveSettings: function firstRunWizard__saveSettings() {
    // Do nothing if settings already saved.
    if (this._savedSettings)
      return;

    // Get all first-run wizard page elements.
    var firstRunWizardPageElemList =
          DOMUtils.getElementsByAttribute(this._wizardElem,
                                          "firstrunwizardpage",
                                          "true");

    // Save settings for each first-run wizard page.
    for (var i = 0; i < firstRunWizardPageElemList.length; i++) {
      // Invoke the page saveSettings method.
      var firstRunWizardPageElem = firstRunWizardPageElemList[i];
      if (typeof(firstRunWizardPageElem.saveSettings) == "function")
        firstRunWizardPageElem.saveSettings();
    }

    // Settings have now been saved.
    this._savedSettings = true;
  }
};

