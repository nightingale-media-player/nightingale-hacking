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

#include <algorithm>
#include <list>
#include <vector>

#include <windows.h>
#include <tchar.h>
#include <shellapi.h>
#include <shlobj.h>
#include <stdlib.h>

#include "reghandlers.h"
#include "stringconvert.h"
#include "toolslib.h"
#include "debug.h"
#include "error.h"
#include "regparse.h"

#ifndef RH_REGKEY_ENGAGE_SAFETY
  /* WARNING: live ammunition... */
  LPCTSTR KEY_SHARED_DLLS =
    _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedDLLs");
  LPCTSTR KEY_DEVICE_CDROM =
   _T("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E965-E325-11CE-BFC1-08002BE10318}");
  LPCTSTR KEY_DEVICE_TAPE =
   _T("SYSTEM\\CurrentControlSet\\Control\\Class\\{6D807884-7D21-11CF-801C-08002BE10318}");
  LPCTSTR KEY_DEVICE_MEDIACHANGER =
   _T("SYSTEM\\CurrentControlSet\\Control\\Class\\{CE5939AE-EBDE-11D0-B181-0000F8753EC4}");
#else
  /* Safety is on; testing keys... */
  LPCTSTR KEY_SHARED_DLLS =
   _T("SOFTWARE\\Songbird\\gearworks-test\\SharedDLLs");
  LPCTSTR KEY_DEVICE_CDROM =
   _T("SOFTWARE\\Songbird\\gearworks-test\\dkey1");
  LPCTSTR KEY_DEVICE_TAPE =
   _T("SOFTWARE\\Songbird\\gearworks-test\\dkey2");
  LPCTSTR KEY_DEVICE_MEDIACHANGER =
   _T("SOFTWARE\\Songbird\\gearworks-test\\dkey3");
#endif

LPCTSTR DRIVER_SUBKEY_STR = _T("UpperFilters");

LPCTSTR STR_ASPI_DLL_NAME = _T("GEARAspi.dll");
LPCTSTR STR_ASPI_SYS_NAME = _T("Drivers\\GEARAspiWDM.sys");
LPCTSTR STR_ASPI_SYS_BASENAME = _T("GEARAspiWDM.sys");
LPCTSTR STR_ASPI_SERVICE_NAME = _T("GEARAspiWDM");

static const TCHAR GEARWORKS_REG_VALUE_STR[] = _T("GEARAspiWDM");
static const TCHAR REDBOOK_REG_VALUE_STR[] = _T("redbook");

// Careful; if you change this, you have to change it in the uninstaller!
static const TCHAR SONGBIRD_CDRIP_REG_KEY_ROOT[] = 
 _T("Software\\Songbird\\CdripDriverInstallations");

static const size_t DEFAULT_REG_BUFFER_SZ = 4096;

static const size_t PATH_STORAGE = MAX_PATH + 1;

#define ARRAY_LENGTH(x) (sizeof(x)/sizeof(x)[0])

static const regValue_t GEARWORKS_REG_VALUE(GEARWORKS_REG_VALUE_STR,
 ARRAY_LENGTH(GEARWORKS_REG_VALUE_STR) - 1);

static const regValue_t REDBOOK_REG_VALUE(REDBOOK_REG_VALUE_STR,
 ARRAY_LENGTH(REDBOOK_REG_VALUE_STR) - 1);

LPTSTR GetSystemErrorMessage(DWORD errNum) {
  LPTSTR returnString = NULL; 
  LONG fmRv = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
   FORMAT_MESSAGE_FROM_SYSTEM,           // flags
   NULL,                                 // lpSource
   errNum,                               // dwMessageId
   0,                                    // dwLanguageId
   (LPTSTR)&returnString,                // lpBuffer
   0,                                    // nSize (not necessary, due to flags)
   NULL);                                // var-args list

  if (!fmRv) {
    DebugMessage("GetSystemErrorMessage() failed; errno was %d", errno);
    return NULL;
  }
  else {
    return returnString;
  }
}

void DoLogMessage(LONG rv, LPCTSTR message) {
    LPTSTR errStr = GetSystemErrorMessage(rv);
    DebugMessage("%S: %S", message, errStr);
    LocalFree(errStr);
}

