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

#ifndef __SB_STANDARD_PROPERTIES_H__
#define __SB_STANDARD_PROPERTIES_H__

#define SB_PROPERTY_STORAGEGUID               "http://songbirdnest.com/data/1.0#storageGUID"
#define SB_PROPERTY_CREATED                   "http://songbirdnest.com/data/1.0#created"
#define SB_PROPERTY_UPDATED                   "http://songbirdnest.com/data/1.0#updated"
#define SB_PROPERTY_CONTENTURL                "http://songbirdnest.com/data/1.0#contentURL"
#define SB_PROPERTY_CONTENTTYPE               "http://songbirdnest.com/data/1.0#contentType"
#define SB_PROPERTY_CONTENTLENGTH             "http://songbirdnest.com/data/1.0#contentLength"
#define SB_PROPERTY_HASH                      "http://songbirdnest.com/data/1.0#hash"
#define SB_PROPERTY_TRACKNAME                 "http://songbirdnest.com/data/1.0#trackName"
#define SB_PROPERTY_ALBUMNAME                 "http://songbirdnest.com/data/1.0#albumName"
#define SB_PROPERTY_ARTISTNAME                "http://songbirdnest.com/data/1.0#artistName"
#define SB_PROPERTY_DURATION                  "http://songbirdnest.com/data/1.0#duration"
#define SB_PROPERTY_GENRE                     "http://songbirdnest.com/data/1.0#genre"
#define SB_PROPERTY_TRACKNUMBER               "http://songbirdnest.com/data/1.0#trackNumber"
#define SB_PROPERTY_YEAR                      "http://songbirdnest.com/data/1.0#year"
#define SB_PROPERTY_DISCNUMBER                "http://songbirdnest.com/data/1.0#discNumber"
#define SB_PROPERTY_TOTALDISCS                "http://songbirdnest.com/data/1.0#totalDiscs"
#define SB_PROPERTY_TOTALTRACKS               "http://songbirdnest.com/data/1.0#totalTracks"
#define SB_PROPERTY_ISPARTOFCOMPILATION       "http://songbirdnest.com/data/1.0#isPartOfCompilation"
#define SB_PROPERTY_PRODUCERNAME              "http://songbirdnest.com/data/1.0#producerName"
#define SB_PROPERTY_COMPOSERNAME              "http://songbirdnest.com/data/1.0#composerName"
#define SB_PROPERTY_CONDUCTORNAME             "http://songbirdnest.com/data/1.0#conductorName"
#define SB_PROPERTY_LYRICISTNAME              "http://songbirdnest.com/data/1.0#lyricistName"
#define SB_PROPERTY_LYRICS                    "http://songbirdnest.com/data/1.0#lyrics"
#define SB_PROPERTY_RECORDLABELNAME           "http://songbirdnest.com/data/1.0#recordLabelName"
#define SB_PROPERTY_PRIMARYIMAGEURL           "http://songbirdnest.com/data/1.0#primaryImageURL"
#define SB_PROPERTY_LASTPLAYTIME              "http://songbirdnest.com/data/1.0#lastPlayTime"
#define SB_PROPERTY_PLAYCOUNT                 "http://songbirdnest.com/data/1.0#playCount"
#define SB_PROPERTY_LASTSKIPTIME              "http://songbirdnest.com/data/1.0#lastSkipTime"
#define SB_PROPERTY_SKIPCOUNT                 "http://songbirdnest.com/data/1.0#skipCount"
#define SB_PROPERTY_RATING                    "http://songbirdnest.com/data/1.0#rating"
#define SB_PROPERTY_BITRATE                   "http://songbirdnest.com/data/1.0#bitRate"
#define SB_PROPERTY_CHANNELS                  "http://songbirdnest.com/data/1.0#channels"
#define SB_PROPERTY_SAMPLERATE                "http://songbirdnest.com/data/1.0#sampleRate"
#define SB_PROPERTY_BPM                       "http://songbirdnest.com/data/1.0#bpm"
#define SB_PROPERTY_KEY                       "http://songbirdnest.com/data/1.0#key"
#define SB_PROPERTY_LANGUAGE                  "http://songbirdnest.com/data/1.0#language"
#define SB_PROPERTY_COMMENT                   "http://songbirdnest.com/data/1.0#comment"
#define SB_PROPERTY_COPYRIGHT                 "http://songbirdnest.com/data/1.0#copyright"
#define SB_PROPERTY_COPYRIGHTURL              "http://songbirdnest.com/data/1.0#copyrightURL"
#define SB_PROPERTY_SUBTITLE                  "http://songbirdnest.com/data/1.0#subtitle"
#define SB_PROPERTY_METADATAUUID              "http://songbirdnest.com/data/1.0#metadataUUID"
#define SB_PROPERTY_SOFTWAREVENDOR            "http://songbirdnest.com/data/1.0#softwareVendor"
#define SB_PROPERTY_ORIGINURL                 "http://songbirdnest.com/data/1.0#originURL"
#define SB_PROPERTY_ORIGINPAGE                "http://songbirdnest.com/data/1.0#originPage"
#define SB_PROPERTY_ORIGINPAGEIMAGE           "http://songbirdnest.com/data/1.0#originPageImage"
#define SB_PROPERTY_ORIGINPAGETITLE           "http://songbirdnest.com/data/1.0#originPageTitle"
/* for items copied from other libraries, the library it came from */
#define SB_PROPERTY_ORIGINLIBRARYGUID         "http://songbirdnest.com/data/1.0#originLibraryGuid"
/* for items copied from other libraries, the original item's guid */
#define SB_PROPERTY_ORIGINITEMGUID            "http://songbirdnest.com/data/1.0#originItemGuid"
#define SB_PROPERTY_GUID                      "http://songbirdnest.com/data/1.0#GUID"
#define SB_PROPERTY_HIDDEN                    "http://songbirdnest.com/data/1.0#hidden"
#define SB_PROPERTY_ISLIST                    "http://songbirdnest.com/data/1.0#isList"
#define SB_PROPERTY_LISTTYPE                  "http://songbirdnest.com/data/1.0#listType"
#define SB_PROPERTY_ISREADONLY                "http://songbirdnest.com/data/1.0#isReadOnly"
#define SB_PROPERTY_ISCONTENTREADONLY         "http://songbirdnest.com/data/1.0#isContentReadOnly"
#define SB_PROPERTY_ORDINAL                   "http://songbirdnest.com/data/1.0#ordinal"
#define SB_PROPERTY_MEDIALISTNAME             "http://songbirdnest.com/data/1.0#mediaListName"
#define SB_PROPERTY_COLUMNSPEC                "http://songbirdnest.com/data/1.0#columnSpec"
#define SB_PROPERTY_DEFAULTCOLUMNSPEC         "http://songbirdnest.com/data/1.0#defaultColumnSpec"
#define SB_PROPERTY_CUSTOMTYPE                "http://songbirdnest.com/data/1.0#customType"
#define SB_PROPERTY_DESTINATION               "http://songbirdnest.com/data/1.0#destination"
#define SB_PROPERTY_DOWNLOADBUTTON            "http://songbirdnest.com/data/1.0#downloadButton"
#define SB_PROPERTY_DOWNLOAD_STATUS_TARGET    "http://songbirdnest.com/data/1.0#downloadStatusTarget"
#define SB_PROPERTY_DOWNLOAD_DETAILS          "http://songbirdnest.com/data/1.0#downloadDetails"
#define SB_PROPERTY_ISSORTABLE                "http://songbirdnest.com/data/1.0#isSortable"
#define SB_PROPERTY_RAPISCOPEURL              "http://songbirdnest.com/data/1.0#rapiScopeURL"
#define SB_PROPERTY_RAPISITEID                "http://songbirdnest.com/data/1.0#rapiSiteID"
#define SB_PROPERTY_ENABLE_AUTO_DOWNLOAD      "http://songbirdnest.com/data/1.0#enableAutoDownload"
#define SB_PROPERTY_TRANSFER_POLICY           "http://songbirdnest.com/data/1.0#transferPolicy"
#define SB_PROPERTY_DEFAULT_MEDIAPAGE_URL     "http://songbirdnest.com/data/1.0#defaultMediaPageURL"
#define SB_PROPERTY_ONLY_CUSTOM_MEDIAPAGES    "http://songbirdnest.com/data/1.0#onlyCustomMediaPages"
#define SB_PROPERTY_AVAILABILITY              "http://songbirdnest.com/data/1.0#availability"
#define SB_PROPERTY_ALBUMARTISTNAME           "http://songbirdnest.com/data/1.0#albumArtistName"
#define SB_PROPERTY_OUTERGUID                 "http://songbirdnest.com/data/1.0#outerGUID"
#define SB_PROPERTY_ALBUMDETAIL               "http://songbirdnest.com/data/1.0#albumDetailImage"
#define SB_PROPERTY_ARTISTDETAIL              "http://songbirdnest.com/data/1.0#artistDetailImage"
#define SB_PROPERTY_ALBUMDETAILURL            "http://songbirdnest.com/data/1.0#albumDetailUrl"
#define SB_PROPERTY_ARTISTDETAILURL           "http://songbirdnest.com/data/1.0#artistDetailUrl"
#define SB_PROPERTY_EXCLUDE_FROM_HISTORY      "http://songbirdnest.com/data/1.0#excludeFromHistory"
#define SB_PROPERTY_DISABLE_DOWNLOAD          "http://songbirdnest.com/data/1.0#disableDownload"
#define SB_PROPERTY_ISSUBSCRIPTION            "http://songbirdnest.com/data/1.0#isSubscription"
#define SB_PROPERTY_CDRIP_STATUS              "http://songbirdnest.com/data/1.0#cdRipStatus"
#define SB_PROPERTY_CDDISCHASH                "http://songbirdnest.com/data/1.0#cdDiscHash"
#define SB_PROPERTY_SHOULDRIP                 "http://songbirdnest.com/data/1.0#shouldRip"
/* boolean: true if the media is DRM protected; false/empty otherwise */
#define SB_PROPERTY_ISDRMPROTECTED            "http://songbirdnest.com/data/1.0#isDRMProtected"
#define SB_PROPERTY_ISALBUM                   "http://songbirdnest.com/data/1.0#isAlbum"

