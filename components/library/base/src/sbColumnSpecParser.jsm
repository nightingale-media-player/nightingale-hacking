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
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

EXPORTED_SYMBOLS = ["ColumnSpecParser"];

const Cc = Components.classes;
const Ci = Components.interfaces;

// FUEL is not built-in in JSMs
__defineGetter__("Application", function() {
  delete Application;
  Application = Cc["@mozilla.org/fuel/application;1"]
                  .getService(Ci.fuelIApplication);
  return Application;
})

/**
 * Construct a column spec parser and read the column specification immediately
 *
 * @param aMediaList The media list to find columns to display for
 * @param aPlaylist the playlist binding
 * @param aMask a bitfield containing the source to look for; see the
 *              ColumnSpecParser.ORIGIN_* constants
 * @param [optional] aConstraint the media type constraint to use, if any
 */
function ColumnSpecParser(aMediaList, aPlaylist, aMask, aConstraint) {

  if (!aMask) {
    aMask = ~0 >>> 0;
  }

  // Our new "column spec" is a whitespace separated string that looks like
  // this:
  //
  // "propertyID [20] [a] propertyID [40] [d]"
  //
  // The width and sort direction are optional.

  // Try to get this info from the following places:
  //      1: "columnSpec+(constraint)" property on the media list
  //      2: if a constraint is given, preference indicated by
  //         XUL "useColumnSpecPreference" attribute
  //      3: "defaultColumnSpec+(constraint)" property on the medialist
  //      4: "columnSpec+(constraint)" property on the medialist library
  //      5: "defaultColumnSpec+(constraint)" property on the medialist library
  //      6: "columnSpec" property on the medialist
  //      7: Preference indicated by XUL "useColumnSpecPreference" attribute
  //      8: "defaultColumnSpec" property on the medialist
  //      9: "columnSpec" property on the medialist library
  //     10: "defaultColumnSpec" property on the medialist library
  //     11: XUL "columnSpec" attribute
  //     12: Our last-ditch hardcoded list

  // use a function so we can do early returns from it
  var self = this;
  function getColumns() {
    var cols = null;

    /**
     * check for a column spec, with a given constraint
     * @param aPossibleConstraint the constraint to use; may be empty string
     *                            to look for a column spec with no constraints
     * @param aFlag the flag to be added to the column spec origin
     * @return the parsed column spec, or null if not found
     */
    function checkWithConstraint(aPossibleConstraint, aFlag) {

      if (aMask & self.ORIGIN_PROPERTY) {
        let prop = aMediaList.getProperty(SBProperties.columnSpec +
                                            aPossibleConstraint);
        cols = self._getColumnMap(prop, self.ORIGIN_PROPERTY | aFlag);
        if (cols && cols.columnMap.length) {
          return cols;
        }
      }

      // we explicitly do not want to support using constraints with attributes
      // hence we also don't add the flag, since no constraint would be used
      if (aMask & self.ORIGIN_PREFERENCES) {
        if (aPlaylist && aPlaylist.hasAttribute("useColumnSpecPreference")) {
          let pref = aPlaylist.getAttribute("useColumnSpecPreference");
          cols = self._getColumnMap(Application.prefs.getValue(pref, null),
                                    self.ORIGIN_PREFERENCES);
          if (cols && cols.columnMap.length) {
            return cols;
          }
        }
      }

      if (aMask & self.ORIGIN_MEDIALISTDEFAULT) {
        let prop = aMediaList.getProperty(SBProperties.defaultColumnSpec +
                                            aPossibleConstraint);
        cols = self._getColumnMap(prop, self.ORIGIN_MEDIALISTDEFAULT | aFlag);
        if (cols && cols.columnMap.length) {
          return cols;
        }
      }

      if (aMask & self.ORIGIN_LIBRARY) {
        let prop = aMediaList.library
                             .getProperty(SBProperties.columnSpec +
                                            aPossibleConstraint)
        cols = self._getColumnMap(prop, self.ORIGIN_LIBRARY | aFlag);
        if (cols && cols.columnMap.length) {
          return cols;
        }
      }

      if (aMask & self.ORIGIN_LIBRARYDEFAULT) {
        let prop = aMediaList.library
                             .getProperty(SBProperties.defaultColumnSpec +
                                            aPossibleConstraint);
        cols = self._getColumnMap(prop, self.ORIGIN_LIBRARYDEFAULT | aFlag);
        if (cols && cols.columnMap.length) {
          return cols;
        }
      }
      return null;
    }

    if (aConstraint) {
      cols = checkWithConstraint("+(" + aConstraint + ")",
                                 self.ORIGIN_ONLY_CONSTRAINT);
      if (cols && cols.columnMap.length) {
        return cols;
      }
    }

    cols = checkWithConstraint("", 0);
    if (cols && cols.columnMap.length) {
      return cols;
    }

    if (aMask & self.ORIGIN_ATTRIBUTE) {
      if (aPlaylist) {
        cols = self._getColumnMap(aPlaylist.getAttribute("columnSpec"),
                                  self.ORIGIN_ATTRIBUTE);
      }
      if (cols && cols.columnMap.length) {
        return cols;
      }
    }

    // nope, can't find anything at all; use hard coded fallback
    switch (aConstraint) {
      case "video":
        return self._getColumnMap([SBProperties.trackName, 229, "a",
                                   SBProperties.duration, 45,
                                   SBProperties.genre, 101,
                                   SBProperties.year, 45,
                                   SBProperties.rating, 78,
                                   SBProperties.comment, 291,
                                  ].join(" "),
                                  self.ORIGIN_DEFAULT);
      default:
        return self._getColumnMap([SBProperties.trackName, 229,
                                   SBProperties.duration, 45,
                                   SBProperties.artistName, 137, "a",
                                   SBProperties.albumName, 210,
                                   SBProperties.genre, 90,
                                   SBProperties.rating, 78,
                                  ].join(" "),
                                  self.ORIGIN_DEFAULT);
    }
  }

  var columns = getColumns();

  if (!columns || !columns.columnMap.length) {
    throw new Error("Couldn't get columnMap!");
  }

  this._columns = columns;
}

