// JScript source code
/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

var Cc = Components.classes;
var Ci = Components.interfaces;
var Cr = Components.results;
var Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/components/ArrayConverter.jsm");
Cu.import("resource://app/components/sbProperties.jsm");
Cu.import("resource://app/components/kPlaylistCommands.jsm");
Cu.import("resource://app/components/sbAddToPlaylist.jsm");
Cu.import("resource://app/components/sbLibraryUtils.jsm");

const WEB_PLAYLIST_CONTEXT      = "webplaylist";
const WEB_PLAYLIST_TABLE        = "webplaylist";
const WEB_PLAYLIST_TABLE_NAME   = "&device.webplaylist";
const WEB_PLAYLIST_LIBRARY_NAME = "&device.weblibrary";

function SelectionUnwrapper(selection) {
  this._selection = selection;
}

SelectionUnwrapper.prototype = {
  _selection: null,
  
  hasMoreElements : function() {
    return this._selection.hasMoreElements();
  },
  
  getNext : function() {
    return this._selection.getNext().mediaItem;
  },
  
  QueryInterface : function(iid) {
    if (iid.equals(Ci.nsISimpleEnumerator) ||
        iid.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_NOINTERFACE;
  }
}

function PublicPlaylistCommands() {
  this.m_mgr = Components.
    classes["@songbirdnest.com/Songbird/PlaylistCommandsManager;1"]
    .getService(Components.interfaces.sbIPlaylistCommandsManager);

  var obs = Components.classes["@mozilla.org/observer-service;1"]
                      .getService(Components.interfaces.nsIObserverService);
  obs.addObserver(this, "final-ui-startup", false);
}

PublicPlaylistCommands.prototype.constructor = PublicPlaylistCommands;

PublicPlaylistCommands.prototype = {

  classDescription: "Songbird Public Playlist Commands",
  classID:          Components.ID("{1126ee77-2d85-4f79-a07a-b014da404e53}"),
  contractID:       "@songbirdnest.com/Songbird/PublicPlaylistCommands;1",
  
  m_defaultCommands               : null,
  m_webPlaylistCommands           : null,
  m_downloadCommands              : null,
  m_downloadToolbarCommands       : null,
  m_serviceTreeDefaultCommands    : null,
  m_mgr                           : null,

  // Define various playlist commands, they will be exposed to the playlist commands
  // manager so that they can later be retrieved and concatenated into bigger
  // playlist commands objects.
  
  // Commands that act on playlist items
  m_cmd_Play                      : null, // play the selected track
  m_cmd_Remove                    : null, // remove the selected track(s)
  m_cmd_Edit                      : null, // edit the selected track(s)
  m_cmd_Download                  : null, // download the selected track(s)
  m_cmd_AddToLibrary              : null, // add the selection to the library
  m_cmd_CopyTrackLocation         : null, // copy the select track(s) location(s)
  m_cmd_ShowDownloadPlaylist      : null, // switch the outer playlist container to the download playlist
  m_cmd_ShowWebPlaylist           : null, // switch the outer playlist container to the web playlist
  m_cmd_PauseResumeDownload       : null, // auto-switching pause/resume track download
  m_cmd_CleanUpDownloads          : null, // clean up completed download items
  m_cmd_BurnToCD                  : null, // burn selected tracks to CD
  m_cmd_CopyToDevice              : null, // copy selected tracks to device

  // Commands that act on playlist themselves
  m_cmd_list_Remove               : null, // remove the selected playlist
  m_cmd_list_Rename               : null, // rename the selected playlist
  
  // ==========================================================================
  // INIT
  // ==========================================================================
  initCommands: function() {
    if (this.m_mgr.request(kPlaylistCommands.MEDIAITEM_DEFAULT)) {
      // already initialized ! bail out !
      return;
    }
    
    // Add an observer for the application shutdown event, so that we can
    // shutdown our commands

    var obs = Components.classes["@mozilla.org/observer-service;1"]
                        .getService(Components.interfaces.nsIObserverService);
    obs.removeObserver(this, "final-ui-startup");
    obs.addObserver(this, "quit-application", false);

    // --------------------------------------------------------------------------

    var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                          .getService(Components.interfaces.nsIPrefBranch2);
    
    // --------------------------------------------------------------------------

    // Build playlist command actions

    // --------------------------------------------------------------------------
    // The PLAY button is made of two commands, one that is always there for
    // all instantiators, the other that is only created by the shortcuts
    // instantiator. This makes one toolbar button and menu item, and two
    // shortcut keys.
    // --------------------------------------------------------------------------
    
    const PlaylistCommandsBuilder = new Components.
      Constructor("@songbirdnest.com/Songbird/PlaylistCommandsBuilder;1", 
                  "sbIPlaylistCommandsBuilder");

    this.m_cmd_Play = new PlaylistCommandsBuilder();

    // The first item, always created
    this.m_cmd_Play.appendAction(null, 
                                 "library_cmd_play",
                                 "&command.play",
                                 "&command.tooltip.play",
                                 plCmd_Play_TriggerCallback);
    
    this.m_cmd_Play.setCommandShortcut(null,
                                       "library_cmd_play",
                                       "&command.shortcut.key.play",
                                       "&command.shortcut.keycode.play",
                                       "&command.shortcut.modifiers.play",
                                       true);

    this.m_cmd_Play.setCommandEnabledCallback(null,
                                              "library_cmd_play",
                                              plCmd_IsAnyTrackSelected);

    // The second item, only created by the keyboard shortcuts instantiator
    this.m_cmd_Play.appendAction(null, 
                                 "library_cmd_play2",
                                 "&command.play",
                                 "&command.tooltip.play",
                                 plCmd_Play_TriggerCallback);

    this.m_cmd_Play.setCommandShortcut(null,
                                       "library_cmd_play2",
                                       "&command.shortcut.key.altplay",
                                       "&command.shortcut.keycode.altplay",
                                       "&command.shortcut.modifiers.altplay",
                                       true);

    this.m_cmd_Play.setCommandEnabledCallback(null,
                                              "library_cmd_play2",
                                              plCmd_IsAnyTrackSelected);

    this.m_cmd_Play.setCommandVisibleCallback(null,
                                              "library_cmd_play2",
                                              plCmd_IsShortcutsInstantiator);

    // --------------------------------------------------------------------------
    // The REMOVE button, like the play button, is made of two commands, one that 
    // is always there for all instantiators, the other that is only created by 
    // the shortcuts instantiator. This makes one toolbar button and menu item, 
    // and two shortcut keys.
    // --------------------------------------------------------------------------

    this.m_cmd_Remove = new PlaylistCommandsBuilder();

    // The first item, always created
    this.m_cmd_Remove.appendAction(null, 
                                   "library_cmd_remove",
                                   "&command.remove",
                                   "&command.tooltip.remove",
                                   plCmd_Remove_TriggerCallback);
    
    this.m_cmd_Remove.setCommandShortcut(null,
                                         "library_cmd_remove",
                                         "&command.shortcut.key.remove",
                                         "&command.shortcut.keycode.remove",
                                         "&command.shortcut.modifiers.remove",
                                         true);

    this.m_cmd_Remove.setCommandEnabledCallback(null,
                                                "library_cmd_remove",
                                                plCmd_IsAnyTrackSelected);

    // The second item, only created by the keyboard shortcuts instantiator
    this.m_cmd_Remove.appendAction(null, 
                                   "library_cmd_remove2",
                                   "&command.remove",
                                   "&command.tooltip.remove",
                                   plCmd_Remove_TriggerCallback);

    this.m_cmd_Remove.setCommandShortcut(null,
                                         "library_cmd_remove2",
                                         "&command.shortcut.key.altremove",
                                         "&command.shortcut.keycode.altremove",
                                         "&command.shortcut.modifiers.altremove",
                                         true);

    this.m_cmd_Remove.setCommandEnabledCallback(null,
                                                "library_cmd_remove2",
                                                plCmd_IsAnyTrackSelected);

    this.m_cmd_Remove.setCommandVisibleCallback(null,
                                                "library_cmd_remove2",
                                                plCmd_IsShortcutsInstantiator);

    // --------------------------------------------------------------------------
    // The EDIT button
    // --------------------------------------------------------------------------

    this.m_cmd_Edit = new PlaylistCommandsBuilder();

    // The first item, always created
    this.m_cmd_Edit.appendAction(null, 
                                 "library_cmd_edit",
                                 "&command.edit",
                                 "&command.tooltip.edit",
                                 plCmd_Edit_TriggerCallback);
    
    this.m_cmd_Edit.setCommandShortcut(null,
                                       "library_cmd_edit",
                                       "&command.shortcut.key.edit",
                                       "&command.shortcut.keycode.edit",
                                       "&command.shortcut.modifiers.edit",
                                       true);

    this.m_cmd_Edit.setCommandEnabledCallback(null,
                                              "library_cmd_edit",
                                              plCmd_IsAnyTrackSelected);
    
    // --------------------------------------------------------------------------
    // The DOWNLOAD button
    // --------------------------------------------------------------------------

    this.m_cmd_Download = new PlaylistCommandsBuilder();

    this.m_cmd_Download.appendAction(null, 
                                     "library_cmd_download",
                                     "&command.download",
                                     "&command.tooltip.download",
                                     plCmd_Download_TriggerCallback);
    
    this.m_cmd_Download.setCommandShortcut(null,
                                           "library_cmd_download",
                                           "&command.shortcut.key.download",
                                           "&command.shortcut.keycode.download",
                                           "&command.shortcut.modifiers.download",
                                           true);

    this.m_cmd_Download.setCommandEnabledCallback(null,
                                                  "library_cmd_download",
                                                  plCmd_IsAnyTrackSelected);

    // --------------------------------------------------------------------------
    // The ADDTOLIBRARY button
    // --------------------------------------------------------------------------

    this.m_cmd_AddToLibrary = new PlaylistCommandsBuilder();

    this.m_cmd_AddToLibrary.appendAction(null, 
                                         "library_cmd_addtolibrary",
                                         "&command.addtolibrary",
                                         "&command.tooltip.addtolibrary",
                                         plCmd_AddToLibrary_TriggerCallback);

    this.m_cmd_AddToLibrary.setCommandShortcut(null,
                                               "library_cmd_addtolibrary",
                                               "&command.shortcut.key.addtolibrary",
                                               "&command.shortcut.keycode.addtolibrary",
                                               "&command.shortcut.modifiers.addtolibrary",
                                               true);

    this.m_cmd_AddToLibrary.setCommandEnabledCallback(null,
                                                      "library_cmd_addtolibrary",
                                                      plCmd_CanAddToLibrary);

    // --------------------------------------------------------------------------
    // The COPY TRACK LOCATION button
    // --------------------------------------------------------------------------

    this.m_cmd_CopyTrackLocation = new PlaylistCommandsBuilder();

    this.m_cmd_CopyTrackLocation.appendAction(null, 
                                              "library_cmd_copylocation",
                                              "&command.copylocation",
                                              "&command.tooltip.copylocation",
                                              plCmd_CopyTrackLocation_TriggerCallback);

    this.m_cmd_CopyTrackLocation.setCommandShortcut(null,
                                                    "library_cmd_copylocation",
                                                    "&command.shortcut.key.copylocation",
                                                    "&command.shortcut.keycode.copylocation",
                                                    "&command.shortcut.modifiers.copylocation",
                                                    true);

    this.m_cmd_CopyTrackLocation.setCommandEnabledCallback(null,
                                                           "library_cmd_copylocation",
                                                           plCmd_IsAnyTrackSelected);

    // --------------------------------------------------------------------------
    // The SHOW DOWNLOAD PLAYLIST button
    // --------------------------------------------------------------------------

    this.m_cmd_ShowDownloadPlaylist = new PlaylistCommandsBuilder();

    this.m_cmd_ShowDownloadPlaylist.appendAction(null, 
                                                "library_cmd_showdlplaylist",
                                                "&command.showdlplaylist",
                                                "&command.tooltip.showdlplaylist",
                                                plCmd_ShowDownloadPlaylist_TriggerCallback);

    this.m_cmd_ShowDownloadPlaylist.setCommandShortcut(null,
                                                      "library_cmd_showdlplaylist",
                                                      "&command.shortcut.key.showdlplaylist",
                                                      "&command.shortcut.keycode.showdlplaylist",
                                                      "&command.shortcut.modifiers.showdlplaylist",
                                                      this.true);

    this.m_cmd_ShowDownloadPlaylist.setCommandVisibleCallback(null,
                                                              "library_cmd_showdlplaylist",
                                                              plCmd_ContextHasBrowser);

    // --------------------------------------------------------------------------
    // The PAUSE/RESUME DOWNLOAD button
    // --------------------------------------------------------------------------

    this.m_cmd_PauseResumeDownload = new PlaylistCommandsBuilder();

    this.m_cmd_PauseResumeDownload.appendAction(null, 
                                                "library_cmd_pause",
                                                "&command.pausedl",
                                                "&command.tooltip.pause",
                                                plCmd_PauseResumeDownload_TriggerCallback);

    this.m_cmd_PauseResumeDownload.setCommandShortcut(null,
                                                      "library_cmd_pause",
                                                      "&command.shortcut.key.pause",
                                                      "&command.shortcut.keycode.pause",
                                                      "&command.shortcut.modifiers.pause",
                                                      true);

    this.m_cmd_PauseResumeDownload.setCommandVisibleCallback(null,
                                                    "library_cmd_pause",
                                                    plCmd_IsDownloadingOrNotActive);

    this.m_cmd_PauseResumeDownload.setCommandEnabledCallback(null,
                                                    "library_cmd_pause",
                                                    plCmd_IsDownloadActive);

    this.m_cmd_PauseResumeDownload.appendAction(null, 
                                                "library_cmd_resume",
                                                "&command.resumedl",
                                                "&command.tooltip.resume",
                                                plCmd_PauseResumeDownload_TriggerCallback);

    this.m_cmd_PauseResumeDownload.setCommandShortcut(null,
                                                      "library_cmd_resume",
                                                      "&command.shortcut.key.resume",
                                                      "&command.shortcut.keycode.resume",
                                                      "&command.shortcut.modifiers.resume",
                                                      true);

    this.m_cmd_PauseResumeDownload.setCommandVisibleCallback(null,
                                                    "library_cmd_resume",
                                                    plCmd_IsDownloadPaused);

    // --------------------------------------------------------------------------
    // The CLEAN UP DOWNLOADS button
    // --------------------------------------------------------------------------

    this.m_cmd_CleanUpDownloads = new PlaylistCommandsBuilder();

    this.m_cmd_CleanUpDownloads.appendAction
                                      (null, 
                                       "library_cmd_cleanupdownloads",
                                       "&command.cleanupdownloads",
                                       "&command.tooltip.cleanupdownloads",
                                       plCmd_CleanUpDownloads_TriggerCallback);

    this.m_cmd_CleanUpDownloads.setCommandShortcut
                                (null,
                                 "library_cmd_cleanupdownloads",
                                 "&command.shortcut.key.cleanupdownloads",
                                 "&command.shortcut.keycode.cleanupdownloads",
                                 "&command.shortcut.modifiers.cleanupdownloads",
                                 true);

    this.m_cmd_CleanUpDownloads.setCommandEnabledCallback
                                              (null,
                                               "library_cmd_cleanupdownloads",
                                               plCmd_DownloadHasCompletedItems);

    // --------------------------------------------------------------------------
    // The SHOW WEB PLAYLIST button
    // --------------------------------------------------------------------------

    this.m_cmd_ShowWebPlaylist = new PlaylistCommandsBuilder();

    this.m_cmd_ShowWebPlaylist.appendAction(null, 
                                            "library_cmd_showwebplaylist",
                                            "&command.showwebplaylist",
                                            "&command.tooltip.showwebplaylist",
                                            plCmd_ShowWebPlaylist_TriggerCallback);

    this.m_cmd_ShowWebPlaylist.setCommandShortcut(null,
                                                  "library_cmd_showwebplaylist",
                                                  "&command.shortcut.key.showwebplaylist",
                                                  "&command.shortcut.keycode.showwebplaylist",
                                                  "&command.shortcut.modifiers.showwebplaylist",
                                                  true);

    this.m_cmd_ShowWebPlaylist.setCommandVisibleCallback(null,
                                                         "library_cmd_showwebplaylist",
                                                         plCmd_ContextHasBrowser);

    // --------------------------------------------------------------------------
    // The BURN TO CD button
    // --------------------------------------------------------------------------

    this.m_cmd_BurnToCD = new PlaylistCommandsBuilder();

    this.m_cmd_BurnToCD.appendAction(null, 
                                     "library_cmd_burntocd",
                                     "&command.burntocd",
                                     "&command.tooltip.burntocd",
                                     plCmd_BurnToCD_TriggerCallback);

    this.m_cmd_BurnToCD.setCommandShortcut(null,
                                           "library_cmd_burntocd",
                                           "&command.shortcut.key.burntocd",
                                           "&command.shortcut.keycode.burntocd",
                                           "&command.shortcut.modifiers.burntocd",
                                           true);

    //XXX burn to cd is not yet implemented
    this.m_cmd_BurnToCD.setCommandEnabledCallback(null,
                                                  "library_cmd_burntocd",
                                                  /*plCmd_IsAnyTrackSelected*/
                                                  plCmd_False);

    // --------------------------------------------------------------------------
    // The COPY TO DEVICE button
    // --------------------------------------------------------------------------

    this.m_cmd_CopyToDevice = new PlaylistCommandsBuilder();

    this.m_cmd_CopyToDevice.appendAction(null, 
                                         "library_cmd_device",
                                         "&command.device",
                                         "&command.tooltip.device",
                                         plCmd_CopyToDevice_TriggerCallback);

    this.m_cmd_CopyToDevice.setCommandShortcut(null,
                                               "library_cmd_device",
                                               "&command.shortcut.key.device",
                                               "&command.shortcut.keycode.device",
                                               "&command.shortcut.modifiers.device",
                                               true);

    //XXX copy to device is not yet implemented
    this.m_cmd_CopyToDevice.setCommandEnabledCallback(null,
                                                      "library_cmd_device",
                                                      /*plCmd_IsAnyTrackSelected*/
                                                      plCmd_False);

    // --------------------------------------------------------------------------
    // The Remove Playlist action
    // --------------------------------------------------------------------------

    this.m_cmd_list_Remove = new PlaylistCommandsBuilder();

    this.m_cmd_list_Remove.appendAction(null, 
                                       "playlist_cmd_remove",
                                       "&command.playlist.remove",
                                       "&command.tooltip.playlist.remove",
                                       plCmd_RemoveList_TriggerCallback);

    this.m_cmd_list_Remove.setCommandShortcut(null,
                                             "playlist_cmd_remove",
                                             "&command.playlist.shortcut.key.remove",
                                             "&command.playlist.shortcut.keycode.remove",
                                             "&command.playlist.shortcut.modifiers.remove",
                                             true);

    // --------------------------------------------------------------------------
    // The Rename Playlist action
    // --------------------------------------------------------------------------

    this.m_cmd_list_Rename = new PlaylistCommandsBuilder();

    this.m_cmd_list_Rename.appendAction(null, 
                                       "playlist_cmd_rename",
                                       "&command.playlist.rename",
                                       "&command.tooltip.playlist.rename",
                                       plCmd_RenameList_TriggerCallback);

    this.m_cmd_list_Rename.setCommandShortcut(null,
                                             "playlist_cmd_rename",
                                             "&command.playlist.shortcut.key.rename",
                                             "&command.playlist.shortcut.keycode.rename",
                                             "&command.playlist.shortcut.modifiers.rename",
                                             true);

    // --------------------------------------------------------------------------
    
    // Publish atomic commands
    
    this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_PLAY, this.m_cmd_Play);
    this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_REMOVE, this.m_cmd_Remove);
    this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_EDIT, this.m_cmd_Edit);
    this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_DOWNLOAD, this.m_cmd_Download);
    this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_ADDTOLIBRARY, this.m_cmd_AddToLibrary);
    this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_ADDTOPLAYLIST, SBPlaylistCommand_AddToPlaylist);
    this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_COPYTRACKLOCATION, this.m_cmd_CopyTrackLocation);
    this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_SHOWDOWNLOADPLAYLIST, this.m_cmd_ShowDownloadPlaylist);
    this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_SHOWWEBPLAYLIST, this.m_cmd_ShowWebPlaylist);
    this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_PAUSERESUMEDOWNLOAD, this.m_cmd_PauseResumeDownload);
    this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_CLEANUPDOWNLOADS, this.m_cmd_CleanUpDownloads);
    this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_BURNTOCD, this.m_cmd_BurnToCD);
    this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_COPYTODEVICE, this.m_cmd_CopyToDevice);
    
    this.m_mgr.publish(kPlaylistCommands.MEDIALIST_REMOVE, this.m_cmd_list_Remove);
    this.m_mgr.publish(kPlaylistCommands.MEDIALIST_RENAME, this.m_cmd_list_Rename);
    
    // --------------------------------------------------------------------------
    // Construct and publish the main library commands
    // --------------------------------------------------------------------------

    this.m_defaultCommands = new PlaylistCommandsBuilder();

    this.m_defaultCommands.appendPlaylistCommands(null, 
                                                  "library_cmdobj_play",
                                                  this.m_cmd_Play);
    this.m_defaultCommands.appendPlaylistCommands(null, 
                                                  "library_cmdobj_remove",
                                                  this.m_cmd_Remove);
    this.m_defaultCommands.appendPlaylistCommands(null, 
                                                  "library_cmdobj_edit",
                                                  this.m_cmd_Edit);
    this.m_defaultCommands.appendPlaylistCommands(null, 
                                                  "library_cmdobj_addtoplaylist",
                                                  SBPlaylistCommand_AddToPlaylist);
    this.m_defaultCommands.appendPlaylistCommands(null, 
                                                  "library_cmdobj_burntocd",
                                                  this.m_cmd_BurnToCD);
    this.m_defaultCommands.appendPlaylistCommands(null, 
                                                  "library_cmdobj_device",
                                                  this.m_cmd_CopyToDevice);

    this.m_defaultCommands.setVisibleCallback(plCmd_ShowDefaultInToolbarCheck);
    
    this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_DEFAULT, this.m_defaultCommands);

    // --------------------------------------------------------------------------
    // Construct and publish the web playlist commands
    // --------------------------------------------------------------------------

    this.m_webPlaylistCommands = new PlaylistCommandsBuilder();

    this.m_webPlaylistCommands.appendPlaylistCommands(null, 
                                                      "library_cmdobj_play",
                                                      this.m_cmd_Play);
    this.m_webPlaylistCommands.appendPlaylistCommands(null, 
                                                      "library_cmdobj_remove",
                                                      this.m_cmd_Remove);
    this.m_webPlaylistCommands.appendPlaylistCommands(null, 
                                                      "library_cmdobj_download",
                                                      this.m_cmd_Download);
    this.m_webPlaylistCommands.appendPlaylistCommands(null, 
                                                      "library_cmdobj_addtoplaylist",
                                                      SBPlaylistCommand_AddToPlaylist);
    this.m_webPlaylistCommands.appendPlaylistCommands(null, 
                                                      "library_cmdobj_addtolibrary",
                                                      this.m_cmd_AddToLibrary);
    this.m_webPlaylistCommands.appendPlaylistCommands(null, 
                                                      "library_cmdobj_copylocation",
                                                      this.m_cmd_CopyTrackLocation);
    this.m_webPlaylistCommands.appendSeparator(null, "separator");
    this.m_webPlaylistCommands.appendPlaylistCommands(null, 
                                                      "library_cmdobj_showdlplaylist",
                                                      this.m_cmd_ShowDownloadPlaylist);

    this.m_webPlaylistCommands.setVisibleCallback(plCmd_ShowDefaultInToolbarCheck);

    this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_WEBPLAYLIST, this.m_webPlaylistCommands);

    // Register these commands to the web playlist

    var webListGUID =
      prefs.getComplexValue("songbird.library.web",
                            Components.interfaces.nsISupportsString);

    this.m_mgr.registerPlaylistCommandsMediaItem(webListGUID, "", this.m_webPlaylistCommands);

    // --------------------------------------------------------------------------
    // Construct and publish the download playlist commands
    // --------------------------------------------------------------------------
    
    this.m_downloadCommands = new PlaylistCommandsBuilder();

    this.m_downloadCommands.appendPlaylistCommands(null, 
                                                   "library_cmdobj_play",
                                                   this.m_cmd_Play);
    this.m_downloadCommands.appendPlaylistCommands(null, 
                                                   "library_cmdobj_remove",
                                                   this.m_cmd_Remove);
    this.m_downloadCommands.appendPlaylistCommands(null, 
                                                   "library_cmdobj_pauseresumedownload",
                                                   this.m_cmd_PauseResumeDownload);

    this.m_downloadCommands.setVisibleCallback(plCmd_HideForToolbarCheck);

    this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_DOWNLOADPLAYLIST, this.m_downloadCommands);

    // Get the download device
    getDownloadDevice();

    // Register these commands to the download playlist

    var downloadListGUID =
      prefs.getComplexValue("songbird.library.download",
                            Components.interfaces.nsISupportsString);

    this.m_mgr.registerPlaylistCommandsMediaItem(downloadListGUID, "", this.m_downloadCommands);

    // --------------------------------------------------------------------------
    // Construct and publish the download toolbar commands
    // --------------------------------------------------------------------------
    
    this.m_downloadToolbarCommands = new PlaylistCommandsBuilder();

    this.m_downloadToolbarCommands.appendPlaylistCommands
                                            (null, 
                                             "library_cmdobj_cleanupdownloads",
                                             this.m_cmd_CleanUpDownloads);
    this.m_downloadToolbarCommands.appendPlaylistCommands
                                          (null, 
                                           "library_cmdobj_pauseresumedownload",
                                           this.m_cmd_PauseResumeDownload);

    this.m_downloadToolbarCommands.setVisibleCallback
                                                    (plCmd_ShowForToolbarCheck);
    this.m_downloadToolbarCommands.setInitCallback(plCmd_DownloadInit);
    this.m_downloadToolbarCommands.setShutdownCallback(plCmd_DownloadShutdown);

    this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_DOWNLOADTOOLBAR,
                       this.m_downloadToolbarCommands);

    // Register these commands to the download playlist

    this.m_mgr.registerPlaylistCommandsMediaItem
                                              (downloadListGUID,
                                               "",
                                               this.m_downloadToolbarCommands);

    // --------------------------------------------------------------------------
    // Construct and publish the service tree playlist commands
    // --------------------------------------------------------------------------

    this.m_serviceTreeDefaultCommands = new PlaylistCommandsBuilder();
  
    this.m_serviceTreeDefaultCommands.appendPlaylistCommands(null, 
                                            "servicetree_cmdobj_remove",
                                            this.m_cmd_list_Remove);
    this.m_serviceTreeDefaultCommands.appendPlaylistCommands(null, 
                                            "servicetree_cmdobj_rename",
                                            this.m_cmd_list_Rename);
    
    this.m_mgr.publish(kPlaylistCommands.MEDIALIST_DEFAULT, this.m_serviceTreeDefaultCommands);
  
    // Register these commands to simple and smart playlists

    this.m_mgr.registerPlaylistCommandsMediaList( "", "simple", this.m_serviceTreeDefaultCommands );
    this.m_mgr.registerPlaylistCommandsMediaList( "", "smart",  this.m_serviceTreeDefaultCommands );
  },

  // ==========================================================================
  // SHUTDOWN
  // ==========================================================================
  shutdownCommands: function() {
    var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                          .getService(Components.interfaces.nsIPrefBranch2);

    // Un-publish atomic commands

    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_PLAY, this.m_cmd_Play);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_REMOVE, this.m_cmd_Remove);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_EDIT, this.m_cmd_Edit);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_DOWNLOAD, this.m_cmd_Download);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_ADDTOLIBRARY, this.m_cmd_AddToLibrary);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_ADDTOPLAYLIST, SBPlaylistCommand_AddToPlaylist);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_COPYTRACKLOCATION, this.m_cmd_CopyTrackLocation);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_SHOWDOWNLOADPLAYLIST, this.m_cmd_ShowDownloadPlaylist);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_SHOWWEBPLAYLIST, this.m_cmd_ShowWebPlaylist);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_PAUSERESUMEDOWNLOAD, this.m_cmd_PauseResumeDownload);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_CLEANUPDOWNLOADS, this.m_cmd_CleanUpDownloads);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_BURNTOCD, this.m_cmd_BurnToCD);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_COPYTODEVICE, this.m_cmd_CopyToDevice);

    this.m_mgr.withdraw(kPlaylistCommands.MEDIALIST_REMOVE, this.m_cmd_list_Remove);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIALIST_RENAME, this.m_cmd_list_Rename);
                                     
    // Un-publish bundled commands                                 
    
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_DEFAULT, this.m_defaultCommands);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_WEBPLAYLIST, this.m_webPlaylistCommands);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_DOWNLOADPLAYLIST, this.m_downloadCommands);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_DOWNLOADTOOLBAR, this.m_downloadToolbarCommands);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIALIST_DEFAULT, this.m_serviceTreeDefaultCommands);

    // Un-register download playlist commands
    
    var downloadListGUID =
      prefs.getComplexValue("songbird.library.download",
                            Components.interfaces.nsISupportsString);

    this.m_mgr.unregisterPlaylistCommandsMediaItem(downloadListGUID, 
                                                   "",
                                                   this.m_downloadCommands);

    this.m_mgr.unregisterPlaylistCommandsMediaItem
                                              (downloadListGUID, 
                                               "",
                                               this.m_downloadToolbarCommands);
    
    g_downloadDevice = null;
    
    // Un-register web playlist commands
    
    var webListGUID =
      prefs.getComplexValue("songbird.library.web",
                            Components.interfaces.nsISupportsString);

    this.m_mgr.unregisterPlaylistCommandsMediaItem(webListGUID,
                                                   "",
                                                   this.m_webPlaylistCommands); 
    // Un-register servicetree commands 
    
    this.m_mgr.
      unregisterPlaylistCommandsMediaList("",
                                          "simple",
                                          this.m_serviceTreeDefaultCommands);
    this.m_mgr.
      unregisterPlaylistCommandsMediaList("",
                                          "smart",
                                          this.m_serviceTreeDefaultCommands);

    // Shutdown all command objects, this ensures that no external reference 
    // remains in their internal arrays
    this.m_cmd_Play.shutdown();
    this.m_cmd_Remove.shutdown();
    this.m_cmd_Edit.shutdown();
    this.m_cmd_Download.shutdown();
    this.m_cmd_AddToLibrary.shutdown();
    this.m_cmd_CopyTrackLocation.shutdown();
    this.m_cmd_ShowDownloadPlaylist.shutdown();
    this.m_cmd_ShowWebPlaylist.shutdown();
    this.m_cmd_PauseResumeDownload.shutdown();
    this.m_cmd_CleanUpDownloads.shutdown();
    this.m_cmd_BurnToCD.shutdown();
    this.m_cmd_CopyToDevice.shutdown();
    this.m_cmd_list_Remove.shutdown();
    this.m_cmd_list_Rename.shutdown();
    this.m_defaultCommands.shutdown();
    this.m_webPlaylistCommands.shutdown();
    this.m_downloadCommands.shutdown();
    this.m_downloadToolbarCommands.shutdown();
    this.m_serviceTreeDefaultCommands.shutdown();

    g_dataRemoteService = null;

    var obs = Components.classes["@mozilla.org/observer-service;1"]
                        .getService(Components.interfaces.nsIObserverService);

    obs.removeObserver(this, "quit-application");
  },
  
  // nsIObserver
  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "final-ui-startup":
        this.initCommands();
        break;
      case "quit-application":
        this.shutdownCommands();
        break;
    }
  },

  QueryInterface:
    XPCOMUtils.generateQI([Components.interfaces.nsIObserver])
};

