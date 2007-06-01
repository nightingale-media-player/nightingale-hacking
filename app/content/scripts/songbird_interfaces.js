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

//
// Formal constructors to get interfaces - all creations in js should use these unless
//   getting the object as a service, then you should use the proper getService call.
// alpha-sorted!!
//

const sbIDatabaseQuery = new Components.Constructor("@songbirdnest.com/Songbird/DatabaseQuery;1", "sbIDatabaseQuery");
const sbIDynamicPlaylist = new Components.Constructor("@songbirdnest.com/Songbird/DynamicPlaylist;1", "sbIDynamicPlaylist");
// const sbIMediaLibrary = new Components.Constructor("@songbirdnest.com/Songbird/MediaLibrary;1", "sbIMediaLibrary");
const sbIMediaScan = new Components.Constructor("@songbirdnest.com/Songbird/MediaScan;1", "sbIMediaScan");
const sbIMediaScanQuery = new Components.Constructor("@songbirdnest.com/Songbird/MediaScanQuery;1", "sbIMediaScanQuery");
// const sbIPlaylistManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistManager;1", "sbIPlaylistManager");
const sbIPlaylist = new Components.Constructor("@songbirdnest.com/Songbird/Playlist;1", "sbIPlaylist");
const sbIPlaylistReaderListener = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistReaderListener;1", "sbIPlaylistReaderListener");
const sbIPlaylistReaderManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistReaderManager;1", "sbIPlaylistReaderManager");
const sbIPlaylistsource = new Components.Constructor("@mozilla.org/rdf/datasource;1?name=playlist", "sbIPlaylistsource");
const sbISimplePlaylist = new Components.Constructor("@songbirdnest.com/Songbird/SimplePlaylist;1", "sbISimplePlaylist");
const sbISmartPlaylist = new Components.Constructor("@songbirdnest.com/Songbird/SmartPlaylist;1", "sbISmartPlaylist");
const sbIServicesource = new Components.Constructor("@mozilla.org/rdf/datasource;1?name=Servicesource", "sbIServicesource");

/**
 * These properties should be kept in sync with sbStandardProperties.h for the
 * moment. Eventually we'll figure out a better way to do this.
 */

const SB_PROPERTY_PREFACE              = "http://songbirdnest.com/data/1.0#";

const SB_PROPERTY_STORAGEGUID          = SB_PROPERTY_PREFACE + "storageGuid";
const SB_PROPERTY_CREATED              = SB_PROPERTY_PREFACE + "created";
const SB_PROPERTY_UPDATED              = SB_PROPERTY_PREFACE + "updated";
const SB_PROPERTY_CONTENTURL           = SB_PROPERTY_PREFACE + "contentUrl";
const SB_PROPERTY_CONTENTMIMETYPE      = SB_PROPERTY_PREFACE + "contentMimeType";
const SB_PROPERTY_CONTENTLENGTH        = SB_PROPERTY_PREFACE + "contentLength";
const SB_PROPERTY_TRACKNAME            = SB_PROPERTY_PREFACE + "trackName";
const SB_PROPERTY_ALBUMNAME            = SB_PROPERTY_PREFACE + "albumName";
const SB_PROPERTY_ARTISTNAME           = SB_PROPERTY_PREFACE + "artistName";
const SB_PROPERTY_DURATION             = SB_PROPERTY_PREFACE + "duration";
const SB_PROPERTY_GENRE                = SB_PROPERTY_PREFACE + "genre";
const SB_PROPERTY_TRACK                = SB_PROPERTY_PREFACE + "track";
const SB_PROPERTY_YEAR                 = SB_PROPERTY_PREFACE + "year";
const SB_PROPERTY_DISCNUMBER           = SB_PROPERTY_PREFACE + "discNumber";
const SB_PROPERTY_TOTALDISCS           = SB_PROPERTY_PREFACE + "totalDiscs";
const SB_PROPERTY_TOTALTRACKS          = SB_PROPERTY_PREFACE + "totalTracks";
const SB_PROPERTY_ISPARTOFCOMPILATION  = SB_PROPERTY_PREFACE + "isPartOfCompilation";
const SB_PROPERTY_PRODUCERNAME         = SB_PROPERTY_PREFACE + "producerName";
const SB_PROPERTY_COMPOSERNAME         = SB_PROPERTY_PREFACE + "composerName";
const SB_PROPERTY_LYRICISTNAME         = SB_PROPERTY_PREFACE + "lyricistName";
const SB_PROPERTY_LYRICS               = SB_PROPERTY_PREFACE + "lyrics";
const SB_PROPERTY_RECORDLABELNAME      = SB_PROPERTY_PREFACE + "recordLabelName";
const SB_PROPERTY_ALBUMARTURL          = SB_PROPERTY_PREFACE + "albumArtUrl";
const SB_PROPERTY_LASTPLAYTIME         = SB_PROPERTY_PREFACE + "lastPlayTime";
const SB_PROPERTY_PLAYCOUNT            = SB_PROPERTY_PREFACE + "playCount";
const SB_PROPERTY_SKIPCOUNT            = SB_PROPERTY_PREFACE + "skipCount";
const SB_PROPERTY_RATING               = SB_PROPERTY_PREFACE + "rating";
const SB_PROPERTY_ORIGINURL            = SB_PROPERTY_PREFACE + "originURL";
const SB_PROPERTY_GUID                 = SB_PROPERTY_PREFACE + "guid";
const SB_PROPERTY_HIDDEN               = SB_PROPERTY_PREFACE + "hidden";
const SB_PROPERTY_ISLIST               = SB_PROPERTY_PREFACE + "isList";
const SB_PROPERTY_ORDINAL              = SB_PROPERTY_PREFACE + "ordinal";
const SB_PROPERTY_MEDIALISTNAME        = SB_PROPERTY_PREFACE + "mediaListName";
