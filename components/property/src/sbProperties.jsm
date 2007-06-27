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

/**
 * Property constants for use with the Songbird property system. Import into a
 * JS file or component using:
 *
 *   'Components.utils.import("rel:SongbirdProperties.jsm");'
 *
 */

EXPORTED_SYMBOLS = ["SBProperties"];

var SBProperties = {

  _base: "http://songbirdnest.com/data/1.0#",

  get base() { return this._base; },

  get storageGUID() { return this._base + "storageGUID"; },

  get created() { return this._base + "created"; },

  get updated() { return this._base + "updated"; },

  get contentURL() { return this._base + "contentURL"; },

  get contentMimeType() { return this._base + "contentMimeType"; },

  get contentLength() { return this._base + "contentLength"; },

  get trackName() { return this._base + "trackName"; },

  get albumName() { return this._base + "albumName"; },

  get artistName() { return this._base + "artistName"; },

  get duration() { return this._base + "duration"; },

  get genre() { return this._base + "genre"; },

  get trackNumber() { return this._base + "trackNumber"; },

  get year() { return this._base + "year"; },

  get discNumber() { return this._base + "discNumber"; },

  get totalDiscs() { return this._base + "totalDiscs"; },

  get totalTracks() { return this._base + "totalTracks"; },

  get isPartOfCompilation() { return this._base + "isPartOfCompilation"; },

  get producerName() { return this._base + "producerName"; },

  get composerName() { return this._base + "composerName"; },

  get lyricistName() { return this._base + "lyricistName"; },

  get lyrics() { return this._base + "lyrics"; },

  get recordLabelName() { return this._base + "recordLabelName"; },

  get albumArtURL() { return this._base + "albumArtURL"; },

  get lastPlayTime() { return this._base + "lastPlayTime"; },

  get playCount() { return this._base + "playCount"; },

  get skipCount() { return this._base + "skipCount"; },

  get rating() { return this._base + "rating"; },

  get originURL() { return this._base + "originURL"; },

  get originPage() { return this._base + "originPage"; },

  get GUID() { return this._base + "GUID"; },

  get hidden() { return this._base + "hidden"; },

  get isList() { return this._base + "isList"; },

  get ordinal() { return this._base + "ordinal"; },

  get mediaListName() { return this._base + "mediaListName"; },
  
  get progressMode() { return this._base + "progressMode"; },

  get progressValue() { return this._base + "progressValue"; },

  get columnSpec() { return this._base + "columnSpec"; },

  get defaultColumnSpec() { return this._base + "defaultColumnSpec"; },

  createArray: function(properties) {
    var propertyArray =
      Components.classes["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
                .createInstance(Components.interfaces.sbIMutablePropertyArray);
      if (properties) {
        properties.forEach(function(e) {
          if (e.length == 2) {
            propertyArray.appendProperty(e[0], e[1]);
          }
        });
      }
      return propertyArray;
  }
}

