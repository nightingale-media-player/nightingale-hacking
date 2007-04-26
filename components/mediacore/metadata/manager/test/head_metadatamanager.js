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
 * \brief Some globally useful stuff for the local database library tests
 */

function newFileURI(file) {
  var ioService = Cc["@mozilla.org/network/io-service;1"].
                  getService(Ci.nsIIOService);
  
  return ioService.newFileURI(file);
}

function newURI(spec) {
  var ioService = Cc["@mozilla.org/network/io-service;1"].
                  getService(Ci.nsIIOService);
  
  return ioService.newURI(spec, null, null);
}

function createNewLibrary(databaseGuid, databaseLocation) {

  var directory;
  if (databaseLocation) {
    directory = databaseLocation.QueryInterface(Ci.nsIFileURL).file;
  }
  else {
    directory = Cc["@mozilla.org/file/directory_service;1"].
                getService(Ci.nsIProperties).
                get("ProfD", Ci.nsIFile);
    directory.append("db");
  }
  
  var file = directory.clone();
  file.append(databaseGuid + ".db");

  var libraryFactory =
    Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/LibraryFactory;1"]
      .createInstance(Ci.sbILocalDatabaseLibraryFactory);
  var library = libraryFactory.createLibraryFromDatabase(file);
  try {
    library.clear();
  }
  catch(e) {
  }
  
  if ( library ) {
    var libraryManager = Cc["@songbirdnest.com/Songbird/library/Manager;1"].
                        getService(Ci.sbILibraryManager);
    libraryManager.registerLibrary( library );
  }
  return library;
}


function newAppRelativeFile( path ) {
  var nodes = path.split("/");
  var file = Cc["@mozilla.org/file/directory_service;1"].
                getService(Ci.nsIProperties).
                get("XREExeF", Ci.nsIFile); // Path to the executable
  file = file.parent; // Path to the executable folder
  for ( var i = 0, end = nodes.length; i < end; i++ )
  {
    file.append( nodes[ i ] );
  }
  log( "newAppRelativeFile - " + file.path );
  return file.clone();
}

