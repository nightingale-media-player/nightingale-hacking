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

LPTSTR GetSystemErrorMessage(DWORD errNum);
BOOL LoggedSUCCEEDED(LONG rv, LPCTSTR message);
BOOL LoggedDeleteFile(LPCTSTR);
int InstallAspiDriver(void);
int RemoveAspiDriver(void);
int AddFilteredDriver(LPCTSTR k, const regValue_t &nk, driverLoc_t l); 
int IncrementDriverInstallationCount(void);
LONG CheckAspiDriversInstalled(void);

#endif /* _REGHANDLER_ERROR_H__ */
