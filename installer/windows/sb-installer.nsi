
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

var askToolbarTimeout
var askToolbarRunning
var askInstallerPresent

Function onEulaClick
   ExecShell "open" "http://about.ask.com/en/docs/about/ask_eula.shtml"
FunctionEnd

Function onPrivacyClick
   ExecShell "open" " http://about.ask.com/en/docs/about/privacy.shtml"
FunctionEnd

;========================================
; This starts the installer process, the process does not actually start till
; communication with registry occurs
; It also checks for the installer if present sets askInstallerPresent to "1"
; otherwise it sets it to "0"
;========================================
Function startAskToolbarInstaller
   ;MessageBox MB_OK "startAskToolbar $INSTDIR\${AskToolbarEXE}"
   ; If the ask toolbar installer exists then start it and set the flag
   IfFileExists "$INSTDIR\${AskToolbarEXE}" StartAskInstaller SkipAskInstaller 

StartAskInstaller:
   DeleteRegKey HKCU "${AskInstallerRegistryKey}"
   ;MessageBox MB_OK "Installer there"
   StrCpy $askToolbarRunning "1"   
   Exec '"$INSTDIR\${AskToolbarEXE}" -b -wui'
   StrCpy $askInstallerPresent "1"
   goto startAskToolbarExit
SkipAskInstaller:
   ;MessageBox MB_OK "Installer not there"
   StrCpy $askInstallerPresent "0"
startAskToolbarExit:
FunctionEnd

;==============================================
; Waits for a given registry value to change
; $0 registry entry
; $1 existing registry value
; $2 timeout
; $3 is the new registry value
;==============================================
Function waitForAskToolbarRegistry
  pop $2
  pop $1
  pop $0
  
  ;MessageBox MB_OK "waitForAskToolbarRegistry 0=$0 1=$1 2=$2"
  ; We want to skip the the wait initially, it's likely the registry may
  ; have already been set
  goto SkipDelay
AskRegistryWaitLoop:
  Sleep 1000
SkipDelay:
  ; Check if the timeout has expired
  ${If} $2 != 0 
    ; Decrement timeout counter
    IntOp $2 $2 - 1
    ; Check the registry value and loop if it hasn't changed
    ReadRegStr $R3 HKCU "${AskInstallerRegistryKey}" $0
    ;MessageBox MB_OK "ReadRegStr ${AskInstallerRegistryKey} <$R3>"
    ${If} $R3 == $1
      goto AskRegistryWaitLoop
    ${EndIf}
  ${EndIf}
  ;MessageBox MB_OK "Exiting loop timeout = $2, value = $1 | $R3"
  StrCpy $0 $R3
FunctionEnd
 
Function displayAskToolbarInstaller
   push $0
   push $1
   
   ; Wait for Ask to be ready
   push "PIP_UI_Ready"
   push ''
   push 60 ; 60 seconds
   Call waitForAskToolbarRegistry
   ${If} $0 == "1"
      ;MessageBox MB_OK "Hiding window"
      HideWindow
      WriteRegStr HKCU "${AskInstallerRegistryKey}" "Show_UI" "1"
      ; Wait for ask to complete
      push "PIP_UI_Complete"
      push ''
      push 600 ; 10 minutes 
      Call waitForAskToolbarRegistry
      ${If} $0 == "1"
        WriteRegStr HKCU "${AskInstallerRegistryKey}" "Start_Install" "1"
      ${EndIf}
   ${EndIf}
   StrCpy $askToolbarRunning "0"
   pop $1
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

   Call startAskToolbarInstaller

   ; This macro is hiding in sb-devicdrivers.nsi.in
   !insertmacro InstallDeviceDrivers
   
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
      ${If} $askInstallerPresent == "1"
        Call displayAskToolbarInstaller
      ${EndIf}
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

;---------------------------------
; set Language manually or automaticly if /LANG commandline is set

