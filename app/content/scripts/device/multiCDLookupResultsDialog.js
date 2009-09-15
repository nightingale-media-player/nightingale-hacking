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

var multiCDDialog = {
  /**
   * \brief Handle load events.
   */
  onload: function onload()
  {
    this.library = window.arguments[0].QueryInterface(Ci.sbIDeviceLibrary);
    this._metadataResults = ArrayConverter.JSArray(
                             window.arguments[1].QueryInterface(
                               Ci.nsISimpleEnumerator));

    this._infolist = document.getElementById("infolist");
    this._other = document.getElementById("other");

    this._jobTracksMap = [];
    this._jobResultIndexMap =
         Cc["@mozilla.org/array;1"].createInstance(Ci.nsIMutableArray);

    for (i in this._metadataResults)
    {
      var result = this._metadataResults[i];
      result.QueryInterface(Ci.sbIMetadataAlbumDetail);
      var props = SBProperties.arrayToJSObject(result.properties);

      var radio =
        this._infolist.insertItemAt(
                                this._infolist.getIndexOfItem(this._other),
                                SBFormattedString("cdrip.lookup.name.format",
                                              [props[SBProperties.artistName],
                                               props[SBProperties.albumName]]),
                                i);

      var tracks = document.createElementNS(
          "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul",
          "sb-cdtracks");

      this._infolist.insertBefore(tracks, this._other);

      // Do the follow-up call to get album detail for each track
      var mlm = Cc["@songbirdnest.com/Songbird/MetadataLookup/manager;1"]
                  .getService(Ci.sbIMetadataLookupManager);
      var job = mlm.defaultProvider.getAlbumDetail(result);

      // Map the job to the results index so we can update it later
      this._jobResultIndexMap.appendElement(job, false);
      // Map the job to the sb-cdtracks xbl
      this._jobTracksMap[i] = tracks;
      if (job.status == Ci.sbIJobProgress.STATUS_RUNNING) {
        job.addJobProgressListener(this);
      } else {
        this.onJobProgress(job);
      }
    }
  },

  /**
   * \brief Job progress listener for album detail follow-up calls
   */
  onJobProgress: function onJobProgress(job)
  {
    job.QueryInterface(Ci.sbIMetadataLookupJob);
    if (job.status != Ci.sbIJobProgress.STATUS_SUCCEEDED)
      return;

    if (job.mlJobType != Ci.sbIMetadataLookupJob.JOB_ALBUM_DETAIL_LOOKUP)
      return;

    function getname(props) {
      return SBProperties.arrayToJSObject(props)[SBProperties.trackName];
    }

    var index = this._jobResultIndexMap.indexOf(0, job);
    var tracks = this._jobTracksMap[index];
    // album detail calls should only ever have one result
    var enum = job.getMetadataResults();
    var result = enum.getNext().QueryInterface(Ci.sbIMetadataAlbumDetail);
    tracks.setTrackTitles(ArrayConverter.JSArray(result.tracks).map(getname));
    this._metadataResults[index] = result;
  },

  /**
   * \brief Handle accept/okay button.
   */
  onaccept: function onaccept()
  {
    if (this._infolist.value == "none")
    {
      openDialog(
               "chrome://songbird/content/xul/device/cdInfoNotFoundDialog.xul",
               null,
               "centerscreen,chrome,modal,titlebar,resizable",
               this.library,
               this._metadataResults);
      return;
    }

    var result = this._metadataResults[parseInt(this._infolist.value)];
    var tracks = ArrayConverter.JSArray(result.tracks);

    for (var i=0; i < tracks.length; i++) {
      // Get the media item in this device library that has the same track
      // number (we add one due to the tracknumber being indexed from 1)
      var trackArr = this.library.getItemsByProperty(SBProperties.trackNumber,
                                                     i+1);
      var item = trackArr.queryElementAt(0, Ci.sbIMediaItem);
      item.setProperties(tracks[i]);
    }
  }
}
