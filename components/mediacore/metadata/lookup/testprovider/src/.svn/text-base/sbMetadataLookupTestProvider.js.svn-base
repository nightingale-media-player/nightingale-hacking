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

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/SBJobUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");

function sbTestProvider() {
}

var midnightRock = [
  {artist: "Hootie & The Blowfish", title: "Let Her Cry"},
  {artist: "Genesis", title: "Throwing It All Away"},
  {artist: "Pretenders", title: "I'll Stand By You"},
  {artist: "Jesus Jones", title: "Right Here, Right Now"},
  {artist: "Bad Company", title: "Feel Like Makin' Love"},
  {artist: "Alannah Myles", title: "Black Velvet"},
  {artist: "Foreigner", title: "I Want To Know What Love Is"},
  {artist: "Boston", title: "Amanda"},
  {artist: "Meat Loaf", title: "I'd Do Anything For Love (But I Won't Do That"},
  {artist: "Pat Benatar", title: "We Belong"},
  {artist: "April Wine", title: "Just Between You And Me"},
  {artist: "INXS", title: "Never Tear Us Apart"},
  {artist: "Lita Ford", title: "Shot Of Poison"},
  {artist: "Nazareth", title: "Love Hurts"},
  {artist: "Skid Row", title: "I Remember You"}
  ];

var incredibad = [
  {artist: "The Lonely Island", title: "Who Said We're Wack?"},
  {artist: "The Lonely Island", title: "Santana DVX (feat. E-40)"},
  {artist: "The Lonely Island", title: "Jizz In My Pants"},
  {artist: "The Lonely Island", title: "I'm On A Boat (feat. T-Pain)"},
  {artist: "The Lonely Island", title: "Sax Man (feat. Jack Black)"},
  {artist: "The Lonely Island", title: "Lazy Sunday (feat. Chris Parnell)"},
  {artist: "The Lonely Island", title: "Normal Guy - Interlude"},
  {artist: "The Lonely Island", title: "Boombox (feat. Julian Casablancas)"},
  {artist: "The Lonely Island", title: "Shrooms - Interlude"},
  {artist: "The Lonely Island", title: "Like A Boss"},
  {artist: "The Lonely Island", title: "We Like Sports"},
  {artist: "The Lonely Island", title: "Dreamgirl (feat. Norah Jones)"},
  {artist: "The Lonely Island", title: "Ras Trent"},
  {artist: "The Lonely Island", title: "Dick In A Box (feat. Justin Timberlake)"},
  {artist: "The Lonely Island", title: "The Old Saloon - Interlude"},
  {artist: "The Lonely Island", title: "Punch You In The Jeans"},
  {artist: "The Lonely Island", title: "Space Olympics"},
  {artist: "The Lonely Island", title: "Natalie's Rap (feat. Natalie Portman & Chris Parnell)"},
  {artist: "The Lonely Island", title: "Incredibad"},
  ];

var notIncredibad = [
  {artist: "Not The Lonely Island", title: "Fake Track 1"},
  {artist: "Not The Lonely Island", title: "Fake Track 2"},
  {artist: "Not The Lonely Island", title: "Fake Track 3"},
  {artist: "Not The Lonely Island", title: "Fake Track 4"},
  {artist: "Not The Lonely Island", title: "Fake Track 5"},
  {artist: "Not The Lonely Island", title: "Fake Track 6"},
  {artist: "Not The Lonely Island", title: "Fake Track 7"},
  {artist: "Not The Lonely Island", title: "Fake Track 8"},
  {artist: "Not The Lonely Island", title: "Fake Track 9"},
  {artist: "Not The Lonely Island", title: "Fake Track 10"},
  {artist: "Not The Lonely Island", title: "Fake Track 11"},
  {artist: "Not The Lonely Island", title: "Fake Track 12"},
  {artist: "Not The Lonely Island", title: "Fake Track 13"},
  {artist: "Not The Lonely Island", title: "Fake Track 14"},
  {artist: "Not The Lonely Island", title: "Fake Track 15"},
  {artist: "Not The Lonely Island", title: "Fake Track 16"},
  {artist: "Not The Lonely Island", title: "Fake Track 17"},
  {artist: "Not The Lonely Island", title: "Fake Track 18"},
  {artist: "Not The Lonely Island", title: "Fake Track 19"},
  ];

