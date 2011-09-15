/* vim: le=unix sw=2 : */
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

#ifndef _REGHANDLER_ERROR_H__
#define _REGHANDLER_ERROR_H__

#include "regparse.h"

struct driverLoc_t {
  enum insert_type_t { INSERT_AT_FRONT, INSERT_AFTER_NODE } insertAt;
  const regValue_t *loc;

  driverLoc_t(insert_type_t _i, const regValue_t *_l) : insertAt(_i), loc(_l) {} 
  // convenience constructor for INSERT_AT_FRONT
  driverLoc_t(insert_type_t _i) : insertAt(_i), loc(NULL) {}
};

// Helper functions
LPTSTR GetSystemErrorMessage(DWORD errNum);
BOOL LoggedSUCCEEDED(LONG rv, LPCTSTR message);
BOOL LoggedDeleteFile(LPCTSTR);

// Handlers
LONG InstallAspiDriver(void);
LONG RemoveAspiDriver(void);

// Handler support functions
LONG CheckAspiDriversInstalled(LPCTSTR installationDir);
LONG AddFilteredDriver(LPCTSTR k, const regValue_t &nk, driverLoc_t l); 
LONG AddAspiFilteredDrivers();
LONG RegisterAspiService(void);
LONG UnregisterAspiService(void);

// This records whether or not we've ever tried to install the driver for
// this particular installation path
enum install_type_t { DRIVER_INSTALLED, DRIVER_REMOVED };
LONG RecordAspiDriverInstallState(LPCTSTR path, install_type_t recordAction);

// Calling AdjustDllUseCount() with a value of 0 to query what the current key
// would be (passing result would be useful for this operation, of course)

LONG AdjustDllUseCount(LPCTSTR dllName, int value, int * result);

#endif /* _REGHANDLER_ERROR_H__ */
