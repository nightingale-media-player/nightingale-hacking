/**
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const SONGBIRD_ADDONPANES_IID = Components.interfaces.sbIAddOnPanes;

function SB_NewDataRemote(a,b) {
  return (new Components.Constructor("@songbirdnest.com/Songbird/DataRemote;1",
                    "sbIDataRemote", "init"))(a,b);
}

function AddOnPanes() {
}

AddOnPanes.prototype.constructor = AddOnPanes;

AddOnPanes.prototype = {
  classDescription: "Songbird AddOnPanes Service Interface",
  classID:          Components.ID("{05d56364-11b2-4e21-94df-73b99492f358}"),
  contractID:       "@songbirdnest.com/Songbird/AddOnPanes;1",

  LOG: function(str) {
    var consoleService = Components.classes['@mozilla.org/consoleservice;1']
                            .getService(Components.interfaces.nsIConsoleService);
    consoleService.logStringMessage(str);
  },
  
  _contentList: [],
  _instantiatorsList: [],
  _delayedInstantiations: [],
  
  makeEnumerator: function(enumeratedtable) {
    return {
      ptr: 0,
      table: enumeratedtable,
      hasMoreElements : function() {
        return this.ptr<this.table.length;
      },
      getNext : function() {
        return this.table[this.ptr++];
      },
      QueryInterface : function(iid) {
        if (iid.equals(Components.interfaces.nsISimpleEnumerator) ||
            iid.equals(Components.interfaces.nsISupports))
          return this;
        throw Components.results.NS_NOINTERFACE;
      }
    };
  },

  makeAddOnPaneInfo: function(aContentUrl,
                              aContentTitle,
                              aSuggestedContentGroup,
                              aDefaultWidth,
                              aDefaultHeight) {
    return {
      get contentUrl() { return aContentUrl; },
      get contentTitle() { return aContentTitle; },
      get suggestedContentGroup() { return aSuggestedContentGroup; },
      get defaultWidth() { return aDefaultWidth; },
      get defaultHeight() { return aDefaultHeight; },
      QueryInterface : function(iid) {
        if (iid.equals(Components.interfaces.sbIAddOnPaneInfo) ||
            iid.equals(Components.interfaces.nsISupports))
          return this;
        throw Components.results.NS_NOINTERFACE;
      }
    };
  },

  getPaneInfo: function(aContentUrl) {
    for (var i=0;i<this._contentList.length;i++) {
      if (this._contentList[i].contentUrl == aContentUrl) 
        return this._contentList[i];
    }
    return null;
  },

  get contentList() {
    return this.makeEnumerator(this._contentList);
  },
  
  get instantiatorsList() {
    return this.makeEnumerator(this._instantiatorsList);
  },
  
  registerContent: function(aContentUrl,
                            aContentTitle,
                            aDefaultWidth,
                            aDefaultHeight,
                            aSuggestedContentGroup,
                            aAutoShow) {
    var info = this.getPaneInfo(aContentUrl);
    if (!info) {
      info = this.makeAddOnPaneInfo(aContentUrl,
                                    aContentTitle,
                                    aSuggestedContentGroup,
                                    aDefaultWidth,
                                    aDefaultHeight);
      this._contentList.push(info);
      // if we have never seen this pane, show it in its prefered group
      var known = SB_NewDataRemote("addonpane.known." + aContentUrl, null);
      if (!known.boolValue) {
        if (aAutoShow) {
          if (!this.tryInstantiation(info)) {
            this._delayedInstantiations.push(info);
          }
        }
        // remember we've seen this pane, let the pane hosts reload on their own if they need to
        known.boolValue = true;
      }
    }
  },
  
  unregisterContent: function(aContentUrl) {
    for (var i=0;i<this._contentList.length;i++) {
      if (this._contentList[i].contentUrl == aContentUrl) {
        // any instantiator currently hosting this url should be emptied
        for (var j=0;j<this._instantiatorsList.length;j++) {
          if (this._instantiatorsList[j].contentUrl == aContentUrl)
            this._instantiatorsList[j].hide();
        }
        this._contentList.splice(i, 1);
        return;
      }
    }
  },
  
  registerInstantiator: function(aInstantiator) {
    this._instantiatorsList.push(aInstantiator);
    this.processDelayedInstantiations();
  },

  unregisterInstantiator: function(aInstantiator) {
    for (var i=0;i<this.instantiatorsList.length;i++) {
      if (this.instantiatorsList[i] == aInstantiator) {
        this.instantiatorsList.splice(i, 1);
        return;
      }
    }
  },
  
  getFirstInstantiatorForGroup: function(aContentGroup) {
    var groups = aContentGroup.split(";");
    for (var j=0;j<groups.length;j++) {
      var ugroup = groups[j].toUpperCase();
      for (var i=0;i<this._instantiatorsList.length;i++) {
        if (this._instantiatorsList[i].contentGroup.toUpperCase() == ugroup) 
          return this._instantiatorsList[i];
      }
    }
    return null;
  },
  
  processDelayedInstantiations: function() {
    var table = [];
    for (var i=0;i<this._delayedInstantiations.length;i++) {
      var info = this._delayedInstantiations[i];
      if (!this.isValidPane(info) || this.tryInstantiation(info)) continue;
      table.push(info);
    }
    this._delayedInstantiations = table;
  },
  
  tryInstantiation: function(info) {
    var instantiator = this.getFirstInstantiatorForGroup(info.suggestedContentGroup);
    if (instantiator) {
      instantiator.loadContent(info.contentUrl,
                                info.contentTitle,
                                info.defaultWidth,
                                info.defaultHeight);
      instantiator.collapsed = false;
      return true;
    }
    return false;
  },
  
  isValidPane: function(pane) {
    for (var i=0;i<this._contentList.length;i++) {
      if (this._contentList[i] == pane) return true;
    }
    return false;
  },

  showPane: function(aContentUrl) {
    var info = this.getPaneInfo(aContentUrl);
    if (info) {
      if (!this.tryInstantiation(info)) {
        this._delayedInstantiations.push(info);
      }
    } else {
      throw new Error("Content URL was not found in list of registered panes");
    }
  },
  
  /**
   * See nsISupports.idl
   */
  QueryInterface:
    XPCOMUtils.generateQI([SONGBIRD_ADDONPANES_IID])
}; // AddOnPanes.prototype

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([AddOnPanes]);
}

