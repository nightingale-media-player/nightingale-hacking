
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

SetCompressor /SOLID lzma
SetCompressorDictSize 64

CRCCheck on

!verbose 3

; empty files - except for the comment line - for generating custom pages.
!system 'echo ; > options.ini'
!system 'echo ; > components.ini'
!system 'echo ; > shortcuts.ini'

Var TmpVal
Var StartMenuDir
Var InstallType
Var AddStartMenuSC
Var AddQuickLaunchSC
Var AddDesktopSC

Var fhInstallLog
Var fhUninstallLog

;From NSIS
!include FileFunc.nsh
!include LogicLib.nsh
!include TextFunc.nsh
!include WinMessages.nsh
!include WinVer.nsh
!include WordFunc.nsh
!include MUI.nsh
!include x64.nsh

!insertmacro FileJoin
!insertmacro GetTime
!insertmacro LineFind
!insertmacro StrFilter
!insertmacro TrimNewLines
!insertmacro WordFind
!insertmacro WordReplace
!insertmacro GetSize
!insertmacro GetParameters
!insertmacro GetParent
!insertmacro GetOptions
!insertmacro GetRoot
!insertmacro DriveSpace

; The following includes are custom.
!include defines.nsi

#!include common.nsh
#!include locales.nsi
#!include version.nsh

VIAddVersionKey "FileDescription" "${BrandShortName} Installer"

!insertmacro RegCleanMain
!insertmacro RegCleanUninstall
!insertmacro CloseApp
!insertmacro WriteRegStr2
!insertmacro WriteRegDWORD2
!insertmacro CreateRegKey
!insertmacro CanWriteToInstallDir
!insertmacro CheckDiskSpace
!insertmacro AddHandlerValues
!insertmacro GetSingleInstallPath

#!include shared.nsh

Name "${BrandFullName}"
OutFile "Songbird_${BUILD_ID}_${ARCH}.exe"
InstallDir "${PreferredInstallDir}"

#XXXAus: Try and use this scheme to enable multiple installs of different versions.
#InstallDirRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${BrandFullNameInternal} (${AppVersion} - ${BUILD_ID})" "InstallLocation"

ShowInstDetails nevershow

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Modern User Interface Options
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

!define MUI_ABORTWARNING

; Installer Icon.
!define MUI_ICON ${PreferredInstallerIcon}

; Uninstaller Icon.
!define MUI_UNICON ${PreferredUninstallerIcon}

; Installer should have a header image.
!define MUI_HEADERIMAGE

; Installer header image.
!define MUI_HEADERIMAGE_BITMAP ${NSISDIR}\Contrib\Graphics\Header\nsis.bmp

; Installer Welcome / Finish page image.
!define MUI_WELCOMEFINISHPAGE_BITMAP ${NSISDIR}\Contrib\Graphics\Wizard\win.bmp

; Uninstaller Welcome / Finish page image.
!define MUI_UNWELCOMEFINISHPAGE_BITMAP ${NSISDIR}\Contrib\Graphics\Wizard\win.bmp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Installer pages.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Welcome page options
;!define MUI_WELCOMEPAGE_TITLE
;!define MUI_WELCOMEPAGE_TITLE_3LINES
;!define MUI_WELCOMEPAGE_TEXT

; Welcome page
!insertmacro MUI_PAGE_WELCOME

; Components page options
;!define MUI_COMPONENTSPAGE_TEXT_TOP
;!define MUI_COMPONENTSPAGE_TEXT_COMPLIST
;!define MUI_COMPONENTSPAGE_TEXT_INSTTYPE
;!define MUI_COMPONENTSPAGE_TEXT_DESCRIPTION_TITLE
;!define MUI_COMPONENTSPAGE_TEXT_DESCRIPTION_INFO

; Components page
!insertmacro MUI_PAGE_COMPONENTS

; Install directory page options
;!define MUI_DIRECTORYPAGE_TEXT_TOP
;!define MUI_DIRECTORYPAGE_TEXT_DESTINATION
;!define MUI_DIRECTORYPAGE_VARIABLE

; Install directory page
!insertmacro MUI_PAGE_DIRECTORY

; Custom Shortcuts Page
Page custom preShortcuts leaveShortcuts

; Start Menu Folder Page Configuration
!define MUI_PAGE_CUSTOMFUNCTION_PRE preStartMenu
!define MUI_STARTMENUPAGE_NODISABLE
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\${BrandFullNameInternal}\${AppVersion} (${AB_CD})\Main"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
!insertmacro MUI_PAGE_STARTMENU Application $StartMenuDir

