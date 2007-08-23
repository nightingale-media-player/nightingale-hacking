
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

const SONGBIRD_PLAYLISTCOMMANDSBUILDER_CONTRACTID = "@songbirdnest.com/Songbird/PlaylistCommandsBuilder;1";
const SONGBIRD_PLAYLISTCOMMANDSBUILDER_CLASSNAME = "Songbird Playlist Commands Builder";
const SONGBIRD_PLAYLISTCOMMANDSBUILDER_CID = Components.ID("{18c4b63c-d5b4-4aa2-918f-a31304d254ec}");
const SONGBIRD_PLAYLISTCOMMANDSBUILDER_IID = Components.interfaces.sbIPlaylistCommandsBuilder;

const SONGBIRD_PLAYLISTCOMMANDS_TYPE_ACTION     = "action";
const SONGBIRD_PLAYLISTCOMMANDS_TYPE_SEPARATOR  = "separator";
const SONGBIRD_PLAYLISTCOMMANDS_TYPE_MENU       = "submenu";
const SONGBIRD_PLAYLISTCOMMANDS_TYPE_FLAG       = "flag";
const SONGBIRD_PLAYLISTCOMMANDS_TYPE_VALUE      = "value";
const SONGBIRD_PLAYLISTCOMMANDS_TYPE_CHOICE     = "choice";
const SONGBIRD_PLAYLISTCOMMANDS_TYPE_CHOICEITEM = "choiceitem";
const SONGBIRD_PLAYLISTCOMMANDS_TYPE_CUSTOM     = "custom";
const SONGBIRD_PLAYLISTCOMMANDS_TYPE_SUBOBJECT  = "subobject";

const SONGBIRD_PLAYLISTCOMMANDSBUILDER_APPEND = 0;
const SONGBIRD_PLAYLISTCOMMANDSBUILDER_BEFORE = 1;
const SONGBIRD_PLAYLISTCOMMANDSBUILDER_AFTER  = 2;

// ----------------------------------------------------------------------------  

function PlaylistCommandsBuilder() {
  this.m_Context.commands = this;
  this.m_root_commands = new Array();
  this.m_menus = new Array();
  var menuitem = {  m_Id      : null,
                    m_Type    : SONGBIRD_PLAYLISTCOMMANDS_TYPE_MENU,
                    m_Menu    : this.m_root_commands};
  this.m_menus.push(menuitem);
}

// ----------------------------------------------------------------------------  

PlaylistCommandsBuilder.prototype.constructor = PlaylistCommandsBuilder;

// ----------------------------------------------------------------------------  

PlaylistCommandsBuilder.prototype = {
  classDescription: SONGBIRD_PLAYLISTCOMMANDSBUILDER_CLASSNAME,
  classID:          SONGBIRD_PLAYLISTCOMMANDSBUILDER_CID,
  contractID:       SONGBIRD_PLAYLISTCOMMANDSBUILDER_CONTRACTID,

  m_Context : {
    medialist       : null,
    playlist        : null,
    window          : null,
    commands        : null,
    QueryInterface  : function(iid) {
      if (iid.equals(Components.interfaces.sbIPlaylistCommandsContext) || 
          iid.equals(Components.interfaces.nsISupports))
        return this;
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
  },
  
  m_menus             : null,
  m_root_commands     : null,
  m_VisibleCallback   : null,

/**
 * ----------------------------------------------------------------------------
 * sbIPlaylistCommandsBuilder Implementation
 * ----------------------------------------------------------------------------
 */

// ----------------------------------------------------------------------------
// Submenus
// ----------------------------------------------------------------------------

  appendSubmenu: function(aParentSubMenuId, 
                          aSubMenuId, 
                          aLabel, 
                          aTooltipText)
  {
    var props = { m_Name    : aLabel,
                  m_Tooltip : aTooltipText };
    
    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_APPEND,
                        null,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_MENU,
                        aSubMenuId,
                        props);
  },

  insertSubmenuBefore: function(aParentSubMenuId, 
                                aBeforeId,
                                aSubMenuId, 
                                aLabel, 
                                aTooltipText)
  {
    var props = { m_Name    : aLabel,
                  m_Tooltip : aTooltipText };

    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_BEFORE,
                        aBeforeId, 
                        eParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_MENU,
                        aSubMenuId,
                        props);
  },

  appendSubmenuAfter: function(aParentSubMenuId, 
                               aAfterId,
                               aSubMenuId, 
                               aLabel, 
                               aTooltipText)
  {
    var props = { m_Name    : aLabel,
                  m_Tooltip : aTooltipText };

    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_AFTER,
                        aAfterId, 
                        eParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_MENU,
                        aSubMenuId,
                        props);
  },
  