function unwrap(obj) {
  if (obj && obj.wrappedJSObject)
    obj = obj.wrappedJSObject;
  return obj;
}

// --------------------------------------------------------------------------
// Called when the play action is triggered
function plCmd_Play_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  // if something is selected, trigger the play event on the playlist
  if (plCmd_IsAnyTrackSelected(aContext, aSubMenuId, aCommandId, aHost)) {
    // Repurpose the command to act as a doubleclick
    unwrap(aContext.playlist).sendPlayEvent();
  }
}

// Called when the edit action is triggered
function plCmd_Edit_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  if (plCmd_IsAnyTrackSelected(aContext, aSubMenuId, aCommandId, aHost)) {
    var playlist = unwrap(aContext.playlist);
    if ( plCmd_IsToolbarInstantiator(aContext, aSubMenuId, aCommandId, aHost) || 
        playlist.mediaListView.treeView.selectionCount > 1 ) {
      // Trigger the edit event on the playlist, this will spawn
      // the multiple tracks editor
      playlist.sendEditorEvent();
    } else {
      // Edit the context cell
      playlist.startCellEditing();
    }
  }
}

// Called when the remove action is triggered
function plCmd_Remove_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  // remove the selected tracks, if any
  if (plCmd_IsAnyTrackSelected(aContext, aSubMenuId, aCommandId, aHost)) {
    unwrap(aContext.playlist).removeSelectedTracks();
  }
}

