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

// This is a temporary file to house methods that need to roll into
// our new Menubar XBL object that we'll be building for 0.3

function doMenu( command ) {
  switch ( command )
  {
    case "file.new":
      SBNewPlaylist();
    break;
    case "file.smart":
      SBNewSmartPlaylist();
    break;
    case "file.remote":
      SBSubscribe( "", "", "", "" );
    break;
    case "file.file":
      SBFileOpen();
    break;
    case "file.url":
      SBUrlOpen();
    break;
    case "file.mab":
      SBMabOpen();
    break;
    case "file.playlist":
      SBPlaylistOpen();
    break;
    case "file.scan":
      SBScanMedia();
    break;
    case "file.dlfolder":
      SBSetDownloadFolder();
    break;
    case "file.watch":
      SBWatchFolders();
    break;
/*    
    case "file.htmlbar":
      if ( SBDataGetIntValue( "option.htmlbar" ) == 0 )
      {
        SBDataSetIntValue( "option.htmlbar", 1 );
      }
      else
      {
        SBDataSetIntValue( "option.htmlbar", 0 );
      }
    break;
*/    

    case "file.koshi":
      SBKoshiOpen();
    break;
    case "aboutName": // This has to be hardcoded this way for the stinky mac.
      About(); 
    break;
    case "menu_FileQuitItem": // Stinky mac again.  See nsMenuBarX::AquifyMenuBar
      quitApp();
    break;
    case "control.play":
      gPPS.playing ? ( gPPS.paused ? gPPS.play() : gPPS.pause() ) : gPPS.play();
    break;
    case "control.next":
      gPPS.next();
    break;
    case "control.prev":
      gPPS.previous();
    break;
    case "control.shuf":
      SBDataSetIntValue( "playlist.shuffle", (SBDataGetIntValue( "playlist.shuffle" ) + 1) % 2 );
    break;
    case "control.repa":
      SBDataSetIntValue( "playlist.repeat", 2 );
    break;
    case "control.rep1":
      SBDataSetIntValue( "playlist.repeat", 1 );
    break;
    case "control.repx":
      SBDataSetIntValue( "playlist.repeat", 0 );
    break;
    case "control.jumpto":
      toggleJumpTo();
    break;
    case "menu.extensions":
      SBOpenPreferences("paneAddons");
    break;
    case "menu_preferences":
      SBOpenPreferences();
    break;
/*    case "menu.downloadmgr":
      SBOpenDownloadManager();
    break;*/
    case "menu.clearprivatedata":
      clearPrivateData();
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
      gBrowser.loadURI(pref.value);
    }
  }
}