BOOL LoggedSUCCEEDED(LONG rv, LPCTSTR message) {
  BOOL callSucceeded = (rv == ERROR_SUCCESS);

  if (!callSucceeded)
    DoLogMessage(rv, message);

  return callSucceeded;
}

// Call AdjustDllUseCount with a value of 0 to query what the current key
// would be (passing result would be useful for this operation, of course)

LONG AdjustDllUseCount(LPCTSTR dllName, int value, int* result = NULL) {
  HRESULT hr;
  LONG ret;

  TCHAR sysDir[PATH_STORAGE], nameBuffer[PATH_STORAGE];

  // Ensure these are 0s from the start...
  memset(sysDir, 0, PATH_STORAGE);
  memset(nameBuffer, 0, PATH_STORAGE);

  hr = SHGetFolderPath(NULL,               // HWND owner
                       CSIDL_SYSTEM,       // folder (system32)
                       NULL,               // access token
                       SHGFP_TYPE_CURRENT, // flags
                       sysDir);            // [out] path

  if (FAILED(hr) || hr == S_FALSE) {
    DoLogMessage(hr, _T("Get system directory"));
    return RH_ERROR_QUERY_KEY;
  }

  sysDir[MAX_PATH] = _T('\0'); // never trust 

  HKEY hKeySharedDLLs;
  ret = RegCreateKey(HKEY_LOCAL_MACHINE, KEY_SHARED_DLLS, &hKeySharedDLLs);
  if (!LoggedSUCCEEDED(ret, _T("Open SharedDLLs key")))
    return RH_ERROR_OPENKEY;

  // adjust the refcount
  int len = _sntprintf(nameBuffer, MAX_PATH, _T("%s\\%s"), sysDir, dllName);

  if (len < 0) {
    DebugMessage("Failed to get DLL name");
    return RH_ERROR_QUERY_VALUE;
  }

  DWORD dllCount, bufferSize = sizeof(DWORD);

  ret = RegQueryValueEx(hKeySharedDLLs,    // key
                        nameBuffer,        // name
                        NULL,              // reserved
                        NULL,              // type (ignored)
                        (LPBYTE)&dllCount, // [out] data
                        &bufferSize);      // sizeof(data)

  if (ret == ERROR_FILE_NOT_FOUND)
    dllCount = 0;
  else if (!LoggedSUCCEEDED(ret, _T("Get dll refcount")))
    return RH_ERROR_QUERY_VALUE;

  // If we're called with an "adjustment" value of 0, that's actually a
  // Request to query the current value; see the header file for more info
  if (0 == value) {
    if (result)
      *result = dllCount; 

    return RH_OK;
  }

  dllCount += value;

  if (dllCount > 0) {
    ret = RegSetValueEx(hKeySharedDLLs,
                        nameBuffer,
                        NULL,
                        REG_DWORD,
                        (LPBYTE)&dllCount,
                        sizeof(DWORD));
  } else {
    ret = RegDeleteValue(hKeySharedDLLs, nameBuffer);
  }

  if (!LoggedSUCCEEDED(ret, _T("Set dll refcount")))
    return RH_ERROR_SETVALUE;

  RegCloseKey(hKeySharedDLLs);

  if (result)
    *result = dllCount; 

  return RH_OK;
}

