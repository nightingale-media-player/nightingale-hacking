/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");

// Export data file constants
// NOTE: When changing these constants, be sure to update the file
//       'sbMediaExportDefines.h' as welll!
var TASKFILE_NAME                     = "songbird_export.task";
var TASKFILE_SCHEMAVERSION            = "2"
var TASKFILE_SCHEMAVERSION_HEADER     = "schema-version";
var TASKFILE_ADDEDMEDIALISTS_HEADER   = "added-medialists";
var TASKFILE_REMOVEDMEDIALISTS_HEADER = "removed-medialists";
var TASKFILE_ADDEDMEDIAITEMS_HEADER   = "added-mediaitems";
var TASKFILE_UPDATEDMEDIAITEMS_HEADER = "updated-mediaitems";

// Media exporter prefs.
var PREF_IMPORTEXPORT_ONSHUTDOWN =
           "songbird.library_importexport.autoshutdown";
var PREF_IMPORTEXPORT_ONSTARTUP =
           "songbird.library_importexport.autostartup";
var PREF_EXPORT_TRACKS =
           "songbird.library_exporter.export_tracks";
var PREF_EXPORT_PLAYLISTS =
           "songbird.library_exporter.export_playlists";
var PREF_EXPORT_SMARTPLAYLISTS =
           "songbird.library_exporter.export_smartplaylists";
var PREF_EXPORT_STARTAGENT =
           "songbird.library_exporter.start_agent"

var AppPrefs = Cc["@mozilla.org/fuel/application;1"]
                 .getService(Ci.fuelIApplication).prefs;

//------------------------------------------------------------------------------
// File management utils

function newTestFileURI(aFileName) {
  var fileClone = TEST_FILES.clone();
  fileClone.append(aFileName);

  return newFileURI(fileClone);
}

function removeAllTaskFiles()
{
  var taskFileFolder = Cc["@mozilla.org/file/directory_service;1"]
                         .getService(Ci.nsIProperties)
                         .get("AppRegD", Ci.nsIFile);

  var entries = taskFileFolder.directoryEntries;
  while (entries.hasMoreElements()) {
    var curFile = entries.getNext().QueryInterface(Ci.nsIFile);

    if (curFile.leafName.indexOf(TASKFILE_NAME) > -1) {
      curFile.remove(false);
    }
  }
}

function getExportedTaskFile()
{
  var dataFile = Cc["@mozilla.org/file/directory_service;1"]
                   .getService(Ci.nsIProperties)
                   .get("AppRegD", Ci.nsIFile);
  dataFile.append(TASKFILE_NAME);
  return dataFile;
}

function newLineInputStream(aFile)
{
  var stream = Cc["@mozilla.org/network/file-input-stream;1"]
                 .createInstance(Ci.nsIFileInputStream);
  stream.init(aFile, 0x1, 0, 0);
  stream.QueryInterface(Ci.nsILineInputStream);
  return stream;
}

//------------------------------------------------------------------------------
// Media export pref management

//
// \brief Turn off all media exporting.
//
function setExportNothing()
{
  AppPrefs.setValue(PREF_EXPORT_TRACKS, false);
  AppPrefs.setValue(PREF_EXPORT_PLAYLISTS, false);
  AppPrefs.setValue(PREF_EXPORT_SMARTPLAYLISTS, false);
  AppPrefs.setValue(PREF_EXPORT_STARTAGENT, false);
}

//
// \brief Toggle exporting to only do tracks to the main library.
//
function setExportTracksOnly()
{
  AppPrefs.setValue(PREF_EXPORT_TRACKS, true);
  AppPrefs.setValue(PREF_EXPORT_PLAYLISTS, false);
  AppPrefs.setValue(PREF_EXPORT_SMARTPLAYLISTS, false);
  AppPrefs.setValue(PREF_EXPORT_STARTAGENT, false);
}

//
// \brief Toggle exporting of tracks and regular playlists (not smart).
//
function setExportTracksPlaylists()
{
  AppPrefs.setValue(PREF_EXPORT_TRACKS, true);
  AppPrefs.setValue(PREF_EXPORT_PLAYLISTS, true);
  AppPrefs.setValue(PREF_EXPORT_SMARTPLAYLISTS, false);
  AppPrefs.setValue(PREF_EXPORT_STARTAGENT, false);
}

