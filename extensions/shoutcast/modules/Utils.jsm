var EXPORTED_SYMBOLS = [ "Utils" ];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const SP_CONTRACTID = "@songbirdnest.com/servicepane/library;1";

Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");
var Application = Cc["@mozilla.org/fuel/application;1"]
                    .getService(Ci.fuelIApplication);

/**
 * Utility functions to be used by multiple windows
 */
Utils = {
  LS_NS: "http://songbirdnest.com/rdf/library-servicepane#",
  SC_NS: "http://songbirdnest.com/rdf/shoutcast-servicepane#",

  ensureFavouritesNode: function() {
    let SPS = Cc['@songbirdnest.com/servicepane/service;1']
                .getService(Ci.sbIServicePaneService);
    
    // Don't do anything if node exists already
    if (SPS.getNodesByAttributeNS(this.SC_NS, "SHOUTcast", "true").length > 0)
      return;
    
    // Get favorites list
    let libGuid = Application.prefs.getValue("extensions.shoutcast-radio.templib.guid", "");
    if (libGuid == "")
      return;

    let libraryManager =
        Cc["@songbirdnest.com/Songbird/library/Manager;1"]
        .getService(Ci.sbILibraryManager);
    let tempLib;
    try {
      tempLib = libraryManager.getLibrary(libGuid);
    } catch (e) {
      // Library wasn't created yet - don't create a node
      return;
    }

    let array = tempLib.getItemsByProperty(SBProperties.customType,
                                           "radio_favouritesList");
    if (!array.length)
      return;

    let favesList = array.queryElementAt(0, Ci.sbIMediaList);
    if (!favesList.length) {
      // Don't create a node if we don't have any favorites
      return;
    }

    // Get radio folder
    let radioFolder = SPS.getNode("SB:RadioStations");
    if (!radioFolder) {
      Components.utils.reportError(new Exception("Unexpected: attempt to create SHOUTcast favorites node without radio folder being created first"));
      return;
    }

    // Create a node
    let strings = new SBStringBundle("chrome://shoutcast-radio/locale/overlay.properties");
    let node = SPS.createNode();
    // The new service pane service needs the list guid in the id
    node.id = "urn:item:" + favesList.guid;
    node.className = "medialist-favorites medialist medialisttype-" + favesList.type;
    node.name = strings.get("favourites", "Favorite Stations");
    node.contractid = SP_CONTRACTID;
    node.editable = false;
    node.setAttributeNS(this.LS_NS, "ListType", favesList.type);
    node.setAttributeNS(this.LS_NS, "ListGUID", favesList.guid);
    node.setAttributeNS(this.LS_NS, "LibraryGUID", tempLib.guid);
    node.setAttributeNS(this.LS_NS, "ListCustomType", favesList.getProperty(SBProperties.customType));
    node.setAttributeNS(this.LS_NS, "LibraryCustomType", tempLib.getProperty(SBProperties.customType));
    node.setAttributeNS(this.SC_NS, "SHOUTcast", "true"); // to be recognizeable
    radioFolder.appendChild(node);
  }
}
