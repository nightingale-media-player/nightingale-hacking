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
set ICON_FILE=PublicSVNInstall.ico
set DEPTH=..\..
set DIST_DEPTH=..

if "%2"=="prepare" goto prepare

if "%2"=="package" goto package

:prepare
@del /s /f /q %DEPTH%\_built_installer\*.*
@del /q /f %DEPTH%\dist\Songbird_*.exe
@rd /s /q %DEPTH%\_built_installer

@copy /y Songbird.nsi %DEPTH%\dist\
@copy /y %ICON_FILE% %DEPTH%\dist\

@mkdir %DEPTH%\dist\chrome\icons\default
@copy  %DEPTH%\app\branding\songbird.ico %DEPTH%\dist\chrome\icons\default\frame_outer.ico
@copy  %DEPTH%\app\branding\songbird.ico %DEPTH%\dist\songbird.ico

@copy /y LICENSE.txt %DEPTH%\dist\
@copy /y GPL.txt %DEPTH%\dist\
@copy /y TRADEMARK.txt %DEPTH%\dist\

@cd %DEPTH%\dist
@%DIST_DEPTH%\tools\win32\reshacker\ResHacker.exe -addoverwrite xulrunner\xulrunner.exe, xulrunner\xulrunner.exe, %ICON_FILE%, icongroup, IDI_APPICON, 1033
@%DIST_DEPTH%\tools\win32\reshacker\ResHacker.exe -addoverwrite xulrunner\xulrunner.exe, xulrunner\xulrunner.exe, %ICON_FILE%, icongroup, IDI_DOCUMENT, 1033
@%DIST_DEPTH%\tools\win32\reshacker\ResHacker.exe -addoverwrite xulrunner\xulrunner.exe, xulrunner\xulrunner.exe, %ICON_FILE%, icongroup, 32512, 1033
@%DIST_DEPTH%\tools\win32\reshacker\ResHacker.exe -addoverwrite songbird.exe, songbird.exe, %ICON_FILE%, icongroup, IDI_APPICON, 1033
@%DIST_DEPTH%\tools\win32\reshacker\ResHacker.exe -addoverwrite songbird.exe, songbird.exe, %ICON_FILE%, icongroup, IDI_DOCUMENT, 1033
@cd %DIST_DEPTH%\installer\win32

if "%2"=="prepare" goto end

:package
@cd %DEPTH%\dist
@%DIST_DEPTH%\tools\win32\nsis\makensis /DBUILD_ID="%BUILD_ID%" /O"Songbird_%BUILD_ID%.log" /V4 Songbird.nsi
@cd %DIST_DEPTH%\installer\win32

if exist "%DEPTH%\dist\Songbird_%BUILD_ID%.exe" goto success
goto failure

:success
@echo.
@echo =====================================================
@echo Built the Songbird installer Songbird_%BUILD_ID%.exe. 
@echo A copy of it is in \Songbird\BuiltInstaller.
@echo =====================================================
@echo.

@mkdir %DEPTH%\_built_installer
@copy /y %DEPTH%\dist\Songbird_%BUILD_ID%.exe %DEPTH%\_built_installer
@%DEPTH%\tools\win32\fsum\fsum.exe -d%DEPTH%\_built_installer -md5 -sha1 -jm Songbird_%BUILD_ID%.exe > %DEPTH%\_built_installer\Songbird_%BUILD_ID%.exe.md5

if "%2"=="publish" goto publish
goto nopublish

:publish
REM @call PublishInstaller.bat %BUILD_ID%

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