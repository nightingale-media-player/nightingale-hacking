
!macro AbortOperation un
   Function ${un}AbortOperation
      ${UAC.Unload}
   FunctionEnd
!macroend
!insertmacro AbortOperation ""
!insertmacro AbortOperation "un."

Function CallUninstaller
   Exch $0
   Push $1
   Push $2

   System::Call 'Kernel32::SetEnvironmentVariableA(t, t) i("SB_INSTALLER_NOMUTEX", "1").r1'
   ExecWait '"$0\${PreferredUninstallerName}" /S _?=$0' $1
   DetailPrint '"$0\${PreferredUninstallerName}" /S _?=$0 returned $1'

   IfErrors UninstallerFailed

   ${If} $1 != 0
      Goto UninstallerFailed
   ${Else}
      Goto UninstallerSuccess
   ${EndIf}

UninstallerFailed:
   DetailPrint '"$0\${PreferredUninstallerName}" /S _?=$0 returned $1'
   Goto UninstallerOut

UninstallerSuccess:
   ; We use this key existing as a reasonable hueristic about whether the
   ; installer really did anything (and didn't bail out because it needed
   ; input while in silent mode).
   EnumRegKey $1 HKLM "${RootAppRegistryKey}" 0

   ${If} $1 == ""
      Delete $0\${PreferredUninstallerName}
   ${EndIf}

   RMDir $0
UninstallerOut:
   Pop $2
   Pop $1
   Pop $0
FunctionEnd


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
            goto exit
         ${Else}
            MessageBox MB_OK|MB_ICONSTOP "${AppKillFailureMessage}"
            goto exit
         ${EndIf}
      ${EndIf}
  ${Else}
    goto out
  ${EndIf}

exit:
   ${nsProcess::Unload}
   Quit

out:
   ${nsProcess::Unload}
FunctionEnd
!macroend
!insertmacro CloseApp ""
!insertmacro CloseApp "un."

# Initialization stuff common to both the installer and the uninstaller
!macro CommonInstallerInit un
   !insertmacro ${un}GetParameters
   !insertmacro ${un}GetOptions

   Function ${un}CommonInstallerInit
      StrCpy $UnpackMode ${FALSE}

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
   FunctionEnd
!macroend
!insertmacro CommonInstallerInit ""
!insertmacro CommonInstallerInit "un."

Function LaunchAppUserPrivilege 
   Exec '"${SongbirdInstDir}\${FileMainEXE}"'    
FunctionEnd
