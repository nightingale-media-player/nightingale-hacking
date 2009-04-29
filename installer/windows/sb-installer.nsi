
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

   Call CloseApp

   ${If} ${AtLeastWinVista}
      StrCpy $LinkIconFile ${VistaIcon}
   ${Else}
      StrCpy $LinkIconFile ${PreferredIcon}
   ${EndIf}

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
         ExecWait '$INSTDIR\${DistHelperEXE} install'
      ${EndIf}
   ${EndIf}
SectionEnd

Function InstallAppRegistryKeys
   ; Register DLLs
   ; XXXrstrong - AccessibleMarshal.dll can be used by multiple applications but
   ; is only registered for the last application installed. When the last
   ; application installed is uninstalled AccessibleMarshal.dll will no longer 
   ; be registered. bug 338878
   ; XXXaus - It's unclear to me if we need to do the same thing, need to
   ; investigate.
   ClearErrors
   RegDLL "$INSTDIR\${XULRunnerDir}\AccessibleMarshal.dll"

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
   ExecWait '$0\${FileUninstallEXE} /S _?=$0'
   Delete '$0\${FileUninstallEXE}'
   RMDir $0
   Pop $0
FunctionEnd

Function PreviousInstallationCheck
   Call GetOldVersionLocation
   ${If} $0 != ""
      MessageBox MB_YESNO|MB_ICONQUESTION "${UninstallMessageSameFolder}" /SD IDNO IDYES CallCallUninstaller
      Abort

      CallCallUninstaller:
         Push $0
         Call CallUninstaller
   ${EndIf}
FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Installer Initialization Functions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Function .onInit
   ; preedTODO: Include mutex hack
   ; preedTODO: Check drive space
   ${UAC.I.Elevate.AdminOnly}

   Call CommonInstallerInit

   ; Explain this; include details about why it's nightly only, and the
   ; differences in RootAppRegistryKey
   ${If} $InstallerType != "nightly"
      Call PreviousInstallationCheck
   ${EndIf}
FunctionEnd