// Called when the download action is triggered
function plCmd_Download_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  try
  {
    var playlist = unwrap(aContext.playlist);
    var window = unwrap(aContext.window);
    if(playlist.treeView.selectionCount) {
      onBrowserTransfer(new SelectionUnwrapper(
                       playlist.treeView.selectedMediaItems));
    }
    else {
      var allItems = {
        items: [],
        onEnumerationBegin: function(aMediaList) {
          return true;
        },
        onEnumeratedItem: function(aMediaList, aMediaItem) {
          this.items.push(aMediaItem);
          return true;
        },
        onEnumerationEnd: function(aMediaList, aResultCode) {
          return;
        }
      };

      playlist.mediaList.enumerateAllItems(allItems, 
        Ci.sbIMediaList.ENUMERATIONTYPE_SNAPSHOT);
      
      var itemEnum = ArrayConverter.enumerator(allItems.items);
      onBrowserTransfer(itemEnum);
    }
    
    // And show the download table in the chrome playlist if possible.
    var browser = window.gBrowser;
    if (browser) browser.mCurrentTab.switchToDownloadView();
  }
  catch( err )          
  {
    LOG( "plCmd_Download_TriggerCallback Error:" + err );
  }
}

// Called when the "add to library" action is triggered
function plCmd_AddToLibrary_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var libraryManager =
    Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
              .getService(Components.interfaces.sbILibraryManager);
  var mediaList = libraryManager.mainLibrary;
  
  var playlist = unwrap(aContext.playlist);
  var treeView = playlist.treeView;
  var selectionCount = treeView.selectionCount;
  
  var unwrapper = new SelectionUnwrapper(treeView.selectedMediaItems);
  
  var oldLength = mediaList.length;
  mediaList.addSome(unwrapper);

  var itemsAdded = mediaList.length - oldLength;
  playlist._reportAddedTracks(itemsAdded,
                              selectionCount - itemsAdded, 
                              mediaList.name); 
}