// ----------------------------------------------------------------------------
// Actions
// ----------------------------------------------------------------------------

  appendAction: function(aParentSubMenuId, 
                         aId,
                         aLabel, 
                         aTooltipText, 
                         aTriggerCallback)
  {
    var props = { m_Name            : aLabel,
                  m_Tooltip         : aTooltipText,
                  m_TriggerCallback : aTriggerCallback }
                  
    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_APPEND,
                        null,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_ACTION,
                        aId,
                        props);
  },
  
  insertActionBefore: function(aParentSubMenuId, 
                               aBeforeId,
                               aId,
                               aLabel, 
                               aTooltipText, 
                               aTriggerCallback)
  {
    var props = { m_Name            : aLabel,
                  m_Tooltip         : aTooltipText,
                  m_TriggerCallback : aTriggerCallback }

    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_BEFORE,
                        aBeforeId,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_ACTION,
                        aId,
                        props);
  },
  
  insertActionAfter: function(aParentSubMenuId, 
                              aAfterId,
                              aId,
                              aLabel, 
                              aTooltipText, 
                              aTriggerCallback)
  {
    var props = { m_Name            : aLabel,
                  m_Tooltip         : aTooltipText,
                  m_TriggerCallback : aTriggerCallback }

    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_AFTER,
                        aAfterId,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_ACTION,
                        aId,
                        props);
  },
  
// ----------------------------------------------------------------------------
// Separators
// ----------------------------------------------------------------------------
  
  appendSeparator: function(aParentSubMenuId, 
                            aId)
  {
    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_APPEND,
                        null,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_SEPARATOR,
                        aId);
  },
  
  insertSeparatorBefore: function(aParentSubMenuId, 
                                  aBeforeId,
                                  aId)
  {
    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_BEFORE,
                        aBeforeId,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_SEPARATOR,
                        aId);
  },
  
  insertActionAfter: function(aParentSubMenuId, 
                              aAfterId,
                              aId)
  {
    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_AFTER,
                        aAfterId,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_SEPARATOR,
                        aId);
  },

// ----------------------------------------------------------------------------
// Flag
// ----------------------------------------------------------------------------

  appendFlag: function(aParentSubMenuId, 
                       aId,
                       aLabel, 
                       aTooltipText, 
                       aTriggerCallback,
                       aValueCallback)
  {
    var props = { m_Name            : aLabel,
                  m_Tooltip         : aTooltipText,
                  m_TriggerCallback : aTriggerCallback,
                  m_ValueCallback   : aValueCallback    };
                       
    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_APPEND,
                        null,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_FLAG,
                        aId,
                        props);
                               
  },
  
  insertFlagBefore: function(aParentSubMenuId, 
                             aBeforeId,
                             aId,
                             aLabel, 
                             aTooltipText, 
                             aTriggerCallback,
                             aValueCallback)
  {
    var props = { m_Name            : aLabel,
                  m_Tooltip         : aTooltipText,
                  m_TriggerCallback : aTriggerCallback,
                  m_ValueCallback   : aValueCallback    };

    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_BEFORE,
                        aBeforeId,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_FLAG,
                        aId,
                        props);
  },
  
  insertFlagAfter: function(aParentSubMenuId, 
                            aAfterId,
                            aId,
                            aLabel, 
                            aTooltipText, 
                            aTriggerCallback,
                            aValueCallback)
  {
    var props = { m_Name            : aLabel,
                  m_Tooltip         : aTooltipText,
                  m_TriggerCallback : aTriggerCallback,
                  m_ValueCallback   : aValueCallback    };

    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_AFTER,
                        aAfterId,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_FLAG,
                        aId,
                        props);
  },
  
