/* vim: set sw=2 :*/
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

/**
 * test the localizationKey attribute on sbIPropertyInfo on registered properties
 */

function runTest () {

  // list of property localization keys found in SBProperties.jsm
  var localizationKeys = {
    ordinal: 'ordinal',
    created: 'date_created',
    updated: 'date_updated',
    contentURL: 'content_url',
    contentType: 'content_type',
    contentLength: 'content_length',
    hash: 'content_hash',
    trackName: 'track_name',
    albumName: 'album_name',
    artistName: 'artist_name',
    trackType: 'track_type',
    duration: 'duration',
    genre: 'genre',
    year: 'year',
    trackNumber: 'track_no',
    discNumber: 'disc_no',
    totalDiscs: 'total_discs',
    totalTracks: 'total_tracks',
    isPartOfCompilation: 'is_part_of_compilation',
    producerName: 'producer',
    composerName: 'composer',
    conductorName: 'conductor',
    lyricistName: 'lyricist',
    lyrics: 'lyrics',
    recordLabelName: 'record_label_name',
    primaryImageURL: 'primary_image_url',
    lastPlayTime: 'last_play_time',
    playCount: 'play_count',
    lastSkipTime: 'last_skip_time',
    skipCount: 'skip_count',
    bitRate: 'bitrate',
    sampleRate: 'samplerate',
    channels: 'channels',
    bpm: 'bpm',
    key: 'key',
    language: 'language',
    comment: 'comment',
    copyright: 'copyright',
    copyrightURL: 'copyright_url',
    subtitle: 'subtitle',
    metadataUUID: 'metadata_uuid',
    softwareVendor: 'vendor',
    originURL: 'origin_url',
    destination: 'destination',
    hidden: 'hidden',
    columnSpec: 'column_spec',
    defaultColumnSpec: 'default_column_spec',
    originPage: 'origin_page',
    artistDetailUrl: 'artist_detail_url',
    albumDetailUrl: 'album_detail_url',
    originPageTitle: 'origin_pagetitle',
    downloadButton: 'download_button',
    downloadDetails: 'download_details',
    rating: 'rating',
    originPageImage: 'origin_page_image',
    artistDetailImage: 'artist_detail_image',
    albumDetailImage: 'album_detail_image',
    rapiScopeURL: 'rapi_scope_url',
    rapiSiteID: 'rapi_site_id',
    mediaListName: 'media_list_name',
    defaultMediaPageURL: 'default_mediapage_url',
    availability: 'availability',
    albumArtistName: 'albumartistname',
    cdRipStatus: 'cdrip_status',
    shouldRip: 'shouldrip',
    keywords: 'keywords',
    description: 'description',
    showName: 'showName',
    episodeNumber: 'episodeNumber',
    seasonNumber: 'seasonNumber',
    playlistURL: 'playlist_url'
  };

  // list of properties with localization keys, but not in SBProperties.jsm
  var specialLocalizationKeys = {
    "http://songbirdnest.com/dummy/smartmedialists/1.0#playlist":
      "property.dummy.playlist",
  };

  var propMgr =
    Components.classes["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
              .getService(Components.interfaces.sbIPropertyManager);
  var info;

  // test the properties found in SBProperties.jsm
  for (var prop in localizationKeys) {
    info = propMgr.getPropertyInfo(SBProperties[prop]);
    try {
      assertEqual("property." + localizationKeys[prop],
                  info.localizationKey,
                  "Property does not have expected localization info");
    } catch (e if e == Cr.NS_ERROR_ABORT) {
      // keep going; this will cause the run to fail anyway
    }
  }

  // test the properties not in SBProperties.jsm
  var propEnum = propMgr.propertyIDs;
  while (propEnum.hasMore()) {
    var propID = propEnum.getNext();
    var propName = propID.replace(SBProperties.base, "");
    if (propName in localizationKeys) {
      // found in SBProperties.jsm, see list above
      continue;
    }
    info = propMgr.getPropertyInfo(propID);
    try {
      if (propID in specialLocalizationKeys) {
        // this is a special non-standard property, yay.
        assertEqual(specialLocalizationKeys[propID],
                    info.localizationKey,
                    "Property does not have expected special localization info");
      } else {
        // property with no explicit localization key, should be default
        assertEqual(propID, info.localizationKey,
                    "Property has unknown custom localization info");
      }
    } catch (e if e == Cr.NS_ERROR_ABORT) {
      // keep going; this will cause the run to fail anyway
    }
  }
}