// Device library specific properties
#define SB_PROPERTY_DEVICE_PERSISTENT_ID      "http://songbirdnest.com/data/1.0#deviceId"
#define SB_PROPERTY_LAST_SYNC_PLAYCOUNT       "http://songbirdnest.com/data/1.0#playCount_AtLastSync"
#define SB_PROPERTY_LAST_SYNC_SKIPCOUNT       "http://songbirdnest.com/data/1.0#skipCount_AtLastSync"

// Smart media list specific properties
#define SB_PROPERTY_SMARTMEDIALIST_STATE      "http://songbirdnest.com/data/1.0#smartMediaListState"

// Main library specific properties
#define SB_PROPERTY_CREATED_FIRSTRUN_SMARTPLAYLISTS "http://songbirdnest.com/data/1.0#createdFirstRunSmartPlaylists"
#define SB_PROPERTY_DOWNLOAD_MEDIALIST_GUID   "http://songbirdnest.com/data/1.0#downloadMediaListGUID"

// GUID of the device library. Present if the library is the one created for a
// device library
#define SB_PROPERTY_DEVICE_LIBRARY_GUID       "http://songbirdnest.com/data/1.0#deviceLibraryGuid"

// iTunes Import/export related properties
#define SB_PROPERTY_ITUNES_GUID "http://songbirdnest.com/data/1.0#iTunesGUID"

#endif /* __SB_STANDARD_PROPERTIES_H__ */