// Called when the "copy track location" action is triggered
function plCmd_CopyTrackLocation_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var clipboardtext = "";
  var urlCol = "url";
  var window = unwrap(aContext.window);
  var playlist = unwrap(aContext.playlist);
  var columnElem = window.
                     document.getAnonymousElementByAttribute(playlist,
                                                             "bind",
                                                             SBProperties.contentURL);
  var columnObj = playlist.tree.columns.getColumnFor(columnElem);
  var rangeCount = playlist.mediaListView.treeView.selection.getRangeCount();
  for (var i=0; i < rangeCount; i++) 
  {
    var start = {};
    var end = {};
    playlist.mediaListView.treeView.selection.getRangeAt( i, start, end );
    for( var c = start.value; c <= end.value; c++ )
    {
      if (c >= playlist.mediaListView.treeView.rowCount) 
      {
        continue; 
      }
      
      var val = playlist.mediaListView.treeView.getCellText(c, columnObj);
      if (clipboardtext != "") clipboardtext += "\n";
      clipboardtext += val;
    }
  }

  var clipboard = Components.classes["@mozilla.org/widget/clipboardhelper;1"]
                            .getService(Components.interfaces.nsIClipboardHelper);
  clipboard.copyString(clipboardtext);
}

// Called whne the "show download playlist" action is triggered
function plCmd_ShowDownloadPlaylist_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var window = unwrap(aContext.window);
  var browser = window.gBrowser;
  if (browser) {
    browser.loadMediaList(browser.downloadList);
  }
}

