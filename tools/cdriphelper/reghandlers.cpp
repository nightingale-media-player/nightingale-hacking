
#include <windows.h>
#include <tchar.h>
#include <shellapi.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>

#include "stringconvert.h"
#include "debug.h"
#include "error.h"
#include "regparse.h"

/*
 * disthelper
 * usage: $0 <mode> [<ini file>]
 * the mode is used to find the [steps:<mode>] section in the ini file
 * steps are sorted in ascii order.  (not numerical order, 2 > 10!)
 * if the ini file is not given, the environment variable DISTHELPER_DISTINI 
 * is used (must be a platform-native absolute path)
 */

/* WARNING: live ammunition...

LPCTSTR KEY_DEVICE_CDROM = _T("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E965-E325-11CE-BFC1-08002BE10318}");

LPCTSTR KEY_DEVICE_TAPE = _T("SYSTEM\\CurrentControlSet\\Control\\Class\\{6D807884-7D21-11CF-801C-08002BE10318}");

LPCTSTR KEY_DEVICE_MEDIACHANGER = _T("SYSTEM\\CurrentControlSet\\Control\\Class\\{CE5939AE-EBDE-11D0-B181-0000F8753EC4}");

*/

LPCTSTR KEY_DEVICE_CDROM = _T("SOFTWARE\\Songbird\\gearworks-test\\dkey1");
LPCTSTR KEY_DEVICE_TAPE = _T("SOFTWARE\\Songbird\\gearworks-test\\dkey2");
LPCTSTR KEY_DEVICE_MEDIACHANGER = _T("SOFTWARE\\Songbird\\gearworks-test\\dkey3");

LPCTSTR DRIVER_SUBKEY_STR = _T("UpperFilters");

static const TCHAR GEARWORKS_REG_VALUE_STR[] = _T("GEARAspiWDM");
static const TCHAR REDBOOK_REG_VALUE_STR[] = _T("redbook");

// The second arguments to these constructors look scary, but they're just
// non-function call versions of _tcslen()

static const regValue_t GEARWORKS_REG_VALUE(GEARWORKS_REG_VALUE_STR,
 sizeof(GEARWORKS_REG_VALUE_STR)/sizeof(GEARWORKS_REG_VALUE_STR[0]) - 1);

static const regValue_t REDBOOK_REG_VALUE(REDBOOK_REG_VALUE_STR,
 sizeof(REDBOOK_REG_VALUE_STR)/sizeof(REDBOOK_REG_VALUE_STR[0]) - 1);

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

BOOL LoggedSUCCEEDED(HRESULT rv, LPCTSTR message) {
  BOOL callSucceeded = SUCCEEDED(rv);

  if (!callSucceeded) {
    LPTSTR errStr = GetSystemErrorMessage(rv);
    DebugMessage("%S: %S", message, errStr);
    LocalFree(errStr);
  }
  return callSucceeded;
}

