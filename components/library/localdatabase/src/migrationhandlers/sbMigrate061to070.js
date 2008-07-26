/*
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

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/sbLocalDatabaseMigrationUtils.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const SBLDBCOMP = "@songbirdnest.com/Songbird/Library/LocalDatabase/";

function LOG(s) {
  dump("----++++----++++\nsbLocalDatabaseMigrate061to070 ---> " + 
       s + 
       "\n----++++----++++");
}

function sbLocalDatabaseMigrate061to070()
{
  sbLocalDatabaseMigrationUtils.BaseMigrationHandler.call(this);
  
  this._fromVersion = 5;
  this._toVersion   = 6;
}

//-----------------------------------------------------------------------------
// sbLocalDatabaseMigration Implementation
//-----------------------------------------------------------------------------

sbLocalDatabaseMigrate061to070.prototype = {
  __proto__: sbLocalDatabaseMigrationUtils.BaseMigrationHandler.prototype,
  
  migrate: function sbLocalDatabaseMigrate061to070_migrate(aLibrary) {
    return;
  }
}