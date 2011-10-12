/* vim: le=unix sw=2 : */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Songbird Distribution Stub Helper.
 *
 * The Initial Developer of the Original Code is
 * POTI <http://www.songbirdnest.com/>.
 * Portions created by the Initial Developer are Copyright (C) 2008
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Mook <mook@songbirdnest.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef _DISTHERLPER_READINI_H__
#define _DISTHERLPER_READINI_H__

/* Yet another INI parser, because the one in toolkit/mozapps/update/
 * doesn't support sections */

#ifdef XP_WIN
  #include <tchar.h>
#else
  #define TCHAR char
#endif

#include <map>
#include <string>
#include <algorithm>

#include <stringconvert.h>
#include <debug.h>

/* all strings are assumed to be UTF-8 */
class LowerCaseCompare {
public:
  bool operator()(const std::string& aLeft, const std::string& aRight) const {
    return lexicographical_compare(aLeft.begin(), aLeft.end(),
                                   aRight.begin(), aRight.end(),
                                   LowerCaseCompare());
  }
  /* note: this is not expected to deal with more than simple English
     (unaccented Latin alphabet) */
  bool operator()(const char& aLeft, const char& aRight) const {
    return tolower(aLeft) < tolower(aRight);
  }
};
typedef std::map<std::string, std::string, LowerCaseCompare> IniEntry_t;
typedef std::map<std::string, IniEntry_t, LowerCaseCompare> IniFile_t;

class VersionLessThan {
public:
  bool operator()(const std::string& aLeft, const std::string& aRight) const {
    std::string::const_iterator leftBegin = aLeft.begin(), leftEnd,
                                rightBegin = aRight.begin(), rightEnd;
    const char *leftData = aLeft.c_str(),
               *rightData = aRight.c_str();
    do {
      unsigned long leftVal, rightVal;
      const char *leftPart = leftData + distance(aLeft.begin(), leftBegin),
                 *rightPart = rightData + distance(aRight.begin(), rightBegin);
      leftVal = strtoul(leftPart, NULL, 10);
      rightVal = strtoul(rightPart, NULL, 10);
      DebugMessage("version parts: %s (%lu) / %s (%lu)",
                   ConvertUTF8toUTFn(leftPart).c_str(),
                   leftVal,
                   ConvertUTF8toUTFn(rightPart).c_str(),
                   rightVal);
      if (leftVal != rightVal) {
        return (leftVal < rightVal);
      }
      leftEnd = find(leftBegin, aLeft.end(), '.');
      rightEnd = find(rightBegin, aRight.end(), '.');
      if (leftEnd != aLeft.end()) {
        leftBegin = ++leftEnd;
      } else {
        leftBegin = leftEnd;
      }
      if (rightEnd != aRight.end()) {
        rightBegin = ++rightEnd;
      } else {
        rightBegin = rightEnd;
      }
    } while (leftEnd != aLeft.end() || rightEnd != aRight.end());
    DebugMessage("EOF");
    return false;
  }
};

/**
 * This function reads in entries in a .ini file
 * returns DH_ERROR_OK on success, non-zero on failure (see error.h)
 */
int ReadIniFile(const TCHAR* path, IniFile_t& results);

#endif /* _DISTHERLPER_READINI_H__ */
