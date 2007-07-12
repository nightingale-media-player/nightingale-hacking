
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

; Copies a file to a temporary backup directory and then checks if it is in use
; by attempting to delete the file. If the file is in use an error is displayed
; and the user is given the options to either retry or cancel. If cancel is
; selected then the files are restored.
Function CheckInUse
  ${If} ${FileExists} "$INSTDIR\$R1"
    retry:
    ClearErrors
    CopyFiles /SILENT "$INSTDIR\$R1" "$TmpVal\$R1"
    ${Unless} ${Errors}
      Delete "$INSTDIR\$R1"
    ${EndUnless}
    ${If} ${Errors}
      StrCpy $0 "$INSTDIR\$R1"
      ${WordReplace} "$(^FileError_NoIgnore)" "\r\n" "$\r$\n" "+*" $0
      MessageBox MB_RETRYCANCEL|MB_ICONQUESTION "$0" IDRETRY retry
      Delete "$TmpVal\$R1"
      CopyFiles /SILENT "$TmpVal\*" "$INSTDIR\"
      SetOutPath $INSTDIR
      RmDir /r "$TmpVal"
      Quit
    ${EndIf}
  ${EndIf}
FunctionEnd

Function un.CheckInUse
  ${If} ${FileExists} "$INSTDIR\$R1"
    retry:
    ClearErrors
    CopyFiles /SILENT "$INSTDIR\$R1" "$TmpVal\$R1"
    ${Unless} ${Errors}
      Delete "$INSTDIR\$R1"
    ${EndUnless}
    ${If} ${Errors}
      StrCpy $0 "$INSTDIR\$R1"
      ${un.WordReplace} "$(^FileError_NoIgnore)" "\r\n" "$\r$\n" "+*" $0
      MessageBox MB_RETRYCANCEL|MB_ICONQUESTION "$0" IDRETRY retry
      Delete "$TmpVal\$R1"
      CopyFiles /SILENT "$TmpVal\*" "$INSTDIR\"
      SetOutPath $INSTDIR
      RmDir /r "$TmpVal"
      Quit
    ${EndIf}
  ${EndIf}
FunctionEnd
