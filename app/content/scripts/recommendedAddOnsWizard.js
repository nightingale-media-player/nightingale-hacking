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
 * \file  recommendedAddOnsWizard.js
 * \brief Javascript source for the recommended add-ons wizard.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Recommended add-ons wizard.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Recommended add-ons wizard imported services.
//
//------------------------------------------------------------------------------

Components.utils.import("resource://app/jsmodules/AddOnUtils.jsm");
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");


//------------------------------------------------------------------------------
//
// Recommended add-ons wizard defs.
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
// Recommended add-ons wizard services.
//
//------------------------------------------------------------------------------

var recommendedAddOnsWizard = {
  //
  // Internal recommended add-ons wizard fields.
  //
  //   _addOnBundle             Recommended add-on bundle.
  //

  _addOnBundle: null,


  //----------------------------------------------------------------------------
  //
  // Event handling services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle a load event.
   */

  doLoad: function recommendedAddOnsWizard_doLoad() {
    // Initialize the services.
    this._initialize();
  },


  /**
   * Handle a wizard finish event.
   */

  doFinish: function recommendedAddOnsWizard_doFinish() {
    // Add all add-ons in bundle to recommended add-ons blacklist.
    if (this._addOnBundle) {
      var extensionCount = this._addOnBundle.bundleExtensionCount;
      for (var i = 0; i < extensionCount; i++) {
        // Get the bundle add-on ID.
        var addOnID = this._addOnBundle.getExtensionAttribute(i, "id");

        // Add add-on to blacklist.
        AddOnBundleLoader.addAddOnToBlacklist(addOnID);
      }
    }
  },


  //----------------------------------------------------------------------------
  //
  // Internal services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the recommended add-ons wizard services.
   */

  _initialize: function recommendedAddOnsWizard__initialize() {
    // Get the dialog parameters.
    var dialogPB = window.arguments[0].QueryInterface(Ci.nsIDialogParamBlock);
    this._addOnBundle = dialogPB.objects.queryElementAt(0, Ci.sbIBundle);

    // Get the recommended add-on bundle info.
    var addOnCount = this._addOnBundle.bundleExtensionCount;

    // Set the add-on bundle object for the recommended add-on bundle widget.
    var addOnBundleElem = document.getElementById("recommended_add_on_bundle");
    addOnBundleElem.addOnBundle = this._addOnBundle;

    // Set the recommended add-ons wizard page header.
    var headerElem = document.getElementById("recommended_add_ons_header");
    headerElem.value = SBFormattedCountString
                         ("recommended_add_ons.header.label", addOnCount);

    // Set the recommended add-ons wizard page description.
    var descriptionText =
          SBFormattedCountString("recommended_add_ons.description", addOnCount);
    var descriptionTextNode = document.createTextNode(descriptionText);
    var descriptionElem =
          document.getElementById("recommended_add_ons_description");
    descriptionElem.appendChild(descriptionTextNode);
  }
};

