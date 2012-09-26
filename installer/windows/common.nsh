
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

;
; Bah; super-complicated function, so let's keep track of our registers here:
; checkApp:
;   $0 - result of nsProcess::FindProcess
;   $1 - result of FindWindow() and KillProcess()
; 
; checkAgent:
;   $0 - result of nsProcess::FindProcess
;   $4 - space to allocate for the executable name
;   $5 - negative(strlen($AgentEXE))
;   $R1 - pBytesReturned from Psapi::EnumProcesses(); also the munged path of
;         the executable that we match against
;   $R2 - calculated number of processes returned from Psapi::EnumProcesses()
;   $R4 - Counter to iterate through R2
;   $R5 - The PID in the procIterate loop
;   $R6 - Allocated memory to store the path of the executable
;   $R7 - Return val from GetModuleFileNameEx 
;   $R8 - (throw-away) return value of various System::Calls; also the process
;         handle used to query the path of the executable
;   $R9 - allocated memory for EnumProcesses()

!macro CloseApp un
Function ${un}CloseApp
checkApp:
   ${nsProcess::FindProcess} "${FileMainEXE}" $0

   ${If} $0 == 0
      IfSilent exit
      MessageBox MB_OKCANCEL|MB_ICONQUESTION "${ForceQuitMessage}" IDCANCEL exit

      FindWindow $1 "${WindowClass}"
      ; If we can't find the window, but the process is running, just kill
      ; the app.
      IntCmp $1 0 killApp
      DetailPrint "${DetailPrintQuitApp}"
      System::Call 'user32::PostMessage(i, i, i, i) v ($1, ${WM_QUIT}, 0, 0)'
    
      DetailPrint "${DetailPrintQuitAppWait}"
      sleep ${QuitApplicationWait}

      ${nsProcess::FindProcess} "${FileMainEXE}" $0

      ${If} $0 == 0
      killApp:
         DetailPrint "${DetailPrintKillApp}"
         ${nsProcess::KillProcess} "${FileMainEXE}" $1
         ${If} $1 == 0
            ; We sleep here, just in case the Bird kicked off the agent before
            ; quitting.
            DetailPrint "${DetailPrintQuitAppWait}"
            sleep ${QuitApplicationWait}
            goto checkAgent
         ${Else}
            MessageBox MB_OK|MB_ICONSTOP "${AppKillFailureMessage}"
            goto exit
         ${EndIf}
      ${EndIf}
  ${EndIf}

checkAgent:
   ${nsProcess::FindProcess} "${AgentEXE}" $0

   ;
   ; This code largely based upon 
   ; http://nsis.sourceforge.net/Get_a_list_of_running_processes
   ;

  
   ; Initialize various loops variables that don't change outside of the loop
   ; Allocate enough for a unicode string...
   IntOp $4 ${NSIS_MAX_STRLEN} * 2
   StrLen $5 ${AgentEXE}
   IntOp $5 0 - $5
   ${If} $0 == 0
      System::Alloc ${NSIS_MAX_STRLEN}
      Pop $R9
      System::Call "Psapi::EnumProcesses(i R9, i ${NSIS_MAX_STRLEN}, *i .R1)i .R8"
      StrCmp $R8 0 procHandleError

      ; Divide by sizeof(DWORD) to get # of processes into $R2
      IntOp $R2 $R1 / 4 

      ; R4 is our counter variable
      StrCpy $R4 0 

      procIterate:
         System::Call "*$R9(i .R5)" ; Get next PID
         ; break if PID < 0, continue if PID = 0
         IntCmp $R5 0 procNextIteration procIterateEnd 0 

         ; "Magic" number 1040 is what 
         ; PROCESS_QUERY_INFORMATION | PROCESS_VM_READ turns out to be; see:
         ; http://msdn.microsoft.com/en-us/library/ms684880%28VS.85%29.aspx
         System::Call "Kernel32::OpenProcess(i 1040, i 0, i R5)i .R8"
         StrCmp $R8 0 procNextIteration
         System::Alloc $4
         Pop $R6

         System::Call "Psapi::GetModuleFileNameEx(i R8, i 0, t .R6, i ${NSIS_MAX_STRLEN})i .R7"
         System::Call "Kernel32::CloseHandle(i R8)"
         StrCmp $R7 0 procNextIteration
         ${If} $InstallerMode == "debug"
            MessageBox MB_OK "GetModuleFileNameEx(pid $R5): rv: $R7, $R6"
         ${EndIf}

         ; Check to see if the process ($R6, with PID $R5) matches the agent
         ; string

         StrCpy $R1 $R6 "" $5

         ; Looks like this is our agent...
         ${If} $R1 == ${AgentEXE}
            ${If} $InstallerMode == "debug"
               MessageBox MB_OK "Found ${AgentEXE} process called $R6 with PID $R5"
            ${EndIf}

            DetailPrint "${DetailPrintQuitAgent}"
            
            ; No need to check for errors here as we'll catch them anyway
            ; when we check to make sure the process is actually gone.
            ExecWait '"$R6" --kill'
            
            DetailPrint "${DetailPrintQuitAppWait}"
            sleep ${SubApplicationWait}
            ${nsProcess::FindProcess} "${AgentEXE}" $0
            ${If} $0 == 0
               IfSilent procIterateExit
               MessageBox MB_OK|MB_ICONSTOP "${AppKillFailureMessage}"
               goto procIterateExit
            ${EndIf}
            Goto procIterateEnd
         ${EndIf}

   procNextIteration:
      System::Free $R6
      IntOp $R4 $R4 + 1 ; Add 1 to our counter
      IntOp $R9 $R9 + 4 ; Add sizeof(int) to our buffer address
 
      IntCmp $R4 $R2 procIterateEnd procIterate procIterateEnd
   procIterateEnd:
      System::Free $R6
      System::Free $R9
      Goto out
   procIterateExit:
      System::Free $R6
      System::Free $R9
      Goto exit
   procHandleError:
      ${If} $InstallerMode == "debug"
         MessageBox MB_OK "checkAgent: HandleError() called: $R4, $R6, $R5"
      ${EndIf}
      Goto exit
   ${EndIf}
   Goto out
 