ColumnSpecParser.prototype = {

  _columns: null,
  _columnSpecOrigin: null,

  ORIGIN_PROPERTY:         1 << 0,
  ORIGIN_PREFERENCES:      1 << 1,
  ORIGIN_MEDIALISTDEFAULT: 1 << 2,
  ORIGIN_LIBRARYDEFAULT:   1 << 3,
  ORIGIN_LIBRARY:          1 << 4,
  ORIGIN_ATTRIBUTE:        1 << 5,
  ORIGIN_DEFAULT:          1 << 6,

  // add this bit mask to enable only column spec sources that depend on the
  // constraint given
  ORIGIN_ONLY_CONSTRAINT:  1 << 31,

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

/**
 * The playlist columns are no longer locked to 100% of the screen width,
 * so we often need to resize all columns when appending new ones.
 * 
 * \param aColumnsArray Array of column objects produced by parseColumnSpec
 * \param aNeededWidth Amount of room to free up in the column array
 */
ColumnSpecParser.reduceWidthsProportionally = function(aColumnsArray, 
                                                       aNeededWidth) 
{
  var fullWidth = 0;
  for each (var col in aColumnsArray) {
    if (!col.width || col.width < 80) continue;
    fullWidth += parseInt(col.width);
  }
  for each (var col in aColumnsArray) {
    if (!col.width || col.width < 80) continue;
    var fraction = parseInt(col.width)/fullWidth;
    var subtract = fraction * aNeededWidth;
    // Round down, since it is better to be too small than to overflow
    // and require a scroll bar
    col.width = Math.floor(parseInt(col.width) - subtract);
  }
}
