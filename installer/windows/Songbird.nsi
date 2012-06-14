
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; This installer is based on the Firefox NSIS installer by Rob Strong from
; Mozilla. Thanks Rob :) I couldn't have gotten our installer all up to date
; this fast without you.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Global Variables
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
var LinkIconFile
var StartMenuDir
var RootAppRegistryKey

; Modes/Types
var InstallerType
var InstallerMode
var UnpackMode
var CheckOSVersion
var DistributionMode
var DistributionName

;Ask toolbar installer variables
var askInstallChecked
var alreadyInstalled

var askInstallToolbarArg
var askSetDefaultSearchEngineArg
var askSetHomePageArg
var installAskToolbar


; Compressor settings
SetCompressor /SOLID lzma
SetCompressorDictSize 64
CRCCheck force

; Vista compatibility - admin priviledges needed to write things such as
; uninstaller registry entries into HKLM
RequestExecutionLevel user

; Addional include directories
; Relative to dist, so this is the $(objdir)/installer/windows...
!addincludedir ..\installer\windows
; ... and this is $(srcdir)/installer/windows
!addincludedir ..\..\installer\windows

; and add includes/plugins for nsProcess... and UAC
!addincludedir ..\..\dependencies\windows-i686\nsis-2.45\extra-plugins\nsProcess\include
!addplugindir ..\..\dependencies\windows-i686\nsis-2.45\extra-plugins\nsProcess\Plugin

; ... and UAC
!addincludedir ..\..\dependencies\windows-i686\nsis-2.45\extra-plugins\uac
!addplugindir ..\..\dependencies\windows-i686\nsis-2.45\extra-plugins\uac\Release\A

; From NSIS
!include FileFunc.nsh
!include LogicLib.nsh
!include MUI.nsh
!include TextFunc.nsh
!include WinMessages.nsh
!include WinVer.nsh
!include WordFunc.nsh
!include x64.nsh
!include nsDialogs.nsh

; Extra plugins
!include UAC.nsh
!include nsProcess.nsh

!insertmacro DirState

; The following includes are custom. 
; defines.nsi is generated from defines.nsi.in!
!include defines.nsi
!include common.nsh
!include sb-filelist.nsi

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
VIAddVersionKey "SpecialBuild"    "${DebugBuild}"
VIAddVersionKey "BuildID"         "${AppBuildNumber}"

Name "${BrandFullName}"
OutFile "${PreferredInstallerName}"
InstallDir "${PreferredInstallDir}"

BrandingText " "

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
!insertmacro MUI_PAGE_LICENSE "license.rtf"
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

; Ask Toolbar Page
Page Custom askToolbarPage askToolbarLeave

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

!include sb-installer.nsi
!include sb-uninstaller.nsi

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; UAC Handling 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
${UAC.AutoCodeUnload} "1"

