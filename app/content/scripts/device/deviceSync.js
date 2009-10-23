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

if (typeof(LibraryUtils) == "undefined")
  Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");

if (typeof(SBProperties) == "undefined")
  Cu.import("resource://app/jsmodules/sbProperties.jsm");

if (typeof(XUL_NS) == "undefined")
  var XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

var DeviceSyncWidget = {
  //
  // Device sync object fields.
  //
  //   _widget                  Device sync widget.
  //   _deviceLibrary           Device library we are working with.
  //   _isIdle                  Flag for state of device.
  //

  _widget: null,
  _deviceLibrary: null,
  _isIdle: true,

  /**
   * \brief Initialize the device progress services for the device progress
   *        widget specified by aWidget.
   *
   * \param aWidget             Device progress widget.
   */

  initialize: function DeviceSyncWidget__initialize(aWidget) {
    // Get the sync widget.
    this._widget = aWidget;
    
    // Initialize object fields.
    this._device = this._widget.device;

    // Get the device library we are dealing with
    // Currently we just grab the first one since we only deal with one library
    // XXX When we deal with multiple libraries we will need to change this
    //     so that it can handle a particular library rather than defaulting
    //     to the first one.
    this._deviceLibrary = this._device.content.libraries
                              .queryElementAt(0, Ci.sbIDeviceLibrary);

    // Initialize, read, and apply the music preferences.
    this.musicPrefsInitialize();
    this.musicPrefsRead();
    this.musicPrefsApply();

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

  //----------------------------------------------------------------------------
  //
  // Device sync event handler services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Handle the event specified by aEvent for elements with defined
   *        actions.
   *
   * \param aEvent              Event to handle.
   */

  onAction: function DeviceSyncWidget_onAction(aEvent) {
    // Dispatch processing of action.
    switch (aEvent.target.getAttribute("action")) {
      case "cancel" :
        // Re-read and apply the music preferences.
        this.musicPrefsRead();
        this.musicPrefsApply();
        break;

      case "save" :
        // Extract and write the preferences.
        this.musicPrefsExtract();
        this.musicPrefsWrite();

        // Re-read and apply the preferences in case the user cancels a switch
        // to sync mode.
        this.musicPrefsRead();
        this.musicPrefsApply();

        break;

      default :
        break;
    }
  },
  
  onUIPrefChange: function DeviceSyncWidget_onUIPrefChange() {
    /* Extract and reapply the preferences. */
    this.musicPrefsExtract();
    this.musicPrefsApply();
  },


  /**
   * \brief Handle changes to the library playlists.
   */

  _onPlaylistChange: function DeviceSyncWidget_onPlaylistChange() {
    // Make a copy of the current sync preferences.
    var prevSyncPrefs = {};
    this.musicPrefsCopy(this._syncPrefs, prevSyncPrefs);

    // Re-initialize the sync preferences.
    this.musicPrefsInitialize();

    // Re-read the preferences to update the stored preference set.
    this.musicPrefsRead();

    // Copy previous preference values and apply them.
    this.musicPrefsCopyValues(prevSyncPrefs, this._syncPrefs);
    this.musicPrefsApply();
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
        // If any sync preferences changed, re-read and apply the preferences.
        var prevSyncPrefs = {};
        var curSyncPrefs = {};
        this.musicPrefsCopy(this._storedSyncPrefs, prevSyncPrefs);
        this.musicPrefsCopy(this._storedSyncPrefs, curSyncPrefs);
        this.musicPrefsRead(curSyncPrefs);
        if (this.musicPrefsChanged(prevSyncPrefs, curSyncPrefs)) {
          this.musicPrefsRead();
          this.musicPrefsApply();
        }
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
    // Handle playlist changes.
    if (aMediaItem.getProperty(SBProperties.isList))
        this._onPlaylistChange();

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
   * \brief Called before a media list is cleared.
   * \return True if you do not want any further onListCleared notifications
   *         for the current batch.  If there is no current batch, the return
   *         value is ignored.
   */

  onBeforeListCleared: function DeviceSyncWidget_onBeforeListCleared(aMediaList) {
    return true;
  },

  /**
   * \brief Called when a media list is cleared.
   * \return True if you do not want any further onListCleared notifications
   *         for the current batch.  If there is no current batch, the return
   *         value is ignored.
   */

  onListCleared: function DeviceSyncWidget_onListCleared(aMediaList) {
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
   * \brief selects the radio element specified by aRadioID.
   *
   * \param aRadioID               ID of radio to select.
   *
   */
  
  _selectRadio : function DeviceSyncWidget__selectRadio(aRadioID)
  {
      var radioElem;
  
      /* Get the radio element. */
      radioElem = this._getElement(aRadioID);
  
      /* Select the radio. */
      radioElem.radioGroup.selectedItem = radioElem;
  },

  /* *****************************************************************************
   *
   * device sync preference services.
   *
   ******************************************************************************/
  
  /*
   * _syncPrefs                  Working music preferences.
   * _storedSyncPrefs            Stored music preferences.
   *
   *   The current working set of music preferences are maintained in _syncPrefs.
   * These are the preferences that the user has set and that is represented by
   * the UI but has not neccessarily been written to the preference storage.
   *   The set of music preferences in the preference storage is maintained in
   * _storedSyncPrefs.  This is used to determine whether any preferences have
   * been changed and need to be written to the preference storage.
   */
  
  _syncPrefs : null,
  _storedSyncPrefs : null,


  /*
   * musicPrefsInitialize
   *
   * \breif This function initializes the music preference services.
   */

  musicPrefsInitialize: function DeviceSyncWidget_musicPrefsInitialize()
  {

      /* Initialize the working preference set. */
      this.musicPrefsInitPrefs();
  
      /* Initialize the preference UI. */
      this.musicPrefsInitUI();
  },


  /*
   * musicPrefsFinalize
   *
   * \breif This function finalizes the music preference services.
   */

  musicPrefsFinalize: function DeviceSyncWidget_musicPrefsFinalize()
  {
      /* Clear object fields. */
      this._syncPrefs = null;
      this._storedSyncPrefs = null;
  },


  /*
   * musicPrefsInitPrefs
   *
   * \brief This function initializes the working music preference set.  It sets up the
   * working music preference object with all of the preference fields and sets
   * default values.  It does not read the preferences from the preference
   * storage.
   */

  musicPrefsInitPrefs: function DeviceSyncWidget_musicPrefsInitPrefs()
  {
      var                         mediaListList;
      var                         mediaList;
      var                         mediaListArray;
      var                         guid;
      var                         readableName;
      var                         pref;
      var                         i;

      /* Clear the music preferences. */
      this._syncPrefs = {};
  
      /* Create and initialize the management type preference. */
      this._syncPrefs.mgmtType = {};
      this._syncPrefs.mgmtType.value = Ci.sbIDeviceLibrary.MGMT_TYPE_MANUAL;
  
      /* Create the sync playlist list preference. */
      this._syncPrefs.syncPlaylistList = {};
  
      /* Get the list of library media lists. */
      mediaListArray = new LibraryUtils.MediaListEnumeratorToArray();
      LibraryUtils.mainLibrary.enumerateItemsByProperty
                                      (SBProperties.isList,
                                       "1",
                                       mediaListArray,
                                       1);
      mediaListList = mediaListArray.array;

      /* Fill in the sync playlist list preferences. */
      for (i = 0; i < mediaListList.length; i++)
      {
          /* Get the media list info. */
          mediaList = mediaListList[i];
          
          // Skip if it's a hidden playlist
          if (mediaList.getProperty(SBProperties.hidden) == "1")
            continue;
            
          var mCustomType = mediaList.getProperty(SBProperties.customType);
          if (mCustomType != "download") {
            guid = mediaList.guid;
            readableName = mediaList.name;
  
            /* Set up the sync playlist preference. */
            pref = {};
            pref.guid = guid;
            pref.readableName = readableName;
            pref.value = false;
  
            /* Add the preference to the sync playlist list preference. */
            this._syncPrefs.syncPlaylistList[guid] = pref;
          }
      }
  },


  /*
   * musicPrefsInitUI
   *
   * \breif This function initializes the music preference UI elements to match the
   * set of working music preference fields.  It does not apply the preference
   * values to the UI.
   */

  musicPrefsInitUI: function DeviceSyncWidget_musicPrefsInitUI()
  {
      var                         syncPlaylistListBox;
      var                         syncPlaylistList;
      var                         listItem;
      var                         guid;
      var                         readableName;
  
      /* Get the sync playlist list box and prefs. */
      syncPlaylistListBox = this._getElement("content_auto_sync_playist_listbox");
      syncPlaylistList = this._syncPrefs.syncPlaylistList;
  
      /* Clear the sync playlist list box. */
      while (syncPlaylistListBox.firstChild)
          syncPlaylistListBox.removeChild(syncPlaylistListBox.firstChild);
  
      /* Fill in the sync playlist list box. */
      for (guid in syncPlaylistList)
      {
          /* Get the sync playlist info. */
          readableName = syncPlaylistList[guid].readableName;
  
          /* Create a new list item. */
          listItem = document.createElementNS(XUL_NS, "listitem");
          listItem.value = guid;
          listItem.setAttribute("sbid", "content_sync_playlist." + guid);
          listItem.setAttribute("label", readableName);
          listItem.setAttribute("type", "checkbox");
  
          /* Add the list item to the list. */
          syncPlaylistListBox.appendChild(listItem);
      }
  },


  /*
   * musicPrefsRead
   *
   * \brief This function reads the music preferences from the preference
   *        storage into the preferences object specified by aPrefs.  If a
   *        preferences object is not specified, this function reads the
   *        preferences into the working music preferences.  In addition, this
   *        function updates the stored music preferences object.
   */

  musicPrefsRead: function DeviceSyncWidget_musicPrefsRead(aPrefs)
  {
      var                         readPrefs;
      var                         storedSyncPlaylistList;
      var                         syncPlaylistList;
      var                         syncPlaylistML;
      var                         guid;
      var                         i;

      /* If no preference object is specified, */
      /* read into the working preferences.    */
      if (aPrefs)
          readPrefs = aPrefs;
      else
          readPrefs = this._syncPrefs;

      /* Read the management type preference. */
      readPrefs.mgmtType.value = this._deviceLibrary.mgmtType;
      
      /* Read the stored sync playlist list preferences. */
      storedSyncPlaylistList = this._deviceLibrary.getSyncPlaylistList();

      /* Clear and read the sync playlist list preferences. */
      syncPlaylistList = readPrefs.syncPlaylistList;
      for (guid in syncPlaylistList)
          syncPlaylistList[guid].value = false;
      for (i = 0; i < storedSyncPlaylistList.length; i++)
      {
          syncPlaylistML = storedSyncPlaylistList.queryElementAt(i,
                                                              Ci.sbIMediaList);
          if (syncPlaylistML.guid in syncPlaylistList)
              syncPlaylistList[syncPlaylistML.guid].value = true;
      }
  
      /* Make a copy of the stored music prefs. */
      this._storedSyncPrefs = {};
      this.musicPrefsCopy(readPrefs, this._storedSyncPrefs);
  },


  /*
   * musicPrefsWrite
   *
   * \brief This function writes the working music preferences to the preference
   * storage.
   */

  musicPrefsWrite: function DeviceSyncWidget_musicPrefsWrite()
  {
      var                         storePlaylistList;
      var                         syncPlaylistList;
      var                         mediaList;
      var                         guid;

      /* we must read only the playlist list preference array before writing
       * anything to the prefs, otherwise the act of writing will go and clobber
       * our changes.
       */
      /* Set up the store playlist list preference array. */
      storePlaylistList =
                  Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"].createInstance(Ci.nsIMutableArray);
      syncPlaylistList = this._syncPrefs.syncPlaylistList;
      for (guid in syncPlaylistList)
      {
          if (syncPlaylistList[guid].value)
          {
              /* Get the playlist media list.  If it */
              /* no longer exists, don't store it.   */
              mediaList = LibraryUtils.mainLibrary.getMediaItem(guid);
              if (mediaList)
                  storePlaylistList.appendElement(mediaList, false);
          }
      }

      /* Write the management type preference. */
      this._deviceLibrary.mgmtType = this._syncPrefs.mgmtType.value;
  
      /* Write the sync playlist list preferences. */
      this._deviceLibrary.setSyncPlaylistList(storePlaylistList);
  },


  /*
   * musicPrefsApply
   *
   * \brief This function applies the working preference set to the preference UI.
   */

  musicPrefsApply: function DeviceSyncWidget_musicPrefsApply()
  {
      var                         syncRadioGroup;
      var                         syncPlaylistListBox;
      var                         syncPlaylistList;
      var                         cancelButton;
      var                         saveButton;
      var                         listItem;
      var                         guid;
  
      /* Get the management type pref UI elements. */
      syncRadioGroup = this._getElement("content_auto_sync_type_radio_group");
      syncPlaylistListBox= this._getElement("content_auto_sync_playist_listbox");
  
      /* Apply management type prefs. */
      syncRadioGroup.disabled = true;
      syncPlaylistListBox.disabled = true;
      switch (this._syncPrefs.mgmtType.value)
      {
          case Ci.sbIDeviceLibrary.MGMT_TYPE_MANUAL :
              this._selectRadio("content_manual_radio");
              break;
  
          case Ci.sbIDeviceLibrary.MGMT_TYPE_SYNC_ALL :
              this._selectRadio("content_auto_sync_radio");
              this._selectRadio("content_auto_sync_all_radio");
              syncRadioGroup.disabled = false;
              break;
  
          case Ci.sbIDeviceLibrary.MGMT_TYPE_SYNC_PLAYLISTS :
              this._selectRadio("content_auto_sync_radio");
              this._selectRadio("content_auto_sync_selected_radio");
              syncRadioGroup.disabled = false;
              syncPlaylistListBox.disabled = false;
              break;
  
          default :
              break;
      }
  
      /* Apply the sync playlist list prefs. */
      syncPlaylistList = this._syncPrefs.syncPlaylistList;
      for (guid in syncPlaylistList)
      {
          /* Get the sync playlist list item. */
          listItem = this._getElement("content_sync_playlist." + guid);
          if (!listItem)
              continue;
  
          /* Apply the preference. */
          if (syncPlaylistList[guid].value)
              listItem.setAttribute("checked", "true");
          else
              listItem.setAttribute("checked", "false");
      }
      
      this.musicPrefsUpdateButtons();
  },

  /*
   * musicPrefsUpdateButtons
   *
   * \brief This function updates the cancel and save buttons to the proper
   *        states.
   */

  musicPrefsUpdateButtons: function DeviceSyncWidget_musicPrefsUpdateButtons()
  {
    /* Show the save and cancel buttons if the      */
    /* prefs have changed; otherwise, disable them. */
    var cancelButton = this._getElement("cancel_button");
    var saveButton = this._getElement("save_button");
    if (this.musicPrefsChanged(this._syncPrefs, this._storedSyncPrefs))
    {
        cancelButton.disabled = false;
        saveButton.disabled = false;
    }
    else
    {
        cancelButton.disabled = true;
        saveButton.disabled = true;
    }
  },

  /*
   * musicPrefsExtract
   *
   * \brief This function extracts the preference settings from the preference
   *        UI and writes them to the working preference set.
   */

  musicPrefsExtract: function DeviceSyncWidget_musicPrefsExtract()
  {
      var                         mgmtType;
      var                         syncPlaylistList;
      var                         listItem;
  
      /* Extract the management type preference. */
      if (this._getElement("content_auto_sync_radio").selected) {
        if (this._getElement("content_auto_sync_all_radio").selected)
            mgmtType = Ci.sbIDeviceLibrary.MGMT_TYPE_SYNC_ALL;
        else if (this._getElement("content_auto_sync_selected_radio").selected)
            mgmtType = Ci.sbIDeviceLibrary.MGMT_TYPE_SYNC_PLAYLISTS;
      } else {
          mgmtType = Ci.sbIDeviceLibrary.MGMT_TYPE_MANUAL;
      }
      this._syncPrefs.mgmtType.value = mgmtType;
      
      /* Extract the sync playlist list preferences. */
      syncPlaylistList = this._syncPrefs.syncPlaylistList;
      for (guid in syncPlaylistList)
      {
          /* Get the sync playlist list item. */
          listItem = this._getElement("content_sync_playlist." + guid);
  
          /* Extract the preference. */
          if (listItem && (listItem.getAttribute("checked") == "true"))
              syncPlaylistList[guid].value = true;
          else
              syncPlaylistList[guid].value = false;
      }
  },


  /*
   * musicPrefsChanged
   *
   * \param aPrefs1,               Preferences to compare.
   * \param aPrefs2
   *
   * \return                       True if any preference value changed.
   *
   * \brief This function compares the set of preferences specified by aPrefs1 and
   * aPrefs2 to determine whether any preference values changed.  If any
   * preference value changed, this function returns true; otherwise, it returns
   * false.
   */

  musicPrefsChanged: function DeviceSyncWidget_musicPrefsChanged(aPrefs1, aPrefs2)
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


  /*
   * musicPrefsCopy
   *
   * \param aSrcPrefs              Copy source preferences.
   * \param aDstPrefs              Copy destination preferences.
   *
   * \brief This function copies the preference set specified by aSrcPrefs to the
   * preference set specified by aDstPrefs.  This function copies the preference
   * values as well as all other preference fields.
   */

  musicPrefsCopy: function DeviceSyncWidget_musicPrefsCopy(aSrcPrefs, aDstPrefs)
  {
      var                         field;
  
      /* Copy each preference field. */
      for (field in aSrcPrefs)
      {
          if (aSrcPrefs[field] instanceof Object)
          {
              aDstPrefs[field] = {};
              this.musicPrefsCopy(aSrcPrefs[field], aDstPrefs[field]);
          }
          else
          {
              aDstPrefs[field] = aSrcPrefs[field];
          }
      }
  },


  /*
   * musicPrefsCopyValues
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

  musicPrefsCopyValues: function DeviceSyncWidget_musicPrefsCopyValues(aSrcPrefs, aDstPrefs)
  {
      var                         field;
  
      /* Copy each preference field. */
      for (field in aSrcPrefs)
      {
          if (aSrcPrefs[field] instanceof Object)
          {
              if (typeof(aDstPrefs[field]) != "undefined")
                  this.musicPrefsCopyValues(aSrcPrefs[field], aDstPrefs[field]);
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
