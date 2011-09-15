/* vim: set sw=2 :miv */
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

#include <tchar.h>
#include "regparse.h"

regValueList_t ParseValues(const multiSzBuffer_t& data, DWORD aRegType) {
  regValueList_t values;

  const TCHAR* p = reinterpret_cast<const TCHAR*>(data.value);
  size_t bytes = static_cast<size_t>(data.cbBytes);
  size_t count = bytes / sizeof(TCHAR);
  
  if (aRegType == REG_SZ || aRegType == REG_EXPAND_SZ) {
    values.push_back(regValue_t(p, count - 1));
  } else if (aRegType == REG_MULTI_SZ ) {
    while (count > 0) {
      size_t len = _tcsnlen(p, count);
      if (!len) {
        // empty, meaning end of strings
        break;
      }
      values.push_back(regValue_t(p, len));
      p += len + 1; // include null terminator
    }
  }
  
  return values;
}

void MakeMultiSzRegString(multiSzBuffer_t& data, const regValueList_t& values) {
  size_t bytesLeft = static_cast<size_t>(data.cbBytes);
  TCHAR* p = reinterpret_cast<TCHAR*>(data.value);
  for (regValueList_t::const_iterator it = values.begin();
   it != values.end(); ++it) {
    size_t bytes = it->len * sizeof(TCHAR);
    if (bytesLeft < bytes) {
      // BAIL BAIL BAIL
      break;
    }
    memcpy(p, it->p, bytes);
    p += it->len;
    *p++ = 0;
    bytesLeft -= bytes + sizeof(TCHAR);
  }
  // add the null to terminate the whole list
  *p++ = 0;

  // remember to report the byte count back out
  data.cbBytes = reinterpret_cast<LPBYTE>(p) - data.value;
}
