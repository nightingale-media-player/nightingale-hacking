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
 * \file songbirdInterfaces.js
 * A collection of constructors to facilitate the usage of
 * sbIDatabaseQuery, sbIFileScan, sbIFileScanQuery, sbIPlaylistReaderListener, 
 * sbIPlaylistReaderManager and sbIPlaylistCommandsManager.
 * \deprecated This file will be deleted.
 * \internal
 */

//
// Formal constructors to get interfaces - all creations in js should use these unless
//   getting the object as a service, then you should use the proper getService call.
// alpha-sorted!!
//

/**
 * \brief Database Query constructor.
 * Constructs an object that implements sbIDatabaseQuery.
 * \internal
 */
const sbIDatabaseQuery = new Components.Constructor("@songbirdnest.com/Songbird/DatabaseQuery;1", "sbIDatabaseQuery");

/**
 * \brief File Scan constructor.
 * \internal
 */
const sbIFileScan = new Components.Constructor("@songbirdnest.com/Songbird/FileScan;1", "sbIFileScan");

/**
 * \brief File Scan Query constructor.
 * \internal
 */
const sbIFileScanQuery = new Components.Constructor("@songbirdnest.com/Songbird/FileScanQuery;1", "sbIFileScanQuery");

/**
 * \brief Playlist Reader Listener constructor.
 * \internal
 */
const sbIPlaylistReaderListener = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistReaderListener;1", "sbIPlaylistReaderListener");

/**
 * \brief Database Query constructor.
 * \internal
 */
const sbIPlaylistReaderManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistReaderManager;1", "sbIPlaylistReaderManager");

/**
 * \brief Database Query constructor.
 * \internal
 */
const sbIPlaylistCommandsManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistCommandsManager;1", "sbIPlaylistCommandsManager");

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_PREFACE              = "http://songbirdnest.com/data/1.0#";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_STORAGEGUID          = SB_PROPERTY_PREFACE + "storageGUID";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_CREATED              = SB_PROPERTY_PREFACE + "created";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_UPDATED              = SB_PROPERTY_PREFACE + "updated";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_CONTENTURL           = SB_PROPERTY_PREFACE + "contentURL";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_CONTENTMIMETYPE      = SB_PROPERTY_PREFACE + "contentMimeType";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_CONTENTLENGTH        = SB_PROPERTY_PREFACE + "contentLength";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_TRACKNAME            = SB_PROPERTY_PREFACE + "trackName";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_ALBUMNAME            = SB_PROPERTY_PREFACE + "albumName";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_ARTISTNAME           = SB_PROPERTY_PREFACE + "artistName";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_DURATION             = SB_PROPERTY_PREFACE + "duration";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_GENRE                = SB_PROPERTY_PREFACE + "genre";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_TRACK                = SB_PROPERTY_PREFACE + "trackNumber";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_YEAR                 = SB_PROPERTY_PREFACE + "year";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_DISCNUMBER           = SB_PROPERTY_PREFACE + "discNumber";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_TOTALDISCS           = SB_PROPERTY_PREFACE + "totalDiscs";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_TOTALTRACKS          = SB_PROPERTY_PREFACE + "totalTracks";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_ISPARTOFCOMPILATION  = SB_PROPERTY_PREFACE + "isPartOfCompilation";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_PRODUCERNAME         = SB_PROPERTY_PREFACE + "producerName";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_COMPOSERNAME         = SB_PROPERTY_PREFACE + "composerName";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_LYRICISTNAME         = SB_PROPERTY_PREFACE + "lyricistName";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_LYRICS               = SB_PROPERTY_PREFACE + "lyrics";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_RECORDLABELNAME      = SB_PROPERTY_PREFACE + "recordLabelName";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_ALBUMARTURL          = SB_PROPERTY_PREFACE + "albumArtURL";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_LASTPLAYTIME         = SB_PROPERTY_PREFACE + "lastPlayTime";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_PLAYCOUNT            = SB_PROPERTY_PREFACE + "playCount";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_SKIPCOUNT            = SB_PROPERTY_PREFACE + "skipCount";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_RATING               = SB_PROPERTY_PREFACE + "rating";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_ORIGINURL            = SB_PROPERTY_PREFACE + "originURL";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_ORIGINPAGE           = SB_PROPERTY_PREFACE + "originPage";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_GUID                 = SB_PROPERTY_PREFACE + "GUID";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_HIDDEN               = SB_PROPERTY_PREFACE + "hidden";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_ISLIST               = SB_PROPERTY_PREFACE + "isList";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_ORDINAL              = SB_PROPERTY_PREFACE + "ordinal";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_MEDIALISTNAME        = SB_PROPERTY_PREFACE + "mediaListName";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_COLUMNSPEC           = SB_PROPERTY_PREFACE + "columnSpec";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_DEFAULTCOLUMNSPEC    = SB_PROPERTY_PREFACE + "defaultColumnSpec";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_CUSTOMTYPE           = SB_PROPERTY_PREFACE + "customType";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_DOWNLOADBUTTON       = SB_PROPERTY_PREFACE + "downloadButton";

/**
 * \brief Part of standard set of properties.
 * \deprecated Please see sbProperties.jsm.
 * \internal
 */
const SB_PROPERTY_DOWNLOAD_STATUS_TARGET = SB_PROPERTY_PREFACE + "downloadStatusTarget";