// Called whne the "show web playlist" action is triggered
function plCmd_ShowWebPlaylist_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var window = unwrap(aContext.window);
  var browser = window.gBrowser;
  if (browser) {
    if (window.location.pathname ==
        '/content/xul/sb-library-page.xul') {
      // we're in the download library / inner playlist view
      browser.loadMediaList(browser.webLibrary);
    } else {
      // we're in a web playlist / outer playlist view
      browser.mCurrentTab.switchToWebPlaylistView();
    }
  }
}

// Called when the pause/resume download action is triggered
function plCmd_PauseResumeDownload_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var device = getDownloadDevice();
  if (!device) return;
  var deviceState = device.getDeviceState('');
  if ( deviceState == Ci.sbIDeviceBase.STATE_DOWNLOADING )
  {
    device.suspendTransfer('');
  }
  else if ( deviceState == Ci.sbIDeviceBase.STATE_DOWNLOAD_PAUSED )
  {
    device.resumeTransfer('');
  }
  // Command buttons will update when device sends notification
}

// Called when the clean up downloads action is triggered
function plCmd_CleanUpDownloads_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var device = getDownloadDevice();
  if (!device) return;

  device.clearCompletedItems();

  // Command buttons will update when device sends notification
}

// Called when the "burn to cd" action is triggered
function plCmd_BurnToCD_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  // if something is selected, trigger the burn event on the playlist
  if (plCmd_IsAnyTrackSelected(aContext, aSubMenuId, aCommandId, aHost)) {
    unwrap(aContext.playlist).sendBurnToCDEvent();
  }
}

