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
Cu.import("resource://app/jsmodules/PlayQueueUtils.jsm");
Cu.import("resource://app/jsmodules/SBDataRemoteUtils.jsm");
Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");

var playQueue = {

  _LOG: DebugUtils.generateLogFunction("sbPlayQueue", 5),

  /**
   * Called when the display pane loads.
   */
  onLoad: function playQueue_onLoad() {
    this._LOG(arguments.callee.name);

    this._playlist = document.getElementById("playqueue-playlist");
    this._playlistBox = document.getElementById("playqueue-playlist-box");
    this._innerMessageBox =
      document.getElementById("playqueue-message-layer-inner-box");
    this._messageLayer =
      document.getElementById("playqueue-message-layer-outer-box");
    this._inProgressLayer =
      document.getElementById("playqueue-in-progress-layer-outer-box");
    this._innerInProgressBox =
      document.getElementById("playqueue-in-progress-layer-inner-box");

    var playQueueService = Cc["@songbirdnest.com/Songbird/playqueue/service;1"]
                             .getService(Ci.sbIPlayQueueService);

    // Show the empty queue messaging if the queue is empty
    if (playQueueService.mediaList.isEmpty) {
      this._showEmptyQueueLayer();
    } else {
      this._hideEmptyQueueLayer();
    }

    // Listen to underlying medialist to enable/disable empty queue messaging
    var self = this;
    this._listListener = {
      onItemAdded: function(aList, aItem, aIndex) {
        if (self._listEmpty) {
          self._hideEmptyQueueLayer();
        }
      },
      onAfterItemRemoved: function(aList, aItem, aIndex) {
        if (aList.isEmpty) {
          self._showEmptyQueueLayer();
        }
      },
      onListCleared: function(aList) {
        self._showEmptyQueueLayer();
      }
    };

    playQueueService.mediaList.addListener(this._listListener,
                                           false,
                                           Ci.sbIMediaList.LISTENER_FLAGS_ITEMADDED |
                                           Ci.sbIMediaList.LISTENER_FLAGS_AFTERITEMREMOVED |
                                           Ci.sbIMediaList.LISTENER_FLAGS_LISTCLEARED);

    // Listen to the queue service index updates, for auto-scroll
    this._playQueueServiceListener = {
      onIndexUpdated: function(aToIndex) {
        view.treeView.selection.tree.ensureRowIsVisible(aToIndex);
      },

      onQueueOperationStarted: function() {
        self._playlistBox.setAttribute("disabled", "true");
        self._messageLayer.setAttribute("hidden", "true");
        self._inProgressLayer.removeAttribute("hidden");
      },

      onQueueOperationCompleted: function() {
        self._playlistBox.removeAttribute("disabled");
        self._messageLayer.setAttribute("hidden", "true");
        self._inProgressLayer.setAttribute("hidden", "true");
      }
    };

    playQueueService.addListener(this._playQueueServiceListener);

    // Bind the playlist to a a view.
    var view = LibraryUtils.createStandardMediaListView
                            (playQueueService.mediaList);

    // Attach our listener to the ShowCurrentTrack event issued by the
    // faceplate.  We're in a display pane, so we need to get the main window.
    var sbWindow = Cc["@mozilla.org/appshell/window-mediator;1"]
        .getService(Ci.nsIWindowMediator)
        .getMostRecentWindow("Songbird:Main").window;
    sbWindow.addEventListener("ShowCurrentTrack", this.onShowCurrentTrack, true);

    this._playlist.bind(view);

    var mediacoreManager = Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
                             .getService(Ci.sbIMediacoreManager);
    // Disable the shuffle button when playing from the queue.
    this._remoteShuffleDisabled =
        SB_NewDataRemote( "playlist.shuffle.disabled", null );

    this._mediacoreListener = {
      onMediacoreEvent: function (ev) {
        if (mediacoreManager.sequencer.view == view &&
            ev.type == Ci.sbIMediacoreEvent.VIEW_CHANGE) {
          // we're entering the view, disable shuffle
          mediacoreManager.sequencer.mode =
              Ci.sbIMediacoreSequencer.MODE_FORWARD;
          self._remoteShuffleDisabled.boolValue = true;
        }
      }
    };

    mediacoreManager.addListener(this._mediacoreListener);
  },

  /**
   * Called on unload.
   */
  onUnload: function playQueue_onUnload() {
    this._LOG(arguments.callee.name);

    var playQueueService = Cc["@songbirdnest.com/Songbird/playqueue/service;1"]
                             .getService(Ci.sbIPlayQueueService);

    if (this._listListener) {
      playQueueService.mediaList.removeListener(this._listListener);
      this._listListener = null;
    }
    if (this._playQueueServiceListener) {
      playQueueService.removeListener(this._playQueueServiceListener);
      this._playQueueServiceListener = null;
    }
    if (this._playlist) {
      this._playlist.destroy();
      this._playlist = null;
    }

    // Remove the ShowCurrentTrack event listener.
    var sbWindow = Cc["@mozilla.org/appshell/window-mediator;1"]
        .getService(Ci.nsIWindowMediator)
        .getMostRecentWindow("Songbird:Main").window;
    if (sbWindow)
      sbWindow.removeEventListener("ShowCurrentTrack",
                                   this.onShowCurrentTrack,
                                   true);

   if (this._mediacoreListener) {
      var mediacoreManager = Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
                               .getService(Ci.sbIMediacoreManager);

      mediacoreManager.removeListener(this._mediacoreListener);
      this._mediacoreListener = null;
    }

    if(this._remoteShuffleDisabled) {
      this._remoteShuffleDisabled.unbind();
      this._remoteShuffleDisabled = null;
    }
  },

  /**
   * Dim playlist (via disabled attribute) and show messaging layer when the
   * queue is empty.
   */
  _showEmptyQueueLayer: function playQueue__showEmptyQueueLayer() {
    this._LOG(arguments.callee.name);
    this._playlistBox.setAttribute("disabled", "true");
    this._listEmpty = true;
    this._messageLayer.removeAttribute("hidden");
    this._inProgressLayer.setAttribute("hidden", "true");
  },

  /**
   * Hide messaging layer when queue is not empty.
   */
  _hideEmptyQueueLayer: function playQueue__hideEmptyQueueLayer() {
    this._LOG(arguments.callee.name);
    this._playlistBox.removeAttribute("disabled");
    this._messageLayer.setAttribute("hidden", "true");
    this._inProgressLayer.setAttribute("hidden", "true");
    this._listEmpty = false;
  },

  /**
   * xxx slloyd This handler is not hooked up to the box due to Bug 13018. Also
   *            see comments in Bug 21874.
   *
   * Event handler for dragenter on the messaging layer. If the item can
   * be dropped on the playlist, hide the message box until the item is either
   * dropped or dragged back out of the queue area.
   */
  onEmptyQueueDragEnter:
      function playQueue_onEmptyQueueDragEnter(aEvent) {

    this._LOG(arguments.callee.name);
    if (this._playlist._canDrop(aEvent)) {
      this._playlistBox.removeAttribute("disabled");
      this._innerMessageBox.setAttribute("hidden", "true");
    }
  },

  /*
   * xxx slloyd This handler is not hooked up to the box due to Bug 13018. Also
   *            see comments in Bug 21874.
   *
   * Event handler for dragexit on the messaging layer. Show the message box and
   * dim the playlist.
   */
  onEmptyQueueDragExit:
      function playQueue_onEmptyQueueDragExit(aEvent) {

    this._LOG(arguments.callee.name);
    this._playlistBox.setAttribute("disabled", "true");
    this._innerMessageBox.removeAttribute("hidden");
  },

  /**
   * Event handler for dragdrop on the messaging layer. Handle the drop with the
   * playlist.
   */
  onEmptyQueueDragDrop:
      function playQueue_onEmptyQueueDragDrop(aEvent) {

    this._LOG(arguments.callee.name);
    var dragService = Cc["@mozilla.org/widget/dragservice;1"]
                        .getService(Ci.nsIDragService);
    var session = dragService.getCurrentSession();

    this._playlist._dropOnTree(this._playlist.mediaListView.length,
                               Ci.sbIMediaListViewTreeViewObserver.DROP_AFTER,
                               session);

    // Stop propagation so the default drag and drop handler doesn't try to
    // handle a drop that we already handled with _dropOnTree
    aEvent.stopPropagation();
  },

  /**
   * Event handler for dragover on the in progress layer. Drop disabled.
   */
  onQueueInProgressDragOver:
      function playQueue_onQueueInProgressDragOver(aEvent) {
    this._LOG(arguments.callee.name);
    aEvent.dataTransfer.effectAllowed = "none";
    aEvent.stopPropagation();
  },

  /**
   * Event handler for for the ShowCurrentTrack event - triggered when the
   * user clicks the track title in the faceplate.
   */
  onShowCurrentTrack:
      function playQueue_onShowCurrentTrack(aEvent) {
    var mediacoreManager = Cc['@songbirdnest.com/Songbird/Mediacore/Manager;1']
                             .getService(Ci.sbIMediacoreManager);
    var item = mediacoreManager.sequencer.currentItem;

    var playQueueService = Cc["@songbirdnest.com/Songbird/playqueue/service;1"]
                             .getService(Ci.sbIPlayQueueService);

    // The queue has its own library, so items contained in the media list are
    // unique to the queue.
    if (playQueueService.mediaList.contains(item)) {
      PlayQueueUtils.openPlayQueue();

      var view = mediacoreManager.sequencer.view;
      var row = mediacoreManager.sequencer.viewPosition;
      view.selection.selectOnly(row);
      view.treeView.selection.tree.ensureRowIsVisible(row);
      aEvent.preventDefault();
      aEvent.stopPropagation();
    }
  }

};
