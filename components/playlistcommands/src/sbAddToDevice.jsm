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

Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://app/jsmodules/DropHelper.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/SBUtils.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");


const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

const SB_MEDIALISTDUPLICATEFILTER_CONTRACTID =
  "@songbirdnest.com/Songbird/Library/medialistduplicatefilter;1";

const ADDTODEVICE_MENU_TYPE      = "submenu";
const ADDTODEVICE_MENU_ID        = "library_cmd_addtodevice";
const ADDTODEVICE_MENU_NAME      = "&command.addtodevice";
const ADDTODEVICE_MENU_TOOLTIP   = "&command.tooltip.addtodevice";
const ADDTODEVICE_MENU_KEY       = "&command.shortcut.key.addtodevice";
const ADDTODEVICE_MENU_KEYCODE   = "&command.shortcut.keycode.addtodevice";
const ADDTODEVICE_MENU_MODIFIERS = "&command.shortcut.modifiers.addtodevice";


const ADDTODEVICE_COMMAND_ID = "library_cmd_addtodevice:";

EXPORTED_SYMBOLS = [ "addToDeviceHelper",
                     "SBPlaylistCommand_AddToDevice" ];

/**
 * \brief Creates a new unwrapper helper object which ensures
 *        downloadStatusTarget is always set when adding items
 *        to another library or playlist on a device.
 * \param aSelection An sbIMediaListViewSelection.selectedMediaItems enumerator
 * \return A new unwrapper helper object.
 */
function createUnwrapper(aSelection) {
  var unwrapper = Cc["@songbirdnest.com/Songbird/Library/EnumeratorWrapper;1"]
                    .createInstance(Ci.sbIMediaListEnumeratorWrapper);
  unwrapper.initialize(aSelection);
  
  return unwrapper;
}

