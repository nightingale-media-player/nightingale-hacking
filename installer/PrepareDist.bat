@echo off

:prepare
@Tools\unzip\unzip -qq -uo Dependencies\xulrunner\xulrunner-current-win32.zip -d Staging\

@xcopy /q /e /y .\Dependencies\browser-plugins\win32\quicktime\* .\Staging\
@xcopy /q /e /y .\Dependencies\browser-plugins\win32\realplayer\* .\Staging\
@xcopy /q /e /y .\Dependencies\browser-plugins\win32\vlc\plugins .\Staging\plugins\
@xcopy /q /e /y .\Dependencies\browser-plugins\win32\vlc\vlcplugins .\Staging\vlcplugins\
@xcopy /q /e /y .\Dependencies\browser-plugins\win32\vlc\vlcrc .\Staging\

@copy .\Dependencies\sqlite\lib\win32\sqlite.dll .\Staging\
@copy .\Dependencies\runtimelibs\win32\*.* .\Staging\

@copy .\Tools\sqlite\*.* .\Staging\

:end
