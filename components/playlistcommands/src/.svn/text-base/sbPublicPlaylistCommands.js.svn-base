// JScript source code
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2011 POTI, Inc.
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

var Cc = Components.classes;
var Ci = Components.interfaces;
var Cr = Components.results;
var Cu = Components.utils;

Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/DropHelper.jsm");
Cu.import("resource://app/jsmodules/kPlaylistCommands.jsm");
Cu.import("resource://app/jsmodules/sbAddToPlaylist.jsm");
Cu.import("resource://app/jsmodules/sbAddToDevice.jsm");
Cu.import("resource://app/jsmodules/sbAddToLibrary.jsm");
Cu.import("resource://app/jsmodules/sbCoverHelper.jsm");
Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://app/jsmodules/SBJobUtils.jsm");
Cu.import("resource://app/jsmodules/SBUtils.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

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
    var item = this._selection.getNext().mediaItem;
    item.setProperty(SBProperties.downloadStatusTarget,
                     item.library.guid + "," + item.guid);
    return item;
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
    .getService(Ci.sbIPlaylistCommandsManager);

  var obs = Cc["@mozilla.org/observer-service;1"]
              .getService(Ci.nsIObserverService);
  obs.addObserver(this, "final-ui-startup", false);
}

PublicPlaylistCommands.prototype.constructor = PublicPlaylistCommands;

