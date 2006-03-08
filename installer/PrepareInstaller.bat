@echo off

REM Songbird Installer Build Script
REM
REM v1.0
REM
REM Original author Ghislain 'Aus' Lacroix
REM 
REM aus@noiseport.org
REM 

if "%1"=="" goto usage

set BUILD_ID=%1

if "%2"=="prepare" goto checkfordist

if "%2"=="package" goto package

:checkfordist

if exist Dist goto cleanup
goto prepare

:cleanup
@del /f /s /q Dist
@rd /s /q Dist

:prepare
@mkdir .\Dist
@mkdir .\Dist\chrome\

@copy /y .\Staging\songbird.ini .\Dist\application.ini
@xcopy /q /y .\Staging\chrome\chrome.manifest Dist\chrome

@Tools\unzip\unzip -qq -o Dependencies\xulrunner\xulrunner-current-win32.zip -d .\Dist\xulrunner\
@move .\Dist\xulrunner\xulrunner-stub.exe .\Dist\Songbird.exe

REM Can't package these for public builds.
REM @xcopy /q /e /y .\Dependencies\browser-plugins\win32\quicktime\* .\Dist\xulrunner\
REM @xcopy /q /e /y .\Dependencies\browser-plugins\win32\realplayer\* .\Dist\xulrunner\
@xcopy /q /e /y .\Dependencies\browser-plugins\win32\flash .\Dist\xulrunner\plugins\

@xcopy /q /e /y .\Dependencies\browser-plugins\win32\vlc\plugins .\Dist\xulrunner\plugins\
@xcopy /q /e /y .\Dependencies\browser-plugins\win32\vlc\vlcplugins .\Dist\xulrunner\vlcplugins\
@xcopy /q /e /y .\Dependencies\browser-plugins\win32\vlc\vlcrc .\Dist\

REM @xcopy /q /e /y .\Dependencies\runtimelibs\win32\*.dll .\Dist\xulrunner\
REM @xcopy /q /e /y .\Dependencies\runtimelibs\win32\primosdk.dll .\Dist\xulrunner\

@xcopy /q /e /y .\Dependencies\runtimelibs\win32\msvcp71.dll .\Dist
@xcopy /q /e /y .\Dependencies\runtimelibs\win32\msvcp71.dll .\Dist\xulrunner\
@xcopy /q /e /y .\Dependencies\runtimelibs\win32\msvcr71.dll .\Dist\xulrunner\
@xcopy /q /e /y .\Dependencies\runtimelibs\win32\msvcr70.dll .\Dist\xulrunner\

@xcopy /q /e /y .\Staging\chrome\browser .\Dist\chrome\browser\
@xcopy /q /e /y .\Staging\chrome\content .\Dist\chrome\content\
@xcopy /q /e /y .\Staging\chrome\locale .\Dist\chrome\locale\
@xcopy /q /e /y .\Staging\chrome\skin .\Dist\chrome\skin\
@xcopy /q /e /y .\Staging\chrome\skin2 .\Dist\chrome\skin2\

@xcopy /q /e /y .\Staging\defaults\preferences\* .\Dist\defaults\preferences\

REM Hmmm, after we unzip, copy over our own files.
@xcopy /q /e /y .\Components\MediaLibrary\Release\sbMediaLibrary.xpt .\Dist\components\
@xcopy /q /e /y .\Components\MediaLibrary\Release\sbMediaLibrary.dll .\Dist\components\
@xcopy /q /e /y .\Components\MediaLibrary\sbI*.js .\Dist\components\
@xcopy /q /e /y .\Dependencies\sqlite\lib\win32\sqlite.dll .\Dist\xulrunner\

@xcopy /q /y .\Components\Playlists\PlaylistReaderPLS\Release\sbPlaylistReaderPLS.xpt .\Dist\components\
@xcopy /q /y .\Components\Playlists\PlaylistReaderPLS\Release\sbPlaylistReaderPLS.dll .\Dist\components\

@xcopy /q /y .\Components\Playlistsource\sbIPlaylistsource.xpt .\Dist\components\
@xcopy /q /y .\Components\Playlistsource\Release\sbPlaylistsource.dll .\Dist\components\

@xcopy /q /y .\Components\Servicesource\sbIServicesource.xpt .\Dist\components\
@xcopy /q /y .\Components\Servicesource\Release\sbServicesource.dll .\Dist\components\

