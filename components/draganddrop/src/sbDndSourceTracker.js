/**
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

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

/**
 * \file sbDndSourceTracker.js
 */

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const CONTRACTID = "@songbirdnest.com/Songbird/DndSourceTracker;1";
const CLASSNAME = "Songbird Drag and Drop Source Tracker Service";
const CID = Components.ID("{54b21a93-5b1b-46a2-a60f-5378ccf7f65c}");
const IID = Ci.sbIDndSourceTracker;

function DndSourceTracker() {
  this._sources = {};
}
DndSourceTracker.prototype = {
  _sources: null,
  _nextHandle: 0,

  reset: function reset() {
    this._sources = {};
  },

  registerSource: function registerSource(aSource) {
    var handle = this._nextHandle.toString();
    this._nextHandle++;
    this._sources[handle] = aSource;
    return handle;
  },

  getSource: function getSource(aHandle) {
    if (aHandle in this._sources) {
      return this._sources[aHandle];
    }
    throw Cr.NS_ERROR_NOT_AVAILABLE;
  },

  getSourceSupports: function getSourceSupports(aHandle) {
    return this.getSource(aHandle.data);
  },

  QueryInterface: XPCOMUtils.generateQI([
    IID,
    Ci.nsISupports
  ]),

  className: CLASSNAME,
  classID: CID,
  contractID: CONTRACTID
};


/**
 * ----------------------------------------------------------------------------
 * Registration for XPCOM
 * ----------------------------------------------------------------------------
 */

var NSGetFactory = XPCOMUtils.generateNSGetFactory([DndSourceTracker]);
