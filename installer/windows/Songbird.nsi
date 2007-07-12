
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

; Compressor settings
SetCompressor /SOLID lzma
SetCompressorDictSize 64

; empty files - except for the comment line - for generating custom pages.
!system 'echo ; > options.ini'
!system 'echo ; > components.ini'
!system 'echo ; > shortcuts.ini'

; Addional include directories
!addincludedir ..\installer\windows

Var TmpVal
Var StartMenuDir
Var InstallType
Var AddStartMenuSC
Var AddQuickLaunchSC
Var AddDesktopSC

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
!include common.nsh

#VIProductVersion "${AppVersion}"
VIProductVersion "0.3.0.0"

VIAddVersionKey "CompanyName"     "${CompanyName}"
VIAddVersionKey "FileDescription" "${BrandShortName} Installer"
VIAddVersioNKey "FileVersion"     "${InstallerVersion}"
VIAddVersionKey "LegalCopyright"  "© ${CompanyName}"
VIAddVersionKey "LegalTrademark"  "™ ${CompanyName}"
VIAddVersionKey "ProductVersion"  "${AppVersion}"
VIAddVersioNKey "SpecialBuild"    "${DebugBuild}" 

Name "${BrandFullName}"
OutFile "Songbird_${BUILD_ID}_${ARCH}.exe"
InstallDir "${PreferredInstallDir}"

BrandingText " "

#XXXAus: Try and use this scheme to enable multiple installs of different versions.
#InstallDirRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${BrandFullNameInternal} ${AppVersion} (${BUILD_ID})" "InstallLocation"

ShowInstDetails show
ShowUninstDetails show

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Modern User Interface Options
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

!define MUI_ABORTWARNING

; Installer Icon.
!define MUI_ICON ${PreferredInstallerIcon}

; Uninstaller Icon.
!define MUI_UNICON ${PreferredUninstallerIcon}

; Installer should have a header image.
;!define MUI_HEADERIMAGE

; Installer header image.
;!define MUI_HEADERIMAGE_BITMAP ${NSISDIR}\Contrib\Graphics\Header\nsis.bmp

; Installer Welcome / Finish page image.
;!define MUI_WELCOMEFINISHPAGE_BITMAP ${NSISDIR}\Contrib\Graphics\Wizard\win.bmp

; Uninstaller Welcome / Finish page image.
;!define MUI_UNWELCOMEFINISHPAGE_BITMAP ${NSISDIR}\Contrib\Graphics\Wizard\win.bmp

; Language
!insertmacro MUI_LANGUAGE "English"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Installer pages.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; License page
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"

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

; Start Menu Folder Page Configuration
!define MUI_STARTMENUPAGE_NODISABLE
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\${BrandFullNameInternal}\${AppVersion} (${BUILD_ID})"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
!insertmacro MUI_PAGE_STARTMENU Application $StartMenuDir

; Install Files Page
!insertmacro MUI_PAGE_INSTFILES

; Finish Page
!define MUI_FINISHPAGE_NOREBOOTSUPPORT
!define MUI_FINISHPAGE_RUN $INSTDIR/${FileMainEXE}
!define MUI_FINISHPAGE_RUN_TEXT "Launch ${BrandFullName}!"
!insertmacro MUI_PAGE_FINISH

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Uninstaller pages.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Uninstall Confirm Page
!insertmacro MUI_UNPAGE_CONFIRM

; Remove Files Page
!insertmacro MUI_UNPAGE_INSTFILES

