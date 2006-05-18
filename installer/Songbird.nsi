;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;  Songbird Installer for Windows
;;
;;  v1.0
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SetCompressor /SOLID lzma
SetCompressorDictSize 64

Name "Songbird 0.1 (Win32)"
Caption "Songbird 0.1 (Win32)"

ComponentText "You may customize the Songbird installation below. Click next when you're ready to continue the installation process." \
"" \
"Songbird Installation Options:"

CompletedText ""

DirText "The installation will install the Songbird 0.1 (Win32) in the folder indicated below. You may change it if you wish to install to a different folder." \
"Current Destination Folder" \
"Browse ..." \
"Choose a folder for Songbird ..."

SpaceTexts "Required Disk Space: " \
"Available Disk Space: "

InstallColors /windows

Icon "SongbirdInstall.ico"
UninstallIcon "SongbirdInstall.ico"

XPStyle on

OutFile "Songbird_${BUILD_ID}.exe"
InstallDir "$PROGRAMFILES\Songbird\"
;InstallDirRegKey HKLM SOFTWARE\Songbird "Install_Dir"

LicenseData LICENSE.txt

Page license
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

Section "Songbird Base (Required)"
  SectionIn RO
  SetOutPath $INSTDIR

  ReadRegStr $R1 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Songbird" "UninstallString"
  StrCmp $R1 "" NoUninstall
  
  ClearErrors
  ExecWait '$R1 /S _?=$INSTDIR'
  
NoUninstall:
  
  File *.ini
  File *.exe
  File *.ico
  File LICENSE.txt
  File GPL.txt
  File TRADEMARK.txt
  
  File /r chrome
  File /r components
  File /r defaults
  File /r plugins
  File /r vlcplugins
  File /r xulrunner
  
  FindFirst $0 $1 $SYSDIR\msvcp71.dll
  StrCmp $1 "" RequiresMSVCP71
  Goto NoRequireMSVCP71

RequiresMSVCP71:
  SetOutPath $SYSDIR
  File xulrunner\msvcp71.dll

NoRequireMSVCP71:
  FindClose $0
  
  ;VLC Configuration File.
  SetOutPath $APPDATA\Songbird_vlc
  File vlcrc
  
  ;Back to normal install directory.
  SetOutPath $INSTDIR
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\Songbird "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Songbird" "DisplayName" "Songbird 0.1 (Win32)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Songbird" "UninstallString" '"$INSTDIR\songbird-uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Songbird" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Songbird" "NoRepair" 1
  WriteUninstaller "songbird-uninstall.exe"

SectionEnd

Section "Start Menu Entries"
  CreateDirectory "$SMPROGRAMS\Songbird"
  CreateShortCut "$SMPROGRAMS\Songbird\Songbird.lnk" "$INSTDIR\songbird.exe" "" "$INSTDIR\songbird.ico" 0
  CreateShortCut "$SMPROGRAMS\Songbird\Uninstall Songbird.lnk" "$INSTDIR\songbird-uninstall.exe" "" "$INSTDIR\songbird-uninstall.exe" 0
SectionEnd

Section "Desktop Icon"
  CreateShortCut "$DESKTOP\Songbird.lnk" "$INSTDIR\songbird.exe" "" "$INSTDIR\Songbird.ico" 0
SectionEnd

Section "QuickLaunch Icon"
  CreateShortCut "$QUICKLAUNCH\Songbird.lnk" "$INSTDIR\songbird.exe" "" "$INSTDIR\Songbird.ico" 0
SectionEnd

Section "Uninstall"
  
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
  
  ;I commented this out, because I don't think we *truly* want to do this.
  ;But we might have to later.
  
  Delete $APPDATA\Songbird_vlc\vlcrc
  
  ;RMDir /r "$APPDATA\Pioneers of the Inevitable\Songbird"
  
  RMDir /r $INSTDIR\chrome
  RMDir /r $INSTDIR\components
  RMDir /r $INSTDIR\defaults
  RMDir /r $INSTDIR\plugins
  RMDir /r $INSTDIR\vlcplugins
  RMDir /r $INSTDIR\xulrunner

  RMDir "$SMPROGRAMS\Songbird"
  RMDir "$INSTDIR"

SectionEnd