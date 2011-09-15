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

#include "stringconvert.h"

// table of number of bytes given a UTF8 lead char
static const int UTF8_SIZE[] = {
  // 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 00
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 10
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 20
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 30
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 40
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 50
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 60
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 70
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 80
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 90
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // A0
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // B0
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // C0
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // D0
     2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // E0
     3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5  // F0
};

std::wstring ConvertUTF8ToUTF16(const std::string& src) {
  std::wstring result;
  wchar_t c;
  std::string::const_iterator it, end = src.end();
  #define GET_BITS(c,n) (((c) & 0xFF) & ((unsigned char)(1 << (n)) - 1))
  for (it = src.begin(); it < end;) {
    int bits = UTF8_SIZE[(*it) & 0xFF];
    c = GET_BITS(*it++, 8 - 1 - bits);
    switch(bits) {
      case 5: c <<= 6; c |= GET_BITS(*it++, 6);
      case 4: c <<= 6; c |= GET_BITS(*it++, 6);
      case 3: c <<= 6; c |= GET_BITS(*it++, 6);
      case 2: c <<= 6; c |= GET_BITS(*it++, 6);
      case 1: c <<= 6; c |= GET_BITS(*it++, 6);
    }
    result.append(1, c);
  }
  return result;
  #undef GET_BITS
}

std::string ConvertUTF16ToUTF8(const std::wstring& src) {
  std::string result;
  unsigned long c;
  unsigned char buffer[6];
  std::wstring::const_iterator it, end = src.end();
  #define GET_BITS(c,n) (((c) & 0xFF) & ((unsigned char)(1 << (n)) - 1))
  for (it = src.begin(); it < end; ++it) {
    c = (unsigned long)(*it);
    int trailCount;
    if (c >= 0x04000000) {
      trailCount = 5;
    } else if (c >= 0x200000) {
      trailCount = 4;
    } else if (c >= 0x010000) {
      trailCount = 3;
    } else if (c >= 0x0800) {
      trailCount = 2;
    } else if (c >= 0x80) {
      trailCount = 1;
    } else {
      // raw, special case
      result.append(1, (char)(c & 0x7F));
      continue;
    }
    for (int i = trailCount; i > 0; --i) {
      buffer[i] = (unsigned char)(c & 0x3F);
      c >>= 6;
    }
    buffer[0] = ((1 << (trailCount + 1)) - 1) << (8 - trailCount - 1);
    buffer[0] |= c & ((1 << (8 - trailCount - 2)) - 1);
    result.append((std::string::value_type*)buffer, trailCount + 1);
  }
  return result;
  #undef GET_BITS
}

tstring ConvertUTF8toUTFn(const std::string& src) {
  #if defined(XP_WIN) && defined(_UNICODE)
    return ConvertUTF8ToUTF16(src);
  #else
    return src;
  #endif
}

std::wstring ConvertUTFnToUTF16(const std::wstring& src) {
  return src;
}
std::wstring ConvertUTFnToUTF16(const std::string& src) {
  return ConvertUTF8ToUTF16(src);
}
std::string  ConvertUTFnToUTF8(const std::wstring& src) {
  return ConvertUTF16ToUTF8(src);
}
std::string  ConvertUTFnToUTF8(const std::string& src) {
  return src;
}
