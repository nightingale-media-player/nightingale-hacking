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

#ifndef sbMediaExportDefines_h_
#define sbMediaExportDefines_h_

#include <prlog.h>
#include <nsStringAPI.h>
#include <nsAppDirectoryServiceDefs.h>
#include <list>
#include <set>
 
typedef std::list<nsString>                 sbStringList;
typedef sbStringList::const_iterator        sbStringListIter;

typedef std::set<nsString>                  sbStringSet;
typedef sbStringSet::const_iterator         sbStringSetIter;

//------------------------------------------------------------------------------
// Import/Export preference constants

#define PREF_EXPORT_TRACKS \
  "songbird.library_exporter.export_tracks"
#define PREF_EXPORT_PLAYLISTS \
  "songbird.library_exporter.export_playlists"
#define PREF_EXPORT_SMARTPLAYLISTS \
  "songbird.library_exporter.export_smartplaylists"
#define PREF_EXPORT_STARTAGENT \
  "songbird.library_exporter.start_agent"

//------------------------------------------------------------------------------
// Misc constants

const PRUint32 LISTENER_NOTIFY_ITEM_DELTA = 10;

//------------------------------------------------------------------------------
// Task file constants
 
//
// NOTE: When changing these values, please reflect the changes in the unittest!
//
#define TASKFILE_NAME                        "songbird_export.task"
#define TASKFILE_SCHEMAVERSION               "2"
#define TASKFILE_SCHEMAVERSION_HEADER        "schema-version"
#define TASKFILE_ADDEDMEDIALISTS_HEADER      "added-medialists"
#define TASKFILE_REMOVEDMEDIALISTS_HEADER    "removed-medialists"
#define TASKFILE_ADDEDMEDIAITEMS_HEADER      "added-mediaitems"
#define TASKFILE_UPDATEDMEDIAITEMS_HEADER    "updated-mediaitems"
#define TASKFILE_UPDATEDSMARTPLAYLIST_HEADER "updated-smartplaylist"
#define SHUTDOWN_NAME                        "songbird_export.shutdown"

// Sentinel value used in task file to indicate items added to the main library
// If you change this make sure you update it in:
//    tools/itunesagent/macosx/sbiTunesAgentMacProcessor.h
//    tools/itunesagent/windows/sbiTunesLibrary.h
#define SONGBIRD_MAIN_LIBRARY_NAME "#####SONGBIRD_MAIN_LIBRRAY#####"

//------------------------------------------------------------------------------
// Media export service XPCOM info

#define SONGBIRD_MEDIAEXPORTSERVICE_CID                   \
{ /* 7DD185B9-F91E-473D-82DC-65060802D091 */              \
  0x7DD185B9,                                             \
  0xF91E,                                                 \
  0x473D,                                                 \
  {0x82, 0xDC, 0x65, 0x06, 0x08, 0x02, 0xD0, 0x91}        \
}

#endif  // sbMediaExportDefines_h_

