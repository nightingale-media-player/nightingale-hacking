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
Components.utils.import("resource://app/components/sbProperties.jsm");

EXPORTED_SYMBOLS = ["ColumnSpecParser"];

const Cc = Components.classes;
const Ci = Components.interfaces;

var Application = null;

function ColumnSpecParser(aMediaList, aPlaylist) {

  if (!Application) {
    // XXXsteve FUEL is not available in jsm
    Application = Cc["@mozilla.org/fuel/application;1"]
                    .getService(Ci.fuelIApplication);
  }

  // Our new "column spec" is a whitespace separated string that looks like
  // this:
  //
  // "propertyID [20] [a] propertyID [40] [d]"
  //
  // The width and sort direction are optional.

  // Try to get this info from the following places:
  //      1: "columnSpec" property on the medialist
  //      2: Preference indicated by XUL "useColumnSpecPreference" attribute
  //      3: "defaultColumnSpec" property on the medialist
  //      4: "defaultColumnSpec" property on the medialist library
  //      5: "columnSpec" property on the medialist library
  //      6: XUL "columnSpec" attribute
  //      7: Our last-ditch hardcoded list

  var columns;

  columns =
    this._getColumnMap(aMediaList.getProperty(SBProperties.columnSpec),
                       this.ORIGIN_PROPERTY);

  if (!columns.columnMap.length &&
      aPlaylist &&
      aPlaylist.hasAttribute("useColumnSpecPreference")) {
    var pref = aPlaylist.getAttribute("useColumnSpecPreference");

    try {
      if (Application.prefs.has(pref)) {
        columns =
            this._getColumnMap(Application.prefs.get(pref).value,
                               this.ORIGIN_PREFERENCES);
      }
    } catch (e) {}
  }

  if (!columns.columnMap.length) {
    columns =
      this._getColumnMap(aMediaList.getProperty(SBProperties.defaultColumnSpec),
                         this.ORIGIN_MEDIALISTDEFAULT);
  }

  if (!columns.columnMap.length) {
    columns =
      this._getColumnMap(aMediaList.library.getProperty(SBProperties.defaultColumnSpec),
                         this.ORIGIN_LIBRARYDEFAULT);
  }

  if (!columns.columnMap.length) {
    columns =
      this._getColumnMap(aMediaList.library.getProperty(SBProperties.columnSpec),
                         this.ORIGIN_LIBRARY);
  }

  if (!columns.columnMap.length && aPlaylist) {
    columns = this._getColumnMap(aPlaylist.getAttribute("columnSpec"),
                                 this.ORIGIN_ATTRIBUTE);
  }

  if (!columns.columnMap.length) {
    columns =
      ColumnSpecParser.parseColumnSpec(SBProperties.trackName + " 265 " +
                                       SBProperties.duration + " 43 " +
                                       SBProperties.artistName + " 177 a " +
                                       SBProperties.albumName + " 159 " +
                                       SBProperties.genre + " 53 " +
                                       SBProperties.rating + " 80");
    this._columnSpecOrigin = this.ORIGIN_DEFAULT;
  }

  if (!columns.columnMap.length) {
    throw new Error("Couldn't get columnMap!");
  }

  this._columns = columns;
}

ColumnSpecParser.prototype = {

  _columns: null,
  _columnSpecOrigin: null,

  ORIGIN_PROPERTY: 1,
  ORIGIN_PREFERENCES: 2,
  ORIGIN_MEDIALISTDEFAULT: 3,
  ORIGIN_LIBRARYDEFAULT: 4,
  ORIGIN_LIBRARY: 5,
  ORIGIN_ATTRIBUTE: 6,
  ORIGIN_DEFAULT: 7,

  get columnMap() {
    return this._columns.columnMap;
  },

  get origin() {
    return this._columnSpecOrigin;
  },

  get sortID() {
    return this._columns.sortID;
  },

  get sortIsAscending() {
    return this._columns.sortIsAscending;
  },

  _getColumnMap: function(columnSpec, columnSpecOrigin) {
    var columns = {
      columnMap: [],
      sortID: null,
      sortIsAscending: null
    }

    if (columnSpec) {
      try {
        columns = ColumnSpecParser.parseColumnSpec(columnSpec);
        this._columnSpecOrigin = columnSpecOrigin;
      }
      catch (e) {
        Components.utils.reportError(e);
      }
    }

    return columns;
  }

}

ColumnSpecParser.parseColumnSpec = function(spec) {

  var sortID;
  var sortIsAscending;

  // strip leading and trailing whitespace.
  var strippedSpec = spec.match(/^\s*(.*?)\s*$/);
  if (!strippedSpec.length > 0)
    throw new Error("RegEx failed to match string");

  // split based on whitespace.
  var tokens = strippedSpec[1].split(/\s+/);

  var columns = [];
  var columnIndex = -1;
  var seenSort = false;

  for (var index = 0; index < tokens.length; index++) {
    var token = tokens[index];
    if (!token)
      throw new Error("Zero-length token");

    if (isNaN(parseInt(token))) {
      if (token.length == 1 && (token == "a" || token == "d")) {
        // This is a sort specifier
        if (columnIndex < 0) {
          throw new Error("You passed in a bunk string!");
        }
        // Multiple sorts will be ignored!
        if (!seenSort) {
          var column = columns[columnIndex];
          column.sort = token == "a" ? "ascending" : "descending";
          seenSort = true;
          sortID = column.property;
          sortIsAscending = token == "a";
        }
      }
      else {
        // This is a property name.
        columns[++columnIndex] = { property: token, sort: null };
      }
    }
    else {
      // This is a width specifier
      if (columnIndex < 0) {
        throw new Error("You passed in a bunk string!");
      }
      var column = columns[columnIndex];
      column.width = token;
    }
  }

  var result = {
    columnMap: columns,
    sortID: sortID,
    sortIsAscending: sortIsAscending
  }

  return result;
}
