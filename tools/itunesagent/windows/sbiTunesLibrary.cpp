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

wchar_t const ITUNES_APP_PROGID[] = L"iTunes.Application";


sbiTunesLibrary::sbiTunesLibrary() {
}

sbiTunesLibrary::~sbiTunesLibrary() {
}

/**
 * Returns an error message for the COM error, if any
 */
sbError MakeCOMError(HRESULT aError, std::string const & aAction) {
  if (FAILED(aError)) {
    std::stringstream msg;
    msg << "Error: iTunes error [" << aError << "] occurred while " << aAction;
    return sbError(msg.str());
  }
  return sbNoError;
}

/**
 * Returns true if this is the busy error and we need to retry
 * also perform a sleep so we don't hog the CPU
 */
inline
bool COMBusyError(HRESULT error) {
  bool const busy = error == SB_ITUNES_ERROR_BUSY;
  if (busy) {
    // Sleep for 1/10 second
    Sleep(100);
  }
  return busy; 
}

/**
 * Returns true if the variant is a non-null dispatch pointer
 */
inline
bool IsIDispatch(VARIANTARG const & aVar) {
  return aVar.vt == VT_DISPATCH && aVar.pdispVal != 0;
}

void WaitForCompletion(IDispatch * aStatus) {
  if (aStatus) {
    sbIDispatchPtr status(aStatus);
    bool inProgress;
    for (HRESULT hr = status.GetProperty(L"InProgress", inProgress); 
         SUCCEEDED(hr) && inProgress;
         hr = status.GetProperty(L"InProgress", inProgress)) {
      Sleep(100);
    }
  }
}

sbError sbiTunesLibrary::Initialize() {
  CLSID iTunesClassID;
  HRESULT hr = CLSIDFromProgID(ITUNES_APP_PROGID,
                               &iTunesClassID);
  if (FAILED(hr)) {
    return sbError("iTunes was not found");
  }
  hr = CoCreateInstance(iTunesClassID, 
                        0,
                        CLSCTX_LOCAL_SERVER,
                        __uuidof(IDispatch),
                        miTunesApp);
  if (FAILED(hr) || !miTunesApp.get()) {
    return sbError("Failed to initialize the iTunesApp object");
  }
  // Often the iTunes app is busy and we have to retry
  for (;;) {
    // Get the library playlist
    if (!mLibraryPlaylist.get()) {
      hr = miTunesApp.GetProperty(L"LibraryPlaylist", &mLibraryPlaylist);
      if (COMBusyError(hr)) {
        continue;
      }
      else if (FAILED(hr) || !mLibraryPlaylist.get()) {
        return sbError("Failed to initialize the library playlist object");
      }
    }
    // Get the sources for iTunes
    sbIDispatchPtr sources;
    hr = miTunesApp.GetProperty(L"Sources", &sources);
    if (COMBusyError(hr)) {
      continue;
    }
    else if (FAILED(hr) || !sources.get()) {
      return sbError("Failed to get the source collection");
    }
    // Get the main library source
    if (!mLibrarySource.get()) {
      sbIDispatchPtr::VarArgs args(1);
      args.Append(L"Library");
      VARIANTARG result;
      VariantInit(&result);
      hr = sources.Invoke(L"ItemByName", args, result, DISPATCH_PROPERTYGET);  
      if (COMBusyError(hr)) {
        continue;
      }
      else if (FAILED(hr) || !IsIDispatch(result)) {
        return sbError("Failed to initialize the library source object");
      }
      mLibrarySource = result.pdispVal;
    }
    if (!mSongbirdPlaylist.get()) {
      // If we haven't gotten the Songbird playlist source then go get it
      // Get the playlists
      sbIDispatchPtr playlists;
      hr = mLibrarySource.GetProperty(L"Playlists", &playlists);
      if (COMBusyError(hr)) {
        continue;
      }
      else if (FAILED(hr) || !playlists.get()) {
        return sbError("Failed to initialize the playlist collection object");
      }
      // Find the songbird playlist and source
      sbIDispatchPtr playlist;
      sbIDispatchPtr::VarArgs args(1);
      args.Append(SB_ITUNES_PLAYLIST_NAME);
      VARIANTARG result;
      VariantInit(&result);
      hr = playlists.Invoke(L"ItemByName", args, result, DISPATCH_PROPERTYGET);
      if (COMBusyError(hr)) {
        continue;
      }
      // Did we get a playlist back, if not create one
      else if (FAILED(hr) || !IsIDispatch(result)) {
        sbIDispatchPtr::VarArgs args(1);
        args.Append(SB_ITUNES_PLAYLIST_NAME);
        VariantClear(&result);
        hr = miTunesApp.Invoke(L"CreateFolder", args, result);
        if (COMBusyError(hr)) {
          continue;
        }
        // Did we create the playlist? If not return an error
        else if (FAILED(hr) || !IsIDispatch(result)) {
          return sbError("Failed to create the Songbird playlist");
        }
      }
      mSongbirdPlaylist = result.pdispVal;
    }    
    if (!mSongbirdPlaylistSource.get()) {
      hr = mSongbirdPlaylist.GetProperty(L"Source", &mSongbirdPlaylistSource);
      if (COMBusyError(hr)) {
        continue;
      }
      else if (FAILED(hr) || !mSongbirdPlaylistSource.get()) {
        return sbError("Failed to get the source for the Songbird playlist");
      }
    }
    if (!mSongbirdPlaylists.get()) {
      hr = mSongbirdPlaylistSource.GetProperty(L"Playlists", &mSongbirdPlaylists);
      if (COMBusyError(hr)) {
        continue;
      }
      else if (FAILED(hr) || !mSongbirdPlaylists.get()) {
        return sbError("Failed to get the playlist "
                       "from the Songbird playlist source");
      }
    }
    // All good exit
    break;
  }
  return sbNoError;
}

