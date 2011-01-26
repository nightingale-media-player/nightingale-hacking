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
 * The Original Code is McCoy.
 *
 * The Initial Developer of the Original Code is
 * the Mozilla Foundation <http://www.mozilla.org/>.
 * Portions created by the Initial Developer are Copyright (C) 2008
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Dave Townsend <dtownsend@oxymoronical.com>
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
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <tchar.h>
#include <string>

#include "error.h"
#include "stringconvert.h"
#include "debug.h"
#include "commands.h"

#ifndef UNICODE
#error This only supports the UNICODE configuration (no SBCS/MBCS)
#endif

/*
Icon files are made up of:

IconHeader
IconDirEntry1
IconDirEntry2
...
IconDirEntryN
IconData1
IconData2
...
IconDataN

Each IconData must be added as a new RT_ICON resource to the exe. Then
an RT_GROUP_ICON resource must be added that contains an equivalent
header:

IconHeader
IconResEntry1
IconResEntry2
...
IconResEntryN
*/

#pragma pack(push, 2)
typedef struct {
  WORD Reserved;
  WORD ResourceType;
  WORD ImageCount;
} IconHeader;

typedef struct {
  BYTE Width;
  BYTE Height;
  BYTE Colors;
  BYTE Reserved;
  WORD Planes;
  WORD BitsPerPixel;
  DWORD ImageSize;
  DWORD ImageOffset;
} IconDirEntry;

typedef struct {
  BYTE Width;
  BYTE Height;
  BYTE Colors;
  BYTE Reserved;
  WORD Planes;
  WORD BitsPerPixel;
  DWORD ImageSize;
  WORD ResourceID;    // This field is the one difference to above
} IconResEntry;
#pragma pack(pop)


int CommandSetIcon(std::string aExecutable, std::string aIconFile, std::string aIconName) {
  tstring executable, iconfile;
  executable.assign(ResolvePathName(aExecutable));
  iconfile.assign(ResolvePathName(aIconFile));
  int file = _topen(iconfile.c_str(), _O_BINARY | _O_RDONLY);
  if (file == -1) {
    DebugMessage("Failed to open icon file %S.", aIconFile);
    return DH_ERROR_READ;
  }

  // Load all the data from the icon file
  long filesize = _filelength(file);
  if (filesize > 100 * 1024 * 1024) {
    // icon file larger than 100 MB, I don't think so
    DebugMessage("Icon file too big, aborting");
    return DH_ERROR_PARSE;
  }
  char* data = (char*)malloc(filesize);
  _read(file, data, filesize);
  _close(file);
  IconHeader* header = (IconHeader*)data;

  // Open the target library for updating
  HANDLE updateRes = BeginUpdateResource(executable.c_str(), FALSE);
  if (updateRes == NULL) {
    DebugMessage("Unable to open file %S for modification", executable.c_str());
    return DH_ERROR_WRITE;
  }

  // Allocate the group resource entry
  if ((long)(sizeof(IconHeader) + header->ImageCount * sizeof(IconDirEntry)) >
      filesize)
  {
    OutputDebugString(_T("Inconsistent icon header.\n"));
    free(data);
    return DH_ERROR_PARSE;
  }
  long groupsize = sizeof(IconHeader) + header->ImageCount * sizeof(IconResEntry);
  char* group = (char*)malloc(groupsize);
  if (!group) {
    OutputDebugString(_T("Failed to allocate memory for new images.\n"));
    free(data);
    return DH_ERROR_MEM;
  }
  memcpy(group, data, sizeof(IconHeader));

  IconDirEntry* sourceIcon = (IconDirEntry*)(data + sizeof(IconHeader));
  IconResEntry* targetIcon = (IconResEntry*)(group + sizeof(IconHeader));

  for (int id = 1; id <= header->ImageCount; ++id) {
    // Add the individual icon
    if (!UpdateResource(updateRes, RT_ICON, MAKEINTRESOURCE(id),
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                        data + sourceIcon->ImageOffset, sourceIcon->ImageSize)) {
      OutputDebugString(_T("Unable to update resource.\n"));
      EndUpdateResource(updateRes, TRUE);  // Discard changes, ignore errors
      free(data);
      free(group);
      return DH_ERROR_WRITE;
    }
    // Copy the entry for this icon (note that the structs have different sizes)
    targetIcon->Width        = sourceIcon->Width;
    targetIcon->Height       = sourceIcon->Height;
    targetIcon->Colors       = sourceIcon->Colors;
    targetIcon->Reserved     = sourceIcon->Reserved;
    targetIcon->Planes       = sourceIcon->Planes;
    targetIcon->BitsPerPixel = sourceIcon->BitsPerPixel;
    targetIcon->ImageSize    = sourceIcon->ImageSize;
    targetIcon->ResourceID   = id;
    sourceIcon++;
    targetIcon++;
  }
  free(data);

  tstring name(ConvertUTF8ToUTF16(aIconName));
  if (name.empty()) {
    name.assign(_T("IDI_APPICON"));
  }
  LPCTSTR rawName = name.c_str();
  if (!name.empty() && name[0] == _T('#')) {
    // assume we want some int resource
    rawName = MAKEINTRESOURCE(_tstoi(name.c_str() + 1));
  }
  if (!UpdateResource(updateRes, RT_GROUP_ICON, rawName,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                      group, groupsize)) {
    OutputDebugString(_T("Unable to update resource.\n"));
    EndUpdateResource(updateRes, TRUE);  // Discard changes
    free(group);
    return DH_ERROR_WRITE;
  }

  free(group);

  // Save the modifications
  if (!EndUpdateResource(updateRes, FALSE)) {
    OutputDebugString(_T("Unable to write changes to library.\n"));
    return DH_ERROR_WRITE;
  }

  return DH_ERROR_OK;
}
