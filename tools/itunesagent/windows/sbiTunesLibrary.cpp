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

#include "sbiTunesLibrary.h"

#include <assert.h>
#include <sstream>

HRESULT const SB_ITUNES_ERROR_BUSY = 0x8001010a;

wchar_t const SB_ITUNES_MAIN_LIBRARY_SOURCE_NAME[] = L"Library";
wchar_t const SB_ITUNES_PLAYLIST_NAME[] = L"Songbird";

sbiTunesLibrary::sbiTunesLibrary() {
}

sbiTunesLibrary::~sbiTunesLibrary() {
}

/**
 * Returns the error unless it's the busy error
 */
sbError CheckCOMError(_com_error const & error) {
  if (error.Error() != SB_ITUNES_ERROR_BUSY) {
    std::stringstream msg;
    msg << L"Error: iTunes Exception occurred " << error.Error();
    return sbError(msg.str());
  }
  return sbNoError;
}

void WaitForCompletion(iTunesLib::IITOperationStatus * aStatus) {
  if (aStatus) {
    while (aStatus->InProgress) {
      Sleep(100);
    }
  }
}

sbError sbiTunesLibrary::Initialize() {
  HRESULT hr = CoCreateInstance(__uuidof(iTunesLib::iTunesApp), 
                                0,
                                CLSCTX_LOCAL_SERVER,
                                __uuidof(iTunesLib::IiTunes),
                                reinterpret_cast<LPVOID*>(&miTunesApp));
  if (FAILED(hr) || !miTunesApp) {
    return sbError(L"Failed to initialize the iTunesApp object");
  }
  // Often the iTunes app is busy and we have to retry
  for (;;) {
    try {
      // Get the library playlist
      if (!mLibraryPlaylist) {
        mLibraryPlaylist = miTunesApp->GetLibraryPlaylist();
        if (!mLibraryPlaylist) {
          return sbError(L"Failed to initialize the library playlist object");
        }
      }
      // Get the sources for iTunes
      iTunesLib::IITSourceCollectionPtr sources = miTunesApp->GetSources();
      if (!sources) {
        return sbError(L"Failed to get the source collection");
      }
      // Get the main library source
      if (!mLibrarySource) {
        mLibrarySource = sources->GetItemByName(L"Library");
        if (!mLibrarySource) {
          return sbError(L"Failed to initialize the library source object");
        }
      }
      // If we haven't gotten the Songbird playlist source then go get it
      // Get the playlists
      iTunesLib::IITPlaylistCollectionPtr playlists = 
        mLibrarySource->GetPlaylists();
      if (!playlists) {
        playlists = mLibrarySource->GetPlaylists();
        if (!mLibrarySource) {
          return sbError(L"Failed to initialize the playlist collection object");
        }
      }
      // Find the songbird playlist and source
      iTunesLib::IITPlaylistPtr playlist = 
        playlists->GetItemByName(SB_ITUNES_PLAYLIST_NAME);
      if (!playlist) {
        playlist = miTunesApp->CreateFolder(SB_ITUNES_PLAYLIST_NAME);
        if (!playlist) {
          return sbError(L"Failed to create the Songbird playlist");
        }
      }
      hr = playlist.QueryInterface(__uuidof(iTunesLib::IITUserPlaylist),
                                   &mSongbirdPlaylist);
      if (FAILED(hr) || !mSongbirdPlaylist) {
        return sbError(L"Failed to get user playlist from Songbird playlist");
      }
      
      if (!mSongbirdPlaylistSource) {
        mSongbirdPlaylistSource = mSongbirdPlaylist->GetSource();
        if (!mSongbirdPlaylistSource) {
          return sbError(L"Failed to get the source for the Songbird playlist");
        }
      }
      if (!mSongbirdPlaylists) {
        mSongbirdPlaylists = mSongbirdPlaylistSource->GetPlaylists();
        if (!mSongbirdPlaylists) {
          return sbError(L"Failed to get the playlist "
                         L"from the Songbird playlist source");
        }
      }
      // All good exit
      break;
    }
    catch (_com_error const & COMError) {
      sbError error = CheckCOMError(COMError);
      if (error) {
        return error;
      }
      // Give up the CPU so we don't hog it 
      Sleep(100);
    }
  }
  return sbNoError;
}

/**
 * Creates a safearray of BSTR from a deque of strings
 * Caller assumes ownership of array
 */