// ----------------------------------------------------------------------------
// Value
// ----------------------------------------------------------------------------

  appendValue: function(aParentSubMenuId, 
                        aId,
                        aLabel, 
                        aTooltipText, 
                        aTriggerCallback,
                        aValueCallback)
  {
    var props = { m_Name            : aLabel,
                  m_Tooltip         : aTooltipText,
                  m_TriggerCallback : aTriggerCallback,
                  m_ValueCallback   : aValueCallback    };

    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_APPEND,
                        null,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_VALUE,
                        aId,
                        props);
  },
  
  insertValueBefore: function(aParentSubMenuId, 
                              aBeforeId,
                              aId,
                              aLabel, 
                              aTooltipText, 
                              aTriggerCallback,
                              aValueCallback)
  {
    var props = { m_Name            : aLabel,
                  m_Tooltip         : aTooltipText,
                  m_TriggerCallback : aTriggerCallback,
                  m_ValueCallback   : aValueCallback    };
                       
    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_BEFORE,
                        aBeforeId,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_VALUE,
                        aId,
                        props);
  },
  
  insertValueAfter: function(aParentSubMenuId, 
                             aAfterId,
                             aId,
                             aLabel, 
                             aTooltipText, 
                             aTriggerCallback,
                             aValueCallback)
  {
    var props = { m_Name            : aLabel,
                  m_Tooltip         : aTooltipText,
                  m_TriggerCallback : aTriggerCallback,
                  m_ValueCallback   : aValueCallback    };

    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_AFTER,
                        aAfterId,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_VALUE,
                        aId,
                        props);
  },
  
// ----------------------------------------------------------------------------
// Choice menus
// ----------------------------------------------------------------------------

  appendChoiceMenu: function(aParentSubMenuId, 
                             aId,
                             aLabel, 
                             aTooltipText)
  {
    var props = { m_Name    : aLabel,
                  m_Tooltip : aTooltipText };

    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_APPEND,
                        null,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_CHOICE,
                        aId,
                        props);
  },

  insertChoiceMenuBefore: function(aParentSubMenuId, 
                                   aBeforeId,
                                   aId, 
                                   aLabel, 
                                   aTooltipText)
  {
    var props = { m_Name    : aLabel,
                  m_Tooltip : aTooltipText };

    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_BEFORE,
                        aBeforeId, 
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_CHOICE,
                        aId,
                        props);
  },

  appendChoiceMenuAfter: function(aParentSubMenuId, 
                                  aAfterId,
                                  aId, 
                                  aLabel, 
                                  aTooltipText)
  {
    var props = { m_Name    : aLabel,
                  m_Tooltip : aTooltipText };

    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_AFTER,
                        aAfterId, 
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_CHOICE,
                        aId,
                        props);
  },

// ----------------------------------------------------------------------------
// Choice Menu Items
// ----------------------------------------------------------------------------

  appendChoiceMenuItem: function(aParentSubMenuId, 
                                 aId,
                                 aLabel, 
                                 aTooltipText, 
                                 aTriggerCallback)
  {
    var props = { m_Name    : aLabel,
                  m_Tooltip : aTooltipText };

    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_APPEND,
                        null,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_CHOICEITEM,
                        aId,
                        props);
  },
  
  insertChoiceMenuItemBefore: function(aParentSubMenuId, 
                                       aBeforeId,
                                       aId,
                                       aLabel, 
                                       aTooltipText, 
                                       aTriggerCallback)
  {
    var props = { m_Name    : aLabel,
                  m_Tooltip : aTooltipText };

    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_BEFORE,
                        aBeforeId,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_CHOICEITEM,
                        aId,
                        props);
  },
  
  insertChoiceMenuItemAfter: function(aParentSubMenuId, 
                                      aAfterId,
                                      aId,
                                      aLabel, 
                                      aTooltipText, 
                                      aTriggerCallback)
  {
    var props = { m_Name    : aLabel,
                  m_Tooltip : aTooltipText };

    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_AFTER,
                        aAfterId,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_CHOICEITEM,
                        aId,
                        props);
  },

