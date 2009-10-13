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

#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <string>
#include <map>

#include "error.h"
#include "stringconvert.h"
#include "commands.h"
#include "readini.h"

struct stringData_t {
  std::wstring value;
  WORD type;
  WORD size;
  stringData_t(std::wstring s, WORD t, WORD z)
    :value(s), type(t), size(z) {}
  stringData_t() :value(L""), type(0), size(0xFFFF) {}
};
typedef std::map<std::wstring, stringData_t> stringMap_t;

/* macros for rounding to 32-bit aligned boundaries */
/* align down to 32 bit */
#define ROUND_DOWN_4(x) ((ULONG_PTR)(x) & ~3)
/* align up to 32 bit */
#define ROUND_UP_4(x) (((ULONG_PTR)(x) + 3) & ~3)

int CommandSetVersionInfo(std::string aExecutable, IniEntry_t& aSection) {

  int result = DH_ERROR_UNKNOWN;
  LPVOID sourceData(NULL);
  stringMap_t stringData;
  LPSTR newdata(NULL);
  WORD newSize = 0;  // size of array of strings (excluding StringTable header)
  LPCSTR tailer(NULL);
  WORD stringTableOffset(0), // offset from start to StringTable
       dataAfterStringFileInfoSize(0), // size of data _after_ StringFileInfo
       headerSize(0), // size of everything up to first string
       tailerSize(0); // to start of relevant StringTable
  HANDLE updateRes(NULL);

  tstring executableName(ResolvePathName(aExecutable));

  // get the source data block...
  DWORD dummy;
  DWORD sourceSize = GetFileVersionInfoSize(executableName.c_str(), &dummy);
  if (!sourceSize) {
    OutputDebugString(_T("Failed to get version info size"));
    result = DH_ERROR_READ;
    goto CLEANUP;
  }
  if (sourceSize > 0x20000) {
    // too large to fit in a word, x2 (this normally comes back at twice the
    // space actually required)
    OutputDebugString(_T("File version info size larger than expected"));
    result = DH_ERROR_PARSE;
    goto CLEANUP;
  }
  sourceData = (LPVOID)malloc(sourceSize);
  if (!sourceData) {
    OutputDebugString(_T("Failed to allocate source data"));
    result = DH_ERROR_MEM;
    goto CLEANUP;
  }
  BOOL success = GetFileVersionInfo(executableName.c_str(), dummy, sourceSize, sourceData);
  if (!success) {
    OutputDebugString(_T("Failed to get version info data"));
    result = DH_ERROR_PARSE;
    goto CLEANUP;
  }

  // GetFileVersionInfoSize lies, don't trust it
  { /* for scope */
    WORD newSize = *(WORD*)sourceData;
    if (newSize > sourceSize) {
      OutputDebugString(_T("Failed to get real version info data size"));
      result = DH_ERROR_PARSE;
      goto CLEANUP;
    }
    sourceSize = newSize;
  }

  // figure out the translations
  LPSTR translationBuffer;
  UINT translationLength;
  success = VerQueryValue(sourceData, _T("\\VarFileInfo\\Translation"),
                          (LPVOID*)&translationBuffer, &translationLength);
  if (!success) {
    OutputDebugString(_T("Failed to get translation array"));
    result = DH_ERROR_PARSE;
    goto CLEANUP;
  }
  // end of strings = translationBuffer - 0x40

  LPSTR stringFileInfoBuffer;
  UINT stringFileInfoLength;
  success = VerQueryValue(sourceData, _T("\\StringFileInfo"),
                          (LPVOID*)&stringFileInfoBuffer, &stringFileInfoLength);
  if (!success) {
    OutputDebugString(_T("Failed to get StringFileInfo"));
    result = DH_ERROR_PARSE;
    goto CLEANUP;
  }

  /* The pointer returned points to start of the StringTable data (or padding).
   * We want to find the start of the StringFileInfo structure; this is found at
   * -34 bytes in this case (6 bytes, plus 14 characters for "StringFileInfo")
   * without a null terminator; there may also be random bits of padding involved
   *
   * This is using undocumented behaviour and really should be rewritten; see
   * bug 18353
   */
  stringFileInfoBuffer -= (sizeof(L"StringFileInfo") - sizeof(L""))
                          + sizeof(WORD) * 3;
  stringFileInfoBuffer = (LPSTR)ROUND_DOWN_4(stringFileInfoBuffer);
  if (stringFileInfoBuffer < sourceData) {
    OutputDebugString(_T("Unexpected StringFileInfo buffer"));
    result = DH_ERROR_PARSE;
    goto CLEANUP;
  }
  // again, the length lies, read it from the file
  stringFileInfoLength = *(WORD*)stringFileInfoBuffer;
  if (stringFileInfoLength > sourceSize ||
      (char*)stringFileInfoBuffer + stringFileInfoLength > (char*)sourceData + sourceSize)
  {
    OutputDebugString(_T("Invalid StringFileInfo length"));
    result = DH_ERROR_PARSE;
    goto CLEANUP;
  }
  /* Calculate the size of the data after the StringFileInfo block; note that
   * this data will need to be aligned on a 4-byte boundary, therefore we need
   * to round up the size of the StringFileInfo block.  (The offset to the start
   * of the StringFileInfo block is already aligned.)
   *
   * this really needs to go away (see bug 18353)
   */
  dataAfterStringFileInfoSize = sourceSize -
                                ((stringFileInfoBuffer - sourceData) +
                                 ROUND_UP_4(stringFileInfoLength));

  LPSTR stringTableBuffer;
  UINT stringTableLength;
  TCHAR stringTableQueryTemplate[] = _T("\\StringFileInfo\\00000000");
  TCHAR stringTableQuery[sizeof(stringTableQueryTemplate) / sizeof(TCHAR)];
  // only modify the first string table for now
  _stprintf(stringTableQuery,
            _T("\\StringFileInfo\\%04x%04x"),
            *((WORD*)translationBuffer),
            *((WORD*)translationBuffer + 1));
  success = VerQueryValue(sourceData, stringTableQuery,
                          (LPVOID*)&stringTableBuffer, &stringTableLength);
  if (!success) {
    OutputDebugString(_T("Failed to get string table"));
    result = DH_ERROR_PARSE;
    goto CLEANUP;
  }
  if (stringTableBuffer < sourceData) {
    OutputDebugString(_T("Invalid string table buffer"));
    result = DH_ERROR_PARSE;
    goto CLEANUP;
  }

  // reach back to the start of the string table
  LPCSTR stringTableNext = (stringTableBuffer + 2);
  stringTableBuffer -= 0x16; // code page identifier string, other header
  stringTableLength = *(WORD*)stringTableBuffer;
  if (stringTableLength > sourceSize ||
      stringTableBuffer + stringTableLength > (char*)sourceData + sourceSize)
  {
    OutputDebugString(_T("Invalid string table length"));
    result = DH_ERROR_PARSE;
    goto CLEANUP;
  }

  // copy the header so we can modify things
  stringTableOffset = (UINT_PTR)stringTableBuffer - (UINT_PTR)sourceData;
  headerSize = (UINT_PTR)stringTableNext - (UINT_PTR)sourceData;

  while (stringTableNext < stringTableBuffer + stringTableLength) {
    LPCWSTR buffer = (LPCWSTR)stringTableNext;
    WORD structLength = *(WORD*)buffer++; // in bytes
    if (structLength > stringTableLength ||
        stringTableNext + structLength > (char*)sourceData + sourceSize)
    {
      OutputDebugString(_T("Invalid string struct length"));
      result = DH_ERROR_PARSE;
      goto CLEANUP;
    }
    WORD valueLength = *(WORD*)buffer++;  // in wchars
    WORD type = *(WORD*)buffer++;         // hopefully 1
    WORD keyLength = (structLength - 6) / 2 - valueLength;  // in wchars
    if (keyLength > structLength / 2) {
      OutputDebugString(_T("Invalid string key length"));
      result = DH_ERROR_PARSE;
      goto CLEANUP;
    }
    std::wstring key(buffer, keyLength - 1);
    // Remove additional trailing nulls
    #if DEBUG
    {
      std::string debugBuffer("KEY: ");
      for (std::wstring::const_iterator it = key.begin(); it != key.end(); ++it) {
        char hexBuffer[0x10];
        _snprintf(hexBuffer, sizeof(hexBuffer)/sizeof(hexBuffer[0]), "%04X", *it);
        debugBuffer.append(hexBuffer);
      }
      ::OutputDebugStringA(debugBuffer.c_str());
    }
    #endif
    std::wstring::size_type firstNull = key.find(L'\0');
    if (firstNull != std::wstring::npos) {
      key.erase(firstNull);
    }
    buffer += keyLength;
    // the pointer must be 32-bit aligned
    buffer = (LPCWSTR)ROUND_UP_4(buffer);
    if ((char*)buffer + valueLength * sizeof(std::wstring::value_type) >
        (char*)sourceData + sourceSize) {
      OutputDebugString(_T("Invalid string value length"));
      result = DH_ERROR_PARSE;
      goto CLEANUP;
    }
    std::wstring value(buffer, valueLength - 1);

    stringData[key] = stringData_t(value, type, structLength);

    structLength = ROUND_UP_4(structLength);
    stringTableNext += structLength;
  }

  tailer = stringTableNext;
  tailerSize = (WORD)(sourceSize - ((ULONG_PTR)stringTableNext - (ULONG_PTR)sourceData));

  // Read from the .ini file
  {
    IniEntry_t::const_iterator it, end = aSection.end();
    for (it = aSection.begin(); it != end; ++it) {
      std::wstring key = ConvertUTF8ToUTF16(it->first);
      std::wstring value = ConvertUTF8ToUTF16(it->second);
      WORD length = 3 * sizeof(WORD) + /* struct length + value length + type */
                    (key.length() + 1) * sizeof(WCHAR);
      length = ROUND_UP_4(length);
      length += (value.length() + 1) * sizeof(WCHAR);
      stringData[key] = stringData_t(value, 1, length);
    }
  }

  {
    stringMap_t::const_iterator begin(stringData.begin()),
                                end(stringData.end());
    for(stringMap_t::const_iterator it = begin; it != end; ++it) {
      WORD paddedSize = it->second.size;
      paddedSize = ROUND_UP_4(paddedSize);
      newSize += paddedSize;
    }
    newdata = (LPSTR)malloc(headerSize + newSize + tailerSize);
    memcpy(newdata, sourceData, headerSize);
    ZeroMemory(newdata + headerSize, newSize);
    memcpy(newdata + headerSize + newSize, tailer, tailerSize);
    LPSTR pOut = newdata + headerSize;

    // and copy the data over...
    for(stringMap_t::const_iterator it = begin; it != end; ++it) {
      WORD length = it->second.value.length();
      *(WORD*)pOut = it->second.size;
      pOut += sizeof(WORD);
      *(WORD*)pOut = (length + 1); // + terminating null
      pOut += sizeof(WORD);
      *(WORD*)pOut = it->second.type;
      pOut += sizeof(WORD);
      memcpy(pOut, it->first.data(), it->first.length() * sizeof(WCHAR));
      pOut += it->first.length() * sizeof(WCHAR);
      *(WCHAR*)pOut = '\0';
      pOut += sizeof(WCHAR);
      pOut = (LPSTR)ROUND_UP_4(pOut);
      memcpy(pOut, it->second.value.data(), length * sizeof(WCHAR));
      pOut += length * sizeof(WCHAR);
      *(WCHAR*)pOut = '\0';
      pOut += sizeof(WCHAR);
      pOut = (LPSTR)ROUND_UP_4(pOut);
    }

    // fix up the size info
    WORD totalSize = headerSize + newSize + tailerSize;
    // VS_VERSIONINFO->wLength
    *(WORD*)(newdata) = totalSize;
    WORD dataBeforeStringFileInfoSize = stringFileInfoBuffer - sourceData;
    // StringFileInfo->wLength
    *(WORD*)(newdata + dataBeforeStringFileInfoSize) =
      totalSize - dataAfterStringFileInfoSize - dataBeforeStringFileInfoSize;
    // StringTable->wLength
    *(WORD*)(newdata + stringTableOffset) = newSize + 0x18; // account for header size too
  }

  #if DEBUG
  {
    ::OutputDebugStringA("RESULT DATA: ");
    std::string debugBuffer("000ef120 ");
    for (unsigned long i = 0; i < headerSize + newSize + tailerSize; ++i) {
      char hexBuffer[0x10];
      _snprintf(hexBuffer, sizeof(hexBuffer)/sizeof(hexBuffer[0]),
                "%02X ", (unsigned char)newdata[i]);
      debugBuffer.append(hexBuffer);
      if (i % 0x10 == 0x10 - 1) {
        ::OutputDebugStringA(debugBuffer.c_str());
        _snprintf(hexBuffer, sizeof(hexBuffer)/sizeof(hexBuffer[0]),
                  "%08x ", 0x000ef120 + i + 1);
        debugBuffer.assign(hexBuffer);
      }
    }
    ::OutputDebugStringA(debugBuffer.c_str());
  }
  #endif

  // write the new data
  updateRes = BeginUpdateResource(executableName.c_str(), FALSE);
  if (updateRes == NULL) {
    OutputDebugString(_T("Unable to open library for writing.\n"));
    result = DH_ERROR_WRITE;
    goto CLEANUP;
  }

  success = UpdateResource(updateRes, RT_VERSION, MAKEINTRESOURCE(1),
                           MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                           newdata, headerSize + newSize + tailerSize);
  if (!success) {
    OutputDebugString(_T("Unable to update version info tables.\n"));
    result = DH_ERROR_WRITE;
    goto CLEANUP;
  }

  success = EndUpdateResource(updateRes, FALSE);
  updateRes = NULL;
  if (!success) {
    OutputDebugString(_T("Unable to commit version info tables.\n"));
    result = DH_ERROR_WRITE;
    goto CLEANUP;
  }

  result = 0;
CLEANUP:
  if (updateRes) {
    // abort the resource update
    EndUpdateResource(updateRes, TRUE);
    updateRes = NULL;
  }
  if (sourceData) {
    free(sourceData);
    sourceData = NULL;
  }
  return result;
}
