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

// Songbird imports.
Components.utils.import("resource://app/jsmodules/DOMUtils.jsm");


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
    var func = function() { _this._doPageAdvanced(); };
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
   * Handle a wizard page advanced event.
   */

  _doPageAdvanced: function firstRunConnectionSvc__doPageAdvanced() {
    // Rewind back to the wizard page that encountered a connection error.
    // Advance is enabled so that the wizard next button is displayed, but
    // rewind is used so that the wizard connection page isn't in the wizard
    // page history.
    firstRunWizard.wizardElem.rewind();
    return false;
  }
}

