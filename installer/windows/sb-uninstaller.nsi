
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

!insertmacro un.GetParameters
!insertmacro un.GetOptions
!insertmacro un.LineRead

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Uninstaller Options
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
UninstallIcon ${PreferredUninstallerIcon}

ShowUninstDetails hide

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Uninstall Section
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Section "Uninstall"
   Call un.CloseApp

   ${If} ${TRUE} == $DistributionMode
      System::Call 'Kernel32::SetEnvironmentVariableA(t, t) i("DISTHELPER_DISTINI", "$INSTDIR\distribution\distribution.ini").r0'
   ${EndIf}

   ; Before doing any uninstallation activities, execute disthelper.exe to 
   ; allow it to do any pre-uninstallation cleanup; it does this (obviously)
   ; in distribution mode, but now that we have a songbird.ini for our own
   ; tasks, we need to always call it.
   ;
   ; We don't really bother checking if we failed here because there's not
   ; much we can do about it; we do save off the logfile if it hasn't been
   ; deleted (disthelper will delete the logfile itself if everything is 
   ; successful) for later inspection/debugging

   ExecWait '"$INSTDIR\${DistHelperEXE}" uninstall' $0
   DetailPrint "$INSTDIR\${DistHelperEXE} exit: $0"
   IfFileExists "$INSTDIR\${DistHelperLog}" 0 +2
      Rename "$INSTDIR\${DistHelperLog}" "$TEMP\${DistHelperLog}"

   ${If} $DistributionMode != ${TRUE}
      Call un.RemoveBrandingRegistryKeys
   ${EndIf}

   ; Disabled for now; see bug 22964
   ; Call un.RDSConfigRemove
   Call un.RemoveCdrip
   Call un.RemoveAppRegistryKeys

   Call un.CleanVirtualStore

   ; This macro is hiding in sb-filelist.nsi.in
   !insertmacro un.UninstallFiles
   !insertmacro un.UninstallDeviceDrivers
   ; Refresh desktop.
   System::Call "shell32::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)"
SectionEnd


Function un.RemoveBrandingRegistryKeys
   SetShellVarContext all

   ; Remove Songbird AutoPlay services.
   ExecWait '"$INSTDIR\sbAutoPlayUtil" -Remove'

   ; Read where start menu shortcuts are installed
   ReadRegStr $R0 HKLM $RootAppRegistryKey ${MuiStartmenupageRegName}

   ; Remove start menu shortcuts and start menu folder.
   ${If} ${FileExists} "$SMPROGRAMS\$R0\${BrandFullNameInternal}.lnk"
      RMDir /r "$SMPROGRAMS\$R0\*.*"
   ${EndIf}

   ; Read location of desktop shortcut and remove if present.
   ReadRegStr $R0 HKLM $RootAppRegistryKey ${DesktopShortcutRegName}
   ${If} ${FileExists} $R0
      Delete $R0
   ${EndIf}

   ; Read location of quicklaunch shortcut and remove if present.
   ReadRegStr $R0 HKLM $RootAppRegistryKey ${QuicklaunchRegName}
   ${If} ${FileExists} $R0
      Delete $R0
   ${EndIf}

   ; Always remove this temporary key on uninstall; we shouldn't ever have it
   ; around
   DeleteRegKey HKLM ${MuiStartmenupageRegKey}
FunctionEnd

Function un.RemoveCdrip
   ReadRegStr $0 HKLM $RootAppRegistryKey ${CdripRegKey}

   ${If} $0 == ${TRUE}
      ; We don't check the return value or error flag here because
      ; there's nothing we can do about it and we want to finish uninstalling
      ; so that we cleanup after ourselves as much as possible.
      ExecWait '"$INSTDIR\${CdripHelperEXE}" remove' $0
      ${If} $InstallerMode == "debug"
         MessageBox MB_OK "$INSTDIR\${CdripHelperEXE} returned $0"
      ${EndIf}
      ; If the CD ripping/Gearworks driver is flagged for removal
      ; we need to reboot to remove it. Only prompt to reboot
      ; if neccessary 
      ReadRegDWORD $0 HKLM "${CdripServiceRegKey}" "DeleteFlag"
      ${If} $0 == "1"
         SetRebootFlag true
      ${EndIf} 
   ${EndIf}

   DeleteRegKey HKLM "$RootAppRegistryKey\${CdripRegKey}"
   DeleteRegKey /ifempty HKLM "${RootAppRegistryKeyBase}\${CdripDriverInstallations}"
FunctionEnd

Function un.RDSConfigRemove
   ; We mostly ignore the return value because there's not much we can do!
   ExecWait '"$INSTDIR\${RDSConfigEXE}" remove' $0
   ${If} $InstallerMode == "debug"
      MessageBox MB_OK "$INSTDIR\${RDSConfigEXE} returned $0"
   ${EndIf}
