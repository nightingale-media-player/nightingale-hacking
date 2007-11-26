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
Components.utils.import("resource://app/components/sbProperties.jsm");

EXPORTED_SYMBOLS = ["BatchHelper", "MultiBatchHelper", "LibraryUtils"];

const Cc = Components.classes;
const Ci = Components.interfaces;

var LibraryUtils = {

  get manager() {
    var manager = Cc["@songbirdnest.com/Songbird/library/Manager;1"].
                  getService(Ci.sbILibraryManager);
    if (manager) {
      // Succeeded in getting the library manager, don't do this again.
      this.__defineGetter__("manager", function() {
        return manager;
      });
    }
    return manager;
  },

  get mainLibrary() {
    return this.manager.mainLibrary;
  },

  get webLibrary() {
    var webLibraryGUID = Cc["@mozilla.org/preferences-service;1"].
                         getService(Ci.nsIPrefBranch).
                         getCharPref("songbird.library.web");
    var webLibrary = this.manager.getLibrary(webLibraryGUID);
    delete webLibraryGUID;

    if (webLibrary) {
      // Succeeded in getting the web library, don't ever do this again.
      this.__defineGetter__("webLibrary", function() {
        return webLibrary;
      });
    }

    return webLibrary;
  },

  _standardFilterConstraint: null,
  get standardFilterConstraint() {
    if (!this._standardFilterConstraint) {
      this._standardFilterConstraint = this.createConstraint([
        [
          [SBProperties.isList, ["0"]]
        ],
        [
          [SBProperties.hidden, ["0"]]
        ]
      ]);
    }
    return this._standardFilterConstraint;
  },

  createConstraint: function(aObject) {
    var builder = Cc["@songbirdnest.com/Songbird/Library/ConstraintBuilder;1"]
                    .createInstance(Ci.sbILibraryConstraintBuilder);
    aObject.forEach(function(aGroup, aIndex) {
      aGroup.forEach(function(aPair) {
        var property = aPair[0];
        var values = aPair[1];
        var enumerator = {
          a: values,
          i: 0,
          hasMore: function() {
            return this.i < this.a.length;
          },
          getNext: function() {
            return this.a[this.i++];
          }
        };
        builder.includeList(property, enumerator);
      });
      if (aIndex < aObject.length - 1) {
        builder.intersect();
      }
    });
    return builder.get();
  },

  createStandardSearchConstraint: function(aSearchString) {
    var builder = Cc["@songbirdnest.com/Songbird/Library/ConstraintBuilder;1"]
                    .createInstance(Ci.sbILibraryConstraintBuilder);
    var a = aSearchString.split(" ");
    for (var i = 0; i < a.length; i++) {
      builder.include(SBProperties.artistName, a[i]);
      builder.include(SBProperties.albumName, a[i]);
      builder.include(SBProperties.trackName, a[i]);
      if (i + 1 < a.length) {
        builder.intersect();
      }
    }
    return builder.get();
  }
}

function BatchHelper() {
  this._depth = 0;
}

BatchHelper.prototype.begin =
function BatchHelper_begin()
{
  this._depth++;
}

BatchHelper.prototype.end =
function BatchHelper_end()
{
  this._depth--;
  if (this._depth < 0) {
    throw new Error("Invalid batch depth!");
  }
}

BatchHelper.prototype.depth =
function BatchHelper_depth()
{
  return this._depth;
}

BatchHelper.prototype.isActive =
function BatchHelper_isActive()
{
  return this._depth > 0;
}

function MultiBatchHelper() {
  this._libraries = {};
}

MultiBatchHelper.prototype.get =
function MultiBatchHelper_get(aLibrary)
{
  var batch = this._libraries[aLibrary.guid];
  if (!batch) {
    batch = new BatchHelper();
    this._libraries[aLibrary.guid] = batch;
  }
  return batch;
}

MultiBatchHelper.prototype.begin =
function MultiBatchHelper_begin(aLibrary)
{
  var batch = this.get(aLibrary);
  batch.begin();
}

MultiBatchHelper.prototype.end =
function MultiBatchHelper_end(aLibrary)
{
  var batch = this.get(aLibrary);
  batch.end();
}

MultiBatchHelper.prototype.depth =
function MultiBatchHelper_depth(aLibrary)
{
  var batch = this.get(aLibrary);
  return batch.depth();
}

MultiBatchHelper.prototype.isActive =
function MultiBatchHelper_isActive(aLibrary)
{
  var batch = this.get(aLibrary);
  return batch.isActive();
}
