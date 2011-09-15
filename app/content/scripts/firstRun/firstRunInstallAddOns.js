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
 * \file  firstRunInstallAddOns.js
 * \brief Javascript source for the first-run wizard install add-ons widget
 *        services.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// First-run wizard install add-ons widget services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// First-run wizard install add-ons widget imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/DOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");


//------------------------------------------------------------------------------
//
// First-run wizard install add-ons widget services defs.
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
// First-run wizard install add-ons widget services.
//
//------------------------------------------------------------------------------

/**
 * Construct a first-run wizard install add-ons widget services object for the
 * widget specified by aWidget.
 *
 * \param aWidget               First-run wizard install add-ons widget.
 */

function firstRunInstallAddOnsSvc(aWidget) {
  this._widget = aWidget;
}

// Define the object.
firstRunInstallAddOnsSvc.prototype = {
  // Set the constructor.
  constructor: firstRunInstallAddOnsSvc,

  //
  // Widget services fields.
  //
  //   _widget                  First-run wizard install add-ons widget.
  //   _domEventListenerSet     Set of DOM event listeners.
  //   _wizardElem              First-run wizard element.
  //   _wizardPageElem          First-run install add-ons wizard page element.
  //   _addOnBundleInstallerElem
  //                            Add-on bundle installer element.
  //

  _widget: null,
  _domEventListenerSet: null,
  _wizardElem: null,
  _wizardPageElem: null,
  _addOnBundleInstallerElem: null,


  //----------------------------------------------------------------------------
  //
  // Widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the widget services.
   */

  initialize: function firstRunInstallAddOnsSvc_initialize() {
    var _this = this;
    var func;

    // Create a DOM event listener set.
    this._domEventListenerSet = new DOMEventListenerSet();

    // Get the first-run wizard and wizard page elements.
    this._wizardPageElem = this._widget.parentNode;
    this._wizardElem = this._wizardPageElem.parentNode;

    // Get the add-on bundle installer element.
    this._addOnBundleInstallerElem =
      this._getElement("add_on_bundle_installer");

    // Listen for page show and hide events.
    func = function() { _this._doPageShow(); };
    this._domEventListenerSet.add(this._wizardPageElem,
                                  "pageshow",
                                  func,
                                  false);
    func = function() { _this._doPageHide(); };
    this._domEventListenerSet.add(this._wizardPageElem,
                                  "pagehide",
                                  func,
                                  false);

    // Listen for add-on bundle installer completion events.
    func = function(aEvent) { return _this._doInstallComplete(aEvent); };
    this._domEventListenerSet.add(this._addOnBundleInstallerElem,
                                  "complete",
                                  func,
                                  false);
  },


  /**
   * Finalize the widget services.
   */

  finalize: function firstRunInstallAddOnsSvc_finalize() {
    // Remove DOM event listeners.
    if (this._domEventListenerSet) {
      this._domEventListenerSet.removeAll();
    }
    this._domEventListenerSet = null;

    // Cancel any add-on download in progress.
    if (this._addOnBundleInstallerElem)
      this._addOnBundleInstallerElem.cancel();

    // Clear object fields.
    this._widget = null;
    this._wizardElem = null;
    this._addOnBundleInstallerElem = null;
  },


  //----------------------------------------------------------------------------
  //
  // Widget event handling services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle a wizard page show event.
   */

  _doPageShow: function firstRunInstallAddOnsSvc__doPageShow() {
    // Get the add-on bundle object.
    var addOnsID = this._widget.getAttribute("addonsid");
    var addOnBundleProperty =
          this._widget.getAttribute("addonbundleproperty");
    var addOnsElem = document.getElementById(addOnsID);
    var addOnBundle = addOnsElem[addOnBundleProperty];

    // If an add-on bundle is available, start add-on bundle installation.
    // Otherwise, advance the wizard.
    if (addOnBundle) {
      this._addOnBundleInstallerElem.install(addOnBundle);
    } else {
      this._wizardElem.canAdvance = true;
      this._wizardElem.advance();
    }
  },


  /**
   * Handle a wizard page hide event.
   */

  _doPageHide: function firstRunInstallAddOnsSvc__doPageHide() {
    // Cancel any add-on installation in progress.
    this._addOnBundleInstallerElem.cancel();
  },


  /**
   * Handle the add-on install complete event specified by aEvent.
   *
   * \param aEvent              Add-on install complete event.
   */

  _doInstallComplete:
    function firstRunInstallAddOnsSvc__doInstallComplete(aEvent) {
    // Mark first-run wizard for application restart if required.
    if (this._addOnBundleInstallerElem.restartRequired)
      firstRunWizard.restartApp = true;

    // If installation completed successfully, advance wizard.  Otherwise, allow
    // user to view errors.
    if (this._addOnBundleInstallerElem.errorCount == 0) {
      // Advance wizard.
      this._wizardElem.canAdvance = true;
      this._wizardElem.advance();
    } else {
      // Change the next button to an OK button.
      var okButton = this._wizardElem.getButton("next");
      okButton.label = SBString("first_run.ok.label");
      okButton.accessKey = SBString("first_run.ok.accesskey");

      // Hide the cancel button and show the OK button.
      this._wizardPageElem.setAttribute("hidecancel", "true");
      this._wizardPageElem.setAttribute("shownext", "true");
    }
  },


  //----------------------------------------------------------------------------
  //
  // Internal widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Return the XUL element with the ID specified by aElementID.  Use the
   *        element "anonid" attribute as the ID.
   *
   * \param aElementID          ID of element to get.
   *
   * \return Element.
   */

  _getElement: function firstRunInstallAddOnsSvc__getElement(aElementID) {
    return document.getAnonymousElementByAttribute(this._widget,
                                                   "anonid",
                                                   aElementID);
  }
}

