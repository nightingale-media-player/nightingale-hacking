@echo off

REM Songbird Installer Publish Script
REM
REM v1.0
REM
REM Original author Ghislain 'Aus' Lacroix
REM 
REM aus@noiseport.org
REM 

if "%1"=="" goto usage


set INSTALLER=Songbird_%1.exe
set INSTALLER_DIR=BuiltInstaller

set HOSTNAME=pi-hq.com
set USER=u38731495
set PASSWORD=G8m4ctE7
set PLATFORM=win32

set REMOTE_DIR=www.songbirdnest.com.net.org/corp/download

@Tools\scp\pscp -pw %PASSWORD% %INSTALLER_DIR%\%INSTALLER% %USER%@%HOSTNAME%:%REMOTE_DIR%/%PLATFORM%
goto end

:usage
@echo Usage: PublishInstaller [build id]
@echo.

:end
