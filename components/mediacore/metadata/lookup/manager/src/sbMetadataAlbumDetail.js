/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the "GPL").
 *
 * Software distributed under the License is distributed
 * on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
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

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Ce = Components.Exception;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/WindowUtils.jsm");
Cu.import("resource://app/jsmodules/SBTimer.jsm");

function sbMLAlbumDetail() {
  this._tracks = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                   .createInstance(Ci.nsIMutableArray);

  this._properties =
    Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
    .createInstance(Ci.sbIMutablePropertyArray);
}

sbMLAlbumDetail.prototype = {
  classDescription : 'Songbird Metadata Lookup Album Detail',
  classID          : Components.ID("84dd6e90-1dd2-11b2-bad9-c6f63b798098"),
  contractID       : "@songbirdnest.com/Songbird/MetadataLookup/albumdetail;1",
  QueryInterface   : XPCOMUtils.generateQI([Ci.sbIMetadataAlbumDetail]),

  /** sbIMetadataAlbumDetail **/
  get tracks() {
    return this._tracks;
  },

  get properties() {
    return this._properties;
  },
}

var NSGetModule = XPCOMUtils.generateNSGetFactory([sbMLAlbumDetail]);
