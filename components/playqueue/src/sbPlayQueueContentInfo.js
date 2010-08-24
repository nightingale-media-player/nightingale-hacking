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
 * \file sbPlayQueueContentInfo.js
 * \brief Implementation of sbIDisplayPaneContentInfo to target play queue UI
          content at display panes.
 */

const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");

// Constants for display pane content info
const SB_PLAYQUEUE_CONTENTURL =
    "chrome://songbird/content/xul/playQueue.xul";
const SB_PLAYQUEUE_CONTENTICON = "";
const SB_PLAYQUEUE_DEFAULTWIDTH = 200;
const SB_PLAYQUEUE_DEFAULTHEIGHT = 300;
const SB_PLAYQUEUE_SUGGESTEDCONTENTGROUPS = "sidebar";

function PlayQueueContentInfo () {
}

PlayQueueContentInfo.prototype = {

  classID: Components.ID("{e0d6e860-1dd1-11b2-8663-a4d535a29859}"),
  classDescription: "Songbird Play Queue Content Info",
  contractID: "@songbirdnest.com/Songbird/playqueue/contentInfo;1",
  _xpcom_categories:
    [{
      category: "display-pane-provider",
      entry: "play-queue"
    }],

  QueryInterface: XPCOMUtils.generateQI([Ci.sbIDisplayPaneContentInfo]),

  //----------------------------------------------------------------------------
  //
  // Implementation of sbIDisplayPaneContentInfo
  //
  //----------------------------------------------------------------------------

  get contentUrl() {
    return SB_PLAYQUEUE_CONTENTURL;
  },

  get contentTitle() {
    return SBString("playqueue.pane.title");
  },

  get contentIcon() {
    return SB_PLAYQUEUE_CONTENTICON;
  },

  get defaultWidth() {
    return SB_PLAYQUEUE_DEFAULTWIDTH;
  },

  get defaultHeight() {
    return SB_PLAYQUEUE_DEFAULTHEIGHT;
  },

  get suggestedContentGroups() {
    return SB_PLAYQUEUE_SUGGESTEDCONTENTGROUPS;
  }

};

var NSGetModule = XPCOMUtils.generateNSGetModule([PlayQueueContentInfo]);