PublicPlaylistCommands.prototype = {

  classDescription: "Songbird Public Playlist Commands",
  classID:          Components.ID("{1126ee77-2d85-4f79-a07a-b014da404e53}"),
  contractID:       "@songbirdnest.com/Songbird/PublicPlaylistCommands;1",

  m_mgr                           : null,

  m_defaultCommands               : null,
  m_webPlaylistCommands           : null,
  m_webMediaHistoryToolbarCommands: null,
  m_smartPlaylistsCommands        : null,
  m_downloadCommands              : null,
  m_downloadToolbarCommands       : null,
  m_downloadCommandsServicePane   : null,
  m_serviceTreeDefaultCommands    : null,
  m_deviceLibraryCommands         : null,
  m_cdDeviceLibraryCommands       : null,
  m_playQueueCommands             : null,
  m_playQueueLibraryCommands      : null,

  // Define various playlist commands, they will be exposed to the playlist commands
  // manager so that they can later be retrieved and concatenated into bigger
  // playlist commands objects.

  // Commands that act on playlist items
  m_cmd_Play                      : null, // play the selected track
  m_cmd_Pause                     : null, // pause the selected track
  m_cmd_Remove                    : null, // remove the selected track(s)
  m_cmd_Edit                      : null, // edit the selected track(s)
  m_cmd_Download                  : null, // download the selected track(s)
  m_cmd_Rescan                    : null, // rescan the selected track(s)
  m_cmd_Reveal                    : null, // reveal the selected track
  m_cmd_CopyTrackLocation         : null, // copy the select track(s) location(s)
  m_cmd_ShowDownloadPlaylist      : null, // switch the browser to show the download playlist
  m_cmd_PauseResumeDownload       : null, // auto-switching pause/resume track download
  m_cmd_CleanUpDownloads          : null, // clean up completed download items
  m_cmd_ClearHistory              : null, // clear the web media history
  m_cmd_UpdateSmartPlaylist       : null, // update the smart playlist
  m_cmd_EditSmartPlaylist         : null, // edit  the smart playlist properties
  m_cmd_SmartPlaylistSep          : null, // a separator (its own object for visible cb)
  m_cmd_LookupCDInfo              : null, // look up cd information
  m_cmd_CheckAll                  : null, // check all
  m_cmd_UncheckAll                : null, // uncheck all

  m_cmd_QueueNext                 : null, // add to next position in play queue
  m_cmd_QueueLast                 : null, // add to last position in play queue

  // Commands that act on playlist themselves
  m_cmd_list_Play                 : null, // play the selected playlist
  m_cmd_list_Remove               : null, // remove the selected playlist
  m_cmd_list_Rename               : null, // rename the selected playlist
  m_cmd_list_QueueNext            : null, // add all contents of the selected
                                          // playlist to next position in queue
  m_cmd_list_QueueLast            : null, // add all contents of the selected
                                          // playlist to last position in queue

  // Play queue library commands
  m_cmd_playqueue_SaveToPlaylist  : null,
  m_cmd_playqueue_ClearHistory    : null,
  m_cmd_playqueue_ClearAll        : null,

  m_downloadListGuid              : null,

  // ==========================================================================
  // INIT
  // ==========================================================================
  initCommands: function() {
    try {
      if (this.m_mgr.request(kPlaylistCommands.MEDIAITEM_DEFAULT)) {
        // already initialized ! bail out !
        return;
      }

      // Add an observer for the application shutdown event, so that we can
      // shutdown our commands

      var obs = Cc["@mozilla.org/observer-service;1"]
                  .getService(Ci.nsIObserverService);
      obs.removeObserver(this, "final-ui-startup");
      obs.addObserver(this, "quit-application", false);

      // --------------------------------------------------------------------------

      var prefs = Cc["@mozilla.org/preferences-service;1"]
                    .getService(Ci.nsIPrefBranch2);

      var ddh =
        Components.classes["@songbirdnest.com/Songbird/DownloadDeviceHelper;1"]
                  .getService(Components.interfaces.sbIDownloadDeviceHelper);
      var downloadMediaList = ddh.getDownloadMediaList();

      // We save this rather than getting it again because we need to remove
      // by the same id. If we re-fetch it, we might get a different GUID -
      // not in the actual app, because the download media list doesn't just
      // go away, but in unit tests that clear the library.
      if (downloadMediaList)
        this.m_downloadListGuid = downloadMediaList.guid;
      else
        this.m_downloadListGuid = "";


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
                    "sbIPlaylistCommandsBuilder", "init");

      this.m_cmd_Play = new PlaylistCommandsBuilder("play-default_download_webplaylist_playqueue-cmd");

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

      this.m_cmd_Play.setCommandVisibleCallback(null,
                                                "library_cmd_play",
                                                plCmd_Play_VisibleCallback);

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

      this.m_cmd_Pause = new PlaylistCommandsBuilder("pause-default_download_webplaylist_playqueue-cmd");

      this.m_cmd_Pause.appendAction(null,
                                    "library_cmd_pause",
                                    "&command.pause",
                                    "&command.tooltip.pause",
                                    plCmd_Pause_TriggerCallback);

      this.m_cmd_Pause.setCommandVisibleCallback(null,
                                                 "library_cmd_pause",
                                                 plCmd_Pause_VisibleCallback);

      // --------------------------------------------------------------------------
      // The REMOVE button, like the play button, is made of two commands, one that
      // is always there for all instantiators, the other that is only created by
      // the shortcuts instantiator. This makes one toolbar button and menu item,
      // and two shortcut keys.
      // --------------------------------------------------------------------------

      this.m_cmd_Remove = new PlaylistCommandsBuilder("remove-default_download_device_webplaylist_playqueue-cmd");

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
                                                  plCmd_AND(
                                                    plCmd_IsAnyTrackSelected,
                                                    plCmd_CanModifyPlaylistContent));

      // The second item, only created by the keyboard shortcuts instantiator,
      // and only for the mac.
      var sysInfo = Components.classes["@mozilla.org/system-info;1"]
                    .getService(Components.interfaces.nsIPropertyBag2);
      if ( sysInfo.getProperty("name") == "Darwin" ) {
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
                                                    plCmd_AND(
                                                      plCmd_IsAnyTrackSelected,
                                                      plCmd_CanModifyPlaylistContent));

        this.m_cmd_Remove.setCommandVisibleCallback(null,
                                                    "library_cmd_remove2",
                                                    plCmd_IsShortcutsInstantiator);
      }

      // --------------------------------------------------------------------------
      // The EDIT button
      // --------------------------------------------------------------------------

      this.m_cmd_Edit = new PlaylistCommandsBuilder("edit-default_device_cddevice-cmd");

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

      this.m_cmd_Download = new PlaylistCommandsBuilder("download-webplaylist-cmd");

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
                                                    plCmd_Download_EnabledCallback);

      // --------------------------------------------------------------------------
      // The RESCAN button
      // --------------------------------------------------------------------------


      this.m_cmd_Rescan = new PlaylistCommandsBuilder("rescan-default_playqueue-cmd");

      this.m_cmd_Rescan.appendAction(null,
                                     "library_cmd_rescan",
                                     "&command.rescan",
                                     "&command.tooltip.rescan",
                                     plCmd_Rescan_TriggerCallback);

      this.m_cmd_Rescan.setCommandShortcut(null,
                                           "library_cmd_rescan",
                                           "&command.shortcut.key.rescan",
                                           "&command.shortcut.keycode.rescan",
                                           "&command.shortcut.modifiers.rescan",
                                           true);

      this.m_cmd_Rescan.setCommandEnabledCallback(null,
                                                  "library_cmd_rescan",
                                                  plCmd_IsAnyTrackSelected);

      this.m_cmd_Rescan.setCommandVisibleCallback(null,
                                                  "library_cmd_rescan",
                                                  plCmd_IsRescanItemEnabled);

      // --------------------------------------------------------------------------
      // The REVEAL button
      // --------------------------------------------------------------------------


      this.m_cmd_Reveal = new PlaylistCommandsBuilder("reveal-default_device_playqueue-cmd");

      this.m_cmd_Reveal.appendAction(null,
                                     "library_cmd_reveal",
                                     "&command.reveal",
                                     "&command.tooltip.reveal",
                                     plCmd_Reveal_TriggerCallback);

      this.m_cmd_Reveal.setCommandShortcut(null,
                                           "library_cmd_reveal",
                                           "&command.shortcut.key.reveal",
                                           "&command.shortcut.keycode.reveal",
                                           "&command.shortcut.modifiers.reveal",
                                           true);

      this.m_cmd_Reveal.setCommandEnabledCallback(null,
                                                  "library_cmd_reveal",
                                                  plCmd_isSelectionRevealable);

      // --------------------------------------------------------------------------
      // The COPY TRACK LOCATION button
      // --------------------------------------------------------------------------

      this.m_cmd_CopyTrackLocation = new PlaylistCommandsBuilder
                                         ("copylocation-webplaylist-cmd");

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
      // The CLEAR HISTORY button
      // --------------------------------------------------------------------------

      this.m_cmd_ClearHistory = new PlaylistCommandsBuilder
                                    ("clearhistory-webhistory-cmd");

      this.m_cmd_ClearHistory.appendAction
                                        (null,
                                         "library_cmd_clearhistory",
                                         "&command.clearhistory",
                                         "&command.tooltip.clearhistory",
                                         plCmd_ClearHistory_TriggerCallback);

      this.m_cmd_ClearHistory.setCommandShortcut
                                  (null,
                                   "library_cmd_clearhistory",
                                   "&command.shortcut.key.clearhistory",
                                   "&command.shortcut.keycode.clearhistory",
                                   "&command.shortcut.modifiers.clearhistory",
                                   true);

      this.m_cmd_ClearHistory.setCommandEnabledCallback
                                                (null,
                                                 "library_cmd_clearhistory",
                                                 plCmd_WebMediaHistoryHasItems);

      // --------------------------------------------------------------------------
      // The SHOW DOWNLOAD PLAYLIST button
      // --------------------------------------------------------------------------

      this.m_cmd_ShowDownloadPlaylist = new PlaylistCommandsBuilder
                                            ("showdl-webplaylist-cmd");

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
                                                        true);

      this.m_cmd_ShowDownloadPlaylist.setCommandVisibleCallback(null,
                                                                "library_cmd_showdlplaylist",
                                                                plCmd_ContextHasBrowser);

      // --------------------------------------------------------------------------
      // The PAUSE/RESUME DOWNLOAD button
      // --------------------------------------------------------------------------

      this.m_cmd_PauseResumeDownload = new PlaylistCommandsBuilder
                                           ("pauseresume-download-cmd");

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

      this.m_cmd_CleanUpDownloads = new PlaylistCommandsBuilder
                                        ("cleanup-download-cmd");

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

      // -----------------------------------------------------------------------
      // The Play Playlist action
      // -----------------------------------------------------------------------

      this.m_cmd_list_Play = new PlaylistCommandsBuilder("play-servicetree-cmd");

      this.m_cmd_list_Play.appendAction(null,
                                        "playlist_cmd_play",
                                        "&command.playlist.play",
                                        "&command.tooltip.playlist.play",
                                        plCmd_PlayPlaylist_TriggerCallback);

      this.m_cmd_list_Play.setCommandShortcut(null,
                                    "playlist_cmd_play",
                                    "&command.playlist.shortcut.key.play",
                                    "&command.playlist.shortcut.keycode.play",
                                    "&command.playlist.shortcut.modifiers.play",
                                    true);

      // disable the command for empty playlists.
      this.m_cmd_list_Play.setCommandEnabledCallback(null,
                                                     "playlist_cmd_play",
                                                     plCmd_IsNotEmptyPlaylist);

      // --------------------------------------------------------------------------
      // The Remove Playlist action
      // --------------------------------------------------------------------------

      this.m_cmd_list_Remove = new PlaylistCommandsBuilder("remove-servicetree-cmd");

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

      // disable the command for readonly playlists.
      this.m_cmd_list_Remove.setCommandEnabledCallback
                                                (null,
                                                 "playlist_cmd_remove",
                                                 plCmd_CanModifyPlaylist);

      // --------------------------------------------------------------------------
      // The Rename Playlist action
      // --------------------------------------------------------------------------

      this.m_cmd_list_Rename = new PlaylistCommandsBuilder("rename-servicetree-cmd");

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

      // disable the command for readonly playlists.
      this.m_cmd_list_Rename.setCommandEnabledCallback(null,
                                                       "playlist_cmd_rename",
                                                       plCmd_CanModifyPlaylist);

      // --------------------------------------------------------------------------
      // The QueueNext Playlist action
      // --------------------------------------------------------------------------

      this.m_cmd_list_QueueNext = new PlaylistCommandsBuilder("queuenext-servicetree-cmd");

      this.m_cmd_list_QueueNext.appendAction(null,
                                             "playlist_cmd_queuenext",
                                             "&command.playlist.queuenext",
                                             "&command.tooltip.playlist.queuenext",
                                             plCmd_QueueListNext_TriggerCallback);

      this.m_cmd_list_QueueNext.setCommandShortcut(null,
                                                   "playlist_cmd_queuenext",
                                                   "&command.playlist.shortcut.key.queuenext",
                                                   "&command.playlist.shortcut.keycode.queuenext",
                                                   "&command.playlist.shortcut.modifiers.queuenext",
                                                   true);

      this.m_cmd_list_QueueNext.setCommandVisibleCallback(null,
                                                          "playlist_cmd_queuenext",
                                                          plCmd_CanQueuePlaylist);

      // disable the command for empty playlists.
      this.m_cmd_list_QueueNext.setCommandEnabledCallback(null,
                                                          "playlist_cmd_queuenext",
                                                          plCmd_ListContextMenuQueueEnabled);

      // --------------------------------------------------------------------------
      // The QueueLast Playlist action
      // --------------------------------------------------------------------------

      this.m_cmd_list_QueueLast = new PlaylistCommandsBuilder("queuelast-servicetree-cmd");

      this.m_cmd_list_QueueLast.appendAction(null,
                                             "playlist_cmd_queuelast",
                                             "&command.playlist.queuelast",
                                             "&command.tooltip.playlist.queuelast",
                                             plCmd_QueueListLast_TriggerCallback);

      this.m_cmd_list_QueueLast.setCommandShortcut(null,
                                                   "playlist_cmd_queuelast",
                                                   "&command.playlist.shortcut.key.queuelast",
                                                   "&command.playlist.shortcut.keycode.queuelast",
                                                   "&command.playlist.shortcut.modifiers.queuelast",
                                                   true);

      this.m_cmd_list_QueueLast.setCommandVisibleCallback(null,
                                                          "playlist_cmd_queuelast",
                                                          plCmd_CanQueuePlaylist);

      // disable the command for empty playlists.
      this.m_cmd_list_QueueLast.setCommandEnabledCallback(null,
                                                          "playlist_cmd_queuelast",
                                                          plCmd_ListContextMenuQueueEnabled);

      // --------------------------------------------------------------------------
      // The Get Artwork action
      // --------------------------------------------------------------------------

      this.m_cmd_GetArtwork = new PlaylistCommandsBuilder("getartwork-default-cmd");

      this.m_cmd_GetArtwork.appendAction(null,
                                         "library_cmd_getartwork",
                                         "&command.getartwork",
                                         "&command.tooltip.getartwork",
                                         plCmd_GetArtwork_TriggerCallback);

      this.m_cmd_GetArtwork.setCommandShortcut(null,
                                               "library_cmd_getartwork",
                                               "&command.shortcut.key.getartwork",
                                               "&command.shortcut.keycode.getartwork",
                                               "&command.shortcut.modifiers.getartwork",
                                               true);

      this.m_cmd_GetArtwork.setCommandVisibleCallback(null,
                                                      "library_cmd_getartwork",
                                                      plCmd_IsAnyAudioSelected);

      this.m_cmd_GetArtwork.setCommandEnabledCallback(null,
                                                      "library_cmd_getartwork",
                                                      plCmd_IsAnyTrackSelected);

      // --------------------------------------------------------------------------
      // The 'Show Device Only' and 'Show All On Device' actions for MSC & MTP
      // --------------------------------------------------------------------------
      // A command object for devices to hold commands that show in the toolbar
      this.m_cmd_DeviceToolbarCmds =
        new PlaylistCommandsBuilder("device_cmds_toolbar");

      // A command to show items only on the device
      this.m_cmd_ShowOnlyOnDevice =
        new PlaylistCommandsBuilder("device_cmd_showdeviceonly");

      this.m_cmd_ShowOnlyOnDevice.appendAction
                               (null,
                                "device_cmd_showdeviceonly",
                                "&device.command.show_only_on_device",
                                "",
                                plCmd_ShowOnlyOnDevice_TriggerCallback);

      // Show the 'device only' command only when we aren't already filtering
      this.m_cmd_ShowOnlyOnDevice
          .setVisibleCallback(plCmd_ShowOnlyOnDevice_VisibleCallback);

      this.m_cmd_DeviceToolbarCmds.appendPlaylistCommands
                                  (null,
                                   "device_cmdobj_showdeviceonly",
                                   this.m_cmd_ShowOnlyOnDevice);

      // A command to shall all items on the device
      this.m_cmd_ShowAllOnDevice =
        new PlaylistCommandsBuilder("device_cmd_showallondevice");

      this.m_cmd_ShowAllOnDevice.appendAction
                                (null,
                                 "device_cmd_showallondevice",
                                 "&device.command.show_all_on_device",
                                 "",
                                 plCmd_ShowAllOnDevice_TriggerCallback);


      this.m_cmd_DeviceToolbarCmds.appendPlaylistCommands
                                  (null,
                                   "device_cmdobj_showallondevice",
                                   this.m_cmd_ShowAllOnDevice);

      // Show the 'show all' command when we are filtering for device only
      this.m_cmd_ShowAllOnDevice
          .setVisibleCallback(plCmd_ShowAllOnDevice_VisibleCallback);

      // Publish this command so that it can be registered to the device
      // library when a device is mounted.
      this.m_mgr.publish(kPlaylistCommands.MEDIALIST_DEVICE_LIBRARY_TOOLBAR,
                         this.m_cmd_DeviceToolbarCmds);

      // --------------------------------------------------------------------------
      // The Lookup CD Info action
      // --------------------------------------------------------------------------

      this.m_cmd_LookupCDInfo = new PlaylistCommandsBuilder("lookupcd-cddevice-cmd");

      this.m_cmd_LookupCDInfo.appendAction(null,
                                           "library_cmd_lookupcdinfo",
                                           "&command.lookupcdinfo",
                                           "&command.tooltip.lookupcdinfo",
                                           plCmd_LookupCDInfo_TriggerCallback);

      this.m_cmd_LookupCDInfo.setCommandShortcut(null,
                                                 "library_cmd_lookupcdinfo",
                                                 "&command.shortcut.key.lookupcdinfo",
                                                 "&command.shortcut.keycode.lookupcdinfo",
                                                 "&command.shortcut.modifiers.lookupcdinfo",
                                                 true);
      this.m_cmd_LookupCDInfo.setCommandEnabledCallback(null,
                                                        "library_cmd_lookupcdinfo",
                                                        plCmd_NOT(plCmd_IsMediaListReadOnly));

      // --------------------------------------------------------------------------
      // Play Queue bundled commands
      // --------------------------------------------------------------------------

      this.m_cmd_playqueue_SaveToPlaylist = new PlaylistCommandsBuilder
                                                ("saveto-playqueue-cmd");

      this.m_cmd_playqueue_SaveToPlaylist.appendAction(null,
                                                       "playqueue_cmd_savetoplaylist",
                                                       "&command.queuesavetoplaylist",
                                                       "&command.tooltip.queuesavetoplaylist",
                                                       plCmd_QueueSaveToPlaylist_TriggerCallback);

      this.m_cmd_playqueue_SaveToPlaylist.setCommandShortcut(null,
                                                             "playqueue_cmd_savetoplaylist",
                                                             "&command.shortcut.key.queuesavetoplaylist",
                                                             "&command.shortcut.keycode.queuesavetoplaylist",
                                                             "&command.shortcut.modifiers.queuesavetoplaylist",
                                                             true);

      this.m_cmd_playqueue_ClearAll = new PlaylistCommandsBuilder
                                          ("clearall-playqueue-cmd");

      this.m_cmd_playqueue_ClearAll.appendAction(
                             null,
                             "playqueue_cmd_clearall",
                             "&command.queueclearall",
                             "&command.tooltip.queueclearall",
                             plCmd_QueueClearAll_TriggerCallback);

      this.m_cmd_playqueue_ClearAll.setCommandShortcut(
                             null,
                             "playqueue_cmd_clearall",
                             "&command.shortcut.key.queueclearall",
                             "&command.shortcut.keycode.queueclearall",
                             "&command.shortcut.modifiers.queueclearall",
                             true);

      this.m_cmd_playqueue_ClearHistory = new PlaylistCommandsBuilder
                                              ("clearhistory-playqueue-cmd");

      this.m_cmd_playqueue_ClearHistory.appendSubmenu(
                             null,
                             "playqueue_cmd_clearhistory",
                             null,
                             "&command.tooltip.queueclearhistory",
                             true);

      this.m_cmd_playqueue_ClearHistory.appendAction(
                             "playqueue_cmd_clearhistory",
                             "playqueue_cmd_clearhistorymenuitem",
                             "&command.queueclearhistory",
                             "&command.tooltip.queueclearhistory",
                             plCmd_QueueClearHistory_TriggerCallback);

      this.m_cmd_playqueue_ClearHistory.setCommandShortcut(
                             "playqueue_cmd_clearhistory",
                             "playqueue_cmd_clearhistorymenuitem",
                             "&command.shortcut.key.queueclearhistory",
                             "&command.shortcut.keycode.queueclearhistory",
                             "&command.shortcut.modifiers.queueclearhistory",
                             true);

      // ----------------------------------------------------------------------
      // The Check/Uncheck All actions
      // ----------------------------------------------------------------------
      this.m_cmd_CheckAll = new PlaylistCommandsBuilder("checkall-cddevice-cmd");
      this.m_cmd_CheckAll.appendAction(null,
                                       "library_cmd_checkall",
                                       "&command.checkall",
                                       "&command.tooltip.checkall",
                                       plCmd_CheckAll_TriggerCallback);
      this.m_cmd_CheckAll.setCommandVisibleCallback(
                                       null,
                                       "library_cmd_checkall",
                                       plCmd_CheckAllVisibleCallback);
      this.m_cmd_CheckAll.setCommandEnabledCallback(null,
                                                    "library_cmd_checkall",
                                                    plCmd_NOT(plCmd_IsMediaListReadOnly));

      this.m_cmd_UncheckAll = new PlaylistCommandsBuilder("uncheckall-cddevice-cmd");
      this.m_cmd_UncheckAll.appendAction(
                                    null,
                                    "library_cmd_uncheckall",
                                    "&command.uncheckall",
                                    "&command.tooltip.uncheckall",
                                    plCmd_UncheckAll_TriggerCallback);
      this.m_cmd_UncheckAll.setCommandVisibleCallback(null,
                                    "library_cmd_uncheckall",
                                    plCmd_NOT(plCmd_CheckAllVisibleCallback));
      this.m_cmd_UncheckAll.setCommandEnabledCallback(null,
                                                      "library_cmd_uncheckall",
                                                      plCmd_NOT(plCmd_IsMediaListReadOnly));

      // -----------------------------------------------------------------------
      // The Queue Next action
      // -----------------------------------------------------------------------
      this.m_cmd_QueueNext = new PlaylistCommandsBuilder("queuenext-default_webplaylist-cmd");
      this.m_cmd_QueueNext.appendAction(null,
                                        "library_cmd_queuenext",
                                        "&command.queuenext",
                                        "&command.tooltip.queuenext",
                                        plCmd_QueueNext_TriggerCallback);

      this.m_cmd_QueueNext.setCommandShortcut(null,
                                              "library_cmd_queuenext",
                                              "&command.shortcut.key.queuenext",
                                              "&command.shortcut.keycode.queuenext",
                                              "&command.shortcut.modifiers.queuenext",
                                              true);

      this.m_cmd_QueueNext.setCommandEnabledCallback(null,
                                                     "library_cmd_queuenext",
                                                     plCmd_ContextMenuQueueEnabled);

      // -----------------------------------------------------------------------
      // The Queue Last action
      // -----------------------------------------------------------------------
      this.m_cmd_QueueLast = new PlaylistCommandsBuilder("queuelast-default_webplaylist-cmd");
      this.m_cmd_QueueLast.appendAction(null,
                                        "library_cmd_queuelast",
                                        "&command.queuelast",
                                        "&command.tooltip.queuelast",
                                        plCmd_QueueLast_TriggerCallback);

      this.m_cmd_QueueLast.setCommandShortcut(null,
                                              "library_cmd_queuelast",
                                              "&command.shortcut.key.queuelast",
                                              "&command.shortcut.keycode.queuelast",
                                              "&command.shortcut.modifiers.queuelast",
                                              true);
      this.m_cmd_QueueLast.setCommandEnabledCallback(null,
                                                     "library_cmd_queuelast",
                                                     plCmd_ContextMenuQueueEnabled);

      // --------------------------------------------------------------------------

      // Publish atomic commands

      this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_PLAY, this.m_cmd_Play);
      this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_PAUSE, this.m_cmd_Pause);
      this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_REMOVE, this.m_cmd_Remove);
      this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_EDIT, this.m_cmd_Edit);
      this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_DOWNLOAD, this.m_cmd_Download);
      this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_RESCAN, this.m_cmd_Rescan);
      this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_REVEAL, this.m_cmd_Reveal);
      this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_ADDTOPLAYLIST, SBPlaylistCommand_AddToPlaylist);
      this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_ADDTODEVICE, SBPlaylistCommand_AddToDevice);
      this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_ADDTOLIBRARY, SBPlaylistCommand_AddToLibrary);
      this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_COPYTRACKLOCATION, this.m_cmd_CopyTrackLocation);
      this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_SHOWDOWNLOADPLAYLIST, this.m_cmd_ShowDownloadPlaylist);
      this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_PAUSERESUMEDOWNLOAD, this.m_cmd_PauseResumeDownload);
      this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_CLEANUPDOWNLOADS, this.m_cmd_CleanUpDownloads);
      this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_CLEARHISTORY, this.m_cmd_ClearHistory);
      this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_GETARTWORK, this.m_cmd_GetArtwork);
      this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_QUEUENEXT, this.m_cmd_QueueNext);
      this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_QUEUELAST, this.m_cmd_QueueLast);

      this.m_mgr.publish(kPlaylistCommands.MEDIALIST_PLAY, this.m_cmd_list_Play);
      this.m_mgr.publish(kPlaylistCommands.MEDIALIST_REMOVE, this.m_cmd_list_Remove);
      this.m_mgr.publish(kPlaylistCommands.MEDIALIST_RENAME, this.m_cmd_list_Rename);
      this.m_mgr.publish(kPlaylistCommands.MEDIALIST_QUEUENEXT, this.m_cmd_list_QueueNext);
      this.m_mgr.publish(kPlaylistCommands.MEDIALIST_QUEUELAST, this.m_cmd_list_QueueLast);

      // --------------------------------------------------------------------------
      // Construct and publish the main library commands
      // --------------------------------------------------------------------------

      this.m_defaultCommands = new PlaylistCommandsBuilder("default_cmds");

      this.m_defaultCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_play",
                                                    this.m_cmd_Play);

      this.m_defaultCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_pause",
                                                    this.m_cmd_Pause);

      this.m_defaultCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_edit",
                                                    this.m_cmd_Edit);
      this.m_defaultCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_reveal",
                                                    this.m_cmd_Reveal);
      this.m_defaultCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_getartwork",
                                                    this.m_cmd_GetArtwork);

      this.m_defaultCommands.appendSeparator(null, "default_commands_separator_1");

      this.m_defaultCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_addtoplaylist",
                                                    SBPlaylistCommand_AddToPlaylist);
      this.m_defaultCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_addtodevice",
                                                    SBPlaylistCommand_AddToDevice);

      this.m_defaultCommands.appendSeparator(null, "default_commands_separator_2");

      this.m_defaultCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_rescan",
                                                    this.m_cmd_Rescan);

      this.m_defaultCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_remove",
                                                    this.m_cmd_Remove);

      this.m_defaultCommands.appendSeparator(null, "default_commands_separator_3");

      this.m_defaultCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_queuenext",
                                                    this.m_cmd_QueueNext);

      this.m_defaultCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_queuenast",
                                                    this.m_cmd_QueueLast);

      this.m_defaultCommands.setVisibleCallback(plCmd_ShowDefaultInToolbarCheck);

      this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_DEFAULT, this.m_defaultCommands);


      // --------------------------------------------------------------------------
      // Construct and publish the smart playlists commands
      // --------------------------------------------------------------------------

      this.m_cmd_UpdateSmartPlaylist = new PlaylistCommandsBuilder
                                           ("update-smartplaylist-cmd");

      this.m_cmd_UpdateSmartPlaylist.appendAction
                                        (null,
                                         "smartpl_cmd_update",
                                         "&command.smartpl.update",
                                         "&command.tooltip.smartpl.update",
                                         plCmd_UpdateSmartPlaylist_TriggerCallback);

      this.m_cmd_UpdateSmartPlaylist.setCommandShortcut
                                  (null,
                                   "smartpl_cmd_update",
                                   "&command.shortcut.key.smartpl.update",
                                   "&command.shortcut.keycode.smartpl.update",
                                   "&command.shortcut.modifiers.smartpl.update",
                                   true);

     this.m_cmd_UpdateSmartPlaylist.setCommandVisibleCallback(null,
                                                              "smartpl_cmd_update",
                                                              plCmd_NOT(plCmd_isLiveUpdateSmartPlaylist));

      this.m_cmd_EditSmartPlaylist = new PlaylistCommandsBuilder
                                         ("properties-smartplaylist-cmd");

      this.m_cmd_EditSmartPlaylist.appendAction
                                        (null,
                                         "smartpl_cmd_properties",
                                         "&command.smartpl.properties",
                                         "&command.tooltip.smartpl.properties",
                                         plCmd_EditSmartPlaylist_TriggerCallback);

      this.m_cmd_EditSmartPlaylist.setCommandShortcut
                                  (null,
                                   "smartpl_cmd_properties",
                                   "&command.shortcut.key.smartpl.properties",
                                   "&command.shortcut.keycode.smartpl.properties",
                                   "&command.shortcut.modifiers.smartpl.properties",
                                   true);

      this.m_cmd_EditSmartPlaylist.setCommandVisibleCallback(null,
                                                             "smartpl_cmd_properties",
                                                             plCmd_CanModifyPlaylist);

      this.m_smartPlaylistsCommands = new PlaylistCommandsBuilder
                                          ("smartplaylist_cmds");

      this.m_smartPlaylistsCommands.appendPlaylistCommands(null,
                                                           "library_cmdobj_defaults",
                                                           this.m_defaultCommands);

      this.m_cmd_SmartPlaylistSep = new PlaylistCommandsBuilder
                                        ("separator-smartplaylist-cmd");

      this.m_cmd_SmartPlaylistSep.appendSeparator(null, "smartpl_separator");

      this.m_cmd_SmartPlaylistSep.setVisibleCallback(plCmd_NOT(plCmd_ShowForToolbarCheck));

      this.m_smartPlaylistsCommands.appendPlaylistCommands(null,
                                                           "smartpl_cmdobj_sep",
                                                           this.m_cmd_SmartPlaylistSep);

      this.m_smartPlaylistsCommands.appendPlaylistCommands(null,
                                                           "smartpl_cmdobj_update",
                                                           this.m_cmd_UpdateSmartPlaylist);

      this.m_smartPlaylistsCommands.appendPlaylistCommands(null,
                                                           "smartpl_cmdobj_properties",
                                                           this.m_cmd_EditSmartPlaylist);

      this.m_mgr.publish(kPlaylistCommands.MEDIALIST_UPDATESMARTMEDIALIST, this.m_cmd_UpdateSmartPlaylist);
      this.m_mgr.publish(kPlaylistCommands.MEDIALIST_EDITSMARTMEDIALIST, this.m_cmd_EditSmartPlaylist);

      this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_SMARTPLAYLIST, this.m_smartPlaylistsCommands);
      this.m_mgr.registerPlaylistCommandsMediaItem("", "smart", this.m_smartPlaylistsCommands);

      // --------------------------------------------------------------------------
      // Construct and publish the web playlist commands
      // --------------------------------------------------------------------------

      this.m_webPlaylistCommands = new PlaylistCommandsBuilder
                                       ("webplaylist-cmds");

      this.m_webPlaylistCommands.appendPlaylistCommands(null,
                                                        "library_cmdobj_play",
                                                        this.m_cmd_Play);
      this.m_webPlaylistCommands.appendPlaylistCommands(null,
                                                        "library_cmdobj_pause",
                                                        this.m_cmd_Pause);

      this.m_webPlaylistCommands.appendPlaylistCommands(null,
                                                        "library_cmdobj_remove",
                                                        this.m_cmd_Remove);
      this.m_webPlaylistCommands.appendPlaylistCommands(null,
                                                        "library_cmdobj_download",
                                                        this.m_cmd_Download);


      this.m_webPlaylistCommands.appendPlaylistCommands(null,
                                                        "library_cmdobj_addtoplaylist",
                                                        SBPlaylistCommand_DownloadToPlaylist);

      this.m_webPlaylistCommands.appendPlaylistCommands(null,
                                                        "library_cmdobj_copylocation",
                                                        this.m_cmd_CopyTrackLocation);
      this.m_webPlaylistCommands.appendSeparator(null, "separator");
      this.m_webPlaylistCommands.appendPlaylistCommands(null,
                                                        "library_cmdobj_showdlplaylist",
                                                        this.m_cmd_ShowDownloadPlaylist);
      this.m_webPlaylistCommands.appendSeparator(null, "web_commands_separator_2");

      this.m_webPlaylistCommands.appendPlaylistCommands(null,
                                                        "library_cmdobj_queuenext",
                                                        this.m_cmd_QueueNext);

      this.m_webPlaylistCommands.appendPlaylistCommands(null,
                                                        "library_cmdobj_queuelast",
                                                        this.m_cmd_QueueLast);

      this.m_webPlaylistCommands.setVisibleCallback(plCmd_ShowDefaultInToolbarCheck);

      this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_WEBPLAYLIST, this.m_webPlaylistCommands);

      // Register these commands to the web playlist

      var webListGUID =
        prefs.getComplexValue("songbird.library.web",
                              Ci.nsISupportsString);

      this.m_mgr.registerPlaylistCommandsMediaItem(webListGUID, "", this.m_webPlaylistCommands);

      // --------------------------------------------------------------------------
      // Construct and publish the web media history toolbar commands
      // --------------------------------------------------------------------------

      this.m_webMediaHistoryToolbarCommands = new PlaylistCommandsBuilder
                                                  ("webhistory-cmds");

      this.m_webMediaHistoryToolbarCommands.appendPlaylistCommands
                                              (null,
                                               "library_cmdobj_clearhistory",
                                               this.m_cmd_ClearHistory);

      this.m_webMediaHistoryToolbarCommands.setVisibleCallback
                                                      (plCmd_ShowForToolbarCheck);

      this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_WEBTOOLBAR,
                         this.m_webMediaHistoryToolbarCommands);

      // Register these commands to the web media history library

      this.m_mgr.registerPlaylistCommandsMediaItem
                                                (webListGUID,
                                                 "",
                                                 this.m_webMediaHistoryToolbarCommands);
      // --------------------------------------------------------------------------
      // Construct and publish the download playlist commands
      // --------------------------------------------------------------------------

      this.m_downloadCommands = new PlaylistCommandsBuilder
                                    ("download-cmds");

      this.m_downloadCommands.appendPlaylistCommands(null,
                                                     "library_cmdobj_play",
                                                     this.m_cmd_Play);
      this.m_downloadCommands.appendPlaylistCommands(null,
                                                     "library_cmdobj_pause",
                                                     this.m_cmd_Pause);
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

      this.m_mgr.registerPlaylistCommandsMediaItem(this.m_downloadListGuid, "", this.m_downloadCommands);

      // --------------------------------------------------------------------------
      // Construct and publish the device library commands
      // --------------------------------------------------------------------------

      this.m_deviceLibraryCommands = new PlaylistCommandsBuilder
                                         ("device-cmds");

      this.m_deviceLibraryCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_edit",
                                                    this.m_cmd_Edit);
      this.m_deviceLibraryCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_reveal",
                                                    this.m_cmd_Reveal);

      this.m_deviceLibraryCommands.appendSeparator(null, "default_commands_separator_1");

      this.m_deviceLibraryCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_addtolibrary",
                                                    SBPlaylistCommand_AddToLibrary);

      this.m_deviceLibraryCommands.appendSeparator(null, "default_commands_separator_2");

      this.m_deviceLibraryCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_remove",
                                                    this.m_cmd_Remove);

      this.m_deviceLibraryCommands.setVisibleCallback(plCmd_ShowDefaultInToolbarCheck);

      this.m_mgr.publish(kPlaylistCommands.MEDIALIST_DEVICE_LIBRARY_CONTEXTMENU,
                         this.m_deviceLibraryCommands);

      // --------------------------------------------------------------------------
      // Construct and publish the cd device library commands
      // --------------------------------------------------------------------------

      this.m_cdDeviceLibraryCommands = new PlaylistCommandsBuilder
                                           ("cddevice-cmds");

      this.m_cdDeviceLibraryCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_check",
                                                    this.m_cmd_CheckAll);

      this.m_cdDeviceLibraryCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_uncheck",
                                                    this.m_cmd_UncheckAll);

      this.m_cdDeviceLibraryCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_edit",
                                                    this.m_cmd_Edit);

      this.m_cdDeviceLibraryCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_lookup",
                                                    this.m_cmd_LookupCDInfo);

      this.m_mgr.publish(kPlaylistCommands.MEDIALIST_CDDEVICE_LIBRARY,
                         this.m_cdDeviceLibraryCommands);

      // --------------------------------------------------------------------------
      // Construct and publish the play queue library commands
      // --------------------------------------------------------------------------

      this.m_playQueueLibraryCommands = new PlaylistCommandsBuilder
                                            ("playqueue-toolbar-cmds");

      this.m_playQueueLibraryCommands.appendPlaylistCommands(null,
                                                    "playqueue_cmdobj_savetoplaylist",
                                                    this.m_cmd_playqueue_SaveToPlaylist);

      this.m_playQueueLibraryCommands.appendPlaylistCommands(null,
                                                    "playqueue_cmdobj_clearall",
                                                    this.m_cmd_playqueue_ClearAll);

      this.m_playQueueLibraryCommands.appendPlaylistCommands(null,
                                                    "playqueue_cmdobj_clearhistory",
                                                    this.m_cmd_playqueue_ClearHistory);

      this.m_playQueueLibraryCommands.setVisibleCallback(plCmd_ShowForToolbarCheck);

      this.m_mgr.publish(kPlaylistCommands.MEDIALIST_PLAYQUEUE_LIBRARY,
                         this.m_playQueueLibraryCommands);

      // get the GUID for the play queue's media list
      var queueService = Cc["@songbirdnest.com/Songbird/playqueue/service;1"]
                           .getService(Ci.sbIPlayQueueService);
      var playQueueListGUID = queueService.mediaList.guid;

      this.m_mgr.registerPlaylistCommandsMediaItem(playQueueListGUID, "", this.m_playQueueLibraryCommands);

      // --------------------------------------------------------------------------
      // Construct and publish the play queue commands
      // --------------------------------------------------------------------------

      this.m_playQueueCommands = new PlaylistCommandsBuilder
                                     ("playqueue-menu-cmds");

      this.m_playQueueCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_play",
                                                    this.m_cmd_Play);

      this.m_playQueueCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_pause",
                                                    this.m_cmd_Pause);

      this.m_playQueueCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_reveal",
                                                    this.m_cmd_Reveal);

      this.m_playQueueCommands.appendSeparator(null, "default_commands_separator_1");

      this.m_playQueueCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_addtoplaylist",
                                                    SBPlaylistCommand_AddToPlaylist);

      this.m_playQueueCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_addtolibrary",
                                                    SBPlaylistCommand_AddToLibrary);

      this.m_playQueueCommands.appendSeparator(null, "default_commands_separator_2");

      this.m_playQueueCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_rescan",
                                                    this.m_cmd_Rescan);

      this.m_playQueueCommands.appendPlaylistCommands(null,
                                                    "library_cmdobj_remove",
                                                    this.m_cmd_Remove);

      this.m_playQueueCommands.setVisibleCallback(plCmd_HideForToolbarCheck);

      this.m_mgr.publish(kPlaylistCommands.MEDIAITEM_PLAYQUEUE, this.m_playQueueCommands);

      this.m_mgr.registerPlaylistCommandsMediaItem(playQueueListGUID, "", this.m_playQueueCommands);

      // --------------------------------------------------------------------------
      // Construct and publish the download toolbar commands
      // --------------------------------------------------------------------------

      this.m_downloadToolbarCommands = new PlaylistCommandsBuilder
                                           ("download-toolbar-cmds");

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
                                                (this.m_downloadListGuid,
                                                 "",
                                                 this.m_downloadToolbarCommands);

      // --------------------------------------------------------------------------
      // Construct and publish the download service tree commands
      // --------------------------------------------------------------------------

      this.m_downloadCommandsServicePane = new PlaylistCommandsBuilder
                                               ("download-servicepane-cmds");

      this.m_downloadCommandsServicePane.
        appendPlaylistCommands(null,
                               "library_cmdobj_pauseresumedownload",
                               this.m_cmd_PauseResumeDownload);

      this.m_downloadCommandsServicePane.setInitCallback(plCmd_DownloadInit);
      this.m_downloadCommandsServicePane.setShutdownCallback(plCmd_DownloadShutdown);

      this.m_mgr.publish(kPlaylistCommands.MEDIALIST_DOWNLOADPLAYLIST, this.m_downloadCommandsServicePane);

      this.m_mgr.registerPlaylistCommandsMediaList(this.m_downloadListGuid, "", this.m_downloadCommandsServicePane);

      // --------------------------------------------------------------------------
      // Construct and publish the service tree playlist commands
      // --------------------------------------------------------------------------

      this.m_serviceTreeDefaultCommands = new PlaylistCommandsBuilder
                                              ("servicetree-cmds");

      this.m_serviceTreeDefaultCommands.appendPlaylistCommands(null,
                                              "servicetree_cmdobj_play",
                                              this.m_cmd_list_Play);

      this.m_serviceTreeDefaultCommands.appendPlaylistCommands(null,
                                              "servicetree_cmdobj_remove",
                                              this.m_cmd_list_Remove);

      this.m_serviceTreeDefaultCommands.setCommandEnabledCallback(null,
                                              "servicetree_cmdobj_remove",
                                              plCmd_CanModifyPlaylist);

      this.m_serviceTreeDefaultCommands.appendPlaylistCommands(null,
                                              "servicetree_cmdobj_rename",
                                              this.m_cmd_list_Rename);

      this.m_serviceTreeDefaultCommands.setCommandEnabledCallback(null,
                                              "servicetree_cmdobj_rename",
                                              plCmd_CanModifyPlaylist);

      this.m_serviceTreeDefaultCommands.appendPlaylistCommands(null,
                                              "servicetree_cmdobj_queuenext",
                                              this.m_cmd_list_QueueNext);

      this.m_serviceTreeDefaultCommands.appendPlaylistCommands(null,
                                              "servicetree_cmdobj_queuelast",
                                              this.m_cmd_list_QueueLast);

      this.m_mgr.publish(kPlaylistCommands.MEDIALIST_DEFAULT, this.m_serviceTreeDefaultCommands);

      // Register these commands to simple and smart playlists

      this.m_mgr.registerPlaylistCommandsMediaList( "", "simple", this.m_serviceTreeDefaultCommands );
      this.m_mgr.registerPlaylistCommandsMediaList( "", "smart",  this.m_serviceTreeDefaultCommands );
    } catch (e) {
      Cu.reportError(e);
    }

    // notify observers that the default commands are now ready
    var observerService = Cc["@mozilla.org/observer-service;1"]
                            .getService(Ci.nsIObserverService);
    observerService.notifyObservers(null, "playlist-commands-ready", "default");
  },

  // ==========================================================================
  // SHUTDOWN
  // ==========================================================================
  shutdownCommands: function() {
    // notify observers that the default commands are going away
    var observerService = Cc["@mozilla.org/observer-service;1"]
                            .getService(Ci.nsIObserverService);
    observerService.notifyObservers(null, "playlist-commands-shutdown", "default");

    var prefs = Cc["@mozilla.org/preferences-service;1"]
                  .getService(Ci.nsIPrefBranch2);

    // Un-publish atomic commands

    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_PLAY, this.m_cmd_Play);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_PAUSE, this.m_cmd_Pause);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_REMOVE, this.m_cmd_Remove);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_EDIT, this.m_cmd_Edit);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_DOWNLOAD, this.m_cmd_Download);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_RESCAN, this.m_cmd_Rescan);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_REVEAL, this.m_cmd_Reveal);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_ADDTOPLAYLIST, SBPlaylistCommand_AddToPlaylist);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_ADDTODEVICE, SBPlaylistCommand_AddToDevice);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_ADDTOLIBRARY, SBPlaylistCommand_AddToLibrary);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_COPYTRACKLOCATION, this.m_cmd_CopyTrackLocation);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_SHOWDOWNLOADPLAYLIST, this.m_cmd_ShowDownloadPlaylist);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_PAUSERESUMEDOWNLOAD, this.m_cmd_PauseResumeDownload);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_CLEANUPDOWNLOADS, this.m_cmd_CleanUpDownloads);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_CLEARHISTORY, this.m_cmd_ClearHistory);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_GETARTWORK, this.m_cmd_GetArtwork);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_QUEUENEXT, this.m_cmd_QueueNext);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_QUEUELAST, this.m_cmd_QueueLast);

    this.m_mgr.withdraw(kPlaylistCommands.MEDIALIST_PLAY, this.m_cmd_list_Play);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIALIST_REMOVE, this.m_cmd_list_Remove);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIALIST_RENAME, this.m_cmd_list_Rename);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIALIST_QUEUENEXT, this.m_cmd_list_QueueNext);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIALIST_QUEUELAST, this.m_cmd_list_QueueLast);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIALIST_UPDATESMARTMEDIALIST, this.m_cmd_UpdateSmartPlaylist);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIALIST_EDITSMARTMEDIALIST, this.m_cmd_EditSmartPlaylist);

    // Un-publish bundled commands

    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_DEFAULT, this.m_defaultCommands);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_WEBPLAYLIST, this.m_webPlaylistCommands);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_WEBTOOLBAR, this.m_webMediaHistoryToolbarCommands);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_DOWNLOADPLAYLIST, this.m_downloadCommands);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_DOWNLOADTOOLBAR, this.m_downloadToolbarCommands);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIALIST_DEFAULT, this.m_serviceTreeDefaultCommands);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIALIST_DOWNLOADPLAYLIST, this.m_downloadCommandsServicePane);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_SMARTPLAYLIST, this.m_smartPlaylistsCommands);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIALIST_DEVICE_LIBRARY_CONTEXTMENU, this.m_deviceLibraryCommands);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIALIST_DEVICE_LIBRARY_TOOLBAR, this.m_cmd_DeviceToolbarCmds);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIALIST_CDDEVICE_LIBRARY, this.m_cdDeviceLibraryCommands);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIAITEM_PLAYQUEUE, this.m_playQueueCommands);
    this.m_mgr.withdraw(kPlaylistCommands.MEDIALIST_PLAYQUEUE_LIBRARY, this.m_playQueueLibraryCommands);

    // Un-register download playlist commands

    this.m_mgr.unregisterPlaylistCommandsMediaItem(this.m_downloadListGuid,
                                                   "",
                                                   this.m_downloadCommands);

    this.m_mgr.unregisterPlaylistCommandsMediaItem
                                              (this.m_downloadListGuid,
                                               "",
                                               this.m_downloadToolbarCommands);

    this.m_mgr.unregisterPlaylistCommandsMediaList
                                              (this.m_downloadListGuid,
                                               "",
                                               this.m_downloadCommandsServicePane);

    g_downloadDevice = null;

    g_webLibrary = null;

    // Un-register web playlist commands

    var webListGUID =
      prefs.getComplexValue("songbird.library.web",
                            Ci.nsISupportsString);

    this.m_mgr.unregisterPlaylistCommandsMediaItem(webListGUID,
                                                   "",
                                                   this.m_webPlaylistCommands);

    // Un-register web media history commands

    this.m_mgr.
      unregisterPlaylistCommandsMediaItem(webListGUID,
                                          "",
                                          this.m_webMediaHistoryToolbarCommands);

    // Un-register the playqueue commands
    var queueService = Cc["@songbirdnest.com/Songbird/playqueue/service;1"]
                         .getService(Ci.sbIPlayQueueService);
    var playQueueListGUID = queueService.mediaList.guid;

    this.m_mgr.
      unregisterPlaylistCommandsMediaItem(playQueueListGUID,
                                          "",
                                          this.m_playQueueCommands);

    this.m_mgr.
      unregisterPlaylistCommandsMediaItem(playQueueListGUID,
                                          "",
                                          this.m_playQueueLibraryCommands);


    // Un-register smart playlist commands

    this.m_mgr.
      unregisterPlaylistCommandsMediaItem("",
                                          "smart",
                                          this.m_smartPlaylistsCommands);


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
    // remains in their internal arrays. The AddTo helpers may or may not be
    // active depending on which set of playlist commands is actually used,
    // but their shutdownCommands methods are no-ops if the commands do not
    // exist.
    SBPlaylistCommand_AddToPlaylist.shutdownCommands();
    SBPlaylistCommand_AddToDevice.shutdownCommands();
    SBPlaylistCommand_AddToLibrary.shutdownCommands();
    this.m_cmd_Play.shutdown();
    this.m_cmd_Pause.shutdown();
    this.m_cmd_Remove.shutdown();
    this.m_cmd_Edit.shutdown();
    this.m_cmd_Download.shutdown();
    this.m_cmd_Rescan.shutdown();
    this.m_cmd_Reveal.shutdown();
    this.m_cmd_CopyTrackLocation.shutdown();
    this.m_cmd_ShowDownloadPlaylist.shutdown();
    this.m_cmd_PauseResumeDownload.shutdown();
    this.m_cmd_CleanUpDownloads.shutdown();
    this.m_cmd_ClearHistory.shutdown();
    this.m_cmd_GetArtwork.shutdown();
    this.m_cmd_QueueNext.shutdown();
    this.m_cmd_QueueLast.shutdown();
    this.m_cmd_list_Play.shutdown();
    this.m_cmd_list_Remove.shutdown();
    this.m_cmd_list_Rename.shutdown();
    this.m_cmd_list_QueueNext.shutdown();
    this.m_cmd_list_QueueLast.shutdown();
    this.m_cmd_UpdateSmartPlaylist.shutdown();
    this.m_cmd_EditSmartPlaylist.shutdown();
    this.m_cmd_SmartPlaylistSep.shutdown();
    this.m_defaultCommands.shutdown();
    this.m_webPlaylistCommands.shutdown();
    this.m_webMediaHistoryToolbarCommands.shutdown();
    this.m_smartPlaylistsCommands.shutdown();
    this.m_downloadCommands.shutdown();
    this.m_downloadToolbarCommands.shutdown();
    this.m_downloadCommandsServicePane.shutdown();
    this.m_serviceTreeDefaultCommands.shutdown();
    this.m_deviceLibraryCommands.shutdown();
    this.m_cmd_ShowOnlyOnDevice.shutdown();
    this.m_cmd_ShowAllOnDevice.shutdown();
    this.m_cmd_DeviceToolbarCmds.shutdown();
    this.m_cdDeviceLibraryCommands.shutdown();
    this.m_cmd_playqueue_SaveToPlaylist.shutdown();
    this.m_cmd_playqueue_ClearAll.shutdown();
    this.m_cmd_playqueue_ClearHistory.shutdown();
    this.m_playQueueCommands.shutdown();
    this.m_playQueueLibraryCommands.shutdown();
    this.m_cmd_LookupCDInfo.shutdown();
    this.m_cmd_CheckAll.shutdown();
    this.m_cmd_UncheckAll.shutdown();

    g_dataRemoteService = null;

    var obs = Cc["@mozilla.org/observer-service;1"]
                .getService(Ci.nsIObserverService);

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
    XPCOMUtils.generateQI([Ci.nsIObserver])
};

