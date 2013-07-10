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
 * \file sbMediaItemControllerTest.js
 * \brief Silly test-only media item controller component for use with the
 * media item controller cleanup test
 */

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const SB_MEDIAITEMCONTROLLER_PARTIALCONTRACTID =
        "@songbirdnest.com/Songbird/library/mediaitemcontroller;1?type=";
const K_TRACKTYPE_ADDED = "TEST_MEDIA_ITEM_CONTROLLER_ADDED";

function TestMediaItemController() {}

TestMediaItemController.prototype = {
  classDescription: "Test Media Item Controller",
  classID: Components.ID("{6c236836-1dd2-11b2-bd72-d1a4ceb6bef3}"),
  contractID: SB_MEDIAITEMCONTROLLER_PARTIALCONTRACTID + K_TRACKTYPE_ADDED
};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([TestMediaItemController]);