// ----------------------------------------------------------------------------
// The "Add to device" dynamic command object
// ----------------------------------------------------------------------------
var SBPlaylistCommand_AddToDevice =
{
  m_Context: {
    m_Playlist: null,
    m_Window: null
  },

  m_addToDevice: null,

  m_root_commands :
  {
    m_Types: new Array
    (
      ADDTODEVICE_MENU_TYPE
    ),

    m_Ids: new Array
    (
      ADDTODEVICE_MENU_ID
    ),

    m_Names: new Array
    (
      ADDTODEVICE_MENU_NAME
    ),

    m_Tooltips: new Array
    (
      ADDTODEVICE_MENU_TOOLTIP
    ),

    m_Keys: new Array
    (
      ADDTODEVICE_MENU_KEY
    ),

    m_Keycodes: new Array
    (
      ADDTODEVICE_MENU_KEYCODE
    ),

    m_Enableds: new Array
    (
      true
    ),

    m_Modifiers: new Array
    (
      ADDTODEVICE_MENU_MODIFIERS
    ),

    m_PlaylistCommands: new Array
    (
      null
    ),

    m_DeviceIds: new Array
    (
      null
    )
  },

  _getMenu: function(aSubMenu)
  {
    var cmds;

    cmds = this.m_addToDevice.handleGetMenu(aSubMenu);
    if (cmds) return cmds;

    switch (aSubMenu) {
      default:
        cmds = this.m_root_commands;
        break;
    }
    return cmds;
  },

  getVisible: function( aHost )
  {
    return this.m_addToDevice.hasDevices();
  },

  getNumCommands: function( aSubMenu, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    return cmds.m_Ids.length;
  },

  getCommandId: function( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    if ( aIndex >= cmds.m_Ids.length ) return "";
    return cmds.m_Ids[ aIndex ];
  },

  getCommandType: function( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    if ( aIndex >= cmds.m_Ids.length ) return "";
    return cmds.m_Types[ aIndex ];
  },

  getCommandText: function( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    if ( aIndex >= cmds.m_Names.length ) return "";
    return cmds.m_Names[ aIndex ];
  },

  getCommandFlex: function( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    if ( cmds.m_Types[ aIndex ] == "separator" ) return 1;
    return 0;
  },

  getCommandToolTipText: function( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    if ( aIndex >= cmds.m_Tooltips.length ) return "";
    return cmds.m_Tooltips[ aIndex ];
  },

  getCommandValue: function( aSubMenu, aIndex, aHost )
  {
  },

  instantiateCustomCommand: function( aDocument, aId, aHost )
  {
    return null;
  },

  refreshCustomCommand: function( aElement, aId, aHost )
  {
  },

  getCommandVisible: function( aSubMenu, aIndex, aHost )
  {
    return true;
  },

  getCommandFlag: function( aSubmenu, aIndex, aHost )
  {
    return false;
  },

  getCommandChoiceItem: function( aChoiceMenu, aHost )
  {
    return "";
  },

  getCommandEnabled: function( aSubMenu, aIndex, aHost )
  {
    if (this.m_Context.m_Playlist.tree.currentIndex == -1) return false;

    var cmds = this._getMenu(aSubMenu);
    if (aSubMenu == ADDTODEVICE_MENU_ID) {
      var deviceRegistrar =
        Components.classes["@songbirdnest.com/Songbird/DeviceManager;2"]
                  .getService(Components.interfaces.sbIDeviceRegistrar);

      var device = deviceRegistrar.getDevice(cmds.m_DeviceIds[aIndex]);
      if (device.state == Components.interfaces.sbIDevice.STATE_MOUNTING) {
        // Don't enable the device command if the device is
        // currently mounting.
        return false;
      }
    }

    return cmds.m_Enableds[ aIndex ];
  },

  getCommandShortcutModifiers: function ( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    if ( aIndex >= cmds.m_Modifiers.length ) return "";
    return cmds.m_Modifiers[ aIndex ];
  },

  getCommandShortcutKey: function ( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    if ( aIndex >= cmds.m_Keys.length ) return "";
    return cmds.m_Keys[ aIndex ];
  },

  getCommandShortcutKeycode: function ( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    if ( aIndex >= cmds.m_Keycodes.length ) return "";
    return cmds.m_Keycodes[ aIndex ];
  },

  getCommandShortcutLocal: function ( aSubMenu, aIndex, aHost )
  {
    return true;
  },

  getCommandSubObject: function ( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    if ( aIndex >= cmds.m_PlaylistCommands.length ) return null;
    return cmds.m_PlaylistCommands[ aIndex ];
  },

  onCommand: function( aSubMenu, aIndex, aHost, id, value )
  {
    if ( id )
    {
      // ADDTODEVICE
      if (this.m_addToDevice.handleCommand(id)) return;

      // ...
    }
  },

  // The object registered with the sbIPlaylistCommandsManager interface acts
  // as a template for instances bound to specific playlist elements

  dupObject: function (obj) {
    var r = {};
    for ( var i in obj )
    {
      r[ i ] = obj[ i ];
    }
    return r;
  },

  duplicate: function()
  {
    var obj = this.dupObject(this);
    obj.m_Context = this.dupObject(this.m_Context);
    return obj;
  },

  initCommands: function(aHost) {
    this.m_addToDevice = new addToDeviceHelper();
    this.m_addToDevice.init(this);
  },

  shutdownCommands: function() {
    if (!this.m_addToDevice) {
      dump("this.m_addToDevice is null in SBPlaylistCommand_AddToDevice ?!!\n");
      return;
    }
    this.m_addToDevice.shutdown();
    this.m_addToDevice = null;
    this.m_Context = null;
  },

  setContext: function( context )
  {
    var playlist = context.playlist;
    var window = context.window;

    // Ah.  Sometimes, things are being secure.

    if ( playlist && playlist.wrappedJSObject )
      playlist = playlist.wrappedJSObject;

    if ( window && window.wrappedJSObject )
      window = window.wrappedJSObject;

    this.m_Context.m_Playlist = playlist;
    this.m_Context.m_Window = window;
  },

  QueryInterface : function(aIID)
  {
    if (!aIID.equals(Components.interfaces.sbIPlaylistCommands) &&
        !aIID.equals(Components.interfaces.nsISupportsWeakReference) &&
        !aIID.equals(Components.interfaces.nsISupports))
    {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }

    return this;
  }
}; // SBPlaylistCommand_AddToDevice declaration

function addToDeviceHelper() {
}

