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

#ifndef SBITUNESIMPORTER_H_
#define SBITUNESIMPORTER_H_

#include <map>
#include <memory>
#include <vector>

#include <nsDataHashtable.h>
#include <nsIRunnable.h>
#include <nsStringAPI.h>

#include <sbILibraryImporter.h>
#include <sbIiTunesXMLParserListener.h>

#include "sbiTunesDatabaseServices.h"
#include "sbiTunesImporterCommon.h"
#include "sbiTunesImporterStatus.h"
#include "sbiTunesSignature.h"
#include "sbiTunesXMLParser.h"

// Mozilla forwards
class nsIArray;
class nsIIOService;
class nsIInputStream;

// Songbird forwards
class sbIAlbumArtFetcherSet;
class sbILibrary;
class sbILocalDatabaseLibrary;
class sbIMediacoreTypeSniffer;
class sbIMediaList;
class sbIPropertyArray;
#ifdef SB_ENABLE_TEST_HARNESS
class sbITimingService;
#endif
class sbPrefBranch;

// Component constants
#define SBITUNESIMPORTER_CONTRACTID                     \
  "@songbirdnest.com/Songbird/ITunesImporter;1"
#define SBITUNESIMPORTER_CLASSNAME                      \
  "iTunes Library Importer"
// {56202248-E7AB-4e45-BFF3-F296844688C4}
#define SBITUNESIMPORTER_CID                            \
{ 0x56202248, 0xe7ab, 0x4e45, { 0xbf, 0xf3, 0xf2, 0x96, 0x84, 0x46, 0x88, 0xc4 } }

/**
 * This class is the main driver of iTunes import. Import is the main
 * entry point. Processing continues after Import returns and driven by the 
 * stream pump in sbiTunesXMLParser.
 */
