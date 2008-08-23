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
  // Public first-run wizard fields.
  //
  //   wizardElem               Wizard element.
  //   restartApp               Restart application on wizard close.
  //   restartWizard            Restart wizard on wizard close.
  //

  restartApp: false,
  restartWizard: false,


  //
  // Internal first-run wizard fields.
  //
  //   _initialized             True if these services have been initialized.
  //   _domEventListenerSet     Set of DOM event listeners.
  //   _savedSettings           True if settings have been saved.
  //   _postFinish              True if wizard is in the post-finish pages.
  //   _markFirstRunComplete    True if first-run should be marked as complete
  //                            when wizard exits.
  //   _connectionErrorHandled  True if a connection error has been handled.
  //

  _initialized: false,
  _domEventListenerSet: null,
  _savedSettings: false,
  _postFinish: false,
  _markFirstRunComplete: false,
  _connectionErrorHandled: false,


  //----------------------------------------------------------------------------
  //
  // Public first-run wizard field getters/setters.
  //
  //----------------------------------------------------------------------------

  //
  // wizardElem
  //

  _wizardElem: null,

  get wizardElem() {
    if (!this._wizardElem)
      this._wizardElem = document.getElementById("first_run_wizard");
    return this._wizardElem;
  },


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
    if (this._markFirstRunComplete) {
      // Set the first-run check preference and flush to disk.
      Application.prefs.setValue("songbird.firstrun.check.0.3", true);
      var prefService = Cc["@mozilla.org/preferences-service;1"]
                          .getService(Ci.nsIPrefService);
      prefService.savePrefFile(null);
    }

    // Indicate that the wizard is complete and whether it should be restarted.
    window.arguments[0].onComplete(this.restartWizard);

    // Finalize the services.
    this._finalize();

    // Restart application as specified. (AFTER we're done finalizing)
    if (this.restartApp)
      restartApp();
  },


  /**
   * Handle a wizard finish event.
   */

  doFinish: function firstRunWizard_doFinish() {
    // Record our time startup pref
    try {
        var timingService = Cc["@songbirdnest.com/Songbird/TimingService;1"]
                              .getService(Ci.sbITimingService);
        timingService.startPerfTimer("CSPerfEndEULA");
        timingService.stopPerfTimer("CSPerfEndEULA");
    } catch (e) {
        dump("Timing service exception: " + e);
        // Ignore errors
    }

    // Save wizard settings.
    this._saveSettings();

    // Advance to post-finish pages if specified.  Return false to prevent
    // finish.
    var currentPage = this.wizardElem.currentPage;
    if (currentPage.hasAttribute("postfinish")) {
      // Indicate post-finish state before switching pages.
      this._postFinish = true;

      // Switch to post-finish pages.
      var postFinishPageID = currentPage.getAttribute("postfinish");
      this.wizardElem.goTo(postFinishPageID);

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
    var currentPage = this.wizardElem.currentPage;

    // If the cancel button is disabled or hidden, block the escape key.
    if (keyCode == Ci.nsIDOMKeyEvent.DOM_VK_ESCAPE) {
      var cancelButton = this.wizardElem.getButton("cancel");
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
      var nextButton = this.wizardElem.getButton("next");
      var hideNextButton =
            nextButton.getAttribute("hidewizardbutton") == "true";
      var finishButton = this.wizardElem.getButton("finish");
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
   * Handle internet connection errors.
   *
   * \return                    True if error was handled.
   */

  handleConnectionError: function firstRunWizard_handleConnectionError() {
    // Only handle connection errors once.
    if (this._connectionErrorHandled)
      return false;

    // Go to the first-run wizard connection page.
    this.wizardElem.canAdvance = true;
    this.wizardElem.advance("first_run_connection_page");

    // A connection error has been handled.
    this._connectionErrorHandled = true;

    return true;
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
    var currentPage = this.wizardElem.currentPage;

    // Set to mark first-run complete if showing welcome page.
    if (currentPage.id == "first_run_welcome_page")
      this._markFirstRunComplete = true;

    // Update the buttons.
    this._updateButtons();

    // Focus the next or finish buttons unless they're disabled or hidden.
    var finishButton = this.wizardElem.getButton("finish");
    var hideFinishButton =
          finishButton.getAttribute("hidewizardbutton") == "true";
    var nextButton = this.wizardElem.getButton("next");
    var hideNextButton =
          nextButton.getAttribute("hidewizardbutton") == "true";
    if (!finishButton.hidden && !finishButton.disabled && !hideFinishButton)
      finishButton.focus();
    else if (!nextButton.hidden && !nextButton.disbled && !hideNextButton)
      nextButton.focus();
    // If we're perf testing then we want to just after the EULA is done
    if (window.arguments[0].perfTest && this._markFirstRunComplete) {
        finishButton.click();
    }
    
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
    this.wizardElem = document.getElementById("first_run_wizard");

    // Create a DOM event listener set.
    this._domEventListenerSet = new DOMEventListenerSet();

    // Listen for quit button events.  These don't bubble to attribute based
    // handlers.
    var _this = this;
    var func = function(aEvent) { return _this.doQuit(aEvent); }
    this._domEventListenerSet.add(this.wizardElem, "extra1", func, false);

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
    var currentPage = this.wizardElem.currentPage;

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
    var quitButton = this.wizardElem.getButton("extra1");
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
    var button = this.wizardElem.getButton(aButtonID);
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
    var button = this.wizardElem.getButton(aButtonID);
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
          DOMUtils.getElementsByAttribute(this.wizardElem,
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

