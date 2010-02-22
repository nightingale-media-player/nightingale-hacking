
#
# BEGIN SONGBIRD GPL
# 
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2008 POTI, Inc.
# http://songbirdnest.com
# 
# This file may be licensed under the terms of of the
# GNU General Public License Version 2 (the "GPL").
# 
# Software distributed under the License is distributed 
# on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
# express or implied. See the GPL for the specific language 
# governing rights and limitations.
#
# You should have received a copy of the GPL along with this 
# program. If not, go to http://www.gnu.org/licenses/gpl.html
# or write to the Free Software Foundation, Inc., 
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
# 
# END SONGBIRD GPL
#

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Installer Options
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Icon ${PreferredInstallerIcon}

ShowInstDetails hide

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Install Sections
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Section "-Application" Section1
   SectionIn 1 RO

   ${If} ${AtLeastWinVista}
      StrCpy $LinkIconFile ${VistaIcon}
   ${Else}
      StrCpy $LinkIconFile ${PreferredIcon}
   ${EndIf}

   ; This macro is hiding in sb-filelist.nsi.in
   !insertmacro InstallFiles

   ${If} $UnpackMode == ${FALSE}
      Call InstallAppRegistryKeys
   
      ${If} $DistributionMode == ${FALSE}
         Call InstallUninstallRegistryKeys
         Call InstallBrandingRegistryKeys

         ; Refresh desktop icons
         System::Call "shell32::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)"
      ${Else}
         ; Execute disthelper.exe in install mode; disthelper.exe needs a 
         ; distribution.ini, but gets it from the environment; we expect the 
         ; partner installer *calling us* to set this.
         ExecWait '"$INSTDIR\${DistHelperEXE}" install'
      ${EndIf}

      Call InstallCdrip
   ${EndIf}

   IfRebootFlag 0 noReboot
      MessageBox MB_YESNO|MB_ICONQUESTION "${InstallRebootNow}" /SD IDNO IDNO noReboot
      Reboot
   noReboot:
SectionEnd

Function InstallAppRegistryKeys
   ; Register DLLs
   ; XXXrstrong - AccessibleMarshal.dll can be used by multiple applications but
   ; is only registered for the last application installed. When the last
   ; application installed is uninstalled AccessibleMarshal.dll will no longer 
   ; be registered. bug 338878
   ; XXXeps - sbWindowsFormatter.dll also behaves as AccessibleMarshall.dll does
   ; with respect to multiple applications
   ; XXXaus - It's unclear to me if we need to do the same thing, need to
   ; investigate.
   ClearErrors
   RegDLL "$INSTDIR\${XULRunnerDir}\AccessibleMarshal.dll"
   RegDLL "$INSTDIR\lib\sbWindowsFormatter.dll"

   ; Check if QuickTime is installed and copy the nsIQTScriptablePlugin.xpt from
   ; its plugins directory into the app's components directory.
   ClearErrors
   ReadRegStr $R0 HKLM "Software\Apple Computer, Inc.\QuickTime" "InstallDir"
   ${Unless} ${Errors}
      Push $R0
      ${GetPathFromRegStr}
      Pop $R0
      ${Unless} ${Errors}
         GetFullPathName $R0 "$R0\Plugins\nsIQTScriptablePlugin.xpt"
         ${Unless} ${Errors}
            CopyFiles /SILENT "$R0" "$INSTDIR\components"
         ${EndUnless}
      ${EndUnless}
   ${EndUnless}
   ClearErrors

   ; Write the installation path into the registry
   WriteRegStr HKLM $RootAppRegistryKey "InstallDir" "$INSTDIR"
   WriteRegStr HKLM $RootAppRegistryKey "BuildNumber" "${AppBuildNumber}"
   WriteRegStr HKLM $RootAppRegistryKey "BuildVersion" "${AppVersion}"

   ; Default this to false; we'll set it to true in the installation
   ; section if need-be
   WriteRegStr HKLM $RootAppRegistryKey ${CdripRegKey} ${FALSE}
  
   ${DirState} $INSTDIR $R0
   ${If} $R0 == -1
      WriteRegDWORD HKLM $RootAppRegistryKey "CreatedInstallationDir" 1
   ${EndIf}

   ; These need special handling on uninstall since they may be overwritten by
   ; an install into a different location.
   StrCpy $0 "Software\Microsoft\Windows\CurrentVersion\App Paths\${FileMainEXE}"
   WriteRegStr HKLM "$0" "Path" "$INSTDIR"
   WriteRegStr HKLM "$0" "" "$INSTDIR\${FileMainEXE}"

   ; Add XULRunner and Songbird to the Windows Media Player Shim Inclusion List.
   WriteRegStr HKLM "Software\Microsoft\MediaPlayer\ShimInclusionList\${XULRunnerEXE}" "" ""
   WriteRegStr HKLM "Software\Microsoft\MediaPlayer\ShimInclusionList\${FileMainEXE}" "" ""