LONG AddFilteredDriver(LPCTSTR regSubKey,
                      const regValue_t &newKey,
                      const driverLoc_t insertionInfo) {

  DWORD keyCreationResult;
  HKEY keyHandle;

  LONG rv = RegCreateKeyEx(HKEY_LOCAL_MACHINE,  // handle
                           regSubKey,           // lpSubKey
                           0,                   // dwReserved
                           NULL,                // lpClass
                           0,                   // dwOptions
                           KEY_ALL_ACCESS,      // samDesired
                           NULL,                // lpSecurityAttributes
                           &keyHandle,          // phkResult
                           &keyCreationResult); // lpdwDisposition

  if (!LoggedSUCCEEDED(rv, _T("RegCreateKeyEx() failed")))
    return RH_ERROR_INIT_CREATEKEY;

  DWORD currentKeyType = 0;
  DWORD dataBufferLen = DEFAULT_REG_BUFFER_SZ;
  LPTSTR data = (LPTSTR)malloc(dataBufferLen);

  memset(data, 0, dataBufferLen);

  rv = RegQueryValueEx(keyHandle,          // key handle
                       DRIVER_SUBKEY_STR,  // lpValueName
                       0,                  // lpReserved
                       &currentKeyType,    // lpType
                       (LPBYTE)data,       // lpData
                       &dataBufferLen);    // lpcbData

  if (REG_CREATED_NEW_KEY == keyCreationResult || ERROR_FILE_NOT_FOUND == rv) {

    if (REG_CREATED_NEW_KEY == keyCreationResult)
      DebugMessage("Created new key: %S", newKey.p);
    if (ERROR_FILE_NOT_FOUND == rv)
      DebugMessage("Creatint new subkey %S in existing key: %S",
                   GEARWORKS_REG_VALUE.p, newKey.p);

    regValueList_t newRegValueList;
    newRegValueList.insert(newRegValueList.begin(), GEARWORKS_REG_VALUE);

    multiSzBuffer_t newRegValue((LPBYTE)new char[DEFAULT_REG_BUFFER_SZ], 
     DEFAULT_REG_BUFFER_SZ);
    MakeMultiSzRegString(newRegValue, newRegValueList);

    rv = RegSetValueEx(keyHandle,             // key handle
                       DRIVER_SUBKEY_STR,     // lpValueName
                       0,                     // reserved
                       REG_MULTI_SZ,          // dwType
                       newRegValue.value,     // data
                       newRegValue.cbBytes);  // cbData

    if (!LoggedSUCCEEDED(rv, _T("RegSetValueEx() failed")))
      return RH_ERROR_SET_KEY;

    rv = RegCloseKey(keyHandle);
    LoggedSUCCEEDED(rv, _T("RegCloseKey() failed"));

  }
  else if (REG_OPENED_EXISTING_KEY == keyCreationResult) {
    OutputDebugString(_T("Opened old key"));

    if (currentKeyType != REG_SZ &&
        currentKeyType != REG_EXPAND_SZ &&
        currentKeyType != REG_MULTI_SZ) {
      DebugMessage("Unknown type %d for key %S", currentKeyType, regSubKey);
      free(data);
      return RH_ERROR_UNKNOWN_KEY_TYPE; 
    }

    multiSzBuffer_t oldRegValue((BYTE*)data, dataBufferLen);

    regValueList_t regValues = ParseValues(oldRegValue, currentKeyType);

    regValueList_t::iterator oldPosition =
      find(regValues.begin(), regValues.end(), newKey);

    // We code it with a switch() because it works out elegantly in the
    // case that we were asked to insert after a particular element; if
    // that element doesn't exist in the list, the operation effectively
    // becomes an INSERT_AT_FRONT, so we fall through to that case.

    switch (insertionInfo.insertAt) {
      case driverLoc_t::INSERT_AFTER_NODE: { 
        if (NULL == insertionInfo.loc) {
          DebugMessage("AddFilteredDriver(): Null insertion location");
          free(data);
          return RH_ERROR_INVALID_ARG;
        }

        regValueList_t::iterator insertAfterNode =
          find(regValues.begin(), regValues.end(), *insertionInfo.loc);

        // If we didn't find the node we were looking for, we default to 
        // inserting it up front
        if (insertAfterNode != regValues.end() && oldPosition < insertAfterNode ) {
          if (oldPosition != regValues.end()) {
            // somebody had the old value after the node we need to be before
            // delete the old value
            regValues.erase(oldPosition);
            insertAfterNode = find(regValues.begin(),
                                   regValues.end(),
                                   *insertionInfo.loc);
          }
          // insert() inserts to the node _before_; we want to be inserted
          // after that value, so increment the iterator; we shouldn't fall 
          // off the end because we test for that above.
          insertAfterNode++;
          regValues.insert(insertAfterNode, newKey);
          break;
        }

        /* ELSE FALL THROUGH */
      }

      case driverLoc_t::INSERT_AT_FRONT: {
        if (oldPosition == regValues.end()) {
          regValues.insert(regValues.begin(), newKey);
        }
        break;
      }

      default:
        DebugMessage("AddFilteredDriver(): Invalid insertion disposition: %d",
                     insertionInfo.insertAt);
        free(data);
        return RH_ERROR_INVALID_ARG;
    }

    multiSzBuffer_t newRegValue((LPBYTE)new char[DEFAULT_REG_BUFFER_SZ],
                                DEFAULT_REG_BUFFER_SZ);

    MakeMultiSzRegString(newRegValue, regValues);

    rv = RegSetValueEx(keyHandle,
                       DRIVER_SUBKEY_STR,
                       0,
                       REG_MULTI_SZ,
                       newRegValue.value, 
                       newRegValue.cbBytes);

    if (!LoggedSUCCEEDED(rv, _T("RegSetValueEx() failed"))) {
      free(data);
      delete [] newRegValue.value;
      return RH_ERROR_SETVALUE;
    }

    rv = RegCloseKey(keyHandle);
    LoggedSUCCEEDED(rv, _T("RegCloseKey() failed"));
    free(data);
    delete [] newRegValue.value;
  }
  else {
    OutputDebugString(_T("Unknown creationResult"));
  }

  return RH_OK;
}

