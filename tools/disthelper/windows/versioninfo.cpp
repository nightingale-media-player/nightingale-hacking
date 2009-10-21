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

#include <assert.h>
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

/* round up to a multiple of 4 bytes */
#define ROUND_UP_4(x) (((ULONG_PTR)(x) + 3) & ~3)

/**
 * Read a UTF-16 null terminated string from data + *offset, without
 * overrunning 'length'. Updates *offset after reading.
 * Returns empty string if the string is not terminated.
 */
static std::wstring ReadString(void *data, int length, int *offset)
{
  size_t buffer_len = (length - *offset) / sizeof(wchar_t);
  wchar_t *src_data = reinterpret_cast<wchar_t*>(data) + *offset / sizeof(wchar_t);
  size_t src_len = wcsnlen(src_data, buffer_len);
  if (src_len == buffer_len) {
      return std::wstring();
  }
  *offset += (src_len + 1) * sizeof(wchar_t);
  return std::wstring(src_data, src_len);
}

/**
 * Write a UTF-16 string and null terminator into data + *offset.
 * Updates *offset after writing.
 */
static void WriteString(void *data, int *offset, std::wstring value)
{
  wchar_t *buffer = reinterpret_cast<wchar_t*>(data);
  buffer += *offset / sizeof(wchar_t);
  wcsncpy(buffer, value.c_str(), value.length() + 1);
  *offset += (value.length() + 1) * sizeof(wchar_t);
}

/**
 * Read a WORD (16 bits unsigned) from data + *offset, without overrunning
 * 'length'.  Updates *offset after reading.
 */
static WORD ReadWord(void *data, int length, int *offset)
{
  size_t buffer_len = (length - *offset);
  if (buffer_len < sizeof(WORD)) {
    // not enough bytes
    throw("What am I supposed to do without exceptions?");
  }
  WORD result = *(reinterpret_cast<WORD*>(data) + *offset / sizeof(WORD));
  *offset += sizeof(WORD);
  return result;
}

/**
 * Write a WORD (unsigned 16 bit integer) into data + *offset.
 * Updates *offset after writing.
 */
static void WriteWord(void *data, int *offset, WORD value)
{
  *(reinterpret_cast<WORD*>(data) + *offset / sizeof(WORD)) = value;
  *offset += sizeof(WORD);
}

/**
 * Find child of structure with the key 'key' in this data, return
 * pointer to and length of this child. 
 * If 'key' is empty, return the first child.
 */
static int
FindChildWithKey(std::wstring searchKey,
                 void *data,
                 int length,
                 void **childData,
                 int *childDataLength)
{
  int offset = 0;
  WORD structureLength = ReadWord (data, length, &offset);
  WORD valueLength = ReadWord (data, length, &offset);
  WORD type = ReadWord (data, length, &offset);
  std::wstring key = ReadString (data, length, &offset);
  offset = ROUND_UP_4 (offset);
  offset += valueLength;
  offset = ROUND_UP_4 (offset);

  /* Now we have the children; find the one with the appropriate key */
  while (offset < length) {
    int childOffset = offset;

    WORD childLength = ReadWord (data, length, &offset);
    assert(childLength > 0);
    if (childLength < 1) {
      return DH_ERROR_PARSE;
    }
    WORD childValueLength = ReadWord (data, length, &offset);
    WORD childType = ReadWord (data, length, &offset);
    std::wstring childKey = ReadString (data, length, &offset);

    if (searchKey.empty() || childKey == searchKey) {
      *childData = (void *)((unsigned char *)data + childOffset);
      *childDataLength = childLength;

      return DH_ERROR_OK;
    }

    offset = childOffset + childLength;
    offset = ROUND_UP_4 (offset);
  }

  OutputDebugString(_T("Key not found"));
  return DH_ERROR_PARSE;
}

/**
 * Get the VS_VERION_INFO structure from an executable file
 * @param [in] executableName the full path to the executable to open
 * @param [out] data the data read from the file
 * @param [out] dataSize the size of the data read
 */
static int
GetVersionInfoBlock(std::wstring executableName,
                    void **data,
                    int *dataSize)
{
  // get the source data block...
  DWORD dummy;
  DWORD sourceSize = GetFileVersionInfoSize(executableName.c_str(), &dummy);
  if (!sourceSize) {
    OutputDebugString(_T("Failed to get version info size"));
    return DH_ERROR_READ;
  }
  if (sourceSize > 0x20000) {
    // too large to fit in a word, x2 (this normally comes back at twice the
    // space actually required)
    OutputDebugString(_T("File version info size larger than expected"));
    return DH_ERROR_PARSE;
  }
  void *sourceData = malloc(sourceSize);
  if (!sourceData) {
    OutputDebugString(_T("Failed to allocate source data"));
    return DH_ERROR_MEM;
  }
  BOOL success = GetFileVersionInfo(executableName.c_str(), dummy, sourceSize, sourceData);
  if (!success) {
    free (sourceData);
    OutputDebugString(_T("Failed to get version info data"));
    return DH_ERROR_PARSE;
  }

  // GetFileVersionInfoSize lies, don't trust it
  { /* for scope */
    int offset = 0;
    WORD realSize = ReadWord (sourceData, sourceSize, &offset);
    if (realSize > sourceSize) {
      free (sourceData);
      OutputDebugString(_T("Failed to get real version info data size"));
      return DH_ERROR_PARSE;
    }
    sourceSize = realSize;
  }

  *dataSize = sourceSize;
  *data = sourceData;

  return DH_ERROR_OK;
}

