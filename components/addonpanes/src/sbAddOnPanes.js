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
                            aSuggestedContentGroup) {
  
    var info = {
      get contentUrl() { return aContentUrl; },
      get contentTitle() { return aContentTitle; },
      get defaultWidth() { return aDefaultWidth; },
      get defaultHeight() { return aDefaultHeight; },
      get suggestedContentGroup() { return aSuggestedContentGroup; },
      QueryInterface : function(iid) {
        if (iid.equals(Components.interfaces.sbIAddOnPaneInfo) ||
            iid.equals(Components.interfaces.nsISupports))
          return this;
        throw Components.results.NS_NOINTERFACE;
      }
      
    }
    this._contentList.push(info);
    if (!this.inDb(aContentUrl)) {
      this.saveToDb(info);
      if (!this.tryInstantiation(info)) {
        this._delayedInstantiations.push(info);
      }
    }
  },
  
  unregisterContent: function(aContentUrl) {
    for (var i=0;i<this.contentList.length;i++) {
      if (this.contentList[i].contentUrl == aContentUrl) {
        this.contentList.splice(i, 1);
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
      }
    }
  },
  
  inDb: function(aContentUrl) {
    return false;
  },
  
  saveInDb: function(aContentUrl) {
  },
  
  getFirstInstantiatorForGroup: function(aContentGroup) {
    for (var i=0;i<this._instantiatorsList.length;i++) {
      if (this._instantiatorsList[i].contentGroup.toUpperCase() == aContentGroup.toUpperCase()) 
        return this._instantiatorsList[i];
    }
    return null;
  },
  
  processDelayedInstantiations: function() {
    var table = [];
    for (var i=0;i<this._delayedInstantiations.length;i++) {
      var info = this._delayedInstantiations[i];
      if (this.tryInstantiation(info)) continue;
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
      instantiator.expand();
      return true;
    }
    return false;
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

