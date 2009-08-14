/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the "GPL").
 *
 * Software distributed under the License is distributed
 * on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
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

const Cc = Components.classes;
const CC = Components.Constructor;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

const FREEDB_URL = "http://freedb.freedb.org/~cddb/cddb.cgi";
const FREEDB_NS  = "http://freedb.org/#";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/SBJobUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");

var KEYMAP = {
  "DISCID" : FREEDB_NS + "discId",
  "DYEAR"  : SBProperties.year,
  "DGENRE" : SBProperties.genre,
};

/*
 * given an array of key/value pairs, build up a url-encoded query-string
 * formatted string of all the key/values used for GET and POST queries
 */
function urlencode(aArgArray) {
  var s = '';
  for (var k in aArgArray) {
    var v = aArgArray[k];
    if (s.length) { s += '&'; }
    s += encodeURIComponent(k) + '=' + encodeURIComponent(v);
  }
  return s;
}

function POST(url, params, onload, onerror) {
  var xhr = null;
  try {
    xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance();
    xhr.mozBackgroundRequest = true;
    xhr.onload = function(event) { onload(xhr); }
    xhr.onerror = function(event) { onerror(xhr); }
    xhr.open('POST', url, true);
    xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
    xhr.send(urlencode(params));
  } catch(e) {
    Cu.reportError(e);
    onerror(xhr);
  }
  return xhr;
}

function sbFreeDB() {
  // needed to formulate FreeDB commands
  this._hostname = Cc["@mozilla.org/network/dns-service;1"]
            .getService(Ci.nsIDNSService).myHostName;
  this._sbVersion = Cc["@mozilla.org/xre/app-info;1"]
            .getService(Ci.nsIXULAppInfo).version;

  // in case the user wants to use their own freedb server
  this._freedbUrl = Cc["@mozilla.org/fuel/application;1"]
                       .getService(Ci.fuelIApplication).prefs.getValue(
                        "metadata.lookup.freedb.url", FREEDB_URL);

  Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService)
    .addObserver(this, "songbird-library-manager-before-shutdown", false);
}