function unwrap(obj) {
  if (obj && obj.wrappedJSObject)
    obj = obj.wrappedJSObject;
  return obj;
}

// --------------------------------------------------------------------------
// Called when the play action is triggered
function plCmd_Play_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var playbackControl = Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
                          .getService(Ci.sbIMediacoreManager).playbackControl;
  var view = unwrap(aContext.playlist).mediaListView;

  /* if the current selection is in the sequencer, but not actively playing
   * have the sequencer resume playback */
  if (isCurrentSelectionActive(view)) {
    playbackControl.play();
  }
  else if (plCmd_IsAnyTrackSelected(aContext, aSubMenuId, aCommandId, aHost)) {
    /* if something is selected but it is not active at the moment,
     * begin playing it */
    unwrap(aContext.playlist).sendPlayEvent();
  }
}

// Called to determine if the Play playlist command should be visible
function plCmd_Play_VisibleCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var view = unwrap(aContext.playlist).mediaListView

  // we don't show play when more than 1 item is selected
  if (view.selection.count > 1)
  {
    return false;
  }

  var mediaCoreMgr = Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
                    .getService(Ci.sbIMediacoreManager);

  /* we show the play command as long as we are not currently playing the
   * selected track*/
  return !(mediaCoreMgr.status.state == Ci.sbIMediacoreStatus.STATUS_PLAYING &&
           isCurrentSelectionActive(view))
}

