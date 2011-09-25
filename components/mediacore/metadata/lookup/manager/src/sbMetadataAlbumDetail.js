/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
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
  this._tracks = Cc["@getnightingale.com/moz/xpcom/threadsafe-array;1"]
                   .createInstance(Ci.nsIMutableArray);

  this._properties =
    Cc["@getnightingale.com/Nightingale/Properties/MutablePropertyArray;1"]
    .createInstance(Ci.sbIMutablePropertyArray);
}

sbMLAlbumDetail.prototype = {
  classDescription : 'Nightingale Metadata Lookup Album Detail',
  classID          : Components.ID("84dd6e90-1dd2-11b2-bad9-c6f63b798098"),
  contractID       : "@getnightingale.com/Nightingale/MetadataLookup/albumdetail;1",
  QueryInterface   : XPCOMUtils.generateQI([Ci.sbIMetadataAlbumDetail]),

  /** sbIMetadataAlbumDetail **/
  get tracks() {
    return this._tracks;
  },

  get properties() {
    return this._properties;
  },
}

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbMLAlbumDetail]);
}
