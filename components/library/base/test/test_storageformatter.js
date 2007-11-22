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

function runTest () {

  Components.utils.import("resource://app/components/sbStorageFormatter.jsm");

  assertEqual(StorageFormatter.shortUnits[StorageFormatter.B], "B");
  assertEqual(StorageFormatter.shortUnits[StorageFormatter.KB], "KB");
  assertEqual(StorageFormatter.shortUnits[StorageFormatter.MB], "MB");
  assertEqual(StorageFormatter.shortUnits[StorageFormatter.GB], "GB");
  assertEqual(StorageFormatter.longUnits[StorageFormatter.B], "bytes");
  assertEqual(StorageFormatter.longUnits[StorageFormatter.KB], "kilobytes");
  assertEqual(StorageFormatter.longUnits[StorageFormatter.MB], "megabytes");
  assertEqual(StorageFormatter.longUnits[StorageFormatter.GB], "gigabytes");

  assertEqual(StorageFormatter.format(0), "0 B");
  assertEqual(StorageFormatter.format(100), "100 B");
  assertEqual(StorageFormatter.format(1000), "1000 B");
  assertEqual(StorageFormatter.format(1024), "1 KB");
  assertEqual(StorageFormatter.format(1126), "1.1 KB");
  assertEqual(StorageFormatter.format(1048576), "1 MB");
  assertEqual(StorageFormatter.format(1572864), "1.5 MB");
  assertEqual(StorageFormatter.format(3889201152), "3.6 GB");
  assertEqual(StorageFormatter.format(42949672960), "40 GB");
}