; Install Files Page
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE leaveInstFiles
!insertmacro MUI_PAGE_INSTFILES

; Finish Page
!define MUI_FINISHPAGE_NOREBOOTSUPPORT
!define MUI_FINISHPAGE_TITLE_3LINES
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_FUNCTION LaunchApp
!define MUI_FINISHPAGE_RUN_TEXT $(LAUNCH_TEXT)
!define MUI_PAGE_CUSTOMFUNCTION_PRE preFinish
!insertmacro MUI_PAGE_FINISH

################################################################################
# Install Sections

Section "-Application" Section1
  SectionIn 1 RO
  SetDetailsPrint textonly
  DetailPrint $(STATUS_CLEANUP)
  SetDetailsPrint none

  ; Try to delete the app's main executable and if we can't delete it try to
  ; close the app. This allows running an instance that is located in another
  ; directory and prevents the launching of the app during the installation.
  ; A copy of the executable is placed in a temporary directory so it can be
  ; copied back in the case where a specific file is checked / found to be in
  ; use that would prevent a successful install.

  ; Create a temporary backup directory.
  GetTempFileName $TmpVal "$TEMP"
  ${DeleteFile} $TmpVal
  SetOutPath $TmpVal

  ${If} ${FileExists} "$INSTDIR\${FileMainEXE}"
    ClearErrors
    CopyFiles /SILENT "$INSTDIR\${FileMainEXE}" "$TmpVal\${FileMainEXE}"
    Delete "$INSTDIR\${FileMainEXE}"
    ${If} ${Errors}
      ClearErrors
      ${CloseApp} "true" $(WARN_APP_RUNNING_INSTALL)
      ; Try to delete it again to prevent launching the app while we are
      ; installing.
      ClearErrors
      CopyFiles /SILENT "$INSTDIR\${FileMainEXE}" "$TmpVal\${FileMainEXE}"
      Delete "$INSTDIR\${FileMainEXE}"
      ${If} ${Errors}
        ClearErrors
        ; Try closing the app a second time
        ${CloseApp} "true" $(WARN_APP_RUNNING_INSTALL)
        StrCpy $R1 "${FileMainEXE}"
        Call CheckInUse
      ${EndIf}
    ${EndIf}
  ${EndIf}

  SetOutPath $INSTDIR
  RmDir /r "$TmpVal"
  ClearErrors

  ; Finally try and uninstall the old version if it's present.
  SetShellVarContext all
  
  SectionIn RO
  SetOutPath $INSTDIR

  ReadRegStr $R1 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Songbird" "UninstallString"

  ${If} $R1 != ""
    ClearErrors
    ExecWait '$R1 /S _?=$INSTDIR'
  ${EndIf}

  ; During an install Vista checks if a new entry is added under the uninstall
  ; registry key (e.g. ARP). When the same version of the app is installed on
  ; top of an existing install the key is deleted / added and the Program
  ; Compatibility Assistant doesn't see this as a new entry and displays an
  ; error to the user. See Bug 354000.
  StrCpy $0 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${BrandFullNameInternal} (${AppVersion})"
  DeleteRegKey HKLM "$0"

  ; List of files to install
  File *.ini
  File *.exe
  File *.dll
  File *.ico
  File LICENSE.txt
  File GPL.txt
  File TRADEMARK.txt
  
  ; List of directories to install
  File /r chrome
  File /r components
  File /r defaults
  File /r extensions
  File /r plugins
  File /r searchplugins
  File /r scripts
  File /r xulrunner

  ; Register DLLs
  ; XXXrstrong - AccessibleMarshal.dll can be used by multiple applications but
  ; is only registered for the last application installed. When the last
  ; application installed is uninstalled AccessibleMarshal.dll will no longer be
  ; registered. bug 338878
  ${LogHeader} "DLL Registration"
  ClearErrors
  RegDLL "$INSTDIR\xulrunner\AccessibleMarshal.dll"

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
        ${LogHeader} "Copying QuickTime Scriptable Component"
        CopyFiles /SILENT "$R0" "$INSTDIR\components"
        ${If} ${Errors}
          ${LogMsg} "** ERROR Installing File: $INSTDIR\components\nsIQTScriptablePlugin.xpt **"
        ${Else}
          ${LogMsg} "Installed File: $INSTDIR\components\nsIQTScriptablePlugin.xpt"
          ${LogUninstall} "File: \components\nsIQTScriptablePlugin.xpt"
        ${EndIf}
      ${EndUnless}
    ${EndUnless}
  ${EndUnless}
  ClearErrors

  ; The following keys should only be set if we can write to HKLM
  ${If} $TmpVal == "HKLM"
    ; Uninstall keys can only exist under HKLM on some versions of windows.
    ${SetUninstallKeys}

    ; Set the Start Menu Internet and Vista Registered App HKLM registry keys.
    ${SetStartMenuInternet}

    ; If we are writing to HKLM and create the quick launch and the desktop
    ; shortcuts set IconsVisible to 1 otherwise to 0.
    ${If} $AddQuickLaunchSC == 1
    ${OrIf} $AddDesktopSC == 1
      ${StrFilter} "${FileMainEXE}" "+" "" "" $R9
      StrCpy $0 "Software\Clients\StartMenuInternet\$R9\InstallInfo"
      WriteRegDWORD HKLM "$0" "IconsVisible" 1
    ${Else}
      WriteRegDWORD HKLM "$0" "IconsVisible" 0
    ${EndIf}
  ${EndIf}

  ; These need special handling on uninstall since they may be overwritten by
  ; an install into a different location.
  StrCpy $0 "Software\Microsoft\Windows\CurrentVersion\App Paths\${FileMainEXE}"
  ${WriteRegStr2} $TmpVal "$0" "" "$INSTDIR\${FileMainEXE}" 0
  ${WriteRegStr2} $TmpVal "$0" "Path" "$INSTDIR" 0

  StrCpy $0 "Software\Microsoft\MediaPlayer\ShimInclusionList\$R9"
  ${CreateRegKey} "$TmpVal" "$0" 0

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application

  ; Create Start Menu shortcuts
  ${LogHeader} "Adding Shortcuts"
  ${If} $AddStartMenuSC == 1
    CreateDirectory "$SMPROGRAMS\$StartMenuDir"
    CreateShortCut "$SMPROGRAMS\$StartMenuDir\${BrandFullNameInternal}.lnk" "$INSTDIR\${FileMainEXE}" "" "$INSTDIR\${FileMainEXE}" 0
    ${LogUninstall} "File: $SMPROGRAMS\$StartMenuDir\${BrandFullNameInternal}.lnk"
    CreateShortCut "$SMPROGRAMS\$StartMenuDir\${BrandFullNameInternal} ($(SAFE_MODE)).lnk" "$INSTDIR\${FileMainEXE}" "-safe-mode" "$INSTDIR\${FileMainEXE}" 0
    ${LogUninstall} "File: $SMPROGRAMS\$StartMenuDir\${BrandFullNameInternal} ($(SAFE_MODE)).lnk"
  ${EndIf}

  ; perhaps use the uninstall keys
  ${If} $AddQuickLaunchSC == 1
    CreateShortCut "$QUICKLAUNCH\${BrandFullName}.lnk" "$INSTDIR\${FileMainEXE}" "" "$INSTDIR\${FileMainEXE}" 0
    ${LogUninstall} "File: $QUICKLAUNCH\${BrandFullName}.lnk"
  ${EndIf}

  ${LogHeader} "Updating Quick Launch Shortcuts"
  ${If} $AddDesktopSC == 1
    CreateShortCut "$DESKTOP\${BrandFullName}.lnk" "$INSTDIR\${FileMainEXE}" "" "$INSTDIR\${FileMainEXE}" 0
    ${LogUninstall} "File: $DESKTOP\${BrandFullName}.lnk"
  ${EndIf}

  !insertmacro MUI_STARTMENU_WRITE_END

  ; Refresh desktop icons
  System::Call "shell32::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)"
