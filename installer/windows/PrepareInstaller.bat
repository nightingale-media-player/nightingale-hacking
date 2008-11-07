@echo off

REM Songbird Installer Build Script
REM
REM v1.1
REM
REM Original author Ghislain 'Aus' Lacroix
REM 
REM aus@noiseport.org
REM 

if "%1"=="" goto usage

set BUILD_NUMBER=%1
set DATESTAMP=%2
set ARCH=%3
set DEPTH=..\..
set DIST_DEPTH=..\..
set OBJ_DIR=%DEPTH%\compiled
set DIST_DIR=%OBJ_DIR%\dist

if exist "%DIST_DIR%\songbird.ico" set ICON_FILE=songbird.ico
if exist "%DIST_DIR%\songbird_nightly.ico" set ICON_FILE=songbird_nightly.ico

set PRINTCONF="%DEPTH%\dependencies\%ARCH%\mozilla\release\scripts\printconfigsetting.py"
set BUILDINFO=%OBJ_DIR%\build\sbBuildInfo.ini
for /f %%A in ('python %PRINTCONF% %BUILDINFO% Build AppName') do set BRANDSHORTNAME=%%A
for /f %%A in ('python %PRINTCONF% %BUILDINFO% Build Milestone') do set APPVERSION=%%A
for /f %%A in ('python %PRINTCONF% %BUILDINFO% Build BuildNumber') do set APPBUILDNUMBER=%%A

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
%DIST_DEPTH%\tools\win32\nsis\makensis /NOCD /DBUILD_ID="%DATESTAMP%" /DARCH="%ARCH%" /V4 ../installer/windows/songbird.nsi
cd %DIST_DEPTH%\installer\windows

if exist "%DIST_DIR%\%BRANDSHORTNAME%_%APPVERSION%-%BUILD_NUMBER%_%ARCH%.exe" goto success
goto failure

:success
@echo.
@echo =====================================================
@echo Built the Songbird installer %BRANDSHORTNAME%_%APPVERSION%-%BUILD_NUMBER%_%ARCH%.exe.
@echo A copy of it is in compiled\_built_installer\
@echo =====================================================
@echo.

@mkdir %DEPTH%\compiled\_built_installer
move /y %DIST_DIR%\%BRANDSHORTNAME%_%APPVERSION%-%BUILD_NUMBER%_%ARCH%.exe %DEPTH%\compiled\_built_installer\
%DEPTH%\tools\win32\fsum\fsum.exe -d%DEPTH%\compiled\_built_installer -md5 -sha1 -jm ^
  %BRANDSHORTNAME%_%APPVERSION%-%BUILD_NUMBER%_%ARCH%.exe ^
  > %DEPTH%\compiled\_built_installer\%BRANDSHORTNAME%_%APPVERSION%-%BUILD_NUMBER%_%ARCH%.exe.md5

goto end

:failure
@echo.
@echo ==================================================================
@echo Failed to build the Songbird installer. Errors are above.
@echo ==================================================================

goto end

:usage
@echo "Usage: PrepareInstaller [build number] [datestamp] [prepare|package]"
@echo.
@echo.

:end