addToDeviceHelper.prototype = {
  m_listofdevices: null,
  m_commands: null,
  m_deviceManager: null,
  m_libraryManager: null,

  LOG: function(str) {
    var consoleService = Components.classes['@mozilla.org/consoleservice;1']
                            .getService(Components.interfaces.nsIConsoleService);
    consoleService.logStringMessage(str);
  },

  init: function addToDeviceHelper_init(aCommands) {
    this.m_libraryManager =
      Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
      .getService(Components.interfaces.sbILibraryManager);
    this.m_deviceManager =
      Components.classes["@songbirdnest.com/Songbird/DeviceManager;2"]
      .getService(Components.interfaces.sbIDeviceManager2);
    var eventTarget =
      this.m_deviceManager.QueryInterface(
        Components.interfaces.sbIDeviceEventTarget
      );
    eventTarget.addEventListener(this);
    this.m_commands = aCommands;
    this.makeListOfDevices();
  },

  shutdown: function addToDeviceHelper_shutdown() {
    var eventTarget =
      this.m_deviceManager.QueryInterface(
        Components.interfaces.sbIDeviceEventTarget
      );
    eventTarget.removeEventListener(this);
    this.m_deviceManager = null;
  },

  // returns true if we have at least one device in the list,
  // the commands object hides the "add to device" submenu when
  // no device is present
  hasDevices: function addToDeviceHelper_hasDevices() {
    return (this.m_listofdevices &&
            this.m_listofdevices.m_Types &&
            this.m_listofdevices.m_Types.length > 0);
  },

  // builds the list of devices, called on startup and when a
  // device is added or removed
  makeListOfDevices: function addToDeviceHelper_makeListOfDevices() {
    this._makingList = true;

    this.m_listofdevices = {};
    this.m_listofdevices.m_Types = new Array();
    this.m_listofdevices.m_Ids = new Array();
    this.m_listofdevices.m_Names = new Array();
    this.m_listofdevices.m_Tooltips = new Array();
    this.m_listofdevices.m_Enableds = new Array();
    this.m_listofdevices.m_Modifiers = new Array();
    this.m_listofdevices.m_Keys = new Array();
    this.m_listofdevices.m_Keycodes = new Array();
    this.m_listofdevices.m_PlaylistCommands = new Array();
    this.m_listofdevices.m_DeviceIds = new Array();

    // get all devices
    var registrar =
      this.m_deviceManager.QueryInterface(
        Components.interfaces.sbIDeviceRegistrar
      );
    var devices = Array();
    // turn into a js array
    for (var i=0;i<registrar.devices.length;i++) {
      var device = registrar.devices.queryElementAt
                                       (i, Components.interfaces.sbIDevice);
      if (device && device.connected)
        devices.push(device);
    }
    // order of devices returned by the registrar is undefined,
    // so sort by device name
    function deviceSorter(x, y) {
      var nameX = x.name;
      var nameY = y.name;
      if (x < y) return -1;
      if (y < x) return 1;
      return 0;
    }
    devices.sort(deviceSorter);
    // extract the device names and associated library guids
    // and fill the arrays used by the command object
    for (var d in devices) {
      var libraryguid;
      var device = devices[d];
      var devicename = device.name;

      // don't add devices that are read-only
      var deviceProperties = device.properties.properties;
      var accessCompatibility;
      try {
        accessCompatibility = deviceProperties.getPropertyAsAString(
            "http://songbirdnest.com/device/1.0#accessCompatibility");
      } catch (ex) {}
      if (accessCompatibility == "ro") {
        continue;
      }

      var isEnabled = false;
      if (!devicename)
        devicename = "Unnamed Device";
      var library = device.defaultLibrary;
      if (library) {
        isEnabled = library.userEditable;
        libraryguid = library.guid;
      } else {
        continue;
      }
      this.m_listofdevices.m_Types.push("action");
      this.m_listofdevices.m_Ids.push(ADDTODEVICE_COMMAND_ID +
                                      libraryguid + ";" +
                                      devicename);
      this.m_listofdevices.m_Names.push(devicename);
      this.m_listofdevices.m_Tooltips.push(devicename);
      this.m_listofdevices.m_Enableds.push(isEnabled);
      this.m_listofdevices.m_Modifiers.push("");
      this.m_listofdevices.m_Keys.push("");
      this.m_listofdevices.m_Keycodes.push("");
      this.m_listofdevices.m_PlaylistCommands.push(null);
      this.m_listofdevices.m_DeviceIds.push(device.id);
    }

    this._makingList = false;
  },

  handleGetMenu: function addToDeviceHelper_handleGetMenu(aSubMenu) {
    if (this.m_listofdevices == null) {
      // handleGetMenu called before makeListOfPlaylists, this would
      // cause infinite recursion : the command object would not find
      // the menu either, would return null to getMenu which corresponds
      // to the root menu, and it'd recurse infinitly.
      throw Components.results.NS_ERROR_FAILURE;
    }
    if (aSubMenu == ADDTODEVICE_MENU_ID) return this.m_listofdevices;
    return null;
  },

  // handle click on a device command item
  handleCommand: function addToDeviceHelper_handleCommand(id) {
    try {
      var context = this.m_commands.m_Context;
      var addtodevicestr = ADDTODEVICE_COMMAND_ID;
      if ( id.slice(0, addtodevicestr.length) == addtodevicestr) {
        var r = id.slice(addtodevicestr.length);
        var parts = r.split(';');
        if (parts.length >= 2) {
          var libraryguid = parts[0];
          var devicename = parts[1];
          this.addToDevice(libraryguid, context.m_Playlist, devicename);
          return true;
        }
      }
    } catch (e) {
      Components.utils.reportError(e);
    }
    return false;
  },

  // perform the transfer of the selected items to the device library
  addToDevice: function addToDeviceHelper_addToDevice(
                          devicelibraryguid,
                          sourceplaylist,
                          devicename) {
    var library = this.m_libraryManager.getLibrary(devicelibraryguid);
    if (library) {
      var oldLength = library.length;
      var selection =
        sourceplaylist.mediaListView.selection.selectedMediaItems;

      // Create an enumerator that wraps the enumerator we were handed since
      // the enumerator we get hands back sbIIndexedMediaItem, not just plain
      // 'ol sbIMediaItems

      // Create a media item duplicate enumerator filter to count the number of
      // duplicate items and to remove them from the enumerator if the target is
      // a library.
      let dupFilter =
        Cc[SB_MEDIALISTDUPLICATEFILTER_CONTRACTID]
        .createInstance(Ci.sbIMediaListDuplicateFilter);
      dupFilter.initialize(selection,
                           library,
                           true);
 
      // Create a filtered item enumerator.

      // We also want to set the downloadStatusTarget property as we work.
      var unwrapper = createUnwrapper(dupFilter);

      var asyncListener = {
       _dupFilter: dupFilter,
       onProgress: function(aItemsProcessed, aComplete) {
         if (aComplete) {
           var added = this._dupFilter.totalItems - this._dupFilter.duplicateItems;
           DNDUtils.reportAddedTracks(added,
                                      this._dupFilter.duplicateItems,
                                      0, /* no unsupported reporting */
                                      devicename,
                                      true);
         }
         else {
           DNDUtils.reportAddedTracks(aItemsProcessed,
                                      0, /* no dupe reporting until complete */
                                      0, /* no unsupported reporting */
                                      devicename,
                                      true);
         }
       },
       QueryInterface: XPCOMUtils.generateQI([Ci.sbIMediaListAsyncListener])
      }
         
      library.addSomeAsync(unwrapper, asyncListener);
    }
  },

  //-----------------------------------------------------------------------------
  // methods for refreshing the list when needed, detection is performed via
  // an sbIDeviceEventListener on the device manager
  //-----------------------------------------------------------------------------

  _makingList    : false,

  refreshCommands: function addToDeviceHelper_refreshCommands() {
    if (this.m_commands) {
      if (this.m_commands.m_Context && this.m_commands.m_Context.m_Playlist) {
        this.makeListOfDevices();
        this.m_commands.m_Context.m_Playlist.refreshCommands();
      }
    }
  },

  onUpdateEvent: function addToDeviceHelper_onUpdateEvent() {
    if (this._makingList) return;
    this.refreshCommands();
  },

  QueryInterface: function QueryInterface(iid) {
    if (!iid.equals(Components.interfaces.sbIDeviceEventListener) &&
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  },
 
  // trap event for device library added/removed, and refresh the commands
  // we trap the library add/remove (instead of the plain device add/remove)
  // since we check the library count in makeListOfDevices(), so we need to
  // make sure we don't run before the libraries have been added to the device
  onDeviceEvent: function addToDeviceHelper_onDeviceEvent(aEvent) {
    if (aEvent.type == Components.interfaces.sbIDeviceEvent.EVENT_DEVICE_LIBRARY_ADDED ||
        aEvent.type == Components.interfaces.sbIDeviceEvent.EVENT_DEVICE_LIBRARY_REMOVED ||
        aEvent.type == Components.interfaces.sbIDeviceEvent.EVENT_DEVICE_MOUNTING_END) {
      this.onUpdateEvent();
    }
  }

};