class sbiTunesImporter : public sbILibraryImporter,
                         public sbIiTunesXMLParserListener
{
public:
  enum OSType {
    UNINITIALIZED,
    MAC_OS,
    LINUX_OS,
    WINDOWS_OS,
    UNKNOWN_OS
  };

  NS_DECL_ISUPPORTS
  NS_DECL_SBILIBRARYIMPORTER
  NS_DECL_SBIITUNESXMLPARSERLISTENER
  
  /**
   * Initialize counters and such
   */
  sbiTunesImporter();

private:
  virtual ~sbiTunesImporter();
  
  struct iTunesTrack;
  /**
   * Batch size for doing work
   */
  static PRUint32 const BATCH_SIZE = 100;
  /**
   * Data format version for the database
   */
  static PRUint32 const DATA_FORMAT_VERSION = 2;

  // Various nCOMPtr handy typedefs
  typedef nsCOMPtr<nsIThread>                   nsIThreadPtr;

  typedef nsCOMPtr<sbIAlbumArtFetcherSet>       sbIAlbumArtFetcherSetPtr;
  typedef nsCOMPtr<sbIMediacoreTypeSniffer>     sbIMediacoreTypeSnifferPtr;
  typedef nsCOMPtr<nsIInputStream>              nsIInputStreamPtr;
  typedef nsCOMPtr<nsIIOService>                nsIIOServicePtr;
  typedef nsCOMPtr<sbILibraryImporterListener>  sbILibraryImporterListenerPtr;
  typedef nsCOMPtr<sbILibrary>                  sbILibraryPtr;
  typedef nsCOMPtr<sbILocalDatabaseLibrary>     sbILocalDatabaseLibraryPtr;
#ifdef SB_ENABLE_TEST_HARNESS
  typedef nsCOMPtr<sbITimingService>            sbITimingServicePtr;
#endif  
  typedef nsRefPtr<sbiTunesXMLParser>           sbiTunesXMLParserPtr;
  /**
   * Simple non-ref counted sbiTunesImporterStatus pointer
   */
  typedef std::auto_ptr<sbiTunesImporterStatus> sbiTunesImporterStatusPtr;
  /** 
   * A simple string to string map type
   */
  typedef nsDataHashtable<nsStringHashKey, nsString> StringMap;
  
  /**
   * Type used to hold the batch data for tracks
   */
  typedef std::vector<iTunesTrack *>            TrackBatch;
  /**
   * Maps between track ID and SB GUID.
   * XXX TODO consider using StringMap, code to iterate was easier
   */
  typedef std::map<nsString, nsString>          TrackIDMap;
  /**
   * Maps between the track ID and the index of the track in the batch
   * XXX TODO consider using nsDataHashtable
   */
  typedef std::map<nsString, PRUint32>          TracksByID;

  /**
   * Contains information about the iTunes track
   */
  struct iTunesTrack {
    iTunesTrack();
    ~iTunesTrack();
    nsresult Initialize(sbIStringMap * aProperties);
    nsString GetContentType(sbIStringMap *aProperties);
    nsresult GetPropertyArray(sbIPropertyArray ** aPropertyArray);
    nsresult GetTrackURI(sbiTunesImporter::OSType aOSType, 
                         nsIIOService * aIOService,
                         sbiTunesSignature & aSignanture, 
                         nsIURI ** aURI);
    nsString mTrackID;
    nsString mSBGuid;
    StringMap mProperties;
    nsCOMPtr<nsIURI> mURI;
  };

  /**
   * Album art fetcher used to update album art of imported tracks
   */
  sbIAlbumArtFetcherSetPtr mAlbumArtFetcher;
  /**
   * Flag to let us know if batch has been ended
   */
  PRBool mBatchEnded;
  /**
   * The current data format version of the itunes mapping data
   */
  PRInt32 mDataFormatVersion;
  /**
   * Flag to indicate if we've found any changes
   */
  PRBool mFoundChanges;
  /**
   * Flag to denote if we're importing. TRUE = Importing, FALSE = first run 
   * update
   */
  PRBool mImport;
  /**
   * Flag to indicate whether we're to import playlists
   */
  PRBool mImportPlaylists;
  /**
   * IO service used to get URI's and such
   */
  nsIIOServicePtr mIOService;
  /**
   * The stream tied to the iTunes XML database file
   */
  nsIInputStreamPtr mStream;
  /**
   * This is the iTunes DB service. This is basically used to lookup
   * ID mappings from the database
   */
  sbiTunesDatabaseServices miTunesDBServices;
  /**
   * ID of the iTunes library
   */
  nsString miTunesLibID;
  /**
   * Signature for the iTunes Library
   */
  sbiTunesSignature miTunesLibSig;
  /**
   * a sbILocalDatabaseLibrary version of the mLibrary pointer. Used to
   * start and end batch updates
   */
  sbILocalDatabaseLibraryPtr mLDBLibrary;
  /**
   * The Songbird main library
   */
  sbILibraryPtr mLibrary;
  /**
   * Path to the iTunes library
   */
  nsString mLibraryPath;
  /**
   * The importer listener for our clients
   */
  sbILibraryImporterListenerPtr mListener;
  /**
   * The number of missing media items we found
   */
  PRUint32 mMissingMediaCount;
  /**
   * The type of OS we're running on
   */
  OSType mOSType;
  /**
   * The XML parser
   */
  sbiTunesXMLParserPtr mParser;
  /**
   * This stores the action when the user choses "apply to all"
   */
  nsString mPlaylistAction;
  /**
   * List of playlists NOT to import
   */
  nsString mPlaylistBlacklist;
  /**
   * The is the iTunes persistent ID of the special Songbird Folder
   */
  nsString mSongbirdFolderID;
  /**
   * Our status updating object
   */
  sbiTunesImporterStatusPtr mStatus;
  /**
   * Timing service
   */
#ifdef SB_ENABLE_TEST_HARNESS
  sbITimingServicePtr mTimingService;
#endif
  /**
   * Identifier for timing service
   */
  nsString mTimingServiceIdentifier;
  /**
   * Number of tracks we've processed 
   */
  PRUint32 mTrackCount;
  /**
   * The collection of tracks for the batch
   */
  TrackBatch mTrackBatch;
  /**
   * Mapping of the iTunes ID to the songbird ID
   */
  TrackIDMap mTrackIDMap;
  /**
   * Our type sniffer for determining supported media times
   */
  sbIMediacoreTypeSnifferPtr mTypeSniffer;
  /**
   * Number of unsupported media items we've encountered
   */
  PRUint32 mUnsupportedMediaCount;
  /**
   * Number of bytes read during XML parsing
   */
  PRInt64 mBytesRead;
  
  /**
   * Cancels the current import
   */
  nsresult Cancel();
  /**
   * Determines if the iTunes database has been modified
   */
  nsresult DBModified(sbPrefBranch & aPrefs,
                      nsAString const & aLibPath,
                      PRBool * aModified);
  /**
   * Helper function to destry the iTunesTracks in the mTrackBatch member
   */
  inline
  static void DestructiTunesTrack(iTunesTrack * aTrack);
  /**
   * Asks the user what to do when there is a dirty playlist
   */
  nsresult GetDirtyPlaylistAction(nsAString const & aPlaylistName,
                                  nsAString & aAction);
  /**
   * Returns the OS type. This caches the result, so subsequent calls are cheap
   */
  OSType GetOSType();
  /**
   * Imports a playlist given a collection of properties and track ID's
   * \param aProperties the collection of properties found in the iTunes db
   * \param aTrackIds array of iTunes Track ID's for the playlist
   * \param aTrackIdsCount the number of ID's in aTrackIds array
   * \param aMediaList the Songbird media list to add the tracks to 
   */
  nsresult ImportPlaylist(sbIStringMap *aProperties, 
                          PRInt32 *aTrackIds, 
                          PRUint32 aTrackIdsCount,
                          sbIMediaList * aMediaList);
  /**
   * This does post processing of created items
   * \param aCreatedItems the array of newly created items, sbIMediaItems
   * 
   */
  nsresult ProcessCreatedItems(
    nsIArray * aCreatedItems,
    TracksByID const & aTrackMap);
  /**
   * Processes new items
   * \param aTrackMap the map of Tracks by ID
   * \param aNewItems the list of new items (sbIMediaItem)
   */
  nsresult ProcessNewItems(TracksByID & aTrackMap,
                           nsIArray ** aNewItems);
  /**
   * This handles adding the tracks to the songbird playlist. 
   * Used by ImportPlaylist
   * \param aMediaList The songbird playlist to update
   * \param aTrackIds the array of iTunes track ID's to add
   * \param aTrackIdsCount the number of ID's in aTrackIds
   */
  nsresult ProcessPlaylistItems(sbIMediaList * aMediaList,
                                PRInt32 * aTrackIds,
                                PRUint32 aTrackIdsCount);
  /**
   * Process the tracks in mTrackBatch
   */
  nsresult ProcessTrackBatch();
  /**
   * Processes the tracks that exist and need updating in mTrackBatch
   */
  nsresult ProcessUpdates();
  /**
   * Hold over from old botched thread implementation
   */
  nsresult Run();
  /**
   * Determines if the playlist should be imported. Checks against blacklist
   * and special playlists
   * \param aProperties The properties of the playlist 
   */
  PRBool ShouldImportPlaylist(sbIStringMap * aProperties);
  /**
   * Update our status object's progress 
   */
  nsresult UpdateProgress();

};

#endif /* SBITUNESIMPORTER_H_ */
