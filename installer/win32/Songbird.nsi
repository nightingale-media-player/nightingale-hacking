;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;  Songbird Installer for Windows
;;
;;  v1.0
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SetCompressor /SOLID lzma
SetCompressorDictSize 64

!define RELEASE_NAME "Songbird 'Test Flight' 0.2 (Win32)"
Name "${RELEASE_NAME}"
Caption "${RELEASE_NAME}"

ComponentText "You may customize the Songbird installation below. Click next when you're ready to continue the installation process." \
"" \
"Songbird Installation Options:"

CompletedText ""

DirText "The installation will install the Songbird ${RELEASE_NAME} in the folder indicated below. You may change it if you wish to install to a different folder." \
"Current Destination Folder" \
"Browse ..." \
"Choose a folder for Songbird ..."

SpaceTexts "Required Disk Space: " \
"Available Disk Space: "

InstallColors /windows

Icon "songbird.ico"
UninstallIcon "songbird.ico"

XPStyle on

OutFile "Songbird_${BUILD_ID}_${ARCH}.exe"
InstallDir "$PROGRAMFILES\Songbird\"
;InstallDirRegKey HKLM SOFTWARE\Songbird "Install_Dir"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; License / EULA is displayed on first run.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Page components
Page directory
Page instfiles

UninstPage uninstconfirm
UninstPage instfiles

ShowInstDetails show
ShowUninstDetails show

Function .onInstSuccess
  WriteINIStr $APPDATA\Songbird_vlc\vlcrc main plugin-path $INSTDIR\vlcplugins
FunctionEnd

var LinkIconFile
var Result
var MajorVersion
var MinorVersion
var BuildNumber
var PlatformID
var CSDVersion 
Function .onInit
  ; Figure out if we should give the old or the new icon in our links
  ; XP or 2k3 use new icon, everyone else uses the old.
  
  ; Test for XP
  Version::IsWindowsXP
  Pop $Result
  StrCmp $Result "1" UseNewIcon
  
  ; Test for 2k3
  Version::IsWindows2003
  Pop $Result
  StrCmp $Result "1" UseNewIcon
  
  ; Test for Vista (?)
  Version::GetWindowsVersion
  ; get function result
  Pop $MajorVersion
  Pop $MinorVersion
  Pop $BuildNumber
  Pop $PlatformID
  Pop $CSDVersion  
  StrCmp $MajorVersion "6" UseNewIcon

  UseOldIcon:
    ; Fallthrough to use the old 32x32x8 icon
    Strcpy $LinkIconFile 'songbird32x32x8.ico'
    Goto Done
    
  UseNewIcon:
    ; Use the new 128x128x32 icon
    Strcpy $LinkIconFile 'songbird.ico'
    Goto Done
    
  Done:
FunctionEnd

Section "Songbird Base (Required)"
  SetShellVarContext all
  
  SectionIn RO
  SetOutPath $INSTDIR

  ReadRegStr $R1 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Songbird" "UninstallString"
  StrCmp $R1 "" NoUninstall
  
  ClearErrors
  ExecWait '$R1 /S _?=$INSTDIR'
  
NoUninstall:

  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;; Force creation of new profile / cache for Songbird
  ;; for nightly / developer builds.
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;;SetShellVarContext current
  ;;RMDir /r "$APPDATA\Pioneers of the Inevitable\"
  ;;RMDir /r "$LOCALAPPDATA\Pioneers of the Inevitable\"
  ;;SetShellVarContext all
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  
  File *.ini
  File *.exe
  File *.dll
  File *.ico
  File LICENSE.txt
  File GPL.txt
  File TRADEMARK.txt
  
  File /r chrome
  File /r components
  File /r defaults
  File /r plugins
  File /r scripts
  File /r xulrunner
  
  FindFirst $0 $1 $SYSDIR\msvcp71.dll
  StrCmp $1 "" RequiresMSVCP71
  Goto NoRequireMSVCP71

RequiresMSVCP71:
  SetOutPath $SYSDIR
  File msvcp71.dll

NoRequireMSVCP71:
  FindClose $0
  
  ;VLC Configuration File.
  SetOutPath $APPDATA\SongbirdVLC
  File songbirdvlcrc
  
  ;Back to normal install directory.
  SetOutPath $INSTDIR
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\Songbird "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Songbird" "DisplayName" "${RELEASE_NAME}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Songbird" "UninstallString" '"$INSTDIR\songbird-uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Songbird" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Songbird" "NoRepair" 1
  WriteUninstaller "songbird-uninstall.exe"

SectionEnd

Section "Start Menu Entries"
  CreateDirectory "$SMPROGRAMS\Songbird"
  CreateShortCut "$SMPROGRAMS\Songbird\Songbird.lnk" "$INSTDIR\songbird.exe" "" "$INSTDIR\$LinkIconFile" 0
  CreateShortCut "$SMPROGRAMS\Songbird\Uninstall Songbird.lnk" "$INSTDIR\songbird-uninstall.exe" "" "$INSTDIR\$LinkIconFile" 0
SectionEnd

Section "Desktop Icon"
  CreateShortCut "$DESKTOP\Songbird.lnk" "$INSTDIR\songbird.exe" "" "$INSTDIR\$LinkIconFile" 0
SectionEnd

Section "QuickLaunch Icon"
  CreateShortCut "$QUICKLAUNCH\Songbird.lnk" "$INSTDIR\songbird.exe" "" "$INSTDIR\$LinkIconFile" 0
SectionEnd

Section "Uninstall"
  SetShellVarContext all
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Songbird"
  DeleteRegKey HKLM SOFTWARE\Songbird

  ; Remove files and uninstaller
  Delete $INSTDIR\songbird-uninstall.exe

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\Songbird\*.*"
  Delete "$DESKTOP\Songbird.lnk"
  Delete "$QUICKLAUNCH\Songbird.lnk"

  ; Remove directories used
  Delete $INSTDIR\LICENSE.txt
  Delete $INSTDIR\GPL.txt
  Delete $INSTDIR\TRADEMARK.txt
  
  Delete $INSTDIR\*.chk
  Delete $INSTDIR\*.ini
  Delete $INSTDIR\*.dll
  Delete $INSTDIR\*.exe
  Delete $INSTDIR\*.ico

  Delete $APPDATA\SongbirdVLC\songbirdvlcrc
  
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;; I commented this out, because I don't think we *truly* want to do this. 
  ;; But we might have to later.
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;SetShellVarContext current
  ;RMDir /r "$APPDATA\Songbird"
  ;RMDir /r "$LOCALAPPDATA\Songbird"
  ;SetShellVarContext all

  
  RMDir /r $INSTDIR\chrome
  RMDir /r $INSTDIR\components
  RMDir /r $INSTDIR\defaults
  RMDir /r $INSTDIR\plugins
  RMDir /r $INSTDIR\scripts
  RMDir /r $INSTDIR\xulrunner

  RMDir "$SMPROGRAMS\Songbird"
  RMDir "$INSTDIR"

SectionEnd