var instantJunk = [
  {artist: "Songbird", title: "No Data"},
  ];

sbTestProvider.prototype = {
  classDescription : 'Songbird Test Metadata Lookup Service',
  classID : Components.ID('9e599632-1dd1-11b2-ab82-e0952e7285ce'),
  contractID : '@songbirdnest.com/Songbird/MetadataLookup/testProvider;1',
  QueryInterface : XPCOMUtils.generateQI([Ci.sbIMetadataLookupProvider,
                                          Ci.nsITimerCallback]),

  name : "TestProvider",
  weight : 9999, // set weight to 1 so it can be overridden by Gracenote
  description : "Test provider.  Unless you like Britney, U2, or Midnight Rock, you probably don't want this.",
  infoURL : "http://getsongbird.com",

  identifyTOC : function sbTestProvider_identifyTOC(aTOC) {
    if (aTOC.firstTrackIndex == 1 && aTOC.lastTrackIndex == 15 &&
        aTOC.leadOutTrackOffset == 285675)
      return Ci.sbIMockCDDeviceController.MOCK_MEDIA_DISC_MIDNIGHT_ROCK;
    else if (aTOC.firstTrackIndex == 1 && aTOC.lastTrackIndex == 12 &&
        aTOC.leadOutTrackOffset == 260335)
      return Ci.sbIMockCDDeviceController.MOCK_MEDIA_DISC_BABY_ONE_MORE_TIME;
    else if (aTOC.firstTrackIndex == 1 && aTOC.lastTrackIndex == 11 &&
        aTOC.leadOutTrackOffset == 225562)
      return Ci.sbIMockCDDeviceController.MOCK_MEDIA_DISC_U2;
    else if (aTOC.firstTrackIndex == 1 && aTOC.lastTrackIndex == 19 &&
        aTOC.leadOutTrackOffset == 190565)
      return Ci.sbIMockCDDeviceController.MOCK_MEDIA_DISC_INCREDIBAD;
    else if (aTOC.firstTrackIndex == 1 && aTOC.lastTrackIndex == 1 &&
        aTOC.leadOutTrackOffset == 131072)
      return Ci.sbIMockCDDeviceController.MOCK_MEDIA_DISC_INSTANT_JUNK;
    else
      throw Components.results.NS_ERROR_UNEXPECTED;
  },

  
  queryDisc : function sbTestProvider_queryDisc(aTOC) {
    var id = this.identifyTOC(aTOC);
    this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    
    var job = Cc["@songbirdnest.com/Songbird/MetadataLookup/job;1"]
                .createInstance(Ci.sbIMetadataLookupJob);
    job.init(Ci.sbIMetadataLookupJob.JOB_DISC_LOOKUP,
             Ci.sbIJobProgress.STATUS_RUNNING);
    this.job = job;

    this.whichAlbum = id;

    switch (id) {
      case Ci.sbIMockCDDeviceController.MOCK_MEDIA_DISC_MIDNIGHT_ROCK:
        // Test case 1: Midnight Rock
        // Return a TOC after 5 seconds
        this._timer.initWithCallback(this, 5000,
                                     Ci.nsITimerCallback.TYPE_ONE_SHOT);
        break;
      case Ci.sbIMockCDDeviceController.MOCK_MEDIA_DISC_BABY_ONE_MORE_TIME:
        // Test case 2: Hit Me Baby One More Time
        // Return no TOC found after 5 seconds
        this._timer.initWithCallback(this, 5000,
                                     Ci.nsITimerCallback.TYPE_ONE_SHOT);
        break;
      case Ci.sbIMockCDDeviceController.MOCK_MEDIA_DISC_U2:
        // Test case 3: U2
        // Timeout after 3 minutes
        this._timer.initWithCallback(this, 180000,
                                     Ci.nsITimerCallback.TYPE_ONE_SHOT);
        break;
      case Ci.sbIMockCDDeviceController.MOCK_MEDIA_DISC_INCREDIBAD:
        // Test case 4: Incredibad
        // Return multiple TOCs found after 5 seconds
        this._timer.initWithCallback(this, 5000,
                                     Ci.nsITimerCallback.TYPE_ONE_SHOT);
        break;
      case Ci.sbIMockCDDeviceController.MOCK_MEDIA_DISC_INSTANT_JUNK:
        // Test case 5: Instant Junk
        // Return multiple TOCs found immediately
        this.notify(this._timer);
        break;
      default:
        dump("This is not a recognised disc.\n");
        break;
    }

    return job;
  },

  getAlbumDetail: function(album) {
    var job = Cc["@songbirdnest.com/Songbird/MetadataLookup/job;1"]
                  .createInstance(Ci.sbIMetadataLookupJob);
    job.init(Ci.sbIMetadataLookupJob.JOB_ALBUM_DETAIL_LOOKUP,
             Ci.sbIJobProgress.STATUS_RUNNING);

    job.appendResult(album);
    job.changeStatus(Ci.sbIJobProgress.STATUS_SUCCEEDED);

    return job;
  },

  makeAlbum: function(albumToc, artistName, albumName, genre) {
    // create the skeleton sbIMetadataAlbumDetail object
    var a = new Object;
    a.QueryInterface = XPCOMUtils.generateQI([Ci.sbIMetadataAlbumDetail]);
    a.tracks = null;
    a.properties =
        Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
           .createInstance(Ci.sbIMutablePropertyArray);

    a.properties.appendProperty(SBProperties.genre, genre);
    a.properties.appendProperty(SBProperties.artistName, artistName);
    a.properties.appendProperty(SBProperties.albumName, albumName);

    a.tracks = Cc["@mozilla.org/array;1"].createInstance(Ci.nsIMutableArray);
    for (var i = 0; i < albumToc.length; i++) {
      var track = Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
                    .createInstance(Ci.sbIMutablePropertyArray);
      var trackInfo = albumToc[i];
      track.appendProperty(SBProperties.albumArtistName, artistName);
      track.appendProperty(SBProperties.artistName, trackInfo.artist);
      track.appendProperty(SBProperties.trackName, trackInfo.title);
      track.appendProperty(SBProperties.albumName, albumName);
      track.appendProperty(SBProperties.genre, genre);
      track.appendProperty(SBProperties.trackNumber, (i+1).toString());
      a.tracks.appendElement(track, false);
    }

    return a;
  },

  notify: function (aTimer) {
    if (aTimer != this._timer)
      return;

    this._timer = null;
    var id = this.whichAlbum;
    if (id == Ci.sbIMockCDDeviceController.MOCK_MEDIA_DISC_BABY_ONE_MORE_TIME)
    {
      // do nothing, return empty results
      this.job.changeStatus(Ci.sbIJobProgress.STATUS_SUCCEEDED);
    } else if (id == Ci.sbIMockCDDeviceController.MOCK_MEDIA_DISC_MIDNIGHT_ROCK)
    {
      var a = this.makeAlbum(midnightRock, "Various", "Midnight Rock Disc 1",
                             "Rock");

      // append this result & declare success
      this.job.appendResult(a);
      this.job.changeStatus(Ci.sbIJobProgress.STATUS_SUCCEEDED);
    } else if (id == Ci.sbIMockCDDeviceController.MOCK_MEDIA_DISC_INCREDIBAD)
    {
      var a = this.makeAlbum(incredibad, "The Lonely Island", "Incredibad",
                             "Comedy");
      var b = this.makeAlbum(notIncredibad, "Not The Lonely Island",
                             "Fake Incredibad", "Gospel");
      this.job.appendResult(a);
      this.job.appendResult(b);
      this.job.changeStatus(Ci.sbIJobProgress.STATUS_SUCCEEDED);
    } else if (id == Ci.sbIMockCDDeviceController.MOCK_MEDIA_DISC_INSTANT_JUNK)
    {
      // return a dummy disc immediately
      var a = this.makeAlbum(instantJunk, "Songbird", "Instant Junk",
                             "Soundtrack");

      // append this result & declare success
      this.job.appendResult(a);
      this.job.changeStatus(Ci.sbIJobProgress.STATUS_SUCCEEDED);
    } else {
      this.job.changeStatus(Ci.sbIJobProgress.STATUS_FAILED);
    }
    this.job = null;
  },
}

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbTestProvider],
      // our post-register function to register ourselves with
      // the category manager
      function(aCompMgr, aFileSpec, aLocation) {
        XPCOMUtils.categoryManager.addCategoryEntry(
          "metadata-lookup",
          sbTestProvider.name,
          sbTestProvider.prototype.contractID,
          true,
          true);
      }
    );
}
