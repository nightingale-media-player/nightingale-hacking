/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
//
// Software distributed under the License is distributed
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
// END SONGBIRD GPL
//
*/

/**
 * \file  FolderUtils.jsm
 * \brief Javascript source for the folder utility services.
 */

//------------------------------------------------------------------------------
//
// Folder utility JSM configuration.
//
//------------------------------------------------------------------------------

EXPORTED_SYMBOLS = ["FolderUtils"];


//------------------------------------------------------------------------------
//
// Folder utility imported services.
//
//------------------------------------------------------------------------------

Components.utils.import("resource://app/jsmodules/PlatformUtils.jsm");
Components.utils.import("resource://app/jsmodules/SBDataRemoteUtils.jsm");
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");


//------------------------------------------------------------------------------
//
// Folder utility defs.
//
//------------------------------------------------------------------------------

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results

var FolderUtils = {
  get musicFolder() {
    // Get the user home directory.
    var directorySvc = Cc["@mozilla.org/file/directory_service;1"]
                         .getService(Ci.nsIProperties);
    
    var homeDir = null;
    if (directorySvc.has("Home")) {
      homeDir = directorySvc.get("Home", Ci.nsIFile);
    }
    var musicDir = null;
    
    // Get the default scan directory path.
    var defaultScanPath = null;
    var platform = PlatformUtils.platformString;
    
    switch (platform) {
      case "Windows_NT":
        try {
          musicDir = directorySvc.get("Music", Ci.nsIFile);
        }
        catch(e) {
          musicDir = null;
        }

        if(musicDir && musicDir.exists() && musicDir.isReadable()) {
          // Excellent!
          defaultScanPath = musicDir;
        }
        else {
          // Somewhat crappy fallback
          let folder = directorySvc.get("Pers", Ci.nsIFile).clone();
          
          // First, check for "X:\<Path To User Home>\My Documents\My Music".
          folder.append(SBString("folder_paths.music.windows", "My Music"));
          
          if(folder.exists() && folder.isReadable()) {
            defaultScanPath = folder;
          }
          else if (homeDir) {
            //Second, check for "X:\<Path To User Home>\Music".
            
            folder = homeDir.clone();
            folder.append(SBString("folder_paths.music.vista", "Music"));
            
            if(folder.exists() && folder.isReadable()) {
              defaultScanPath = folder;
            }
          }
        }
      break;
      
      case "Darwin":
        try {
          musicDir = directorySvc.get("Music", Ci.nsIFile);
        }
        catch(e) {
          musicDir = null;
        }

        if(musicDir && musicDir.exists() && musicDir.isReadable()) {
          // Excellent!
          defaultScanPath = musicDir;
        }
        else if (homeDir) {
          // Also, somewhat crappy fallback
          let defaultScanDir = homeDir.clone();
          defaultScanDir.append(SBString("folder_paths.music.darwin", "Music"));
          if (defaultScanDir.exists()) {
            defaultScanPath = defaultScanDir;
          }
        }
      break;
      
      case "SunOS":
      case "Linux":
        // Linux / Unix in general has "XDGMusic" as the directory service
        // definitino. Attempt to get the Music folder using that first.
        try {
          musicDir = directorySvc.get("XDGMusic", Ci.nsIFile);
        }
        catch(e) {
          musicDir = null;
        }
        
        if(musicDir && musicDir.exists() && musicDir.isReadable()) {
          // Excellent!
          defaultScanPath = musicDir;
        }
        else if (homeDir) {
          let folder = homeDir.clone();
          let folderName = SBString("folder_paths.music.linux", "Music");
          folder.append(folderName);
          
          if(folder.exists() && folder.isReadable()) {
            defaultScanPath = folder;
          }
          else {
            folder = homeDir.clone();
            folder.append(folderName.toLowerCase());

            if(folder.exists() && folder.isReadable()) {
              defaultScanPath = folder;
            }
          }
        }
      break;

      default:
        defaultScanPath = null;
      break;
    }
    
    return defaultScanPath;    
  },
  
  get videoFolder() {
    // Get the user home directory.
    var directorySvc = Cc["@mozilla.org/file/directory_service;1"]
                         .getService(Ci.nsIProperties);
    
    var homeDir = null;
    if (directorySvc.has("Home")) {
      homeDir = directorySvc.get("Home", Ci.nsIFile);
    }
    var mediaDir = null;
    
    // Get the default scan directory path.
    var defaultScanPath = null;
    var platform = PlatformUtils.platformString;
    
    switch (platform) {
      case "Windows_NT":
        try {
          mediaDir = directorySvc.get("Video", Ci.nsIFile);
        }
        catch(e) {
          mediaDir = null;
        }

        if(mediaDir && mediaDir.exists() && mediaDir.isReadable()) {
          // Excellent!
          defaultScanPath = mediaDir;
        }
        else {
          // Somewhat crappy fallback
          let folder = directorySvc.get("Pers", Ci.nsIFile).clone();
          
          // First, check for "X:\<Path To User Home>\My Documents\My Videos".
          folder.append(SBString("folder_paths.videos.windows", "My Videos"));
          
          if(folder.exists() && folder.isReadable()) {
            defaultScanPath = folder;
          }
          else if (homeDir) {
            //Second, check for "X:\<Path To User Home>\Videos".
            
            folder = homeDir.clone();
            folder.append(SBString("folder_paths.videos.vista", "Videos"));
            
            if(folder.exists() && folder.isReadable()) {
              defaultScanPath = folder;
            }
          }
        }
      break;
      
      case "Darwin":
        try {
          mediaDir = directorySvc.get("Movies", Ci.nsIFile);
        }
        catch(e) {
          mediaDir = null;
        }

        if(mediaDir && mediaDir.exists() && mediaDir.isReadable()) {
          // Excellent!
          defaultScanPath = mediaDir;
        }
        else if (homeDir) {
          // Also, somewhat crappy fallback
          let defaultScanDir = homeDir.clone();
          defaultScanDir.append(SBString("folder_paths.videos.darwin", "Movies"));
          if (defaultScanDir.exists()) {
            defaultScanPath = defaultScanDir;
          }
        }
      break;
      
      case "SunOS":
      case "Linux":
        // Linux / Unix in general has "XDGVids" as the directory service
        // definitino. Attempt to get the videos folder using that first.
        try {
          mediaDir = directorySvc.get("XDGVids", Ci.nsIFile);
        }
        catch(e) {
          mediaDir = null;
        }
        
        if(mediaDir && mediaDir.exists() && mediaDir.isReadable()) {
          // Excellent!
          defaultScanPath = mediaDir;
        }
        else if (homeDir) {
          let folder = homeDir.clone();
          let folderName = SBString("folder_paths.videos.linux", "Videos");
          folder.append(folderName);
          
          if(folder.exists() && folder.isReadable()) {
            defaultScanPath = folder;
          }
          else {
            folder = homeDir.clone();
            folder.append(folderName.toLowerCase());

            if(folder.exists() && folder.isReadable()) {
              defaultScanPath = folder;
            }
          }
        }
      break;

      default:
        defaultScanPath = null;
      break;
    }
    
    return defaultScanPath;    
  },
  
  getMediaFolder: function (aContentType) {
    switch (aContentType) {
      case "audio":                             return this.musicFolder
      case Ci.sbIMediaList.CONTENTTYPE_AUDIO:   return this.musicFolder;
      case "video":                             return this.videoFolder;
      case Ci.sbIMediaList.CONTENTTYPE_VIDEO:   return this.videoFolder;
    }
    
    return null;
  },

  getDownloadFolder: function (aContentType) {
    var downloadFolder = this.getMediaFolder(aContentType);
    
    if(downloadFolder == null) {
      var directorySvc = Cc["@mozilla.org/file/directory_service;1"]
                           .getService(Ci.nsIProperties);
      
      // If the media folder is not available, default to the Desktop:
      try {
        downloadFolder = directorySvc.get("Desk", Ci.nsIFile);
      }
      catch(e) {
        downloadFolder = null;
      }
    }
    
    return downloadFolder;
  }
};
