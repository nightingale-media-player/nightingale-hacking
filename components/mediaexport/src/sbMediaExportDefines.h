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

#ifndef sbMediaExportDefines_h_
#define sbMediaExportDefines_h_

#include <prlog.h>


//------------------------------------------------------------------------------
// Import/Export preference constants

#define PREF_IMPORTEXPORT_ONSHUTDOWN \
  "songbird.library_importexport.autoshutdown"
#define PREF_IMPORTEXPORT_ONSTARTUP  \
  "songbird.library_importexport.autostartup"
#define PREF_EXPORT_TRACKS \
  "songbird.library_exporter.export_tracks"
#define PREF_EXPORT_PLAYLISTS \
  "songbird.library_exporter.export_playlists"
#define PREF_EXPORT_SMARTPLAYLISTS \
  "songbird.library_exporter.export_smartplaylists"


//------------------------------------------------------------------------------
// Media export service XPCOM info

#define SONGBIRD_MEDIAEXPORTSERVICE_CONTRACTID            \
  "@songbirdnest.com/media-export-component;1"
#define SONGBIRD_MEDIAEXPORTSERVICE_CLASSNAME             \
  "Songbird Media Export Service" 
#define SONGBIRD_MEDIAEXPORTSERVICE_CID                   \
{ /* 7DD185B9-F91E-473D-82DC-65060802D091 */              \
  0x7DD185B9,                                             \
  0xF91E,                                                 \
  0x473D,                                                 \
  {0x82, 0xDC, 0x65, 0x06, 0x08, 0x02, 0xD0, 0x91}        \
}

//------------------------------------------------------------------------------
// To log this module, set the following environment variable:
//   NSPR_LOG_MODULES=sbMediaExportService:5

#ifdef PR_LOGGING
extern PRLogModuleInfo* gMediaExportLog;
#define TRACE(args) PR_LOG(gMediaExportLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gMediaExportLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */
#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif /* __GNUC__ */


#endif  // sbMediaExportDefines_h_