LONG InstallAspiDriverFiles(void) {
  BOOL rv; 
  HRESULT hr;

  TCHAR existingDriverPath[PATH_STORAGE], newDriverPath[PATH_STORAGE],
        windowsSystemDir[PATH_STORAGE];

  // Ensure these are 0s from the start...
  memset(existingDriverPath, 0, PATH_STORAGE);
  memset(newDriverPath, 0, PATH_STORAGE);
  memset(windowsSystemDir, 0, PATH_STORAGE);

  tstring appDir = GetAppDirectory();

  hr = SHGetFolderPath(NULL,
                       CSIDL_SYSTEM,
                       NULL,
                       SHGFP_TYPE_CURRENT,
                       windowsSystemDir);

  if (FAILED(hr) || hr == S_FALSE) {
    DoLogMessage(hr, _T("Get system directory"));
    return RH_ERROR_QUERY_KEY;
  }

  windowsSystemDir[MAX_PATH] = _T('\0'); // never trust 

  int len = _sntprintf(existingDriverPath, MAX_PATH, _T("%sdrivers\\%s"),
            appDir.c_str(), STR_ASPI_DLL_NAME);

  if (len < 0) {
    DebugMessage("Failed to construct STR_ASPI_DLL_NAME source path");
    return RH_ERROR_QUERY_VALUE;
  }

  len = _sntprintf(newDriverPath, MAX_PATH, _T("%s\\%s"), windowsSystemDir,
                   STR_ASPI_DLL_NAME);

  if (len < 0) {
    DebugMessage("Failed to construct STR_ASPI_DLL_NAME dest path");
    return RH_ERROR_QUERY_VALUE;
  }
  
  rv = CopyFile(existingDriverPath,  // lpExistingFileName
                newDriverPath,       // lpNewFileName
                FALSE);              // bFailIfExists

  if (!rv)
    return RH_ERROR_COPYFILE_FAILED;

  // Reset these...
  memset(existingDriverPath, 0, PATH_STORAGE);
  memset(newDriverPath, 0, PATH_STORAGE);

  len = _sntprintf(existingDriverPath, MAX_PATH, _T("%sdrivers\\%s"),
                   appDir.c_str(), STR_ASPI_SYS_BASENAME);

  if (len < 0) {
    DebugMessage("Failed to construct STR_ASPI_SYS_BASENAME source path");
    return RH_ERROR_QUERY_VALUE;
  }

  len = _sntprintf(newDriverPath, MAX_PATH, _T("%s\\%s"),
                   windowsSystemDir, STR_ASPI_SYS_NAME);

  if (len < 0) {
    DebugMessage("Failed to construct STR_ASPI_SYS_NAME dest path");
    return RH_ERROR_QUERY_VALUE;
  }

  rv = CopyFile(existingDriverPath,  // lpExistingFileName
                newDriverPath,       // lpNewFileName
                FALSE);              // bFailIfExists

  if (!rv)
    return RH_ERROR_COPYFILE_FAILED;

  return RH_OK;
}