SectionEnd

################################################################################
# Helper Functions

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

Function onInstallDeleteFile
  ${TrimNewLines} "$R9" "$R9"
  StrCpy $R1 "$R9" 5
  ${If} $R1 == "File:"
    StrCpy $R9 "$R9" "" 6
    ${If} ${FileExists} "$INSTDIR$R9"
      ClearErrors
      Delete "$INSTDIR$R9"
      ${If} ${Errors}
        ${LogMsg} "** ERROR Deleting File: $INSTDIR$R9 **"
      ${Else}
        ${LogMsg} "Deleted File: $INSTDIR$R9"
      ${EndIf}
    ${EndIf}
  ${EndIf}
  ClearErrors
  Push 0
FunctionEnd

; The previous installer removed directories even when they aren't empty so this
; function does as well.
Function onInstallRemoveDir
  ${TrimNewLines} "$R9" "$R9"
  StrCpy $R1 "$R9" 4
  ${If} $R1 == "Dir:"
    StrCpy $R9 "$R9" "" 5
    StrCpy $R1 "$R9" "" -1
    ${If} $R1 == "\"
      StrCpy $R9 "$R9" -1
    ${EndIf}
    ${If} ${FileExists} "$INSTDIR$R9"
      ClearErrors
      RmDir /r "$INSTDIR$R9"
      ${If} ${Errors}
        ${LogMsg} "** ERROR Removing Directory: $INSTDIR$R9 **"
      ${Else}
        ${LogMsg} "Removed Directory: $INSTDIR$R9"
      ${EndIf}
    ${EndIf}
  ${EndIf}
  ClearErrors
  Push 0