sbFreeDB.prototype = {
  classDescription : 'Songbird FreeDB Metadata Lookup Service',
  classID : Components.ID('c435acec-1dd1-11b2-88a4-b869b7fd74e2'),
  contractID : '@songbirdnest.com/Songbird/MetadataLookup/freedb;1',
  QueryInterface : XPCOMUtils.generateQI([Ci.sbIMetadataLookupProvider]),

  name : "FreeDB",
  weight : 1, // set weight to 1 so it can be overridden by Gracenote
  description : "Free GPL'd database for looking up and submitting CD information.",
  infoURL : "http://www.freedb.org",
  detailLookupsNeeded : true,

  // private functions
  _makeCmd: function sbFreeDB_makeCmd(cddbCmd) {
    var postdata = new Object;
    postdata["cmd"] = "cddb " + cddbCmd;
    postdata["hello"] = "user " + this._hostname +
                        " Songbird " + this._sbVersion;
    postdata["proto"] = "6";
    return postdata;
  },

  observe: function(subject, topic, data) {
    if (topic == "songbird-library-manager-before-shutdown") {
      Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService)
        .removeObserver(this, "songbird-library-manager-before-shutdown");

      // no leaky!
      this._sbVersion = null;
      this._hostname = null;
      this._freedbUrl = null;
    }
  },

  calculateIdFromDisc : function sbFreeDB_calculateIdFromDisc(aTOC) {
    var xxChecksum = 0;
    var yyyyLength = 0;
    var zzTracks;

    for each (var track in ArrayConverter.JSArray(aTOC.tracks)) {
      // QI it to an sbICDTOCEntry in case we're testing with JS-y things
      track.QueryInterface(Ci.sbICDTOCEntry);

      // the yyyy value is just the total length in seconds of every track
      yyyyLength += track.length;

      // calculate the xx checksum
      // take the track offset and divide by 75 to get the offset in seconds
      var timeOffset = parseInt(track.frameOffset / 75);
      // sum up the digits in the time offset
      var digitSum = 0;
      while (timeOffset > 0) {
        digitSum += timeOffset % 10;
        timeOffset = parseInt(timeOffset / 10);
      }
      // add that to the xx checksum
      xxChecksum += digitSum;
    }

    // take the checksum modulo 255
    xxChecksum  = parseInt(xxChecksum) % 255;

    // zz is the # of tracks on the CD
    zzTracks = aTOC.tracks.length;

    // convert the xx & yyyy to hex strings
    // zero-pad each string
    return String("0" + xxChecksum.toString(16)).slice(-2) +
           String("000" + yyyyLength.toString(16)).slice(-4) +
           String("0" + zzTracks.toString(16)).slice(-2);
  },

  queryDisc : function sbFreeDB_queryDisc(aTOC) {
    var discId = this.calculateIdFromDisc(aTOC);

    // construct the query command
    var cmd = "query " + discId + " " + aTOC.tracks.length + " ";
    var totalLength = 0;
    for each (var track in ArrayConverter.JSArray(aTOC.tracks)) {
      track.QueryInterface(Ci.sbICDTOCEntry);
      totalLength += track.length;
      cmd += track.frameOffset + " ";
    }
    cmd += totalLength;

    // create our new job and initialise it
    var mlJob = Cc["@songbirdnest.com/Songbird/MetadataLookup/job;1"]
      .createInstance(Ci.sbIMetadataLookupJob);
    mlJob.init(Ci.sbIMetadataLookupJob.JOB_DISC_LOOKUP,
               Ci.sbIJobProgress.STATUS_RUNNING);

    var self = this;
    POST(this._freedbUrl, this._makeCmd(cmd),
      function sbFreeDB_querySuccess(xhr) {
        var lines = xhr.responseText.split("\n");
        var headerline = lines.shift();
        var statusCode = headerline.match("^[0-9]+");

        if (statusCode == 202) {
          // 202 == no matches found
          // we treat this as success, but with zero results found
          mlJob.changeStatus(Ci.sbIJobProgress.STATUS_SUCCEEDED);
          return;
        } else if (statusCode == 403 || statusCode == 409) {
          // 403 == database entry corrupt
          // 409 == no handshake
          // we treat these are irrecoverable errors
          mlJob.changeStatus(Ci.sbIJobProgress.STATUS_FAILED);
          return;
        }

        // we've got some matches!
        if (statusCode == 200) {
          // exact single matches have the status code in the same line, e.g.:
          // 200 misc a00c9e0c U2 / All That You Can't Leave Behind
          lines[0] = headerline.replace(/^200 /, "");
        }

        // multiple matches have the statuscode in the first line, and all
        // results in the subsequent lines.  since we shifted off the status
        // above, we can just iterate now
        var singleAlbum;
        for each (var line in lines) {
          var matches = line.match("([a-zA-Z0-9]+) ([a-z0-9]+) (.*)");
          if (matches) {
            var genre = matches[1];
            var discid = matches[2];
            var title = matches[3];

            // create the skeleton sbIMetadataAlbumDetail object
            var a = new Object;
            a.QueryInterface = XPCOMUtils.generateQI(
                [Ci.sbIMetadataAlbumDetail]);
            a.tracks = null;
            a.properties =
              Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
                .createInstance(Ci.sbIMutablePropertyArray);
            a.properties.appendProperty(SBProperties.genre, genre);
            a.properties.appendProperty(FREEDB_NS + "discId", discid);
            a.properties.appendProperty(FREEDB_NS + "discTitle", title);
            var matches = title.match("(.*?) / (.*)");
            if (matches) {
              try {
                a.properties.getPropertyValue(SBProperties.artistName);
              } catch (e) {
                a.properties.appendProperty(SBProperties.artistName,
                                            matches[1]);
              }
              try {
                a.properties.getPropertyValue(SBProperties.albumName);
              } catch (e) {
                a.properties.appendProperty(SBProperties.albumName,
                                            matches[2]);
              }
            }

            singleAlbum = a;
            mlJob.appendResult(a);
          }
        }

        if (mlJob.mlNumResults == 1) {
          // If we only got one result, then we won't be prompting the user to
          // pick an album, so go ahead and grab the detailed info for this
          // album since we'll need it to populate the cd rip view entries
          //
          // _getAlbumDetail() will set status on the job and notify listeners
          // appropriately
          self._getAlbumDetail(singleAlbum, mlJob);
        } else {
          // otherwise, just send notification that this job is done
          // the caller will need to do followup getAlbumDetail() calls
          mlJob.changeStatus(Ci.sbIJobProgress.STATUS_SUCCEEDED);
        }
      },
      function sbFreeDB_queryFailure(xhr) {
        mlJob.changeStatus(Ci.sbIJobProgress.STATUS_FAILED);
      });

    return mlJob;
  },

  _getAlbumDetail : function sbFreeDB_getAlbumDetail(aAlbum, mlJob) {
    var cmd = "read " + aAlbum.properties.getPropertyValue(SBProperties.genre) +
              " " + aAlbum.properties.getPropertyValue(FREEDB_NS + "discId");
    POST(this._freedbUrl, this._makeCmd(cmd),
      function sbFreeDB_readSuccess(xhr) {
        var lines = xhr.responseText.split("\n");
        var headerline = lines.shift();
        var statusCode = headerline.match("^[0-9]+");
        if (statusCode != 210) {
          // error
          mlJob.changeStatus(Ci.sbIJobProgress.STATUS_FAILED);
          return;
        }

        aAlbum.tracks = Cc["@mozilla.org/array;1"]
                          .createInstance(Ci.nsIMutableArray);
        for each (var line in lines) {
          if (line[0] == "#") // comment
            continue;
          var matches = line.match("([A-Z0-9]+)=(.*)");
          if (matches) {
            var key = matches[1];
            var value = matches[2];
            // if we have a valid value, and it's a key we've got pre-mapped
            if (value && KEYMAP[key]) {
              try {
                aAlbum.properties.getPropertyValue(KEYMAP[key]);
                // if we're here, then the property already exists and
                // we need to replace it
                // this is janky
                for (var i=0; i<aAlbum.properties.length; i++) {
                  var prop = aAlbum.properties.getPropertyAt(i);
                  if (prop.id == KEYMAP[key]) {
                    // found the matching property
                    aAlbum.properties.QueryInterface(Ci.nsIMutableArray);
                    // remove it, and then append the new one
                    aAlbum.properties.removeElementAt(i);
                    aAlbum.properties.appendProperty(KEYMAP[key], value);
                  }
                }
              } catch (e) {
                // the property didn't exist, so just append it
                aAlbum.properties.appendProperty(KEYMAP[key], value);
              }
            } else {
              if (key == "DTITLE") {
                // disc title is in the form: ARTIST / TITLE
                var matches = value.match("(.*?) / (.*)");
                if (matches) {
                  try {
                    aAlbum.properties.getPropertyValue(SBProperties.artistName);
                  } catch (e) {
                    aAlbum.properties.appendProperty(SBProperties.artistName,
                                                     matches[1]);
                  }
                  try {
                    aAlbum.properties.getPropertyValue(SBProperties.albumName);
                  } catch (e) {
                    aAlbum.properties.appendProperty(SBProperties.albumName,
                                                     matches[2]);
                  }
                }
              } else {
                var matches = key.match("^TTITLE([0-9]+)");
                if (matches) {
                  // track title is indexed from 0
                  var trackNum = parseInt(matches[1]) + 1;
                  try {
                    var album = aAlbum.properties.getPropertyValue(
                          SBProperties.albumName);
                    var genre = aAlbum.properties.getPropertyValue(
                          SBProperties.genre);
                    var year = aAlbum.properties.getPropertyValue(
                          SBProperties.year);
                  } catch (e) {
                  }
                  var title;
                  var artist;

                  var matches = value.match("(.*?) / (.*)");
                  if (matches) {
                    artist = matches[1];
                    title = matches[2];
                  } else {
                    title = value;
                    artist = aAlbum.properties.getPropertyValue(
                        SBProperties.artistName);
                  }

                  var track = Cc[
          "@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
          .createInstance(Ci.sbIMutablePropertyArray);
                  track.appendProperty(SBProperties.artistName, artist);
                  track.appendProperty(SBProperties.trackName, title);
                  track.appendProperty(SBProperties.albumName, album);
                  track.appendProperty(SBProperties.genre, genre);
                  if (year)
                    track.appendProperty(SBProperties.year, year);
                  track.appendProperty(SBProperties.trackNumber,
                                       trackNum.toString());
                  aAlbum.tracks.appendElement(track, false);
                }
              }
            }
          }
        }

        mlJob.changeStatus(Ci.sbIJobProgress.STATUS_SUCCEEDED);
      },
      function sbFreeDB_readFailure(xhr) {
        mlJob.changeStatus(Ci.sbIJobProgress.STATUS_FAILED);
      });

    return mlJob;
  },

  getAlbumDetail : function sbFreeDB_getAlbumDetail(aAlbum) {
    // create the new job
    var mlJob = Cc["@songbirdnest.com/Songbird/MetadataLookup/job;1"]
      .createInstance(Ci.sbIMetadataLookupJob);

    // if we already have album detail for this album, then just return it
    if (aAlbum.tracks) {
      // initialise the job and append our album to it
      mlJob.init(Ci.sbIMetadataLookupJob.JOB_ALBUM_DETAIL_LOOKUP,
                 Ci.sbIJobProgress.STATUS_SUCCEEDED);
      mlJob.appendResult(aAlbum);
      return mlJob;
    }

    // initialise the job and append our album to it
    mlJob.init(Ci.sbIMetadataLookupJob.JOB_ALBUM_DETAIL_LOOKUP,
               Ci.sbIJobProgress.STATUS_RUNNING);
    mlJob.appendResult(aAlbum);

    return this._getAlbumDetail(aAlbum, mlJob);
  },
}

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbFreeDB],
      // our post-register function to register ourselves with
      // the category manager
      function(aCompMgr, aFileSpec, aLocation) {
        XPCOMUtils.categoryManager.addCategoryEntry(
          "metadata-lookup",
          sbFreeDB.name,
          sbFreeDB.prototype.contractID,
          true,
          true);
      }
    );
}
