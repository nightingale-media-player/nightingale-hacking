
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

/* WARNING: live amunition...

LPCTSTR DRIVER_KEY_ONE = _T("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E965-E325-11CE-BFC1-08002BE10318}");

LPCTSTR DRIVER_KEY_TWO = _T("\\SYSTEM\\CurrentControlSet\\Control\\Class\\{6D807884-7D21-11CF-801C-08002BE10318}");

LPCTSTR DRIVER_KEY_THREE = _T("HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\{CE5939AE-EBDE-11D0-B181-0000F8753EC4}");

*/

LPCTSTR DRIVER_KEY_ONE = _T("SOFTWARE\\Songbird\\gearworks-test\\dkey1");
LPCTSTR DRIVER_KEY_TWO = _T("SOFTWARE\\Songbird\\gearworks-test\\dkey2");
LPCTSTR DRIVER_KEY_THREE = _T("SOFTWARE\\Songbird\\gearworks-test\\dkey3");

LPCTSTR DRIVER_SUBKEY_STR = _T("UpperFilters");

const TCHAR GEARWORKS_REG_VALUE_STR[] = _T("GEARAspiWDM");
const TCHAR REDBOOK_REG_VALUE_STR[] = _T("redbook");

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

int main(int argc, LPTSTR *argv) {
  int result = 0;
  
  if (argc != 2) {
    OutputDebugString(_T("Incorrect number of arguments"));
    return RH_ERROR_USER;
  }

  std::string mode;
  mode.assign(ConvertUTFnToUTF8(argv[1]));

  if (mode == "install") {
    DWORD keyCreationResult;
    HKEY keyHandle;

    LONG rv = RegCreateKeyEx(HKEY_LOCAL_MACHINE,    // handle
                             DRIVER_KEY_ONE,        // lpSubKey
                             0,                     // dwReserved
                             NULL,                  // lpClass
                             0,                     // dwOptions
                             KEY_ALL_ACCESS,        // samDesired
                             NULL,                  // lpSecurityAttributes
                             &keyHandle,            // phkResult
                             &keyCreationResult);   // lpdwDisposition

    if (FAILED(rv)) {
      LPTSTR errStr = GetSystemErrorMessage(rv);
      DebugMessage("RegCreateKeyEx() on key one failed: %S", errStr);
      LocalFree(errStr);
      return RH_ERROR_INIT_CREATEKEY;
    }

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

      if (FAILED(rv)) {
        LPTSTR errStr = GetSystemErrorMessage(rv);
        DebugMessage("RegQueryValueEx() failed: %S", errStr);
        LocalFree(errStr);
        return RH_ERROR_QUERY_KEY;
      }

      if (currentKeyType != REG_SZ &&
       currentKeyType != REG_EXPAND_SZ &&
       currentKeyType != REG_MULTI_SZ) {
        DebugMessage("Unknown type %d for key %s", currentKeyType, 
         DRIVER_KEY_ONE);
        return RH_ERROR_UNKNOWN_KEY_TYPE; 
      }

      multiSzBuffer_t oldRegValue((BYTE*)data, dataBufferLen);

      regValueList_t regValues = ParseValues(oldRegValue, currentKeyType);

      regValueList_t::iterator redbookNode = find(regValues.begin(),
       regValues.end(), REDBOOK_REG_VALUE);

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

      if (FAILED(rv)) {
        LPTSTR errStr = GetSystemErrorMessage(rv);
        DebugMessage("RegSetValueEx() failed: %S", errStr);
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

    OutputDebugString(_T("Creating driver key 2"));

    rv = RegCreateKeyEx(HKEY_LOCAL_MACHINE,    // handle
                        DRIVER_KEY_TWO,        // lpSubKey
                        0,                     // dwReserved
                        NULL,                  // lpClass
                        0,                     // dwOptions
                        KEY_ALL_ACCESS,        // samDesired
                        NULL,                  // lpSecurityAttributes
                        &keyHandle,            // phkResult
                        &keyCreationResult);   // lpdwDisposition

    if (FAILED(rv)) {
      LPTSTR errStr = GetSystemErrorMessage(rv);
      DebugMessage("RegCreateKeyEx() on key two failed: %S", errStr);
      LocalFree(errStr);
      return RH_ERROR_INIT_CREATEKEY;
    }

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

      if (FAILED(rv)) {
        LPTSTR errStr = GetSystemErrorMessage(rv);
        DebugMessage("RegSetValueEx() failed on key two: %S", errStr);
        LocalFree(errStr);
      }

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
         DRIVER_KEY_ONE);
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

    rv = RegCreateKeyEx(HKEY_LOCAL_MACHINE,    // handle
                        DRIVER_KEY_THREE,      // lpSubKey
                        0,                     // dwReserved
                        NULL,                  // lpClass
                        0,                     // dwOptions
                        KEY_ALL_ACCESS,        // samDesired
                        NULL,                  // lpSecurityAttributes
                        &keyHandle,            // phkResult
                        &keyCreationResult);   // lpdwDisposition

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
         DRIVER_KEY_ONE);
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
  else if (mode == "upgrade") {
    OutputDebugString(_T("Upgrade mode"));
  } else if ("remove" == mode) {
    OutputDebugString(_T("Remove mode"));
  } else {
    OutputDebugString(_T("Unknown mode"));
    return RH_ERROR_USER;
  }

  return RH_OK;
}
