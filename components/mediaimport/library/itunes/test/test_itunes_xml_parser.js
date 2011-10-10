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
 * Sample of an iTunes xml file
 * I was too lazy to figure out how to use and distribute a 
 * a real xml file with the test.
 */
testXML =  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
testXML += "<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
testXML += "<plist version=\"1.0\">\n";
testXML += "<dict>\n";
testXML += "    <key>Major Version</key><integer>1</integer>\n";
testXML += "    <key>Minor Version</key><integer>1</integer>\n";
testXML += "    <key>Application Version</key><string>8.1</string>\n";
testXML += "    <key>Features</key><integer>5</integer>\n";
testXML += "    <key>Show Content Ratings</key><true/>\n";
testXML += "    <key>Music Folder</key><string>file://localhost/C:/Users/dbradley/Music/iTunes/iTunes%20Music/</string>\n";
testXML += "    <key>Library Persistent ID</key><string>4901D1E2BA4B2D9B</string>\n";
testXML += "    <key>Tracks</key>\n";
testXML += "    <dict>\n";
testXML += "        <key>60</key>\n";
testXML += "        <dict>\n";
testXML += "            <key>Track ID</key><integer>60</integer>\n";
testXML += "            <key>Name</key><string>ESRB</string>\n";
testXML += "            <key>Kind</key><string>QuickTime movie file</string>\n";
testXML += "            <key>Size</key><integer>2306052</integer>\n";
testXML += "            <key>Total Time</key><integer>6000</integer>\n";
testXML += "            <key>Date Modified</key><date>2007-02-12T15:53:10Z</date>\n";
testXML += "            <key>Date Added</key><date>2009-02-13T03:04:07Z</date>\n";
testXML += "            <key>Artwork Count</key><integer>1</integer>\n";
testXML += "            <key>Persistent ID</key><string>042FEFDD9488F280</string>\n";
testXML += "            <key>Track Type</key><string>File</string>\n";
testXML += "            <key>Has Video</key><true/>\n";
testXML += "            <key>HD</key><false/>\n";
testXML += "            <key>Movie</key><true/>\n";
testXML += "            <key>Location</key><string>file://localhost/C:/Users/dbradley/Music/ESRB.mpg</string>\n";
testXML += "            <key>File Folder Count</key><integer>-1</integer>\n";
testXML += "            <key>Library Folder Count</key><integer>-1</integer>\n";
testXML += "        </dict>\n";
testXML += "        <key>62</key>\n";
testXML += "        <dict>\n";
testXML += "            <key>Track ID</key><integer>62</integer>\n";
testXML += "            <key>Name</key><string>The Swarm</string>\n";
testXML += "            <key>Artist</key><string>Noble Society</string>\n";
testXML += "            <key>Album Artist</key><string>Various Artists</string>\n";
testXML += "            <key>Album</key><string>The Swarm Riddim</string>\n";
testXML += "            <key>Genre</key><string>Reggae</string>\n";
testXML += "            <key>Kind</key><string>Purchased AAC audio file</string>\n";
testXML += "            <key>Size</key><integer>8118815</integer>\n";
testXML += "            <key>Total Time</key><integer>240160</integer>\n";
testXML += "            <key>Disc Number</key><integer>1</integer>\n";
testXML += "            <key>Disc Count</key><integer>1</integer>\n";
testXML += "            <key>Track Number</key><integer>1</integer>\n";
testXML += "            <key>Track Count</key><integer>13</integer>\n";
testXML += "            <key>Year</key><integer>2007</integer>\n";
testXML += "            <key>Date Modified</key><date>2008-01-29T08:58:06Z</date>\n";
testXML += "            <key>Date Added</key><date>2009-02-13T03:33:07Z</date>\n";
testXML += "            <key>Bit Rate</key><integer>256</integer>\n";;
testXML += "            <key>Sample Rate</key><integer>44100</integer>\n";;
testXML += "            <key>Release Date</key><date>2007-09-18T07:00:00Z</date>\n";
testXML += "            <key>Compilation</key><true/>\n";
testXML += "            <key>Artwork Count</key><integer>1</integer>\n";
testXML += "            <key>Sort Album</key><string>Swarm Riddim</string>\n";
testXML += "            <key>Sort Name</key><string>Swarm</string>\n";
testXML += "            <key>Persistent ID</key><string>D948F4FD9537100C</string>\n";
testXML += "            <key>Explicit</key><true/>\n";
testXML += "            <key>Track Type</key><string>File</string>\n";
testXML += "            <key>Purchased</key><true/>\n";
testXML += "            <key>Location</key><string>file://localhost/C:/Users/dbradley/Music/01%20The%20Swarm.m4a</string>\n";
testXML += "            <key>File Folder Count</key><integer>-1</integer>\n";
testXML += "            <key>Library Folder Count</key><integer>-1</integer>\n";
testXML += "        </dict>\n";
testXML += "        <key>80</key>\n";
testXML += "        <dict>\n";
testXML += "            <key>Track ID</key><integer>80</integer>\n";
testXML += "            <key>Name</key><string>Analog Girl</string>\n";
testXML += "            <key>Artist</key><string>Guy Clark</string>\n";
testXML += "            <key>Album Artist</key><string>Guy Clark</string>\n";
testXML += "            <key>Album</key><string>Workbench Songs</string>\n";
testXML += "            <key>Genre</key><string>2</string>\n";
testXML += "            <key>Kind</key><string>MPEG audio file</string>\n";
testXML += "            <key>Size</key><integer>2052686</integer>\n";
testXML += "            <key>Total Time</key><integer>51095</integer>\n";
testXML += "            <key>Year</key><integer>2006</integer>\n";
testXML += "            <key>Date Modified</key><date>2009-02-13T03:29:16Z</date>\n";
testXML += "            <key>Date Added</key><date>2009-02-13T03:33:08Z</date>\n";
testXML += "            <key>Bit Rate</key><integer>320</integer>\n";
testXML += "            <key>Sample Rate</key><integer>44100</integer>\n";
testXML += "            <key>Persistent ID</key><string>D948F4FD95371018</string>\n";
testXML += "            <key>Track Type</key><string>File</string>\n";
testXML += "            <key>Location</key><string>file://localhost/C:/Users/dbradley/Music/Guy-Clark-Analog-Girl.mp3</string>\n";
testXML += "            <key>File Folder Count</key><integer>-1</integer>\n";
testXML += "            <key>Library Folder Count</key><integer>-1</integer>\n";
testXML += "        </dict>\n";
testXML += "    </dict>\n";
testXML += "    <key>Playlists</key>\n";
testXML += "    <array>\n";
testXML += "        <dict>\n";
testXML += "            <key>Name</key><string>Library</string>\n";
testXML += "            <key>Master</key><true/>\n";
testXML += "            <key>Playlist ID</key><integer>126</integer>\n";
testXML += "            <key>Playlist Persistent ID</key><string>4901D1E2BA4B2D9C</string>\n";
testXML += "            <key>Visible</key><false/>\n";
testXML += "            <key>All Items</key><true/>\n";
testXML += "            <key>Playlist Items</key>\n";
testXML += "            <array>\n";
testXML += "                <dict>\n";
testXML += "                    <key>Track ID</key><integer>80</integer>\n";
testXML += "                </dict>\n";
testXML += "                <dict>\n";
testXML += "                    <key>Track ID</key><integer>62</integer>\n";
testXML += "                </dict>\n";
testXML += "                <dict>\n";
testXML += "                    <key>Track ID</key><integer>60</integer>\n";
testXML += "                </dict>\n";
testXML += "            </array>\n";
testXML += "        </dict>\n";
testXML += "        <dict>\n";
testXML += "            <key>Name</key><string>Music</string>\n";
testXML += "            <key>Playlist ID</key><integer>194</integer>\n";
testXML += "            <key>Playlist Persistent ID</key><string>4901D1E2BA4B2DA6</string>\n";
testXML += "            <key>Distinguished Kind</key><integer>4</integer>\n";
testXML += "            <key>Music</key><true/>\n";
testXML += "            <key>All Items</key><true/>\n";
testXML += "            <key>Smart Info</key>\n";
testXML += "            <data>\n";
testXML += "            AQEAAwAAAAIAAAAZAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAA==\n";
testXML += "            </data>\n";
testXML += "            <key>Smart Criteria</key>\n";
testXML += "            <data>\n";
testXML += "            U0xzdAABAAEAAAABAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADwAAAQAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABEAAAAAAABIbEAAAAAAAAAAAAAAAAAAAAB\n";
testXML += "            AAAAAAABIbEAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAAA=\n";
testXML += "            </data>\n";
testXML += "            <key>Playlist Items</key>\n";
testXML += "            <array>\n";
testXML += "                <dict>\n";
testXML += "                    <key>Track ID</key><integer>80</integer>\n";
testXML += "                </dict>\n";
testXML += "                <dict>\n";
testXML += "                    <key>Track ID</key><integer>62</integer>\n";
testXML += "                </dict>\n";
testXML += "            </array>\n";
testXML += "        </dict>\n";
testXML += "        <dict>\n";
testXML += "            <key>Name</key><string>Movies</string>\n";
testXML += "            <key>Playlist ID</key><integer>229</integer>\n";
testXML += "            <key>Playlist Persistent ID</key><string>4901D1E2BA4B2DA7</string>\n";
testXML += "            <key>Distinguished Kind</key><integer>2</integer>\n";
testXML += "            <key>Movies</key><true/>\n";
testXML += "            <key>All Items</key><true/>\n";
testXML += "            <key>Smart Info</key>\n";
testXML += "            <data>\n";
testXML += "            AQEAAwAAAAIAAAAZAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAA==\n";
testXML += "            </data>\n";
testXML += "            <key>Smart Criteria</key>\n";
testXML += "            <data>\n";
testXML += "            U0xzdAABAAEAAAABAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADwAAAgAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABEAAAAAAAAgAYAAAAAAAAAAAAAAAAAAAAB\n";
testXML += "            AAAAAAAAAAIAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAAA=\n";
testXML += "            </data>\n";
testXML += "            <key>Playlist Items</key>\n";
testXML += "            <array>\n";
testXML += "                <dict>\n";
testXML += "                    <key>Track ID</key><integer>60</integer>\n";
testXML += "                </dict>\n";
testXML += "            </array>\n";
testXML += "        </dict>\n";
testXML += "        <dict>\n";
testXML += "            <key>Name</key><string>TV Shows</string>\n";
testXML += "            <key>Playlist ID</key><integer>233</integer>\n";
testXML += "            <key>Playlist Persistent ID</key><string>4901D1E2BA4B2DA8</string>\n";
testXML += "            <key>Distinguished Kind</key><integer>3</integer>\n";
testXML += "            <key>TV Shows</key><true/>\n";
testXML += "            <key>All Items</key><true/>\n";
testXML += "            <key>Smart Info</key>\n";
testXML += "            <data>\n";
testXML += "            AQEAAwAAAAIAAAAZAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAA==\n";
testXML += "            </data>\n";
testXML += "            <key>Smart Criteria</key>\n";
testXML += "            <data>\n";
testXML += "            U0xzdAABAAEAAAABAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADwAAAgAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABEAAAAAAAAgEQAAAAAAAAAAAAAAAAAAAAB\n";
testXML += "            AAAAAAAAAEAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAAA=\n";
testXML += "            </data>\n";
testXML += "        </dict>\n";
testXML += "        <dict>\n";
testXML += "            <key>Name</key><string>Podcasts</string>\n";
testXML += "            <key>Playlist ID</key><integer>187</integer>\n";
testXML += "            <key>Playlist Persistent ID</key><string>4901D1E2BA4B2DA5</string>\n";
testXML += "            <key>Distinguished Kind</key><integer>10</integer>\n";
testXML += "            <key>Podcasts</key><true/>\n";
testXML += "            <key>All Items</key><true/>\n";
testXML += "        </dict>\n";
testXML += "        <dict>\n";
testXML += "            <key>Name</key><string>Audiobooks</string>\n";
testXML += "            <key>Playlist ID</key><integer>236</integer>\n";
testXML += "            <key>Playlist Persistent ID</key><string>4901D1E2BA4B2DA9</string>\n";
testXML += "            <key>Distinguished Kind</key><integer>5</integer>\n";
testXML += "            <key>Audiobooks</key><true/>\n";
testXML += "            <key>All Items</key><true/>\n";
testXML += "            <key>Smart Info</key>\n";
testXML += "            <data>\n";
testXML += "            AQEAAwAAAAIAAAAZAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAA==\n";
testXML += "            </data>\n";
testXML += "            <key>Smart Criteria</key>\n";
testXML += "            <data>\n";
testXML += "            U0xzdAABAAEAAAABAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADwAAAgAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABEAAAAAAAAgAwAAAAAAAAAAAAAAAAAAAAB\n";
testXML += "            AAAAAAAAAAgAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAAA=\n";
testXML += "            </data>\n";
testXML += "        </dict>\n";
testXML += "        <dict>\n";
testXML += "            <key>Name</key><string>iTunes DJ</string>\n";
testXML += "            <key>Playlist ID</key><integer>184</integer>\n";
testXML += "            <key>Playlist Persistent ID</key><string>4901D1E2BA4B2DA3</string>\n";
testXML += "            <key>Distinguished Kind</key><integer>22</integer>\n";
testXML += "            <key>Party Shuffle</key><true/>\n";
testXML += "            <key>All Items</key><true/>\n";
testXML += "        </dict>\n";
testXML += "        <dict>\n";
testXML += "            <key>Name</key><string>Genius</string>\n";
testXML += "            <key>Playlist ID</key><integer>245</integer>\n";
testXML += "            <key>Playlist Persistent ID</key><string>4901D1E2BA4B2DAC</string>\n";
testXML += "            <key>Distinguished Kind</key><integer>26</integer>\n";
testXML += "            <key>All Items</key><true/>\n";
testXML += "        </dict>\n";
testXML += "        <dict>\n";
testXML += "            <key>Name</key><string>90's Music</string>\n";
testXML += "            <key>Playlist ID</key><integer>162</integer>\n";
testXML += "            <key>Playlist Persistent ID</key><string>4901D1E2BA4B2D9D</string>\n";
testXML += "            <key>All Items</key><true/>\n";
testXML += "            <key>Smart Info</key>\n";
testXML += "            <data>\n";
testXML += "            AQEAAwAAAAIAAAAZAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAA==\n";
testXML += "            </data>\n";
testXML += "            <key>Smart Criteria</key>\n";
testXML += "            <data>\n";
testXML += "            U0xzdAABAAEAAAACAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAcAAAEAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABEAAAAAAAAB8YAAAAAAAAAAAAAAAAAAAAB\n";
testXML += "            AAAAAAAAB88AAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA5AgAAAQAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAARAAAAAAAAAAB\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAQAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAA\n";
testXML += "            </data>\n";
testXML += "        </dict>\n";
testXML += "        <dict>\n";
testXML += "            <key>Name</key><string>Music Videos</string>\n";
testXML += "            <key>Playlist ID</key><integer>181</integer>\n";
testXML += "            <key>Playlist Persistent ID</key><string>4901D1E2BA4B2DA2</string>\n";
testXML += "            <key>All Items</key><true/>\n";
testXML += "            <key>Smart Info</key>\n";
testXML += "            <data>\n";
testXML += "            AQEAAwAAAAIAAAAZAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAA==\n";
testXML += "            </data>\n";
testXML += "            <key>Smart Criteria</key>\n";
testXML += "            <data>\n";
testXML += "            U0xzdAABAAEAAAABAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADwAAAQAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABEAAAAAAAAACAAAAAAAAAAAAAAAAAAAAAB\n";
testXML += "            AAAAAAAAACAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAAA=\n";
testXML += "            </data>\n";
testXML += "        </dict>\n";
testXML += "        <dict>\n";
testXML += "            <key>Name</key><string>My Top Rated</string>\n";
testXML += "            <key>Playlist ID</key><integer>165</integer>\n";
testXML += "            <key>Playlist Persistent ID</key><string>4901D1E2BA4B2D9E</string>\n";
testXML += "            <key>All Items</key><true/>\n";
testXML += "            <key>Smart Info</key>\n";
testXML += "            <data>\n";
testXML += "            AQEAAwAAAAIAAAAZAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAA==\n";
testXML += "            </data>\n";
testXML += "            <key>Smart Criteria</key>\n";
testXML += "            <data>\n";
testXML += "            U0xzdAABAAEAAAABAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABkAAAAQAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABEAAAAAAAAADwAAAAAAAAAAAAAAAAAAAAB\n";
testXML += "            AAAAAAAAADwAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAAA=\n";
testXML += "            </data>\n";
testXML += "        </dict>\n";
testXML += "        <dict>\n";
testXML += "            <key>Name</key><string>Recently Added</string>\n";
testXML += "            <key>Playlist ID</key><integer>174</integer>\n";
testXML += "            <key>Playlist Persistent ID</key><string>4901D1E2BA4B2DA1</string>\n";
testXML += "            <key>All Items</key><true/>\n";
testXML += "            <key>Smart Info</key>\n";
testXML += "            <data>\n";
testXML += "            AQEAAwAAAAIAAAAZAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAA==\n";
testXML += "            </data>\n";
testXML += "            <key>Smart Criteria</key>\n";
testXML += "            <data>\n";
testXML += "            U0xzdAABAAEAAAACAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAIAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABELa4tri2uLa7//////////gAAAAAACTqA\n";
testXML += "            La4tri2uLa4AAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA5AgAAAQAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAARAAAAAAAAAAB\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAQAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAA\n";
testXML += "            </data>\n";
testXML += "            <key>Playlist Items</key>\n";
testXML += "            <array>\n";
testXML += "            </array>\n";
testXML += "        </dict>\n";
testXML += "        <dict>\n";
testXML += "            <key>Name</key><string>Recently Played</string>\n";
testXML += "            <key>Playlist ID</key><integer>171</integer>\n";
testXML += "            <key>Playlist Persistent ID</key><string>4901D1E2BA4B2DA0</string>\n";
testXML += "            <key>All Items</key><true/>\n";
testXML += "            <key>Smart Info</key>\n";
testXML += "            <data>\n";
testXML += "            AQEAAwAAAAIAAAAZAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAA==\n";
testXML += "            </data>\n";
testXML += "            <key>Smart Criteria</key>\n";
testXML += "            <data>\n";
testXML += "            U0xzdAABAAEAAAACAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABcAAAIAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABELa4tri2uLa7//////////gAAAAAACTqA\n";
testXML += "            La4tri2uLa4AAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA5AgAAAQAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAARAAAAAAAAAAB\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAQAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAA\n";
testXML += "            </data>\n";
testXML += "        </dict>\n";
testXML += "        <dict>\n";
testXML += "            <key>Name</key><string>Top 25 Most Played</string>\n";
testXML += "            <key>Playlist ID</key><integer>168</integer>\n";
testXML += "            <key>Playlist Persistent ID</key><string>4901D1E2BA4B2D9F</string>\n";
testXML += "            <key>All Items</key><true/>\n";
testXML += "            <key>Smart Info</key>\n";
testXML += "            <data>\n";
testXML += "            AQEBAwAAABkAAAAZAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAA==\n";
testXML += "            </data>\n";
testXML += "            <key>Smart Criteria</key>\n";
testXML += "            <data>\n";
testXML += "            U0xzdAABAAEAAAACAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADkCAAABAAAAAAAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABEAAAAAAAAAAEAAAAAAAAAAAAAAAAAAAAB\n";
testXML += "            AAAAAAAAAAEAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAWAAAAEAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAARAAAAAAAAAAA\n";
testXML += "            AAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAA\n";
testXML += "            AAAAAAAA\n";
testXML += "            </data>\n";
testXML += "        </dict>\n";
testXML += "    </array>\n";
testXML += "</dict>\n";
testXML += "</plist>\n";

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
      assertEqual(this.topLevelPropertyCount, 7, "Top level properties do not match");
      assertEqual(this.onTrackCount, 2, "Invalid track count");
      assertEqual(this.onTracksCompleteCount, 1, "Invalid track complete count");
      assertEqual(this.onPlaylistCount, 14, "Invalid playlist count");
      assertEqual(this.onPlaylistsCompleteCount, 1, "Invalid playlist complete count");
      assertEqual(this.onErrorCount, 0, "Invalid error count");
      testFinished();
    },
    onError : function(aErrorMessage) {
      ++this.onErrorCount;
      fail("onError called on non-error test case");
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

  // Test normal xml data
  parser = Cc["@getnightingale.com/Nightingale/sbiTunesXMLParser;1"]
             .getService(Ci.sbIiTunesXMLParser);
  assertTrue(parser, "iTunes importer component is not available.");
  parser.parse(stringToStream(testXML), listener);
  testPending();
  parser = null; // drop the reference to prevent leaks
}
