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
 * \file  PlayQueueUtils.jsm
 * \brief Javascript utilities for the play queue.
 */

EXPORTED_SYMBOLS = [ "PlayQueueUtils" ];

const Cc = Components.classes;
const Ci = Components.interfaces;

var PlayQueueUtils = {
  // Open the play queue display pane
  openPlayQueue: function PlayQueueUtils_openPlayQueue() {
    var paneMgr = Cc["@songbirdnest.com/Songbird/DisplayPane/Manager;1"]
                    .getService(Ci.sbIDisplayPaneManager);
    var contentInfo = Cc["@songbirdnest.com/Songbird/playqueue/contentInfo;1"]
                        .createInstance(Ci.sbIDisplayPaneContentInfo);
    paneMgr.showPane(contentInfo.contentUrl);
  }
}