FunctionEnd

Function un.RemoveAppRegistryKeys
   ; Unregister DLLs
   UnRegDLL "$INSTDIR\lib\sbWindowsFormatter.dll"

   ; Remove registry keys
   DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${BrandFullNameInternal}-$InstallerType-${AppBuildNumber}"

   ; Remove XULRunner and Songbird to the Windows Media Player Shim Inclusion List.
   DeleteRegKey HKLM "Software\Microsoft\MediaPlayer\ShimInclusionList\${XULRunnerEXE}"
   DeleteRegKey HKLM "Software\Microsoft\MediaPlayer\ShimInclusionList\${FileMainEXE}"

   StrCpy $0 "Software\Microsoft\Windows\CurrentVersion\App Paths\${FileMainEXE}"
   DeleteRegKey HKLM "$0"

   ; Remove the last of the registry keys
   DeleteRegKey HKLM "$RootAppRegistryKey"
   DeleteRegKey HKLM "Software\Philips Songbird\FirmwareImages\IniFile"

   ; And if we're the last installed copy of Songbird, delete all our reg keys
   DeleteRegKey /ifempty HKLM "${RootAppRegistryKeyBase}\$InstallerType"
   DeleteRegKey /ifempty HKLM "${RootAppRegistryKeyBase}"
   DeleteRegKey /ifempty HKLM "Software\Philips Songbird"

FunctionEnd 
 
;
; Show a prompt to allow the user to take a survey.
;
Function un.PromptSurvey
   ; This check covers dist mode as well, because we set the type to be
   ; "dist" in distribution mode
   ${If} $InstallerType == "release"
      MessageBox MB_YESNO|MB_ICONQUESTION "${TakeSurveyMessage}" IDNO noSurvey
      ExecShell "open" "${UnintallSurveyURL}"
   ${EndIf}
noSurvey:
FunctionEnd

Function un.DeleteUpdateAddedFiles
   ${If} ${FileExists} "$INSTDIR\${AddedFilesList}"
      StrCpy $1 0

      deleteLoop:
         ; Put the increment up here, so we can "goto deleteLoop" in all 
         ; cases, and get the line incremented
         IntOp $1 $1 + 1
         ${un.LineRead} "$INSTDIR\${AddedFilesList}" "$1" $0
         IfErrors deleteAddedFilesDone

         Push $0
         Call un.Trim
         Pop $0
         StrCpy $2 $0 1 0 ; See if we're a comment
         ${If} $2 == "#"
            goto deleteLoop
         ${EndIf}

         ; Get the last character of the file name; checking to see if we're a 
         ; directory
         StrCpy $2 $0 1 -1

         ${If} $2 == "\"
            RMDir /r $INSTDIR\$0
         ${Else}
            Delete $INSTDIR\$0
         ${EndIf}
     
         goto deleteLoop

      deleteAddedFilesDone:
         Delete "$INSTDIR\${AddedFilesList}"
   ${EndIf}
   ; Delete the ini file we created
   Delete "$INSTDIR\Philips\Devices\Firmware\DefaultFirmwareImages.ini"
FunctionEnd

; Based off an original CleanVirtualStore function written by Rob Strong 
; (rstrong@mozilla.com); see mozbug 387385

/**
 * If present removes the VirtualStore directory for this installation. Uses the
 * program files directory path and the current install location to determine
 * the sub-directory in the VirtualStore directory.
 */
Function un.CleanVirtualStore
   Push $R9
   Push $R8
   Push $R7

   StrLen $R9 "$INSTDIR"

   ; Get the installation's directory name including the preceding slash
   start:
      IntOp $R8 $R8 - 1
      IntCmp $R8 -$R9 end end 0
      StrCpy $R7 "$INSTDIR" 1 $R8
      StrCmp $R7 "\" 0 start

   StrCpy $R9 "$INSTDIR" "" $R8

   ClearErrors
   GetFullPathName $R8 "$PROGRAMFILES$R9"
   IfErrors end
   GetFullPathName $R7 "$INSTDIR"

   ; Compare the installation's directory path with the path created by
   ; concatenating the installation's directory name and the path to the
   ; program files directory.
   StrCmp "$R7" "$R8" 0 end

   StrCpy $R8 "$PROGRAMFILES" "" 2 ; Remove the drive letter and colon
   StrCpy $R7 "$PROFILE\AppData\Local\VirtualStore$R8$R9"

   IfFileExists "$R7" 0 end
   RmDir /r "$R7"

   end:
      ClearErrors

   Pop $R7
   Pop $R8
   Pop $R9
FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Uninstaller Initialization Functions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Function un.onInit
   ${UAC.U.Elevate.AdminOnly} ${FileUninstallEXE}
   Call un.CommonInstallerInit
FunctionEnd