FunctionEnd

Function InstallUninstallRegistryKeys
   StrCpy $R0 "${BrandFullNameInternal}-$InstallerType-${AppBuildNumber}"

   ${If} $DistributionMode != ${TRUE}
      ; Write the uninstall keys for Windows
      WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\$R0" "DisplayName" "${BrandFullName} ${AppVersion} (Build ${AppBuildNumber})"
      WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\$R0" "InstallLocation" "$INSTDIR"
      WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\$R0" "UninstallString" '"$INSTDIR\${FileUninstallEXE}"'
      WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\$R0" "NoModify" 1
      WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\$R0" "NoRepair" 1
   ${EndIf}
FunctionEnd

Function InstallBrandingRegistryKeys 
   !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
   CreateDirectory "$SMPROGRAMS\$StartMenuDir"
   CreateShortCut "$SMPROGRAMS\$StartMenuDir\${BrandFullNameInternal}.lnk" "$INSTDIR\${FileMainEXE}" "" "$INSTDIR\$LinkIconFile" 0
   CreateShortCut "$SMPROGRAMS\$StartMenuDir\${BrandFullNameInternal} (Profile Manager).lnk" "$INSTDIR\${FileMainEXE}" "-p" "$INSTDIR\$LinkIconFile" 0 SW_SHOWNORMAL "" "${BrandFullName} w/ Profile Manager"
   CreateShortCut "$SMPROGRAMS\$StartMenuDir\${BrandFullNameInternal} (Safe-Mode).lnk" "$INSTDIR\${FileMainEXE}" "-safe-mode" "$INSTDIR\$LinkIconFile" 0 SW_SHOWNORMAL "" "${BrandFullName} Safe-Mode"
   CreateShortCut "$SMPROGRAMS\$StartMenuDir\Uninstall ${BrandFullNameInternal}.lnk" "$INSTDIR\${FileUninstallEXE}" "" "$INSTDIR\${PreferredUninstallerIcon}" 0
   !insertmacro MUI_STARTMENU_WRITE_END

   ; Because the RootAppRegistryKey changes based on the mode the installer is
   ; run in, we need to copy the value of the key that the MUI Start Menu page
   ; sets (which is set at installer build time, not installer runtime), copy
   ; it over to the correct area for how our installer is being run, and then
   ; delete it.
   ReadRegStr $R0 HKLM ${MuiStartmenupageRegKey} ${MuiStartmenupageRegName}
   WriteRegStr HKLM $RootAppRegistryKey "${MuiStartmenupageRegName}" $R0
   DeleteRegKey HKLM ${MuiStartmenupageRegKey}

   ;
   ; AutoPlay v2 registration.
   ;
   ; See http://msdn.microsoft.com/en-us/magazine/cc301341.aspx
   ;     http://social.msdn.microsoft.com/Forums/en-US/netfxbcl/thread/8341c15b-04ef-438e-ab1e-276186fd2177/
   ;
   ;   AutoPlay support for MSC devices is provided by registering a handler to
   ; handle PlayMusicFilesOnArrival events for volume-based devices as described
   ; in "http://msdn.microsoft.com/en-us/magazine/cc301341.aspx".
   ;

   ; Register a manage volume device ProgID to launch Songbird.  By default,
   ; Songbird will be launched with the volume mount point as the current
   ; working directory.  This prevents the volume from being ejected.  To avoid
   ; this, Songbird is directed to start in the Songbird application directory.
   StrCpy $0 "Software\Classes\${AutoPlayManageVolumeDeviceProgID}\shell\manage\command"
   WriteRegStr HKLM $0 "" "$INSTDIR\${FileMainEXE} -autoplay-manage-volume-device -start-in-app-directory"

   ; Register a volume device arrival handler to invoke the manage volume
   ; device ProgID.
   StrCpy $0 "Software\Microsoft\Windows\CurrentVersion\Explorer\AutoPlayHandlers\Handlers\${AutoPlayVolumeDeviceArrivalHandlerName}"
   WriteRegStr HKLM $0 "" ""
   WriteRegStr HKLM $0 "Action" "${AutoPlayManageDeviceAction}"
   WriteRegStr HKLM $0 "DefaultIcon" "$INSTDIR\${FileMainEXE}"
   WriteRegStr HKLM $0 "InvokeProgID" "${AutoPlayVolumeDeviceArrivalHandlerProgID}"
   WriteRegStr HKLM $0 "InvokeVerb" "manage"
   WriteRegStr HKLM $0 "Provider" "${BrandShortName}"

   ; Register to handle PlayMusicFilesOnArrival events using the volume
   ; device arrival handler.
   StrCpy $0 "Software\Microsoft\Windows\CurrentVersion\Explorer\AutoPlayHandlers\EventHandlers\PlayMusicFilesOnArrival"
   WriteRegStr HKLM $0 "${AutoPlayVolumeDeviceArrivalHandlerName}" ""

   ; Register an MTP device arrival handler to launch Songbird to manage the MTP
   ; device.  Make use of the "Shell.HWEventHandlerShellExecute" COM component.
   ; If any command line arguments are present in InitCmdLine, the executable
   ; string must be enclosed in quotes if it contains spaces.
   StrCpy $0 "Software\Microsoft\Windows\CurrentVersion\Explorer\AutoPlayHandlers\Handlers\${AutoPlayMTPDeviceArrivalHandlerName}"
   WriteRegStr HKLM $0 "Action" "${AutoPlayManageDeviceAction}"
   WriteRegStr HKLM $0 "DefaultIcon" "$INSTDIR\${FileMainEXE}"
   WriteRegStr HKLM $0 "InitCmdLine" '"$INSTDIR\${FileMainEXE}" -autoplay-manage-mtp-device'
   WriteRegStr HKLM $0 "ProgID" "Shell.HWEventHandlerShellExecute"
   WriteRegStr HKLM $0 "Provider" "${BrandShortName}"

   ; Register to handle MTPMediaPlayerArrival events using the MTP device
   ; arrival handler.
   StrCpy $0 "Software\Microsoft\Windows\CurrentVersion\Explorer\AutoPlayHandlers\EventHandlers\MTPMediaPlayerArrival"
   WriteRegStr HKLM $0 "${AutoPlayMTPDeviceArrivalHandlerName}" ""

   ; Register to handle WPD audio and video events using the MTP device arrival
   ; handler.
   StrCpy $0 "Software\Microsoft\Windows\CurrentVersion\Explorer\AutoPlayHandlers\EventHandlers\WPD\Sink\{4AD2C85E-5E2D-45E5-8864-4F229E3C6CF0}"
   WriteRegStr HKLM $0 "${AutoPlayMTPDeviceArrivalHandlerName}" ""
   StrCpy $0 "Software\Microsoft\Windows\CurrentVersion\Explorer\AutoPlayHandlers\EventHandlers\WPD\Sink\{9261B03C-3D78-4519-85E3-02C5E1F50BB9}"
   WriteRegStr HKLM $0 "${AutoPlayMTPDeviceArrivalHandlerName}" ""

   ; Register CD Rip command
   WriteRegStr HKLM "Software\Classes\${AutoPlayProgID}\shell\Rip\command" "" "$INSTDIR\${FileMainEXE} -autoplay-cd-rip"

   ; Register an Audio CD Rip handler
   ; device ProgID.
   StrCpy $0 "Software\Microsoft\Windows\CurrentVersion\Explorer\AutoPlayHandlers\Handlers\${AutoPlayCDRipHandlerName}"
   WriteRegStr HKLM $0 "Action" "${AutoPlayCDRipAction}"
   WriteRegStr HKLM $0 "DefaultIcon" "$INSTDIR\${FileMainEXE}"
   WriteRegStr HKLM $0 "InvokeProgID" "${AutoPlayProgID}"
   WriteRegStr HKLM $0 "InvokeVerb" "Rip"
   WriteRegStr HKLM $0 "Provider" "${BrandShortName}"

   ; Register for CD arrival events for rip
   ; device arrival handler.
   WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Explorer\AutoPlayHandlers\EventHandlers\PlayCDAudioOnArrival" "${AutoPlayCDRipHandlerName}" ""
   
