
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; This installer is based on the Firefox NSIS installer by Rob Strong from
; Mozilla. Thanks Rob :) I couldn't have gotten our installer all up to date
; this fast without you.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Compressor settings
SetCompressor /SOLID lzma
SetCompressorDictSize 64
CRCCheck force

; empty files - except for the comment line - for generating custom pages.
;!system 'echo ; > options.ini'
;!system 'echo ; > components.ini'
;!system 'echo ; > shortcuts.ini'

; Addional include directories
!addincludedir ..\installer\windows

Var TmpVal
Var StartMenuDir

;From NSIS
!include FileFunc.nsh
!include LogicLib.nsh
!include TextFunc.nsh
!include WinMessages.nsh
!include WinVer.nsh
!include WordFunc.nsh
!include MUI.nsh
!include nsProcess.nsh
!include x64.nsh

!insertmacro FileJoin
!insertmacro GetTime
!insertmacro LineFind
!insertmacro StrFilter
!insertmacro TrimNewLines
!insertmacro WordFind
!insertmacro WordReplace
!insertmacro un.WordReplace
!insertmacro GetSize
!insertmacro GetParameters
!insertmacro GetParent
!insertmacro GetOptions
!insertmacro GetRoot
!insertmacro DriveSpace

; The following includes are custom. 
; defines.nsi is generated from defines.nsi.in!
!include defines.nsi
!include common.nsh

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Product version information. 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
VIProductVersion "${AppVersionWindows}"

VIAddVersionKey "CompanyName"     "${CompanyName}"
VIAddVersionKey "FileDescription" "${BrandShortName} Installer"
VIAddVersioNKey "FileVersion"     "${InstallerVersion}"
VIAddVersionKey "LegalCopyright"  "© ${CompanyName}"
VIAddVersionKey "LegalTrademark"  "™ ${CompanyName}"
VIAddVersionKey "ProductVersion"  "${AppVersion}"
VIAddVersioNKey "SpecialBuild"    "${DebugBuild}" 

Name "${BrandFullName}"
OutFile "${PreferredInstallerName}"
InstallDir "${PreferredInstallDir}"