LONG RepairAspiDriver(void) {
  // We purposefully don't check many of the return values in this function; 
  // Since we're in repair mode, we try to clean up as much as possible, but
  // we're very verbose about with DebugMessage() about what happened, even
  // if it did fail...
  int finalRv = RH_OK;

  int rv, currentDllCount;

  rv = InstallAspiDriverFiles();
  DebugMessage("-- RepairMode: InstallAspiDriverFiles() rv: %d", rv);

  if (rv != RH_OK)
    finalRv = rv;

  rv = AdjustDllUseCount(STR_ASPI_DLL_NAME, 0, &currentDllCount);
  DebugMessage("-- RepairMode: AdjustDllUseCount(STD_ASPI_DLL_NAME) rv: %d; currentDllCount: %d",
               rv, currentDllCount);

  if (rv != RH_OK || currentDllCount <= 0) {
    rv = AdjustDllUseCount(STR_ASPI_DLL_NAME, 1);
    DebugMessage("-- RepairMode: AdjustDllUseCount(STR_ASPI_DLL_NAME,1) rv: %d",
                 rv);
    if (rv != RH_OK)
      finalRv = rv;
  }

  rv = AdjustDllUseCount(STR_ASPI_SYS_NAME, 0, &currentDllCount);
  DebugMessage("-- RepairMode: AdjustDllUseCount(STD_ASPI_SYS_NAME) rv: %d; currentDllCount: %d",
               rv, currentDllCount);

  if (rv != RH_OK || currentDllCount <= 0) {
    rv = AdjustDllUseCount(STR_ASPI_SYS_NAME, 1);
    DebugMessage("-- RepairMode: AdjustDllUseCount(STR_ASPI_SYS_NAME,1) rv: %d",
                 rv);
    if (rv != RH_OK)
      finalRv = rv;
  }

  rv = RegisterAspiService();
  DebugMessage("-- RepairMode: RegisterAspiServices(): rv: %d", rv);
  if (rv != RH_OK)
    finalRv = rv;

  rv = AddAspiFilteredDrivers();
  DebugMessage("-- RepairMode: AddAspiFilteredDrivers(): rv: %d", rv);
  if (rv != RH_OK)
    finalRv = rv;

  tstring appDir = GetAppDirectory();
  RecordAspiDriverInstallState(appDir.c_str(), DRIVER_INSTALLED);

  return finalRv;
}

LONG InstallAspiDriver(void) {
  int finalRv = RH_OK;

  int rv;

  rv = InstallAspiDriverFiles();
  if (rv != RH_OK)
    return rv;
 
  rv = AdjustDllUseCount(STR_ASPI_DLL_NAME, 1);
  if (rv != RH_OK)
    return rv;

  rv = AdjustDllUseCount(STR_ASPI_SYS_NAME, 1);
  if (rv != RH_OK)
    return rv;

  rv = RegisterAspiService();
  if (rv != RH_OK)
    return rv;

  rv = AddAspiFilteredDrivers();
  if (rv != RH_OK)
    finalRv = rv;

  tstring appDir = GetAppDirectory();
  RecordAspiDriverInstallState(appDir.c_str(), DRIVER_INSTALLED);

  return finalRv;
}

LONG AddAspiFilteredDrivers(void) {
  int rv; 
  int finalRv = RH_OK;

  DebugMessage("-- Begin installation of key %S", KEY_DEVICE_CDROM);
  rv = AddFilteredDriver(KEY_DEVICE_CDROM,
                         GEARWORKS_REG_VALUE,
                         driverLoc_t(driverLoc_t::INSERT_AFTER_NODE,
                                     &REDBOOK_REG_VALUE));
  DebugMessage("-- Installation of key %S had rv %d", KEY_DEVICE_CDROM, rv);

  if (rv != RH_OK)
    finalRv = rv;

  DebugMessage("-- Begin installation of key %S", KEY_DEVICE_TAPE);
  rv = AddFilteredDriver(KEY_DEVICE_TAPE,
                         GEARWORKS_REG_VALUE,
                         driverLoc_t(driverLoc_t::INSERT_AT_FRONT));
  DebugMessage("-- Installation of key %S had rv %d", KEY_DEVICE_TAPE, rv);

  if (rv != RH_OK)
    finalRv = rv;

  DebugMessage("-- Begin installation of key %S", KEY_DEVICE_MEDIACHANGER);
  rv = AddFilteredDriver(KEY_DEVICE_MEDIACHANGER,
                         GEARWORKS_REG_VALUE,
                         driverLoc_t(driverLoc_t::INSERT_AT_FRONT));
  DebugMessage("-- Installation of key %S had rv %d", KEY_DEVICE_MEDIACHANGER,
   rv);

  if (rv != RH_OK)
    finalRv = rv;

  return finalRv;
}
 
