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
#include <vector>

#include <windows.h>
#include <tchar.h>
#include <shellapi.h>
#include <shlobj.h>
#include <stdlib.h>

#include "reghandlers.h"
#include "stringconvert.h"
#include "debug.h"
#include "error.h"
#include "regparse.h"

#ifndef RH_REGKEY_ENGAGE_SAFETY
  /* WARNING: live ammunition... */
  LPCTSTR KEY_SHARED_DLLS =
    _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedDLLs");
  LPCTSTR STR_API_DLL_NAME =
  _T("GEARAspi.dll");
  LPCTSTR STR_API_SYS_NAME =
   _T("Drivers\\GEARAspiWDM.sys");
  LPCTSTR STR_SERVICE_NAME =
   _T("GEARAspiWDM");
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
  LPCTSTR STR_API_DLL_NAME =
   _T("GEARAspi.dll");
  LPCTSTR STR_API_SYS_NAME =
   _T("Drivers\\GEARAspiWDM.sys");
  LPCTSTR STR_SERVICE_NAME =
   _T("GEARAspiWDM");
  LPCTSTR KEY_DEVICE_CDROM =
   _T("SOFTWARE\\Songbird\\gearworks-test\\dkey1");
  LPCTSTR KEY_DEVICE_TAPE =
   _T("SOFTWARE\\Songbird\\gearworks-test\\dkey2");
  LPCTSTR KEY_DEVICE_MEDIACHANGER =
   _T("SOFTWARE\\Songbird\\gearworks-test\\dkey3");
#endif

LPCTSTR DRIVER_SUBKEY_STR = _T("UpperFilters");

static const TCHAR GEARWORKS_REG_VALUE_STR[] = _T("GEARAspiWDM");
static const TCHAR REDBOOK_REG_VALUE_STR[] = _T("redbook");

static const size_t DEFAULT_REG_BUFFER_SZ = 4096;

// The second arguments to these constructors look scary, but they're just
// non-function call versions of _tcslen()

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
  } else {
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

int AdjustDllUseCount(LPCTSTR dllName, int value, int* result = NULL) {
  HRESULT hr;
  LONG ret;
  const size_t pathStorage = MAX_PATH + 1;

  TCHAR sysDir[pathStorage], nameBuffer[pathStorage];

  hr = SHGetFolderPath(NULL, CSIDL_SYSTEM, NULL, SHGFP_TYPE_CURRENT, sysDir);

  if (FAILED(hr) || hr == S_FALSE) {
    DoLogMessage(hr, _T("Get system directory"));
    return RH_ERROR_QUERY_KEY;
  }

  memset(sysDir, 0, pathStorage); // never trust

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
  ret = RegQueryValueEx(hKeySharedDLLs, nameBuffer, NULL, NULL,
                        (LPBYTE)&dllCount, &bufferSize);
  if (ret == ERROR_FILE_NOT_FOUND) {
    dllCount = 0;
  } else if (!LoggedSUCCEEDED(ret, _T("Get dll refcount"))) {
    return RH_ERROR_QUERY_VALUE;
  }
  dllCount += value;
  if (dllCount != 0) {
    ret = RegSetValueEx(hKeySharedDLLs, nameBuffer, NULL, REG_DWORD,
                        (LPBYTE)&dllCount, sizeof(DWORD));
  } else {
    ret = RegDeleteValue(hKeySharedDLLs, nameBuffer);
  }
  if (!LoggedSUCCEEDED(ret, _T("Set dll refcount")))
    return RH_ERROR_SETVALUE;

  RegCloseKey(hKeySharedDLLs);

  if (result)
    *result = value;

  return RH_OK;
}

int AddFilteredDriver(LPCTSTR regSubKey,
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

  } else if (REG_OPENED_EXISTING_KEY == keyCreationResult) {
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

        regValueList_t::iterator insertAfterNode = find(regValues.begin(),
         regValues.end(), *insertionInfo.loc);

        // If we didn't find the node we were looking for, we default to 
        // inserting it up front
        if (insertAfterNode != regValues.end()) {
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
        regValues.push_front(newKey);
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
  } else {
    OutputDebugString(_T("Unknown creationResult"));
  }

  return RH_OK;
}

int InstallAspiDriver(void) {
  int finalRv = RH_OK;

  int rv;
  
  rv = AdjustDllUseCount(STR_API_DLL_NAME, 1);
  if (rv != RH_OK)
    return rv;
  rv = AdjustDllUseCount(STR_API_SYS_NAME, 1);
  if (rv != RH_OK)
    return rv;

  // register the service
  { /* scope */
    SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCM) {
      DoLogMessage(GetLastError(), _T("Open service control manager"));
      return RH_ERROR_OPENKEY;
    }
    TCHAR filepath[MAX_PATH+1];
    _sntprintf(filepath, MAX_PATH, _T("System32\\%s"), STR_API_SYS_NAME);
    SC_HANDLE hService = CreateService(hSCM,
                                       STR_SERVICE_NAME,
                                       _T("GEAR ASPI Filter Driver"),
                                       SC_MANAGER_ALL_ACCESS,
                                       SERVICE_KERNEL_DRIVER,
                                       SERVICE_DEMAND_START,
                                       SERVICE_ERROR_NORMAL,
                                       filepath,
                                       _T("PnP Filter"),
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL);
    if (hService) {
      CloseServiceHandle(hService);
    } else {
      DWORD lastError = GetLastError();
      if (lastError != ERROR_SERVICE_EXISTS) {
        DoLogMessage(GetLastError(), _T("Create service"));
        return RH_ERROR_SET_KEY;
      }
    }
    CloseServiceHandle(hSCM);
  } /* end scope */

  DebugMessage("Begin installation of key %S", KEY_DEVICE_CDROM);
  rv = AddFilteredDriver(KEY_DEVICE_CDROM,
                         GEARWORKS_REG_VALUE,
                         driverLoc_t(driverLoc_t::INSERT_AFTER_NODE,
                                     &REDBOOK_REG_VALUE));
  DebugMessage("Installation of key %S had rv %d", KEY_DEVICE_CDROM, rv);

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

int RemoveAspiDriver(void) {
  int result = RH_OK;
  
  int dllCount, sysCount;
  result = AdjustDllUseCount(STR_API_DLL_NAME, -1, &dllCount);
  if (result != RH_OK)
    return result;
  result = AdjustDllUseCount(STR_API_SYS_NAME, -1, &sysCount);
  if (result != RH_OK)
    return result;

  if (dllCount > 0 || sysCount > 0) {
    // some other folks are using the driver
    return RH_SUCCESS_NOACTION;
  }

  // unregister the service
  { /* scope */
    SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCM) {
      DoLogMessage(GetLastError(), _T("Open service control manager"));
      return RH_ERROR_OPENKEY;
    }
    SC_HANDLE hService = OpenService(hSCM,
                                     STR_SERVICE_NAME,
                                     SC_MANAGER_ALL_ACCESS);
    if (hService) {
      DeleteService(hService);
    } else {
      DWORD lastError = GetLastError();
      if (lastError != ERROR_INVALID_NAME) {
        DoLogMessage(GetLastError(), _T("Delete service"));
        return RH_ERROR_SET_KEY;
      }
    }
    CloseServiceHandle(hSCM);
  } /* end scope */

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

  return result;
}
