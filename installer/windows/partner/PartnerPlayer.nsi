
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Global Variables
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
var StartMenuDir
;var RootAppRegistryKey

; Modes/Types
var UnpackMode

; Compressor settings
SetCompressor /SOLID lzma
SetCompressorDictSize 64
CRCCheck force

; Vista compatibility - admin priviledges needed to write things such as
; uninstaller registry entries into HKLM
RequestExecutionLevel user

; Addional include directories
; Relative to the objdir, 
!addincludedir ..
; ... and this is $(srcdir)/installer/windows
!addincludedir ..\..\..\..\..\installer\windows\partner

; and add includes/plugins for nsProcess, UAC, and nsiunz
!addincludedir ..\..\..\..\..\dependencies\windows-i686-msvc8\nsis-2.45\extra-plugins\nsProcess\include
!addplugindir ..\..\..\..\..\dependencies\windows-i686-msvc8\nsis-2.45\extra-plugins\nsProcess\Plugin
!addplugindir ..\..\..\..\..\dependencies\windows-i686-msvc8\nsis-2.45\extra-plugins\nsiunz\Release

; ... and UAC
!addincludedir ..\..\..\..\..\dependencies\windows-i686-msvc8\nsis-2.45\extra-plugins\uac
!addplugindir ..\..\..\..\..\dependencies\windows-i686-msvc8\nsis-2.45\extra-plugins\uac\Release\A

; From NSIS
!include FileFunc.nsh
!include LogicLib.nsh
!include MUI.nsh
!include TextFunc.nsh
!include WinMessages.nsh
!include WinVer.nsh
!include WordFunc.nsh
!include x64.nsh

; Extra plugins
!include UAC.nsh
!include nsProcess.nsh

!insertmacro DirState

; The following includes are custom. 
; defines.nsi is generated from defines.nsi.in!
!include defines.nsi
!include functions.nsi

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Product version information. 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
VIProductVersion "${AppVersionWindows}"

VIAddVersionKey "CompanyName"     "${CompanyName}"
VIAddVersionKey "FileDescription" "${BrandShortName} Installer"
VIAddVersioNKey "FileVersion"     "${AppVersionWindows}"
VIAddVersionKey "LegalCopyright"  "© ${CompanyName}"
VIAddVersionKey "LegalTrademarks" "${LegalTrademarks}"
VIAddVersionKey "ProductName"     "${BrandFullName}"
VIAddVersionKey "ProductVersion"  "${AppVersion}"

Name "${BrandFullName}"
OutFile "${PreferredInstallerName}"
InstallDir "${PreferredInstallDir}"

BrandingText "${BrandFullName}"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Modern User Interface Options
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

!define MUI_ABORTWARNING

; Installer Icon.
!define MUI_ICON ${PreferredInstallerIcon}

; Uninstaller Icon.
!define MUI_UNICON ${PreferredUninstallerIcon}

; Installer Welcome / Finish page image.
!define MUI_WELCOMEFINISHPAGE_BITMAP ${NSISDIR}\Contrib\Graphics\Wizard\win.bmp

; Uninstaller Welcome / Finish page image.
!define MUI_UNWELCOMEFINISHPAGE_BITMAP ${NSISDIR}\Contrib\Graphics\Wizard\win.bmp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Installer pages.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Components page
!define MUI_COMPONENTSPAGE_NODESC
!insertmacro MUI_PAGE_COMPONENTS

; Install directory page
!define MUI_DIRECTORYPAGE_VERIFYONLEAVE
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE ValidateInstallationDirectory
!insertmacro MUI_PAGE_DIRECTORY

; Start Menu Folder Page Configuration
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${MuiStartmenupageRegKey}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "${MuiStartmenupageRegName}"
!insertmacro MUI_PAGE_STARTMENU Application $StartMenuDir

; Install Files Page
!insertmacro MUI_PAGE_INSTFILES

; Finish Page
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_FUNCTION LaunchApp
!insertmacro MUI_PAGE_FINISH

; Custom abort
!define MUI_CUSTOMFUNCTION_ABORT AbortOperation
!define MUI_CUSTOMFUNCTION_UNABORT un.AbortOperation

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Uninstaller pages.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Uninstall Confirm Page
!insertmacro MUI_UNPAGE_CONFIRM

