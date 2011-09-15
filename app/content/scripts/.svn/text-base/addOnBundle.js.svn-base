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
 * \file  addOnBundle.js
 * \brief Javascript source for the add-on bundle widget services.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Add-on bundle widget services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Add-on bundle widget imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
Components.utils.import("resource://app/jsmodules/DOMUtils.jsm");


//------------------------------------------------------------------------------
//
// Add-on bundle widget services defs.
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
// Add-on bundle widget services.
//
//------------------------------------------------------------------------------

/**
 * Construct an add-on bundle widget services object for the widget specified by
 * aWidget.
 *
 * \param aWidget               Add-on bundle widget.
 */

function addOnBundleSvc(aWidget) {
  this._widget = aWidget;
}

// Define the object.
addOnBundleSvc.prototype = {
  // Set the constructor.
  constructor: addOnBundleSvc,

  //
  // Internal widget services fields.
  //
  //   _widget                  First-run wizard add-ons widget.
  //   _domEventListenerSet     Set of DOM event listeners.
  //   _addOnBundle             Add-on bundle object.
  //   _addOnsTable             Table of add-ons.
  //

  _widget: null,
  _domEventListenerSet: null,
  _addOnBundle: null,
  _addOnsTable: null,


  //----------------------------------------------------------------------------
  //
  // Widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the widget services.
   */

  initialize: function addOnBundleSvc_initialize() {
    // Create a DOM event listener set.
    this._domEventListenerSet = new DOMEventListenerSet();

    // Listen for add-on item changes.
    var addOnsListElem = this._getElement("add_ons_list");
    var _this = this;
    var func = function(aEvent) { return _this._doCommand(aEvent); };
    this._domEventListenerSet.add(addOnsListElem,
                                  "command",
                                  func,
                                  false);

    // Update the add-on item list.
    this._updateAddOnItemList();
  },


  /**
   * Finalize the widget services.
   */

  finalize: function addOnBundleSvc_finalize() {
    // Remove DOM event listeners.
    if (this._domEventListenerSet) {
      this._domEventListenerSet.removeAll();
    }
    this._domEventListenerSet = null;

    // Clear object fields.
    this._widget = null;
    this._addOnBundle = null;
    this._addOnsTable = null;
  },


  /*
   * Getters/setters.
   */

  /*
   * addOnBundle                Add-on bundle object.
   */

  get addOnBundle() {
    return this._addOnBundle;
  },

  set addOnBundle(aBundle) {
    // Do nothing if bundle is not changing.
    if (aBundle == this._addOnBundle)
      return;

    // Set new bundle.
    this._addOnBundle = aBundle;

    // Update the add-on item list.
    this._updateAddOnItemList();
  },


  //----------------------------------------------------------------------------
  //
  // Widget event handling services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle the command event specified by aEvent.
   */

  _doCommand: function addOnBundleSvc__doCommand(aEvent) {
    // Get the event target info.
    var target = aEvent.target;
    var localName = target.localName;

    // Dispatch event processing.
    switch (localName) {
      case "sb-add-on-bundle-item" :
        this._doAddOnBundleItemChange(target);
        break;

      default :
        break;
    }
  },


  /**
   * Handle changes to the add-on bundle item element specified by aElement.
   *
   * \param aElement            Changed add-on bundle item element.
   */

  _doAddOnBundleItemChange:
    function addOnBundleSvc__doAddOnBundleItemChange(aElement) {
    // Update the add-on bundle item install flag.
    var index = aElement.getAttribute("index");
    var install = aElement.install;
    this.addOnBundle.setExtensionInstallFlag(index, install);
  },


  //----------------------------------------------------------------------------
  //
  // Widget add-ons bundle services.
  //
  //----------------------------------------------------------------------------

  /**
   * Add the add-on from the add-on bundle with the index specified by aIndex.
   *
   * \param aIndex              Index within the add-on bundle of the add-on to
   *                            add.
   */

  _addAddOn: function addOnBundleSvc__addAddOn(aIndex) {
    // Get the add-on ID.  Use the name as an ID if the ID is not available.
    var addOnID = this.addOnBundle.getExtensionAttribute(aIndex, "id");
    if (!addOnID)
      addOnID = this.addOnBundle.getExtensionAttribute(aIndex, "name");

    // Do nothing if add-on already added.
    if (this._addOnsTable[addOnID])
      return;

    // Get the add-on info.
    var addOnInfo = {};
    addOnInfo.index = aIndex;
    addOnInfo.installFlag = this.addOnBundle.getExtensionInstallFlag(aIndex);
    addOnInfo.addOnID = this.addOnBundle.getExtensionAttribute(aIndex, "id");
    addOnInfo.addOnURL = this.addOnBundle.getExtensionAttribute(aIndex, "url");
    addOnInfo.description =
                this.addOnBundle.getExtensionAttribute(aIndex, "description");
    addOnInfo.iconURL = this.addOnBundle.getExtensionAttribute(aIndex, "icon");
    addOnInfo.name = this.addOnBundle.getExtensionAttribute(aIndex, "name");

    // Add the add-on element to the add-on list element.
    addOnInfo.addOnItemElem = this._addAddOnElement(addOnInfo);

    // Add the add-on info to the add-ons table.
    this._addOnsTable[addOnID] = addOnInfo;
  },


  /**
   * Add the add-on element to the add-on list element for the add-on specified
   * by aAddOnInfo.  Return the added add-on element.
   *
   * \param aAddOnInfo          Add-on information.
   *
   * \return                    Add-on item element.
   */

  _addAddOnElement: function addOnBundleSvc__addAddOnElement(aAddOnInfo) {
    // Get the add-on list item template.
    var listItemTemplateElem = this._getElement("list_item_template");

    // Create an add-on list item from the template.
    var listItemElem = listItemTemplateElem.cloneNode(true);
    listItemElem.hidden = false;

    // Set up the add-on item element.
    var itemElem = DOMUtils.getElementsByAttribute(listItemElem,
                                                   "templateid",
                                                   "item")[0];
    itemElem.setAttribute("addonid", aAddOnInfo.addOnID);
    itemElem.setAttribute("addonurl", aAddOnInfo.addOnURL);
    itemElem.setAttribute("defaultinstall", aAddOnInfo.installFlag);
    itemElem.setAttribute("description", aAddOnInfo.description);
    itemElem.setAttribute("icon", aAddOnInfo.iconURL);
    itemElem.setAttribute("index", aAddOnInfo.index);
    itemElem.setAttribute("name", aAddOnInfo.name);

    // Reclone element to force construction with new settings.
    listItemElem = listItemElem.cloneNode(true);

    // Add the add-on list item to the add-on list.
    var itemListElem = this._getElement("add_ons_list");
    itemListElem.appendChild(listItemElem);

    // Get the add-on item element after it's been appended to get a fully
    // functional element object.
    itemElem = DOMUtils.getElementsByAttribute(listItemElem,
                                               "templateid",
                                               "item")[0];

    return itemElem;
  },


  //----------------------------------------------------------------------------
  //
  // Internal widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Update the add-on item list.
   */

  _updateAddOnItemList: function addOnBundleSvc__updateAddOnItemList() {
    // Clear the current add-on item list.
    this._addOnsTable = {};
    var itemListElem = this._getElement("add_ons_list");
    while (itemListElem.firstChild)
      itemListElem.removeChild(itemListElem.firstChild);

    // Add add-ons from bundle.
    if (this.addOnBundle) {
      var extensionCount = this.addOnBundle.bundleExtensionCount;
      for (var i = 0; i < extensionCount; i++) {
        this._addAddOn(i);
      }
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

  _getElement: function addOnBundleSvc__getElement(aElementID) {
    return document.getAnonymousElementByAttribute(this._widget,
                                                   "anonid",
                                                   aElementID);
  }
}

