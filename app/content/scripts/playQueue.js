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
 * \file playQueue.js
 * \brief Controller for play queue display pane.
 */

if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;

Cu.import("resource://app/jsmodules/DebugUtils.jsm");
Cu.import("resource://app/jsmodules/kPlaylistCommands.jsm");

var playQueue = {

  _LOG: DebugUtils.generateLogFunction("sbPlayQueue", 5),

  onLoad: function playQueue_onLoad() {
    this._LOG("playQueue.onLoad");

    var playQueueService = Cc["@songbirdnest.com/Songbird/playqueue/service;1"]
                             .getService(Ci.sbIPlayQueueService);
    var view = playQueueService.mediaList.createView();
    this._playlist = document.getElementById("playqueue-playlist");

    var mgr = Cc["@songbirdnest.com/Songbird/PlaylistCommandsManager;1"]
                .createInstance(Ci.sbIPlaylistCommandsManager);

  /*
   * xxx slloyd Use MEDIAITEM_DEFAULT until Bug 22108 gives us playlist commands
   * for the queue.
   */
    var commands = mgr.request(kPlaylistCommands.MEDIAITEM_DEFAULT);

    this._playlist.bind(view, commands);
  },

  onUnload: function playQueue_onUnload() {
    this._LOG("playQueue.onUnload");
    if (this._playlist) {
      this._playlist.destroy();
      this._playlist = null;
    }
  }

};
