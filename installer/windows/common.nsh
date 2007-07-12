
#
# BEGIN SONGBIRD GPL
# 
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2007 POTI, Inc.
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
 * Attempts to delete a file if it exists. This will fail if the file is in use.
 * @param   _FILE
 *          The path to the file that is to be deleted.
 */
!macro DeleteFile _FILE
  ${If} ${FileExists} "${_FILE}"
    Delete "${_FILE}"
  ${EndIf}
!macroend
!define DeleteFile "!insertmacro DeleteFile"

/**
 * Posts WM_QUIT to the application's message window which is found using the
 * message window's class. This macro uses the nsProcess plugin available
 * from http://nsis.sourceforge.net/NsProcess_plugin
 *
 * @param   _MSG
 *          The message text to display in the message box.
 * @param   _PROMPT
 *          If false don't prompt the user and automatically exit the
 *          application if it is running.
 *
 * $R6 = return value for nsProcess::_FindProcess and nsProcess::_KillProcess
 * $R7 = value returned from FindWindow
 * $R8 = _PROMPT
 * $R9 = _MSG
 */
Function CloseApp
  Exch $R9
  Exch 1
  Exch $R8
  Push $R7
  Push $R6

  loop:
  Push $R6
  nsProcess::_FindProcess /NOUNLOAD "${FileMainEXE}"
  Pop $R6
  StrCmp $R6 0 0 end

  StrCmp $R8 "false" +2 0
  MessageBox MB_OKCANCEL|MB_ICONQUESTION "$R9" IDCANCEL exit 0

  FindWindow $R7 "${WindowClass}"
  IntCmp $R7 0 +4
  System::Call 'user32::PostMessage(i r17, i ${WM_QUIT}, i 0, i 0)'
  # The amount of time to wait for the app to shutdown before prompting again
  Sleep 5000

  Push $R6
  nsProcess::_FindProcess /NOUNLOAD "${FileMainEXE}"
  Pop $R6
  StrCmp $R6 0 0 end
  Push $R6
  nsProcess::_KillProcess /NOUNLOAD "${FileMainEXE}"
  Pop $R6
  Sleep 2000

  Goto loop

  exit:
  nsProcess::_Unload
  Quit

  end:
  nsProcess::_Unload

  Pop $R6
  Pop $R7
  Exch $R8
  Exch 1
  Exch $R9
FunctionEnd

Function un.CloseApp
  Exch $R9
  Exch 1
  Exch $R8
  Push $R7
  Push $R6

  loop:
  Push $R6
  nsProcess::_FindProcess /NOUNLOAD "${FileMainEXE}"
  Pop $R6
  StrCmp $R6 0 0 end

  StrCmp $R8 "false" +2 0
  MessageBox MB_OKCANCEL|MB_ICONQUESTION "$R9" IDCANCEL exit 0

  FindWindow $R7 "${WindowClass}"
  IntCmp $R7 0 +4
  System::Call 'user32::PostMessage(i r17, i ${WM_QUIT}, i 0, i 0)'
  # The amount of time to wait for the app to shutdown before prompting again
  Sleep 5000

  Push $R6
  nsProcess::_FindProcess /NOUNLOAD "${FileMainEXE}"
  Pop $R6
  StrCmp $R6 0 0 end
  Push $R6
  nsProcess::_KillProcess /NOUNLOAD "${FileMainEXE}"
  Pop $R6
  Sleep 2000

  Goto loop

  exit:
  nsProcess::_Unload
  Quit

  end:
  nsProcess::_Unload

  Pop $R6
  Pop $R7
  Exch $R8
  Exch 1
  Exch $R9
FunctionEnd

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
