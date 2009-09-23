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

#include <windows.h>
#include <tchar.h>
#include <shellapi.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>

#include "reghandlers.h"
#include "stringconvert.h"
#include "debug.h"
#include "error.h"
#include "regparse.h"

/* WARNING: live ammunition... */

// LPCTSTR KEY_DEVICE_CDROM = _T("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E965-E325-11CE-BFC1-08002BE10318}");

// LPCTSTR KEY_DEVICE_TAPE = _T("SYSTEM\\CurrentControlSet\\Control\\Class\\{6D807884-7D21-11CF-801C-08002BE10318}");

// LPCTSTR KEY_DEVICE_MEDIACHANGER = _T("SYSTEM\\CurrentControlSet\\Control\\Class\\{CE5939AE-EBDE-11D0-B181-0000F8753EC4}");

/* end live ammunition... */

/* Testing Keys */

LPCTSTR KEY_DEVICE_CDROM = _T("SOFTWARE\\Songbird\\gearworks-test\\dkey1");
LPCTSTR KEY_DEVICE_TAPE = _T("SOFTWARE\\Songbird\\gearworks-test\\dkey2");
LPCTSTR KEY_DEVICE_MEDIACHANGER = _T("SOFTWARE\\Songbird\\gearworks-test\\dkey3");

/* End Testing Keys */

LPCTSTR DRIVER_SUBKEY_STR = _T("UpperFilters");

static const TCHAR GEARWORKS_REG_VALUE_STR[] = _T("GEARAspiWDM");
static const TCHAR REDBOOK_REG_VALUE_STR[] = _T("redbook");

static const size_t DEFAULT_REG_BUFFER_SZ = 4096;

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


int AddFilteredDriver(LPCTSTR regSubKey,
                      const regValue_t &newKey,
                      const driverLoc_t insertionInfo) {

  DWORD keyCreationResult;
  HKEY keyHandle;

  HRESULT rv = RegCreateKeyEx(HKEY_LOCAL_MACHINE,  // handle
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

  if (REG_CREATED_NEW_KEY == keyCreationResult) {
    DebugMessage("Created new key: %S", newKey.p);

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

    DWORD currentKeyType;
    DWORD dataBufferLen = DEFAULT_REG_BUFFER_SZ;
    LPTSTR data = (LPTSTR)malloc(dataBufferLen);

    memset(data, 0, dataBufferLen);

    rv = RegQueryValueEx(keyHandle,          // key handle
                         DRIVER_SUBKEY_STR,  // lpValueName
                         0,                  // lpReserved
                         &currentKeyType,    // lpType
                         (LPBYTE)data,       // lpData
                         &dataBufferLen);    // lpcbData

    if (!LoggedSUCCEEDED(rv, _T("RegQueryValueEx() failed"))) {
      free(data);
      return RH_ERROR_QUERY_KEY;
    }

    if (currentKeyType != REG_SZ &&
        currentKeyType != REG_EXPAND_SZ &&
        currentKeyType != REG_MULTI_SZ) {
      DebugMessage("Unknown type %d for key %s", currentKeyType, 
       KEY_DEVICE_CDROM);
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

int RegInstallKeys(void) {
  int finalRv = RH_OK;

  int rv = AddFilteredDriver(KEY_DEVICE_CDROM,
                             GEARWORKS_REG_VALUE,
                             driverLoc_t(driverLoc_t::INSERT_AFTER_NODE,
                                         &REDBOOK_REG_VALUE));
  DebugMessage("Addition of key %S had rv %d", KEY_DEVICE_CDROM, rv);

  if (rv != RH_OK)
    finalRv = rv;

  rv = AddFilteredDriver(KEY_DEVICE_TAPE,
                         GEARWORKS_REG_VALUE,
                         driverLoc_t(driverLoc_t::INSERT_AT_FRONT));
  DebugMessage("Addition of key %S had rv %d", KEY_DEVICE_CDROM, rv);

  if (rv != RH_OK)
    finalRv = rv;

  rv = AddFilteredDriver(KEY_DEVICE_MEDIACHANGER,
                         GEARWORKS_REG_VALUE,
                         driverLoc_t(driverLoc_t::INSERT_AT_FRONT));
  DebugMessage("Addition of key %S had rv %d", KEY_DEVICE_CDROM, rv);

  if (rv != RH_OK)
    finalRv = rv;

  return finalRv;
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
