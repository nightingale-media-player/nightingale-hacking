
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

   ${If} $DistributionMode == "1"
      System::Call 'Kernel32::SetEnvironmentVariableA(t, t) i("DISTHELPER_DISTINI", "$INSTDIR\distribution\distribution.ini").r0'

      ; We don't really bother checking if we failed here because there's a) not
      ; much we can do about it if so, and b) disthelper.exe will just return
      ; failure if it can't find a distribution.ini set anywhere
      ;StrCmp $0 0 error
      ;
      ; Before doing any uninstallation activities, execute disthelper.exe to 
      ; allow it to do any pre-uninstallation cleanup; it needs a
      ; distribution.ini; hrm... wonder how it's going to get that...
      ExecWait '$INSTDIR\${DistHelperEXE} uninstall'
   ${Else}
      Call un.RemoveBrandingRegistryKeys
   ${EndIf}

   Call un.RemoveAppRegistryKeys
   Call un.UninstallFiles

   ; Refresh desktop.
   System::Call "shell32::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)"
SectionEnd


Function un.RemoveBrandingRegistryKeys
   SetShellVarContext all

   ; Read where start menu shortcuts are installed
   ReadRegStr $0 HKLM $RootAppRegistryKey "Start Menu Folder"

   ; Remove start menu shortcuts and start menu folder.
   ${If} ${FileExists} "$SMPROGRAMS\$0\${BrandFullNameInternal}.lnk"
      RMDir /r "$SMPROGRAMS\$0\*.*"
   ${EndIf}

   ; Read location of desktop shortcut and remove if present.
   ReadRegStr $0 HKLM $RootAppRegistryKey "Desktop Shortcut Location"
   ${If} ${FileExists} $0
      Delete $0
   ${EndIf}

   ; Read location of quicklaunch shortcut and remove if present.
   ReadRegStr $0 HKLM $RootAppRegistryKey "Quicklaunch Shortcut Location"
   ${If} ${FileExists} $0
      Delete $0
   ${EndIf}
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
   DeleteRegKey HKLM "Software\$RootAppRegistryKey"
FunctionEnd 
 
Function un.UninstallFiles
   ; List of files to uninstall
   Delete $INSTDIR\${ApplicationIni}
   Delete $INSTDIR\${FileMainEXE}
   Delete $INSTDIR\${DistHelperEXE}
   !ifndef UsingJemalloc
      Delete $INSTDIR\${CRuntime}
      Delete $INSTDIR\${CPPRuntime}
      Delete $INSTDIR\${CRuntimeManifest}
   !endif
   Delete $INSTDIR\${MozCRuntime}

   Delete $INSTDIR\${PreferredIcon}
   Delete $INSTDIR\${PreferredInstallerIcon}
   Delete $INSTDIR\${PreferredUninstallerIcon}

   Delete $INSTDIR\${VistaIcon}
   ; Log file updater.exe redirects if the PostUpdateWin helper is called
   Delete $INSTDIR\uninstall.update
  
   ; Text files to uninstall
   Delete $INSTDIR\LICENSE.html
   Delete $INSTDIR\TRADEMARK.txt
   Delete $INSTDIR\README.txt
   Delete $INSTDIR\blocklist.xml
  
   ; These files are created by the application
   Delete $INSTDIR\*.chk
 
   ; Mozilla updates can leave this folder behind when updates fail.
   RMDir /r $INSTDIR\updates

   ; Mozilla updates can leave some of these files over when updates fail.
   Delete $INSTDIR\removed-files
   Delete $INSTDIR\active-update.xml
   Delete $INSTDIR\updates.xml
   
   ; List of directories to remove recursively.
   RMDir /r $INSTDIR\chrome
   RMDir /r $INSTDIR\components
   RMDir /r $INSTDIR\defaults
   RMDir /r $INSTDIR\distribution
   RMDir /r $INSTDIR\extensions
   RMDir /r $INSTDIR\jsmodules
   RMDir /r $INSTDIR\plugins
   RMDir /r $INSTDIR\searchplugins
   RMDir /r $INSTDIR\scripts
   RMDir /r $INSTDIR\lib
   RMDir /r $INSTDIR\gst-plugins
   RMDir /r $INSTDIR\${XULRunnerDir}

   ; Remove uninstaller
   Delete $INSTDIR\${FileUninstallEXE}
 
   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; I commented this out, because I don't think we *truly* want to do this. 
   ;; But we might have to later.
   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;SetShellVarContext current
   ;RMDir /r "$APPDATA\Songbird"
   ;RMDir /r "$LOCALAPPDATA\Songbird"
   ;SetShellVarContext all
 
   Call un.DeleteUpdateAddedFiles
  
   ; Do not attempt to remove this directory recursively; see bug 6367
   RMDir $INSTDIR
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
   Call un.SetRootRegistryKey
FunctionEnd