//
// \brief Toggle exporting of tracks, regular playlists, and smart playlists.
//
function setExportTracksPlaylistsSmartPlaylists()
{
  AppPrefs.setValue(PREF_EXPORT_TRACKS, true);
  AppPrefs.setValue(PREF_EXPORT_PLAYLISTS, true);
  AppPrefs.setValue(PREF_EXPORT_SMARTPLAYLISTS, true);
  AppPrefs.setValue(PREF_EXPORT_STARTAGENT, false);
}

//------------------------------------------------------------------------------
// Export task data parser utility class.
// NOTE: This parser is only used for the unit tests and should not be used as
//       an example in real life.
//       @see bug 16221 for parer upgrading.

function TaskFileDataParser(aDataFile)
{
  this._dataFile = aDataFile;
  this._addedMediaLists = [];
  this._removedMediaLists = [];
  this._addedMediaItems = {};
  this._curMediaListName = "";
  this._curParseMode = -1;

  // Do the parse now
  this._parseDataFile();
}

TaskFileDataParser.prototype =
{
  // Added resource getters
  getAddedMediaLists: function() {
    return this._addedMediaLists;
  },
  getRemovedMediaLists: function() {
    return this._removedMediaLists;
  },
  getAddedMediaItems: function() {
    return this._addedMediaItems;
  },

  // Internal parse mode states
  _STATE_MODE_ADDEDMEDIALISTS   : 1,
  _STATE_MODE_REMOVEDMEDIALISTS : 2,
  _STATE_MODE_ADDEDMEDIAITEMS   : 3,

  _parseDataFile: function() {
    var inputStream = newLineInputStream(this._dataFile);

    var curLine = {};
    var hasMore = true;
    var hasSchema = false;
    var nextItem = 0;
    var result;

    while (hasMore) {
      hasMore = inputStream.readLine(curLine);
      dump(curLine.value + "\n");

      if (curLine.value == "") {
        // empty line
        continue;
      }
      else if ((result = /^\[(.*?)(?::(.*))?\]$/(curLine.value))) {
        // this is a section header
        nextItem = 0;
        switch(result[1]) {
          case TASKFILE_SCHEMAVERSION_HEADER:
            assertEqual(result[2], TASKFILE_SCHEMAVERSION,
                        "schema version mismatch");
            hasSchema = true;
            break;
          case TASKFILE_ADDEDMEDIALISTS_HEADER:
            assertEqual(result[2], null,
                        "unexpected junk in add media lists header");
            this._curParseMode = this._STATE_MODE_ADDEDMEDIALISTS;
            break;
          case TASKFILE_REMOVEDMEDIALISTS_HEADER:
            assertEqual(result[2], null,
                        "unexpected junk in remove media lists header");
            this._curParseMode = this._STATE_MODE_REMOVEDMEDIALISTS;
            break;
          case TASKFILE_ADDEDMEDIAITEMS_HEADER:
            this._curParseMode = this._STATE_MODE_ADDEDMEDIAITEMS;
            assertNotEqual(result[2], null, "No media list name found");
            // remember to decode UTF8 encoded list name
            this._curMediaListName = decodeURIComponent(result[2]);
            break;
          default:
            doFail('Unexpected section header "' + escape(result[1]) + '"');
        }
      }
      else if ((result = /^(\d+)=(.*)$/(curLine.value))) {
        // this is an added-to-medialist or removed item
        assertEqual(result[1], nextItem, "items out of order");
        nextItem++;
        // all names are in utf8
        var curValue = decodeURIComponent(result[2]);
        switch (this._curParseMode) {
          case this._STATE_MODE_ADDEDMEDIALISTS:
            this._addedMediaLists.push(curValue);
            break;

          case this._STATE_MODE_REMOVEDMEDIALISTS:
            this._removedMediaLists.push(curValue);
            break;

          default:
            doFail("Unexpected state " + this._curParseMode);
        }
      }
      else if ((result = /^([0-9a-f-]{36})=(.*)$/(curLine.value))) {
        // this is an added item
        // all names are in utf8
        var curValue = decodeURIComponent(result[2]);
        switch (this._curParseMode) {
          case this._STATE_MODE_ADDEDMEDIAITEMS:
            if (!this._addedMediaItems[this._curMediaListName]) {
              this._addedMediaItems[this._curMediaListName] = [];
            }
            this._addedMediaItems[this._curMediaListName].push(curValue);
            break;

          default:
            doFail("Unexpected state " + this._curParseMode);
        }
      }
      else {
        doFail("Unexpected line " + curLine.value);
      }
    }

    assertTrue(hasSchema, "No schema version information found");

    inputStream.close();
  },
};
