/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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

//------------------------------------------------------------------------------
//
// Songbird Windows formatter component COM server.
//
//------------------------------------------------------------------------------

/**
 * \file  sbWindowsFormatterServer.cpp
 * \brief Songbird Windows Formatter Component COM Server Source.
 */

//------------------------------------------------------------------------------
//
// Songbird Windows formatter component COM server imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbWindowsFormatterServer.h"

// Local imports.
#include "sbWindowsFormatter.h"
#include "sbWindowsFormatterClassFactory.h"

// Songbird imports.
#include <sbWindowsUtils.h>


//------------------------------------------------------------------------------
//
// Songbird Windows formatter component COM server globals.
//
//------------------------------------------------------------------------------

//
// gCOMServerLockCount          Count of the number of locks on the COM server.
// gDLLModule                   Component DLL module.
//

UINT      gCOMServerLockCount = 0;
HINSTANCE gDLLModule = NULL;


//------------------------------------------------------------------------------
//
// Songbird Windows formatter component COM server DLL services.
//
//------------------------------------------------------------------------------

/**
 * Handle the DLL main entry point callback for the DLL specified by aModule.
 * The reason for invoking the callback is specified by aReasonForCall.
 *
 * \param aModule               A handle to the DLL module.
 * \param aReasonForCall        The reason code that indicates why the DLL
 *                              entry-point function is being called.
 *
 * \return                      True on success; false otherwise.
 */

BOOL APIENTRY
DllMain(HINSTANCE aModule,
        DWORD     aReasonForCall,
        LPVOID    aReserved)
{
  // Dispatch processing of call.
  switch (aReasonForCall)
  {
    case DLL_PROCESS_ATTACH :
      // Get the DLL module.
      gDLLModule = aModule;

      // Don't need thread notification callbacks.
      DisableThreadLibraryCalls(aModule);

      break;
  }

  return TRUE;
}


/**
 * Determine whether the DLL can be unloaded.
 *
 * \return S_OK                 DLL can be unloaded.
 *         S_FALSE              DLL cannot be unloaded.
 */

STDAPI
DllCanUnloadNow()
{
  // Can only unload DLL if COM server is not locked.
  if (gCOMServerLockCount > 0)
    return S_FALSE;
  return S_OK;
}


/**
 * Return in aClassObject the class object for the class specified by aCLSID
 * with the interface specified by aIID.
 *
 * \param aCLSID                ID of requested class object.
 * \param aIID                  ID of interface to use with class object.
 * \param aClassObject          Returned class object.
 */

STDAPI
DllGetClassObject(REFCLSID aCLSID,
                  REFIID   aIID,
                  void**   aClassObject)
{
  HRESULT result;

  // Only sbWindowsFormatter objects are supported.
  if (!InlineIsEqualGUID(aCLSID, CLSID_sbWindowsFormatter))
    return CLASS_E_CLASSNOTAVAILABLE;

  // Validate return arguments.
  if (IsBadWritePtr(aClassObject, sizeof(void*)))
    return E_POINTER;

  // Set result to NULL.
  *aClassObject = NULL;

  // Create a new addref'ed sbWindowsFormatter object and set it up for
  // auto-release.
  sbWindowsFormatterClassFactory* factory = new sbWindowsFormatterClassFactory;
  if (!factory)
    return E_OUTOFMEMORY;
  factory->AddRef();
  sbAutoIUnknown autoFactory(factory);

  // Get the requested interface.
  result = factory->QueryInterface(aIID, aClassObject);
  if (!SUCCEEDED(result))
    return result;

  return S_OK;
}


/**
 * Register the COM server.
 */