FunctionEnd 

Section "Desktop Icon"
   ${If} $DistributionMode == ${TRUE}
      Goto End
   ${EndIf}
   ${If} $UnpackMode == ${TRUE}
      Goto End
   ${EndIf}

   ; Put the desktop icon in All Users\Desktop
   SetShellVarContext all
   CreateShortCut "$DESKTOP\${BrandFullNameInternal}.lnk" "$INSTDIR\${FileMainEXE}" "" "$INSTDIR\$LinkIconFile" 0

   ; Remember that we installed a desktop shortcut.
   WriteRegStr HKLM $RootAppRegistryKey ${DesktopShortcutRegName} "$DESKTOP\${BrandFullNameInternal}.lnk"
 
End: 
SectionEnd

Section "QuickLaunch Icon"
   ${If} $DistributionMode == ${TRUE}
      Goto End
   ${EndIf}
   ${If} $UnpackMode == ${TRUE}
      Goto End
   ${EndIf}
  
   ; Put the quicklaunch icon in the current users quicklaunch.
   SetShellVarContext current
   CreateShortCut "$QUICKLAUNCH\${BrandFullNameInternal}.lnk" "$INSTDIR\${FileMainEXE}" "" "$INSTDIR\$LinkIconFile" 0

   ; Remember that we installed a quicklaunch shortcut.
   WriteRegStr HKLM $RootAppRegistryKey ${QuicklaunchRegName} "$QUICKLAUNCH\${BrandFullNameInternal}.lnk"
