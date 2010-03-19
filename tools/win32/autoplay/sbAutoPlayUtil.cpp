/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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
// Songbird AutoPlay utilities.
//
//   The Songbird AutoPlay utilities provide support for installing, removing,
// and upgrading Songbird AutoPlay support.
//
//   See http://msdn.microsoft.com/en-us/magazine/cc301341.aspx
//       http://social.msdn.microsoft.com/Forums/en-US/netfxbcl/thread/
//         8341c15b-04ef-438e-ab1e-276186fd2177/
//
//   AutoPlay support for MSC devices is provided by registering a handler to
// handle PlayMusicFilesOnArrival events for volume-based devices as described
// in "http://msdn.microsoft.com/en-us/magazine/cc301341.aspx".
//
//------------------------------------------------------------------------------

/**
 * \file  sbAutoPlayUtil.cpp
 * \brief Songbird AutoPlay Utilities Source.
 */

//------------------------------------------------------------------------------
//
// Songbird AutoPlay utilities imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbBrandString.h>
#include <sbWindowsUtils.h>

// Windows imports.
#include <shellapi.h>


//------------------------------------------------------------------------------
//
// Songbird AutoPlay utilities globals.
//
//------------------------------------------------------------------------------

//
// gAutoPlayUtilModule          AutoPlay utilities module.
// gAutoPlayUtilDistDirPath     Path to the AutoPlay utilities distribution
//                              directory.
//

static HINSTANCE gAutoPlayUtilModule = NULL;
static tstring gAutoPlayUtilDistDirPath;


//------------------------------------------------------------------------------
//
// Songbird AutoPlay utilities services.
//
//------------------------------------------------------------------------------

//
// Internal services.
//

static HRESULT SBAutoPlayValidateBranding();

static HRESULT SBAutoPlayInstall();

static HRESULT SBAutoPlayInstallVolumeDevice();

static HRESULT SBAutoPlayInstallMTP();

static HRESULT SBAutoPlayInstallCD();

static void SBAutoPlayRemove();

static void SBAutoPlayRemoveVolumeDevice();

static void SBAutoPlayRemoveMTP();

static void SBAutoPlayRemoveCD();

static HRESULT SBAutoPlayUpgrade();


/**
 * Main entry point for the Songbird AutoPlay utilities.
 */

int WINAPI
_tWinMain(HINSTANCE aAppHandle,
          HINSTANCE aPrevAppHandle,
          LPCTSTR   aCommandLine,
          int       aCommandShow)
{
  HRESULT hr;
  int     result = 0;

  // Get the command line arguments and set them up for auto-disposal.
  int argc;
  LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  SB_WIN_ENSURE_TRUE(argv, HRESULT_FROM_WIN32(GetLastError()));
  sbAutoLocalFree autoArgv(argv);

  // Get the command.
  LPCTSTR command = NULL;
  if (argc >= 2)
    command = argv[1];

  // Get the AutoPlay utilities module from the application handle.
  gAutoPlayUtilModule = aAppHandle;

  // Get the Songbird distribution directory path from the AutoPlay utility
  // module.
  tstring moduleFilePath;
  hr = sbWinGetModuleFilePath(gAutoPlayUtilModule,
                              moduleFilePath,
                              gAutoPlayUtilDistDirPath);
  SB_WIN_ENSURE_SUCCESS(hr, hr);

  // Validate branding.
  hr = SBAutoPlayValidateBranding();
  SB_WIN_ENSURE_SUCCESS(hr, hr);

  // Dispatch processing of the command line.
  if (command) {
    if (!_tcsicmp(command, _T("-Install")) ||
        !_tcsicmp(command, _T("/Install"))) {
      hr = SBAutoPlayInstall();
      if (!SUCCEEDED(hr))
        result = -1;
    }
    else if (!_tcsicmp(command, _T("-Remove")) ||
             !_tcsicmp(command, _T("/Remove"))) {
      SBAutoPlayRemove();
    }
    else if (!_tcsicmp(command, _T("-Upgrade")) ||
             !_tcsicmp(command, _T("/Upgrade"))) {
      hr = SBAutoPlayUpgrade();
      if (!SUCCEEDED(hr))
        result = -1;
    }
  }

  return result;
}


/**
 * Validate branding settings.
 */