// ----------------------------------------------------------------------------
// Custom Items
// ----------------------------------------------------------------------------

  appendCustomItem: function(aParentSubMenuId, 
                             aId,
                             aInstantiationCallback, 
                             aRefreshCallback)
  {
    var props = { m_InstantiationCallback : aInstantiationCallback,
                  m_RefreshCallback       : aRefreshCallback};

    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_APPEND,
                        null,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_CUSTOM,
                        aId,
                        props);
  },
  
  insertCustomItemBefore: function(aParentSubMenuId, 
                                   aBeforeId,
                                   aId,
                                   aInstantiationCallback, 
                                   aRefreshCallback)
  {
    var props = { m_InstantiationCallback : aInstantiationCallback,
                  m_RefreshCallback       : aRefreshCallback};

    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_BEFORE,
                        aBeforeId,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_CUSTOM,
                        aId,
                        props);
  },
  
  insertCustomItemAfter: function(aParentSubMenuId, 
                                  aAfterId,
                                  aId,
                                  aInstantiationCallback, 
                                  aRefreshCallback)
  {
    var props = { m_InstantiationCallback : aInstantiationCallback,
                  m_RefreshCallback       : aRefreshCallback};

    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_AFTER,
                        aAfterId,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_CUSTOM,
                        aId,
                        props);
  },

// ----------------------------------------------------------------------------
// sbIPlaylistCommands Items
// ----------------------------------------------------------------------------

  appendPlaylistCommands: function(aParentSubMenuId, 
                                   aId,
                                   aPlaylistCommands)
  {
    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_APPEND,
                        null,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_SUBOBJECT,
                        aId,
                        null,
                        aPlaylistCommands);
  },
  
  insertPlaylistCommandsBefore: function(aParentSubMenuId, 
                                         aBeforeId,
                                         aId,
                                         aPlaylistCommands)
  {
    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_BEFORE,
                        aBeforeId,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_SUBOBJECT,
                        aId,
                        null,
                        aPlaylistCommands);
  },
  
  insertPlaylistCommandsAfter: function(aParentSubMenuId, 
                                      aAfterId,
                                      aId,
                                      aPlaylistCommands)
  {
    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_AFTER,
                        aAfterId,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_SUBOBJECT,
                        aId,
                        null,
                        aPlaylistCommands);
  },
  
// ----------------------------------------------------------------------------

  setCommandShortcut: function(aParentSubMenuId, 
                               aCommandId, 
                               aShortcutKey,
                               aShortcutKeyCode,
                               aShortcutModifiers,
                               aShortcutIsLocal)
  {
    var props = { m_ShortcutKey       : aShortcutKey,
                  m_ShortcutKeycode   : aShortcutKeyCode,
                  m_ShortcutModifiers : aShortcutModifiers,
                  m_ShortcutIsLocal   : aShortcutIsLocal };

    this._setCommandProperties(aParentSubMenuId, aCommandId, props);
  },                             

  setCommandEnabledCallback: function(aParentSubMenuId, 
                                      aCommandId, 
                                      aEnabledCallback)
  {
    var props = { m_EnabledCallback : aEnabledCallback };

    this._setCommandProperties(aParentSubMenuId, aCommandId, props);
  },                             

  setCommandVisibleCallback: function(aParentSubMenuId, 
                                      aCommandId, 
                                      aVisibleCallback)
  {
    var props = { m_VisibleCallback : aVisibleCallback };

    this._setCommandProperties(aParentSubMenuId, aCommandId, props);
  },                             

  setVisibleCallback: function(aVisibleCallback)
  {
    this.m_VisibleCallback = aVisibleCallback;
  },                             

// ----------------------------------------------------------------------------  

  removeCommand: function(aParentSubMenuId,
                          aCommandId)
  {
    var menu = this._getMenu(aParentSubMenuId);
    var index = this._getCommandIndex(menu, aCommandId);
    this._removeCommandTree(menu, index);
  },

  removeAllCommands: function()
  {
    while (this.m_root_commands.length) {
      this._removeCommandTree(this.m_root_commands, 0);
    }
  },


