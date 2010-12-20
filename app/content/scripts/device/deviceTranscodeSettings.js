/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

/**
* \file  deviceTranscodeSettings.js
* \brief Javascript source for the device transcode settings widget.
*/

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Device transcode settings widget.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Device transcode settings defs.
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

if (typeof(XUL_NS) == "undefined")
  var XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://app/jsmodules/WindowUtils.jsm");

//------------------------------------------------------------------------------
//
// Device transcode settings service.
//
//------------------------------------------------------------------------------

var DTSW = {
  //
  // Device transcode settings object fields.
  //
  //   _widget                  Device transcode settings widget.
  //   _disabled                Disabled state.
  //   _showDesc                Quality description display.
  //

  _widget: null,
  _disabled: false,
  _showDesc: false,

  /**
   * \brief Initialize the device transcode settings services for the widget.
   *
   * \param aWidget             Device transcode settings widget.
   */

  initialize: function DTSW__initialize(aWidget) {
    // Get the device widget.
    this._widget = aWidget;

    // Initialize object fields.
    var disabled = this._widget.getAttribute("disabled");
    if (disabled)
      this._disabled = true;

    this._showDescription();
    this._update();

  },

  /**
   * \brief Finalize the device transcode settings services.
   */

  finalize: function DTSW_finalize() {
    this._widget = null;
    this._disabled = null;
  },

  //----------------------------------------------------------------------------
  //
  // Device transcode settings UI update services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Update the device transcode settings UI.
   */

  _update: function DTSW__update() {
    var modeDetailsBox = this._getElement("transcoding-mode-details");
    var children = modeDetailsBox.childNodes;
    for (let i = 0; i < children.length; i++) {
      let currChild = children.item(i);
      if (this._disabled) {
        currChild.setAttribute("disabled", true);
      } else {
        currChild.removeAttribute("disabled");
      }
    }
  },

  /**
   * \brief Display the device transcoding description.
   */

  _showDescription: function DTSW__showDescription() {
    var showDesc = this._widget.getAttribute("showDesc");
    var qualityLabel = this._getElement("transcoding-quality-desc");
    if (showDesc) {
      qualityLabel.removeAttribute("hidden");
    } else {
      qualityLabel.setAttribute("hidden", true);
    }
  },

  /**
   * \brief Adds the bitrate to the bitrate menu. Sets the item to selected
   *        when appropriate.
   *
   * \param aBitrate            Integer identifying the bitrate to add.
   * \param aSelected           Selected bitrate.
   *
   * \return The added bitrate.
   */

  _addBitrate: function DTSW__addBitrate(aBitrate, aSelected) {
    var bitrateEntry = this._getElement("transcoding-bitrate-kbps");
    var bitrateList = this._getElement("transcoding-bitrate-list");
    var bitrateMenuItem = document.createElementNS(XUL_NS, "menuitem");
    bitrateMenuItem.setAttribute("label", aBitrate);
    bitrateMenuItem.setAttribute("value", aBitrate);
    bitrateList.appendChild(bitrateMenuItem);
    // Select the bitrate.
    if (aSelected == aBitrate) {
      bitrateMenuItem.setAttribute("selected", "true");
      bitrateEntry.selectedItem = bitrateMenuItem;
    } else {
      bitrateMenuItem.setAttribute("selected", "false");
    }
    return aBitrate;
  },

  /**
   * \brief Rebuilds the profile menu with items specified by aProfiles.
   *
   * \param aProfiles           Array of sbITranscodeProfile objects.
   *
   * \return
   */

  _rebuildProfiles: function DTSW__rebuildProfiles(aProfiles) {
    this._profiles = aProfiles;

    // Get the transcoding format menu list.
    var profilesMenu = this._getElement("encoding-format-menu");
    var profilesMenuList = this._getElement("encoding-format-list");

    // Clear the menu list.
    while (profilesMenuList.firstChild)
      profilesMenuList.removeChild(profilesMenuList.firstChild);

    // Fill in the menu list with each available profile.
    for (let i = 0; i < this._profiles.length; i++) {
      var profile = this._profiles[i];
      var readableName = profile.description;
      var menuItem = document.createElementNS(XUL_NS, "menuitem");
      menuItem.setAttribute("label", readableName);
      menuItem.setAttribute("value", profile.id);
      // Add the menu item to the list.
      profilesMenuList.appendChild(menuItem);
    }
  },

  /**
   * \brief Activates a profile by the profile id.
   *
   * \param aId                 Profile id for the sbITranscodeProfile.
   *
   * \return
   */

  _activateProfileById: function DTSW__activateProfileById(aId) {
    var profile = this._getProfileFromProfileId(aId);
    this._activateProfile(profile);
  },

  /**
   * \brief Activates a profile, building the respective bitrate menu
   *        once a profile is selected.
   *
   * \param aProfile            sbITranscodeProfile to activate.
   *
   * \return
   */

  _activateProfile: function DTSW__activateProfile(aProfile) {
    var defaulted = false;
    // For invalid profiles, obtain the default profile before proceeding.
    if (!aProfile) {
      aProfile = this._getDefaultProfile();
      defaulted = true;
    }

    var formatList = this._getElement("encoding-format-list");
    for (var i = 0; i < formatList.childNodes.length; i++) {
      var formatMenuItem = formatList.childNodes.item(i);
      if (formatMenuItem.value == aProfile.id) {
        formatMenuItem.setAttribute("selected", "true");
        this._getElement("encoding-format-menu").selectedItem = formatMenuItem;
        this._selectedProfile = aProfile;
      } else {
        formatMenuItem.setAttribute("selected", "false");
      }
    }

    // Update the bitrate menu
    this._updateBitrates(aProfile, defaulted);
  },

  /**
   * \brief Updates bitrate menuitems based on profile and selects the active
   *        item.
   *
   * \return
   */

  _updateBitrates: function DTSW__updateBitrates(aProfile, aDefaulted) {
    // Does the active profile have a bitrate property?
    var foundBitrate = false;
    var propertiesArray = aProfile.audioProperties;
    if (propertiesArray) {
      for (var i = 0; i < propertiesArray.length; i++) {
        var property = propertiesArray.queryElementAt(i,
                Ci.sbITranscodeProfileProperty);
        if (property.propertyName == "bitrate") {
          foundBitrate = true;
          var bitrateEntry = this._getElement("transcoding-bitrate-kbps");
          var bitrateList = this._getElement("transcoding-bitrate-list");
          var brIndex = bitrateEntry.selectedIndex;
          var selected = bitrateEntry.value;
          // Clear the bitrate menu items.
          while (bitrateList.firstChild)
            bitrateList.removeChild(bitrateList.firstChild);

          var defaultBitrate = parseInt(property.value) / 1000;
          // Set the bitrate for transcode profile (either by pref or
          // default).
          if (this._selectedBitrate && !aDefaulted) {
            selected = parseInt(this._selectedBitrate) / 1000;
          } else {
            selected = defaultBitrate;
          }

          var bitrate = parseInt(property.valueMin) / 1000;
          var maxBitrate = parseInt(property.valueMax) / 1000;
          var increment = parseInt(bitrateEntry.getAttribute("increment"));
          var bitrateArray = [];

          // Create bitrate menu items for given profile.
          while (bitrate < maxBitrate) {
            bitrateArray.push(this._addBitrate(bitrate, selected));
            // Increment for creating the next bitrate.
            bitrate += increment;
            var mod = bitrate % increment;
            if (mod != 0) {
              bitrate += increment - mod;
            }
          }

          // Create the lastChild for the bitrate menulist.
          bitrateArray.push(this._addBitrate(maxBitrate, selected));

          // In the case where a previous bitrate did not exist in the updated
          // menulist, select the menuitem by previous index or default.
          if (bitrateArray.indexOf(parseInt(bitrateEntry.value)) == -1) {
            if (brIndex >= bitrateArray.length) {
              bitrateEntry.selectedItem = bitrateList.lastChild;
            } else if (brIndex >= 0) {
              bitrateEntry.selectedIndex = brIndex;
            } else {
              bitrateEntry.selectedIndex = bitrateArray.indexOf(defaultBitrate);
            }
          }

          // Store the selected bitrate
          this._selectedBitrate = parseInt(bitrateEntry.value) * 1000;
        }
      }
    }

    var bitrateBox = this._getElement("transcoding-bitrate");
    if (foundBitrate) {
      bitrateBox.removeAttribute("hidden");
    }
    else {
      bitrateBox.setAttribute("hidden", "true");
    }
  },

  /**
   * \brief Retrieves an sbITranscodeProfile object from its profile id.
   *
   * \param aId                 Profile id string.
   *
   * \return sbITranscodeProfile or null.
   */

  _getProfileFromProfileId: function DTSW__getProfileFromProfileId(aId) {
    for (var i = 0; i < this._profiles.length; i++) {
      var profile = this._profiles[i];
      if (profile.id == aId)
        return profile;
    }
    return null;
  },

  /**
   * \brief Retrieves sbITranscodeProfile with highest priority.
   *
   * \return sbITranscodeProfile.
   */

  _getDefaultProfile: function DTSW__getDefaultProfile() {
    return this._profiles.reduce(function(high, next) {
        return high.priority > next.priority ? high : next;
    });
  },

  /**
   * \brief Return the XUL element with the ID specified by aElementID.  Use the
   *        element "sbid" attribute as the ID.
   *
   * \param aElementID          ID of element to get.
   *
   * \return Element.
   */

  _getElement: function DTSW__getElement(aElementID) {
    return document.getAnonymousElementByAttribute(this._widget,
                                                   "sbid",
                                                   aElementID);
  }
};