HRESULT
SBAutoPlayValidateBranding()
{
  static const char* requiredBrandStringList[] = {
    "BrandShortName",
    "FileMainEXE",
    "AutoPlayManageVolumeDeviceProgID",
    "AutoPlayVolumeDeviceArrivalHandlerName",
    "AutoPlayVolumeDeviceArrivalHandlerProgID",
    "AutoPlayMTPDeviceArrivalHandlerName",
    "AutoPlayManageDeviceAction",
    "AutoPlayProgID",
    "AutoPlayCDRipHandlerName",
    "AutoPlayCDRipAction"
  };
  for (int i = 0; i < _countof(requiredBrandStringList); ++i) {
    SB_WIN_ENSURE_TRUE(!sbBrandString(requiredBrandStringList[i], "").empty(),
                       NS_ERROR_FAILURE);
  }

  return S_OK;
}


/**
 * Install the Songbird AutoPlay services.
 */

HRESULT
SBAutoPlayInstall()
{
  HRESULT hr;

  // Install the AutoPlay volume device services.
  hr = SBAutoPlayInstallVolumeDevice();
  SB_WIN_ENSURE_SUCCESS(hr, hr);

  // Install the AutoPlay MTP services.
  hr = SBAutoPlayInstallMTP();
  SB_WIN_ENSURE_SUCCESS(hr, hr);

  // Install the AutoPlay CD services.
  hr = SBAutoPlayInstallCD();
  SB_WIN_ENSURE_SUCCESS(hr, hr);

  return S_OK;
}


/**
 * Install the Songbird AutoPlay volume device services.
 */

HRESULT
SBAutoPlayInstallVolumeDevice()
{
  tstring subKey;
  tstring value;
  HRESULT hr;

  // Get the Songbird executable path.
  tstring songbirdEXEPath = gAutoPlayUtilDistDirPath +
                            sbBrandString("FileMainEXE");

  // Register a manage volume device ProgID to launch Songbird.  By default,
  // Songbird will be launched with the volume mount point as the current
  // working directory.  This prevents the volume from being ejected.  To avoid
  // this, Songbird is directed to start in the Songbird application directory.
  subKey = tstring("Software\\Classes\\") +
           sbBrandString("AutoPlayManageVolumeDeviceProgID") +
           tstring("\\shell\\manage\\command");
  value = songbirdEXEPath +
          tstring(" -autoplay-manage-volume-device -start-in-app-directory");
  hr = sbWinWriteRegStr(HKEY_LOCAL_MACHINE, subKey, "", value);
  SB_WIN_ENSURE_SUCCESS(hr, hr);

  // Register a volume device arrival handler to invoke the manage volume device
  // ProgID.
  subKey = tstring("Software\\Microsoft\\Windows\\CurrentVersion\\"
                   "Explorer\\AutoPlayHandlers\\Handlers\\") +
           sbBrandString("AutoPlayVolumeDeviceArrivalHandlerName");
  hr = sbWinWriteRegStr(HKEY_LOCAL_MACHINE, subKey, "Action",
                        sbBrandString("AutoPlayManageDeviceAction"));
  SB_WIN_ENSURE_SUCCESS(hr, hr);
  hr = sbWinWriteRegStr(HKEY_LOCAL_MACHINE, subKey, "DefaultIcon",
                        songbirdEXEPath);
  SB_WIN_ENSURE_SUCCESS(hr, hr);
  hr = sbWinWriteRegStr
         (HKEY_LOCAL_MACHINE, subKey, "InvokeProgID",
          sbBrandString("AutoPlayVolumeDeviceArrivalHandlerProgID"));
  SB_WIN_ENSURE_SUCCESS(hr, hr);
  hr = sbWinWriteRegStr(HKEY_LOCAL_MACHINE, subKey, "InvokeVerb", "manage");
  SB_WIN_ENSURE_SUCCESS(hr, hr);
  hr = sbWinWriteRegStr(HKEY_LOCAL_MACHINE, subKey, "Provider",
                        sbBrandString("BrandShortName"));
  SB_WIN_ENSURE_SUCCESS(hr, hr);

  // Register to handle PlayMusicFilesOnArrival events using the volume device
  // arrival handler.
  subKey = tstring
             ("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"
              "\\AutoPlayHandlers\\EventHandlers\\PlayMusicFilesOnArrival");
  hr = sbWinWriteRegStr(HKEY_LOCAL_MACHINE, subKey,
                        sbBrandString("AutoPlayVolumeDeviceArrivalHandlerName"),
                        "");
  SB_WIN_ENSURE_SUCCESS(hr, hr);

  return S_OK;
}


