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
    cmdline_items: null,
    
    init: function() {
      var cmdline = Components.classes["@songbirdnest.com/commandlinehandler/general-startup;1?type=songbird"];
      if (cmdline) {
        var cmdline_service = cmdline.getService(Components.interfaces.nsICommandLineHandler);
        if (cmdline_service) {
          cmdline_items = cmdline_service.QueryInterface(Components.interfaces.sbICommandLineItems);
          if (cmdline_items)
            cmdline_items.addItemHandler(this);
        }
      }
    },
    
    shutdown: function() {
      if (cmdline_items) cmdline_items.removeItemHandler(this);
    },
    
    handleItem: function(aUriSpec, aCount, aTotal) {
      if (aUriSpec.toLowerCase().indexOf("http:") == 0 ||
          aUriSpec.toLowerCase().indexOf("https:") == 0) {
        // lone> this should open tabs if necessary, rather than override the last url that might have been loaded on the previous handleItem call
        if (!gServicePane.doneInit) {
          // put the requested url into our dataremote
          SBDataSetStringValue("servicetree.selected_url", aUriSpec);
        } else {
          gBrowser.loadURI(aUriSpec);
        }
      } else {
        if (!_drop_filelist) 
          _drop_filelist = Array();
        
        if (isXPI(aUriSpec)) 
          installXPI(aUriSpec);
        else 
          _drop_filelist.push(aUriSpec);
          
        if (aCount == aTotal-1) {
          // if we are already importing, our items will be imported automatically, otherwise, start the drop handler
          if (!importingDrop) {
            firstDrop = true;
            importingDrop = true;
            setTimeout( SBDropped, 10 ); // Next frame
            importingDrop = false;
          }
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
  dump("cmdline.js - " + e + "\n");
}