/* Returns true if the currently selected track is active in the sequencer.
 * This does not mean the track is necessarily playing as it could be paused */
function isCurrentSelectionActive(mediaListView) {
  var mediaCoreMgr = Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
                       .getService(Ci.sbIMediacoreManager);

  var sequencer = mediaCoreMgr.sequencer;
  var currItem = mediaListView.selection.currentMediaItem;
  var currMediaList = mediaListView.mediaList;

  // if the sequencer's active item is the selected item, return true
  return (currItem.equals(sequencer.currentItem) &&
          currMediaList.equals(sequencer.view.mediaList) &&
          mediaListView.selection.currentIndex == sequencer.viewPosition);
}

// Called when the pause action is triggered
function plCmd_Pause_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var playbackControl = Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
                       .getService(Ci.sbIMediacoreManager).playbackControl;
  playbackControl.pause();
}

// Called to determine if the Pause playlist command should be visible
function plCmd_Pause_VisibleCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var selection = unwrap(aContext.playlist).mediaListView.selection;

  // we don't show pause when more than 1 item is selected
  if (selection.count > 1)
  {
    return false;
  }

  var mediaCoreMgr = Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
                       .getService(Ci.sbIMediacoreManager);

  // figure out if the play command is currently visible
  var isPlayVisible =
      plCmd_Play_VisibleCallback(aContext, aSubMenuId, aCommandId, aHost);

  /* we only show the pause command if we are currently playing and we are not
   * currently showing the play command */
  return (!isPlayVisible &&
          mediaCoreMgr.status.state == Ci.sbIMediacoreStatus.STATUS_PLAYING);
}

