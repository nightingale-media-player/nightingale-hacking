/**
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
 */

function newAppRelativeFile( path ) {

  var file = Cc["@mozilla.org/file/directory_service;1"]
               .getService(Ci.nsIProperties)
               .get("resource:app", Ci.nsIFile);
  file = file.clone();

  var nodes = path.split("/");
  for ( var i = 0, end = nodes.length; i < end; i++ )
  {
    file.append( nodes[ i ] );
  }

  return file;
}

function runTest () {
  var timingService = Cc["@getnightingale.com/Nightingale/TimingService;1"]
                        .getService(Ci.sbITimingService);
                        
  var logFile = newAppRelativeFile("timingServiceLog.txt");
  timingService.logFile = logFile;
  
  timingService.startPerfTimer("testTimer");
  timingService.startPerfTimer("anotherTimer");
  
  sleep(1000);
  
  var actualTotalTime = timingService.stopPerfTimer("testTimer");
  var anotherTotalTime = timingService.stopPerfTimer("anotherTimer");
  
  log("Actual total time of performance timer is: " + actualTotalTime + "us");
  
  assertTrue(actualTotalTime >= 1000, 
             "welcome to the impossible! time travel!");

  return Components.results.NS_OK;
}

