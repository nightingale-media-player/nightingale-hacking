/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

/**
* \file  deviceSync.js
* \brief Javascript source for the device sync widget.
*/

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Device sync widget.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Device sync defs.
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

Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");

if (typeof(XUL_NS) == "undefined")
  var XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

var DeviceSyncWidget = {
  //
  // Device sync object fields.
  //
  //   _widget                  Device sync widget.
  //   _deviceLibrary           Device library we are working with.
  //   _deviceSyncSettings      Device sync settings we are working with.
  //   _mediaSyncSettings       Device sync settings for this media type.
  //   _mediaType               Media Type this sync widget represents.
  //   _ignoreDevicePrefChanges Flag to switch off device pref listener
  //                            temporarily (to be used when we are changing the
  //                            prefs ourselves).
  //

  _widget: null,
  _deviceLibrary: null,
  _deviceSyncSettings: null,
  _mediaSyncSettings: null,
  _mediaType: null,
  _ignoreDevicePrefChanges: false,

  /**
   * \brief Initialize the device progress services for the device progress
   *        widget specified by aWidget.
   *
   * \param aWidget             Device progress widget.
   */

  initialize: function DeviceSyncWidget__initialize(aWidget) {
    // Get the sync widget.
    this._widget = aWidget;

    this._mediaType = this._widget.getAttribute("contenttype") || "audio";

    // Set up some variable UI labels (these depend on the contentType)
    var syncAllLabel = this._getElement("content_auto_sync_all_radio");
    syncAllLabel.label = SBString("device.sync.sync_all.label." +
                                  this._mediaType);
    var syncHeaderCheckbox = this._getElement("content_management_checkbox");
    syncHeaderCheckbox.setAttribute("label",
                                 SBString("device.sync.header.label." +
                                          this._mediaType));

    // Initialize object fields.
    this._device = this._widget.device;
    this._deviceLibrary = this._widget.devLib;
    // Changes we make to the tempSyncSettings will be sent to any listeners
    // these will not be applied to the device until the
    // this._deviceLibrary.applySyncSettings() function has been called.
    this._deviceSyncSettings = this._deviceLibrary.tempSyncSettings;
    var mediaType = this._getMediaType(this._mediaType);
    this._mediaSyncSettings = this._deviceSyncSettings
                                  .getMediaSettings(mediaType);
    this._deviceLibrary.addDeviceLibraryListener(this);

    // Listen for device events.
    var deviceEventTarget =
          this._device.QueryInterface(Ci.sbIDeviceEventTarget);
    deviceEventTarget.addEventListener(this);

    // Listen for changes to playlists.
    LibraryUtils.mainLibrary.addListener
      (this,
       false,
         Ci.sbIMediaList.LISTENER_FLAGS_ITEMADDED |
         Ci.sbIMediaList.LISTENER_FLAGS_AFTERITEMREMOVED |
         Ci.sbIMediaList.LISTENER_FLAGS_ITEMUPDATED);

    // Update the UI.
    this.update();
  },


  /**
   * \brief Finalize the device progress services.
   */

  finalize: function DeviceSyncWidget_finalize() {
    // Remove library listener.
    LibraryUtils.mainLibrary.removeListener(this);

    // Remove the device libary listener for setting changes.
    if (this._deviceLibrary)
      this._deviceLibrary.removeDeviceLibraryListener(this);

    // Stop listening for device events.
    if (this._device) {
      var deviceEventTarget =
            this._device.QueryInterface(Ci.sbIDeviceEventTarget);
      deviceEventTarget.removeEventListener(this);
    }

    // Finalize the device services.
    this._deviceFinalize();

    // Clear object fields.
    this._widget = null;
    this._device = null;
  },


  /**
   * Update the widget UI.
   */

  update: function DeviceSyncWidget_update() {
    // Ensure we have something to update
    if (!this._widget || this._ignoreDevicePrefChanges)
      return;

    // If there is no device library or mediaSyncSettings, hide the widget and
    // return.  Otherwise, show the widget.
    if (!this._deviceLibrary || !this._mediaSyncSettings) {
      this._widget.setAttribute("hidden", "true");
      return;
    }
    this._widget.removeAttribute("hidden");

    /* Get the management type pref UI elements. */
    var syncRadioGroup = this._getElement("content_auto_sync_type_radio_group");
    var syncPlaylistTree = this._getElement("content_auto_sync_playlist_tree");
    var syncEnabledCheckbox = this._getElement("content_management_checkbox");
    var manualMessage = this._getElement("manual-mode-descr");
    var headerBackground =
        this._getElement("content_management_header_background");
    var syncGroupbox = this._getElement("content_management_groupbox");

    // If the device is not in manual mode hide the manual mode message
    var deviceManual = this._deviceSyncSettings.syncMode ==
                            Ci.sbIDeviceLibrarySyncSettings.SYNC_MODE_MANUAL;

    if (deviceManual) {
      manualMessage.removeAttribute("collapsed");
      syncEnabledCheckbox.checked = false;
      syncEnabledCheckbox.setAttribute("disabled", true);
      syncPlaylistTree.setAttribute("disabled", true);
      syncRadioGroup.setAttribute("disabled", true);
      syncRadioGroup.selectedItem = null;
      this._widget.setAttribute("disabled", true);

      // In manual mode, this._widget controls opacity.
      headerBackground.removeAttribute("disabled");
      syncGroupbox.removeAttribute("disabled");
    }
    else {
      manualMessage.setAttribute("collapsed", true);
      syncEnabledCheckbox.removeAttribute("disabled");
      this._widget.removeAttribute("disabled");

      switch (this._mediaSyncSettings.mgmtType) {
        case Ci.sbIDeviceLibraryMediaSyncSettings.SYNC_MGMT_NONE:
          syncEnabledCheckbox.checked = false;
          syncPlaylistTree.setAttribute("disabled", true);
          syncRadioGroup.setAttribute("disabled", true);
          headerBackground.setAttribute("disabled", true);
          syncGroupbox.setAttribute("disabled", true);
          syncRadioGroup.selectedItem = null;
          break;

        case Ci.sbIDeviceLibraryMediaSyncSettings.SYNC_MGMT_ALL:
          syncEnabledCheckbox.checked = true;
          syncPlaylistTree.setAttribute("disabled", true);
          syncRadioGroup.removeAttribute("disabled");
          headerBackground.removeAttribute("disabled");
          syncGroupbox.removeAttribute("disabled");
          syncRadioGroup.selectedItem =
                          this._getElement("content_auto_sync_all_radio");
          break;

        case Ci.sbIDeviceLibraryMediaSyncSettings.SYNC_MGMT_PLAYLISTS:
          syncEnabledCheckbox.checked = true;
          syncPlaylistTree.removeAttribute("disabled");
          syncRadioGroup.removeAttribute("disabled");
          headerBackground.removeAttribute("disabled");
          syncGroupbox.removeAttribute("disabled");
          syncRadioGroup.selectedItem =
                          this._getElement("content_auto_sync_selected_radio");
          break;
      }
    }

    // If we are busy then disable the widget so the user can not make changes
    if (this._device.isBusy) {
      this._widget.setAttribute("disabled", true);
      headerBackground.removeAttribute("disabled");
      syncGroupbox.removeAttribute("disabled");
    }

    // Show the video duration column only for the Video tab.
    var syncPlaylistListVideoHeader =
      this._getElement("content_auto_sync_playlist_duration");
    syncPlaylistListVideoHeader.hidden = (this._mediaType != "video");

    // Set up the playlists
    var syncPlaylistTree = this._getElement("content_auto_sync_playlist_children");

    /* Clear the sync playlist tree. */
    while (syncPlaylistTree.firstChild)
        syncPlaylistTree.removeChild(syncPlaylistTree.firstChild);

    // Get an nsIArray of available playlists for this media type
    var syncPlayLists = this._mediaSyncSettings.syncPlaylists;
    for (var listIndex = 0; listIndex < syncPlayLists.length; listIndex++) {
      var mediaList = syncPlayLists.queryElementAt(listIndex, Ci.sbIMediaList);

      // Load up the information we need
      var guid = mediaList.guid;

      // Duration only applies to video tab so don't waste time on other types
      var duration = -1;
      if (this._mediaType == "video")
        duration = this._syncPrefsCalcDuration(mediaList);

      if (duration >= 0) {
        var durationInfo =
              Cc["@getnightingale.com/Nightingale/Properties/Info/Duration;1"]
                .createInstance(Ci.sbIDurationPropertyInfo);
        duration = durationInfo.format(duration);
      }
      else {
        duration = SBString("device.sync.duration.unavailable");
      }

      // Get the readable name of this list (add mix for mix lists)
      var readableName = mediaList.name;
      var listType = mediaList.getListContentType();
      if (listType == Ci.sbIMediaList.CONTENTTYPE_MIX)
        readableName = SBFormattedString("device.sync.mix." + this._mediaType,
                                         [ readableName ]);

      /* Create our tree row */
      var treeItem = document.createElementNS(XUL_NS, "treeitem");
      var treeRow = document.createElementNS(XUL_NS, "treerow");
      var treeCellCheck = document.createElementNS(XUL_NS, "treecell");
      var treeCellTitle = document.createElementNS(XUL_NS, "treecell");
      var treeCellDuration = document.createElementNS(XUL_NS, "treecell");

      /* Set the value of this row to the guid of the media list */
      treeRow.value = guid;

      /* Setup the cells */
      /* Check box (only editable cell) */
      var isSelected = this._mediaSyncSettings.getPlaylistSelected(mediaList);
      treeCellCheck.setAttribute("value", isSelected);
      treeCellCheck.setAttribute("sbid",
                                 "content_sync_playlist_checkcell." + guid);

      /* Title of the playlist */
      treeCellTitle.setAttribute("label", readableName);
      treeCellTitle.setAttribute("editable", "false");

      /* Duration of all the _mediaType conent in the playlist */
      treeCellDuration.setAttribute("label", duration);
      treeCellDuration.setAttribute("editable", "false");

      /* Append the cells to the row */
      treeRow.appendChild(treeCellCheck);
      treeRow.appendChild(treeCellTitle);
      treeRow.appendChild(treeCellDuration);

      /* Append the row to the tree item */
      treeItem.appendChild(treeRow);

      /* Add the row to the tree. */
      syncPlaylistTree.appendChild(treeItem);
    }
  },


  //----------------------------------------------------------------------------
  //
  // Device sync event handler services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Update the settings for syncing when the user makes changes.
   */

  onUIPrefChange: function DeviceSyncWidget_onUIPrefChange() {
    // Ignore user interaction if the widget is disabled.
    if (this._widget.hasAttribute("disabled"))
      return;
    
    // Ensure we do not update in the middle of updating the settings.
    this._ignoreDevicePrefChanges = true;

    var syncRadioGroup = this._getElement("content_auto_sync_type_radio_group");
    var syncEnabledCheckbox = this._getElement("content_management_checkbox");

    // First check the managementMode, we need to check the main check box and
    // the radio group
    var oldMgmtType = this._mediaSyncSettings.mgmtType;
    var newMgmtType = Ci.sbIDeviceLibraryMediaSyncSettings.SYNC_MGMT_NONE;
    if (syncEnabledCheckbox.checked) {
      if (syncRadioGroup.selectedItem ==
            this._getElement("content_auto_sync_selected_radio") ||
          this._mediaType == "video") {
        newMgmtType = Ci.sbIDeviceLibraryMediaSyncSettings.SYNC_MGMT_PLAYLISTS;
      }
      else {
        newMgmtType = Ci.sbIDeviceLibraryMediaSyncSettings.SYNC_MGMT_ALL;
      }
    }

    if (oldMgmtType != newMgmtType)
      this._mediaSyncSettings.mgmtType = newMgmtType;

    if (this._mediaSyncSettings.mgmtType ==
          Ci.sbIDeviceLibraryMediaSyncSettings.SYNC_MGMT_PLAYLISTS) {
      // Clear the playlists before we set them
      this._mediaSyncSettings.clearSelectedPlaylists();

      // Now scan all the playlists in the list of playlists
      var syncPlayLists = this._mediaSyncSettings.syncPlaylists;
      for (var listIndex = 0; listIndex < syncPlayLists.length; listIndex++) {
        var mediaList = syncPlayLists.queryElementAt(listIndex,
                                                     Ci.sbIMediaList);
        if (mediaList) {
          var guid = mediaList.guid;
          var treeCellCheck = this._getElement(
                                    "content_sync_playlist_checkcell." + guid);
          if (treeCellCheck) {
            // Get the new value
            var isSelected = (treeCellCheck.getAttribute("value") == "true");
            // First see if it is already set to the same value
            var oldSelected = this._mediaSyncSettings
                                  .getPlaylistSelected(mediaList);
            // Now set it if changed
            if (oldSelected != isSelected)
              this._mediaSyncSettings.setPlaylistSelected(mediaList,
                                                          isSelected);
          }
        }
      }
    }

    // Finally update to ensure it all applied
    this._ignoreDevicePrefChanges = false;
    this.update();
  },

  //----------------------------------------------------------------------------
  //
  // sbIDeviceLibraryListener
  //
  //----------------------------------------------------------------------------

  // See sbIMediaListListener below for the other functions like onBatchBegin
  
  /**
   * \brief Called when the sync settings for a library have been applied or
   *  reset or if they have changed.
   * \param aAction what happend to the sync settings
   *  (see sbIDeviceLibraryListener).
   * \param aSyncSettings what the new sync settings are.
   */

  onSyncSettings: function DeviceSyncWidget_onSyncSettings(aAction,
                                                           aSyncSettings) {
    // Re-read the sync settings and media sync settings.
    this._deviceSyncSettings = this._deviceLibrary.tempSyncSettings;
    var mediaType = this._getMediaType(this._mediaType);
    this._mediaSyncSettings = this._deviceSyncSettings
                                  .getMediaSettings(mediaType);

    this.update();
  },

  // Implementation of sbIDeviceLibraryListener methods that must return true to
  // prevent SB_NOTIFY_LISTENERS_ASK_PERMISSION in sbDeviceLibrary.cpp from
  // aborting erroneously.

  onBeforeCreateMediaItem: function DeviceSyncWidget_onBeforeCreateMediaItem(
                                      aContentUri,
                                      aProperties,
                                      aAllowDuplicates) {
    return true;
  },

  onBeforeCreateMediaList: function DeviceSyncWidget_onBeforeCreateMediaList(
                                      aType,
                                      aProperties) {
    return true;
  },

  onBeforeAdd: function DeviceSyncWidget_onBeforeAdd(aMediaItem) {
    return true;
  },

  onBeforeAddAll: function DeviceSyncWidget_onBeforeAddAll(aMediaList) {
    return true;
  },

  onBeforeAddSome: function DeviceSyncWidget_onBeforeAddSome(aMediaItems) {
    return true;
  },

  onBeforeClear: function DeviceSyncWidget_onBeforeClear() {
    return true;
  },

  //----------------------------------------------------------------------------
  //
  // Device sync sbIDeviceEventListener services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Handle the device event specified by aEvent.
   *
   * \param aEvent              Device event.
   */

  onDeviceEvent: function DeviceSyncWidget_onDeviceEvent(aEvent) {
    // Dispatch processing of the event.
    switch(aEvent.type)
    {
      case Ci.sbIDeviceEvent.EVENT_DEVICE_PREFS_CHANGED :
        this.update();
        break;

      case Ci.sbIDeviceEvent.EVENT_DEVICE_STATE_CHANGED:
        this.update();
        break;

      default :
        break;
    }
  },


  //----------------------------------------------------------------------------
  //
  // Device sync sbIMediaListListener services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Called when a media item is added to the list.
   * \param aMediaList The list that has changed.
   * \param aMediaItem The new media item.
   * \return True if you do not want any further onItemAdded notifications for
   *         the current batch.  If there is no current batch, the return value
   *         is ignored.
   */

  onItemAdded: function DeviceSyncWidget_onItemAdded(aMediaList,
                                                     aMediaItem,
                                                     aIndex) {
    // Handle unhidden playlist changes.
    if (aMediaItem.getProperty(SBProperties.isList) &&
        !aMediaItem.getProperty(SBProperties.hidden)) {
      this.update();
    }

    return false;
  },


  /**
   * \brief Called before a media item is removed from the list.
   * \param aMediaList The list that has changed.
   * \param aMediaItem The media item to be removed
   * \return True if you do not want any further onBeforeItemRemoved
   *         notifications for the current batch.  If there is no current batch,
   *         the return value is ignored.
   */

  onBeforeItemRemoved: function DeviceSyncWidget_onBeforeItemRemoved
                                  (aMediaList,
                                   aMediaItem,
                                   aIndex) {
    return true;
  },


  /**
   * \brief Called after a media item is removed from the list.
   * \param aMediaList The list that has changed.
   * \param aMediaItem The removed media item.
   * \return True if you do not want any further onAfterItemRemoved for the
   *         current batch.  If there is no current batch, the return value is
   *         ignored.
   */

  onAfterItemRemoved: function DeviceSyncWidget_onAfterItemRemoved
                                 (aMediaList,
                                  aMediaItem,
                                  aIndex) {
    // Handle playlist changes.
    if (aMediaItem.getProperty(SBProperties.isList))
      this.update();

    return false;
  },


  /**
   * \brief Called when a media item is changed.
   * \param aMediaList The list that has changed.
   * \param aMediaItem The item that has changed.
   * \param aProperties Array of properties that were updated.  Each property's
   *        value is the previous value of the property
   * \return True if you do not want any further onItemUpdated notifications
   *         for the current batch.  If there is no current batch, the return
   *         value is ignored.
   */

  onItemUpdated: function DeviceSyncWidget_onItemUpdated(aMediaList,
                                                         aMediaItem,
                                                         aIndex,
                                                         aProperties) {
    // Handle playlist changes.
    if (aMediaItem.getProperty(SBProperties.isList))
      this.update();

    return false;
  },

  /**
   * \Brief Called before a media list is cleared.
   * \param sbIMediaList aMediaList The list that is about to be cleared.
   * \param aExcludeLists If true, only media items, not media lists, are being
   *                      cleared.
   * \return True if you do not want any further onBeforeListCleared notifications
   *         for the current batch.  If there is no current batch, the return
   *         value is ignored.
   */

  onBeforeListCleared:
    function DeviceSyncWidget_onBeforeListCleared(aMediaList,
                                                  aExcludeLists) {
    return true;
  },

  /**
   * \Brief Called when a media list is cleared.
   * \param sbIMediaList aMediaList The list that was cleared.
   * \param aExcludeLists If true, only media items, not media lists, were
   *                      cleared.
   * \return True if you do not want any further onListCleared notifications
   *         for the current batch.  If there is no current batch, the return
   *         value is ignored.
   */

  onListCleared: function DeviceSyncWidget_onListCleared(aMediaList,
                                                         aExcludeLists) {
    // Handle playlist changes.
    if (aExcludeLists)
      this.update();

    return false;
  },


  /**
   * \brief Called when the library is about to perform multiple operations at
   *        once.
   *
   * This notification can be used to optimize behavior. The consumer may
   * choose to ignore further onItemAdded or onItemRemoved notifications until
   * the onBatchEnd notification is received.
   *
   * \param aMediaList The list that has changed.
   */

  onBatchBegin: function DeviceSyncWidget_onBatchBegin(aMediaList) {},


  /**
   * \brief Called when the library has finished performing multiple operations
   *        at once.
   *
   * This notification can be used to optimize behavior. The consumer may
   * choose to stop ignoring onItemAdded or onItemRemoved notifications after
   * receiving this notification.
   *
   * \param aMediaList The list that has changed.
   */

  onBatchEnd: function DeviceSyncWidget_onBatchEnd(aMediaList) {},


  //----------------------------------------------------------------------------
  //
  // Device sync XUL services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Return the XUL element with the ID specified by aElementID.  Use the
   *        element "sbid" attribute as the ID.
   *
   * \param aElementID          ID of element to get.
   *
   * \return Element.
   */

  _getElement: function DeviceSyncWidget__getElement(aElementID) {
    return document.getAnonymousElementByAttribute(this._widget,
                                                   "sbid",
                                                   aElementID);
  },

  /*
   * \brief get the ID of media type out of the media type string
   *
   * \param aMediaType           media type string
   *
   */

  _getMediaType: function DeviceSyncWidget__getMediaType(aMediaType)
  {
    var mediaType = Ci.sbIDeviceLibrary.MEDIATYPE_UNKOWN;
    if (aMediaType == "audio")
      mediaType = Ci.sbIDeviceLibrary.MEDIATYPE_AUDIO;
    else if (aMediaType == "video")
      mediaType = Ci.sbIDeviceLibrary.MEDIATYPE_VIDEO;

    return mediaType;
  },

  /**
    * _syncPrefsCalcDuration
    *
    * \brief This function will calculate the total duration of items in a list
    * that match the content type of this._mediaType.
    */

  _syncPrefsCalcDuration:
    function DeviceSyncWidget__syncPrefsCalcDuration(aMediaList) {
    // sbIMediaListEnumerationListener
    // This gives us a count and duration of items of contentType
    var contentDuration = -1;
    var durationCounter = {
      onEnumerationBegin : function(aMediaList) {
        return Ci.sbIMediaListEnumerationListener.CONTINUE;
      },
      onEnumeratedItem : function(aMediaList, aMediaItem) {
        var duration = aMediaItem.getProperty(SBProperties.duration);
        if (duration != null) {
          contentDuration += parseFloat(duration);
        }
        return Ci.sbIMediaListEnumerationListener.CONTINUE;
      },
      onEnumerationEnd : function(aMediaList, aStatusCode) {
      }
    };

    aMediaList.enumerateItemsByProperty(SBProperties.contentType,
                                        this._mediaType,
                                        durationCounter,
                                        Ci.sbIMediaList
                                          .ENUMERATIONTYPE_SNAPSHOT);

    return contentDuration;
  },

  //----------------------------------------------------------------------------
  //
  // Device sync device services.
  //
  //   These services provide an interface to the device object.
  //   These services register for any pertinent device notifications and call
  //   _update to update the UI accordingly.  In particular, these services
  //   register to receive notifications when the device state changes.
  //
  //----------------------------------------------------------------------------

  //
  // Device info services fields.
  //
  //   _device                  sbIDevice object.
  //

  _device: null,

  /**
   * \brief Finalize the device services.
   */

  _deviceFinalize: function DeviceSyncWidget__deviceFinalize() {
    // Clear object fields.
    this._device = null;
  }
};
