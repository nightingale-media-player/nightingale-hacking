/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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
  //   _isBusy                  Flag for state of device.
  //   _mediaType               Media Type this sync widget represents.
  //   _syncSettingsChanged     Will be true if settings changed and haven't
  //                            been saved yet.
  //   _ignoreDevicePrefChanges Flag to switch off device pref listener
  //                            temporarily (to be used when we are changing the
  //                            prefs ourselves).
  //

  _widget: null,
  _deviceLibrary: null,
  _isBusy: false,
  _mediaType: null,
  _syncSettingsChanged: false,
  _ignoreDevicePrefChanges: false,

  /**
   * \brief Initialize the device progress services for the device progress
   *        widget specified by aWidget.
   *
   * \param aWidget             Device progress widget.
   */

  initialize: function DeviceSyncWidget__initialize(aWidget) {
    Cu.import("resource://app/jsmodules/DeviceHelper.jsm", this);

    // Get the sync widget.
    this._widget = aWidget;

    this._mediaType = this._widget.getAttribute("contenttype") || "audio";

    // Set up some variable UI labels (these depend on the contentType)
    var syncAllLabel = this._getElement("content_auto_sync_all_radio");
    syncAllLabel.label = SBString("device.sync.sync_all.label." +
                                  this._mediaType);
    var syncHeaderLabel = this._getElement("content_management_header_label");
    syncHeaderLabel.setAttribute("label",
                                 SBString("device.sync.header.label." +
                                          this._mediaType));

    // Initialize object fields.
    this._device = this._widget.device;
    this._deviceLibrary = this._widget.devLib;

    // Listen for sync settings apply/cancel events.
    let self = this;
    document.addEventListener("sbDeviceSync-settings", function(aEvent) {
      self._onSettingsEvent(aEvent);
    }, false);

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

    // Initialize, read, and apply the sync preferences.
    this.syncPrefsInitialize();
    this.syncPrefsRead();
    this.syncPrefsApply();

    // Update the UI.
    this.update();
  },


  /**
   * \brief Finalize the device progress services.
   */

  finalize: function DeviceSyncWidget_finalize() {
    // Remove library listener.
    LibraryUtils.mainLibrary.removeListener(this);

    // Stop listeneing for device events.
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
    // If there is no device library, hide the widget and return.  Otherwise,
    // show the widget.
    if (!this._deviceLibrary) {
      this._widget.setAttribute("hidden", "true");
      return;
    }
    this._widget.removeAttribute("hidden");
  },


  //----------------------------------------------------------------------------
  //
  // Device sync event handler services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Update the settings for syncing when the user makes changes and
   * save these to the preferences.
   */

  onUIPrefChange: function DeviceSyncWidget_onUIPrefChange() {
    // Extract preferences
    this.syncPrefsExtract();
    this.syncPrefsApply();

    if (!this._syncSettingsChanged) {
      // Check whether something really changed
      var oldPrefs = {};
      this.syncPrefsCopy(this._syncPrefs, oldPrefs);
      this.syncPrefsRead(oldPrefs);
      this._syncSettingsChanged = this.syncPrefsChanged(this._syncPrefs, oldPrefs);

      // Notify listeners that we are in editing mode now
      if (this._syncSettingsChanged)
        this._dispatchSettingsEvent(this.SYNCSETTINGS_CHANGE);
    }
  },

  /**
   * \brief Notifies listener about a pref change actions.
   *
   * \param detail              One of the SYNCSETTINGS_* constants
   */

  _dispatchSettingsEvent: function DPW__dispatchSettingsEvent(detail) {
    let event = document.createEvent("UIEvents");
    event.initUIEvent("sbDeviceSync-settings", false, false, window, detail);
    document.dispatchEvent(event);
  },

  /**
   * \brief Handles sbDeviceSync-settings events and cancels/applies edit in
   *        progress as required.
   *
   * \param aEvent              Event to handle.
   */

  _onSettingsEvent: function DeviceSyncWidget__onSettingsEvent(aEvent) {
    switch (aEvent.detail) {
      case this.SYNCSETTINGS_CANCEL:
        // Reset displayed preferences
        this._syncSettingsChanged = false;
        this.syncPrefsRead();
        this.syncPrefsApply();
        break;
      case this.SYNCSETTINGS_APPLY:
        // Save preferences
        this._syncSettingsChanged = false;
        this._dispatchSettingsEvent(this.SYNCSETTINGS_SAVING);
        try {
          this.syncPrefsWrite();
        }
        finally {
          this._dispatchSettingsEvent(this.SYNCSETTINGS_SAVED);
        }
        break;
      case this.SYNCSETTINGS_SAVING:
        this._ignoreDevicePrefChanges = true;
        break;
      case this.SYNCSETTINGS_SAVED:
        this._ignoreDevicePrefChanges = false;
        break;
    }
  },

  /**
   * \brief Handle changes to the library playlists.
   */

  _onPlaylistChange: function DeviceSyncWidget_onPlaylistChange() {
    // Make a copy of the current sync preferences.
    var prevSyncPrefs = {};
    this.syncPrefsCopy(this._syncPrefs, prevSyncPrefs);

    // Re-initialize the sync preferences.
    this.syncPrefsInitialize();

    // Re-read the preferences to update the stored preference set.
    this.syncPrefsRead();

    // Copy previous preference values and apply them.
    this.syncPrefsCopyValues(prevSyncPrefs, this._syncPrefs);
    this.syncPrefsApply();
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
        if (this._ignoreDevicePrefChanges)
          return;

        // If any sync preferences changed, cancel the edit in progress and
        // reload prefs.
        if (this._syncSettingsChanged)
          this._dispatchSettingsEvent(this.SYNCSETTINGS_CANCEL);
        else {
          this.syncPrefsRead();
          this.syncPrefsApply();
        }
        break;

      case Ci.sbIDeviceEvent.EVENT_DEVICE_STATE_CHANGED:
        try {
          this._isBusy = !(aEvent.deviceState == Ci.sbIDevice.STATE_IDLE);
        } catch (e) {}
        this.syncPrefsApply();
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
      this._onPlaylistChange();
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
      this._onPlaylistChange();

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
      this._onPlaylistChange();

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
    this.onPlaylistChange();

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

  /*
   * \brief get the ID of list content type out of the content type string
   *
   * \param aContentType           content type string
   *
   */

  _getListContentType:
  function DeviceSyncWidget__getListContentType(aContentType)
  {
    var contentType = Ci.sbIMediaList.CONTENTTYPE_NONE;
    if (aContentType == "audio")
      contentType = Ci.sbIMediaList.CONTENTTYPE_AUDIO;
    else if (aContentType == "video")
      contentType = Ci.sbIMediaList.CONTENTTYPE_VIDEO;
    else if (aContentType == "mix")
      contentType = Ci.sbIMediaList.CONTENTTYPE_MIX;

    return contentType;
  },

  /* ***************************************************************************
   *
   * device sync preference services.
   *
   ****************************************************************************/

  /*
   * _syncPrefs                  Working sync preferences.
   * _storedSyncPrefs            Stored sync preferences.
   *
   *   The current working set of sync preferences are maintained in _syncPrefs.
   * These are the preferences that the user has set and that is represented by
   * the UI but has not neccessarily been written to the preference storage.
   *   The set of sync preferences in the preference storage is maintained in
   * _storedSyncPrefs.  This is used to determine whether any preferences have
   * been changed and need to be written to the preference storage.
   */

  _syncPrefs : null,
  _storedSyncPrefs : null,


  /*
   * syncPrefsInitialize
   *
   * \brief This function initializes the sync preference services.
   */

  syncPrefsInitialize: function DeviceSyncWidget_syncPrefsInitialize()
  {
    /* Initialize the working preference set. */
    this.syncPrefsInitPrefs();

    /* Initialize the preference UI. */
    this.syncPrefsInitUI();
  },


  /*
   * syncPrefsFinalize
   *
   * \brief This function finalizes the sync preference services.
   */

  syncPrefsFinalize: function DeviceSyncWidget_syncPrefsFinalize()
  {
    /* Clear object fields. */
    this._syncPrefs = null;
    this._storedSyncPrefs = null;
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
        if (aMediaItem.getProperty(SBProperties.duration) != null) {
          contentDuration +=
            parseFloat(aMediaItem.getProperty(SBProperties.duration));
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

  /**
   * syncPrefsGetListInfo
   *
   * \brief This function gets the information from a media list required to
   * place it under the correct tabs.
   */

  syncPrefsGetListInfo:
    function DeviceSyncWidget_syncPrefsGetListInfo(aMediaList) {
    var listInfo = {
      belongsInTab: true,     // If this list belongs in this tab
      contentType: "audio",   // Content type of this list (audio/video/mix)
      isMix: false,           // If it is a mix different content (audio/video)
      contentDuration: -1     // Duration of items of main content type
    };

    // Do some quick check on some types of lists
    // These lists do not belong in any tabs but could not be filtered out
    var mCustomType = aMediaList.getProperty(SBProperties.customType);
    if (mCustomType == "download") {
      listInfo.belongsInTab = false;
      return listInfo;
    }
    else if (mCustomType == "video-togo") {
      // This should always be video type
      listInfo.belongsInTab = (this._mediaType == "video");
      listInfo.contentType = "video";
      listInfo.contentDuration = this._syncPrefsCalcDuration(aMediaList);
      return listInfo;
    }

    // Lists that have no items default to the Music tab.
    if (aMediaList.isEmpty) {
      listInfo.belongsInTab = (this._mediaType == "audio");
      return listInfo;
    }

    var listType = aMediaList.getListContentType();
    switch (listType) {
      case Ci.sbIMediaList.CONTENTTYPE_AUDIO:
        listInfo.belongsInTab = (this._mediaType == "audio")
        break;
      case Ci.sbIMediaList.CONTENTTYPE_VIDEO:
        listInfo.contentType = "video";
        listInfo.belongsInTab = (this._mediaType == "video")
        listInfo.contentDuration = this._syncPrefsCalcDuration(aMediaList);
        break;
      case Ci.sbIMediaList.CONTENTTYPE_MIX:
        listInfo.contentType = "mix";
        listInfo.isMix = true;
        if (this._mediaType == "video")
          listInfo.contentDuration = this._syncPrefsCalcDuration(aMediaList);
        break;
      default:
        Cu.reportError("Unsupported media list type!");
        break;
    }

    return listInfo;
  },

  /**
   * syncPrefsInitPrefs
   *
   * \brief This function initializes the working sync preference set. It sets
   * up the working sync preference object with all of the preference fields
   * and sets default values.  It does not read the preferences from the
   * preference storage.
   */

  syncPrefsInitPrefs: function DeviceSyncWidget_syncPrefsInitPrefs()
  {
    var                         mediaListList;
    var                         mediaList;
    var                         mediaListArray;
    var                         guid;
    var                         readableName;
    var                         pref;
    var                         i;

    /* Clear the sync preferences. */
    this._syncPrefs = {};

    /* Create and initialize the management type preference. */
    this._syncPrefs.mgmtType = {};
    this._syncPrefs.mgmtType.value = Ci.sbIDeviceLibrary.MGMT_TYPE_MANUAL;

    /* Create the sync playlist list preference. */
    this._syncPrefs.syncPlaylistList = {};

    /* Get the list of library media lists. */
    mediaListArray = new LibraryUtils.MediaListEnumeratorToArray();
    var propArray =
      Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
        .createInstance(Ci.sbIMutablePropertyArray);;
    propArray.appendProperty(SBProperties.isList, "1");
    propArray.appendProperty(SBProperties.hidden, "0");
    LibraryUtils.mainLibrary.enumerateItemsByProperties
                                    (propArray,
                                     mediaListArray,
                                     Ci.sbIMediaList.ENUMERATIONTYPE_SNAPSHOT);
    mediaListList = mediaListArray.array;

    /* Fill in the sync playlist list preferences. */
    for (i = 0; i < mediaListList.length; i++)
    {
      /* Get the media list info. */
      mediaList = mediaListList[i];

      // Get the information for this media list we need
      var listInfo = this.syncPrefsGetListInfo(mediaList);
      if (listInfo.belongsInTab) {
        guid = mediaList.guid;
        readableName = mediaList.name;
        if (listInfo.isMix)
          readableName = SBFormattedString("device.sync.mix." + this._mediaType,
                                           [ readableName ]);

        /* Set up the sync playlist preference. */
        pref = {};
        pref.guid = guid;
        pref.readableName = readableName;
        pref.duration = listInfo.contentDuration;
        pref.contentType = listInfo.contentType;
        pref.value = false;

        /* Add the preference to the sync playlist list preference. */
        this._syncPrefs.syncPlaylistList[guid] = pref;
      }
    }
  },


  /**
   * syncPrefsInitUI
   *
   * \brief This function initializes the sync preference UI elements to match
   * the set of working sync preference fields.  It does not apply the
   * preference values to the UI.
   */

  syncPrefsInitUI: function DeviceSyncWidget_syncPrefsInitUI()
  {
    var                         syncPlaylistList;
    var                         syncPlaylistTree;
    var                         syncPlaylistListVideoHeader;
    var                         guid;
    var                         readableName;
    var                         duration;
    var                         durationInfo;

    durationInfo = Cc["@songbirdnest.com/Songbird/Properties/Info/Duration;1"]
                     .createInstance(Ci.sbIDurationPropertyInfo);

    /* Get the sync playlist tree and prefs. */
    syncPlaylistTree = this._getElement("content_auto_sync_playlist_children");
    syncPlaylistList = this._syncPrefs.syncPlaylistList;

    // Show the video duration column only for the Video tab.
    syncPlaylistListVideoHeader =
      this._getElement("content_auto_sync_playlist_duration");
    syncPlaylistListVideoHeader.hidden = (this._mediaType != "video");

    /* Clear the sync playlist tree. */
    while (syncPlaylistTree.firstChild)
        syncPlaylistTree.removeChild(syncPlaylistTree.firstChild);

    /* Fill in the sync playlist list box. */
    for (guid in syncPlaylistList)
    {
      // Don't add playlists that have been removed
      if (!syncPlaylistList[guid])
        continue;

      /* Get the sync playlist info. */
      readableName = syncPlaylistList[guid].readableName;

      /* Get the properly formated duration */
      if (syncPlaylistList[guid].duration >= 0)
        duration = durationInfo.format(syncPlaylistList[guid].duration);
      else
        duration = SBString("device.sync.duration.unavailable");

      /* Create our tree row */
      var treeItem = document.createElementNS(XUL_NS, "treeitem");
      var treeRow = document.createElementNS(XUL_NS, "treerow");
      var treeCellCheck = document.createElementNS(XUL_NS, "treecell");
      var treeCellTitle = document.createElementNS(XUL_NS, "treecell");
      var treeCellDuration = document.createElementNS(XUL_NS, "treecell");

      treeRow.value = guid;
      treeRow.setAttribute("sbid", "content_sync_playlist_row." + guid);

      /* Setup the cells */
      /* Check box (only editable cell) */
      treeCellCheck.setAttribute("value", "false");
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


  /**
   * syncPrefsRead
   *
   * \brief This function reads the sync preferences from the preference
   *        storage into the preferences object specified by aPrefs.  If a
   *        preferences object is not specified, this function reads the
   *        preferences into the working sync preferences.  In addition, this
   *        function updates the stored sync preferences object.
   */

  syncPrefsRead: function DeviceSyncWidget_syncPrefsRead(aPrefs)
  {
    var                         readPrefs;
    var                         storedSyncPlaylistList;
    var                         syncPlaylistList;
    var                         syncPlaylistML;
    var                         guid;
    var                         i;
    var                         j;

    // If there's no device library, just return.
    if (!this._deviceLibrary)
      return;

    /* If no preference object is specified, */
    /* read into the working preferences.    */
    if (aPrefs)
      readPrefs = aPrefs;
    else
      readPrefs = this._syncPrefs;

    /* Read the management type preference. */
    var mediaType = this._getMediaType(this._mediaType);
    var manual = 
      this._deviceLibrary.syncMode == Ci.sbIDeviceLibrary.SYNC_MANUAL ?
        Ci.sbIDeviceLibrary.MGMT_TYPE_MANUAL : 0;
    readPrefs.mgmtType.value = 
      this._deviceLibrary.getMgmtType(mediaType) | manual;                               

    /* Clear and read the sync playlist list preferences. */
    syncPlaylistList = readPrefs.syncPlaylistList;
    for (guid in syncPlaylistList)
      syncPlaylistList[guid].value = false;

    /* Read the stored sync playlist list preferences for both audio/video. */
    for (i = 0; i < Ci.sbIDeviceLibrary.MEDIATYPE_COUNT; ++i) {
      try {
        storedSyncPlaylistList =
            this._deviceLibrary.getSyncPlaylistListByType(i);
      }
      catch (e if e.result == Cr.NS_ERROR_NOT_IMPLEMENTED) {
        // Expected for types not synced by playlists
        continue;
      }

      for (j = 0; j < storedSyncPlaylistList.length; ++j) {
        syncPlaylistML = storedSyncPlaylistList.queryElementAt(j,
                                                               Ci.sbIMediaList);
        if (syncPlaylistML.guid in syncPlaylistList) {
          syncPlaylistList[syncPlaylistML.guid].value = true;
          // The playlist content type can change when device is
          // disconnected. Update the preference when reconnect.
          var contentType =
              this._getListContentType(
                  syncPlaylistList[syncPlaylistML.guid].contentType);
          if (i != mediaType &&
              (contentType != Ci.sbIMediaList.CONTENTTYPE_MIX)) {
            this._deviceLibrary.addToSyncPlaylistList(mediaType,
                                                      syncPlaylistML);
            this._deviceLibrary.removeFromSyncPlaylistList(i,
                                                           syncPlaylistML);
          }
        }
      }
    }

    /* Make a copy of the stored sync prefs. */
    this._storedSyncPrefs = {};
    this.syncPrefsCopy(readPrefs, this._storedSyncPrefs);
  },


  /**
   * syncPrefsWrite
   *
   * \brief This function writes the working sync preferences to the preference
   * storage.
   */

  syncPrefsWrite: function DeviceSyncWidget_syncPrefsWrite()
  {
    var                         syncPlaylistList;
    var                         mediaList;
    var                         guid;

    var mediaType = this._getMediaType(this._mediaType);

    syncPlaylistList = this._syncPrefs.syncPlaylistList;
    for (guid in syncPlaylistList)
    {
      mediaList = LibraryUtils.mainLibrary.getMediaItem(guid);
      if (mediaList) {
        var listType =
            this._getListContentType(syncPlaylistList[guid].contentType);
        if (syncPlaylistList[guid].value) {
          // Add mix type media list to both audio and video.
          if (listType == Ci.sbIMediaList.CONTENTTYPE_MIX) {
            for (var i = 0; i < Ci.sbIDeviceLibrary.MEDIATYPE_COUNT; ++i) {
              try {
                this._deviceLibrary.addToSyncPlaylistList(i, mediaList);
              }
              catch (e if e.result == Cr.NS_ERROR_NOT_IMPLEMENTED) {
                // Expected for types not synced by playlists
              }
            }
          }
          // Otherwise, only add to the current media type.
          else {
            this._deviceLibrary.addToSyncPlaylistList(mediaType, mediaList);
          }
        }
        else {
          // Remove mix type media list from both audio and video.
          if (listType == Ci.sbIMediaList.CONTENTTYPE_MIX) {
            for (var i = 0; i < Ci.sbIDeviceLibrary.MEDIATYPE_COUNT; ++i) {
              try {
                this._deviceLibrary.removeFromSyncPlaylistList(i, mediaList);
              }
              catch (e if e.result == Cr.NS_ERROR_NOT_IMPLEMENTED) {
                // Expected for types not synced by playlists
              }
            }
          }
          // Otherwise, only remove from the current media type.
          else {
            this._deviceLibrary.removeFromSyncPlaylistList(mediaType, mediaList);
          }
        }
      }
    }

    /* Write the management type preference. */
    this._deviceLibrary.setMgmtType(mediaType, 
                                    this._syncPrefs.mgmtType.value &
                                    ~Ci.sbIDeviceLibrary.MGMT_TYPE_MANUAL);
  },

  /**
   * syncPrefsMgmtTypeIsAll
   *
   * \brief Checks if the management type for this contentType is ALL
   * \returns true if the MGMT_TYPE_ is set to ALL for the contentType
   */

  syncPrefsMgmtTypeIsAll: function DeviceSyncWidget_syncPrefsMgmtTypeIsAll()
  {
    return !!(this._syncPrefs.mgmtType.value &
              Ci.sbIDeviceLibrary.MGMT_TYPE_SYNC_ALL);
  },

  /**
   * syncPrefsMgmtTypeIsManual
   *
   * \brief Checks if the management type for this contentType is MANUAL
   * \returns true if the MGMT_TYPE_ is set to MANUAL for the contentType
   */

  syncPrefsMgmtTypeIsManual: function DeviceSyncWidget_syncPrefsMgmtTypeIsManual()
  {
    return !!(this._syncPrefs.mgmtType.value &
              Ci.sbIDeviceLibrary.MGMT_TYPE_MANUAL);
  },

  /**
   * syncPrefsApply
   *
   * \brief This function applies the working preference set to the preference
   * UI.
   */

  syncPrefsApply: function DeviceSyncWidget_syncPrefsApply()
  {
    var                         syncPlaylistTreeCell;
    var                         syncPlaylistList;
    var                         guid;
    var self = this;

    /* Get the management type pref UI elements. */
    var selector = this._getElement("content_selector");
    var syncRadioGroup = this._getElement("content_auto_sync_type_radio_group");
    var syncPlaylistTree = this._getElement("content_auto_sync_playlist_tree");
    var manualMessage = this._getElement("manual-mode-descr");

    /* Apply management type prefs. */
    function selectRadio(id) {
      syncRadioGroup.selectedItem = self._getElement(id);
    }
    function boolprop(prop) {
      return function(node, state) {
        if (state)
          node.setAttribute(prop, "true");
        else
          node.removeAttribute(prop);
      };
    }
    var disable = boolprop("disabled");
    var collapse = boolprop("collapsed");

    collapse(manualMessage, !this.syncPrefsMgmtTypeIsManual());
    disable(this._widget, this._isBusy || this.syncPrefsMgmtTypeIsManual());
    disable(syncPlaylistTree,
            this.syncPrefsMgmtTypeIsAll() || this.syncPrefsMgmtTypeIsManual());
    selectRadio(this.syncPrefsMgmtTypeIsAll() ? "content_auto_sync_all_radio"
                                              : "content_auto_sync_selected_radio");

    /* Apply the sync playlist list prefs. */
    syncPlaylistList = this._syncPrefs.syncPlaylistList;
    for (guid in syncPlaylistList)
    {
      /* Get the sync playlist tree checkbox cell */
      syncPlaylistTreeCell =
        this._getElement("content_sync_playlist_checkcell." + guid);
      if (!syncPlaylistTreeCell)
        continue;

      /* Apply the preference. */
      syncPlaylistTreeCell.setAttribute("value", syncPlaylistList[guid].value);
    }

    // Update the UI.
    this.update();
  },

  /**
   * syncPrefsSetMgmtType
   *
   * \brief This function sets the correct mgmt type for the content, without
   * affecting the other ones.
   */

  syncPrefsSetMgmtType:
    function DeviceSyncWidget_syncPrefsSetMgmtType(isSyncAll)
  {
    var mgmtType = this._syncPrefs.mgmtType.value;

    var mask = ~(Ci.sbIDeviceLibrary.MGMT_TYPE_SYNC_ALL |
                 Ci.sbIDeviceLibrary.MGMT_TYPE_SYNC_PLAYLISTS);

    mgmtType &= mask;

    if (isSyncAll)
      mgmtType |= Ci.sbIDeviceLibrary.MGMT_TYPE_SYNC_ALL;
    else
      mgmtType |= Ci.sbIDeviceLibrary.MGMT_TYPE_SYNC_PLAYLISTS;

    this._syncPrefs.mgmtType.value = mgmtType;
  },

  /**
   * syncPrefsExtract
   *
   * \brief This function extracts the preference settings from the preference
   *        UI and writes them to the working preference set.
   */

  syncPrefsExtract: function DeviceSyncWidget_syncPrefsExtract()
  {
      var                         syncPlaylistList;
      var                         syncPlaylistTreeCell;
      var                         mgmtAll;

      mgmtAll = this._getElement("content_auto_sync_all_radio").selected;
      this.syncPrefsSetMgmtType(this._mediaType == "video" ? false : mgmtAll);

      /* Extract the sync playlist list preferences. */
      syncPlaylistList = this._syncPrefs.syncPlaylistList;
      for (guid in syncPlaylistList)
      {
          /* Get the sync playlist list item. */
          syncPlaylistTreeCell =
            this._getElement("content_sync_playlist_checkcell." + guid);

          /* Extract the preference. */
          if (syncPlaylistTreeCell &&
              (syncPlaylistTreeCell.getAttribute("value") == "true"))
              syncPlaylistList[guid].value = true;
          else
              syncPlaylistList[guid].value = false;
      }
  },


  /**
   * syncPrefsChanged
   *
   * \param aPrefs1,               Preferences to compare.
   * \param aPrefs2
   *
   * \return                       True if any preference value changed.
   *
   * \brief This function compares the set of preferences specified by aPrefs1
   * and aPrefs2 to determine whether any preference values changed.  If any
   * preference value changed, this function returns true; otherwise, it returns
   * false.
   */

  syncPrefsChanged: function DeviceSyncWidget_syncPrefsChanged(aPrefs1, aPrefs2)
  {
      var                         guid;

      /* Check for changes to management type. */
      if (aPrefs1.mgmtType.value != aPrefs2.mgmtType.value)
          return true;

      /* Check for changes to the sync playlist list preferences. */
      for (guid in aPrefs1.syncPlaylistList)
      {
          /* Ignore differences if one of the playlist prefs doesn't exist. */
          if (!aPrefs2.syncPlaylistList[guid])
              continue;

          /* Check if playlist sync preference changed. */
          if (aPrefs1.syncPlaylistList[guid].value !=
              aPrefs2.syncPlaylistList[guid].value)
          {
              return true;
          }
      }

      return false;
  },


  /**
   * syncPrefsCopy
   *
   * \param aSrcPrefs              Copy source preferences.
   * \param aDstPrefs              Copy destination preferences.
   *
   * \brief This function copies the preference set specified by aSrcPrefs to
   * the preference set specified by aDstPrefs.  This function copies the
   * preference values as well as all other preference fields.
   */

  syncPrefsCopy: function DeviceSyncWidget_syncPrefsCopy(aSrcPrefs, aDstPrefs)
  {
      var                         field;

      /* Copy each preference field. */
      for (field in aSrcPrefs)
      {
          if (aSrcPrefs[field] instanceof Object)
          {
              aDstPrefs[field] = {};
              this.syncPrefsCopy(aSrcPrefs[field], aDstPrefs[field]);
          }
          else
          {
              aDstPrefs[field] = aSrcPrefs[field];
          }
      }
  },


  /**
   * syncPrefsCopyValues
   *
   * \param aSrcPrefs              Copy source preferences.
   * \param aDstPrefs              Copy destination preferences.
   *
   * \brief This function copies the preference values from the preference set
   * specified by aSrcPrefs to the preference set specified by aDstPrefs.  This
   * function does not copy any other preference fields.  In addition, if the
   * destination preference set does not contain a preference contained in the
   * source, the preference value is not copied.
   */

  syncPrefsCopyValues: function DeviceSyncWidget_syncPrefsCopyValues(aSrcPrefs,
                                                                     aDstPrefs)
  {
      var                         field;

      /* Copy each preference field. */
      for (field in aSrcPrefs)
      {
          if (aSrcPrefs[field] instanceof Object)
          {
              if (typeof(aDstPrefs[field]) != "undefined")
                  this.syncPrefsCopyValues(aSrcPrefs[field], aDstPrefs[field]);
          }
          else
          {
              if (field == "value")
                  aDstPrefs[field] = aSrcPrefs[field];
          }
      }
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