// Called when the edit action is triggered
function plCmd_Edit_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  if (plCmd_IsAnyTrackSelected(aContext, aSubMenuId, aCommandId, aHost)) {
    var playlist = unwrap(aContext.playlist);
    playlist.onPlaylistEditor();
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
    if(playlist.mediaListView.selection.count) {
      onBrowserTransfer(new SelectionUnwrapper(
                       playlist.mediaListView.selection.selectedIndexedMediaItems));
    }
    else {
      var allItems = {
        items: [],
        onEnumerationBegin: function(aMediaList) {
        },
        onEnumeratedItem: function(aMediaList, aMediaItem) {
          this.items.push(aMediaItem);
        },
        onEnumerationEnd: function(aMediaList, aResultCode) {
        }
      };

      playlist.mediaList.enumerateAllItems(allItems);

      var itemEnum = ArrayConverter.enumerator(allItems.items);
      onBrowserTransfer(itemEnum);
    }
  }
  catch( err )
  {
    Cu.reportError(err);
  }
}

function plCmd_Rescan_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  try
  {
    var playlist = unwrap(aContext.playlist);
    var window = unwrap(aContext.window);

    if(playlist.mediaListView.selection.count) {
      var mediaItemArray = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                             .createInstance(Ci.nsIMutableArray);

      var selection = playlist.mediaListView.selection.selectedIndexedMediaItems;
      while(selection.hasMoreElements()) {
        let item = selection.getNext()
                            .QueryInterface(Ci.sbIIndexedMediaItem).mediaItem;
        mediaItemArray.appendElement(item, false);
      }

      var metadataService = Cc["@songbirdnest.com/Songbird/FileMetadataService;1"]
                              .getService(Ci.sbIFileMetadataService);
      var job = metadataService.read(mediaItemArray);
      SBJobUtils.showProgressDialog(job, null);
    }
  }
  catch( err )
  {
    Cu.reportError(err);
  }
}

