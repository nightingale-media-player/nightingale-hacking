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
 * \file commandLine.js
 * \brief Command Line listener object implementation.
 * \internal
 */

if (typeof(ExternalDropHandler) == "undefined")
  Components.utils.import("resource://app/jsmodules/DropHelper.jsm");

try 
{

  // Module specific global for auto-init/deinit support
  var commandline_module = {};
  commandline_module.init_once = 0;
  commandline_module.deinit_once = 0;

  commandline_module.onLoad = function()
  {
    if (commandline_module.init_once++) { dump("WARNING: commandline_module double init!!\n"); return; }
    commandLineItemHandler.init();
  }

  commandline_module.onUnload = function()
  {
    if (commandline_module.deinit_once++) { dump("WARNING: commandline_module double deinit!!\n"); return; }
    commandLineItemHandler.shutdown();
    window.removeEventListener("load", commandline_module.onLoad, false);
    window.removeEventListener("unload", commandline_module.onUnload, false);
  }

  var commandLineItemHandler = {
    cmdline_mgr: null,

    init: function() {
      var cmdline = Components.classes["@songbirdnest.com/commandlinehandler/general-startup;1?type=songbird"];
      if (cmdline) {
        var cmdline_service = cmdline.getService(Components.interfaces.nsICommandLineHandler);
        if (cmdline_service) {
          this.cmdline_mgr = cmdline_service.QueryInterface(Components.interfaces.sbICommandLineManager);
          this.cmdline_mgr.addItemHandler(this);
        }
      }
    },
    
    shutdown: function() {
      if (this.cmdline_mgr) this.cmdline_mgr.removeItemHandler(this);
    },
    
    handleItem: function(aUriSpec, aCount, aTotal) {
      if (aUriSpec.toLowerCase().indexOf("http:") == 0 ||
          aUriSpec.toLowerCase().indexOf("https:") == 0 ||
          aUriSpec.toLowerCase().indexOf("ngale:") == 0)
      {
        if (gBrowser._sessionStore && !gBrowser._sessionStore.tabStateRestored) {
          // process this after the session store is complete
          // TODO: does the session store go away when it's done? be sure this doesn't break already-running behavior.
          gBrowser.addEventListener("sessionstore-tabs-restored", function() {
            var newTab = gBrowser.loadOneTab(aUriSpec, null, null, null, false, false);
            gBrowser.selectedTab = newTab;
            gBrowser.removeEventListener("sessionstore-tabs-restored", arguments.callee, false);
          }, false);
        }
        else {
          var newTab = gBrowser.loadOneTab(aUriSpec, null, null, null, false, false);
          gBrowser.selectedTab = newTab;
        }  
      } else {
        var typeSniffer = Components.classes["@songbirdnest.com/Songbird/Mediacore/TypeSniffer;1"]
                                    .createInstance(Components.interfaces.sbIMediacoreTypeSniffer);
        var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                                  .getService(Components.interfaces.nsIIOService);
        var aUri = ioService.newURI(aUriSpec, null, null);
        if (typeSniffer.isValidPlaylistURL(aUri)) {
          var list = SBOpenPlaylistURI(aUriSpec);
          if (list) {
            var view = LibraryUtils.createStandardMediaListView(list);
            var mm = Components.classes["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
                               .getService(Components.interfaces.sbIMediacoreManager);
            mm.sequencer.playView(view, 0);
          }
        } else {
          var dropHandlerListener = {
            onDropComplete: function(aTargetList,
                                     aImportedInLibrary,
                                     aDuplicates,
                                     aInsertedInMediaList,
                                     aOtherDropsHandled) {
              // show the standard report on the status bar
              return true;
            },
            onFirstMediaItem: function(aTargetList, aFirstMediaItem) {
              var view = LibraryUtils.createStandardMediaListView(LibraryUtils.mainLibrary);

              var index = view.getIndexForItem(aFirstMediaItem);
              
              // If we have a browser, try to show the view
              if (window.gBrowser) {
                gBrowser.showIndexInView(view, index);
              }
              
              // Play the item
              var mm = Components.classes["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
                                 .getService(Components.interfaces.sbIMediacoreManager);
              mm.sequencer.playView(view, index);
            },
          };
          ExternalDropHandler.dropUrls(window, [aUriSpec], dropHandlerListener);
        }
      }
      return true;
    },
    
    QueryInterface : function(aIID) {
      if (!aIID.equals(Components.interfaces.sbICommandLineItemHandler) &&
          !aIID.equals(Components.interfaces.nsISupports)) 
      {
        throw Components.results.NS_ERROR_NO_INTERFACE;
      }
      return this;
    }
  };

  // Auto-init/deinit registration
  window.addEventListener("load", commandline_module.onLoad, false);
  window.addEventListener("unload", commandline_module.onUnload, false);
}
catch(e)
{
  dump("commandLine.js - " + e + "\n");
}