int RegInstallKeys() {
  DWORD keyCreationResult;
  HKEY keyHandle;

  LONG rv = RegCreateKeyEx(HKEY_LOCAL_MACHINE,    // handle
                           KEY_DEVICE_CDROM,      // lpSubKey
                           0,                     // dwReserved
                           NULL,                  // lpClass
                           0,                     // dwOptions
                           KEY_ALL_ACCESS,        // samDesired
                           NULL,                  // lpSecurityAttributes
                           &keyHandle,            // phkResult
                           &keyCreationResult);   // lpdwDisposition

  if (!LoggedSUCCEEDED(rv, _T("RegCreateKeyEx() on key one failed")))
    return RH_ERROR_INIT_CREATEKEY;

  if (REG_CREATED_NEW_KEY == keyCreationResult) {
    OutputDebugString(_T("Created new key one"));

    regValueList_t newRegValueList;
    newRegValueList.insert(newRegValueList.begin(), GEARWORKS_REG_VALUE);

    multiSzBuffer_t newRegValue((LPBYTE)new char[4096], 4096);
    MakeMultiSzRegString(newRegValue, newRegValueList);

    rv = RegSetValueEx(keyHandle,             // key handle
                       DRIVER_SUBKEY_STR,     // lpValueName
                       0,                     // reserved
                       REG_MULTI_SZ,          // dwType
                       newRegValue.value,     // data
                       newRegValue.cbBytes);  // cbData

    if (FAILED(rv)) {
      LPTSTR errStr = GetSystemErrorMessage(rv);
      DebugMessage("RegSetValueEx() failed: %S", errStr);
      LocalFree(errStr);
    }

    rv = RegCloseKey(keyHandle);
    if (FAILED(rv)) {
      LPTSTR errStr = GetSystemErrorMessage(rv);
      DebugMessage("RegCloseKey() failed: %S", errStr);
      LocalFree(errStr);
    }
  } else if (REG_OPENED_EXISTING_KEY == keyCreationResult) {
    OutputDebugString(_T("Opened old key one"));

    DWORD currentKeyType;
    DWORD dataBufferLen = 4096;
    LPTSTR data = (LPTSTR)malloc(dataBufferLen);

    memset(data, 0, dataBufferLen);

    rv = RegQueryValueEx(keyHandle,          // key handle
                         DRIVER_SUBKEY_STR,  // lpValueName
                         0,                  // lpReserved
                         &currentKeyType,    // lpType
                         (LPBYTE)data,       // lpData
                         &dataBufferLen);    // lpcbData

    if (!LoggedSUCCEEDED(rv, _T("RegQueryValueEx() failed")))
      return RH_ERROR_QUERY_KEY;

    if (currentKeyType != REG_SZ &&
        currentKeyType != REG_EXPAND_SZ &&
        currentKeyType != REG_MULTI_SZ) {
      DebugMessage("Unknown type %d for key %s", currentKeyType, 
       KEY_DEVICE_CDROM);
      return RH_ERROR_UNKNOWN_KEY_TYPE; 
    }

    multiSzBuffer_t oldRegValue((BYTE*)data, dataBufferLen);

    regValueList_t regValues = ParseValues(oldRegValue, currentKeyType);

    regValueList_t::iterator redbookNode = find(regValues.begin(), regValues.end(), REDBOOK_REG_VALUE);

    // We didn't find the redbook node, so put our filter driver first.
    if (redbookNode == regValues.end()) {
      regValues.push_front(GEARWORKS_REG_VALUE);
    } else {
      // insert() inserts to the node _before_; we want to be inserted
      // after that value, so increment the iterator; we shouldn't fall off 
      // the end because we test for that above.
      redbookNode++;
      regValues.insert(redbookNode, GEARWORKS_REG_VALUE);
    }

    multiSzBuffer_t newRegValue((LPBYTE)new char[4096], 4096);

    // Does this modify cbBytes to be the correct number of bytes?
    MakeMultiSzRegString(newRegValue, regValues);

    rv = RegSetValueEx(keyHandle,
                       DRIVER_SUBKEY_STR,
                       0,
                       REG_MULTI_SZ,
                       newRegValue.value, 
                       newRegValue.cbBytes);

    if (!LoggedSUCCEEDED(rv, _T("RegSetValueEx() failed")))
      return RH_ERROR_SETVALUE;

    rv = RegCloseKey(keyHandle);
    LoggedSUCCEEDED(rv, _T("RegCloseKey() failed"));
  } else {
    OutputDebugString(_T("Unknown creationResult"));
  }

  OutputDebugString(_T("Creating driver key 2"));

  rv = RegCreateKeyEx(HKEY_LOCAL_MACHINE,    // handle
                      KEY_DEVICE_TAPE,       // lpSubKey
                      0,                     // dwReserved
                      NULL,                  // lpClass
                      0,                     // dwOptions
                      KEY_ALL_ACCESS,        // samDesired
                      NULL,                  // lpSecurityAttributes
                      &keyHandle,            // phkResult
                      &keyCreationResult);   // lpdwDisposition

  if (!LoggedSUCCEEDED(rv, _T("RegCreateKeyEx() on key two failed")))
    return RH_ERROR_INIT_CREATEKEY;

  if (REG_CREATED_NEW_KEY == keyCreationResult) {
    OutputDebugString(_T("Created new key two"));

    regValueList_t newRegValueList;
    newRegValueList.insert(newRegValueList.begin(), GEARWORKS_REG_VALUE);

    multiSzBuffer_t newRegValue((LPBYTE)new char[4096], 4096);
    MakeMultiSzRegString(newRegValue, newRegValueList);

    rv = RegSetValueEx(keyHandle,             // key handle
                       DRIVER_SUBKEY_STR,     // lpValueName
                       0,                     // reserved
                       REG_MULTI_SZ,          // dwType
                       newRegValue.value,     // data
                       newRegValue.cbBytes);  // cbData

    LoggedSUCCEEDED(rv, _T("RegSetValueEx() failed on key two"));

  } else if (REG_OPENED_EXISTING_KEY == keyCreationResult) {
    OutputDebugString(_T("Opened old key two"));

    DWORD currentKeyType;
    DWORD dataBufferLen = 4096;
    LPTSTR data = (LPTSTR)malloc(dataBufferLen);

    memset(data, 0, dataBufferLen);

    rv = RegQueryValueEx(keyHandle,          // key handle
                         DRIVER_SUBKEY_STR,  // lpValueName
                         0,                  // lpReserved
                         &currentKeyType,    // lpType
                         (LPBYTE)data,       // lpData
                         &dataBufferLen);    // lpcbData

    if (FAILED(rv)) {
      LPTSTR errStr = GetSystemErrorMessage(rv);
      DebugMessage("RegQueryValueEx() on key two failed: %S", errStr);
      LocalFree(errStr);
      return RH_ERROR_QUERY_KEY;
    }

    if (currentKeyType != REG_SZ &&
        currentKeyType != REG_EXPAND_SZ &&
        currentKeyType != REG_MULTI_SZ) {
      DebugMessage("Unknown type %d for key %s", currentKeyType, 
       KEY_DEVICE_CDROM);
      return RH_ERROR_UNKNOWN_KEY_TYPE;
    }

    multiSzBuffer_t oldRegValue((BYTE*)data, dataBufferLen);

    regValueList_t regValues = ParseValues(oldRegValue, currentKeyType);
    regValues.push_front(GEARWORKS_REG_VALUE);

    multiSzBuffer_t newRegValue((LPBYTE)new char[4096], 4096);

    MakeMultiSzRegString(newRegValue, regValues);

    rv = RegSetValueEx(keyHandle,
                       DRIVER_SUBKEY_STR,
                       0,
                       REG_MULTI_SZ,
                       newRegValue.value, 
                       newRegValue.cbBytes);

    if (FAILED(rv)) {
      LPTSTR errStr = GetSystemErrorMessage(rv);
      DebugMessage("RegSetValueEx() failed on key two: %S", errStr);
      LocalFree(errStr);
      return RH_ERROR_SETVALUE;
    }

    rv = RegCloseKey(keyHandle);
    if (FAILED(rv)) {
      LPTSTR errStr = GetSystemErrorMessage(rv);
      DebugMessage("RegCloseKey() failed: %S", errStr);
      LocalFree(errStr);
    }

  } else {
    OutputDebugString(_T("Unknown creationResult"));
  }

  OutputDebugString(_T("Creating driver key 3"));

  rv = RegCreateKeyEx(HKEY_LOCAL_MACHINE,       // handle
                      KEY_DEVICE_MEDIACHANGER,  // lpSubKey
                      0,                        // dwReserved
                      NULL,                     // lpClass
                      0,                        // dwOptions
                      KEY_ALL_ACCESS,           // samDesired
                      NULL,                     // lpSecurityAttributes
                      &keyHandle,               // phkResult
                      &keyCreationResult);      // lpdwDisposition

  if (FAILED(rv)) {
    LPTSTR errStr = GetSystemErrorMessage(rv);
    DebugMessage("RegCreateKeyEx() on key three failed: %S", errStr);
    LocalFree(errStr);
    return RH_ERROR_INIT_CREATEKEY;
  }

  if (REG_CREATED_NEW_KEY == keyCreationResult) {
    OutputDebugString(_T("Created new key three"));

    regValueList_t newRegValueList;
    newRegValueList.insert(newRegValueList.begin(), GEARWORKS_REG_VALUE);

    multiSzBuffer_t newRegValue((LPBYTE)new char[4096], 4096);
    MakeMultiSzRegString(newRegValue, newRegValueList);

    rv = RegSetValueEx(keyHandle,             // key handle
                       DRIVER_SUBKEY_STR,     // lpValueName
                       0,                     // reserved
                       REG_MULTI_SZ,          // dwType
                       newRegValue.value,     // data
                       newRegValue.cbBytes);  // cbData

    if (FAILED(rv)) {
      LPTSTR errStr = GetSystemErrorMessage(rv);
      DebugMessage("RegSetValueEx() failed on key three: %S", errStr);
      LocalFree(errStr);
    }
  } else if (REG_OPENED_EXISTING_KEY == keyCreationResult) {
    OutputDebugString(_T("Opened old key three"));

    DWORD currentKeyType;
    DWORD dataBufferLen = 4096;
    LPTSTR data = (LPTSTR)malloc(dataBufferLen);

    memset(data, 0, dataBufferLen);

    rv = RegQueryValueEx(keyHandle,          // key handle
                         DRIVER_SUBKEY_STR,  // lpValueName
                         0,                  // lpReserved
                         &currentKeyType,    // lpType
                         (LPBYTE)data,       // lpData
                         &dataBufferLen);    // lpcbData

    if (FAILED(rv)) {
      LPTSTR errStr = GetSystemErrorMessage(rv);
      DebugMessage("RegQueryValueEx() on key three failed: %S", errStr);
      LocalFree(errStr);
      return RH_ERROR_QUERY_KEY;
    }

    if (currentKeyType != REG_SZ &&
     currentKeyType != REG_EXPAND_SZ &&
     currentKeyType != REG_MULTI_SZ) {
      DebugMessage("Unknown type %d for key %s", currentKeyType, 
       KEY_DEVICE_CDROM);
      return RH_ERROR_UNKNOWN_KEY_TYPE; 
    }

    multiSzBuffer_t oldRegValue((BYTE*)data, dataBufferLen);

    regValueList_t regValues = ParseValues(oldRegValue, currentKeyType);
    regValues.push_front(GEARWORKS_REG_VALUE);

    multiSzBuffer_t newRegValue((LPBYTE)new char[4096], 4096);

    MakeMultiSzRegString(newRegValue, regValues);

    rv = RegSetValueEx(keyHandle,
                       DRIVER_SUBKEY_STR,
                       0,
                       REG_MULTI_SZ,
                       newRegValue.value, 
                       newRegValue.cbBytes);

    if (FAILED(rv)) {
      LPTSTR errStr = GetSystemErrorMessage(rv);
      DebugMessage("RegSetValueEx() failed on key three: %S", errStr);
      LocalFree(errStr);
      return RH_ERROR_SETVALUE;
    }

  } else {
    OutputDebugString(_T("Unknown creationResult"));
  }
  rv = RegCloseKey(keyHandle);
  if (FAILED(rv)) {
    LPTSTR errStr = GetSystemErrorMessage(rv);
    DebugMessage("RegCloseKey() failed: %S", errStr);
    LocalFree(errStr);
  }
}

int RegRemoveKeys(void) {
  int result = RH_OK;

  std::list<LPCTSTR> regKeysToMunge;
  regKeysToMunge.push_back(KEY_DEVICE_CDROM);
  regKeysToMunge.push_back(KEY_DEVICE_TAPE);
  regKeysToMunge.push_back(KEY_DEVICE_MEDIACHANGER);

  for (std::list<LPCTSTR>::const_iterator itr = regKeysToMunge.begin();
       itr != regKeysToMunge.end();
       ++itr) {

    HKEY keyHandle;
    DWORD currentKeyType;
    DWORD dataBufferLen = 4096;
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

    multiSzBuffer_t newRegValue((LPBYTE)new char[4096], 4096);

    MakeMultiSzRegString(newRegValue, regValues);

    rv = RegSetValueEx(keyHandle,
                       DRIVER_SUBKEY_STR,
                       0,
                       REG_MULTI_SZ,
                       newRegValue.value, 
                       newRegValue.cbBytes);

    LoggedSUCCEEDED(rv, _T("RegSetValueEx() on remove"));
    free(data);
    free(newRegValue.value);
  }

  return result;
}