function plCmd_Reveal_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  try
  {
    var playlist = unwrap(aContext.playlist);
    var window = unwrap(aContext.window);

    if (playlist.mediaListView.selection.count != 1) { return; }

    var selection = playlist.mediaListView.selection.selectedIndexedMediaItems;
    var item = selection.getNext().QueryInterface(Ci.sbIIndexedMediaItem).mediaItem;
    if (!item) {
      Cu.reportError("No item selected in reveal playlist command.")
    }

    var uri = item.contentSrc;
    if (!uri || uri.scheme != "file") { return; }

    let f = uri.QueryInterface(Ci.nsIFileURL).file;
    try {
      // Show the directory containing the file and select the file
      f.QueryInterface(Ci.nsILocalFile);
      f.reveal();
    } catch (e) {
      // If reveal fails for some reason (e.g., it's not implemented on unix or
      // the file doesn't exist), try using the parent if we have it.
      let parent = f.parent.QueryInterface(Ci.nsILocalFile);
      if (!parent)
        return;

      try {
        // "Double click" the parent directory to show where the file should be
        parent.launch();
      } catch (e) {
        // If launch also fails (probably because it's not implemented), let the
        // OS handler try to open the parent
        var parentUri = Cc["@mozilla.org/network/io-service;1"]
                          .getService(Ci.nsIIOService).newFileURI(parent);

        var protocolSvc = Cc["@mozilla.org/uriloader/external-protocol-service;1"]
                            .getService(Ci.nsIExternalProtocolService);
        protocolSvc.loadURI(parentUri);
      }
    }
  }
  catch( err )
  {
    Cu.reportError(err);
  }
}

// Called when the "copy track location" action is triggered
function plCmd_CopyTrackLocation_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var clipboardtext = "";
  var urlCol = "url";
  var playlist = unwrap(aContext.playlist);

  var selectedEnum = playlist.mediaListView.selection.selectedIndexedMediaItems;
  while (selectedEnum.hasMoreElements()) {
    var curItem = selectedEnum.getNext();
    if (curItem) {
      var originURL = curItem.mediaItem.getProperty(SBProperties.contentURL);
      if (clipboardtext != "")
        clipboardtext += "\n";
      clipboardtext += originURL;
    }
  }

  var clipboard = Cc["@mozilla.org/widget/clipboardhelper;1"]
                    .getService(Ci.nsIClipboardHelper);
  clipboard.copyString(clipboardtext);
}

// Called whne the "show download playlist" action is triggered
function plCmd_ShowDownloadPlaylist_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var window = unwrap(aContext.window);
  var browser = window.gBrowser;
  if (browser) {
    var device = getDownloadDevice();
    if (device) {
      browser.loadMediaList(device.downloadMediaList);
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

// Called when the clear history action is triggered
function plCmd_ClearHistory_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var wl = getWebLibrary();
  if (wl) {
    wl.clear();
  }
}

// Called when the get album artwork action is triggered
function plCmd_GetArtwork_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  // Load up a fetcher set, create a list and start the fetch
  if (plCmd_IsAnyTrackSelected(aContext, aSubMenuId, aCommandId, aHost)) {
    var playlist = unwrap(aContext.playlist);

    // Only look up artwork for audio items.
    var isAudioItem = function(aElement) {
      return aElement.getProperty(SBProperties.contentType) == "audio";
    };
    var selectedAudioItems = new SBFilteredEnumerator(
        playlist.mediaListView.selection.selectedMediaItems,
        isAudioItem);

    // We need to convert our JS object into an XPCOM object.
    // Mook claims this is actually the best way to do this.
    var sip = Cc["@mozilla.org/supports-interface-pointer;1"]
                    .createInstance(Ci.nsISupportsInterfacePointer);
    sip.data = selectedAudioItems;
    selectedAudioItems = sip.data;

    sbCoverHelper.getArtworkForItems(selectedAudioItems,
                                     null,
                                     playlist.library);
  }
}

// Called when the queueNext action is triggered
function plCmd_QueueNext_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var wrapper =
      plCmd_GetSelectedItemWrapperForPlaylist(unwrap(aContext.playlist));
  if (wrapper) {
    let queueService = Cc["@songbirdnest.com/Songbird/playqueue/service;1"]
                       .getService(Ci.sbIPlayQueueService);
    queueService.queueSomeNext(wrapper);
  }
}

// Called when the queueLast action is triggered
function plCmd_QueueLast_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var wrapper =
      plCmd_GetSelectedItemWrapperForPlaylist(unwrap(aContext.playlist));
  if (wrapper) {
    let queueService = Cc["@songbirdnest.com/Songbird/playqueue/service;1"]
                         .getService(Ci.sbIPlayQueueService);
    queueService.queueSomeLast(wrapper);
  }
}

