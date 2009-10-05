/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#ifndef sbWatchFolderDefines_h_
#define sbWatchFolderDefines_h_

#include <prlog.h>


//------------------------------------------------------------------------------
// Watch folder prefs

#define PREF_WATCHFOLDER_ROOT        "songbird.watch_folder."
#define PREF_WATCHFOLDER_ENABLE      "songbird.watch_folder.enable"
#define PREF_WATCHFOLDER_PATH        "songbird.watch_folder.path"
#define PREF_WATCHFOLDER_SESSIONGUID "songbird.watch_folder.sessionguid"

//------------------------------------------------------------------------------
// Timer delays

#define STARTUP_TIMER_DELAY      3000
#define FLUSH_FS_WATCHER_DELAY   1000
#define CHANGE_DELAY_TIMER_DELAY 30000
#define EVENT_PUMP_TIMER_DELAY   1000

//------------------------------------------------------------------------------
// PRLogging module
// To log this module, set the following environment variable:
// NSPR_LOG_MODULES=sbWatchFoldersComponent:5

#ifdef PR_LOGGING
extern PRLogModuleInfo* gWatchFoldersLog;
#define TRACE(args) PR_LOG(gWatchFoldersLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gWatchFoldersLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */
#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif /* __GNUC__ */

#endif // sbWatchFolderDefines_h_