End:
SectionEnd

Function InstallCdrip
   ; We theoretically should check the return value here, and maybe not set
   ; this registry key?
   WriteRegStr HKLM $RootAppRegistryKey ${CdripRegKey} ${TRUE}
   ExecWait '"$INSTDIR\${CdripHelperEXE}" install'
FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Installer Helper Functions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Function LaunchAppUserPrivilege 
   Exec '"$INSTDIR\${FileMainEXE}"'    
FunctionEnd

Function LaunchApp
   Call CloseApp
   GetFunctionAddress $0 LaunchAppUserPrivilege 
   UAC::ExecCodeSegment $0 
FunctionEnd 

; Only valid for release and distribution installers
Function GetOldVersionLocation
   ${If} $InstallerType == "nightly"
      MessageBox MB_OK "Assertion Failed: InstallerType = nightly in GetOldVersionLocation; aborting."
      Abort
   ${EndIf}

   ReadRegStr $R0 HKLM $RootAppRegistryKey "InstallDir"
   ClearErrors
   StrCpy $0 $R0
FunctionEnd

; This checks for pre-1.2 versions, which used entirely different reg keys
;
; We return the first installation we find and search in reverse order of
; releases, i.e. newer releases first.
;
; HUGE HACK NOTE: _UNLIKE_ GetOldVersionLocation, this function does NOT 
; return a location, but rather returns the registry key where that location
; can be found; this is because we need to clean that key up, because the
; installer doesn't do it correctly.
;
; See bug 16567 for more details.

