/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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

// This is a temporary file to house methods that need to roll into
// our new Menubar XBL object that we'll be building for 0.3
function doMenu( command, event ) {
  switch ( command )
  {
    case "menuitem_file_new":
      var enumerator = null;
      if (event && event.shiftKey) {
        if (gBrowser &&
            gBrowser.selectedTab &&
            gBrowser.selectedTab.mediaListView)
        {
          enumerator = gBrowser.selectedTab
                               .mediaListView
                               .selection
                               .selectedMediaItems;
        }
      }
      if(gServicePane.open) {
        SBNewPlaylist(enumerator, true);
      }
    break;
    case "menuitem_file_smart":
      SBNewSmartPlaylist();
    break;
    case "menuitem_file_remote":
      SBSubscribe(null, null);
    break;
    case "menuitem_file_podcast":
      SBNewPodcast();
    break;
    case "menuitem_file_file":
      SBFileOpen();
    break;
    case "menuitem_file_url":
      SBUrlOpen();
    break;
    case "menuitem_file_playlist":
      SBPlaylistOpen();
    break;
    case "menuitem_file_library":
      SBLibraryOpen(null, false);
    break;
    case "menuitem_file_newtab":
      gBrowser.loadOneTab("about:blank", null, null, null, false, null);
      // |setTimeout| here to ensure we set the location bar focus after
      //   gBrowser.loadOneTab brought the new tab to the front
      window.setTimeout(function _locationbarFocus() {
          var locationbar = document.getElementById("location_bar");
          locationbar.value = "";
          locationbar.focus();
        }, 0);
    break;
    case "menuitem_file_closetab":
      gBrowser.removeTab(gBrowser.selectedTab);
    break;
    case "menuitem_file_scan":
      SBScanMedia();
    break;
    case "aboutName": // This has to be hardcoded this way for the stinky mac.
      About();
    break;
    case "menu_FileQuitItem": // Stinky mac again.  See nsMenuBarX::AquifyMenuBar
      quitApp();
    break;
    case "menuitem_control_play":
      var sbIMediacoreStatus = Components.interfaces.sbIMediacoreStatus;
      var state = gMM.status.state;
      if ( state == sbIMediacoreStatus.STATUS_PLAYING ||
           state == sbIMediacoreStatus.STATUS_BUFFERING) {
        gMM.playbackControl.pause();
      }
      else if(state == sbIMediacoreStatus.STATUS_PAUSED) {
        gMM.playbackControl.play();
      // Otherwise dispatch a play event.  Someone should catch this
      // and intelligently initiate playback.  If not, just have
      // the playback service play the default.
      } 
      else {
        var event = document.createEvent("Events");
        event.initEvent("Play", true, true);
        window.dispatchEvent(event);
      }
    break;
    case "menuitem_control_next":
      gMM.sequencer.next();
    break;
    case "menuitem_control_prev":
      gMM.sequencer.previous();
    break;
    case "menuitem_control_shuf": {
      let sequencer = gMM.sequencer;
      let sequencerMode = sequencer.mode;
      sequencer.mode = 
        (sequencerMode != sequencer.MODE_SHUFFLE) ? 
          sequencer.MODE_SHUFFLE : sequencer.MODE_FORWARD;
    }
    break;
    case "menuitem_control_repa":
      gMM.sequencer.repeatMode = gMM.sequencer.MODE_REPEAT_ALL;
    break;
    case "menuitem_control_rep1":
      gMM.sequencer.repeatMode = gMM.sequencer.MODE_REPEAT_ONE;
    break;
    case "menuitem_control_repx":
      gMM.sequencer.repeatMode = gMM.sequencer.MODE_REPEAT_NONE;
    break;
    case "menuitem_control_stop":
      gMM.playbackControl.stop();
    break;
    case "menu_extensions":
      SBOpenPreferences("paneAddons");
    break;
    case "menu_preferences":
      SBOpenPreferences();
    break;
    case "menu_downloadmgr":
      SBOpenDownloadManager();
    break;
    case "menuitem_tools_clearprivatedata":
      Sanitizer.showUI();
    break;
    // Renamed to match FireFox
    case "javascriptConsole":
      javascriptConsole();
    break;
    case "checkForUpdates": {
      var um =
          Components.classes["@mozilla.org/updates/update-manager;1"].
          getService(Components.interfaces.nsIUpdateManager);
      var prompter =
          Components.classes["@mozilla.org/updates/update-prompt;1"].
          createInstance(Components.interfaces.nsIUpdatePrompt);

      // If there's an update ready to be applied, show the "Update Downloaded"
      // UI instead and let the user know they have to restart the browser for
      // the changes to be applied.
      if (um.activeUpdate && um.activeUpdate.state == "pending")
        prompter.showUpdateDownloaded(um.activeUpdate);
      else
        prompter.checkForUpdates();
    }
    break;
    case "services.browse":
      var addonsURL = Application.prefs.get("songbird.url.addons").value;
      gBrowser.loadURI(addonsURL);
    break;

    default:
      return false;
  }
  return true;
}

// Menubar handling
function onMenu(target) {
  // See if this is a special case.
  if (doMenu(target.getAttribute("id"))) {
    return;
  }

  // The value property may contain a pref key that specifies a url to load.
  if (target.value) {
    var pref = Application.prefs.get(target.value);
    if (pref && pref.value) {
      if (typeof gBrowser != 'undefined') {
        gBrowser.loadURI(pref.value);
      } else {
        SBBrowserOpenURLInNewWindow(pref.value);
      }
    }
  }
}