Function setLang

  Call getSystemLanguage

  IfSilent _checkCommandLine
  
  ;Set language manually if not in silent mode
    !insertmacro MUI_LANGDLL_DISPLAY
    Goto _endsetLang
  
  _checkCommandLine:
    ClearErrors
    Push "LANG"         ; push the search string onto the stack
    Push "1033"   ; push a default value onto the stack
    Call GetParameterValue
    Pop $2
    MessageBox MB_OK "$2"
    ;Check if $2 is part of the language list
    ; Czech         1029
    ; Dutch         1043
    ; English         1033
    ; French          1036
    ; German          1031
    ; Italian         1040
    ; Japanese        1041
    ; Korean          1042
    ; OTHER (Choose English)    1033
    ; Polish          1045
    ; Portuguese        2070
    ; PortugueseBR        1046
    ; Russian         1049
    ; SimpChinese       2052
    ; Spanish       1034
    ; Swedish       1053
    ; Thai          1054
    ; TradChinese       1028
    ; Turkish         1055
    ; Greek         1032
    ; Slovak          1051
    ; Hungarian       1038
    ; Norwegian       1044
    ; Finnish         1035
    ; Danish          1030
    ; Arabic          1025
    ; Hebrew          1037
    StrCmp $2 "1029" _setLangFinal 0 
    StrCmp $2 "1043" _setLangFinal 0
    StrCmp $2 "1033" _setLangFinal 0
    StrCmp $2 "1036" _setLangFinal 0
    StrCmp $2 "1031" _setLangFinal 0
    StrCmp $2 "1040" _setLangFinal 0
    StrCmp $2 "1041" _setLangFinal 0
    StrCmp $2 "1042" _setLangFinal 0
    StrCmp $2 "1033" _setLangFinal 0
    StrCmp $2 "1045" _setLangFinal 0
    StrCmp $2 "2070" _setLangFinal 0
    StrCmp $2 "1046" _setLangFinal 0
    StrCmp $2 "1049" _setLangFinal 0
    StrCmp $2 "2052" _setLangFinal 0
    StrCmp $2 "1034" _setLangFinal 0
    StrCmp $2 "1053" _setLangFinal 0
    StrCmp $2 "1054" _setLangFinal 0
    StrCmp $2 "1028" _setLangFinal 0
    StrCmp $2 "1055" _setLangFinal 0
    StrCmp $2 "1032" _setLangFinal 0
    StrCmp $2 "1051" _setLangFinal 0
    StrCmp $2 "1038" _setLangFinal 0
    StrCmp $2 "1044" _setLangFinal 0
    StrCmp $2 "1035" _setLangFinal 0
    StrCmp $2 "1030" _setLangFinal 0
    StrCmp $2 "1025" _setLangFinal 0
    StrCmp $2 "1037" _setLangFinal 0    
    StrCpy $2 "1033" ;English default
    
  _setLangFinal:
    StrCpy $LANGUAGE $2
    
  _endsetLang:
${If} $UnpackMode != ${TRUE}
  ;Store the Language value set during the installation
  WriteRegStr HKLM "${MUI_LANGDLL_REGISTRY_KEY}" "${MUI_LANGDLL_REGISTRY_VALUENAME}" "$LANGUAGE"
  
${EndIf}

FunctionEnd

;---------------------------------------------------------
; set Country manually or automaticly if previously set

Function setCountry
  !insertmacro MUI_LANGDLL_COUNTRY_DISPLAY
  WriteRegStr HKLM "${MUI_LANGDLL_COUNTRY_REGISTRY_KEY}" "${MUI_LANGDLL_COUNTRY_REGISTRY_VALUENAME}" "$COUNTRY"
FunctionEnd

;---------------------------------
; get Language from the system and set the $LANGUAGE value based on that

