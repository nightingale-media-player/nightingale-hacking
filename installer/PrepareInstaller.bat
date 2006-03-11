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

if "%2"=="prepare" goto prepare

if "%2"=="package" goto package

:prepare

@copy /y Songbird.nsi ..\dist\
@copy /y SongbirdInstall.ico ..\dist\

@mkdir ..\dist\chrome\icons\default
@copy  ..\app\icons\songbird.ico ..\dist\chrome\icons\default\frame_outer.ico
@copy  ..\app\icons\songbird.ico ..\dist\songbird.ico

@copy /y LICENSE.txt ..\dist\
@copy /y GPL.txt ..\dist\
@copy /y TRADEMARK.txt ..\dist\

@cd ..\dist
@..\tools\win32\reshacker\ResHacker.exe -addoverwrite xulrunner\xulrunner.exe, xulrunner\xulrunner.exe, songbird.ico, icongroup, IDI_APPICON, 1033
@..\tools\win32\reshacker\ResHacker.exe -addoverwrite xulrunner\xulrunner.exe, xulrunner\xulrunner.exe, songbird.ico, icongroup, IDI_DOCUMENT, 1033
@..\tools\win32\reshacker\ResHacker.exe -addoverwrite xulrunner\xulrunner.exe, xulrunner\xulrunner.exe, songbird.ico, icongroup, 32512, 1033
@..\tools\win32\reshacker\ResHacker.exe -addoverwrite songbird.exe, songbird.exe, songbird.ico, icongroup, IDI_APPICON, 1033
@..\tools\win32\reshacker\ResHacker.exe -addoverwrite songbird.exe, songbird.exe, songbird.ico, icongroup, IDI_DOCUMENT, 1033
@cd ..\installer

if "%2"=="prepare" goto end

:package
@cd ..\dist
@..\tools\win32\nsis\makensis /DBUILD_ID="%BUILD_ID%" /O"Songbird_%BUILD_ID%.log" /V4 Songbird.nsi
@cd ..\installer

if exist "..\dist\Songbird_%BUILD_ID%.exe" goto success
goto failure

:success
@echo.
@echo =====================================================
@echo Built the Songbird installer Songbird_%BUILD_ID%.exe. 
@echo A copy of it is in \Songbird\BuiltInstaller.
@echo =====================================================
@echo.

@mkdir ..\_built_installer
@copy /y ..\dist\Songbird_%BUILD_ID%.exe ..\_built_installer
@..\tools\win32\fsum\fsum.exe -d..\_built_installer -md5 -sha1 -jm Songbird_%BUILD_ID%.exe > ..\_built_installer\Songbird_%BUILD_ID%.exe.md5

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