
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
   EnumRegKey $1 HKLM "$RootAppRegistryKey" 0

   ${If} $1 == ""
      Delete $0\${PreferredUninstallerName}
   ${EndIf}

   RMDir $0
UninstallerOut:
   Pop $2
   Pop $1
   Pop $0
FunctionEnd

