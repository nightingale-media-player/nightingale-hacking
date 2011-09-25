/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

/**
* \file  test_itunes_xml_parser.js
* \brief JavaScript source for the iTunes XML Parser unit tests.
*/

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

/**
 * Chopped version of an iTunes XML file to test error handling
 */

badTestXML =  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
badTestXML += "<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
badTestXML += "<plist version=\"1.0\">\n";
badTestXML += "<dict>\n";
badTestXML += "    <key>Major Version</key><integer>1</integer>\n";
badTestXML += "    <key>Minor Version</key><integer>1</integer>\n";
badTestXML += "    <key>Application Version</key><string>8.1</string>\n";

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// iTunes importer unit tests.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * Run the unit tests.
 */

function stringToStream(aString) {
  stream = 
    Cc["@mozilla.org/io/string-input-stream;1"]
      .createInstance(Ci.nsIStringInputStream);
  stream.setData(aString, -1);
  return stream;
}

function runTest() {
  /**
   * Listener used to track the events
   */
  var listener = {
    expectedTopLevelProperties : [ "Major Version",
                                   "Minor Version",
                                   "Application Version",
                                   "Features",
                                   "Show Content Ratings",
                                   "Music Folder",
                                   "Library Persistent ID" ],
    onTopLevelProperties : function(aProperties) {
      var length = this.expectedTopLevelProperties.length;
      this.topLevelPropertyCount = 0;
      for (var index =0; index < length; ++index) {
        try {
          if (aProperties.get(this.expectedTopLevelProperties[index]).length > 0) {
            ++this.topLevelPropertyCount;
          }
        }
        catch (e) {
        }
      }
    },
    onTrack : function(aProperties) {
      ++this.onTrackCount;
    },
    onTracksComplete : function() {
      ++this.onTracksCompleteCount;
    },
    onPlaylist : function(aProperties, tracks) {
      ++this.onPlaylistCount;
    },
    onPlaylistsComplete : function () {
      ++this.onPlaylistsCompleteCount;
      fail("onPlaylistsComplete called when an error should occur");
    },
    onError : function(aErrorMessage) {
      ++this.onErrorCount;
      assertEqual(this.topLevelPropertyCount, 0, "Top level properties do not match");
      assertEqual(this.onTrackCount, 0, "Invalid track count for error situation");
      assertEqual(this.onTracksCompleteCount, 0, "Invalid track complete count for error situation");
      assertEqual(this.onPlaylistCount, 0, "Invalid playlist count for error situation");
      assertEqual(this.onPlaylistsCompleteCount, 0, "Invalid playlist complete count for error situation");
      assertEqual(this.onErrorCount, 1, "Invalid error count for error situation");
      parser.finalize();
      testFinished();
    },
    onProgress : function() { /* nothing */ },
    topLevelPropertyCount : 0,
    onTrackCount : 0,
    onTracksCompleteCount : 0,
    onPlaylistCount : 0,
    onPlaylistsCompleteCount : 0,
    onErrorCount : 0,
    
    QueryInterface: XPCOMUtils.generateQI([Ci.sbIiTunesXMLParserListener])
  };    

  // Test error condition
  var parser = Cc["@getnightingale.com/Nightingale/sbiTunesXMLParser;1"]
                 .getService(Ci.sbIiTunesXMLParser);
  assertTrue(parser, "iTunes importer component is not available.");
  parser.parse(stringToStream(badTestXML), listener);
  testPending();
  parser = null; // drop the reference to prevent leaks
}