/**
 * Install the Songbird AutoPlay MTP services.
 */

HRESULT
SBAutoPlayInstallMTP()
{
  tstring subKey;
  tstring value;
  HRESULT hr;

  // Get the Songbird executable path.
  tstring songbirdEXEPath = gAutoPlayUtilDistDirPath +
                            sbBrandString("FileMainEXE");

  // Register an MTP device arrival handler to launch Songbird to manage the MTP
  // device.  Make use of the "Shell.HWEventHandlerShellExecute" COM component.
  // If any command line arguments are present in InitCmdLine, the executable
  // string must be enclosed in quotes if it contains spaces.
  subKey = tstring("Software\\Microsoft\\Windows\\CurrentVersion\\"
                   "Explorer\\AutoPlayHandlers\\Handlers\\") +
           sbBrandString("AutoPlayMTPDeviceArrivalHandlerName");
  hr = sbWinWriteRegStr(HKEY_LOCAL_MACHINE, subKey, "Action",
                        sbBrandString("AutoPlayManageDeviceAction"));
  SB_WIN_ENSURE_SUCCESS(hr, hr);
  hr = sbWinWriteRegStr(HKEY_LOCAL_MACHINE, subKey, "DefaultIcon",
                        songbirdEXEPath);
  SB_WIN_ENSURE_SUCCESS(hr, hr);
  value = tstring("\"") + songbirdEXEPath + tstring("\"") +
          tstring(" -autoplay-manage-mtp-device");
  hr = sbWinWriteRegStr(HKEY_LOCAL_MACHINE, subKey, "InitCmdLine", value);
  SB_WIN_ENSURE_SUCCESS(hr, hr);
  hr = sbWinWriteRegStr(HKEY_LOCAL_MACHINE, subKey, "ProgID",
                        "Shell.HWEventHandlerShellExecute");
  SB_WIN_ENSURE_SUCCESS(hr, hr);
  hr = sbWinWriteRegStr(HKEY_LOCAL_MACHINE, subKey, "Provider",
                        sbBrandString("BrandShortName"));
  SB_WIN_ENSURE_SUCCESS(hr, hr);

  // Register to handle MTPMediaPlayerArrival events using the MTP device
  // arrival handler.
  subKey = tstring("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"
                   "\\AutoPlayHandlers\\EventHandlers\\MTPMediaPlayerArrival");
  hr = sbWinWriteRegStr(HKEY_LOCAL_MACHINE, subKey,
                        sbBrandString("AutoPlayMTPDeviceArrivalHandlerName"),
                        "");
  SB_WIN_ENSURE_SUCCESS(hr, hr);

  // Register to handle WPD audio and video events using the MTP device arrival
  // handler.
  subKey = tstring
             ("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"
              "AutoPlayHandlers\\EventHandlers\\WPD\\Sink\\"
              "{4AD2C85E-5E2D-45E5-8864-4F229E3C6CF0}");
  hr = sbWinWriteRegStr(HKEY_LOCAL_MACHINE, subKey,
                        sbBrandString("AutoPlayMTPDeviceArrivalHandlerName"),
                        "");
  SB_WIN_ENSURE_SUCCESS(hr, hr);
  subKey = tstring
             ("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"
              "AutoPlayHandlers\\EventHandlers\\WPD\\Sink\\"
              "{9261B03C-3D78-4519-85E3-02C5E1F50BB9}");
  hr = sbWinWriteRegStr(HKEY_LOCAL_MACHINE, subKey,
                        sbBrandString("AutoPlayMTPDeviceArrivalHandlerName"),
                        "");
  SB_WIN_ENSURE_SUCCESS(hr, hr);

  return S_OK;
}


/**
 * Install the Songbird AutoPlay CD services.
 */