// register the service
LONG RegisterAspiService(void) { 
  SC_HANDLE hSCM = OpenSCManager(NULL,                  // machine (local)
                                 NULL,                  // db (default)
                                 SC_MANAGER_ALL_ACCESS);
  if (!hSCM) {
    DoLogMessage(GetLastError(), _T("Open service control manager"));
    return RH_ERROR_OPENKEY;
  }

  TCHAR filepath[PATH_STORAGE];
  _sntprintf(filepath, MAX_PATH, _T("System32\\%s"), STR_ASPI_SYS_NAME);
  SC_HANDLE hService = CreateService(hSCM,
                                     STR_ASPI_SERVICE_NAME,
                                     _T("GEAR ASPI Filter Driver"),
                                     SC_MANAGER_ALL_ACCESS,
                                     SERVICE_KERNEL_DRIVER,
                                     SERVICE_DEMAND_START,
                                     SERVICE_ERROR_NORMAL,
                                     filepath,
                                     _T("PnP Filter"), // load order group
                                     NULL,             // tag id
                                     NULL,             // dependencies
                                     NULL,             // start (logon) name
                                     NULL);            // password
  if (hService) {
    CloseServiceHandle(hService);
  }
  else {
    DWORD lastError = GetLastError();
    if (lastError != ERROR_SERVICE_EXISTS) {
      DoLogMessage(GetLastError(), _T("Create service"));
      return RH_ERROR_SET_KEY;
    }
  }
  CloseServiceHandle(hSCM);

  return RH_OK;
}

LONG UnregisterAspiService(void) {
  SC_HANDLE hSCM = OpenSCManager(NULL,                  // machine (local)
                                 NULL,                  // db (default)
                                 SC_MANAGER_ALL_ACCESS);
  if (!hSCM) {
    DoLogMessage(GetLastError(), _T("Open service control manager"));
    return RH_ERROR_OPENKEY;
  }
  SC_HANDLE hService = OpenService(hSCM,
                                   STR_ASPI_SERVICE_NAME,
                                   SC_MANAGER_ALL_ACCESS);
  if (hService) {
    DeleteService(hService);
  }
  else {
    DWORD lastError = GetLastError();
    if (lastError != ERROR_INVALID_NAME) {
      DoLogMessage(GetLastError(), _T("Delete service"));
      return RH_ERROR_SET_KEY;
    }
  }

  CloseServiceHandle(hSCM);
  return RH_OK;
}