Function GetAncientVersionLocation
   ${If} $InstallerType == "nightly"
      MessageBox MB_OK "Assertion Failed: InstallerType = nightly in GetOldVersionLocation; aborting."
      Abort
   ${EndIf}

   StrCpy $0 ""

   StrCpy $R1 "Software\Songbird\${AncientRegKeyBranding112}"
   ReadRegStr $R0 HKLM $R1 "Start Menu Folder"
   ${If} $R0 != ""
      StrCpy $R1 "Software\Songbird\${AncientRegKeyApp112}"
      ReadRegStr $R0 HKLM $R1 "InstallDir"
      ${If} $R0 != ""
         StrCpy $0 $R1
         Goto out
      ${EndIf}
   ${EndIf}

   StrCpy $R1 "Software\Songbird\${AncientRegKeyBranding111}"
   ReadRegStr $R0 HKLM $R1 "Start Menu Folder"
   ${If} $R0 != ""
      StrCpy $R1 "Software\Songbird\${AncientRegKeyApp111}"
      ReadRegStr $R0 HKLM $R1 "InstallDir"
      ${If} $R0 != ""
         StrCpy $0 $R1
         Goto out
      ${EndIf}
   ${EndIf}

   StrCpy $R1 "Software\Songbird\${AncientRegKeyBranding100}"
   ReadRegStr $R0 HKLM $R1 "Start Menu Folder"
   ${If} $R0 != ""
      StrCpy $R1 "Software\Songbird\${AncientRegKeyApp100}"
      ReadRegStr $R0 HKLM $R1 "InstallDir"
      ${If} $R0 != ""
         StrCpy $0 $R1
         Goto out
      ${EndIf}
   ${EndIf}
out:

FunctionEnd

Function ValidateInstallationDirectory
RevalidateInstallationDirectory:
   ${DirState} "$INSTDIR" $R0

   ${If} $R0 == 1
      IfSilent +1 +2
         Quit
      ${If} $DistributionMode == ${TRUE}
         Quit
      ${EndIf}
      ${If} $UnpackMode == ${TRUE}
         Quit
      ${EndIf}

      ; This only works because we haven't changed the name of our uninstaller;
      ; go us.
      IfFileExists "$INSTDIR\${FileUninstallEXE}" 0 ConfirmDirtyDirectory
         MessageBox MB_YESNO|MB_ICONQUESTION "${UninstallMessageSameFolder}" /SD IDNO IDYES CallCallUninstaller
         Abort

         CallCallUninstaller:
            Push $INSTDIR
            Call CallUninstaller
            Goto RevalidateInstallationDirectory

      ConfirmDirtyDirectory:
         MessageBox MB_YESNO|MB_ICONQUESTION "${DirectoryNotEmptyMessage}" IDYES OverrideDirCheck IDNO NotValid

      NotValid:
         Abort
   ${EndIf}

   OverrideDirCheck:
FunctionEnd

Function CallUninstaller
   Exch $0
   Push $1
   System::Call 'Kernel32::SetEnvironmentVariableA(t, t) i("SB_INSTALLER_NOMUTEX", "1").r1'
   ExecWait '"$0\${FileUninstallEXE}" /S _?=$0' $1
   DetailPrint '"$0\${FileUninstallEXE}" /S _?=$0 returned $1'

   ; We use this key existing as a reasonable hueristic about whether the
   ; installer really did anything (and didn't bail out because it needed
   ; input while in silent mode).
   EnumRegKey $1 HKLM "$RootAppRegistryKey" 0

   ${If} $1 == ""
      Delete $0\${FileUninstallEXE}
   ${EndIf}

   RMDir $0
   Pop $1
   Pop $0