static int
ReadStringTable(void *buffer,
                int length,
                stringMap_t &stringData,
                std::wstring &langid)
{
  int offset = 0;
  WORD len = ReadWord(buffer, length, &offset);
  WORD dummy = ReadWord(buffer, length, &offset);
  WORD type = ReadWord(buffer, length, &offset);
  langid = ReadString(buffer, length, &offset);
  assert(len <= length);

  offset = ROUND_UP_4 (offset);
  while (offset < len) {
    WORD stringEntryLength = ReadWord(buffer, length, &offset);
    if (offset - sizeof(WORD) + stringEntryLength > len) {
      OutputDebugString(_T("Invalid string struct length"));
      return DH_ERROR_PARSE;
    }
    WORD valueLength = ReadWord (buffer, length, &offset);
    WORD stringType = ReadWord (buffer, length, &offset);

    std::wstring key = ReadString (buffer, length, &offset);
    offset = ROUND_UP_4 (offset);
    std::wstring value = ReadString (buffer, length, &offset);
    offset = ROUND_UP_4 (offset);

    // TODO: is this assertion correct?
    assert (value.length() + 1 == valueLength);
    // Add the string entry to our map.
    stringData[key] = stringData_t(value, stringType, stringEntryLength);
  }

  return DH_ERROR_OK;
}

static int
ReadStringsFromINIFile(IniEntry_t &aSection,
                       stringMap_t &stringData)
{
  IniEntry_t::const_iterator it, end = aSection.end();
  for (it = aSection.begin(); it != end; ++it) {
    std::wstring key = ConvertUTF8ToUTF16(it->first);
    std::wstring value = ConvertUTF8ToUTF16(it->second);
    /* length is the length member of the String structure.
       It does NOT include the padding at the end, but DOES
       include the mid-structure padding */
    int length = 3 * sizeof(WORD) + /* struct length + value length + type */
                  (key.length() + 1) * sizeof(WCHAR);
    length = ROUND_UP_4 (length);
    length += (value.length() + 1) * sizeof(WCHAR);
    stringData[key] = stringData_t(value, 1, length);
  }
  return DH_ERROR_OK;
}

static int
GetStringTableSize(const stringMap_t &stringData)
{
  // Start with the StringTable header size: 3 words, plus 8 character
  // language ID (plus null terminator)
  int size = ROUND_UP_4 (3 * sizeof(WORD) + 9 * sizeof(WCHAR));

  // Add all the entry sizes.
  stringMap_t::const_iterator begin(stringData.begin()),
                              end(stringData.end());
  for(stringMap_t::const_iterator it = begin; it != end; ++it) {
    // The amount of space we use for each entry is rounded up to
    // a multiple of 4, even though the _used_ size of the entry
    // is not a multiple of 4.
    size += ROUND_UP_4 (it->second.size);
  }

  return size;
}

static int
WriteStringTable(void *buffer,
                 const stringMap_t &stringData,
                 const std::wstring langid)
{
  int tableSize = GetStringTableSize (stringData);

  // Write the StringTable header.
  int offset = 0;
  WriteWord (buffer, &offset, tableSize);
  WriteWord (buffer, &offset, 0);
  WriteWord (buffer, &offset, 1);
  WriteString (buffer, &offset, langid);

  offset = ROUND_UP_4 (offset);

  // and copy the data over...
  stringMap_t::const_iterator begin = stringData.begin();
  stringMap_t::const_iterator end = stringData.end();
  for(stringMap_t::const_iterator it = begin; it != end; ++it) {
    // String structure size (NOT rounded up to a multiple of 4).
    WriteWord (buffer, &offset, it->second.size);
    // String length (including null terminator)
    WriteWord (buffer, &offset, it->second.value.length() + 1);
    // Type
    WriteWord (buffer, &offset, it->second.type);
    // Key string (including null terminator)
    WriteString (buffer, &offset, it->first.data());

    offset = ROUND_UP_4 (offset);
    // Value string (including null terminator)
    WriteString (buffer, &offset, it->second.value);

    // However, the amount we actually write (unlike the length
    // member) IS rounded up to a multiple of 4.
    offset = ROUND_UP_4 (offset);
  }
  return DH_ERROR_OK;
}

