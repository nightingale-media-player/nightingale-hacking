/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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

if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;

Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm"); 
Cu.import("resource://app/jsmodules/StringUtils.jsm");

var multiCDDialog = (function multiCDDialog() {
  var infolist, other;
  var library = window.arguments[0].QueryInterface(Ci.sbIDeviceLibrary);
  var metadataResults =
            ArrayConverter.JSArray(window.arguments[1].QueryInterface(
                                     Ci.nsISimpleEnumerator));

  var XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
  function newelem(tagname) {
    return document.createElementNS(XUL_NS, tagname);
  }

  /**
   * \brief Handle load events.
   */
  multiCDDialog.onload = function onload()
  {
    infolist = document.getElementById("infolist");
    other = document.getElementById("other");

    for (i in metadataResults)
    {
      var result = metadataResults[i];
      result.QueryInterface(Ci.sbIMetadataAlbumDetail);
      var props = SBProperties.arrayToJSObject(result.properties);

      var radio =
          infolist.insertItemAt(infolist.getIndexOfItem(other),
                                SBFormattedString("cdrip.lookup.name.format",
                                              [props[SBProperties.artistName],
                                               props[SBProperties.albumName]]),
                                i);

      var tracks = newelem("sb-cdtracks");
      infolist.insertBefore(tracks, other);

      function getname(props) {
        return SBProperties.arrayToJSObject(props)[SBProperties.trackName];
      }
      tracks.setTrackTitles(ArrayConverter.JSArray(result.tracks).map(getname));
    }
  };

  /**
   * \brief Handle accept/okay button.
   */
  multiCDDialog.onaccept = function onaccept()
  {
    if (infolist.value == "none")
    {
      openDialog(
               "chrome://songbird/content/xul/device/cdInfoNotFoundDialog.xul",
               null,
               "centerscreen,chrome,modal,titlebar,resizable",
               library,
               metadataResults);
      return;
    }

    var result = metadataResults[parseInt(infolist.value)];
    var tracks = ArrayConverter.JSArray(result.tracks);

    for (var i=0; i < tracks.length; i++) {
      // Get the media item in this device library that has the same track
      // number (we add one due to the tracknumber being indexed from 1)
      var trackArr = library.getItemsByProperty(SBProperties.trackNumber, i+1);
      var item = trackArr.queryElementAt(0, Ci.sbIMediaItem);
      item.setProperties(tracks[i]);
    }
  };

  return multiCDDialog;
})();
