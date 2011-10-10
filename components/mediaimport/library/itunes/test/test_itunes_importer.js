/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
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

/**
* \file  test_itunes_importer.js
* \brief Javascript source for the iTunes importer unit tests.
*/

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

function runTest() {
  // Test that the importer component is available.
  var importer;
  try {
    importer = Cc["@getnightingale.com/Nightingale/ITunesImporter;1"]
                 .getService(Ci.sbILibraryImporter);
  } catch (ex) {}
  assertTrue(importer, "iTunes importer component is not available.");
}

