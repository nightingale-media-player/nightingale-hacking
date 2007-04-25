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
 * \brief Test file
 */
 
var gLocalFiles = [
  "test1-artist-songstowearpantsto.com-.mp3",
  "test2-trackName-Trisagion-.mp3",
  "test3-album-The Singing Dictionary-.mp3"
];

var gRemoteUrls = [
  "test1-artist-songstowearpantsto.com-.mp3",
  "test2-trackName-Trisagion-.mp3",
  "test3-album-The Singing Dictionary-.mp3"
];

var gTestMatrix = [
  // "test1-artist-songstowearpantsto.com-.mp3",
  [ "http://songbirdnest.com/data/1.0#artistName", "songstowearpantsto.com" ],  
  
  // "test2-trackName-Trisagion-.mp3",
  [ "http://songbirdnest.com/data/1.0#trackName", "Trisagion" ],                
  
  // "test3-album-The Singing Dictionary-.mp3"
  [ "http://songbirdnest.com/data/1.0#album", "The Singing Dictionary" ]        
];

function runTest () {
//  log( "Run test_metadatajob.js!" );
  
  var text = "\n=============\n";
  for ( var i = 0; i < gLocalFiles.length; i++ )
  {
    text += "Local: ";
    text += gLocalFiles[ i ];
    text += "\nRemote: ";
    text += gRemoteUrls[ i ];
    text += "\nTestColumn: ";
    text += gTestMatrix[ i ][ 0 ];
    text += "\nTestValue: ";
    text += gTestMatrix[ i ][ 1 ];
    text += "\n=============\n";
  }
//  log( text );
  
  // Make a fake library
  
  // Add gLocalFiles to it
  
  // Add gRemoteUrls to it
  
  // Get an array of sbMediaItem
  
  // Request metadata
  
  // Start a timer to wait for the metadata to complete
}

function onTimer () {

  // See if all of the metadata is complete
  
  // When it is, check the values and kill the timer
  
  // So testing is complete
}