LONG RemoveAspiDriver(void) {
  int result = RH_OK;
  
  int dllCount, sysCount;

  result = AdjustDllUseCount(STR_ASPI_DLL_NAME, -1, &dllCount);
  if (result != RH_OK)
    return result;
  result = AdjustDllUseCount(STR_ASPI_SYS_NAME, -1, &sysCount);
  if (result != RH_OK) {
    // OMGWTFBBQ we failed to change the second one
    // let's try to change the first one back (but ignore whether we succeeded)
    AdjustDllUseCount(STR_ASPI_DLL_NAME, 1);
    return result;
  }

  // We do this early on so that we don't have to deal with the various places
  // that we return() out of this function; the fact that we called this
  // Remove() function for this installatiopn is generally good enough for
  // our purposes
  tstring appDir = GetAppDirectory();
  RecordAspiDriverInstallState(appDir.c_str(), DRIVER_REMOVED);

  if (dllCount > 0 || sysCount > 0) {
    // some other folks are using the driver
    return RH_SUCCESS_NOACTION;
  }

  result = UnregisterAspiService();

  if (result != RH_OK)
    DoLogMessage(result, _T("UnregisterAspiService() returned: "));

  std::list<LPCTSTR> regKeysToMunge;
  regKeysToMunge.push_back(KEY_DEVICE_CDROM);
  regKeysToMunge.push_back(KEY_DEVICE_TAPE);
  regKeysToMunge.push_back(KEY_DEVICE_MEDIACHANGER);

  for (std::list<LPCTSTR>::const_iterator itr = regKeysToMunge.begin();
       itr != regKeysToMunge.end();
       ++itr) {

    HKEY keyHandle;
    DWORD currentKeyType;
    DWORD dataBufferLen = DEFAULT_REG_BUFFER_SZ;
    LPTSTR data = (LPTSTR)malloc(dataBufferLen);

    memset(data, 0, dataBufferLen);

    LONG rv = RegOpenKeyEx(HKEY_LOCAL_MACHINE, // hKey
                           *itr,               // lpSubKey
                           0,                  // ulOptions
                           KEY_ALL_ACCESS,     // samDesired
                           &keyHandle);        // phkResult

    if (!LoggedSUCCEEDED(rv, _T("RegOpenKeyEx() on remove"))) {
      result = RH_ERROR_OPENKEY;
      free(data);
      continue;
    }

    rv = RegQueryValueEx(keyHandle,          // key handle
                         DRIVER_SUBKEY_STR,  // lpValueName
                         0,                  // lpReserved
                         &currentKeyType,    // lpType
                         (LPBYTE)data,       // lpData
                         &dataBufferLen);    // lpcbData

    if (!LoggedSUCCEEDED(rv, _T("RegQueryValueEx() on remove"))) {
      result = RH_ERROR_QUERY_VALUE;
      free(data);
      continue;
    }

    multiSzBuffer_t oldRegValue((BYTE*)data, dataBufferLen);

    regValueList_t regValues = ParseValues(oldRegValue, currentKeyType);

    regValueList_t::iterator gearworksNode = find(regValues.begin(),
                                                  regValues.end(),
                                                  GEARWORKS_REG_VALUE);

    if (gearworksNode == regValues.end()) {
      DebugMessage("Couldn't find gearworks value in key %S", *itr);
      free(data);
      continue;
    }

    regValues.erase(gearworksNode);

    multiSzBuffer_t newRegValue((LPBYTE)new char[DEFAULT_REG_BUFFER_SZ],
     DEFAULT_REG_BUFFER_SZ);

    MakeMultiSzRegString(newRegValue, regValues);

    rv = RegSetValueEx(keyHandle,
                       DRIVER_SUBKEY_STR,
                       0,
                       REG_MULTI_SZ,
                       newRegValue.value, 
                       newRegValue.cbBytes);

    if (!LoggedSUCCEEDED(rv, _T("RegSetValueEx() on remove"))) {
      free(data);
      delete [] newRegValue.value;
      return RH_ERROR_SET_KEY; 
    }

    rv = RegCloseKey(keyHandle);
    LoggedSUCCEEDED(rv, _T("RegCloseKey() failed"));
    free(data);
    delete [] newRegValue.value;
  }

  TCHAR existingDriverPath[PATH_STORAGE], windowsSystemDir[PATH_STORAGE];

  // Ensure these are 0s from the start...
  memset(existingDriverPath, 0, PATH_STORAGE);
  memset(windowsSystemDir, 0, PATH_STORAGE);

  HRESULT hr = SHGetFolderPath(NULL,
                               CSIDL_SYSTEM,
                               NULL,
                               SHGFP_TYPE_CURRENT,
                               windowsSystemDir);

  if (FAILED(hr) || hr == S_FALSE) {
    DoLogMessage(hr, _T("Get system directory"));
    return RH_ERROR_QUERY_KEY;
  }

  windowsSystemDir[MAX_PATH] = _T('\0'); // never trust 

  int len = _sntprintf(existingDriverPath, MAX_PATH, _T("%s\\%s"),
            windowsSystemDir, STR_ASPI_DLL_NAME);

  if (len < 0) {
    DebugMessage("Failed to construct GEARWORKS_ASPI_DRIVER path for delete");
    return RH_ERROR_QUERY_VALUE;
  }

  if (!LoggedDeleteFile(existingDriverPath))
    result = RH_ERROR_DELETEFILE_FAILED;

  memset(existingDriverPath, 0, PATH_STORAGE);

  len = _sntprintf(existingDriverPath, MAX_PATH, _T("%s\\%s"),
                   windowsSystemDir, STR_ASPI_SYS_NAME);

  if (len < 0) {
    DebugMessage("Failed to construct GEARWORKS_ASPIWDM_DRIVER path for delete");
    return RH_ERROR_QUERY_VALUE;
  }

  if (!LoggedDeleteFile(existingDriverPath))
    result = RH_ERROR_DELETEFILE_FAILED;

  return result;
}

