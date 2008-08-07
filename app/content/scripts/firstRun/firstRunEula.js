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
 * \file  firstRunEula.js
 * \brief Javascript source for the first-run wizard EULA widget services.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// First-run wizard EULA widget services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// First-run wizard EULA widget imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
Components.utils.import("resource://app/jsmodules/DOMUtils.jsm");


//------------------------------------------------------------------------------
//
// First-run wizard EULA widget services defs.
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
// First-run wizard EULA widget services.
//
//------------------------------------------------------------------------------

/**
 * Construct a first-run wizard EULA widget services object for the widget
 * specified by aWidget.
 *
 * \param aWidget               First-run wizard EULA widget.
 */

function firstRunEULASvc(aWidget) {
  this._widget = aWidget;
}

// Define the object.
firstRunEULASvc.prototype = {
  // Set the constructor.
  constructor: firstRunEULASvc,

  //
  // Widget services fields.
  //
  //   _widget                  First-run wizard EULA widget.
  //   _domEventListenerSet     Set of DOM event listeners.
  //   _wizardElem              First-run wizard element.
  //   _wizardPageElem          First-run wizard EULA widget wizard page element.
  //

  _widget: null,
  _domEventListenerSet: null,
  _wizardElem: null,
  _wizardPageElem: null,


  //----------------------------------------------------------------------------
  //
  // Widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the widget services.
   */

  initialize: function firstRunEULASvc_initialize() {
    var _this = this;
    var func;

    // Create a DOM event listener set.
    this._domEventListenerSet = new DOMEventListenerSet();

    // Get the first-run wizard and wizard page elements.
    this._wizardPageElem = this._widget.parentNode;
    this._wizardElem = this._wizardPageElem.parentNode;

    // Listen for page show and advanced events.
    func = function() { _this._doPageShow(); };
    this._domEventListenerSet.add(this._wizardPageElem,
                                  "pageshow",
                                  func,
                                  false);
    func = function() { _this._doPageAdvanced(); };
    this._domEventListenerSet.add(this._wizardPageElem,
                                  "pageadvanced",
                                  func,
                                  false);
    try
    {
      /**
       * 
       * Start and stop so we get a timestamp for when the EULA is displayed
       * We only care about the start time to calc from app start to EULA
       */
      var timingService = Cc["@songbirdnest.com/Songbird/TimingService;1"]
                          .getService(Ci.sbITimingService);
      timingService.startPerfTimer("CSPerfEULA");
      timingService.stopPerfTimer("CSPerfEULA");
    }
    catch (e)
    {
      // Ignore errors
    }
  },


  /**
   * Finalize the widget services.
   */

  finalize: function firstRunEULASvc_finalize() {
    // Remove DOM event listeners.
    if (this._domEventListenerSet) {
      this._domEventListenerSet.removeAll();
    }
    this._domEventListenerSet = null;

    // Clear object fields.
    this._widget = null;
    this._wizardElem = null;
    this._wizardPageElem = null;
    
  },


  //----------------------------------------------------------------------------
  //
  // Widget event handling services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle an accept EULA changed event.
   */

  doAcceptChanged: function firstRunEULASvc_doAcceptChanged() {
    // Update the UI.
    this._update();
  },


  /**
   * Handle a wizard page show event.
   */

  _doPageShow: function firstRunEULASvc__doPageShow() {
    // Update the UI.
    this._update();
  },


  /**
   * Handle a wizard page advanced event.
   */

  _doPageAdvanced: function firstRunEULASvc__doPageAdvanced() {
    // Don't advance if accept EULA checkbox is not checked.
    var acceptCheckboxElem = this._getElement("accept_checkbox");
    if (!acceptCheckboxElem.checked)
      return false;

    // Set the EULA accepted preference and flush to disk.
    Application.prefs.setValue("songbird.eulacheck", true);
    var prefService = Cc["@mozilla.org/preferences-service;1"]
                        .getService(Ci.nsIPrefService);
    prefService.savePrefFile(null);


    // Allow advancement.
    return true;
  },


  //----------------------------------------------------------------------------
  //
  // Internal widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Update the UI.
   */

  _update: function firstRunEULASvc__update() {
    // Do nothing if wizard page is not being shown.
    if (this._wizardElem.currentPage != this._wizardPageElem)
      return;

    // If the EULA has already been accepted, skip the first-run wizard EULA
    // page.
    var eulaAccepted = Application.prefs.getValue("songbird.eulacheck", false);
    if (eulaAccepted) {
      this._wizardElem.canAdvance = true;
      this._wizardElem.advance();
      return;
    }

    // Only allow the first-run wizard to advance if the accept EULA checkbox is
    // checked.
    var acceptCheckboxElem = this._getElement("accept_checkbox");
    this._wizardElem.canAdvance = acceptCheckboxElem.checked;
  },


  /**
   * \brief Return the XUL element with the ID specified by aElementID.  Use the
   *        element "anonid" attribute as the ID.
   *
   * \param aElementID          ID of element to get.
   *
   * \return Element.
   */

  _getElement: function firstRunEULASvc__getElement(aElementID) {
    return document.getAnonymousElementByAttribute(this._widget,
                                                   "anonid",
                                                   aElementID);
  }
};

