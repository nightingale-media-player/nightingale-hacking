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
 * \file videoWinInit.js
 * \brief Video window initialization and shutdown functions.
 * \internal
 */

//
// Module specific auto-init/deinit support
//
var videoWinInit = {};
videoWinInit.init_once = 0;
videoWinInit.deinit_once = 0;
videoWinInit.onLoad = function()
{
  if (videoWinInit.init_once++) { dump("WARNING: videoWinInit double init!!\n"); return; }
  // Videowin, initialize thyself.
  SBVideoInitialize();
}
videoWinInit.onUnload = function()
{
  if (videoWinInit.deinit_once++) { dump("WARNING: videoWinInit double deinit!!\n"); return; }
  window.removeEventListener("load", videoWinInit.onLoad, false);
  window.removeEventListener("unload", videoWinInit.onUnload, false);
  SBVideoDeinitialize();
}

window.addEventListener("load", videoWinInit.onLoad, false);
window.addEventListener("unload", videoWinInit.onUnload, false);

//
// Video Window Initialization.  Setup all the cores based upon the runtime platform
//
var songbird_playingVideo; // global for DataRemote

// observer for DataRemote
const sb_playing_video_changed = { 
  observe: function ( aSubject, aTopic, aData ) { 
    SBPlayingVideoChanged(aData); 
  } 
} 

/**
 * \brief Initialize the Video window.
 * \note Do not call this more than once.
 * \internal
 */