int CommandSetVersionInfo(std::string aExecutable, IniEntry_t& aSection) {

  int result = DH_ERROR_UNKNOWN;
  void *sourceData(NULL);
  int sourceSize;
  stringMap_t stringData;
  void *newdata(NULL);
  HANDLE updateRes(NULL);
  std::wstring langid;

  std::wstring executableName(ResolvePathName(aExecutable));

  result = GetVersionInfoBlock(executableName, &sourceData, &sourceSize);
  if (result != DH_ERROR_OK) {
    OutputDebugString(_T("Failed to get version info"));
    goto CLEANUP;
  }

  /* Ok, we have our VS_VERSIONINFO data in sourceData.
   *
   * We find the StringFileInfo structure in here, then we
   * find the first string table within this, and update it.
   *
   * We then go back and update various length members in
   * the StringFileInfo and VS_VERSIONINFO structures.
   */
  void *stringFileInfoBuffer;
  int stringFileInfoLength;
  result = FindChildWithKey(L"StringFileInfo",
                            sourceData,
                            sourceSize,
                            &stringFileInfoBuffer,
                            &stringFileInfoLength);
  if (result != DH_ERROR_OK) {
    OutputDebugString(_T("Failed to find StringFileInfo"));
    goto CLEANUP;
  }

  void *stringTableBuffer;
  int stringTableLength;
  result = FindChildWithKey(L"",
                            stringFileInfoBuffer,
                            stringFileInfoLength,
                            &stringTableBuffer,
                            &stringTableLength);
  if (result != DH_ERROR_OK) {
    OutputDebugString(_T("Failed to find string table"));
    goto CLEANUP;
  }

  /* So we're now going to completely rewrite this string table,
     in our output buffer. The bits following this string table
     remain identical. Those preceeding it just have the sizes
     updated where needed. Calculate the chunks we're going to
     merely copy around.
   */
  void *initialData = sourceData;
  int initialDataSize = reinterpret_cast<char*>(stringTableBuffer) -
                        reinterpret_cast<char*>(sourceData);

  /* This is a bit tricky. The stringTableLength may (on input) not be a
     multiple of four. So stringTableBuffer + stringTableLength might point
     at some padding. If instead we round this up to a multiple of 4, we get
     a pointer to the actual _data_ following our string table.

     However, we don't re-introduce this padding (if required) on the way out
     - because of the specific way we build the string table itself, we end up
     including the padding in there, so the string table itself is always a
     multiple of four.
   */
  void *followingData = reinterpret_cast<char*>(stringTableBuffer) +
                        ROUND_UP_4 (stringTableLength);
  int followingDataSize = reinterpret_cast<char*>(sourceData) + sourceSize -
                          reinterpret_cast<char*>(followingData);

  /* Read the original strings */
  result = ReadStringTable(stringTableBuffer,
                           stringTableLength,
                           stringData,
                           langid);
  if (result != DH_ERROR_OK) {
    OutputDebugString(_T("Failed to read string table"));
    goto CLEANUP;
  }

  result = ReadStringsFromINIFile(aSection,
                                  stringData);
  if (result != DH_ERROR_OK) {
    OutputDebugString(_T("Failed to read strings from ini file"));
    goto CLEANUP;
  }

  int newStringTableSize = GetStringTableSize(stringData);

  /* Create our new buffer, copy in the old bits, and zero out the
     bit we're replacing with our new string table */
  int totalSize = initialDataSize + newStringTableSize + followingDataSize;
  newdata = malloc(totalSize);
  memcpy(newdata, initialData, initialDataSize);
  ZeroMemory((char*)newdata + initialDataSize, newStringTableSize);
  memcpy((char*)newdata + initialDataSize + newStringTableSize,
         followingData, followingDataSize);

  result = WriteStringTable((char*)newdata + initialDataSize,
                            stringData,
                            langid);
  if (result != DH_ERROR_OK) {
    OutputDebugString(_T("Failed to write string table"));
    goto CLEANUP;
  }

  /* Fix up header sizes */
  // VS_VERSIONINFO->wLength;
  int offset = 0;
  WriteWord(newdata, &offset, totalSize);
  // StringFileInfo->wLength;
  stringFileInfoLength += newStringTableSize - stringTableLength;
  offset = (char*)stringFileInfoBuffer - (char*)sourceData;
  WriteWord(newdata, &offset, stringFileInfoLength);

  // write the new data
  updateRes = BeginUpdateResource(executableName.c_str(), FALSE);
  if (updateRes == NULL) {
    OutputDebugString(_T("Unable to open library for writing.\n"));
    result = DH_ERROR_WRITE;
    goto CLEANUP;
  }

  BOOL success;
  success = UpdateResource(updateRes, RT_VERSION, MAKEINTRESOURCE(1),
                           MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                           newdata, totalSize);
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
