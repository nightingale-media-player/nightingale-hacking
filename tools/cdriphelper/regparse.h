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

#include <windows.h>
#include <vector>

#ifndef _REGHELPER_REGPARSE_H__
#define _REGHELPER_REGPARSE_H__

struct multiSzBuffer_t {
  LPBYTE value;
  DWORD cbBytes;
  multiSzBuffer_t(LPBYTE _v, DWORD _b)
    :value(_v), cbBytes(_b) {}
};

struct regValue_t {
  const TCHAR* p;
  size_t len; // in characters, no terminating null
  regValue_t(const TCHAR* _p, size_t _len)
    :p(_p), len(_len) {}

  // Comparison of reg keys is case-insensitive, becuase reg keys are case
  // insensitive, even if the strings we actually use to define the
  // regValue_t's have case.
  bool operator==(const regValue_t &rhs) const { 
    return (0 == _tcsicmp(p, rhs.p));
  }
};

typedef std::vector<regValue_t> regValueList_t;

regValueList_t ParseValues(const multiSzBuffer_t& data, DWORD regKeyType);
void MakeMultiSzRegString(multiSzBuffer_t& data, const regValueList_t& values); 

#endif /* _REGHELPER_REGPARSE_H__ */