// ----------------------------------------------------------------------------  

  _insertCommand: function(aInsertHow,
                           aRefPointId,
                           aParentSubMenuId, 
                           aType,
                           aId,
                           aPropArray,
                           aCommandSubObject)
  {
    if (!aId) return;
    switch (aType) {
      case SONGBIRD_PLAYLISTCOMMANDS_TYPE_ACTION:
      case SONGBIRD_PLAYLISTCOMMANDS_TYPE_SEPARATOR:
      case SONGBIRD_PLAYLISTCOMMANDS_TYPE_MENU:
      case SONGBIRD_PLAYLISTCOMMANDS_TYPE_FLAG:
      case SONGBIRD_PLAYLISTCOMMANDS_TYPE_VALUE:
      case SONGBIRD_PLAYLISTCOMMANDS_TYPE_CHOICE:
      case SONGBIRD_PLAYLISTCOMMANDS_TYPE_CHOICEITEM:
      case SONGBIRD_PLAYLISTCOMMANDS_TYPE_CUSTOM:
      case SONGBIRD_PLAYLISTCOMMANDS_TYPE_SUBOBJECT:
        break;
      default:
        // unknown type !
        throw new Error("Unknown type in insertCommand");
    }
    var menu = this._getMenu(aParentSubMenuId);
    var index = this._getCommandIndex(menu, aId);
    if (index >= 0) {
      throw new Error("Duplicate command '" + 
                      aId + 
                      "' in menu '" + 
                      (aParentSubMenuId ? aParentSubMenuId : "<rootmenu>"));
    }
    var refindex = ((aInsertHow != SONGBIRD_PLAYLISTCOMMANDSBUILDER_APPEND) && 
                    aRefPointId) ? 
                    this._getCommandIndex(menu, aRefPointId) : -1;
    var vrefindex = refindex;
    switch (aInsertHow) {
      case SONGBIRD_PLAYLISTCOMMANDSBUILDER_APPEND:
        break;
      case SONGBIRD_PLAYLISTCOMMANDSBUILDER_AFTER:
        vrefindex++;
        // fallthru
      case SONGBIRD_PLAYLISTCOMMANDSBUILDER_BEFORE:
        if (refindex < 0) {
          throw new Error("Reference command '" + 
                          aRefPointId + 
                          "' not found in menu '" + 
                          (aParentSubMenuId ? aParentSubMenuId : "<rootmenu>"));
        }
        break;
    }
    this._insertCommandInArray(menu,
                               vrefindex,
                               aType,
                               aId,
                               aPropArray,
                               aCommandSubObject);
  },

  _insertCommandInArray: function(aMenu, 
                                  aInsertionIndex,
                                  aType,
                                  aId,
                                  aPropArray,
                                  aCommandSubObject)
  {
    var item = { m_ParentMenu            : aMenu,
                 m_Type                  : aType,
                 m_Id                    : aId };
    if (aPropArray) {
      for (var i in aPropArray) {
        item[i] = aPropArray[i];
      }
    }
    
    switch (aType) {
      case SONGBIRD_PLAYLISTCOMMANDS_TYPE_SEPARATOR:
        item.m_Flex = 1;
        item.m_Name = "*separator*";
        break;
      case SONGBIRD_PLAYLISTCOMMANDS_TYPE_SUBOBJECT:
        if (!aCommandSubObject) return 0;
        aCommandSubObject = aCommandSubObject.QueryInterface(Components.
                                                             interfaces.
                                                             sbIPlaylistCommands);
        item.m_CommandSubObject = aCommandSubObject.duplicate();
        item.m_OriginalCommandSubObject = aCommandSubObject;
        break;
      case SONGBIRD_PLAYLISTCOMMANDS_TYPE_MENU:
      case SONGBIRD_PLAYLISTCOMMANDS_TYPE_CHOICE:
        var menuitem = { m_Id   : aId,
                         m_Type : aType,
                         m_Menu : new Array() };
        this.m_menus.push(menuitem);
        break;
    }
    
    // insert the item in the menu at the requested position
    if (aInsertionIndex < 0 || aInsertionIndex >= aMenu.length)
      aMenu.push(item);
    else
      aMenu.splice(aInsertionIndex, 0, item);
  },
  
  _removeCommandTree: function(aMenu, aIndex) {
    if (aIndex >= aMenu.length || aIndex < 0) {
      throw new Error("Invalid index " + aIndex + " in _removeCommandTree");
    }

    var item = aMenu[aIndex];
    
    if (item.m_Type == SONGBIRD_PLAYLISTCOMMANDS_TYPE_MENU ||
        item.m_Type == SONGBIRD_PLAYLISTCOMMANDS_TYPE_CHOICE) {
      var submenu = this._getMenu(item.m_Id);
      if (submenu && submenu != this.m_root_commands) {
        for (var i=0;i<submenu.length;i++) {
          this._removeCommandTree(submenu, i);
        }
        var idx = this._getMenuIndex(item.m_Id);
        if (idx > 0 && idx < aMenu.length) { // yes, '>', we do not want the root menu
          // command array is empty by now, discard it now
          this.m_menus[idx].m_Menu = null;
          // remove the menu entry
          this.m_menus.splice(idx, 1);
        }
      }
    }
    
    // Remove the command item
    aMenu[aIndex].m_InstantiationCallback = null;
    aMenu[aIndex].m_RefreshCallback = null;
    aMenu[aIndex].m_ValueCallback = null;
    aMenu[aIndex].m_EnabledCallback = null;
    aMenu[aIndex].m_VisibleCallback = null;
    aMenu[aIndex].m_TriggerCallback = null;
    aMenu.splice(aIndex, 1);
  },

  _getMenu: function(aSubMenuId)
  {
    if (!aSubMenuId) return this.m_root_commands;
    for (var i=0;i<this.m_menus.length;i++) {
      if (this.m_menus[i].m_Id == aSubMenuId) {
        return this.m_menus[i].m_Menu;
      }
    }
    throw new Error("Submenu " + aSubMenuId + " not found");
  },

  _getMenuIndex: function(aSubMenuId)
  {
    if (!aSubMenu) return 0;
    for (var i=0;i<this.m_menus.length;i++) {
      if (this.m_menus[i].m_Id == aSubMenuId) {
        return i;
      }
    }
    throw new Error("Submenu " + aSubMenuId + " not found");
    return -1;
  },
  
  _getCommandIndex: function(aSubMenu, aCommandId)
  {
    if (!aSubMenu) return -1;
    for (var i=0;i<aSubMenu.length;i++) {
      if (aSubMenu[i].m_Id == aCommandId) {
        return i;
      }
    }
    return -1;
  },
  
  _getCommandProperty: function(aSubMenuId, 
                                aIndex, 
                                aHost, 
                                aProperty, 
                                aDefault)
  {
    try {
      var menu = this._getMenu(aSubMenuId);
      if (!menu) return aDefault;
      if (aIndex >= menu.length) return aDefault;
      var prop = menu[aIndex][aProperty];
      if (prop === undefined) return aDefault;
      if (typeof(prop) == "object" && prop.handleCallback) {
        return prop.handleCallback(this.m_Context,
                                   aSubMenuId, 
                                   menu[aIndex].m_Id, 
                                   aHost, 
                                   aProperty.substr(2));
      }
      return prop;
    } catch (e) {
      this.LOG("_getCommandProperty(" + aSubMenuId + ", " + aIndex + ", " + aHost +
            ", " + aProperty + ", " + aDefault + ") - " + e);
    }
    return aDefault;
  },
  
  _getMenuProperty: function(aMenu, aHost, aProperty, aDefault)
  {
    try {
      var prop = aMenu[aProperty];
      if (prop === undefined) return aDefault;
      if (typeof(prop) == "object" && prop.handleCallback) {
        return prop.handleCallback(this.m_Context,
                                  aMenu.m_ParentMenu,
                                  aMenu.m_Id,
                                  aHost, 
                                  aProperty.substr(2));
      }
      return prop;
    } catch (e) {
      this.LOG("_getMenuProperty(" + aMenu.m_Id + ", " + aHost +
            ", " + aProperty + ", " + aDefault + ") - " + e);
    }
    return aDefault;
  },

  _setCommandProperties: function(aSubMenuId, aCommandId, aPropArray)
  {
    var menu = this._getMenu(aSubMenuId);
    var index = this._getCommandIndex(menu, aCommandId);
    if (index >= menu.length || index < 0) {
      throw new Error("Invalid command index " + index + " in _setCommandProperties");
    }
    for (var i in aPropArray) {
      menu[index][i] = aPropArray[i];
    }
  },

