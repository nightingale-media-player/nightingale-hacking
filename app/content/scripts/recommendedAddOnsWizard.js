/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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
  //   _dialogParameterBlock    Dialog parameter block.
  //   _addOnBundle             Recommended add-on bundle.
  //   _domEventListenerSet     Set of DOM event listeners.
  //   _wizardElem              First-run wizard element.
  //   _addOnBundleInstallerElem
  //                            Add-on bundle installer element.
  //   _restartRequired         True if application needs to be restarted.
  //

  _dialogParameterBlock: null,
  _addOnBundle: null,
  _domEventListenerSet: null,
  _wizardElem: null,
  _addOnBundleInstallerElem: null,
  _restartRequired: false,


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
   * Handle an unload event.
   */

  doUnload: function recommendedAddOnsWizard_doUnload() {
    // Indicate whether a restart is required.
    if (this._dialogParameterBlock) {
      if (this._restartRequired)
        this._dialogParameterBlock.SetString(0, "true");
      else
        this._dialogParameterBlock.SetString(0, "false");
    }

    // Finalize the services.
    this._finalize();
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


  /**
   * Handle a show add-on bundle install page event.
   */

  doShowAddOnBundleInstallPage:
    function recommendedAddOnsWizard_doShowAddOnBundleInstallPage() {
    // Check if add-on installation is required.
    var installRequired = false;
    var extensionCount = this._addOnBundle.bundleExtensionCount;
    for (var i = 0; i < extensionCount; i++) {
      if (this._addOnBundle.getExtensionInstallFlag(i)) {
        installRequired = true;
        break;
      }
    }

    // Install add-ons if required.  Otherwise, advance the wizard.
    if (installRequired) {
      this._addOnBundleInstallerElem.install(this._addOnBundle);
    } else {
      this._wizardElem.canAdvance = true;
      this._wizardElem.advance();
    }
  },


  /**
   * Handle the add-on install complete event specified by aEvent.
   *
   * \param aEvent              Add-on install complete event.
   */

  _doInstallComplete:
    function recommendedAddOnsWizard__doInstallComplete(aEvent) {
    // Set up for application restart if required.
    this._restartRequired = this._addOnBundleInstallerElem.restartRequired;

    // If installation completed successfully, advance wizard.  Otherwise, allow
    // user to view errors.
    if (this._addOnBundleInstallerElem.errorCount == 0) {
      // Advance wizard.
      this._wizardElem.canAdvance = true;
      this._wizardElem.advance();
    } else {
      // Change the next button to an OK button.
      var okButton = this._wizardElem.getButton("next");
      okButton.label = SBString("recommended_add_ons.ok.label");
      okButton.accessKey = SBString("recommended_add_ons.ok.accesskey");

      // Hide the cancel button and show the OK button.
      var wizardPageElem =
            document.getElementById("recommended_add_ons_installation_page");
      wizardPageElem.setAttribute("hidecancel", "true");
      wizardPageElem.setAttribute("shownext", "true");
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
    try {
      this._dialogParameterBlock =
             window.arguments[0].QueryInterface(Ci.nsIDialogParamBlock);
      this._addOnBundle =
             this._dialogParameterBlock.objects.queryElementAt(0, Ci.sbIBundle);
    } catch (ex) {
      Cu.reportError
           ("Recommended add-ons wizard opened with invalid parameters.");
      onExit();
      return;
    }

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

    // Get the wizard element.
    this._wizardElem = document.getElementById
                                  ("recommended_add_ons_update_wizard");

    // Create a DOM event listener set.
    this._domEventListenerSet = new DOMEventListenerSet();

    // Listen for add-on bundle installer completion events.
    var _this = this;
    var func = function(aEvent) { return _this._doInstallComplete(aEvent); };
    this._addOnBundleInstallerElem = document.getElementById
                                                ("add_on_bundle_installer");
    this._domEventListenerSet.add(this._addOnBundleInstallerElem,
                                  "complete",
                                  func,
                                  false);
  },


  /**
   * Finalize the recommended add-ons wizard services.
   */

  _finalize: function recommendedAddOnsWizard__finalize() {
    // Remove DOM event listeners.
    if (this._domEventListenerSet)
      this._domEventListenerSet.removeAll();
    this._domEventListenerSet = null;

    // Clear object fields.
    this._dialogParameterBlock = null;
    this._addOnBundle = null;
    this._wizardElem = null;
    this._addOnBundleInstallerElem = null;
  }
};

