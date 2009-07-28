
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

   Call un.RemoveAppRegistryKeys
   !insertmacro un.UninstallFiles

   ; Refresh desktop.
   System::Call "shell32::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)"
SectionEnd


Function un.RemoveBrandingRegistryKeys
   SetShellVarContext all

   ;
   ; Remove AutoPlay registry keys.
   ;

   ; Remove the volume device arrival handler from the PlayMusicFilesOnArrival
   ; event.
   StrCpy $0 "Software\Microsoft\Windows\CurrentVersion\Explorer\AutoPlayHandlers\EventHandlers\PlayMusicFilesOnArrival"
   DeleteRegValue HKLM $0 "${AutoPlayVolumeDeviceArrivalHandlerName}"

   ; Remove the volume device arrival handler.
   StrCpy $0 "Software\Microsoft\Windows\CurrentVersion\Explorer\AutoPlayHandlers\Handlers\${AutoPlayVolumeDeviceArrivalHandlerName}"
   DeleteRegKey HKLM $0

   ; Remove the manage volume device ProgID.
   StrCpy $0 "Software\Classes\${AutoPlayManageVolumeDeviceProgID}"
   DeleteRegKey HKLM $0

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

Function un.RemoveAppRegistryKeys
   ; Remove registry keys
   DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${BrandFullNameInternal}-$InstallerType-${AppBuildNumber}"

   ; Remove XULRunner and Songbird to the Windows Media Player Shim Inclusion List.
   DeleteRegKey HKLM "Software\Microsoft\MediaPlayer\ShimInclusionList\${XULRunnerEXE}"
   DeleteRegKey HKLM "Software\Microsoft\MediaPlayer\ShimInclusionList\${FileMainEXE}"

   StrCpy $0 "Software\Microsoft\Windows\CurrentVersion\App Paths\${FileMainEXE}"
   DeleteRegKey HKLM "$0"

   ; Remove the last of the registry keys
   DeleteRegKey HKLM "$RootAppRegistryKey"

   ; And if we're the last installed copy of Songbird, delete all our reg keys
   DeleteRegKey /ifempty HKLM "${RootAppRegistryKeyBase}\$InstallerType"
   DeleteRegKey /ifempty HKLM "${RootAppRegistryKeyBase}"
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
FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Uninstaller Initialization Functions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Function un.onInit
   ${UAC.U.Elevate.AdminOnly} ${FileUninstallEXE}
   Call un.CommonInstallerInit
FunctionEnd
