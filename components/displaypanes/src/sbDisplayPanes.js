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
Components.utils.import("resource://app/components/ArrayConverter.jsm");

const SONGBIRD_DISPLAYPANE_MANAGER_IID = Components.interfaces.sbIDisplayPaneManager;

function SB_NewDataRemote(a,b) {
  return (new Components.Constructor("@songbirdnest.com/Songbird/DataRemote;1",
                    "sbIDataRemote", "init"))(a,b);
}

function DisplayPaneManager() {
}

DisplayPaneManager.prototype.constructor = DisplayPaneManager;

DisplayPaneManager.prototype = {
  classDescription: "Songbird Display Pane Manager Service Interface",
  classID:          Components.ID("{6aef120f-d7ad-414d-a93d-3ac945e64301}"),
  contractID:       "@songbirdnest.com/Songbird/DisplayPane/Manager;1",

  LOG: function(str) {
    var consoleService = Components.classes['@mozilla.org/consoleservice;1']
                            .getService(Components.interfaces.nsIConsoleService);
    consoleService.logStringMessage(str);
  },
  
  _contentList: [],
  _instantiatorsList: [],
  _delayedInstantiations: [],
  _listenersList: [],
  
  /**
   * given a list of pane parameters, return a new sbIDisplayPaneContentInfo
   */
  makePaneInfo: function(aContentUrl,
                         aContentTitle,
                         aContentIcon,
                         aSuggestedContentGroups,
                         aDefaultWidth,
                         aDefaultHeight) {
    return {
      _title: aContentTitle,
      _icon: aContentIcon,
      get contentUrl() { return aContentUrl; },
      get contentTitle() { return this._title; },
      get contentIcon() { return this._icon; },
      get suggestedContentGroups() { return aSuggestedContentGroups; },
      get defaultWidth() { return aDefaultWidth; },
      get defaultHeight() { return aDefaultHeight; },
      QueryInterface : function(iid) {
        if (iid.equals(Components.interfaces.sbIDisplayPaneContentInfo) ||
            iid.equals(Components.interfaces.nsISupports))
          return this;
        throw Components.results.NS_NOINTERFACE;
      },
      updateContentInfo: function(aNewTitle, aNewIcon) {
        this._title = aNewTitle;
        this._icon = aNewIcon;
      }
    };
  },

  /**
   * \see sbIDisplayPaneManager
   */
  getPaneInfo: function(aContentUrl) {
    for each (var pane in this._contentList) {
      if (pane.contentUrl == aContentUrl) 
        return pane;
    }
    return null;
  },

  /**
   * \see sbIDisplayPaneManager
   */
  get contentList() {
    return ArrayConverter.enumerator(this._contentList);
  },
  
  /**
   * \see sbIDisplayPaneManager
   */
  get instantiatorsList() {
    return ArrayConverter.enumerator(this._instantiatorsList);
  },
  
  /**
   * \see sbIDisplayPaneManager
   */
  registerContent: function(aContentUrl,
                            aContentTitle,
                            aContentIcon,
                            aDefaultWidth,
                            aDefaultHeight,
                            aSuggestedContentGroups,
                            aAutoShow) {
    var info = this.getPaneInfo(aContentUrl);
    if (info) {
      throw Components.results.NS_ERROR_ALREADY_INITIALIZED;
    }
    info = this.makePaneInfo(aContentUrl,
                                  aContentTitle,
                                  aContentIcon,
                                  aSuggestedContentGroups,
                                  aDefaultWidth,
                                  aDefaultHeight);
    this._contentList.push(info);
    for each (var listener in this._listenersList) {
      listener.onRegisterContent(info);
    }
    // if we have never seen this pane, show it in its prefered group
    var known = SB_NewDataRemote("displaypane.known." + aContentUrl, null);
    if (!known.boolValue) {
      if (aAutoShow) {
        if (!this.tryInstantiation(info)) {
          this._delayedInstantiations.push(info);
        }
      }
      // remember we've seen this pane, let the pane hosts reload on their own if they need to
      known.boolValue = true;
    }
  },
  
  /**
   * \see sbIDisplayPaneManager
   */
  unregisterContent: function(aContentUrl) {
    for (var contentIndex = 0; contentIndex < this._contentList.length; contentIndex++) {
      if (this._contentList[contentIndex].contentUrl != aContentUrl) {
        continue;
      }

      // any instantiator currently hosting this url should be emptied
      for each (var instantiator in this._instantiatorsList) {
        if (instantiator.contentUrl == aContentUrl) {
          instantiator.hide();
        }
      }
      // also remove it from the delayed instantiation list
      for (instantiatorIndex = this._delayedInstantiations.length - 1; instantiatorIndex >= 0; --instantiatorIndex) {
        if (this._delayedInstantiations[instantiatorIndex].contentUrl == aContentUrl) {
          this._delayedInstantiations.splice(instantiatorIndex, 1);
        }
      }

      var [info] = this._contentList.splice(contentIndex, 1);
      
      for each (var listener in this._listenersList) {
        listener.onUnregisterContent(info);
      }
      return;
    }
  },
  
  /**
   * \see sbIDisplayPaneManager
   */
  registerInstantiator: function(aInstantiator) {
    this._instantiatorsList.push(aInstantiator);
    for each (var listener in this._listenersList) {
      listener.onRegisterInstantiator(aInstantiator);
    }
    this.processDelayedInstantiations();
  },

  /**
   * \see sbIDisplayPaneManager
   */
  unregisterInstantiator: function(aInstantiator) {
    var index = this._instantiatorsList.indexOf(aInstantiator);
    if (index < 0) {
      // not found
      return;
    }
    this._instantiatorsList.splice(index, 1);
    for each (var listener in this._listenersList) {
      listener.onUnregisterInstantiator(aInstantiator);
    }
  },
  
  /**
   * given a content group list (from a sbIDisplayPaneContentInfo),
   * return the the first instantiator that matches the earliest possible
   * content group
   */
  getFirstInstantiatorForGroupList: function(aContentGroupList) {
    var groups = aContentGroupList.toUpperCase().split(";");
    for each (var group in groups) {
      for each (var instantiator in this._instantiatorsList) {
        if (instantiator.contentGroup.toUpperCase() == group) {
          return instantiator;
        }
      }
    }
    return null;
  },
  
  processDelayedInstantiations: function() {
    var table = [];
    for each (var info in this._delayedInstantiations) {
      if (!this.isValidPane(info) || this.tryInstantiation(info)) {
        continue;
      }
      table.push(info);
    }
    this._delayedInstantiations = table;
  },
  
  tryInstantiation: function(info) {
    var instantiator = this.getFirstInstantiatorForGroupList(info.suggestedContentGroups);
    if (instantiator) {
      instantiator.loadContent(info);
      return true;
    }
    return false;
  },
  
  isValidPane: function(aPane) {
    for each (var pane in this._contentList) {
      if (pane == aPane) return true;
    }
    return false;
  },

  showPane: function(aContentUrl) {
    for each (var instantiator in this._instantiatorsList) {
      if (instantiator.contentUrl == aContentUrl) {
        // we already have a pane with this content
        instantiator.collapsed = false;
        return;
      }
    }
    var info = this.getPaneInfo(aContentUrl);
    if (info) {
      if (!this.tryInstantiation(info)) {
        this._delayedInstantiations.push(info);
      }
    } else {
      throw new Error("Content URL was not found in list of registered panes");
    }
  },
  
  addListener: function(aListener) {
    this._listenersList.push(aListener);
  },
  
  removeListener: function(aListener) {
    var index = this._listenersList.indexOf(aListener);
    if (index > -1)
      this._listenersList.splice(index, 1);
  },
  
  updateContentInfo: function(aContentUrl, aNewContentTitle, aNewContentIcon) {
    var info = this.getPaneInfo(aContentUrl);
    if (!info) {
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    }

    info.updateContentInfo(aNewContentTitle, aNewContentIcon);
    // change the live title for every instance of this content
    for each (var instantiator in this._instantiatorsList) {
      if (instantiator.contentUrl == aContentUrl) {
        instantiator.contentTitle = aNewContentTitle;
        instantiator.contentIcon = aNewContentIcon;
      }
    }
    for each (var listener in this._listenersList) {
      listener.onPaneInfoChanged(info);
    }
  },

  /**
   * \see nsISupports.idl
   */
  QueryInterface:
    XPCOMUtils.generateQI([SONGBIRD_DISPLAYPANE_MANAGER_IID])
}; // DisplayPaneManager.prototype

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([DisplayPaneManager]);
}