function SBVideoInitialize()
{
  dump("SBVideoInitialize\n");
  try
  {
    // Cook up the mac pseudo-chrome.
    try {
      fixOSXWindow("cheezy_window_top", "app_title");
    }
    catch (e) { }

    // Create and bind DataRemote
    songbird_playingVideo = SB_NewDataRemote( "faceplate.playingvideo", null );
    songbird_playingVideo.bindObserver( sb_playing_video_changed, true );

    // Set our window constraints
    setVideoMinMaxCallback();

    // Trap ALTF4
    window.addEventListener("keydown", videoCheckAltF4, true);

    windowPlacementSanityChecks();

    var platform;
    try {
      var sysInfo =
        Components.classes["@mozilla.org/system-info;1"]
                  .getService(Components.interfaces.nsIPropertyBag2);
      platform = sysInfo.getProperty("name");
    }
    catch (e) {
      dump("System-info not available, trying the user agent string.\n");
      var user_agent = navigator.userAgent;
      if (user_agent.indexOf("Windows") != -1)
        platform = "Windows_NT";
      else if (user_agent.indexOf("Mac OS X") != -1)
        platform = "Darwin";
      else if (user_agent.indexOf("Linux") != -1)
        platform = "Linux";
      else if (user_agent.indexOf("SunOS") != -1)
        platform = "SunOS";
    }

    //
    // Initialize the video window on the mediacore manager.
    //
    var mm = Components.classes["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
                       .getService(Components.interfaces.sbIMediacoreManager);
    
    var box = document.getElementById("video-box");
    mm.video.videoWindow = box;
    
    if (platform == "Darwin") {
      var quitMenuItem = document.getElementById("menu_FileQuitItem");
      quitMenuItem.removeAttribute("hidden");
    }
    
    // If we are a top level window, hide us.
    if ( window.parent == window && document.__dont_hide_video_window != true) {
      SBHideCoreWindow();
    }
  }
  catch( e ) {
    alert("SBVideoInitialize\n" + e); 
    throw(e);
  }
}

/**
 * \brief Deinitialize the Video window.
 * \note Do not call this more than once.
 * \internal
 */
function SBVideoDeinitialize()
{
  // Stop trapping altf4
  window.removeEventListener("keydown", videoCheckAltF4, true);
  // Reset window constraints
  resetVideoMinMaxCallback();
  // Unbind the playing video watcher. (used by the code that uncloaks the video window)
  songbird_playingVideo.unbind();
  songbird_playingVideo = null;
  // Save position before closing, in case the window has been moved, but its position hasnt been saved yet (the window is still up)
}

/**
 * \brief Contains logic that needs to be applied when the currently playing video changes.
 * \internal
 */
function SBPlayingVideoChanged(value)
{
  var windowCloak =
    Components.classes["@songbirdnest.com/Songbird/WindowCloak;1"]
              .getService(Components.interfaces.sbIWindowCloak);

  if (value == 1) {
    windowCloak.uncloak(window);
    window.focus();
  }
  else {
    // restore window if it was maximized before cloaking it
    restoreWindow();
    // hide window.
    SBHideCoreWindow();
  }
}

/**
 * \brief Handler for the specific UI event from the video window.
 * \internal
 */
function onHideButtonClick()
{
  // Stop video playback
  try {
    var mm = Components.classes["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
               .getService(Components.interfaces.sbIMediacoreManager);
    mm.playbackControl.stop();
  } catch (e) {}

  SBHideCoreWindow();
}

/**
 * \brief Save Video window position and hide it.
 * \deprecated Does not work anymore.
 * \internal
 */
function SBHideCoreWindow()
{
  // Save position before cloaking, because if we close the app after 
  // the window has been cloaked, we can't record its position
  onHide();
}

var SBVideoMinMaxCB =
{
  // Shrink until the box doesn't match the window, then stop.
  GetMinWidth: function()
  {
    // What we'd like it to be
    var outerframe = window.gOuterFrame;
    var retval = 720;
    // However, if in resizing the window size is different from the document's box object
    if (window.innerWidth != outerframe.boxObject.width)
    {
      // That means we found the document's min width.  Because you can't query it directly.
      retval = outerframe.boxObject.width - 1;
    }
    return retval;
  },

  GetMinHeight: function()
  {
    // What we'd like it to be
    var outerframe = window.gOuterFrame;
    var retval = 450;
    // However, if in resizing the window size is different from the document's box object
    if (window.innerHeight != outerframe.boxObject.height)
    {
      // That means we found the document's min height.  Because you can't query it directly.
      retval = outerframe.boxObject.height - 1;
    }
    return retval;
  },

  GetMaxWidth: function()
  {
    return -1;
  },

  GetMaxHeight: function()
  {
    return -1;
  },

  OnWindowClose: function()
  {
    setTimeout(onHideButtonClick, 0);
  },

  QueryInterface : function(aIID)
  {
    if (!aIID.equals(Components.interfaces.sbIWindowMinMaxCallback) &&
        !aIID.equals(Components.interfaces.nsISupports))
    {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }

    return this;
  }
}

/**
 * \brief Set Video window Min/Max width and height window listener.
 * \internal
 */
function setVideoMinMaxCallback()
{
  try {
    var windowMinMax = Components.classes["@songbirdnest.com/Songbird/WindowMinMax;1"];
    if (windowMinMax) {
      var service = windowMinMax.getService(Components.interfaces.sbIWindowMinMax);
      if (service)
        service.setCallback(document, SBVideoMinMaxCB);
    }
  }
  catch (err) {
    // No component
    dump("Error. No WindowMinMax component available." + err + "\n");
  }
}

/**
 * \brief Reset the Video window Min/Max width and height window listener to the default listener.
 * \internal
 */
function resetVideoMinMaxCallback() {
  try {
    var windowMinMax = Components.classes["@songbirdnest.com/Songbird/WindowMinMax;1"];
    if (windowMinMax) {
      var service = windowMinMax.getService(Components.interfaces.sbIWindowMinMax);
      if (service)
        service.resetCallback(document);
    }
  }
  catch (err) {
    // No component
    dump("Error. No WindowMinMax component available." + err + "\n");
  }
}

/**
 * \brief Check for ALT+F4 key combo, close window.
 */
function videoCheckAltF4(evt)
{
  if (evt.keyCode == evt.VK_F4 && evt.altKey)
  {
    onHideButtonClick();
  }
}