; Remove Files Page
!insertmacro MUI_UNPAGE_INSTFILES

; Finish Page
;!define MUI_PAGE_CUSTOMFUNCTION_SHOW un.PromptSurvey
!insertmacro MUI_UNPAGE_FINISH

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Languages
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!insertmacro MUI_LANGUAGE "English" ; First is default

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; UAC Handling 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
${UAC.AutoCodeUnload} "1"


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Installer Options
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Icon ${PreferredInstallerIcon}

ShowInstDetails hide

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Install Sections
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Section "-Application" Section1
   SectionIn 1 RO

   SetOutPath ${InstallerTmpDir}
   File ${SongbirdInstallerEXE}
   File /r partnerdist

   ExecWait '"${InstallerTmpDir}\${SongbirdInstallerEXE}" /S /DIST /D=$INSTDIR\Partner Player' $1
   DetailPrint '"${InstallerTmpDir}\${SongbirdInstallerEXE}" /S /DIST /D=$INSTDIR\Partner Player returned $1'

   ;Call InstallExtensions

   RMDir /r ${InstallerTmpDir}

   WriteUninstaller ${PreferredUninstallerName}
 
   ; Refresh desktop icons
   System::Call "shell32::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)"
SectionEnd

Section "Desktop Icon"
   ${If} $UnpackMode == ${TRUE}
      Goto End
   ${EndIf}

   ; Put the desktop icon in All Users\Desktop
   ;SetShellVarContext all
   ;CreateShortCut "$DESKTOP\${BrandFullNameInternal}.lnk" "$INSTDIR\${FileMainEXE}" "" "$INSTDIR\${PreferredIcon}" 0

   ; Remember that we installed a desktop shortcut.
   ;WriteRegStr HKLM $RootAppRegistryKey ${DesktopShortcutRegName} "$DESKTOP\${BrandFullNameInternal}.lnk"
 
End: 
SectionEnd

Section "QuickLaunch Icon"
   ${If} $UnpackMode == ${TRUE}
      Goto End
   ${EndIf}
  
   ; Put the quicklaunch icon in the current users quicklaunch.
   ;SetShellVarContext current
   ;CreateShortCut "$QUICKLAUNCH\${BrandFullNameInternal}.lnk" "$INSTDIR\${FileMainEXE}" "" "$INSTDIR\${PreferredIcon}" 0

   ; Remember that we installed a quicklaunch shortcut.
   ;WriteRegStr HKLM $RootAppRegistryKey ${QuicklaunchRegName} "$QUICKLAUNCH\${BrandFullNameInternal}.lnk"
End:
SectionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Installer Helper Functions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Function LaunchApp
;   Call CloseApp
;   GetFunctionAddress $0 LaunchAppUserPrivilege 
;   UAC::ExecCodeSegment $0 
FunctionEnd 

Function ValidateInstallationDirectory
RevalidateInstallationDirectory:
   ${DirState} "$INSTDIR" $R0

   ${If} $R0 == 1
      IfSilent +1 +2
         Quit
      ${If} $UnpackMode == ${TRUE}
         Quit
      ${EndIf}

      ; This only works because we haven't changed the name of our uninstaller;
      ; go us.
      IfFileExists "$INSTDIR\${PreferredUninstallerName}" 0 ConfirmDirtyDirectory
         MessageBox MB_YESNO|MB_ICONQUESTION "${UninstallMessageSameFolder}" /SD IDNO IDYES CallCallUninstaller
         Abort

         CallCallUninstaller:
            Push $INSTDIR
            Call CallUninstaller
            Goto RevalidateInstallationDirectory

      ConfirmDirtyDirectory:
         MessageBox MB_YESNO|MB_ICONQUESTION "${DirectoryNotEmptyMessage}" IDYES OverrideDirCheck IDNO NotValid

      NotValid:
         Abort
   ${EndIf}

   OverrideDirCheck:
FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Installer Initialization Functions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Function .onInit
   InitPluginsDir

   ;${UAC.I.Elevate.AdminOnly}

   ; check for unpack.

   ${If} $UnpackMode == ${TRUE}
      ; Force a silent installation if we're just /UNPACK'ing
      SetSilent silent
   ${EndIf}
FunctionEnd