// This function is a bit confusing; while it does delete files, it doesn't use
// DeleteFile() (as it originally did) because these drivers are likely to be
// in use when we're called; so, we have to instead use MoveFileEx()
BOOL LoggedDeleteFile(LPCTSTR file) {
  DebugMessage("Attempting to delete %S", file);
  BOOL succeeded;

  succeeded = DeleteFile(file);

  if (!succeeded) {
    DoLogMessage(GetLastError(), _T("DeleteFile() failed: "));

    succeeded = MoveFileEx(file,                          // lpExistingFileName
                           NULL,                          // lpNewFileName
                           MOVEFILE_DELAY_UNTIL_REBOOT);  // dwFlags
    if (!succeeded)
      DoLogMessage(GetLastError(), _T("MoveFileEx() failed: "));
  }

  return succeeded;
}

LONG RecordAspiDriverInstallState(LPCTSTR installationPath,
                                  install_type_t recordAction) {

  HKEY installationFlagHandle;
  DWORD keyCreationResult, currentKeyType;
  DWORD dwordSize = sizeof(DWORD);

  LONG rv = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                           SONGBIRD_CDRIP_REG_KEY_ROOT,
                           0,
                           NULL,
                           0,
                           KEY_ALL_ACCESS,
                           NULL,
                           &installationFlagHandle,
                           &keyCreationResult);

  if (!LoggedSUCCEEDED(rv, _T("RegCreateKeyEx() for driver installation count:")))
    return RH_ERROR_INIT_CREATEKEY;

  if (recordAction == DRIVER_REMOVED) {
    rv = RegDeleteValue(installationFlagHandle,
                        installationPath);

    RegCloseKey(installationFlagHandle);

    // We handle the RegDeleteKey() in the uninstaller...
    if (!LoggedSUCCEEDED(rv, _T("RegDeleteValue() for driver installation count:")))
      return RH_ERROR_DELETEKEY_FAILED; 
    else   
      return RH_OK;
  }

  // We don't execute any of this if we're called in DRIVER_REMOVED mode
  DWORD installationFlag = 1;
  rv = RegQueryValueEx(installationFlagHandle,
                       installationPath,
                       0,
                       &currentKeyType,
                       (LPBYTE)&installationFlag,
                       &dwordSize);

  if (SUCCEEDED(rv) && currentKeyType != REG_DWORD) {
    DebugMessage("Unexpected key type in installation flag key: type %d",
     currentKeyType);
  }

  rv = RegSetValueEx(installationFlagHandle,
                     installationPath,
                     0,
                     REG_DWORD,
                     (LPBYTE)&installationFlag,
                     sizeof(DWORD));

  RegCloseKey(installationFlagHandle);

  if (!LoggedSUCCEEDED(rv, _T("RegCreateKeyEx() for driver installation flag:")))
    return RH_ERROR_INIT_CREATEKEY;

  return RH_OK;
}

LONG CheckAspiDriversInstalled(LPCTSTR installationPath) {
  DWORD cdripInstalled = 0;
  DWORD currentKeyType;
  DWORD dwordSize = sizeof(DWORD);
  HKEY cdripKeyHandle;

  int rv = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        SONGBIRD_CDRIP_REG_KEY_ROOT,
                        NULL,
                        KEY_READ,
                        &cdripKeyHandle);

  if (ERROR_FILE_NOT_FOUND == rv)
    return RH_SUCCESS_CDRIP_NOT_INSTALLED;

  rv = RegQueryValueEx(cdripKeyHandle,
                       installationPath,
                       0,
                       &currentKeyType,
                       (LPBYTE)cdripInstalled,
                       &dwordSize);

  RegCloseKey(cdripKeyHandle);

  if (SUCCEEDED(rv)) {
    if (currentKeyType != REG_DWORD) { 
      DebugMessage("CheckAspiDriversInstalled(): invalid key type: %d",  
                  currentKeyType); 
      return RH_SUCCESS_CDRIP_NOT_INSTALLED;
    } 
    else if (1 == cdripInstalled) {
      return RH_OK;
    }
  }

  return RH_SUCCESS_CDRIP_NOT_INSTALLED;
}