// Helper function to get a wrapped enumerator of the selected items in a
// mediaList. Returns null if nothing is selected.
function plCmd_GetSelectedItemWrapperForPlaylist(aPlaylist) {
  var wrapper = null;
  if (aPlaylist.mediaListView.selection.count) {
    let indexedSelection =
      aPlaylist.mediaListView.selection.selectedIndexedMediaItems;
    wrapper = Cc["@songbirdnest.com/Songbird/Library/EnumeratorWrapper;1"]
                .createInstance(Ci.sbIMediaListEnumeratorWrapper);
    wrapper.initialize(indexedSelection);
  }

  return wrapper;
}

// Called when the save to playlist action is triggered
function plCmd_QueueSaveToPlaylist_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var queueService = Cc["@songbirdnest.com/Songbird/playqueue/service;1"]
                       .getService(Ci.sbIPlayQueueService);
  var newMediaList = aContext.window.makeNewPlaylist("simple");
  newMediaList.addAll(queueService.mediaList);
}

// Called when the clear all action is triggered
function plCmd_QueueClearAll_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var queueService = Cc["@songbirdnest.com/Songbird/playqueue/service;1"]
                       .getService(Ci.sbIPlayQueueService);
  queueService.clearAll();
}

// Called when the clear history playlist action is triggered
function plCmd_QueueClearHistory_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var queueService = Cc["@songbirdnest.com/Songbird/playqueue/service;1"]
                       .getService(Ci.sbIPlayQueueService);
  queueService.clearHistory();
}

// Called when the lookup cd info action is triggered
function plCmd_LookupCDInfo_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var firstItem;
  try {
    var playlist = unwrap(aContext.playlist);
    var medialist = playlist.mediaListView.mediaList;
    var firstItem = medialist.getItemByIndex(0);

    if (!firstItem) {
      Cu.reportError("Unable to get CD Device: " + err);
      return;
    }
  } catch (err) {
    Cu.reportError("Unable to get CD Device: " + err);
    return;
  }

  var devMgr = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                 .getService(Ci.sbIDeviceManager2);
  var device = devMgr.getDeviceForItem(firstItem);
  if (!device) {
    Cu.reportError("Unable to get CD Device");
    return;
  }

  // Invoke the CD Lookup
  var bag = Cc["@mozilla.org/hash-property-bag;1"]
              .createInstance(Ci.nsIPropertyBag2);
  device.submitRequest(Ci.sbICDDeviceEvent.REQUEST_CDLOOKUP, bag);
}

// The 'Show new items only' command for device trigger callback
function plCmd_ShowOnlyOnDevice_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  // Get the current view and append a filter so we only get items that do
  // not have a known origin in the main library
  var playlist = unwrap(aContext.playlist);
  var view = playlist.mediaListView;
  var propIndex = view.cascadeFilterSet
                      .appendFilter(SBProperties.originIsInMainLibrary);

  // Constrain the filter to allow only items with '0' as
  // originIsInMainLibrary to be displayed.
  var constraints = [0];
  view.cascadeFilterSet.set(propIndex, constraints, constraints.length);

  // As we are now displaying only items on the device, refresh commands
  // so that 'Show new items' becomes 'Show all items'.
  playlist.refreshCommands();
}

// The 'Show all items' command for device trigger callback
function plCmd_ShowAllOnDevice_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  // Get the current view and remove the originIsInMainLibrary filter
  var playlist = unwrap(aContext.playlist);
  var filterSet = playlist.mediaListView.cascadeFilterSet;
  for (var i = 0; i < filterSet.length; i++) {
    if (filterSet.getProperty(i) == SBProperties.originIsInMainLibrary) {
      filterSet.remove(i);
      break;
    }
  }

  // As we are now displaying all items on the device, refresh commands
  // so that 'Show all items' becomes 'Show new items'.
  playlist.refreshCommands();
}

// Given a playlist binding, grab all the items with the shouldRip property
// set to aVal and set them to aNewVal
function playlist_setRipProperty(aPlaylist, aVal, aNewVal) {
  var mediaList = aPlaylist.mediaListView.mediaList;
  mediaList.enumerateItemsByProperty(
                   "http://songbirdnest.com/data/1.0#shouldRip",
                   aVal,
                   {
                     onEnumerationBegin: function() { },
                     onEnumeratedItem: function(aList, aItem) {
                       aItem.setProperty(
                         "http://songbirdnest.com/data/1.0#shouldRip",
                         aNewVal);
                       return Ci.sbIMediaListEnumerationListener.CONTINUE;
                     },
                     onEnumerationEnd: function() { }
                   });
  aPlaylist.refreshCommands(false);
  return;
}

// Called when the select all cd info action is triggered
function plCmd_CheckAll_TriggerCallback(aContext,
                                         aSubMenuId, aCommandId, aHost) {
  var pls = unwrap(aContext.playlist);
  playlist_setRipProperty(pls, "0", "1");
}

function plCmd_UncheckAll_TriggerCallback(aContext,
                                         aSubMenuId, aCommandId, aHost) {
  var pls = unwrap(aContext.playlist);
  playlist_setRipProperty(pls, "1", "0");
  return;
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

// Called when the "play playlist" action is triggered
function plCmd_PlayPlaylist_TriggerCallback(aContext,
                                            aSubMenuId, aCommandId, aHost) {
  var window = unwrap(aContext.window);
  var mediaList = unwrap(aContext.medialist);
  var gBrowser = window.gBrowser;
  var gMM = Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
              .getService(Ci.sbIMediacoreManager);
  gBrowser.loadMediaList(mediaList);
  var view = mediaList.createView();
  gMM.sequencer.playView(view);
}

// Called when the "remove playlist" action is triggered
function plCmd_RemoveList_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  unwrap(aContext.window).SBDeleteMediaList(aContext.medialist);
}

// Called when the "rename playlist" action is triggered
function plCmd_RenameList_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var window = unwrap(aContext.window);
  var medialist = unwrap(aContext.medialist);
  var servicePane = window.gServicePane;
  // If we have a servicetree, tell it to make the new playlist node editable
  if (servicePane) {
    var librarySPS = Cc['@songbirdnest.com/servicepane/library;1']
                       .getService(Ci.sbILibraryServicePaneService);
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
    var promptService = Cc["@mozilla.org/embedcomp/prompt-service;1"  ]
                          .getService(Ci.nsIPromptService);

    var input = {value: medialist.name};
    var title = SBString("renamePlaylist.title", "Rename Playlist");
    var prompt = SBString("renamePlaylist.prompt", "Enter the new name of the playlist.");

    if (promptService.prompt(window, title, prompt, input, null, {})) {
      medialist.name = input.value;
    }
  }
}

// Called when the queueNext action is triggered for a playlist
function plCmd_QueueListNext_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var mediaList = unwrap(aContext.medialist);
  var queueService = Cc["@songbirdnest.com/Songbird/playqueue/service;1"]
                       .getService(Ci.sbIPlayQueueService);
  queueService.queueNext(mediaList);
}

// Called when the queueLast action is triggered for a playlist
function plCmd_QueueListLast_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var mediaList = unwrap(aContext.medialist);
  var queueService = Cc["@songbirdnest.com/Songbird/playqueue/service;1"]
                       .getService(Ci.sbIPlayQueueService);
  queueService.queueLast(mediaList);
}

function plCmd_UpdateSmartPlaylist_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var medialist = unwrap(aContext.medialist);
  if (medialist instanceof Ci.sbILocalDatabaseSmartMediaList)
    medialist.rebuild();
}

function plCmd_EditSmartPlaylist_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var medialist = unwrap(aContext.medialist);
  if (medialist instanceof Ci.sbILocalDatabaseSmartMediaList) {
    var watcher = Cc["@mozilla.org/embedcomp/window-watcher;1"]
                    .getService(Ci.nsIWindowWatcher);
    watcher.openWindow(aContext.window,
                       "chrome://songbird/content/xul/smartPlaylist.xul",
                       "_blank",
                       "chrome,resizable=yes,dialog=yes,centerscreen,modal,titlebar=no",
                       medialist);
    unwrap(aContext.playlist).refreshCommands();
  }
}

// Returns true when at least one track is selected in the playlist
function plCmd_IsAnyTrackSelected(aContext, aSubMenuId, aCommandId, aHost) {
  return (unwrap(aContext.playlist).mediaListView.selection.count != 0);
}


// If we have any audio selected, show this command.
function plCmd_IsAnyAudioSelected(aContext, aSubMenuId, aCommandId, aHost) {
  var selection = unwrap(aContext.playlist).mediaListView.selection;

  // if the current media item is audio, just return.
  var item = selection.currentMediaItem;
  if (item && item.getProperty(SBProperties.contentType) == "audio") {
    return true;
  }

  if (selection.isContentTypeSelected("audio"))
    return true;

  return false;
}

// Returns true when the library is not read only.
function plCmd_IsMediaListReadOnly(aContext, aSubMenuId, aCommandId, aHost) {
  return (unwrap(aContext.playlist).mediaListView
                                   .mediaList
                                   .getProperty(SBProperties.isReadOnly) == "1");
}

// Returns true when "select all" should be visible, per comment #1 in
// bug 18049: It should default to 'Check All' unless all tracks are
// already checked, in which case it should say 'Uncheck All'. This means if the
// playlist is partially checked the button should read 'Check All' and to
// uncheck all tracks the user will need to click twice.
function plCmd_CheckAllVisibleCallback(aContext, aSubMenuId, aCommandId, aHost)
{
  var returnValue;
  var pls = unwrap(aContext.playlist);
  var checkedItemCount = 0;

  var mediaList = pls.mediaListView.mediaList;
  mediaList.enumerateItemsByProperty(
                   "http://songbirdnest.com/data/1.0#shouldRip",
                   "1",
                   {
                     onEnumerationBegin: function() { },
                     onEnumeratedItem: function(aList, aItem) {
                       checkedItemCount++;
                     },
                     onEnumerationEnd: function() { }
                   });
  returnValue = mediaList.length != checkedItemCount;
  return returnValue;
}

// Returns true when at least one track is selected in the playlist and none of the selected tracks have downloading forbidden
function plCmd_Download_EnabledCallback(aContext, aSubMenuId, aCommandId, aHost) {
  if (!plCmd_IsAnyTrackSelected(aContext, aSubMenuId, aCommandId, aHost)) {
    return false;
  }
  try {
    var playlist = unwrap(aContext.playlist);
    var window = unwrap(aContext.window);
    var enumerator = playlist.mediaListView.selection.selectedMediaItems;
    while (enumerator.hasMoreElements()) {
      var item = enumerator.getNext().QueryInterface(Ci.sbIMediaItem);
      if (!item) continue; // WTF?
      if (item.getProperty(SBProperties.disableDownload) == '1') {
        // one of the items has download disabled, we need to disable the command
        return false;
      }
    }
    // none of the items had download disabled, enable the command
    return true;
  } catch (e) {
    Cu.reportError(err);
    // something bad happened - I say no.
    return false;
  }
}

