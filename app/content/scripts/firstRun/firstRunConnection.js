/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
 */

/**
 * \file  firstRunConnection.js
 * \brief Javascript source for the first-run wizard connection widget services.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// First-run wizard connection widget services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// First-run wizard EULA widget imported services.
//
//------------------------------------------------------------------------------

// Nightingale imports.
Components.utils.import("resource://app/jsmodules/DOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/WindowUtils.jsm");


//------------------------------------------------------------------------------
//
// First-run wizard connection widget services.
//
//------------------------------------------------------------------------------

/**
 * Construct a first-run wizard connection widget services object for the widget
 * specified by aWidget.
 *
 * \param aWidget               First-run wizard connection widget.
 */

function firstRunConnectionSvc(aWidget) {
  this._widget = aWidget;
}

// Define the object.
firstRunConnectionSvc.prototype = {
  // Set the constructor.
  constructor: firstRunConnectionSvc,

  //
  // Widget services fields.
  //
  //   _widget                  First-run wizard connection widget.
  //   _domEventListenerSet     Set of DOM event listeners.
  //   _wizardPageElem          First-run wizard EULA widget wizard page element.
  //

  _widget: null,
  _domEventListenerSet: null,
  _wizardPageElem: null,


  //----------------------------------------------------------------------------
  //
  // Widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the widget services.
   */

  initialize: function firstRunConnectionSvc_initialize() {
    // Get the first-run wizard page element.
    this._wizardPageElem = this._widget.parentNode;

    // Listen for page advanced events.
    this._domEventListenerSet = new DOMEventListenerSet();
    var _this = this;
    var func = function(aEvent) { return _this._doPageAdvanced(aEvent); };
    this._domEventListenerSet.add(this._wizardPageElem,
                                  "pageadvanced",
                                  func,
                                  false);
  },


  /**
   * Finalize the widget services.
   */

  finalize: function firstRunConnectionSvc_finalize() {
    // Remove DOM event listeners.
    if (this._domEventListenerSet) {
      this._domEventListenerSet.removeAll();
    }
    this._domEventListenerSet = null;

    // Clear object fields.
    this._widget = null;
    this._wizardPageElem = null;
  },


  //----------------------------------------------------------------------------
  //
  // Widget event handling services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle the command event specified by aEvent.
   *
   * \param aEvent             Command event.
   */

  doCommand: function firstRunConnectionSvc_doCommand(aEvent) {
    // Dispatch processing of the command.
    var action = aEvent.target.getAttribute("action");
    switch (action) {
      case "doConnectionSettings" :
        this._doConnectionSettings();
        break;

      default :
        break;
    }
  },


  /**
   * Handle the wizard page advanced event specified by aEvent.
   *
   * \param aEvent              Page advanced event.
   */

  _doPageAdvanced: function firstRunConnectionSvc__doPageAdvanced(aEvent) {
    // Rewind back to the wizard page that encountered a connection error.
    // Advance is enabled so that the wizard next button is displayed, but
    // rewind is used so that the wizard connection page isn't in the wizard
    // page history.
    firstRunWizard.wizardElem.rewind();

    // Prevent advancing.
    aEvent.preventDefault();
    return false;
  },


  //----------------------------------------------------------------------------
  //
  // Internal widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Present the network connection settings dialog.  If it's accepted, advance
   * out of the first-run connection page.
   */

  _doConnectionSettings:
    function firstRunConnectionSvc__doConnectionSettings() {
    // Get the preference services.
    var prefService = Cc["@mozilla.org/preferences-service;1"]
                        .getService(Ci.nsIPrefService);
    prefService = prefService.QueryInterface(Ci.nsIPrefBranch);

    // Switch instant apply to true.
    // It doesnt actually make it apply the settings instantly unless you're on
    // a Mac, but it makes clicking 'ok' apply the changes on all platforms
    // (because the code for the prefwindow has no provision for child
    // prefwindows running standalone when they are not instantApply).
    var prevInstantApply =
          prefService.getBoolPref("browser.preferences.instantApply");
    prefService.setBoolPref("browser.preferences.instantApply", true);

    // Open the connection settings dialog.
    var accepted = WindowUtils.openModalDialog
                     (window,
                      "chrome://browser/content/preferences/connection.xul",
                      "Connections",
                      "chrome,modal=yes,centerscreen",
                      null,
                      null);

    // Switch back to previous instant apply.
    prefService.setBoolPref("browser.preferences.instantApply",
                            prevInstantApply);

    // Flush settings to disk.
    prefService.savePrefFile(null);

    // Send connection reset event and advance out of first-run connection page
    // if the connection settings were accepted.
    if (accepted) {
      // Send a connection reset event.
      var event = document.createEvent("Events");
      event.initEvent("firstRunConnectionReset", true, true);
      this._widget.dispatchEvent(event);

      // Advance out of first-run connection page.
      firstRunWizard.wizardElem.advance();
    }
  }
}

