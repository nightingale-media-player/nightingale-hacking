/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: set ts=8 et sw=2 ai tw=80: */
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


#ifndef sbITunesLibrary_h_
#define sbITunesLibrary_h_

#include <CoreServices/CoreServices.h>
#include <Carbon/Carbon.h>
#include <string>
#include <deque>
#include <vector>
#include "sbiTunesAgentProcessor.h"

class sbError;

//------------------------------------------------------------------------------
// Base class for stashing objects from iTunes.

class sbiTunesObject
{
public:
  sbiTunesObject();
  virtual ~sbiTunesObject();

  sbError Init(AppleEvent *aAppleEvent);
  AEDesc* GetDescriptor();

protected:
  AEDesc mDescriptor;
};

//------------------------------------------------------------------------------
// Playlist class for representing playlists from iTunes.

class sbiTunesPlaylist : public sbiTunesObject
{
public:
  sbiTunesPlaylist();
  virtual ~sbiTunesPlaylist();

  sbError GetPlaylistName(std::string & aOutString);
  sbError SetPlaylistName(std::string const & aPlaylistName);

protected:
  std::string mPlaylistName;
  bool        mLoadedPlaylist;
};

//------------------------------------------------------------------------------
// Typedefs 

typedef std::auto_ptr<sbiTunesPlaylist>      sbiTunesPlaylistPtr;
typedef std::vector<sbiTunesPlaylist *>      sbiTunesPlaylistContainer;
typedef sbiTunesPlaylistContainer::iterator  sbiTunesPlaylistContainerIter;

extern sbError const sbiTunesNotRunningError;

//------------------------------------------------------------------------------
// Manager class for modifying the users iTunes library.

class sbiTunesLibraryManager
{
public:
  typedef std::pair<unsigned long long, std::string> TrackProperty;
  typedef std::deque<TrackProperty> TrackPropertyList;
  sbiTunesLibraryManager();
  virtual ~sbiTunesLibraryManager();

  //
  // \brief Initialize the iTunes library class. This method should be called
  //        before attempting to call any other method on this class.
  //
  sbError Init(std::string const & aFolderName);

  //
  // \brief If iTunes is shutdown after the manager has been initialized, the
  //        the internal data is invalid. Call this method after iTunes has
  //        been reopened to re-inialize all the managers data.
  //
  sbError ReloadManager();

  //
  // \brief Get the playlist item for the main iTunes library.
  //
  // NOTE: The out-param is owned by this class, do not |delete|.
  //
  sbError GetMainLibraryPlaylist(sbiTunesPlaylist **aPlaylistPtr);

  //
  // \brief Get the persistent ID of the library
  //
  sbError GetLibraryId(std::string &aId);

  //
  // \brief Get the Songbird folder playlist item from iTunes.
  //        This method will create the folder if it doesn't currently exist
  //        inside of iTunes.
  //
  // NOTE: The out-param is owned by this class, do not |delete|.
  //
  sbError GetSongbirdPlaylistFolder(sbiTunesPlaylist **aPlaylistPtr);

  //
  // \brief Get the name of the main iTunes library.
  //
  sbError GetMainLibraryPlaylistName(std::string & aPlaylistName);

  //
  // \brief Get the playlist object under the Songbird folder by the given
  //        name. 
  //
  // NOTE: The out-param is owned by this class, do not |delete|.
  //        
  sbError GetSongbirdPlaylist(std::string const & aPlaylistName,
                              sbiTunesPlaylist **aOutPlaylist);

  //
  // \brief Add a list of files to iTunes to the specified playlist.
  //
  // \param aTargetPlaylist the playlist to add the tracks into
  // \param aTrackPaths the paths to add
  // \param aDatabaseID the (non-persistent) database IDs of the new tracks
  //
  sbError AddTrackPaths(sbiTunesPlaylist *aTargetPlaylist,
                        std::deque<std::string> const & aTrackPaths,
                        std::deque<std::string> & aDatabaseIds);

  //
  // \brief Updates a track in the library, given the iTunes persistent ID and
  //        a new URI
  //
  sbError UpdateTracks(TrackPropertyList const & aTrackProperties);

  //
  // \brief Create a playlist under the Songbird folder in iTunes.
  //
  sbError CreateSongbirdPlaylist(std::string const & aPlaylistName);

  //
  // \brief Remove a playlist under the Songbird folder in iTunes.
  //
  sbError DeleteSongbirdPlaylist(sbiTunesPlaylist *aPlaylist);

protected:
  sbError LoadMainLibraryPlaylist();
  sbError LoadSongbirdPlaylistFolder();
  sbError BuildSongbirdPlaylistFolderCache();

private:
  sbiTunesPlaylistPtr             mMainLibraryPlaylistPtr;
  sbiTunesPlaylistPtr             mSongbirdFolderPlaylistPtr;
  sbiTunesPlaylistContainer       mCachedSongbirdPlaylists;
};


#endif  // sbITunesLibrary_h_

