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
 * \file  wizardService.js
 * \brief Javascript source for the Songbird wizard widget services.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird wizard widget services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Songbird wizard widget imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
Components.utils.import("resource://app/jsmodules/DOMUtils.jsm");


//------------------------------------------------------------------------------
//
// Songbird wizard widget services defs.
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
//------------------------------------------------------------------------------
//
// Songbird wizard widget services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * Construct a Songbird wizard widget services object for the widget specified
 * by aWidget.
 *
 * \param aWidget               Songbird wizard widget.
 */

function sbWizardSvc(aWidget) {
  this._widget = aWidget;
}

// Define the object.
sbWizardSvc.prototype = {
  // Set the constructor.
  constructor: sbWizardSvc,

  //
  // Internal widget services fields.
  //
  //   _widget                  Songbird wizard page widget.
  //   _domEventListenerSet     Set of DOM event listeners.
  //

  _widget: null,
  _domEventListenerSet: null,


  //----------------------------------------------------------------------------
  //
  // Widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the widget services.
   */

  initialize: function sbWizardSvc_initialize() {
    var _this = this;
    var func;

    // Create a DOM event listener set.
    this._domEventListenerSet = new DOMEventListenerSet();

    // Listen for wizard events.
    func = function(aEvent) { return _this._doKeyPress(aEvent); };
    this._domEventListenerSet.add(this._widget, "keypress", func, false);
    func = function(aEvent) { return _this._doFinish(aEvent); };
    this._domEventListenerSet.add(this._widget, "wizardfinish", func, false);
  },


  /**
   * Finalize the widget services.
   */

  finalize: function sbWizardSvc_finalize() {
    // Remove DOM event listeners.
    if (this._domEventListenerSet) {
      this._domEventListenerSet.removeAll();
    }
    this._domEventListenerSet = null;

    // Clear object fields.
    this._widget = null;
  },


  //----------------------------------------------------------------------------
  //
  // Widget event handling services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle the key press event specified by aEvent.
   *
   * \param aEvent              Key press event.
   */

  _doKeyPress: function sbWizardSvc__doKeyPress(aEvent) {
    // Get the pressed key code.
    var keyCode = aEvent.keyCode;

    // Get the current page.
    var currentPage = this._widget.currentPage;

    // If the cancel button is disabled or hidden, block the escape key.
    if (keyCode == Ci.nsIDOMKeyEvent.DOM_VK_ESCAPE) {
      var cancelButton = this._widget.getButton("cancel");
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
      var nextButton = this._widget.getButton("next");
      var hideNextButton =
            nextButton.getAttribute("hidewizardbutton") == "true";
      var finishButton = this._widget.getButton("finish");
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
   * Handle the wizard finish event specified by aEvent.
   *
   * \param aEvent              Wizard finish event.
   */

  _doFinish: function sbWizardSvc__doFinish(aEvent) {
    // Advance to post-finish pages if specified.  Return false to prevent
    // finish.
    var currentPage = this._widget.currentPage;
    if (currentPage.hasAttribute("postfinish")) {
      // Indicate post-finish state before switching pages.
      this._widget._postFinish = true;

      // Switch to post-finish pages.
      var postFinishPageID = currentPage.getAttribute("postfinish");
      this._widget.goTo(postFinishPageID);

      // Cancel finish.
      aEvent.preventDefault();
      return false;
    }

    return true;
  }
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird wizard page widget services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * Construct a Songbird wizard page widget services object for the widget
 * specified by aWidget.
 *
 * \param aWidget               Songbird wizard page widget.
 */

function sbWizardPageSvc(aWidget) {
  this._widget = aWidget;
}

// Define the object.
sbWizardPageSvc.prototype = {
  // Set the constructor.
  constructor: sbWizardPageSvc,

  //
  // Internal widget services fields.
  //
  //   _widget                  Songbird wizard page widget.
  //   _domEventListenerSet     Set of DOM event listeners.
  //   _wizardElem              Wizard page wizard element.
  //

  _widget: null,
  _domEventListenerSet: null,
  _wizardElem: null,


  //----------------------------------------------------------------------------
  //
  // Widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the widget services.
   */

  initialize: function sbWizardPageSvc_initialize() {
    var _this = this;
    var func;

    // Create a DOM event listener set.
    this._domEventListenerSet = new DOMEventListenerSet();

    // Get the parent wizard page element.
    this._wizardElem = this._widget.parentNode;

    // Listen for page show events.
    func = function() { return _this._doPageShow(); };
    this._domEventListenerSet.add(this._widget, "pageshow", func, false);

    // Listen for button related attribute modified events.
    func = function(aEvent) { return _this._doButtonDOMAttrModified(aEvent); };
    var buttonAttributesElem = this._getElement("button_attributes");
    this._domEventListenerSet.add(buttonAttributesElem,
                                  "DOMAttrModified",
                                  func,
                                  false);
  },


  /**
   * Finalize the widget services.
   */

  finalize: function sbWizardPageSvc_finalize() {
    // Remove DOM event listeners.
    if (this._domEventListenerSet) {
      this._domEventListenerSet.removeAll();
    }
    this._domEventListenerSet = null;

    // Clear object fields.
    this._widget = null;
    this._wizardElem = null;
  },


  //----------------------------------------------------------------------------
  //
  // Widget event handling services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle a wizard page show event.
   */

  _doPageShow: function sbWizardPageSvc__doPageShow() {
    // Update the wizard buttons.
    this._updateButtons();
  },


  /**
   * Handle the button DOM attribute modified event specified by aEvent.
   *
   * \param aEvent              DOM attribute modified event.
   */

  _doButtonDOMAttrModified:
    function sbWizardPageSvc__doButtonDOMAttrModified(aEvent) {
    // Update the wizard buttons.
    this._updateButtons();
  },


  //----------------------------------------------------------------------------
  //
  // Internal widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Update the wizard buttons.
   */

  _updateButtons: function sbWizardPageSvc__updateButtons() {
    // Get the current wizard page.
    var currentPage = this._wizardElem.currentPage;

    // Get the button hide and show settings.
    var hideBackButton = currentPage.getAttribute("hideback") == "true";
    var hideCancelButton = currentPage.getAttribute("hidecancel") == "true";
    var hideNextButton = currentPage.getAttribute("hidenext") == "true";
    var hideFinishButton = currentPage.getAttribute("hidefinish") == "true";
    var showBackButton = currentPage.getAttribute("showback") == "true";
    var showCancelButton = currentPage.getAttribute("showcancel") == "true";
    var showNextButton = currentPage.getAttribute("shownext") == "true";
    var showFinishButton = currentPage.getAttribute("showfinish") == "true";
    var showExtra1Button = currentPage.getAttribute("showextra1") == "true";
    var showExtra2Button = currentPage.getAttribute("showextra2") == "true";

    // Hide navigation buttons by default on post-finish pages.
    if (this._wizardElem.postFinish) {
      hideBackButton = true;
      hideNextButton = true;
      hideFinishButton = true;
    }

    // Update the buttons.  Hide extra buttons by default.
    this._showHideButton("back", showBackButton, hideBackButton);
    this._showHideButton("cancel", showCancelButton, hideCancelButton);
    this._showHideButton("next", showNextButton, hideNextButton);
    this._showHideButton("finish", showFinishButton, hideFinishButton);
    this._showHideButton("extra1", showExtra1Button, true);
    this._showHideButton("extra2", showExtra2Button, true);

    // Focus the next or finish buttons unless they're disabled or hidden.
    var finishButton = this._wizardElem.getButton("finish");
    var hideFinishButton =
          finishButton.getAttribute("hidewizardbutton") == "true";
    var nextButton = this._wizardElem.getButton("next");
    var hideNextButton =
          nextButton.getAttribute("hidewizardbutton") == "true";
    if (!finishButton.hidden && !finishButton.disabled && !hideFinishButton)
      finishButton.focus();
    else if (!nextButton.hidden && !nextButton.disabled && !hideNextButton)
      nextButton.focus();
  },


  /**
   * Set the wizard buttons specified by aButtonID to be shown or hidden as
   * specified by aShow and aHide.  aShow takes precedence over aHide.
   */

  _showHideButton: function sbWizardPageSvc__showHideButton(aButtonID,
                                                            aShow,
                                                            aHide) {
    // Get the button element.
    var button = this._wizardElem.getButton(aButtonID);

    // If button is set to be shown, force it to be visible.
    // If button is set to be hidden, force it to be hidden.  Use an attribute
    // and CSS to hide in order to let the Mozilla wizard widget implementation
    // control the hidden attribute.
    // If button is not set to be shown or hidden, remove the hidden attribute
    // and allow the Mozilla wizard widget to control the show/hide state.
    if (aShow) {
      button.removeAttribute("hidewizardbutton");
      button.hidden = false;
    } else if (aHide) {
      button.setAttribute("hidewizardbutton", "true");
    } else {
      button.removeAttribute("hidewizardbutton");
    }
  },


  /**
   * \brief Return the XUL element with the ID specified by aElementID.  Use the
   *        element "anonid" attribute as the ID.
   *
   * \param aElementID          ID of element to get.
   *
   * \return Element.
   */

  _getElement: function sbWizardPageSvc__getElement(aElementID) {
    return document.getAnonymousElementByAttribute(this._widget,
                                                   "anonid",
                                                   aElementID);
  }
}

