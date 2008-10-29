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

set ARCH=%3
set BUILD_ID=%1
set DEPTH=..\..
set DIST_DEPTH=..\..
set DIST_DIR=%DEPTH%\compiled\dist

if exist "%DIST_DIR%\songbird.ico" set ICON_FILE=songbird.ico
if exist "%DIST_DIR%\songbird_nightly.ico" set ICON_FILE=songbird_nightly.ico

goto prepareorpackage

:visualstudio

set ARCH=%3
set BUILD_ID=%1
set ICON_FILE=songbirdinstall.ico
set DEPTH=..\..
set DIST_DEPTH=..
set DIST_DIR=%DEPTH%\dist

goto prepareorpackage

:prepareorpackage

if "%4"=="prepare" goto prepare
if "%4"=="package" goto package

:prepare
del /s /f /q %DEPTH%\compiled\_built_installer\*.*
rd /s /q %DEPTH%\compiled\_built_installer

cd %DIST_DIR%
%DIST_DEPTH%\tools\win32\unix2dos\unix2dos.exe README.txt
%DIST_DEPTH%\tools\win32\unix2dos\unix2dos.exe GPL.txt
%DIST_DEPTH%\tools\win32\unix2dos\unix2dos.exe LICENSE.txt
%DIST_DEPTH%\tools\win32\unix2dos\unix2dos.exe TRADEMARK.txt

cd %DIST_DEPTH%\installer\windows

if "%4"=="prepare" goto end

:package
cd %DIST_DIR%
%DIST_DEPTH%\tools\win32\nsis\makensis /NOCD /DBUILD_ID="%BUILD_ID%" /DARCH="%ARCH%" /V4 ../installer/windows/songbird.nsi
cd %DIST_DEPTH%\installer\windows

if exist "%DIST_DIR%\Songbird_*_%ARCH%.exe" goto success
goto failure

:success
@echo.
@echo =====================================================
@echo Built the Songbird installer Songbird_%BUILD_ID%.exe. 
@echo A copy of it is in \Songbird\BuiltInstaller.
@echo =====================================================
@echo.

@mkdir %DEPTH%\compiled\_built_installer
@move /y %DIST_DIR%\Songbird_*_%ARCH%.exe %DEPTH%\compiled\_built_installer
@%DEPTH%\tools\win32\fsum\fsum.exe -d%DEPTH%\compiled\_built_installer -md5 -sha1 -jm Songbird_%BUILD_ID%_%ARCH%.exe > %DEPTH%\compiled\_built_installer\Songbird_%BUILD_ID%_%ARCH%.exe.md5

goto end

:failure
@echo.
@echo ==================================================================
@echo Failed to build the Songbird installer. Errors are above.
@echo ==================================================================

goto end

:usage
@echo "Usage: PrepareInstaller [build id] [vs|cygwin] [prepare|package]"
@echo vs: Using output from a Visual Studio Build.
@echo cygwin: Using output from a Cygwin Build.
@echo.
@echo.

:end
