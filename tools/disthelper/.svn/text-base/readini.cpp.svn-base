/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Google Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Darin Fisher <darin@meer.net>
 *  Mook <mook@songbirdnest.com>
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

#include "readini.h"

#include <stdio.h>
#include <ctype.h>

#ifdef XP_WIN
  #include <tchar.h>
#else
  #define _tfopen fopen
  #define _T(s) (s)
#endif

#include "error.h"
#include "debug.h"

#define MAX_TEXT_LEN 500

// stack based FILE wrapper to ensure that fclose is called.
class AutoFILE {
public:
  AutoFILE(FILE *fp) : fp_(fp) {}
  ~AutoFILE() { if (fp_) fclose(fp_); }
  operator FILE *() { return fp_; }
private:
  FILE *fp_;
};

int ReadIniFile(const TCHAR* path, IniFile_t& results) {
  AutoFILE fp = _tfopen(path, _T("r"));
  if (!fp) {
    LogMessage("Failed to open file %s\n", path);
    return DH_ERROR_READ;
  }
  
  std::string section;
  char buffer[MAX_TEXT_LEN + 1];
  buffer[MAX_TEXT_LEN] = '\0';
  
  while(fgets(buffer, MAX_TEXT_LEN, fp)) {
    char* p = buffer;
    // strip leading spaces
    while (*p && isspace(*p)) ++p;
    
    int len = strlen(p);
    if (!len) {
      // empty line (harmless)
      continue;
    }
    // trim CR/LF
    if (p[len - 1] == '\n') {
      --len;
    }
    if (p[len - 1] == '\r') {
      --len;
    }
    
    // look for sections
    switch(p[0]) {
      case '[':
        // start of section
        if (p[len - 1] != ']') {
          // badly formatted
          LogMessage("Line %s badly formatted\n", ConvertUTF8toUTFn(p).c_str());
          return DH_ERROR_PARSE;
        }
        section.assign(p + 1, len - 2);
        continue;
      case ';':
      case '#':
        // comment
        continue;
    }
    
    char* sep = strchr(p, '=');
    if (!sep) {
      // no separator (may want to make this mean delete?)
      LogMessage("Failed to read line %s", ConvertUTF8toUTFn(p).c_str());
      return DH_ERROR_PARSE;
    }
    *sep = '\0';
    p[len] = '\0';
    
    results[section][p] = (sep + 1);
  }

  return DH_ERROR_OK;
}
