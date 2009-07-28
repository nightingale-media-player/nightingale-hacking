
#
# BEGIN SONGBIRD GPL
# 
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2008 POTI, Inc.
# http://songbirdnest.com
# 
# This file may be licensed under the terms of of the
# GNU General Public License Version 2 (the "GPL").
# 
# Software distributed under the License is distributed 
# on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
# express or implied. See the GPL for the specific language 
# governing rights and limitations.
#
# You should have received a copy of the GPL along with this 
# program. If not, go to http://www.gnu.org/licenses/gpl.html
# or write to the Free Software Foundation, Inc., 
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
# 
# END SONGBIRD GPL
#

;
; We centralize the list of files for the installer and uninstaller so it's
; easier to make sure when files get *added* to the installer, they also get
; added to the uninstaller ('cause the lists are in the same file!)
;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; List of files to be installed
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

!macro InstallFiles
   SetOutPath $INSTDIR
   SetShellVarContext all

   ; List of files to install
   File ${ApplicationIni}
   File ${UpdaterIni}
   File ${FileMainEXE}
   File ${DistHelperEXE}
   File ${AgentEXE}
   !ifdef IncludeLib
      File ${CRuntime}
      File ${CPPRuntime}
   !endif
   File ${MozCRuntime}
   File ${PreferredIcon}
   File ${PreferredInstallerIcon}
   File ${PreferredUninstallerIcon}
   File ${VistaIcon}
  
   ; List of text files to install
   File LICENSE.html
   File TRADEMARK.txt
   File README.txt
   File blocklist.xml
   File songbird.ini
  
   ; List of directories to install
   File /r chrome
   File /r components
   File /r defaults
   File /r extensions
   File /r jsmodules
   File /r plugins
   File /r searchplugins
   File /r scripts
   File /r ${XULRunnerDir}

   # Gstreamer stuff
   File /r lib
   File /r gst-plugins
   File /r gstreamer

   # We only need to do this if we're not using jemalloc...
   !ifndef UsingJemalloc
      ; With VC8, we need the CRT and the manifests all over the place due to 
      ; SxS until BMO 350616 gets fixed
      !ifdef CRuntimeManifest
         SetOutPath $INSTDIR
         File ${CRuntime}
         File ${CPPRuntime}
         File ${CRuntimeManifest}
         SetOutPath $INSTDIR\${XULRunnerDir}
         File ${CRuntime}
         File ${CPPRuntime}
         File ${CRuntimeManifest}
      !endif
   !endif

   SetOutPath $INSTDIR
   WriteUninstaller ${FileUninstallEXE}
!macroend

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; List of files to be uninstalled
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

!macro un.UninstallFiles
   ; List of files to uninstall
   Delete $INSTDIR\${ApplicationIni}
   Delete $INSTDIR\${UpdaterIni}
   Delete $INSTDIR\${FileMainEXE}
   Delete $INSTDIR\${DistHelperEXE}
   Delete $INSTDIR\${AgentEXE}
   !ifndef UsingJemalloc
      Delete $INSTDIR\${CRuntime}
      Delete $INSTDIR\${CPPRuntime}
      Delete $INSTDIR\${CRuntimeManifest}
   !endif
   Delete $INSTDIR\${MozCRuntime}

   Delete $INSTDIR\${PreferredIcon}
   Delete $INSTDIR\${PreferredInstallerIcon}
   Delete $INSTDIR\${PreferredUninstallerIcon}

   Delete $INSTDIR\${VistaIcon}
   ; Log file updater.exe redirects if the PostUpdateWin helper is called
   Delete $INSTDIR\uninstall.update
  
   ; Text files to uninstall
   Delete $INSTDIR\LICENSE.html
   Delete $INSTDIR\TRADEMARK.txt
   Delete $INSTDIR\README.txt
   Delete $INSTDIR\blocklist.xml
   Delete $INSTDIR\songbird.ini
  
   ; These files are created by the application
   Delete $INSTDIR\*.chk
 
   ; Mozilla updates can leave this folder behind when updates fail.
   RMDir /r $INSTDIR\updates

   ; Mozilla updates can leave some of these files over when updates fail.
   Delete $INSTDIR\removed-files
   Delete $INSTDIR\active-update.xml
   Delete $INSTDIR\updates.xml
   
   ; List of directories to remove recursively.
   RMDir /r $INSTDIR\chrome
   RMDir /r $INSTDIR\components
   RMDir /r $INSTDIR\defaults
   RMDir /r $INSTDIR\distribution
   RMDir /r $INSTDIR\extensions
   RMDir /r $INSTDIR\jsmodules
   RMDir /r $INSTDIR\plugins
   RMDir /r $INSTDIR\searchplugins
   RMDir /r $INSTDIR\scripts
   RMDir /r $INSTDIR\lib
   RMDir /r $INSTDIR\gst-plugins
   RMDir /r $INSTDIR\gstreamer
   RMDir /r $INSTDIR\${XULRunnerDir}

   ; Remove uninstaller
   Delete $INSTDIR\${FileUninstallEXE}
 
   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; I commented this out, because I don't think we *truly* want to do this. 
   ;; But we might have to later.
   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;SetShellVarContext current
   ;RMDir /r "$APPDATA\Songbird"
   ;RMDir /r "$LOCALAPPDATA\Songbird"
   ;SetShellVarContext all
 
   Call un.DeleteUpdateAddedFiles
  
   ; Do not attempt to remove this directory recursively; see bug 6367
   RMDir $INSTDIR
!macroend
