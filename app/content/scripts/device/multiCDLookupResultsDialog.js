/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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
  library: null,
  _infolist: null,
  // Artist name to be set (for onEnumeratedItem)
  _artistValue: null,
  // Album title to be set (for onEnumeratedItem)
  _albumValue: null,
  // True if user cancels - only set artist/album if empty, don't overwrite
  // existing data.
  _setEmptyOnly: false,
  _curTrackIndex: 1,

  /**
   * \brief Handle load events.
   */
  onload: function onload()
  {
    this.library = window.arguments[0].QueryInterface(Ci.sbIDeviceLibrary);
    this._device = this.library.device;
    this._metadataResults = ArrayConverter.JSArray(
                             window.arguments[1].QueryInterface(
                               Ci.nsISimpleEnumerator));

    this._infolist = document.getElementById("infolist");
    this._other = document.getElementById("other");

    this._jobTracksMap = [];
    this._jobResultIndexMap =
         Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
           .createInstance(Ci.nsIMutableArray);

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
      if (i == 0)
        tracks.setAttribute("first-sb-cdtracks", "true");

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

    this._infolist.selectedIndex = 0;

    // Listen for device events.
    var deviceManager = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                          .getService(Ci.sbIDeviceManager2);
    deviceManager.addEventListener(this);
  },

  /**
   * \brief Handle unload events.
   */
  onunload: function onunload()
  {
    // Remove device event listener.
    var deviceManager = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                          .getService(Ci.sbIDeviceManager2);
    deviceManager.removeEventListener(this);
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
    var enumDetails = job.getMetadataResults();
    var result = enumDetails.getNext().QueryInterface(Ci.sbIMetadataAlbumDetail);
    tracks.setTrackTitles(ArrayConverter.JSArray(result.tracks).map(getname));
    this._metadataResults[index] = result;
  },

  /**
   * \brief Updates element state whenever radio group selection changes.
   */
  onSelectionChange: function onSelectionChange()
  {
    // This might get called during radiogroup construction, before load event.
    if (!this._infolist)
      return;

    document.getElementById("artist-textbox").disabled =
                                      (this._infolist.value != "other");
    document.getElementById("album-textbox").disabled =
                                      (this._infolist.value != "other");
  },

  /**
   * \brief Handle accept/okay button.
   */
  onaccept: function onaccept()
  {
    if (this._infolist.value == "other")
    {
      this._artistValue = document.getElementById("artist-textbox").value ||
                          SBString("cdrip.lookup.default_artistname");
      this._albumValue = document.getElementById("album-textbox").value ||
                         SBString("cdrip.lookup.default_albumname");

      // Update the device library album name if one was specified.
      if (document.getElementById("album-textbox").value)
        this.library.setProperty(SBProperties.albumName, this._albumValue);

      this._setEmptyOnly = false;
      this.library.enumerateAllItems(this);
      return;
    }

    var result = this._metadataResults[parseInt(this._infolist.value)];
    var tracks = ArrayConverter.JSArray(result.tracks);

    if (tracks.length > 0) {
      // Update the device library album name.
      this.library.setProperty
        (SBProperties.albumName,
         result.properties.getPropertyValue(SBProperties.albumName));
    }

    for (var i=0; i < tracks.length; i++) {
      // Get the media item in this device library that has the same track
      // number (we add one due to the tracknumber being indexed from 1)
      var trackArr = this.library.getItemsByProperty(SBProperties.trackNumber,
                                                     i+1);
      var item = trackArr.queryElementAt(0, Ci.sbIMediaItem);
      item.setProperties(tracks[i]);
    }
  },

  /**
   * \brief Handle device events.
   */
  onDeviceEvent: function onDeviceEvent(aEvent) {
    switch(aEvent.type) {
      case Ci.sbIDeviceEvent.EVENT_DEVICE_REMOVED :
        // Close dialog if device is removed.
        if (aEvent.data == this._device)
          window.close();
        break;

      default :
        break;
    }
  },

  /**
   * \brief Handle cancel button.
   */
  oncancel: function oncancel()
  {
    // Populate all of the tracks w/ the default entries.
    this._artistValue = SBString("cdrip.lookup.default_artistname");
    this._albumValue = SBString("cdrip.lookup.default_albumname");

    this._setEmptyOnly = true;
    this.library.enumerateAllItems(this);
  },

  /**
   * sbIMediaListEnumerationListener interface
   */
  onEnumerationBegin: function (aList) {},
  onEnumeratedItem: function (aList, aItem)
  {
    if (!this._setEmptyOnly || !aItem.getProperty(SBProperties.albumArtistName))
      aItem.setProperty(SBProperties.albumArtistName, this._artistValue);
    if (!this._setEmptyOnly || !aItem.getProperty(SBProperties.artistName))
      aItem.setProperty(SBProperties.artistName, this._artistValue);
    if (!this._setEmptyOnly || !aItem.getProperty(SBProperties.albumName))
      aItem.setProperty(SBProperties.albumName, this._albumValue);

    if (!aItem.getProperty(SBProperties.trackName)) {
      // Only pad the track count to two digits since a CD can only have
      // up to 99 tracks on it.
      var curTrackNum = this._curTrackIndex++;
      if (curTrackNum < 10 && this.library.length >= 10) {
        curTrackNum = "0" + curTrackNum;
      }
      aItem.setProperty(SBProperties.trackName,
                        SBFormattedString("cdrip.lookup.default_trackname",
                                          [curTrackNum]));
    }
  },
  onEnumerationEnd: function (aList, aStatus) {}
}