; Finish Page
!insertmacro MUI_UNPAGE_FINISH

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Global Variables
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
var LinkIconFile

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Install Sections
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Section "-Application" Section1
  SectionIn 1 RO

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
      StrCpy $R8 "true"
      StrCpy $R9 $(WARN_APP_RUNNING_INSTALL)
      Call CloseApp

      ; Try to delete it again to prevent launching the app while we are
      ; installing.
      ClearErrors
      CopyFiles /SILENT "$INSTDIR\${FileMainEXE}" "$TmpVal\${FileMainEXE}"
      Delete "$INSTDIR\${FileMainEXE}"
      ${If} ${Errors}
        ClearErrors
        ; Try closing the app a second time
        StrCpy $R8 "true"
        StrCpy $R9 $(WARN_APP_RUNNING_INSTALL)
        Call CloseApp
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
  
  ; Reset output path
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
  File ${ApplicationIni}
  File ${FileMainEXE}
  File ${CRuntime}
  File ${CPPRuntime}
  File ${PreferredIcon}
  
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
  File /r ${XULRunnerDir}

  ;The XULRunner stub loader also fails to find certain symbols when launched
  ;without a profile (yes, it's confusing). The quick work around is to 
  ;leave a copy of msvcr71.dll in xulrunner/ as well.
  ${If} ${AtLeastWinVista}
    SetOutPath $INSTDIR\${XULRunnerDir}
    File ${CRuntime} 
    StrCpy $LinkIconFile ${VistaIcon}
  ${Else}
    StrCpy $LinkIconFile ${PreferredIcon}
  ${EndIf}

  ; Register DLLs
  ; XXXrstrong - AccessibleMarshal.dll can be used by multiple applications but
  ; is only registered for the last application installed. When the last
  ; application installed is uninstalled AccessibleMarshal.dll will no longer be
  ; registered. bug 338878
  ClearErrors
  RegDLL "$INSTDIR\${XULRunnerDir}\AccessibleMarshal.dll"

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
        CopyFiles /SILENT "$R0" "$INSTDIR\components"
      ${EndUnless}
    ${EndUnless}
  ${EndUnless}
  ClearErrors

  ; These need special handling on uninstall since they may be overwritten by
  ; an install into a different location.
  StrCpy $0 "Software\Microsoft\Windows\CurrentVersion\App Paths\${FileMainEXE}"
  WriteRegStr HKLM "$0" "$INSTDIR\${FileMainEXE}" ""
  WriteRegStr HKLM "$0" "Path" "$INSTDIR"

  ; Add XULRunner and Songbird to the Windows Media Player Shim Inclusion List.
  WriteRegStr HKLM "Software\Microsoft\MediaPlayer\ShimInclusionList\${XULRunnerEXE}" "" ""
  WriteRegStr HKLM "Software\Microsoft\MediaPlayer\ShimInclusionList\${FileMainEXE}" "" ""

  ; Write the installation path into the registry
  WriteRegStr HKLM Software\Songbird "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${BrandFullNameInternal}" "DisplayName" "${BrandFullName} - ${AppVersion} (${BUILD_ID})"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${BrandFullNameInternal}" "UninstallString" '"$INSTDIR\${FileUninstallEXE}.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${BrandFullNameInternal}" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${BrandFullNameInternal}" "NoRepair" 1
  WriteUninstaller ${FileUninstallEXE}

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application

  CreateDirectory "$SMPROGRAMS\$StartMenuDir"
  CreateShortCut "$SMPROGRAMS\$StartMenuDir\${BrandFullNameInternal}.lnk" "$INSTDIR\${FileMainEXE}" "" "$INSTDIR\$LinkIconFile" 0
  CreateShortCut "$SMPROGRAMS\$StartMenuDir\Uninstall ${BrandFullNameInternal}.lnk" "$INSTDIR\${FileUninstallEXE}" "" "$INSTDIR\$LinkIconFile" 0

  !insertmacro MUI_STARTMENU_WRITE_END
  
  ; Refresh desktop icons
  System::Call "shell32::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)"
SectionEnd

Section "Desktop Icon"
  CreateShortCut "$DESKTOP\${BrandFullNameInternal}.lnk" "$INSTDIR\${FileMainEXE}" "" "$INSTDIR\$LinkIconFile" 0
SectionEnd

Section "QuickLaunch Icon"
  CreateShortCut "$QUICKLAUNCH\${BrandFullNameInternal}.lnk" "$INSTDIR\${FileMainEXE}" "" "$INSTDIR\$LinkIconFile" 0
SectionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Uninstall Section
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Section "Uninstall"
  SetShellVarContext all
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${BrandFullNameInternal}"
  DeleteRegKey HKLM "Software\${BrandFullNameInternal}"

  ; Remove XULRunner and Songbird to the Windows Media Player Shim Inclusion List.
  DeleteRegKey HKLM "Software\Microsoft\MediaPlayer\ShimInclusionList\${XULRunnerEXE}"
  DeleteRegKey HKLM "Software\Microsoft\MediaPlayer\ShimInclusionList\${FileMainEXE}"

  StrCpy $0 "Software\Microsoft\Windows\CurrentVersion\App Paths\${FileMainEXE}"
  DeleteRegKey HKLM "$0"

  ; Remove files and uninstaller
  Delete $INSTDIR\${FileUninstallEXE}

  ; Read where shortcuts are installed
  ReadRegStr $0 HKLM "Software\${BrandFullNameInternal}\${AppVersion} (${BUILD_ID})" "Start Menu Folder"

  ; Remove shortcuts, if any.
  ${If} ${FileExists} "$SMPROGRAMS\$0\*.*"
    RMDir /r "$SMPROGRAMS\$0\*.*"
  ${EndIf}

  ; Remove desktop and quicklaunch shortcuts.  
  Delete "$DESKTOP\${BrandNameFullInternal}.lnk"
  Delete "$QUICKLAUNCH\${BrandNameFullInternal}.lnk"

  ; Remove directories used
  Delete $INSTDIR\LICENSE.txt
  Delete $INSTDIR\GPL.txt
  Delete $INSTDIR\TRADEMARK.txt
  
  ; List of files to install
  Delete ${ApplicationIni}
  Delete ${FileMainEXE}
  Delete ${CRuntime}
  Delete ${CPPRuntime}
  Delete ${PreferredIcon}
  Delete *.chk
  
  Delete LICENSE.txt
  Delete GPL.txt
  Delete TRADEMARK.txt
  
  ; List of directories to install
  RMDir /r chrome
  RMDir /r components
  RMDir /r defaults
  RMDir /r extensions
  RMDir /r plugins
  RMDir /r searchplugins
  RMDir /r scripts
  RMDir /r ${XULRunnerDir}

  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;; I commented this out, because I don't think we *truly* want to do this. 
  ;; But we might have to later.
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;SetShellVarContext current
  ;RMDir /r "$APPDATA\Songbird"
  ;RMDir /r "$LOCALAPPDATA\Songbird"
  ;SetShellVarContext all
  
  RMDir /r "$INSTDIR"
  
  ; Refresh desktop.
  System::Call "shell32::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)"
  
SectionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Helper Functions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

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

Function LaunchApp
  StrCpy $R8 "true"
  StrCpy $R9 $(WARN_APP_RUNNING_INSTALL)
  Call CloseApp
  Exec "$INSTDIR\${FileMainEXE}"
FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Initialization Functions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Function .onInit
FunctionEnd