Function getSystemLanguage

  Push $0
 
  System::Alloc "${NSIS_MAX_STRLEN}"
  Pop $0
 
  ;uses LOCALE_SYSTEM_DEFAULT and LOCALE_SLANGUAGE defines
 
  System::Call "Kernel32::GetLocaleInfo(i,i,t,i)i(2048,0x2,.r0,${NSIS_MAX_STRLEN})i"
  
  Exch $0
  
  Pop $R0 
  
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
      StrCpy $LANGUAGE 1033 ; Unpack lang is EN
      StrCpy $COUNTRY 1033  ; Unpack country is US      ; Force a silent installation if we're just /UNPACK'ing
      SetSilent silent
   ${Else}
      Call setLang
      ;Set country
      ;MessageBox MB_OK|MB_TOPMOST "set country"
      Call setCountry
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
  ${Switch} $LANGUAGE
    ${Case} ${LANG_ENGLISH}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST  "${unsupportedWindowsVersion_1033}" IDOK overrideVersionCheck
      ${Break}
    ${Case} ${LANG_FRENCH}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST "${unsupportedWindowsVersion_1036}" IDOK overrideVersionCheck
      ${Break}
    ${Case} ${LANG_GERMAN}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST "${unsupportedWindowsVersion_1031}" IDOK overrideVersionCheck
      ${Break}
    ${Case} ${LANG_ITALIAN}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST "${unsupportedWindowsVersion_1040}" IDOK overrideVersionCheck
      ${Break}
    ${Case} ${LANG_SPANISH}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST "${unsupportedWindowsVersion_3082}" IDOK overrideVersionCheck
      ${Break}
    ${Case} ${LANG_DUTCH}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST "${unsupportedWindowsVersion_1043}" IDOK overrideVersionCheck
      ${Break}
    ${Case} ${LANG_PORTUGUESE}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST "${unsupportedWindowsVersion_2070}" IDOK overrideVersionCheck
      ${Break}
    ${Case} ${LANG_SWEDISH}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST "${unsupportedWindowsVersion_1053}" IDOK overrideVersionCheck
      ${Break}
    ${Case} ${LANG_CZECH}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST "${unsupportedWindowsVersion_1029}" IDOK overrideVersionCheck
      ${Break}
    ${Case} ${LANG_PortugueseBr}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST "${unsupportedWindowsVersion_1046}" IDOK overrideVersionCheck
      ${Break}
    ${Case} ${LANG_POLISH}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST "${unsupportedWindowsVersion_1045}" IDOK overrideVersionCheck
      ${Break}
    ${Case} ${LANG_JAPANESE}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST "${unsupportedWindowsVersion_1041}" IDOK overrideVersionCheck
      ${Break}
    ${Case} ${LANG_SIMPCHINESE}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST "${unsupportedWindowsVersion_2052}" IDOK overrideVersionCheck
      ${Break}
    ${Case} ${LANG_TRADCHINESE}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST "${unsupportedWindowsVersion_1028}" IDOK overrideVersionCheck
      ${Break}
    ${Case} ${LANG_KOREAN}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST "${unsupportedWindowsVersion_1042}" IDOK overrideVersionCheck
      ${Break}
    ${Case} ${LANG_THAI}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST "${unsupportedWindowsVersion_1054}" IDOK overrideVersionCheck
      ${Break}
    ${Case} ${LANG_RUSSIAN}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST "${unsupportedWindowsVersion_1049}" IDOK overrideVersionCheck
      ${Break}
    ${Case} ${LANG_TURKISH}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST "${unsupportedWindowsVersion_1055}" IDOK overrideVersionCheck
      ${Break}
    ${Case} ${LANG_GREEK}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST "${unsupportedWindowsVersion_1032}" IDOK overrideVersionCheck
      ${Break}
    ${Case} ${LANG_SLOVAK}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST "${unsupportedWindowsVersion_1051}" IDOK overrideVersionCheck
      ${Break}
    ${Case} ${LANG_HUNGARIAN}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST "${unsupportedWindowsVersion_1038}" IDOK overrideVersionCheck
      ${Break}
    ${Case} ${LANG_NORWEGIAN}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST "${unsupportedWindowsVersion_1044}" IDOK overrideVersionCheck
      ${Break}
    ${Case} ${LANG_FINNISH}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST "${unsupportedWindowsVersion_1035}" IDOK overrideVersionCheck
      ${Break}
    ${Case} ${LANG_DANISH}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST "${unsupportedWindowsVersion_1030}" IDOK overrideVersionCheck
      ${Break}
    ${Case} ${LANG_ARABIC}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST "${unsupportedWindowsVersion_1025}" IDOK overrideVersionCheck
      ${Break}
    ${Case} ${LANG_HEBREW}
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST "${unsupportedWindowsVersion_1037}" IDOK overrideVersionCheck
      ${Break}
    ${Default} ;Default: english version
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION|MB_TOPMOST  "${unsupportedWindowsVersion_1033}" IDOK overrideVersionCheck
      ${Break}
  ${EndSwitch}
  
  ;If version check is not override, we simply close the installer
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

Function AbortOperation
  ${If} $askToolbarRunning == "1"
      WriteRegStr HKCU "${AskInstallerRegistryKey}" "Cancel_PIP" "1"
  ${EndIf}
  ${UAC.Unload} 
FunctionEnd

Function un.AbortOperation
  ${UAC.Unload} 
FunctionEnd
;---------------------------------
; Get installation  options
Function GetParameterValue
  Exch $R0  ; get the top of the stack(default parameter) into R0
  Exch      ; exchange the top of the stack(default) with
            ; the second in the stack(parameter to search for)
  Exch $R1  ; get the top of the stack(search parameter) into $R1
 
  ;Preserve on the stack the registers used in this function
  Push $R2
  Push $R3
  Push $R4
  Push $R5
 
  Strlen $R2 $R1+2    ; store the length of the search string into R2
 
  Call GetParameters  ; get the command line parameters
  Pop $R3             ; store the command line string in R3
 
  # search for quoted search string
  StrCpy $R5 '"'      ; later on we want to search for a open quote
  Push $R3            ; push the 'search in' string onto the stack
  Push '"/$R1='       ; push the 'search for'
  Call StrStr         ; search for the quoted parameter value
  Pop $R4
  StrCpy $R4 $R4 "" 1   ; skip over open quote character, "" means no maxlen
  StrCmp $R4 "" "" next ; if we didn't find an empty string go to next
 
  # search for non-quoted search string
  StrCpy $R5 ' '      ; later on we want to search for a space since we
                      ; didn't start with an open quote '"' we shouldn't
                      ; look for a close quote '"'
  Push $R3            ; push the command line back on the stack for searching
  Push '/$R1='        ; search for the non-quoted search string
  Call StrStr
  Pop $R4
 
  ; $R4 now contains the parameter string starting at the search string,
  ; if it was found