HRESULT
SBAutoPlayInstallCD()
{
  tstring subKey;
  tstring value;
  HRESULT hr;

  // Get the Songbird executable path.
  tstring songbirdEXEPath = gAutoPlayUtilDistDirPath +
                            sbBrandString("FileMainEXE");

  // Register CD rip command.
  subKey = tstring("Software\\Classes\\") +
           sbBrandString("AutoPlayProgID") +
           tstring("\\shell\\Rip\\command");
  value = songbirdEXEPath + tstring(" -autoplay-cd-rip");
  hr = sbWinWriteRegStr(HKEY_LOCAL_MACHINE, subKey, "", value);
  SB_WIN_ENSURE_SUCCESS(hr, hr);

  // Register an audio CD rip handler device ProgID.
  subKey = tstring("Software\\Microsoft\\Windows\\CurrentVersion\\"
                   "Explorer\\AutoPlayHandlers\\Handlers\\") +
           sbBrandString("AutoPlayCDRipHandlerName");
  hr = sbWinWriteRegStr(HKEY_LOCAL_MACHINE, subKey, "Action",
                        sbBrandString("AutoPlayCDRipAction"));
  SB_WIN_ENSURE_SUCCESS(hr, hr);
  hr = sbWinWriteRegStr(HKEY_LOCAL_MACHINE, subKey, "DefaultIcon",
                        songbirdEXEPath);
  SB_WIN_ENSURE_SUCCESS(hr, hr);
  hr = sbWinWriteRegStr(HKEY_LOCAL_MACHINE, subKey, "InvokeProgID",
                        sbBrandString("AutoPlayProgID"));
  SB_WIN_ENSURE_SUCCESS(hr, hr);
  hr = sbWinWriteRegStr(HKEY_LOCAL_MACHINE, subKey, "InvokeVerb", "Rip");
  SB_WIN_ENSURE_SUCCESS(hr, hr);
  hr = sbWinWriteRegStr(HKEY_LOCAL_MACHINE, subKey, "Provider",
                        sbBrandString("BrandShortName"));
  SB_WIN_ENSURE_SUCCESS(hr, hr);

  // Register for CD arrival events for rip device arrival handler.
  subKey = tstring
             ("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"
              "\\AutoPlayHandlers\\EventHandlers\\PlayCDAudioOnArrival");
  hr = sbWinWriteRegStr(HKEY_LOCAL_MACHINE, subKey,
                        sbBrandString("AutoPlayCDRipHandlerName"),
                        "");
  SB_WIN_ENSURE_SUCCESS(hr, hr);

  return S_OK;
}


/**
 * Remove the Songbird AutoPlay services.
 */

void
SBAutoPlayRemove()
{
  // Remove the AutoPlay volume device services.
  SBAutoPlayRemoveVolumeDevice();

  // Remove the AutoPlay MTP services.
  SBAutoPlayRemoveMTP();

  // Remove the AutoPlay CD services.
  SBAutoPlayRemoveCD();
}


/**
 * Remove the Songbird AutoPlay volume device services.
 */

void
SBAutoPlayRemoveVolumeDevice()
{
  tstring subKey;

  // Remove the volume device arrival handler from the PlayMusicFilesOnArrival
  // event.
  subKey = tstring
             ("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"
              "\\AutoPlayHandlers\\EventHandlers\\PlayMusicFilesOnArrival");
  sbWinDeleteRegValue(HKEY_LOCAL_MACHINE, subKey,
                      sbBrandString("AutoPlayVolumeDeviceArrivalHandlerName"));

  // Remove the volume device arrival handler.
  subKey = tstring("Software\\Microsoft\\Windows\\CurrentVersion\\"
                   "Explorer\\AutoPlayHandlers\\Handlers\\") +
           sbBrandString("AutoPlayVolumeDeviceArrivalHandlerName");
  sbWinDeleteRegKey(HKEY_LOCAL_MACHINE, subKey);

  // Remove the manage volume device ProgID.
  subKey = tstring("Software\\Classes\\") +
           sbBrandString("AutoPlayManageVolumeDeviceProgID") +
           tstring("\\shell\\manage\\command");
  sbWinDeleteRegKey(HKEY_LOCAL_MACHINE, subKey);
  subKey = tstring("Software\\Classes\\") +
           sbBrandString("AutoPlayManageVolumeDeviceProgID") +
           tstring("\\shell\\manage");
  sbWinDeleteRegKey(HKEY_LOCAL_MACHINE, subKey);
  subKey = tstring("Software\\Classes\\") +
           sbBrandString("AutoPlayManageVolumeDeviceProgID") +
           tstring("\\shell");
  sbWinDeleteRegKey(HKEY_LOCAL_MACHINE, subKey);
  subKey = tstring("Software\\Classes\\") +
           sbBrandString("AutoPlayManageVolumeDeviceProgID");
  sbWinDeleteRegKey(HKEY_LOCAL_MACHINE, subKey);
}