// Called when the "copy to device" action is triggered
function plCmd_CopyToDevice_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  // if something is selected, trigger the burn event on the playlist
  if (plCmd_IsAnyTrackSelected(aContext, aSubMenuId, aCommandId, aHost)) {
    //XXX not implemented
    //unwrap(aContext.playlist).sendCopyToDeviceEvent();
  }
}

// Called when the "remove playlist" action is triggered 
function plCmd_RemoveList_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  unwrap(aContext.medialist).library.remove(aContext.medialist);
}

// Called when the "rename playlist" action is triggered 
function plCmd_RenameList_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var window = unwrap(aContext.window);
  var medialist = unwrap(aContext.medialist);
  var servicePane = window.gServicePane;
  // If we have a servicetree, tell it to make the new playlist node editable
  if (servicePane) {
    // Ask the library service pane provider to suggest where
    // a new playlist should be created
    var librarySPS = Components.classes['@songbirdnest.com/servicepane/library;1']
                              .getService(Components.interfaces.sbILibraryServicePaneService);
    // Find the servicepane node for our new medialist
    var node = librarySPS.getNodeForLibraryResource(medialist);
    
    if (node) {
      // Ask the service pane to start editing our new node
      // so that the user can give it a name
      servicePane.startEditingNode(node);
    } else {
      throw("Error: Couldn't find a service pane node for the list we just created\n");
    }

  // Otherwise pop up a dialog and ask for playlist name
  } else {
    var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"  ]
                                  .getService(Components.interfaces.nsIPromptService);

    var input = {value: medialist.name};
    var title = SBString("renamePlaylist.title", "Rename Playlist");
    var prompt = SBString("renamePlaylist.prompt", "Enter the new name of the playlist.");

    if (promptService.prompt(window, title, prompt, input, null, {})) {
      medialist.name = input.value;
    }
  }
}

