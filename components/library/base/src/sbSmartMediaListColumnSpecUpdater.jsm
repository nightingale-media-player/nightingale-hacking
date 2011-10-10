/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://app/jsmodules/sbColumnSpecParser.jsm");

EXPORTED_SYMBOLS = ["SmartMediaListColumnSpecUpdater"];

const Cc = Components.classes;
const Ci = Components.interfaces;

var SmartMediaListColumnSpecUpdater = {

  update: function(list) {
    var sort;
    var sortDirection;
    var columnWidth;

    var storageGuid = list.getProperty(SBProperties.storageGUID);
    var storageList = list.library.getMediaItem(storageGuid);
    
    // if we are limiting, use the selectby property/direction as sort
    if (list.limitType != Ci.sbILocalDatabaseSmartMediaList.LIMIT_TYPE_NONE) {
      if (list.randomSelection) {
        // unless we are selecting randomly, in which case we want the ordinal
        // order (unsorted)
        sort = SBProperties.ordinal;
        sortDirection = "a";
      } else {
        sort = list.selectPropertyID;
        sortDirection = list.selectDirection ? "a" : "d";
      }
    } else {
      // if there is one rule, use it as the sort, otherwise use
      // default (artistname)
      if (list.conditionCount == 1) {
        var condition = list.getConditionAt(0);
        if (!this._isDummyProperty(condition.propertyID)) {
          sort = condition.propertyID;
        } else {
          sort = SBProperties.artistName;
        }
      } else {
        sort = SBProperties.artistName;
      }
    }
    
    // find the property in the smartplaylistpropertyregistrar
    var registeredProp;
    var registrar = 
      Cc["@getnightingale.com/Nightingale/SmartPlaylistPropertyRegistrar;1"]
        .getService(Ci.sbISmartPlaylistPropertyRegistrar);
    var props = registrar.getPropertiesForContext("default");
    while (props.hasMoreElements()) {
      var p = props.getNext();
      p.QueryInterface(Ci.sbISmartPlaylistProperty);
      if (p.propertyID == sort) {
        registeredProp = p;
        break;
      }
    }
    // if not found, the property was not in the registrar, this could be a
    // programmatically built smart playlist, or the user might have turned on
    // the extended property list, which is not restricted to the registered
    // properties, so make something up.
    if (!registeredProp) {
      registeredProp = {};
      registeredProp.propertyID = sort;
      registeredProp.defaultColumnWidth = 80;
      registeredProp.defaultSortDirection = this._getDefaultSortDirection(sort);
    }
    
    // if we haven't got a sort direction yet, use the one in the registrar
    if (!sortDirection) {
      sortDirection = registeredProp.defaultSortDirection;
    }
    columnWidth = registeredProp.defaultColumnWidth;

    // get the current user column spec
    var userSpecs = this._getColumnSpec(storageList);
    
    // make the new default column specs based on the library specs
    var columnMap = this._getColumnSpec(LibraryUtils.mainLibrary);

    var currentUserSpec = storageList.getProperty(SBProperties.columnSpec);
    var currentDefaultSpec = storageList.getProperty(SBProperties.defaultColumnSpec);
    
    // Has the user deviated from the default?  
    // If so, we shouldn't undo their changes.
    var userModifiedColumns = currentUserSpec != currentDefaultSpec;

    // playlists have an ordinal column by default, make sure it's in both
    // default and user spec

    var ordinal = {
      property: SBProperties.ordinal,
      sort: "a",
      width: 42
    }
    if (!this._getColumn(columnMap, SBProperties.ordinal)) {
      ColumnSpecParser.reduceWidthsProportionally(columnMap, ordinal.width);
      columnMap.unshift(ordinal);
    }
    if (!this._getColumn(userSpecs, SBProperties.ordinal)) {
      ColumnSpecParser.reduceWidthsProportionally(userSpecs, ordinal.width);
      userSpecs.unshift(ordinal);
    }

    // new column arrays (default and user)
    
    var defaultCols = columnMap;
    var cols = userSpecs;

    // if the new sort column is not in the spec arrays, proportionally resize
    // the other columns to make room for it, and add it
    var newSort = {
      property: sort, 
      sort: null, 
      width: columnWidth
    };
    
    if (!this._getColumn(defaultCols, sort)) {
      ColumnSpecParser.reduceWidthsProportionally(defaultCols, columnWidth);
      defaultCols.push(newSort);
    }
    
    if (!this._getColumn(cols, sort)) {
      ColumnSpecParser.reduceWidthsProportionally(cols, columnWidth);
      cols.push(newSort);
    }
    
    // reset the sort field for everything but our actual sort column
    function resetObsoleteSorts(aArray) {
      for (var i=0;i<aArray.length;i++) {
        var col = aArray[i];
        if (col.property != sort) {
          col.sort = null;
        } else {
          col.sort = sortDirection;
        }
      }
    }
    resetObsoleteSorts(defaultCols);
    if (!userModifiedColumns) {
      // Only reset the current sort if the user hasn't 
      // manually set it
      resetObsoleteSorts(cols);
    }

    // rebuild the column spec strings
    function makeSpecString(aArray) {
      var spec = "";
      for (var i=0;i<aArray.length;i++) {
        var col = aArray[i];
        if (spec != "") spec += " ";
        spec += col.property;
        if (col.width) {
          spec += " ";
          spec += col.width;
        }
        if (col.sort) {
           if (col.sort == "ascending") {
             col.sort = "a";
           }
           else if (col.sort == "descending") {
             col.sort = "d";
           }
          spec += " ";
          spec += col.sort;
        }
      }
      return spec;
    }
    var defaultSpec = makeSpecString(defaultCols);
    var userSpec = makeSpecString(cols);

    // set the new column spec to the storage list.
    storageList.setProperty(SBProperties.defaultColumnSpec, defaultSpec);
    storageList.setProperty(SBProperties.columnSpec, userSpec);
  },

  _getColumn: function(aColumnArray, aPropertyID) {
    for each (var column in aColumnArray) {
      if (column.property == aPropertyID)
        return column;
    }
    return null;
  },

  _getColumnSpec: function(aMediaList) {
    var parser = new ColumnSpecParser(aMediaList, null);
    return parser.columnMap;
  },

  _getDefaultSortDirection: function(prop) {
    var pm = Components.classes["@getnightingale.com/Nightingale/Properties/PropertyManager;1"]
                               .getService(Components.interfaces.sbIPropertyManager);
    var info = pm.getPropertyInfo(prop);
    switch (info.type) {
      case "datetime":
      case "number":
      case "boolean":
      case "duration":
      case "rating":
        return "d";
      case "text":
      case "uri":
        return "a";
    }
    return "d";
  },
  
  _isDummyProperty: function(aProperty) {
    var pm = Cc["@getnightingale.com/Nightingale/Properties/PropertyManager;1"]
               .getService(Components.interfaces.sbIPropertyManager);
    var info = pm.getPropertyInfo(aProperty);
    return (info instanceof Ci.sbIDummyPropertyInfo);
  }
}
