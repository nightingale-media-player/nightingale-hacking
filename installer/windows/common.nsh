
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

/**
 * Removes quotes and trailing path separator from registry string paths.
 * @param   $R0
 *          Contains the registry string
 * @return  Modified string at the top of the stack.
 */
!macro GetPathFromRegStr
  Exch $R0
  Push $R8
  Push $R9

  StrCpy $R9 "$R0" "" -1
  StrCmp $R9 '"' +2 0
  StrCmp $R9 "'" 0 +2
  StrCpy $R0 "$R0" -1

  StrCpy $R9 "$R0" 1
  StrCmp $R9 '"' +2 0
  StrCmp $R9 "'" 0 +2
  StrCpy $R0 "$R0" "" 1

  StrCpy $R9 "$R0" "" -1
  StrCmp $R9 "\" 0 +2
  StrCpy $R0 "$R0" -1

  ClearErrors
  GetFullPathName $R8 $R0
  IfErrors +2 0
  StrCpy $R0 $R8

  ClearErrors
  Pop $R9
  Pop $R8
  Exch $R0
!macroend
!define GetPathFromRegStr "!insertmacro GetPathFromRegStr"

!macro CloseApp un
Function ${un}CloseApp
loop:
   ${nsProcess::FindProcess} "${FileMainEXE}" $0

   ${If} $0 == 0
      MessageBox MB_OKCANCEL|MB_ICONQUESTION "${ForceQuitMessage}" IDCANCEL exit

      FindWindow $1 "${WindowClass}"
      IntCmp $1 0 +4
      System::Call 'user32::PostMessage(i r17, i ${WM_QUIT}, i 0, i 0)'
    
      ; The amount of time to wait for the app to shutdown before prompting
      ; again
      Sleep 5000

      ${nsProcess::FindProcess} "${FileMainEXE}" $0

      ${If} $0 == 0
         ${nsProcess::KillProcess} "${FileMainEXE}" $1
         ${If} $1 == 0
            goto end
         ${Else}
            goto exit
         ${EndIf}
      ${Else}
         goto end
      ${EndIf}
    
      sleep 2000
      goto loop
    
  ${Else}
      goto end
  ${EndIf}
  
exit:
   ${nsProcess::Unload}
   Quit

end:
   ${nsProcess::Unload}
FunctionEnd
!macroend
!insertmacro CloseApp ""
!insertmacro CloseApp "un."

; Taken from
; http://nsis.sourceforge.net/Remove_leading_and_trailing_whitespaces_from_a_string
!macro Trim un
   Function ${un}Trim
      Exch $R1 ; Original string
      Push $R2
 
   Loop:
      StrCpy $R2 "$R1" 1
      StrCmp "$R2" " " TrimLeft
      StrCmp "$R2" "$\r" TrimLeft
      StrCmp "$R2" "$\n" TrimLeft
      StrCmp "$R2" "$\t" TrimLeft
      GoTo Loop2

   TrimLeft:   
      StrCpy $R1 "$R1" "" 1
      Goto Loop
 
   Loop2:
      StrCpy $R2 "$R1" 1 -1
      StrCmp "$R2" " " TrimRight
      StrCmp "$R2" "$\r" TrimRight
      StrCmp "$R2" "$\n" TrimRight
      StrCmp "$R2" "$\t" TrimRight
      GoTo Done

   TrimRight:
      StrCpy $R1 "$R1" -1
      Goto Loop2
 
   Done:
      Pop $R2
      Exch $R1

   FunctionEnd
!macroend
!insertmacro Trim ""
!insertmacro Trim "un."

!macro AbortOperation un
   Function ${un}AbortOperation
      ${UAC.Unload} 
   FunctionEnd
!macroend
!insertmacro AbortOperation ""
!insertmacro AbortOperation "un."

;
; BE CAREFUL about changing this; it's used all over the place, so you'll have
; to match up all the cleanup routines in the uninstaller, etc.
;

!macro SetRootRegistryKey un
   Function ${un}SetRootRegistryKey

   ${If} $InstallerType == "dist"
      StrCpy $R1 "$InstallerType\$DistributionName" 
   ${ElseIf} $InstallerType == "release"
      StrCpy $R1 $InstallerType
   ${ElseIf} $InstallerType == "nightly"
      StrCpy $R1 "$InstallerType\${AppBuildNumber}"
   ${Else}
      MessageBox MB_OK "Invalid InstallerType: $InstallerType"
      Abort 
   ${EndIf}

   StrCpy $RootAppRegistryKey "${RootAppRegistryKeyBase}\$R1"

   ${If} $InstallerMode == "debug"
      MessageBox MB_OK "RootAppRegistryKey is $RootAppRegistryKey"
   ${EndIf}

   FunctionEnd
!macroend
!insertmacro SetRootRegistryKey ""
!insertmacro SetRootRegistryKey "un."

# Initialization stuff common to both the installer and the uninstaller
!macro CommonInstallerInit un
   !insertmacro ${un}GetParameters
   !insertmacro ${un}GetOptions

   Function ${un}CommonInstallerInit
      StrCpy $UnpackMode ${FALSE}
      StrCpy $DistributionMode ${FALSE}
      StrCpy $DistributionName ${DefaultDistributionName}
      StrCpy $InstallerMode ${InstallerBuildMode}
      StrCpy $InstallerType ${InstallerBuildType}

      ${${un}GetParameters} $R0

      ClearErrors

      # This really doesn't make sense for the uninstaller, but we want to
      # centralize the option parsing (and other options do make sense for
      # the uninstaller), so we basically just ignore the option, even if
      # it somehow gets set.
      ${${un}GetOptions} $R0 "/UNPACK" "$0"
      IfErrors +2 0
         StrCpy $UnpackMode ${TRUE}

      ${${un}GetOptions} $R0 "/DIST=" $0
      IfErrors +4 0
         StrCpy $DistributionMode ${TRUE}
         StrCpy $InstallerType "dist"
         StrCpy $DistributionName $0

      ${${un}GetOptions} $R0 "/DEBUG" "$0"
      IfErrors +2 0
         StrCpy $InstallerMode "debug"

      Call ${un}SetRootRegistryKey

      ${If} $InstallerMode == "debug"
         MessageBox MB_OK "UnpackMode is $UnpackMode"
         MessageBox MB_OK "DistributionMode is $DistributionMode"
         MessageBox MB_OK "InstallerMode is $InstallerMode"
         MessageBox MB_OK "InstallerType is $InstallerType"
         SetDetailsView show
      ${EndIf}
   FunctionEnd
!macroend
!insertmacro CommonInstallerInit ""
!insertmacro CommonInstallerInit "un."
