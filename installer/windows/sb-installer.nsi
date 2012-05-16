
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

var Dialog

var installAskToolbarCB
var installAskToolbarState
var setDefaultSearchEngineCB
var setDefaultSearchEngineState
var setHomePageCB
var setHomePageState
var installingMessage
var groupBox
var toolbarImage
var askToolbarParam

Function askToolbarPage
   push $0
   !insertmacro MUI_HEADER_TEXT "Songbird® Toolbar Installation" "Install the Songbird® Toolbar" 
   ${If} $askInstallChecked == "0"
     StrCpy $askInstallChecked "1"
     call AskToolbarCheck
   ${EndIf}
   ${If} $alreadyInstalled == "1"
      Abort
   ${EndIf}
   nsDialogs::Create 1018
   Pop $Dialog
   ${If} $Dialog == Error
      Abort
   ${EndIf}
   ${NSD_CreateLabel} 0u 0u 100% 11u "• Quick link to Songbird® for Facebook right from your browser"
   pop $0
   ${NSD_CreateLabel} 0u 12u 100% 11u "• Get Songbird® for Android from Google Play"
   pop $0
   ${NSD_CreateLabel} 0u 24u 100% 11u "• Easy access to search"
   pop $0
   ${NSD_CreateBitmap} 0u 36u 100% 22 "" 
   pop $0
   ${NSD_SetImage} $0 $INSTDIR\AskToolbar.bmp $toolbarImage
   
   ${NSD_CreateCheckBox} 0u 54u 100% 11u "Install the Songbird® Toolbar?"
   pop $installAskToolbarCB
   
   ${NSD_SetState} $installAskToolbarCB $installAskToolbarState
   
   ${NSD_CreateGroupBox} 0u 66u 320 28u ""
   pop $groupBox
   ${NSD_CreateCheckBox} 8u 72u 100% 11u "Set Ask.com to be your default search engine?"
   pop $setDefaultSearchEngineCB 
   ${NSD_SetState} $setDefaultSearchEngineCB $setDefaultSearchEngineState
   ${NSD_CreateCheckBox} 8u 82u 100% 11u "Set Ask.com to be your home page and new tabs page?"
   pop $setHomePageCB
   ${NSD_SetState} $setHomePageCB $setHomePageState
   ${NSD_OnClick} $installAskToolbarCB installAskToolbarCBChange
   
   ${NSD_CreateLabel} 5u 98u 100% 11u "By installing this application and associated updater, you agree to the"
   ${NSD_CreateLink} 5u 110u 92u 11u "End User License Agreement"
   pop $0
   ${NSD_OnClick} $0 onEulaClick
   ${NSD_CreateLabel} 100u 110u 12u 11u "and"
   pop $0
   ${NSD_CreateLink} 115u 110u 60u 11u "Privacy Policy."
   pop $0
   ${NSD_OnClick} $0 onPrivacyClick
   ${NSD_CreateLabel} 5u 122u 100% 11u "You can remove this application easily at any time." 
   nsDialogs::Show
   ${NSD_FreeImage} $toolbarImage
   pop $0
FunctionEnd

Function onEulaClick
   ExecShell "open" "http://about.ask.com/en/docs/about/ask_eula.shtml"
FunctionEnd

Function onPrivacyClick
   ExecShell "open" " http://about.ask.com/en/docs/about/privacy.shtml"
FunctionEnd

Function installAskToolbarCBChange
   Push $0
   ${NSD_GetState} $installAskToolbarCB $0
   ${If} $0 == ${BST_CHECKED}
     EnableWindow $setDefaultSearchEngineCB 1
     EnableWindow $setHomePageCB 1
   ${Else}
     EnableWindow $setDefaultSearchEngineCB 0
     EnableWindow $setHomePageCB 0
   ${EndIf}
   Pop $0
FunctionEnd

Function askToolbarLeave
   push $0
   ; Install Ask Toolbar?
   ${NSD_GetState} $installAskToolbarCB $0
   ${If} $0 == ${BST_CHECKED}
      StrCpy $installAskToolbar "1"
      
      StrCpy $askInstallToolbarArg "/tbr"
   
     ; Set default search engine
     ${NSD_GetState} $setDefaultSearchEngineCB $0
     ${If} $0 == ${BST_CHECKED}
        StrCpy $askSetDefaultSearchEngineArg "/sa"
     ${EndIf}
     
     ; Set home page
     ${NSD_GetState} $setHomePageCB $0
     ${If} $0 == ${BST_CHECKED}
        StrCpy $askSetHomePageArg "/hpr"
        Strcpy $askToolbarParam "toolbar=SGD"
     ${Else}
        Strcpy $askToolbarParam "toolbar=SGD2"
     ${EndIf}
     Exec '"$INSTDIR\${AskToolbarEXE}" $askInstallToolbarArg $askSetDefaultSearchEngineArg $askSetHomePageArg $askToolbarParam'
     ${NSD_SetText} $installingMessage "Ask Toolbar installation complete"
  ${EndIf}
  pop $0