static void 
CreateVariantArray(std::deque<std::wstring> const & aStrings,
                        VARIANT * aVariant) {
  aVariant->vt = VT_ARRAY | VT_BSTR;
  SAFEARRAYBOUND bounds;
  bounds.cElements = aStrings.size();
  bounds.lLbound = 0;
  aVariant->parray = SafeArrayCreate(VT_BSTR, 1, &bounds);
  BSTR * pBSTRArray;
  SafeArrayAccessData(aVariant->parray, reinterpret_cast<void**>(&pBSTRArray));
  std::deque<std::wstring>::const_iterator const end = aStrings.end();
  for (std::deque<std::wstring>::const_iterator iter = aStrings.begin();
       iter != end;
       ++iter) {
    *pBSTRArray++ = SysAllocString(iter->c_str());
  }
  ::SafeArrayUnaccessData(aVariant->parray);
}

sbError sbiTunesLibrary::AddTracks(std::wstring const & aSource, 
                                   std::deque<std::wstring> const & aTrackPaths) {
  if (!miTunesApp) {
    return sbError(L"iTunes objects not initialized");
  }
  if (aSource == SB_ITUNES_MAIN_LIBRARY_SOURCE_NAME) {
    // Loop until success
    for (;;) {
      try {
        _variant_t var;
        CreateVariantArray(aTrackPaths, &var);
        iTunesLib::IITOperationStatusPtr progress = 
          mLibraryPlaylist->AddFiles(&var);
        WaitForCompletion(progress);
        break;
      }
      catch (_com_error const & COMError) {
        sbError error = CheckCOMError(COMError);
        if (error) {
          return error;
        }
        // Give up the CPU so we don't hog it 
        Sleep(100);
      }
    }
  }
  else {
    iTunesLib::IITUserPlaylistPtr userPlaylist;
    for (;;) {
      try {
        iTunesLib::IITPlaylistPtr playlist = mSongbirdPlaylists->GetItemByName(aSource.c_str());
        if (!playlist) {
          playlist = mSongbirdPlaylist->CreatePlaylist(aSource.c_str());
          if (!playlist) {
            std::ostringstream msg;
            msg << "Unable to create playlist";
            return sbError(msg.str());
          }
        }
        userPlaylist = playlist;
        if (!userPlaylist) {
          std::ostringstream msg;
          msg << "Playlist is not a user defined playlist";
          return sbError(msg.str());
        }
        break;
      }
      catch (_com_error const & COMError) {
        sbError error = CheckCOMError(COMError);
        if (error) {
          return error;
        }
        // Give up the CPU so we don't hog it 
        Sleep(100);
      }
    }
    assert(userPlaylist);
    for (;;) {
      try {
        _variant_t var;
        CreateVariantArray(aTrackPaths, &var);
        iTunesLib::IITOperationStatusPtr progress = 
          mLibraryPlaylist->AddFiles(&var);
        WaitForCompletion(progress);
        break;
      }
      catch (_com_error const & COMError) {
        sbError error = CheckCOMError(COMError);
        if (error) {
          return error;
        }
        // Give up the CPU so we don't hog it 
        Sleep(100);
      }
    }
  }
  return sbNoError;
}

sbError sbiTunesLibrary::CreatePlaylist(std::wstring const & aPlaylistName) {

  for (;;) {
    try {
      iTunesLib::IITPlaylistPtr playlist = 
        mSongbirdPlaylists->GetItemByName(aPlaylistName.c_str());
      // If the playlist exists wipe it, since we're "creating it"
      if (playlist) {
        playlist->Delete();
      }
      //_variant_t var(mSongbirdPlaylistSource);
      //miTunesApp->CreatePlaylistInSource(aPlaylistName.c_str(), &var);
      mSongbirdPlaylist->CreatePlaylist(aPlaylistName.c_str());
      break;
    }
    catch (_com_error const & COMError) {
      sbError error = CheckCOMError(COMError);
      if (error) {
        return error;
      }
      // Give up the CPU so we don't hog it 
      Sleep(100);
    }
  }
  return sbNoError;
}

sbError 
sbiTunesLibrary::RemovePlaylist(std::wstring const & aPlaylistName) {
  for (;;) {
    try {
      iTunesLib::IITPlaylistPtr playlist = 
        mSongbirdPlaylists->GetItemByName(aPlaylistName.c_str());
      if (playlist) {
        playlist->Delete();
      }
      break;
    }
    catch (_com_error const & COMError) {
      sbError error = CheckCOMError(COMError);
      if (error) {
        return error;
      }
      // Give up the CPU so we don't hog it 
      Sleep(100);   
    }
  }
  return sbNoError;
}

void sbiTunesLibrary::Finalize() {
  mLibraryPlaylist = 0;
  mLibrarySource = 0;
  mSongbirdPlaylists = 0;
  mSongbirdPlaylist = 0;
  mSongbirdPlaylistSource = 0;
  miTunesApp = 0;   
}