STDAPI
DllRegisterServer()
{
  TCHAR appIDGUID[] = _T(CLSIDStr_sbWindowsFormatter);
  LONG  result;

  // Get the COM server DLL module path.
  TCHAR dllModulePath[MAX_PATH];
  DWORD length;
  length = GetModuleFileName(gDLLModule, dllModulePath, MAX_PATH);
  if ((length <= 0) || (length == MAX_PATH))
    return HRESULT_FROM_WIN32(GetLastError());


  //
  // HKCR\CLSID\SB_WINDOWS_FORMATTER_CLSID
  //

  // Create the Songbird Windows formatter class ID registry key and set it up
  // to auto-close.
  HKEY clsidKey;
  result = RegCreateKeyEx(HKEY_CLASSES_ROOT,
                          _T("CLSID\\") _T(CLSIDStr_sbWindowsFormatter),
                          0,
                          NULL,
                          REG_OPTION_NON_VOLATILE,
                          KEY_SET_VALUE | KEY_CREATE_SUB_KEY,
                          NULL,
                          &clsidKey,
                          NULL);
  if (result != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(result);
  sbAutoRegKey autoCLSIDKey(clsidKey);

  // Set the class description.
  TCHAR classDescription[] = _T("sbWindowsFormatter class");
  result = RegSetValueEx(clsidKey,
                         NULL,
                         0,
                         REG_SZ,
                         reinterpret_cast<const BYTE*>(classDescription),
                         sizeof(classDescription));
  if (result != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(result);

  // Set the application ID GUID.
  result = RegSetValueEx(clsidKey,
                         _T("AppID"),
                         0,
                         REG_SZ,
                         reinterpret_cast<const BYTE*>(appIDGUID),
                         sizeof(appIDGUID));
  if (result != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(result);

  // Set the localized string.  This must be a MUI string.  If not, elevation
  // will fail with an invalid data error.
  TCHAR localizedString[MAX_PATH + 32];
  StringCchPrintf(localizedString,
                  sizeof(localizedString) / sizeof(TCHAR),
                  _T("@%s,-1000"),
                  dllModulePath);
  result = RegSetValueExW(clsidKey,
                          _T("LocalizedString"),
                          0,
                          REG_SZ,
                          reinterpret_cast<const BYTE*>(localizedString),
                          (_tcsclen(localizedString) + 1) * sizeof(TCHAR));
  if (result != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(result);


  //
  // HKCR\CLSID\SB_WINDOWS_FORMATTER_CLSID\InProcServer32
  //

  // Create the in-process server key and set it up to auto-close.
  HKEY inProcServerKey;
  result = RegCreateKeyEx(clsidKey,
                          _T("InProcServer32"),
                          0,
                          NULL,
                          REG_OPTION_NON_VOLATILE,
                          KEY_SET_VALUE,
                          NULL,
                          &inProcServerKey,
                          NULL);
  if (result != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(result);
  sbAutoRegKey autoInProcServerKey(inProcServerKey);

  // Set the COM server DLL module path in the registry.
  result = RegSetValueEx(inProcServerKey,
                         NULL,
                         0,
                         REG_SZ,
                         reinterpret_cast<const BYTE*>(dllModulePath),
                         sizeof(TCHAR) * (lstrlen(dllModulePath) + 1));
  if (result != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(result);

  // Set the COM server threading model.
  TCHAR threadingModel[] = _T("Apartment");
  result = RegSetValueEx(inProcServerKey,
                         _T("ThreadingModel"),
                         0,
                         REG_SZ,
                         reinterpret_cast<const BYTE*>(threadingModel),
                         sizeof(threadingModel));
  if (result != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(result);


  //
  // HKCR\CLSID\SB_WINDOWS_FORMATTER_CLSID\Elevation
  //

  // Create the elevation key and set it up to auto-close.
  HKEY elevationKey;
  result = RegCreateKeyEx(clsidKey,
                          _T("Elevation"),
                          0,
                          NULL,
                          REG_OPTION_NON_VOLATILE,
                          KEY_SET_VALUE,
                          NULL,
                          &elevationKey,
                          NULL);
  if (result != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(result);
  sbAutoRegKey autoElevationKey(elevationKey);

  // Set elevation enabled.
  DWORD elevationEnabled = 1;
  result = RegSetValueEx(elevationKey,
                         _T("Enabled"),
                         0,
                         REG_DWORD,
                         reinterpret_cast<const BYTE*>(&elevationEnabled),
                         sizeof(elevationEnabled));
  if (result != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(result);


  //
  // HKCR\CLSID\SB_WINDOWS_FORMATTER_CLSID\TypeLib
  //

  // Create the type library key and set it up to auto-close.
  HKEY typeLibKey;
  result = RegCreateKeyEx(clsidKey,
                          _T("TypeLib"),
                          0,
                          NULL,
                          REG_OPTION_NON_VOLATILE,
                          KEY_SET_VALUE,
                          NULL,
                          &typeLibKey,
                          NULL);
  if (result != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(result);
  sbAutoRegKey autoTypeLibKey(typeLibKey);

  // Set type library ID.
  TCHAR typeLibID[] = _T(LIBIDStr_sbWindowsFormatterLibrary);
  result = RegSetValueEx(typeLibKey,
                         NULL,
                         0,
                         REG_SZ,
                         reinterpret_cast<const BYTE*>(typeLibID),
                         sizeof(typeLibID));
  if (result != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(result);


  //
  // HKCR\AppID\SB_WINDOWS_FORMATTER_CLSID
  //

  // Create the Songbird Windows formatter application ID GUID registry key and
  // set it up to auto-close.
  HKEY appIDGUIDKey;
  result = RegCreateKeyEx(HKEY_CLASSES_ROOT,
                          _T("AppID\\") _T(CLSIDStr_sbWindowsFormatter),
                          0,
                          NULL,
                          REG_OPTION_NON_VOLATILE,
                          KEY_SET_VALUE | KEY_CREATE_SUB_KEY,
                          NULL,
                          &appIDGUIDKey,
                          NULL);
  if (result != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(result);
  sbAutoRegKey autoAppIDGUIDKey(appIDGUIDKey);

  // Set the app description.
  TCHAR appDescription[] = _T("Songbird Windows Formatter");
  result = RegSetValueEx(appIDGUIDKey,
                         NULL,
                         0,
                         REG_SZ,
                         reinterpret_cast<const BYTE*>(appDescription),
                         sizeof(appDescription));
  if (result != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(result);

  // Set the DLL surrogate registry value.
  TCHAR dllSurrogate[] = _T("");
  result = RegSetValueEx(appIDGUIDKey,
                         _T("DllSurrogate"),
                         0,
                         REG_SZ,
                         reinterpret_cast<const BYTE*>(dllSurrogate),
                         sizeof(dllSurrogate));
  if (result != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(result);


  //
  // HKCR\AppID\sbWindowsFormatter.dll
  //

  // Create the Songbird Windows formatter application ID executable registry
  // key and set it up to auto-close.
  HKEY appIDExecutableKey;
  result = RegCreateKeyEx(HKEY_CLASSES_ROOT,
                          _T("AppID\\sbWindowsFormatter.dll"),
                          0,
                          NULL,
                          REG_OPTION_NON_VOLATILE,
                          KEY_SET_VALUE | KEY_CREATE_SUB_KEY,
                          NULL,
                          &appIDExecutableKey,
                          NULL);
  if (result != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(result);
  sbAutoRegKey autoAppIDExecutableKey(appIDExecutableKey);

  // Set the application ID GUID.
  result = RegSetValueEx(appIDExecutableKey,
                         _T("AppID"),
                         0,
                         REG_SZ,
                         reinterpret_cast<const BYTE*>(appIDGUID),
                         sizeof(appIDGUID));
  if (result != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(result);


  // Register the Songbird Windows formatter type library and set it up for
  // auto-disposal.
  ITypeLib* typeLib;
  result = LoadTypeLib(dllModulePath, &typeLib);
  if (result != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(result);
  sbAutoIUnknown autoTypeLib(typeLib);
  result = RegisterTypeLib(typeLib, dllModulePath, NULL);
  if (result != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(result);

  return S_OK;
}


/**
 * Unregister the COM server.
 */

STDAPI
DllUnregisterServer()
{
  // Unregister the Songbird Windows formatter type library.
  UnRegisterTypeLib(LIBID_sbWindowsFormatterLibrary,
                    1,
                    0,
                    0,
                    SYS_WIN32);

  // Delete HKCR\CLSID\SB_WINDOWS_FORMATTER_CLSID
  RegDeleteKey(HKEY_CLASSES_ROOT,
               _T("CLSID\\")
                 _T(CLSIDStr_sbWindowsFormatter)
                 _T("\\TypeLib"));
  RegDeleteKey(HKEY_CLASSES_ROOT,
               _T("CLSID\\")
                 _T(CLSIDStr_sbWindowsFormatter)
                 _T("\\Elevation"));
  RegDeleteKey(HKEY_CLASSES_ROOT,
               _T("CLSID\\")
                 _T(CLSIDStr_sbWindowsFormatter)
                 _T("\\InProcServer32"));
  RegDeleteKey(HKEY_CLASSES_ROOT,
               _T("CLSID\\") _T(CLSIDStr_sbWindowsFormatter));

  // Delete HKCR\AppID\SB_WINDOWS_FORMATTER_CLSID
  RegDeleteKey(HKEY_CLASSES_ROOT,
               _T("AppID\\") _T(CLSIDStr_sbWindowsFormatter));

  // Delete HKCR\AppID\sbWindowsFormatter.dll
  RegDeleteKey(HKEY_CLASSES_ROOT, _T("AppID\\sbWindowsFormatter.dll"));

  return S_OK;
}


//------------------------------------------------------------------------------
//
// Songbird Windows formatter component COM server services.
//
//------------------------------------------------------------------------------

/**
 * Add a lock on the COM server.
 */

void
COMServerLock()
{
  gCOMServerLockCount++;
}


/**
 * Remove a lock on the COM server.
 */

void
COMServerUnlock()
{
  gCOMServerLockCount--;
}


