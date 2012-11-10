
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
Var /GLOBAL COUNTRY

; Modes/Types
var InstallerType
var InstallerMode
var UnpackMode
var CheckOSVersion
var DistributionMode
var DistributionName

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
!addincludedir ..\..\dependencies\windows-i686-msvc8\nsis-2.46\extra-plugins\nsProcess\include
!addplugindir ..\..\dependencies\windows-i686-msvc8\nsis-2.46\extra-plugins\nsProcess\Plugin

; ... and UAC
!addincludedir ..\..\dependencies\windows-i686-msvc8\nsis-2.46\extra-plugins\uac
!addplugindir ..\..\dependencies\windows-i686-msvc8\nsis-2.46\extra-plugins\uac\Unicode

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Modern User Interface Options
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

!define MUI_ABORTWARNING

; Installer Welcome / Finish page image.
!define MUI_WELCOMEFINISHPAGE_BITMAP ${NSISDIR}\Contrib\Graphics\Wizard\win.bmp

; Uninstaller Welcome / Finish page image.
!define MUI_UNWELCOMEFINISHPAGE_BITMAP ${NSISDIR}\Contrib\Graphics\Wizard\win.bmp

; Installer Icon.
!define MUI_ICON ${PreferredInstallerIcon}
; Uninstaller Icon.
!define MUI_UNICON ${PreferredUninstallerIcon}


;--------------------------------

;Languages
!include MUI2.nsh
!include MUI_Country.nsh

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

!insertmacro MUI_LANGUAGE "English" ;first language is the default language
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "Italian"
!insertmacro MUI_LANGUAGE "Spanish"
!insertmacro MUI_LANGUAGE "Dutch"
!insertmacro MUI_LANGUAGE "Portuguese"
!insertmacro MUI_LANGUAGE "Swedish"
!insertmacro MUI_LANGUAGE "Czech"
!insertmacro MUI_LANGUAGE "PortugueseBR"
!insertmacro MUI_LANGUAGE "Polish"
!insertmacro MUI_LANGUAGE "Japanese"
!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "TradChinese"
!insertmacro MUI_LANGUAGE "Korean"
!insertmacro MUI_LANGUAGE "Thai"
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "Turkish"
!insertmacro MUI_LANGUAGE "Greek"
!insertmacro MUI_LANGUAGE "Slovak"
!insertmacro MUI_LANGUAGE "Hungarian"
!insertmacro MUI_LANGUAGE "Norwegian"
!insertmacro MUI_LANGUAGE "Finnish"
!insertmacro MUI_LANGUAGE "Danish"
!insertmacro MUI_LANGUAGE "Arabic"
!insertmacro MUI_LANGUAGE "Hebrew"

!include languageStrings.nsi  

;--------------------------------
;Countries
  
!insertmacro MUI_COUNTRY "Argentina" "Argentina"
!insertmacro MUI_COUNTRY "Australia" "Australia"
!insertmacro MUI_COUNTRY "Austria" "Austria"
!insertmacro MUI_COUNTRY "Belgium" "Belgium"
!insertmacro MUI_COUNTRY "Brazil" "${LANG_PortugueseBr}"
!insertmacro MUI_COUNTRY "Bulgaria" "Bulgaria"
!insertmacro MUI_COUNTRY "Canada" "Canada"
!insertmacro MUI_COUNTRY "Central America" "Central America"
!insertmacro MUI_COUNTRY "Chile" "Chile"
!insertmacro MUI_COUNTRY "China" "${LANG_SIMPCHINESE}"
!insertmacro MUI_COUNTRY "Colombia" "Colombia"
!insertmacro MUI_COUNTRY "Czech Republic" "${LANG_CZECH}"
!insertmacro MUI_COUNTRY "Denmark" "${LANG_DANISH}"
!insertmacro MUI_COUNTRY "Finland" "${LANG_FINNISH}"
!insertmacro MUI_COUNTRY "France" "${LANG_FRENCH}"
!insertmacro MUI_COUNTRY "Germany" "${LANG_GERMAN}"
!insertmacro MUI_COUNTRY "Greece" "${LANG_GREEK}"
!insertmacro MUI_COUNTRY "Hong Kong" "Hong Kong"
!insertmacro MUI_COUNTRY "Hungary" "${LANG_HUNGARIAN}"
!insertmacro MUI_COUNTRY "India" "India"
!insertmacro MUI_COUNTRY "Ireland" "Ireland"
!insertmacro MUI_COUNTRY "Italy" "${LANG_ITALIAN}"
!insertmacro MUI_COUNTRY "Japan" "${LANG_JAPANESE}"
;  !insertmacro MUI_COUNTRY "Korea" "${LANG_KOREAN}"
!insertmacro MUI_COUNTRY "Malaysia" "Malaysia"
!insertmacro MUI_COUNTRY "Mexico" "Mexico"
!insertmacro MUI_COUNTRY "Middle East and Africa" "${LANG_ARABIC}"
!insertmacro MUI_COUNTRY "Netherlands" "${LANG_DUTCH}"
!insertmacro MUI_COUNTRY "New Zealand" "New Zealand"
!insertmacro MUI_COUNTRY "Norway" "${LANG_NORWEGIAN}"
!insertmacro MUI_COUNTRY "Pakistan" "Pakistan"
!insertmacro MUI_COUNTRY "Peru" "Peru"
!insertmacro MUI_COUNTRY "Philippines" "Philippines"
!insertmacro MUI_COUNTRY "Poland" "${LANG_POLISH}"
!insertmacro MUI_COUNTRY "Portugal" "${LANG_PORTUGUESE}"
!insertmacro MUI_COUNTRY "Republic of Korea" "Republic of Korea"
!insertmacro MUI_COUNTRY "Romania" "Romania"
!insertmacro MUI_COUNTRY "Russian Federation" "${LANG_RUSSIAN}"
!insertmacro MUI_COUNTRY "Singapore" "Singapore"
!insertmacro MUI_COUNTRY "Slovakia" "${LANG_SLOVAK}"
!insertmacro MUI_COUNTRY "South Africa" "South Africa"
!insertmacro MUI_COUNTRY "Spain" "${LANG_SPANISH}"
!insertmacro MUI_COUNTRY "Sweden" "${LANG_SWEDISH}"
!insertmacro MUI_COUNTRY "Switzerland" "Switzerland"
!insertmacro MUI_COUNTRY "Taiwan" "${LANG_TRADCHINESE}"
!insertmacro MUI_COUNTRY "Thailand" "${LANG_THAI}"
!insertmacro MUI_COUNTRY "Turkey" "${LANG_TURKISH}"
!insertmacro MUI_COUNTRY "Ukraine" "Ukraine"
!insertmacro MUI_COUNTRY "United Kingdom"  "United Kingdom"
!insertmacro MUI_COUNTRY "United States" "${LANG_ENGLISH}"  
!insertmacro MUI_COUNTRY "Uruguay" "Uruguay"
!insertmacro MUI_COUNTRY "Venezuela" "Venezuela"

!insertmacro MUI_RESERVEFILE_LANGDLL

!insertmacro DirState

!include sb-installer.nsi
!include sb-uninstaller.nsi

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; UAC Handling 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
${UAC.AutoCodeUnload} "1"