/**
 * ----------------------------------------------------------------------------
 * sbIPlaylistCommands Implementation
 * ----------------------------------------------------------------------------
 */
 
  getVisible: function( aHost )
  {
    if (this.m_VisibleCallback &&
        typeof(this.m_VisibleCallback) == "object" && 
        this.m_VisibleCallback.handleCallback) {
      return this.m_VisibleCallback.handleCallback(this.m_Context, aHost, "Visible");
    }
    return true;
  },

  getNumCommands: function( aSubMenuId, aHost )
  {
    var menu = this._getMenu(aSubMenuId);
    return menu.length;
  },
  
  getCommandId: function( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_Id", 
                                    "");
  },
  
  getCommandType: function( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_Type", 
                                    "");
  },

  getCommandText: function( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_Name", 
                                    "");
  },

  getCommandFlex: function( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_Flex", 
                                    false);
  },

  getCommandToolTipText: function( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_Tooltip", 
                                    "");
  },

  getCommandValue: function( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_ValueCallback", 
                                    "");
  },

  instantiateCustomCommand: function( aDocument, aId, aHost ) 
  {
    var menu = this._getMenu(aSubMenuId);
    var index = this._getCommandIndex(menu, aId);
    if (index >= menu.length || index < 0) {
      throw new Error("Invalid index " + index + " in instantiateCustomCommand");
    }
    var cb = menu[aIndex].m_InstantiationCallback;
    if (cb) return cb.handleCallback(this.m_Context,
                                     menu[aIndex].m_ParentMenu,
                                     menu[aIndex].m_Id,
                                     aHost);
    throw new Error("No custom command instantiation callback defined for command " + aId);
  },

  refreshCustomCommand: function( aElement, aId, aHost ) 
  {
    var menu = this._getMenu(aSubMenuId);
    var index = this._getCommandIndex(menu, aId);
    if (index >= menu.length || index < 0) {
      throw new Error("Invalid index " + index + " in refreshCustomCommand");
    }
    var cb = menu[aIndex].m_RefreshCallback;
    if (cb) cb.handleCallback(this.m_Context,
                              menu[aIndex].m_ParentMenu,
                              menu[aIndex].m_Id,
                              aHost);
  },

  getCommandVisible: function( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_VisibleCallback", 
                                    true);
  },

  getCommandFlag: function( aSubmenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_ValueCallback", 
                                    false);
  },

  getCommandChoiceItem: function( aChoiceMenu, aHost )
  {
    var menu = this._getMenu(aChoiceMenu);
    if (menu.m_Type != SONGBIRD_PLAYLISTCOMMANDS_TYPE_CHOICE) return "";
    return this._getMenuProperty(menu, aHost, "m_CurItemCallback", true, "");
  },

  getCommandEnabled: function( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_EnabledCallback", 
                                    true);
  },

  getCommandShortcutModifiers: function ( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_ShortcutModifiers", 
                                    "");
  },

  getCommandShortcutKey: function ( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_ShortcutKey", 
                                    "");
  },

  getCommandShortcutKeycode: function ( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_ShortcutKeycode", 
                                    "");
  },

  getCommandShortcutLocal: function ( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_ShortcutIsLocal", 
                                    "");
  },

  getCommandSubObject: function ( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_CommandSubObject", 
                                    null);
  },

  onCommand: function(aSubMenuId, aIndex, aHost, id, value)
  {
    var menu = this._getMenu(aSubMenuId);
    if (aIndex >= menu.length || aIndex < 0) {
      throw new Error("Invalid index " + index + " in PlaylistCommandsBuilder::onCommand");
    }

    if (menu[aIndex].m_Id != id) {
      throw new Error("Id do not match in PlaylistCommandsBuilder::onCommand (" + 
                      id + 
                      "/" + 
                      menu[aIndex].m_Id + 
                      ")");
    }
    var cb = menu[aIndex].m_TriggerCallback;
    if (cb === undefined || !cb.handleCallback) {
      throw new Error("No callback defined for command " + id);
    }
    cb.handleCallback(this.m_Context,
                      aSubMenuId, 
                      menu[aIndex].m_Id, 
                      aHost, 
                      value);
  },
  
  duplicate: function()
  {
    // duplicate this
    var obj = this.dupObject(this);

    // after duplication, the arrays are shared between the two objects,
    // so copy the command structure over to the new object, with new arrays
    obj.m_menus = new Array();
    for (var i=0;i<this.m_menus.length;i++) {
      var menuitem = this.dupObject(this.m_menus[i]);
      menuitem.m_Menu = new Array();
      for (var j=0;j<this.m_menus[i].m_Menu.length;j++) {
        var item = this.dupObject(this.m_menus[i].m_Menu[j]);
        // if the item is an sbIPlaylistCommands sub-object, duplicate it as well
        if (item.m_CommandSubObject) {
          item.m_CommandSubObject = item.m_OriginalCommandSubObject.duplicate();
        }
        menuitem.m_Menu.push(item);
      }
      obj.m_menus.push(menuitem);
    }
    obj.m_root_commands = obj.m_menus[0].m_Menu;
        
    // return the copy
    return obj;
  },
  
  dupObject: function(obj) {
    var r = {};
    for ( var i in obj )
    {
      r[ i ] = obj[ i ];
    }
    return r;
  },

  initCommands: function(aHost) { 
    for (var i=0;i<this.m_menus.length;i++) {
      for (var j=0;j<this.m_menus[i].m_Menu.length;j++) {
        if (this.m_menus[i].m_Menu[j].m_CommandSubObject) {
          this.m_menus[i].m_Menu[j].m_CommandSubObject.initCommands(aHost);
        }
      }
    }
  },

  shutdownCommands: function() { 
    for (var i=0;i<this.m_menus.length;i++) {
      for (var j=0;j<this.m_menus[i].m_Menu.length;j++) {
        if (this.m_menus[i].m_Menu[j].m_CommandSubObject) {
          this.m_menus[i].m_Menu[j].m_CommandSubObject.shutdownCommands();
        }
      }
    }
    // empty arrays, remove any remaining sbIPlaylistCommands reference
    // this also removes all references to item callback functions
    this.removeAllCommands();
    // but one callback function reference remains, the one for the entire 
    // command set, reset it too
    this.m_VisibleCallback = null;
    // and forget context
    this.m_Context = null;
  },
  
  setContext: function( context )
  {
    for (var i=0;i<this.m_menus.length;i++) {
      for (var j=0;j<this.m_menus[i].m_Menu.length;j++) {
        if (this.m_menus[i].m_Menu[j].m_CommandSubObject) {
          this.m_menus[i].m_Menu[j].m_CommandSubObject.setContext(context);
        }
      }
    }
    
    this.m_Context.playlist = context.playlist;
    this.m_Context.medialist = context.medialist;
    this.m_Context.window = context.window;
    this.m_Context.commands = this;
  },
  
    
// ----------------------------------------------------------------------------  

  LOG: function(str) {
    var consoleService = Components.classes['@mozilla.org/consoleservice;1']
                            .getService(Components.interfaces.nsIConsoleService);
    consoleService.logStringMessage(str);
  },
  
// ----------------------------------------------------------------------------  

  /**
   * See nsISupports.idl
   */
  QueryInterface: function(iid) {
    if (!iid.equals(Components.interfaces.sbIPlaylistCommands) &&
        !iid.equals(Components.interfaces.sbIPlaylistCommandsBuilder) &&
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
}; // PlaylistCommandsBuilder.prototype


function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([PlaylistCommandsBuilder]);
}