// Returns true if the 'rescan' item command is enabled
function plCmd_IsRescanItemEnabled(aContext, aSubMenuId, aCommandId, aHost) {
  var prefs = Cc["@mozilla.org/preferences-service;1"]
                .getService(Ci.nsIPrefBranch2);
  var enabled = false;
  try {
   enabled = prefs.getBoolPref("songbird.commands.enableRescanItem");
  } catch (e) {
    // nothing to do
  }
  return enabled;
}

// Returns true if the 'reveal' command is enabled
function plCmd_isSelectionRevealable(aContext, aSubMenuId, aCommandId, aHost) {
  var selection = unwrap(aContext.playlist).mediaListView.selection;
  if (selection.count != 1) { return false; }
  var items = selection.selectedIndexedMediaItems;
  var item = items.getNext().QueryInterface(Ci.sbIIndexedMediaItem).mediaItem;
  return (item.contentSrc.scheme == "file")
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

// Return true if the playlist is empty
function plCmd_IsNotEmptyPlaylist(aContext, aSubMenuId, aCommandId, ahost) {
  return !!(unwrap(aContext.medialist).length);
}

// Return true if the play queue context menu items should be enabled for
// the selected track(s).
function plCmd_ContextMenuQueueEnabled(aContext, aSubMenuId, aCommandId, aHost) {
  if (!plCmd_IsAnyTrackSelected(aContext, aSubMenuId, aCommandId, aHost)) {
    return false;
  }

  var queueService = Cc["@songbirdnest.com/Songbird/playqueue/service;1"]
                       .getService(Ci.sbIPlayQueueService);

  return !queueService.operationInProgress;
}

// Return true if the play queue context menu items should be enabled for
// the selected list.
function plCmd_ListContextMenuQueueEnabled(aContext, aSubMenuId, aCommandId, aHost) {
  if (!plCmd_IsNotEmptyPlaylist(aContext, aSubMenuId, aCommandId, aHost)) {
    return false;
  }

  var queueService = Cc["@songbirdnest.com/Songbird/playqueue/service;1"]
                       .getService(Ci.sbIPlayQueueService);

  return !queueService.operationInProgress;
}

// Returns true if the playlist can be modified (is not read-only)
function plCmd_CanModifyPlaylist(aContext, aSubMenuId, aCommandId, aHost) {
  return !(plCmd_isReadOnlyLibrary(aContext, aSubMenuId, aCommandId, aHost) ||
           plCmd_isReadOnlyPlaylist(aContext, aSubMenuId, aCommandId, aHost));
}

// Returns true if the playlist can be added to the play queue
function plCmd_CanQueuePlaylist(aContext, aSubMenuId, aCommandId, aHost) {

  // Don't allow device playlists to be added to the play queue. See comments in
  // Bug 21895.
  var deviceManager = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                        .getService(Ci.sbIDeviceManager2);

  // device will be null if the list is not in a device library
  var device = deviceManager.getDeviceForItem(unwrap(aContext.medialist));

  return !device;
}

// Returns true if the playlist content can be modified (is not read-only)
function plCmd_CanModifyPlaylistContent(aContext, aSubMenuId, aCommandId, aHost) {
  return !(plCmd_isReadOnlyLibraryContent(aContext, aSubMenuId, aCommandId, aHost) ||
           plCmd_isReadOnlyPlaylistContent(aContext, aSubMenuId, aCommandId, aHost));
}

// Returns true if the library the playlist is in is read-only
function plCmd_isReadOnlyLibrary(aContext, aSubMenuId, aCommandId, aHost) {
  var medialist = unwrap(aContext.medialist);
  return !medialist.library.userEditable;
}

// Returns true if the playlist is read-only
function plCmd_isReadOnlyPlaylist(aContext, aSubMenuId, aCommandId, aHost) {
  var medialist = unwrap(aContext.medialist);
  return !medialist.userEditable;
}

// Returns true if the library the playlist is in is read-only
function plCmd_isReadOnlyLibraryContent(aContext, aSubMenuId, aCommandId, aHost) {
  if (plCmd_isReadOnlyLibrary(aContext, aSubMenuId, aCommandId, aHost))
    return true;
  var medialist = unwrap(aContext.medialist);
  return !medialist.library.userEditableContent;
}

// Returns true if the playlist is read-only
function plCmd_isReadOnlyPlaylistContent(aContext, aSubMenuId, aCommandId, aHost) {
  if (plCmd_isReadOnlyPlaylist(aContext, aSubMenuId, aCommandId, aHost))
    return true;
  var medialist = unwrap(aContext.medialist);
  return !medialist.userEditableContent;
}

// Returns true if the conditions are ok for adding tracks to the library
function plCmd_CanAddToLibrary(aContext, aSubMenuId, aCommandId, aHost) {
  return plCmd_IsAnyTrackSelected(aContext, aSubMenuId, aCommandId, aHost) &&
         plCmd_CanModifyPlaylistContent(aContext, aSubMenuId, aCommandId, aHost) &&
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

// Returns true if there are items in the web media history
function plCmd_WebMediaHistoryHasItems(aContext, aSubMenuId, aCommandId, aHost) {
  var wl = getWebLibrary();
  return wl && !wl.isEmpty;
}

// Returns true if the supplied context contains a gBrowser object
function plCmd_ContextHasBrowser(aContext, aSubMenuId, aCommandId, aHost) {
  var window = unwrap(aContext.window);
  return (window.gBrowser);
}

// Returns true if the playlist is a smart playlist
function plCmd_isSmartPlaylist(aContext, aSubMenuId, aCommandId, aHost) {
  var medialist = unwrap(aContext.medialist);
  return (medialist.type == "smart");
}

function plCmd_isLiveUpdateSmartPlaylist(aContext, aSubMenuId, aCommandId, aHost) {
  var medialist = unwrap(aContext.medialist);
  if (medialist instanceof Ci.sbILocalDatabaseSmartMediaList)
    return medialist.autoUpdate;
  return false;
}

// Returns a function that will return the conjunction of the result of the inputs
function plCmd_AND( /* comma separated list (not array) of functions */ ) {
  var methods = Array.prototype.concat.apply([], arguments);
  return function _plCmd_Conjunction(aContext, aSubMenuId, aCommandId, aHost) {
    for each (var f in methods) {
      if (!f(aContext, aSubMenuId, aCommandId, aHost))
        return false;
    }
    return true;
  }
}

// Returns a function that will return the disjunction of the result of the inputs
function plCmd_OR( /* comma separated list (not array) of functions */ ) {
  var methods = Array.prototype.concat.apply([], arguments);
  return function _plCmd_Disjunction(aContext, aSubMenuId, aCommandId, aHost) {
    for each (var f in methods) {
      if (f(aContext, aSubMenuId, aCommandId, aHost))
        return true;
    }
    return false;
  }
}

// Returns a function that will return the negation of the result of the input
function plCmd_NOT( aFunction ) {
  return function _plCmd_Negation(aContext, aSubMenuId, aCommandId, aHost) {
    return !aFunction(aContext, aSubMenuId, aCommandId, aHost);
  }
}

// Always return false
function plCmd_False(aContext, aSubMenuId, aCommandId, aHost) {
  return false
}

function onBrowserTransfer(mediaItems, parentWindow)
{
  var ddh =
    Cc["@songbirdnest.com/Songbird/DownloadDeviceHelper;1"]
      .getService(Ci.sbIDownloadDeviceHelper);
  ddh.downloadSome(mediaItems);
}

var g_downloadDevice = null;
function getDownloadDevice() {
  try
  {
    if (!g_downloadDevice) {
      var deviceManager = Cc["@songbirdnest.com/Songbird/DeviceManager;1"]
                            .getService(Ci.sbIDeviceManager);

      if (deviceManager)
      {
        var downloadCategory = 'Songbird Download Device';
        if (deviceManager.hasDeviceForCategory(downloadCategory))
        {
          g_downloadDevice = deviceManager.getDeviceByCategory(downloadCategory);
          g_downloadDevice = g_downloadDevice.QueryInterface
                                      (Ci.sbIDownloadDevice);
        }
      }
    }
    return g_downloadDevice;
  } catch(e) {
    Cu.reportError(e);
  }
  return null;
}

var g_webLibrary = null
function getWebLibrary() {
  try {
    if (g_webLibrary == null) {
      var prefs = Cc["@mozilla.org/preferences-service;1"]
        .getService(Ci.nsIPrefBranch2);
      var webListGUID = prefs.getComplexValue("songbird.library.web",
          Ci.nsISupportsString);
      var libraryManager =
        Cc["@songbirdnest.com/Songbird/library/Manager;1"]
          .getService(Ci.sbILibraryManager);
      g_webLibrary = libraryManager.getLibrary(webListGUID);
    }
  } catch(e) {
    Cu.reportError(e);
  }

  return g_webLibrary;
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

// Return true if we are currently showing only device only tracks
// (i.e. if there is a filter in place for originIsInMainLibrary)
function isShowingDeviceOnly(playlist) {
  var filterSet = playlist.mediaListView.cascadeFilterSet;
  for (var i = 0; i < filterSet.length; i++) {
    if (filterSet.getProperty(i) === SBProperties.originIsInMainLibrary) {
      return true;
    }
  }
  return false;
}

// 'Show new items only' device playlist command visibility callback
function plCmd_ShowOnlyOnDevice_VisibleCallback(aContext, aHost) {
  var playlist = unwrap(aContext.playlist);
  return (aHost == "toolbar" && !isShowingDeviceOnly(playlist));
}

// 'Show all items' device playlist command visibility callback
function plCmd_ShowAllOnDevice_VisibleCallback(aContext, aHost) {
  var playlist = unwrap(aContext.playlist);
  return (aHost == "toolbar" && isShowingDeviceOnly(playlist));
}

function plCmd_DownloadInit(aContext, aHost) {
  var implementorContext = {
    context: aContext,
    batchHelper: new LibraryUtils.BatchHelper(),
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

    onBeforeListCleared: function(aMediaList, aExcludeLists) {
      return true;
    },

    onListCleared: function(aMediaList, aExcludeLists) {
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

    onItemAdded: function(aMediaList, aMediaItem, aIndex) { return true; },
    onBeforeItemRemoved: function(aMediaList, aMediaItem, aIndex) {
      return true;
    },
    onItemUpdated: function(aMediaList, aMediaItem, aProperties) {
      return true;
    },
    onItemMoved: function(aMediaList, aFromIndex, aToIndex) { return true; },


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
  var consoleService = Cc['@mozilla.org/consoleservice;1']
                         .getService(Ci.nsIConsoleService);
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