FunctionEnd

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
         StrCpy $0 -1
         ExecWait '"$INSTDIR\${DistHelperEXE}" install' $0
         
         ; Check if we errored here and return the exit code.
         IfErrors DistHelperError

         ; See client/tools/disthelper/error.h for return codes
         ${If} $0 != 0
            Goto DistHelperError
         ${Else}
            Goto DistHelperSuccess
         ${EndIf}

         DistHelperError:
            SetErrors
            DetailPrint "$INSTDIR\${DistHelperEXE} install failed: $0"
            ${If} $InstallerMode == "debug"
               MessageBox MB_OK "$INSTDIR\${DistHelperEXE} install failed: $0"
            ${EndIf}

         DistHelperSuccess:
      ${EndIf}

      Call InstallCdrip
      ; Disabled for now; see bug 22964
      ; Call InstallRDSConfig
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

   ; Install Songbird AutoPlay services.
   ExecWait '"$INSTDIR\sbAutoPlayUtil" -Install'

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
   Push $0
   Push $1
   
   ExecWait '"$INSTDIR\${CdripHelperEXE}" install' $0
   
   IfErrors CdripHelperErrors

   ; See client/tools/cdriphelper/error.h for return codes
   ${If} $0 != 0
   ${AndIf} $0 != 128
      Goto CdripHelperErrors
   ${EndIf}

   Goto CdripHelperSuccess

   CdripHelperErrors:
      SetErrors
      DetailPrint "$INSTDIR\${CdripHelperEXE} install failed: $0"

      ${If} $InstallerMode == "debug"
         MessageBox MB_OK "$INSTDIR\${CdripHelperEXE} install failed: $0"
      ${EndIf}

      ; Don't write the registry key if we didn't succeed.
      Goto CdripHelperOut

CdripHelperSuccess:
   WriteRegStr HKLM $RootAppRegistryKey ${CdripRegKey} ${TRUE}

CdripHelperOut:
   Pop $1
   Pop $0
FunctionEnd

Function InstallRDSConfig
   ExecWait '"$INSTDIR\${RDSConfigEXE}" install' $0
   
   IfErrors RDSConfigErrors

   ${If} $0 != 0
      RDSConfigErrors:
      SetErrors
      DetailPrint "$INSTDIR\${RDSConfigEXE} install failed: $0"

      ${If} $InstallerMode == "debug"
         MessageBox MB_OK "$INSTDIR\${RDSConfigEXE} install failed: $0"
      ${EndIf}
   ${EndIf}
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
   Push $2
   
   System::Call 'Kernel32::SetEnvironmentVariableA(t, t) i("SB_INSTALLER_NOMUTEX", "1").r1'
   ExecWait '"$0\${FileUninstallEXE}" /S _?=$0' $1
   DetailPrint '"$0\${FileUninstallEXE}" /S _?=$0 returned $1'

   IfErrors UninstallerFailed

   ${If} $1 != 0
      Goto UninstallerFailed
   ${Else}
      Goto UninstallerSuccess
   ${EndIf}

UninstallerFailed:
   ${If} $InstallerMode == "debug"
      DetailPrint '"$0\${FileUninstallEXE}" /S _?=$0 returned $1'
   ${EndIf}
   Goto UninstallerOut
 
UninstallerSuccess:
   ; We use this key existing as a reasonable hueristic about whether the
   ; installer really did anything (and didn't bail out because it needed
   ; input while in silent mode).
   EnumRegKey $1 HKLM "$RootAppRegistryKey" 0

   ${If} $1 == ""
      Delete $0\${FileUninstallEXE}
   ${EndIf}

   RMDir $0
UninstallerOut: 
   Pop $2
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

Function AskToolbarCheck
  push $R0
  ; See if we need to install
  ExecWait '"$INSTDIR\${AskToolbarEXE}" /tb=SGD' $R0
  ; 0 means Ask is not installed
  ${if} $R0 == "0"
    StrCpy $alreadyInstalled "0"
  ${Else}
    StrCpy $alreadyInstalled "1"
  ${EndIf}
  pop $R0
FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Installer Initialization Functions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Function .onInit
   ; preedTODO: Check drive space

   StrCpy $installAskToolbarState ${BST_CHECKED}
   StrCpy $setDefaultSearchEngineState ${BST_CHECKED}
   StrCpy $setHomePageState ${BST_CHECKED}
   StrCpy $askInstallChecked "0"
   StrCpy $alreadyInstalled "0"
   
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