FunctionEnd

Function PreviousInstallationCheck
recheck:
   ; Whether or not we need to delete the extra key
   StrCpy $R2 ${FALSE}

   Call GetOldVersionLocation

   ${If} $0 == ""
      Call GetAncientVersionLocation

      ${If} $0 != ""
         StrCpy $R1 $0 ; this is now the reg key we need to clear
         ReadRegStr $0 HKLM $R1 "InstallDir"
         StrCpy $R2 ${TRUE}
      ${EndIf}
   ${EndIf}

   ${If} $0 != ""
      MessageBox MB_YESNO|MB_ICONQUESTION "${UninstallMessageOldVersion}" /SD IDNO IDYES CallCallUninstaller
      Abort

      CallCallUninstaller:
         Push $0
         Call CallUninstaller

      ${If} $R2 == ${TRUE}
         DeleteRegKey HKLM $R1
         ; This is hardcoded as "Songbird" and not "${BrandNameShort}" because
         ; while they _should_ be the same thing, we don't want to mess with
         ; any other keys, in case they weren't.
         DeleteRegKey /ifempty HKLM "Software\Songbird"
      ${EndIf}

      Goto recheck
   ${EndIf}
FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Installer Initialization Functions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Function .onInit
   ; preedTODO: Check drive space

   ; May seem weird, but it's used for update testing; so ignore the man
   ; behind the curtain...
   ReadEnvStr $0 SB_FORCE_NO_UAC
   ${If} $0 == "" 
      ${UAC.I.Elevate.AdminOnly}
   ${EndIf}

   Call CommonInstallerInit

   ${If} $UnpackMode == ${TRUE}
      ; Force a silent installation if we're just /UNPACK'ing
      SetSilent silent
   ${Else}
      ; Version checks! This is not in CommonInstallerInit, because we always 
      ; want the uninstaller to be able to run.
      ${If} $CheckOSVersion == ${TRUE}
         ${If} ${AtLeastWinXP}
         ${AndIf} ${AtMostWin7}
            ${If} ${IsWinXP}
               ${Unless} ${AtLeastServicePack} 2
                  Goto confirmUnsupportedWinVersion
               ${EndUnless}
            ${EndIf}
         ${Else}
            Goto confirmUnsupportedWinVersion
         ${EndIf}
         ${If} ${IsServerOS}
            Goto confirmUnsupportedWinVersion
         ${EndIf}
      ${EndIf}
   ${EndIf}

   ; This really isn't an override, but rather we've passed all the checks
   ; successfully; continue installing...
   Goto overrideVersionCheck

confirmUnsupportedWinVersion:
   MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION "${InstallUnsupportedWinVersion}" /SD IDCANCEL IDOK overrideVersionCheck
   Quit

overrideVersionCheck:

   Call CloseApp

   ${If} $UnpackMode != ${TRUE} 
      ${If} $InstallerType != "nightly"
         Call PreviousInstallationCheck
      ${EndIf}
   ${EndIf}

   ; If we install, uninstall, and install again without rebooting, the cdrip
   ; service will fail to install because it's marked for deletion; check that 
   ; here and ask the user to reboot before we install.
   ;
   ; We have to do this down here because PreviousInstallationCheck could call
   ; the uninstaller, which puts us in exactly this situation.
   ${If} $UnpackMode != ${TRUE}
      ReadRegDWORD $0 HKLM "${CdripServiceRegKey}" "DeleteFlag"
      ${If} $0 == "1"
         MessageBox MB_YESNO|MB_ICONQUESTION "${PreInstallRebootNow}" /SD IDNO IDNO noReboot
         Reboot
      noReboot:
         Quit
      ${EndIf} 
   ${EndIf} 
FunctionEnd
