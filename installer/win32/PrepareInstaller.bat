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

if "%2"=="vs" goto visualstudio

REM Assume Cygwin here...
set BUILD_ID=%1
set ICON_FILE=songbird.ico
set DEPTH=..\..
set DIST_DEPTH=..\..
set DIST_DIR=%DEPTH%\compiled\dist

:visualstudio

if "%3"=="prepare" goto prepare

if "%3"=="package" goto package

:prepare
@del /s /f /q %DEPTH%\_built_installer\*.*
@rd /s /q %DEPTH%\_built_installer

@copy /y Songbird.nsi %DIST_DIR%

@mkdir %DIST_DIR%\chrome\icons\default
@copy /y %DEPTH%\app\branding\songbird.ico %DIST_DIR%chrome\icons\default\frame_outer.ico
@copy /y %DEPTH%\app\branding\songbird*.ico %DIST_DIR%

@copy /y LICENSE.txt %DIST_DIR%
@copy /y GPL.txt %DIST_DIR%
@copy /y TRADEMARK.txt %DIST_DIR%

@cd %DIST_DIR%
@%DIST_DEPTH%\tools\win32\reshacker\ResHacker.exe -addoverwrite xulrunner\xulrunner.exe, xulrunner\xulrunner.exe, %ICON_FILE%, icongroup, IDI_APPICON, 1033
@%DIST_DEPTH%\tools\win32\reshacker\ResHacker.exe -addoverwrite xulrunner\xulrunner.exe, xulrunner\xulrunner.exe, %ICON_FILE%, icongroup, IDI_DOCUMENT, 1033
@%DIST_DEPTH%\tools\win32\reshacker\ResHacker.exe -addoverwrite xulrunner\xulrunner.exe, xulrunner\xulrunner.exe, %ICON_FILE%, icongroup, 32512, 1033
@%DIST_DEPTH%\tools\win32\reshacker\ResHacker.exe -addoverwrite songbird.exe, songbird.exe, %ICON_FILE%, icongroup, IDI_APPICON, 1033
@%DIST_DEPTH%\tools\win32\reshacker\ResHacker.exe -addoverwrite songbird.exe, songbird.exe, %ICON_FILE%, icongroup, IDI_DOCUMENT, 1033
@cd %DIST_DEPTH%\installer\win32

if "%3"=="prepare" goto end

:package
@cd %DIST_DIR%
@%DIST_DEPTH%\tools\win32\nsis\makensis /DBUILD_ID="%BUILD_ID%" /O"%DEPTH%\Songbird_%BUILD_ID%.log" /V4 Songbird.nsi
@del Songbird.nsi
@cd %DIST_DEPTH%\installer\win32

if exist "%DIST_DIR%\Songbird_%BUILD_ID%.exe" goto success
goto failure

:success
@echo.
@echo =====================================================
@echo Built the Songbird installer Songbird_%BUILD_ID%.exe. 
@echo A copy of it is in \Songbird\BuiltInstaller.
@echo =====================================================
@echo.

@mkdir %DEPTH%\_built_installer
@move /y %DIST_DIR%\Songbird_%BUILD_ID%.exe %DEPTH%\_built_installer
@%DEPTH%\tools\win32\fsum\fsum.exe -d%DEPTH%\_built_installer -md5 -sha1 -jm Songbird_%BUILD_ID%.exe > %DEPTH%\_built_installer\Songbird_%BUILD_ID%.exe.md5

goto end

:failure
@echo.
@echo ==================================================================
@echo Failed to build the Songbird installer. Please check the 
@echo log for further information. Log created: Songbird_%BUILD_ID%.log.
@echo ==================================================================

goto end

:usage
@echo "Usage: PrepareInstaller [build id] [vs|cygwin] [prepare|package]"
@echo vs: Using output from a Visual Studio Build.
@echo cygwin: Using output from a Cygwin Build.
@echo.
@echo.

:end