// Returns true when at least one track is selected in the playlist
function plCmd_IsAnyTrackSelected(aContext, aSubMenuId, aCommandId, aHost) {
  return ( unwrap(aContext.playlist).tree.currentIndex != -1 );
}

// Returns true if the host is the shortcuts instantiator
function plCmd_IsShortcutsInstantiator(aContext, aSubMenuId, aCommandId, aHost) {
  return (aHost == "shortcuts");
}

// Returns true if the host is the toolbar instantiator
function plCmd_IsToolbarInstantiator(aContext, aSubMenuId, aCommandId, aHost) {
  return (aHost == "toolbar");
}

// Returns true if the current playlist isn't the library
function plCmd_IsNotLibraryContext(aContext, aSubMenuId, aCommandId, aHost) {
  var medialist = unwrap(aContext.medialist);
  return (medialist.library != medialist);
}

// Returns true if the conditions are ok for adding tracks to the library
function plCmd_CanAddToLibrary(aContext, aSubMenuId, aCommandId, aHost) {
  return plCmd_IsAnyTrackSelected(aContext, aSubMenuId, aCommandId, aHost) &&
        plCmd_IsNotLibraryContext(aContext, aSubMenuId, aCommandId, aHost);
}

// Returns true if a download is currently in progress (not paused)
function plCmd_IsDownloading(aContext, aSubMenuId, aCommandId, aHost) {
  var device = getDownloadDevice();
  if (!device) return false;
  var deviceState = device.getDeviceState('');
  return (deviceState == Ci.sbIDeviceBase.STATE_DOWNLOADING);
}

// Returns true if a download is currently paused
function plCmd_IsDownloadPaused(aContext, aSubMenuId, aCommandId, aHost) {
  var device = getDownloadDevice();
  if (!device) return false;
  var deviceState = device.getDeviceState('');
  return (deviceState == Ci.sbIDeviceBase.STATE_DOWNLOAD_PAUSED);
}

// Returns true if a download has been started, regardless of wether it has
// been paused or not
function plCmd_IsDownloadActive(aContext, aSubMenuId, aCommandId, aHost) {
  return plCmd_IsDownloading(aContext, aSubMenuId, aCommandId, aHost) ||
        plCmd_IsDownloadPaused(aContext, aSubMenuId, aCommandId, aHost);
}

