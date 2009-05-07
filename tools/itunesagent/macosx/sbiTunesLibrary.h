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

  sbError GetPlaylistName(std::string *aOutString);
  sbError SetPlaylistName(std::string const & aPlaylistName);
};

//------------------------------------------------------------------------------
// Manager class for modifying the users iTunes library.

class sbiTunesLibraryManager
{
public:
  sbiTunesLibraryManager();
  virtual ~sbiTunesLibraryManager();

  //
  // \brief Get the playlist item for the main iTunes library.
  //
  sbError GetMainLibraryPlaylist(sbiTunesPlaylist **aOutPlaylist);

  //
  // \brief Get the Songbird folder playlist item from iTunes.
  //        This method will create the folder if it doesn't currently exist
  //        inside of iTunes.
  //
  //        The returned list is maintained by this class and doesn't need
  //        to be release by the caller.
  //
  sbError GetSongbirdPlaylistFolder(sbiTunesPlaylist **aOutPlaylist);

  //
  // \brief Get the name of the main iTunes library.
  //
  sbError GetMainLibraryPlaylistName(std::string & aPlaylistName);

  //
  // \brief Get the playlist object under the Songbird folder by the given
  //        name. 
  //        
  //        The returned playlist is not maintained by this class and must
  //        be released by the caller.
  //
  sbError GetSongbirdPlaylist(std::string const & aPlaylistName,
                              sbiTunesPlaylist **aOutPlaylist);

  //
  // \brief Add a list of files to iTunes to the specified playlist.
  //
  sbError AddTrackPaths(std::deque<std::string> const & aTrackPaths,
                        sbiTunesPlaylist *aTargetPlaylist);

  //
  // \brief Create a playlist under the Songbird folder in iTunes.
  //
  sbError CreateSongbirdPlaylist(std::string const & aPlaylistName);

  //
  // \brief Remove a playlist under the Songbird folder in iTunes.
  //
  sbError DeleteSongbirdPlaylist(sbiTunesPlaylist *aPlaylist);

private:
  sbiTunesPlaylist *mMainLibraryPlaylist;
  sbiTunesPlaylist *mSongbirdFolderPlaylist;
};

#endif  // sbITunesLibrary_h_