FunctionEnd

Function GetDiff
  ${TrimNewLines} "$9" "$9"
  ${If} $9 != ""
    FileWrite $R3 "$9$\r$\n"
    ${LogMsg} "Added To Uninstall Log: $9"
  ${EndIf}
  Push 0
FunctionEnd

Function DoCopyFiles
  StrLen $R2 $R0
  ${LocateNoDetails} "$R0" "/L=FD" "CopyFile"
FunctionEnd

Function CopyFile
  StrCpy $R3 $R8 "" $R2
  retry:
  ClearErrors
  ${If} $R6 ==  ""
    ${Unless} ${FileExists} "$R1$R3\$R7"
      ClearErrors
      CreateDirectory "$R1$R3\$R7"
      ${If} ${Errors}
        ${LogMsg}  "** ERROR Creating Directory: $R1$R3\$R7 **"
        StrCpy $0 "$R1$R3\$R7"
        ${WordReplace} "$(^FileError_NoIgnore)" "\r\n" "$\r$\n" "+*" $0
        MessageBox MB_RETRYCANCEL|MB_ICONQUESTION "$0" IDRETRY retry
        Quit
      ${Else}
        ${LogMsg}  "Created Directory: $R1$R3\$R7"
      ${EndIf}
    ${EndUnless}
  ${Else}
    ${Unless} ${FileExists} "$R1$R3"
      ClearErrors
      CreateDirectory "$R1$R3"
      ${If} ${Errors}
        ${LogMsg}  "** ERROR Creating Directory: $R1$R3 **"
        StrCpy $0 "$R1$R3"
        ${WordReplace} "$(^FileError_NoIgnore)" "\r\n" "$\r$\n" "+*" $0
        MessageBox MB_RETRYCANCEL|MB_ICONQUESTION "$0" IDRETRY retry
        Quit
      ${Else}
        ${LogMsg}  "Created Directory: $R1$R3"
      ${EndIf}
    ${EndUnless}
    ${If} ${FileExists} "$R1$R3\$R7"
      ClearErrors
      Delete "$R1$R3\$R7"
      ${If} ${Errors}
        ${LogMsg} "** ERROR Deleting File: $R1$R3\$R7 **"
        StrCpy $0 "$R1$R3\$R7"
        ${WordReplace} "$(^FileError_NoIgnore)" "\r\n" "$\r$\n" "+*" $0
        MessageBox MB_RETRYCANCEL|MB_ICONQUESTION "$0" IDRETRY retry
        Quit
      ${EndIf}
    ${EndIf}
    ClearErrors
    CopyFiles /SILENT $R9 "$R1$R3"
    ${If} ${Errors}
      ${LogMsg} "** ERROR Installing File: $R1$R3\$R7 **"
      StrCpy $0 "$R1$R3\$R7"
      ${WordReplace} "$(^FileError_NoIgnore)" "\r\n" "$\r$\n" "+*" $0
      MessageBox MB_RETRYCANCEL|MB_ICONQUESTION "$0" IDRETRY retry
      Quit
    ${Else}
      ${LogMsg} "Installed File: $R1$R3\$R7"
    ${EndIf}
    ; If the file is installed into the installation directory remove the
    ; installation directory's path from the file path when writing to the
    ; uninstall.log so it will be a relative path. This allows the same
    ; helper.exe to be used with zip builds if we supply an uninstall.log.
    ${WordReplace} "$R1$R3\$R7" "$INSTDIR" "" "+" $R3
    ${LogUninstall} "File: $R3"
  ${EndIf}
  Push 0
FunctionEnd

Function LaunchApp
  ${CloseApp} "true" $(WARN_APP_RUNNING_INSTALL)
  Exec "$INSTDIR\${FileMainEXE}"
FunctionEnd

################################################################################
# Initialization Functions

Function .onInit
FunctionEnd