/**
 * Creates a safearray of BSTR from a deque of strings
 * Caller assumes ownership of array
 */
static void 
CreateVariantArray(std::deque<std::wstring> const & aStrings,
                        VARIANTARG * aVariant) {
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
  if (!miTunesApp.get()) {
    return sbError("iTunes objects not initialized");
  }
  if (aSource == SB_ITUNES_MAIN_LIBRARY_SOURCE_NAME) {
    // Loop until success
    for (;;) {
      VARIANTARG varFiles;
      CreateVariantArray(aTrackPaths, &varFiles);
      VARIANTARG result;
      VariantInit(&result);
      sbIDispatchPtr::VarArgs args(1);
      args.Append(varFiles);
      VariantClear(&varFiles);
      HRESULT hr = mLibraryPlaylist.Invoke(L"AddFiles", args, result);
      if (COMBusyError(hr)) {
        continue;
      }
      else if (FAILED(hr)) {
        return MakeCOMError(hr, "adding tracks");
      }
      if (IsIDispatch(result)) {
        WaitForCompletion(result.pdispVal);
      }
      VariantClear(&result);
      break;
    }
  }
  else {
    sbIDispatchPtr userPlaylist;
    for (;;) {
      sbIDispatchPtr::VarArgs args(1);
      args.Append(aSource);
      VARIANTARG result;
      VariantInit(&result);
      HRESULT hr = mSongbirdPlaylists.Invoke(L"ItemByName", 
                                             args, 
                                             result, 
                                             DISPATCH_PROPERTYGET);
      if (COMBusyError(hr)) {
        continue;
      }
      else if (FAILED(hr) || !IsIDispatch(result)) {
        VariantClear(&result);
        sbIDispatchPtr::VarArgs args(1);
        args.Append(aSource);
        hr = mSongbirdPlaylist.Invoke(L"CreatePlaylist", args, result);
        if (FAILED(hr)) {
          return MakeCOMError(hr, "creating a playlist");
        }
      }
      userPlaylist = result.pdispVal;
      break;
    }
    assert(userPlaylist.get());
    for (;;) {
      VARIANTARG var;
      VariantInit(&var);
      CreateVariantArray(aTrackPaths, &var);
      VARIANTARG result;
      VariantInit(&result);
      sbIDispatchPtr::VarArgs args(1);
      args.Append(var);
      VariantClear(&var);
      HRESULT hr = userPlaylist.Invoke(L"AddFiles", args, result);
      if (COMBusyError(hr)) {
        continue;
      }
      else if (FAILED(hr)) {
        return MakeCOMError(hr, "adding files");
      }
      if (IsIDispatch(result)) {
        WaitForCompletion(result.pdispVal);
      }
      VariantClear(&result);
      break;
    }
  }
  return sbNoError;
}

sbError sbiTunesLibrary::CreatePlaylist(std::wstring const & aPlaylistName) {
  sbError error = RemovePlaylist(aPlaylistName);
  // We expect errors so just continue and attempt the create
  error.Checked();
  for (;;) {
    sbIDispatchPtr::VarArgs args(1);
    args.Append(aPlaylistName);
    VARIANTARG result;
    VariantInit(&result);
    HRESULT hr = mSongbirdPlaylist.Invoke(L"CreatePlaylist", args, result);
    if (COMBusyError(hr)) {
      continue;
    }
    else if (FAILED(hr)) {
      return MakeCOMError(hr, "creating playlist");
    }
    VariantClear(&result);
    break;
  }
  return sbNoError;
}

sbError 
sbiTunesLibrary::RemovePlaylist(std::wstring const & aPlaylistName) {
  for (;;) {
    sbIDispatchPtr::VarArgs args(1);
    args.Append(aPlaylistName);
    VARIANTARG result;
    VariantInit(&result);
    HRESULT hr = mSongbirdPlaylists.Invoke(L"ItemByName", 
                                           args, 
                                           result, 
                                           DISPATCH_PROPERTYGET);
    if (COMBusyError(hr)) {
      continue;
    }
    else if (FAILED(hr) || !IsIDispatch(result)) {
      return MakeCOMError(hr, "find playlist");
    }
    sbIDispatchPtr playlist(result.pdispVal);
    VariantClear(&result);
    
    sbIDispatchPtr::VarArgs deleteArgs;
    hr = playlist.Invoke(L"Delete");
    if (COMBusyError(hr)) {
      continue;
    }
    else if (FAILED(hr)) {
      return MakeCOMError(hr, "deleting playlist");
    }
    break;
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