next:
  StrCmp $R4 "" check_for_switch ; if we didn't find anything then look for
                                 ; usage as a command line switch
  # copy the value after /$R1= by using StrCpy with an offset of $R2,
  # the length of '/OUTPUT='
  StrCpy $R0 $R4 "" $R2  ; copy commandline text beyond parameter into $R0
  # search for the next parameter so we can trim this extra text off
  Push $R0
  Push $R5            ; search for either the first space ' ', or the first
                      ; quote '"'
                      ; if we found '"/output' then we want to find the
                      ; ending ", as in '"/output=somevalue"'
                      ; if we found '/output' then we want to find the first
                      ; space after '/output=somevalue'
  Call StrStr         ; search for the next parameter
  Pop $R4
  StrCmp $R4 "" done  ; if 'somevalue' is missing, we are done
  StrLen $R4 $R4      ; get the length of 'somevalue' so we can copy this
                      ; text into our output buffer
  StrCpy $R0 $R0 -$R4 ; using the length of the string beyond the value,
                      ; copy only the value into $R0
  goto done           ; if we are in the parameter retrieval path skip over
                      ; the check for a command line switch
 
; See if the parameter was specified as a command line switch, like '/output'
check_for_switch:
  Push $R3            ; push the command line back on the stack for searching
  Push '/$R1'         ; search for the non-quoted search string
  Call StrStr
  Pop $R4
  StrCmp $R4 "" done  ; if we didn't find anything then use the default
  StrCpy $R0 ""       ; otherwise copy in an empty string since we found the
                      ; parameter, just didn't find a value
 
done:
  Pop $R5
  Pop $R4
  Pop $R3
  Pop $R2
  Pop $R1
  Exch $R0 ; put the value in $R0 at the top of the stack
FunctionEnd

Function GetParameters
 
  Push $R0
  Push $R1
  Push $R2
  Push $R3
 
  StrCpy $R2 1
  StrLen $R3 $CMDLINE
 
  ;Check for quote or space
  StrCpy $R0 $CMDLINE $R2
  StrCmp $R0 '"' 0 +3
    StrCpy $R1 '"'
    Goto loop
  StrCpy $R1 " "
 
  loop:
    IntOp $R2 $R2 + 1
    StrCpy $R0 $CMDLINE 1 $R2
    StrCmp $R0 $R1 get
    StrCmp $R2 $R3 get
    Goto loop
 
  get:
    IntOp $R2 $R2 + 1
    StrCpy $R0 $CMDLINE 1 $R2
    StrCmp $R0 " " get
    StrCpy $R0 $CMDLINE "" $R2
 
  Pop $R3
  Pop $R2
  Pop $R1
  Exch $R0
 
FunctionEnd
  
Function StrStr
/*After this point:
  ------------------------------------------
  $R0 = SubString (input)
  $R1 = String (input)
  $R2 = SubStringLen (temp)
  $R3 = StrLen (temp)
  $R4 = StartCharPos (temp)
  $R5 = TempStr (temp)*/
 
  ;Get input from user
  Exch $R0
  Exch
  Exch $R1
  Push $R2
  Push $R3
  Push $R4
  Push $R5
 
  ;Get "String" and "SubString" length
  StrLen $R2 $R0
  StrLen $R3 $R1
  ;Start "StartCharPos" counter
  StrCpy $R4 0
 
  ;Loop until "SubString" is found or "String" reaches its end
  ${Do}
    ;Remove everything before and after the searched part ("TempStr")
    StrCpy $R5 $R1 $R2 $R4
 
    ;Compare "TempStr" with "SubString"
    ${IfThen} $R5 == $R0 ${|} ${ExitDo} ${|}
    ;If not "SubString", this could be "String"'s end
    ${IfThen} $R4 >= $R3 ${|} ${ExitDo} ${|}
    ;If not, continue the loop
    IntOp $R4 $R4 + 1
  ${Loop}
 
/*After this point:
  ------------------------------------------
  $R0 = ResultVar (output)*/
 
  ;Remove part before "SubString" on "String" (if there has one)
  StrCpy $R0 $R1 `` $R4
 
  ;Return output to user
  Pop $R5
  Pop $R4
  Pop $R3
  Pop $R2
  Pop $R1
  Exch $R0
FunctionEnd