/**
 * Remove the Songbird AutoPlay MTP services.
 */

void
SBAutoPlayRemoveMTP()
{
  tstring subKey;

  // Remove the MTP device arrival handler from the MTPMediaPlayerArrival event.
  subKey = tstring
             ("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"
              "AutoPlayHandlers\\EventHandlers\\MTPMediaPlayerArrival");
  sbWinDeleteRegValue(HKEY_LOCAL_MACHINE, subKey,
                      sbBrandString("AutoPlayMTPDeviceArrivalHandlerName"));

  // Remove the MTP device arrival handler from the WPD auto and video events.
  subKey = tstring
             ("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"
              "AutoPlayHandlers\\EventHandlers\\WPD\\Sink\\"
              "{4AD2C85E-5E2D-45E5-8864-4F229E3C6CF0}");
  sbWinDeleteRegValue(HKEY_LOCAL_MACHINE, subKey,
                      sbBrandString("AutoPlayMTPDeviceArrivalHandlerName"));
  subKey = tstring
             ("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"
              "AutoPlayHandlers\\EventHandlers\\WPD\\Sink\\"
              "{9261B03C-3D78-4519-85E3-02C5E1F50BB9}");
  sbWinDeleteRegValue(HKEY_LOCAL_MACHINE, subKey,
                      sbBrandString("AutoPlayMTPDeviceArrivalHandlerName"));

  // Remove the MTP device arrival handler.
  subKey = tstring("Software\\Microsoft\\Windows\\CurrentVersion\\"
                   "Explorer\\AutoPlayHandlers\\Handlers\\") +
           sbBrandString("AutoPlayMTPDeviceArrivalHandlerName");
  sbWinDeleteRegKey(HKEY_LOCAL_MACHINE, subKey);
}


/**
 * Remove the Songbird AutoPlay CD services.
 */

void
SBAutoPlayRemoveCD()
{
  tstring subKey;

  // Remove the CD rip handler from the PlayMusicFilesOnArrival event.
  subKey = tstring
             ("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"
              "\\AutoPlayHandlers\\EventHandlers\\PlayCDAudioOnArrival");
  sbWinDeleteRegValue(HKEY_LOCAL_MACHINE, subKey,
                      sbBrandString("AutoPlayCDRipHandlerName"));

  // Remove the audio CD rip handler.
  subKey = tstring("Software\\Microsoft\\Windows\\CurrentVersion\\"
                   "Explorer\\AutoPlayHandlers\\Handlers\\") +
           sbBrandString("AutoPlayCDRipHandlerName");
  sbWinDeleteRegKey(HKEY_LOCAL_MACHINE, subKey);

  // Remove the CD rip ProgID.
  subKey = tstring("Software\\Classes\\") +
           sbBrandString("AutoPlayProgID") +
           tstring("\\shell\\Rip\\command");
  sbWinDeleteRegKey(HKEY_LOCAL_MACHINE, subKey);
  subKey = tstring("Software\\Classes\\") +
           sbBrandString("AutoPlayProgID") +
           tstring("\\shell\\Rip");
  sbWinDeleteRegKey(HKEY_LOCAL_MACHINE, subKey);
  subKey = tstring("Software\\Classes\\") +
           sbBrandString("AutoPlayProgID") +
           tstring("\\shell");
  sbWinDeleteRegKey(HKEY_LOCAL_MACHINE, subKey);
  subKey = tstring("Software\\Classes\\") + sbBrandString("AutoPlayProgID");
  sbWinDeleteRegKey(HKEY_LOCAL_MACHINE, subKey);
}


/**
 * Upgrade the Songbird AutoPlay services.
 */

HRESULT
SBAutoPlayUpgrade()
{
  HRESULT hr;

  // Install the Songbird AutoPlay services.
  hr = SBAutoPlayInstall();
  SB_WIN_ENSURE_SUCCESS(hr, hr);

  return S_OK;
}