BrandingText " "

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
; Force quit message 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!define ForceQuitMessage "Songbird is currently running and will be forced to quit."

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Install Sections
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Section "-Application" Section1
  SectionIn 1 RO

  ${If} ${FileExists} "$INSTDIR\${FileMainEXE}"
    Call CloseApp
  ${EndIf}

  ; Reset output path
  SetOutPath $INSTDIR

  ; Finally try and uninstall the old version if it's present.
  SetShellVarContext all

  ; Read location of old version
  ReadRegStr $R1 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${BrandFullNameInternal}" "UninstallString"

  ${If} $R1 != ""
    ClearErrors
    ExecWait '$R1 /S _?=$INSTDIR'
  ${EndIf}

  ; List of files to install
  File ${ApplicationIni}
  File ${FileMainEXE}
  File ${CRuntime}
  File ${CPPRuntime}
  File ${PreferredIcon}
  File ${VistaIcon}
  
  ; List of text files to install
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

  ; The XULRunner stub loader also fails to find certain symbols when launched
  ; without a profile (yes, it's confusing). The quick work around is to 
  ; leave a copy of msvcr71.dll in xulrunner/ as well.
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
  ; XXXaus - It's unclear to me if we need to do the same thing, need to investigate.
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
  WriteRegStr HKLM "Software\${BrandFullNameInternal\${AppVersion} - (${BUILD_ID})" "InstallDir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${BrandFullNameInternal}" "DisplayName" "${BrandFullName} ${AppVersion} (${BUILD_ID})"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${BrandFullNameInternal}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${BrandFullNameInternal}" "UninstallString" '"$INSTDIR\${FileUninstallEXE}"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${BrandFullNameInternal}" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${BrandFullNameInternal}" "NoRepair" 1
  WriteUninstaller ${FileUninstallEXE}

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application

  CreateDirectory "$SMPROGRAMS\$StartMenuDir"
  CreateShortCut "$SMPROGRAMS\$StartMenuDir\${BrandFullNameInternal}.lnk" "$INSTDIR\${FileMainEXE}" "" "$INSTDIR\$LinkIconFile" 0
  CreateShortCut "$SMPROGRAMS\$StartMenuDir\${BrandFullNameInternal} (Profile Manager).lnk" "$INSTDIR\${FileMainEXE}" "-p" "$INSTDIR\$LinkIconFile" 0 SW_SHOWNORMAL "" "${BrandFullName} w/ Profile Manager"
  CreateShortCut "$SMPROGRAMS\$StartMenuDir\${BrandFullNameInternal} (Safe-Mode).lnk" "$INSTDIR\${FileMainEXE}" "-safe-mode" "$INSTDIR\$LinkIconFile" 0 SW_SHOWNORMAL "" "${BrandFullName} Safe-Mode"
  CreateShortCut "$SMPROGRAMS\$StartMenuDir\Uninstall ${BrandFullNameInternal}.lnk" "$INSTDIR\${FileUninstallEXE}" "" "$INSTDIR\$LinkIconFile" 0

  !insertmacro MUI_STARTMENU_WRITE_END
  
  ; Refresh desktop icons
  System::Call "shell32::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)"
  
SectionEnd

Section "Desktop Icon"

  ; Put the desktop icon in All Users\Desktop
  SetShellVarContext all
  CreateShortCut "$DESKTOP\${BrandFullNameInternal}.lnk" "$INSTDIR\${FileMainEXE}" "" "$INSTDIR\$LinkIconFile" 0

  ; Remember that we installed a desktop shortcut.
  WriteRegStr HKLM "Software\${BrandFullNameInternal}\${AppVersion} (${BUILD_ID})" "Desktop Shortcut Location" "$DESKTOP\${BrandFullNameInternal}.lnk"
  
SectionEnd

Section "QuickLaunch Icon"
  
  ; Put the quicklaunch icon in the current users quicklaunch.
  SetShellVarContext current
  CreateShortCut "$QUICKLAUNCH\${BrandFullNameInternal}.lnk" "$INSTDIR\${FileMainEXE}" "" "$INSTDIR\$LinkIconFile" 0

  ; Remember that we installed a quicklaunch shortcut.
  WriteRegStr HKLM "Software\${BrandFullNameInternal}\${AppVersion} (${BUILD_ID})" "Quicklaunch Shortcut Location" "$QUICKLAUNCH\${BrandFullNameInternal}.lnk"
  
SectionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Uninstall Section
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Section "Uninstall"

  ${If} ${FileExists} "$INSTDIR\${FileMainEXE}"
    Call un.CloseApp
  ${EndIf}
  
  SetShellVarContext all
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${BrandFullNameInternal}"

  ; Remove XULRunner and Songbird to the Windows Media Player Shim Inclusion List.
  DeleteRegKey HKLM "Software\Microsoft\MediaPlayer\ShimInclusionList\${XULRunnerEXE}"
  DeleteRegKey HKLM "Software\Microsoft\MediaPlayer\ShimInclusionList\${FileMainEXE}"

  StrCpy $0 "Software\Microsoft\Windows\CurrentVersion\App Paths\${FileMainEXE}"
  DeleteRegKey HKLM "$0"

  ; Remove uninstaller
  Delete $INSTDIR\${FileUninstallEXE}

  ; Read where start menu shortcuts are installed
  ReadRegStr $0 HKLM "Software\${BrandFullNameInternal}\${AppVersion} (${BUILD_ID})" "Start Menu Folder"

  ; Remove start menu shortcuts and start menu folder.
  ${If} ${FileExists} "$SMPROGRAMS\$0\${BrandFullNameInternal}.lnk"
    RMDir /r "$SMPROGRAMS\$0\*.*"
  ${EndIf}

  ; Read location of desktop shortcut and remove if present.
  ReadRegStr $0 HKLM "Software\${BrandFullNameInternal}\${AppVersion} (${BUILD_ID})" "Desktop Shortcut Location"
  ${If} ${FileExists} $0
    Delete $0
  ${EndIf}

  ; Read location of quicklaunch shortcut and remove if present.
  ReadRegStr $0 HKLM "Software\${BrandFullNameInternal}\${AppVersion} (${BUILD_ID})" "Quicklaunch Shortcut Location"
  ${If} ${FileExists} $0
    Delete $0
  ${EndIf}
  
  ; Remove the last of the registry keys
  DeleteRegKey HKLM "Software\${BrandFullNameInternal}\${AppVersion} (${BUILD_ID})"

  ; List of files to uninstall
  Delete ${ApplicationIni}
  Delete ${FileMainEXE}
  Delete ${CRuntime}
  Delete ${CPPRuntime}
  Delete ${PreferredIcon}
  Delete ${VistaIcon}
  
  ; Text files to uninstall
  Delete LICENSE.txt
  Delete GPL.txt
  Delete TRADEMARK.txt
  
  ; These files are created by the application
  Delete *.chk
  
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

Function LaunchApp
  Call CloseApp
  Exec "$INSTDIR\${FileMainEXE}"
FunctionEnd

Function CloseApp
  loop:

  ${nsProcess::FindProcess} "${FileMainEXE}" $0

  ${If} $0 == 0
    MessageBox MB_OKCANCEL|MB_ICONQUESTION "${ForceQuitMessage}" IDCANCEL exit

    FindWindow $1 "${WindowClass}"
    IntCmp $1 0 +4
    System::Call 'user32::PostMessage(i r17, i ${WM_QUIT}, i 0, i 0)'
    
    ; The amount of time to wait for the app to shutdown before prompting again
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
    
    Sleep 2000
    Goto loop
    
  ${Else}
    goto end
  ${EndIf}
  
  exit:
  ${nsProcess::Unload}
  Quit

  end:
  ${nsProcess::Unload}

FunctionEnd

Function un.CloseApp
  loop:

  ${nsProcess::FindProcess} "${FileMainEXE}" $0

  ${If} $0 == 0
    MessageBox MB_OKCANCEL|MB_ICONQUESTION "${ForceQuitMessage}" IDCANCEL exit

    FindWindow $1 "${WindowClass}"
    IntCmp $1 0 +4
    System::Call 'user32::PostMessage(i r17, i ${WM_QUIT}, i 0, i 0)'
    
    ; The amount of time to wait for the app to shutdown before prompting again
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
    
    Sleep 2000
    Goto loop
    
  ${Else}
    goto end
  ${EndIf}
  
  exit:
  ${nsProcess::Unload}
  Quit

  end:
  ${nsProcess::Unload}
FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Initialization Functions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Function .onInit

FunctionEnd

Function un.onInit

FunctionEnd