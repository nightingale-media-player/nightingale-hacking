/**
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Ce = Components.Exception;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/RDFHelper.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");


/**
 * Object implementing sbIDirectoryImportJob, responsible
 * for finding media items on disk, adding them to the library
 * and performing a metadata scan.
 */
function DirectoryImportJob() {

}
DirectoryImportJob.prototype = {
  QueryInterface:   XPCOMUtils.generateQI([Ci.sbIDirectoryImportJob]),
  
  _totalAddedToMediaList:     -1,
  _totalImportedToLibrary:    -1,
  _totalDuplicates:           -1
}


/**
 * Object implementing sbIDirectoryImportService. Used to start a 
 * new media import job.
 */
function DirectoryImportService() {
}

DirectoryImportService.prototype = {
  classDescription: "Songbird Directory Import Service",
  classID:          Components.ID("{6e542f90-44a0-11dd-ae16-0800200c9a66}"),
  contractID:       "@songbirdnest.com/Songbird/DirectoryImportService;1",
  QueryInterface:   XPCOMUtils.generateQI([Ci.sbIDirectoryImportService]),
  
  /**
   * \brief Import any media files found in the given directories to 
   *        the specified media list.
   */
  import: function DirectoryImportService_import(aDirectoryArray, aTargetMediaList, aTargetIndex) {

    // TODO move all of mediascan.js here,
    // TODO hook up jobProgress.xul

    return new DirectoryImportJob();
  },

} // DirectoryImportService.prototype


function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([DirectoryImportService]);
}
