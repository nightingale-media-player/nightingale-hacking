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

#ifndef SBITUNESLIBRARY_H_
#define SBITUNESLIBRARY_H_

#ifndef _WIN32_WINNT    // Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501 // Change this to the appropriate value to target other versions of Windows.
#endif            

#include <deque>
#include <stdio.h>
#include <string>
#include <tchar.h>
#include <windows.h>

#include "sbError.h"
#include "sbIDispatchPtr.h"

/**
 * This class wraps the iTunes COM API to provide a simpler interface
 */
class sbiTunesLibrary
{
public:
  /**
   * Does really do anything except place all the pointer initialization 
   * in the cpp reducing code size
   */
  sbiTunesLibrary();
  /**
   * Does really do anything except place all the pointer initialization 
   * in the cpp reducing code size
   */
  ~sbiTunesLibrary();
  /**
   * Initializes commonly used iTunes COM objects
   */
  sbError Initialize();
  /**
   * Adds a track given a source (playlist) and a URI
   */
  sbError AddTracks(std::wstring const & aSource,
                    std::deque<std::wstring> const & aTrackPaths);
  /**
   * Creates a playlist, will erase the playlist if it already exists
   */
  sbError CreatePlaylist(std::wstring const & aPlaylistName);
  /**
   * Releases all the iTunes COM objects
   */
  void Finalize();
  /**
   * Removes a playlist from the iTunes database
   */
  sbError RemovePlaylist(std::wstring const & aPlaylistName);
private:
  sbIDispatchPtr mLibraryPlaylist;
  sbIDispatchPtr mLibrarySource;
  sbIDispatchPtr mSongbirdPlaylists;
  sbIDispatchPtr mSongbirdPlaylist;
  sbIDispatchPtr mSongbirdPlaylistSource;
  sbIDispatchPtr miTunesApp;   
};

#endif /* SBITUNESLIBRARY_H_ */