exit:
   ${nsProcess::Unload}
   Quit

out:
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
      StrCpy $CheckOSVersion ${TRUE}
      StrCpy $DistributionMode ${FALSE}
      StrCpy $DistributionName ${DefaultDistributionName}
      StrCpy $InstallerMode ${InstallerBuildMode}
      StrCpy $InstallerType ${InstallerBuildType}

      ${${un}GetParameters} $R1
      ClearErrors

      ${${un}GetOptions} $R1 "/NOMUTEX" $0
      IfErrors +2 0
         StrCpy $0 ${TRUE}
         
      ${If} $0 == ""
         ReadEnvStr $0 SB_INSTALLER_NOMUTEX
      ${EndIf}
      
      ${If} $0 == ""
         System::Call 'kernel32::CreateMutexA(i, i, t) v (0, 0, "${InstallerMutexName}") ?e'
         Pop $R0
         ${If} $R0 != 0
            IfSilent +2
            MessageBox MB_OK|MB_ICONSTOP "${InstallerRunningMesg}"
            Abort
         ${EndIf}
      ${EndIf}
      ClearErrors

      # This really doesn't make sense for the uninstaller, but we want to
      # centralize the option parsing (and other options do make sense for
      # the uninstaller), so we basically just ignore the option, even if
      # it somehow gets set.
      ${${un}GetOptions} $R1 "/UNPACK" $0
      IfErrors +2 0
         StrCpy $UnpackMode ${TRUE}
         
      ReadEnvStr $0 SB_INSTALLER_UNPACK
      ${If} $0 != ""
         StrCpy $UnpackMode ${TRUE}
      ${EndIf}
      ClearErrors

      ${${un}GetOptions} $R1 "/NOOSVERSIONCHECK" $0
      IfErrors +2 0
         StrCpy $CheckOSVersion ${FALSE}
      ClearErrors

      ${${un}GetOptions} $R1 "/DIST" $0
      IfErrors +4 0
         StrCpy $DistributionMode ${TRUE}
         StrCpy $InstallerType "dist"
         StrCpy $DistributionName $0 "" 1 # strip the "=" from argument's "=foo"

      ReadEnvStr $0 SB_INSTALLER_DIST
      ${If} $0 != ""
         StrCpy $DistributionMode ${TRUE}
         StrCpy $InstallerType "dist"
         StrCpy $DistributionName $0 # no strip; env vars don't have "=" in it
      ${EndIf}
      ClearErrors

      ${If} $DistributionMode == ${TRUE}
         ${If} $DistributionName == ""
            DetailPrint "In dist mode, but empty dist name; aborting..."
            Abort
         ${EndIf}
      ${EndIf}

      ${${un}GetOptions} $R1 "/DEBUG" $0
      IfErrors +2 0
         StrCpy $InstallerMode "debug"
         
      ReadEnvStr $0 SB_INSTALLER_DEBUG
      ${If} $0 != ""
         StrCpy $InstallerMode "debug"
      ${EndIf}
      ClearErrors

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