// Returns true if a download is in progress or if there is not download that
// has been started at all
function plCmd_IsDownloadingOrNotActive(aContext, aSubMenuId, aCommandId, aHost) {
  return plCmd_IsDownloading(aContext, aSubMenuId, aCommandId, aHost) ||
        !plCmd_IsDownloadActive(aContext, aSubMenuId, aCommandId, aHost);
}

// Returns true if any completed download items exist
function plCmd_DownloadHasCompletedItems(aContext, aSubMenuId, aCommandId, aHost) {
  var device = getDownloadDevice();
  if (!device) return false;
  return (device.completedItemCount > 0);
}

// Returns true if the supplied context contains a gBrowser object
function plCmd_ContextHasBrowser(aContext, aSubMenuId, aCommandId, aHost) {
  var window = unwrap(aContext.window);
  return (window.gBrowser);
}

// Always return false
function plCmd_False(aContext, aSubMenuId, aCommandId, aHost) {
  return false
}

function onBrowserTransfer(mediaItems, parentWindow)
{
  var ddh =
    Components.classes["@songbirdnest.com/Songbird/DownloadDeviceHelper;1"]
              .getService(Components.interfaces.sbIDownloadDeviceHelper);
  ddh.downloadSome(mediaItems);
}

var g_downloadDevice = null;
function getDownloadDevice() {
  try
  {
    if (!g_downloadDevice) {
      var deviceManager = Components.classes["@songbirdnest.com/Songbird/DeviceManager;1"].
                                      getService(Components.interfaces.sbIDeviceManager);

      if (deviceManager)
      {
        var downloadCategory = 'Songbird Download Device';
        if (deviceManager.hasDeviceForCategory(downloadCategory))
        {
          g_downloadDevice = deviceManager.getDeviceByCategory(downloadCategory);
          g_downloadDevice = g_downloadDevice.QueryInterface
                                      (Components.interfaces.sbIDownloadDevice);
        }
      }
    }
    return g_downloadDevice;
  } catch(e) {
  }
  return null;
}

function plCmd_ShowDefaultInToolbarCheck(aContext, aHost) {
  if (aHost == "toolbar") {
    if (dataRemote("commands.showdefaultintoolbar", null).boolValue) return true;
    return false;
  }
  return true;
}

function plCmd_HideForToolbarCheck(aContext, aHost) {
  return (aHost != "toolbar");
}

function plCmd_ShowForToolbarCheck(aContext, aHost) {
  return (aHost == "toolbar");
}

function plCmd_DownloadInit(aContext, aHost) {
  var implementorContext = {
    context: aContext,
    batchHelper: new BatchHelper(),
    needRefresh: false,

    // sbIDeviceBaseCallback
    onTransferComplete: function(aMediaItem, aStatus) {
      this.refreshCommands();
    },

    onStateChanged: function(aDeviceIdentifier, aState) {
      this.refreshCommands();
    },

    onDeviceConnect: function(aDeviceIdentifier) {},
    onDeviceDisconnect: function(aDeviceIdentifier) {},
    onTransferStart: function(aMediaItem) {},


    // sbIMediaListListener
    onAfterItemRemoved: function(aMediaList, aMediaItem) {
      return this.onMediaListChanged();
    },

    onListCleared: function(aMediaList) {
      return this.onMediaListChanged();
    },

    onBatchBegin: function(aMediaList) {
      this.batchHelper.begin();
    },

    onBatchEnd: function(aMediaList) {
      this.batchHelper.end();
      if (!this.batchHelper.isActive() && this.needRefresh) {
        this.refreshCommands();
        this.needRefresh = false;
      }
    },

    onItemAdded: function(aMediaList, aMediaItem) { return true; },
    onBeforeItemRemoved: function(aMediaList, aMediaItem) { return true; },
    onItemUpdated: function(aMediaList, aMediaItem) { return true; },


    onMediaListChanged: function() {
      if (this.batchHelper.isActive()) {
        this.needRefresh = true;
        return true;
      }
      this.refreshCommands();
      return false;
    },

    refreshCommands: function() {
      var playlist = unwrap(this.context.playlist);
      if (playlist)
        playlist.refreshCommands(); 
    }
  };

  var device = getDownloadDevice();
  if (!device) return false;
  device.addCallback(implementorContext);
  device.downloadMediaList.addListener
                              (implementorContext,
                               false,
                               Ci.sbIMediaList.LISTENER_FLAGS_AFTERITEMREMOVED |
                               Ci.sbIMediaList.LISTENER_FLAGS_LISTCLEARED |
                               Ci.sbIMediaList.LISTENER_FLAGS_BATCHBEGIN |
                               Ci.sbIMediaList.LISTENER_FLAGS_BATCHEND);
  aContext.implementorContext = implementorContext;
}

function plCmd_DownloadShutdown(aContext, aHost) {
  var device = getDownloadDevice();
  if (!device) return false;
  if (aContext.implementorContext) {
    device.removeCallback(aContext.implementorContext);
    device.downloadMediaList.removeListener(aContext.implementorContext);
  }
  aContext.implementorContext = null;
}

var g_dataRemoteService = null;
function dataRemote(aKey, aRoot) {
  if (!g_dataRemoteService) {
    g_dataRemoteService = new Components.
      Constructor( "@songbirdnest.com/Songbird/DataRemote;1", 
                  Ci.sbIDataRemote, 
                  "init");
  }
  return g_dataRemoteService(aKey, aRoot);
}

function LOG(str) {
  var consoleService = Components.classes['@mozilla.org/consoleservice;1']
                          .getService(Components.interfaces.nsIConsoleService);
  consoleService.logStringMessage(str);
  dump(str+"\n");
};

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([PublicPlaylistCommands],
  function(aCompMgr, aFileSpec, aLocation) {
    XPCOMUtils.categoryManager.addCategoryEntry(
      "app-startup",
      PublicPlaylistCommands.prototype.classDescription,
      "service," + PublicPlaylistCommands.prototype.contractID,
      true,
      true);
  });
}