@xcopy /q /y .\Components\Integration\IWindowDragger.xpt .\Dist\components\
@xcopy /q /y .\Components\Integration\Release\sbIntegration.dll .\Dist\components\

@xcopy /q /y .\Components\Devices\sbIDeviceBase\sbIDeviceBase.xpt .\Dist\components\

@xcopy /q /y .\Components\Devices\sbIDeviceManager\sbIDeviceManager.xpt .\Dist\components\
@xcopy /q /y .\Components\Devices\sbIDeviceManager\Release\sbDeviceManager.dll .\Dist\components\

@xcopy /q /y .\Components\Devices\sbIDownloadDevice\sbIDownloadDevice.xpt .\Dist\components\
@xcopy /q /y .\Components\Devices\sbIDownloadDevice\Release\sbDownloadDevice.dll .\Dist\components\

@xcopy /q /y .\Components\Integration\Release\sbIntegration.dll .\Dist\components\
@xcopy /q /y .\Components\Integration\Release\sbIntegration.xpt .\Dist\components\

@xcopy /q /y .\Installer\Songbird.nsi .\Dist\
@xcopy /q /y .\Installer\SongbirdInstall.ico .\Dist\

@mkdir .\Dist\chrome\icons\default
@copy  .\Staging\chrome\skin\service_icons\Songbird.ico .\Dist\chrome\icons\default\frame_outer.ico

@xcopy /q /y .\Installer\License.txt .\Dist\
@xcopy /q /y .\Installer\GPL.txt .\Dist\
@xcopy /q /y .\Installer\TRADEMARK.txt .\Dist\

@.\Tools\ResHack\ResHacker.exe -addoverwrite Dist\xulrunner\xulrunner.exe, Dist\xulrunner\xulrunner.exe, Staging\chrome\skin\service_icons\Songbird.ico, icongroup, IDI_APPICON, 1033
@.\Tools\ResHack\ResHacker.exe -addoverwrite Dist\xulrunner\xulrunner.exe, Dist\xulrunner\xulrunner.exe, Staging\chrome\skin\service_icons\Songbird.ico, icongroup, IDI_DOCUMENT, 1033
@.\Tools\ResHack\ResHacker.exe -addoverwrite Dist\xulrunner\xulrunner.exe, Dist\xulrunner\xulrunner.exe, Staging\chrome\skin\service_icons\Songbird.ico, icongroup, 32512, 1033
@.\Tools\ResHack\ResHacker.exe -addoverwrite Dist\Songbird.exe, Dist\Songbird.exe, Staging\chrome\skin\service_icons\Songbird.ico, icongroup, IDI_APPICON, 1033
@.\Tools\ResHack\ResHacker.exe -addoverwrite Dist\Songbird.exe, Dist\Songbird.exe, Staging\chrome\skin\service_icons\Songbird.ico, icongroup, IDI_DOCUMENT, 1033

if "%2"=="prepare" goto end

:package
@cd Dist
@..\Tools\nsis\makensis /DBUILD_ID="%BUILD_ID%" /O"Songbird_%BUILD_ID%.log" /V4 Songbird.nsi
@cd ..

if exist "Dist\Songbird_%BUILD_ID%.exe" goto success
goto failure

:success
@echo.
@echo =====================================================
@echo Built the Songbird installer Songbird_%BUILD_ID%.exe. 
@echo A copy of it is in \Songbird\BuiltInstaller.
@echo =====================================================
@echo.

@mkdir .\BuiltInstaller
@xcopy /q /y .\Dist\Songbird_%BUILD_ID%.exe .\BuiltInstaller
@.\Tools\fsum\fsum.exe -dBuiltInstaller -md5 -sha1 -jm Songbird_%BUILD_ID%.exe > .\BuiltInstaller\Songbird_%BUILD_ID%.exe.md5

if "%2"=="publish" goto publish
goto nopublish

:publish
@call PublishInstaller.bat %BUILD_ID%

:nopublish
goto end

:failure
@echo.
@echo ==================================================================
@echo Failed to build the Songbird installer. Please check the 
@echo log for further information. Log created: Songbird_%BUILD_ID%.log.
@echo ==================================================================

goto end

:usage
@echo "Usage: PrepareInstaller [build id] [prepare|package|publish]"
@echo.
@echo Note: Using the "publish" flag will publish the 
@echo installer using the "PublishInstaller" script.
@echo.

:end