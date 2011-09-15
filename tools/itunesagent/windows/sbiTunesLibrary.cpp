/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#include "sbiTunesLibrary.h"

#include <algorithm>
#include <assert.h>
#include <limits.h>
#include <iomanip>
#include <sstream>

#include <sbMemoryUtils.h>

HRESULT const SB_ITUNES_ERROR_BUSY = 0x8001010a;

wchar_t const SB_ITUNES_MAIN_LIBRARY_SOURCE_NAME[] = SONGBIRD_MAIN_LIBRARY_NAME;

wchar_t const ITUNES_APP_PROGID[] = L"iTunes.Application";

SB_AUTO_NULL_CLASS(sbVariantArg, VARIANTARG*, VariantClear(mValue));

sbiTunesLibrary::sbiTunesLibrary()
  : miTunesFolderName(L"Songbird")
{
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

sbError sbiTunesLibrary::Initialize(std::string const & aFolderName) {
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

  // Convert the folder name string to a wide-character string
  if (!aFolderName.empty())
  {
    const char * folderName = aFolderName.c_str();
    wchar_t wfolderName[MAX_PATH];
    int size = MultiByteToWideChar(CP_ACP,
                                   0,
                                   folderName,
                                   -1,
                                   wfolderName,
                                   MAX_PATH);
    if (size) {
      miTunesFolderName = wfolderName;
    }
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
    // Get the main library source
    if (!mLibrarySource.get()) {
      hr = miTunesApp.GetProperty(L"LibrarySource", &mLibrarySource);
      if (COMBusyError(hr)) {
        continue;
      }
      else if (FAILED(hr) || !mLibrarySource.get()) {
        return sbError("Failed to initialize the library source object");
      }
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
      args.Append(miTunesFolderName);
      VARIANTARG result;
      VariantInit(&result);
      hr = playlists.Invoke(L"ItemByName", args, result, DISPATCH_PROPERTYGET);
      if (COMBusyError(hr)) {
        continue;
      }
      // Did we get a playlist back, if not create one
      else if (FAILED(hr) || !IsIDispatch(result)) {
        sbIDispatchPtr::VarArgs args(1);
        args.Append(miTunesFolderName);
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
    // Get the main library persistent id
    {
      long persistentIdHigh, persistentIdLow;
      sbVariantArg result = new VARIANTARG;
      sbIDispatchPtr::VarArgs args(1);
      args.Append(mLibrarySource.get());
      hr = miTunesApp.Invoke(L"ITObjectPersistentIDHigh",
                             args,
                             *result.get(),
                             DISPATCH_METHOD | DISPATCH_PROPERTYGET);
      if (FAILED(hr)) {
        return sbError("Failed to get high persistent ID for library");
      }
      assert(result.get()->vt == VT_I4);
      persistentIdHigh = result.get()->lVal;
      hr = miTunesApp.Invoke(L"ITObjectPersistentIDLow",
                             args,
                             *result.get(),
                             DISPATCH_METHOD | DISPATCH_PROPERTYGET);
      if (FAILED(hr)) {
        return sbError("Failed to get low persistent ID for library");
      }
      assert(result.get()->vt == VT_I4);
      persistentIdLow = result.get()->lVal;

      std::ostringstream sstr;
      sstr << std::hex << std::uppercase << std::setw(8) << std::setfill('0')
           << persistentIdHigh << persistentIdLow;
      mLibraryPersistentId = sstr.str();
    }
    // All good exit
    break;
  }
  return sbNoError;
}

const std::string& sbiTunesLibrary::GetLibraryId() {
  return mLibraryPersistentId;
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
                                   std::deque<std::wstring> const & aTrackPaths,
                                   std::deque<std::wstring> & aDatabaseIds) {
  if (!miTunesApp.get()) {
    return sbError("iTunes objects not initialized");
  }
  if (aSource == SB_ITUNES_MAIN_LIBRARY_SOURCE_NAME) {
    sbIDispatchPtr operationStatus;
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
      operationStatus = result.pdispVal;

      if (!result.pdispVal) {
        // Looks like this batch contained no tracks that iTunes accepted.
        // Go ahead and continue on.
        return sbNoError;
      }
      
      VariantClear(&result);
      break;
    }

    sbIDispatchPtr tracks;
    HRESULT hr = operationStatus.GetProperty(L"Tracks", &tracks);
    if (FAILED(hr)) {
      return MakeCOMError(hr, "fetching added tracks");
    }

    long trackCount = 0;
    hr = tracks.GetProperty(L"Count", trackCount);
    if (FAILED(hr)) {
      return MakeCOMError(hr, "fetching number of added tracks");
    }
    for (long i = 1; i <= trackCount; ++i) {
      sbIDispatchPtr::VarArgs args(1);
      args.Append(i);
      sbVariantArg result = new VARIANTARG;
      hr = tracks.Invoke(L"Item",
                         args,
                         *result.get(),
                         DISPATCH_METHOD | DISPATCH_PROPERTYGET);
      if (FAILED(hr) || !IsIDispatch(*result.get())) {
        // we failed to get an item; skip to the next one, instead of aborting
        continue;
      }
      sbIDispatchPtr track(result.get()->pdispVal);

      long trackId;
      hr = track.GetProperty(L"TrackDatabaseID", trackId);
      if (FAILED(hr)) {
        // failed to get the track ID; try the next item
        continue;
      }

      // the tracks are assumed to be in the same order as the inputs :(

      std::wostringstream trackIdString;
      trackIdString << trackId;
      aDatabaseIds.push_back(trackIdString.str());
    }

  }
  else {
    HRESULT hr;
    bool found = false;
    sbIDispatchPtr userPlaylist;

    // See if there is an existing entry already under the Songbird folder
    // playlist.
    long playlistCount = 0;
    for (;;) {
      hr = mSongbirdPlaylists.GetProperty(L"Count", playlistCount);
      if (COMBusyError(hr)) {
        continue;
      }
      else if (FAILED(hr)) {
        return sbError("Failed to get the number of playlists!");
      }
      else {
        break;
      }
    }

    long index = 1;
    while (index < playlistCount) {
      sbIDispatchPtr::VarArgs args(1);
      args.Append(index);
      VARIANTARG result;
      VariantInit(&result);
      hr = mSongbirdPlaylists.Invoke(L"Item",
                                     args,
                                     result,
                                     DISPATCH_PROPERTYGET);
      if (COMBusyError(hr)) {
        continue;
      }
      else if (FAILED(hr) || !IsIDispatch(result)) {
        return MakeCOMError(hr, "Getting a playlist by index");
      }

      sbIDispatchPtr curPlaylist = result.pdispVal;
      
      // Get the name of the playlist
      std::wstring listName;
      hr = curPlaylist.GetProperty(L"Name", listName);
      if (COMBusyError(hr)) {
        continue;
      }
      else if (FAILED(hr)) {
        return MakeCOMError(hr, "Trying to get the playlists name!");
      }

      if (listName.compare(aSource) == 0) {
        // Now check to see if this playlist's parent is the 
        // Songbird folder list.
        sbIDispatchPtr parentPlaylist;
        hr = curPlaylist.GetProperty(L"Parent", &parentPlaylist);
        if (COMBusyError(hr)) {
          continue;
        }
        else if (SUCCEEDED(hr)) {
          // Get the parent list, ensure that it is the Songbird folder before
          // appending it to the list of cached Songbird playlists.
          std::wstring parentListName;
          hr = parentPlaylist.GetProperty(L"Name", parentListName);
          if (SUCCEEDED(hr) &&
              parentListName.compare(miTunesFolderName) == 0)
          {
            found = true;
            userPlaylist = curPlaylist;
            break;
          }
        }
      }
    
      index++;
    }
    // If the requested source was not found in the cached playlists folder,
    // create it now.
    if (!found) {
      VARIANTARG result;
      VariantClear(&result);
      sbIDispatchPtr::VarArgs args(1);
      args.Append(aSource);
      hr = mSongbirdPlaylist.Invoke(L"CreatePlaylist", args, result);
      if (FAILED(hr)) {
        return MakeCOMError(hr, "creating a playlist");
      }

      userPlaylist = result.pdispVal;
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

sbError sbiTunesLibrary::UpdateTracks(std::deque<sbiTunesLibrary::TrackProperty> const & aTrackPaths)
{
  if (!miTunesApp.get()) {
    return sbError("iTunes objects not initialized");
  }

  HRESULT hr;
  sbIDispatchPtr tracks;

  // Loop until success
  for (;;) {
    hr = mLibraryPlaylist.GetProperty(L"Tracks", &tracks);
    if (COMBusyError(hr)) {
      continue;
    }
    else if (FAILED(hr) || !tracks.get()) {
      return sbError("Failed to get the tracks");
    }
    break;
  }

  sbError lastError = sbNoError;
  std::deque<sbiTunesLibrary::TrackProperty>::const_iterator it;
  std::deque<sbiTunesLibrary::TrackProperty>::const_iterator end = aTrackPaths.end();
  for (it = aTrackPaths.begin(); it != end; ++it) {
    for (;;) { // Loop until success
      sbIDispatchPtr::VarArgs args(2);
      args.Append(static_cast<const long>((it->first >> 32) & ULONG_MAX));
      args.Append(static_cast<const long>((it->first) & ULONG_MAX));

      sbVariantArg result = new VARIANTARG;
      if (!result) {
        lastError = sbError("failed to allocate new variantarg", true);
        // go to next item
        break;
      }
      ::VariantInit(result.get());

      hr = tracks.Invoke(L"ItemByPersistentID",
                         args,
                         *result.get(),
                         DISPATCH_PROPERTYGET);
      if (COMBusyError(hr)) {
        // retry this item
        continue;
      }
      else if (FAILED(hr) || !IsIDispatch(*result.get())) {
        lastError = sbError("Failed to get a track by persistent ID", true);
        // go to the next item
        break;
      }

      sbIDispatchPtr track = result.get()->pdispVal;
      sbVariantArg arg = new VARIANTARG;
      if (!result) {
        lastError = sbError("failed to allocate new variantarg", true);
        // go to next item
        break;
      }
      ::VariantInit(arg.get());
      arg.get()->bstrVal = ::SysAllocStringLen(it->second.c_str(),
                                               it->second.length());
      arg.get()->vt = VT_BSTR;
      if (!arg.get()->bstrVal) {
        lastError = sbError("failed to allocate BSTR for new url", true);
        // go to next item
        break;
      }

      hr = track.SetProperty(L"Location", *arg.get());
      if (COMBusyError(hr)) {
        // retry this item
        continue;
      }
      else if (FAILED(hr) || !IsIDispatch(*arg.get())) {
        lastError = sbError("Failed to set track location", true);
        // go to the next item
        break;
      }
    }
  }

  return lastError;
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
