
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
    implementorContext: null,
    QueryInterface  : function(iid) {
      if (iid.equals(Components.interfaces.sbIPlaylistCommandsContext) || 
          iid.equals(Components.interfaces.sbIPlaylistCommandsBuilderContext) ||
          iid.equals(Components.interfaces.nsISupports))
        return this;
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
  },
  
  m_menus             : null,
  m_root_commands     : null,
  m_InitCallback      : null,
  m_ShutdownCallback  : null,
  m_VisibleCallback   : null,

/**
 * ----------------------------------------------------------------------------
 * sbIPlaylistCommandsBuilder Implementation
 * ----------------------------------------------------------------------------
 */

// ----------------------------------------------------------------------------
// Submenus
// ----------------------------------------------------------------------------

  appendSubmenu: function 
    PlaylistCommandsBuilder_appendSubmenu(aParentSubMenuId, 
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

  insertSubmenuBefore: function 
    PlaylistCommandsBuilder_insertSubmenuBefore(aParentSubMenuId, 
                                                aBeforeId,
                                                aSubMenuId, 
                                                aLabel, 
                                                aTooltipText)
  {
    var props = { m_Name    : aLabel,
                  m_Tooltip : aTooltipText };

    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_BEFORE,
                        aBeforeId, 
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_MENU,
                        aSubMenuId,
                        props);
  },

  insertSubmenuAfter: function 
    PlaylistCommandsBuilder_insertSubmenuAfter(aParentSubMenuId, 
                                               aAfterId,
                                               aSubMenuId, 
                                               aLabel, 
                                               aTooltipText)
  {
    var props = { m_Name    : aLabel,
                  m_Tooltip : aTooltipText };

    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_AFTER,
                        aAfterId, 
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_MENU,
                        aSubMenuId,
                        props);
  },
  
// ----------------------------------------------------------------------------
// Actions
// ----------------------------------------------------------------------------

  appendAction: function 
    PlaylistCommandsBuilder_appendAction(aParentSubMenuId, 
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
  
  insertActionBefore: 
    function PlaylistCommandsBuilder_insertActionBefore(aParentSubMenuId, 
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
  
  insertActionAfter: function 
    PlaylistCommandsBuilder_insertActionAfter(aParentSubMenuId, 
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
  
  appendSeparator: function 
    PlaylistCommandsBuilder_appendSeparator(aParentSubMenuId, aId)
  {
    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_APPEND,
                        null,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_SEPARATOR,
                        aId);
  },
  
  insertSeparatorBefore: function 
    PlaylistCommandsBuilder_insertSeparatorBefore(aParentSubMenuId, 
                                                  aBeforeId,
                                                  aId)
  {
    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_BEFORE,
                        aBeforeId,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_SEPARATOR,
                        aId);
  },
  
  insertSeparatorAfter: function 
    PlaylistCommandsBuilder_insertSeparatorAfter(aParentSubMenuId, 
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

  appendFlag: function 
    PlaylistCommandsBuilder_appendFlag(aParentSubMenuId, 
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
  
  insertFlagBefore: function 
    PlaylistCommandsBuilder_insertFlagBefore(aParentSubMenuId, 
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
  
  insertFlagAfter: function 
    PlaylistCommandsBuilder_insertFlagAfter(aParentSubMenuId, 
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

  appendValue: function 
    PlaylistCommandsBuilder_appendValue(aParentSubMenuId, 
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
  
  insertValueBefore: function 
    PlaylistCommandsBuilder_insertValueBefore(aParentSubMenuId, 
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
  
  insertValueAfter: function 
    PlaylistCommandsBuilder_insertValueAfter(aParentSubMenuId, 
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

  appendChoiceMenu: function 
    PlaylistCommandsBuilder_appendChoiceMenu(aParentSubMenuId, 
                                             aId,
                                             aLabel, 
                                             aTooltipText,
                                             aItemCallback)
  {
    var props = { m_Name         : aLabel,
                  m_Tooltip      : aTooltipText,
                  m_ItemCallback : aItemCallback };

    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_APPEND,
                        null,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_CHOICE,
                        aId,
                        props);
  },

  insertChoiceMenuBefore: function 
    PlaylistCommandsBuilder_insertChoiceMenuBefore(aParentSubMenuId, 
                                                   aBeforeId,
                                                   aId, 
                                                   aLabel, 
                                                   aTooltipText,
                                                   aItemCallback)
  {
    var props = { m_Name         : aLabel,
                  m_Tooltip      : aTooltipText,
                  m_ItemCallback : aItemCallback };

    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_BEFORE,
                        aBeforeId, 
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_CHOICE,
                        aId,
                        props);
  },

  insertChoiceMenuAfter: function 
    PlaylistCommandsBuilder_insertChoiceMenuAfter(aParentSubMenuId, 
                                                  aAfterId,
                                                  aId, 
                                                  aLabel, 
                                                  aTooltipText,
                                                  aItemCallback)
  {
    var props = { m_Name         : aLabel,
                  m_Tooltip      : aTooltipText,
                  m_ItemCallback : aItemCallback };

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

  appendChoiceMenuItem: function 
    PlaylistCommandsBuilder_appendChoiceMenuItem(aParentSubMenuId, 
                                                 aId,
                                                 aLabel, 
                                                 aTooltipText, 
                                                 aTriggerCallback)
  {
    var props = { m_Name            : aLabel,
                  m_Tooltip         : aTooltipText,
                  m_TriggerCallback : aTriggerCallback };

    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_APPEND,
                        null,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_CHOICEITEM,
                        aId,
                        props);
  },
  
  insertChoiceMenuItemBefore: function 
    PlaylistCommandsBuilder_insertChoiceMenuItemBefore(aParentSubMenuId, 
                                                       aBeforeId,
                                                       aId,
                                                       aLabel, 
                                                       aTooltipText, 
                                                       aTriggerCallback)
  {
    var props = { m_Name            : aLabel,
                  m_Tooltip         : aTooltipText,
                  m_TriggerCallback : aTriggerCallback };

    this._insertCommand(SONGBIRD_PLAYLISTCOMMANDSBUILDER_BEFORE,
                        aBeforeId,
                        aParentSubMenuId,
                        SONGBIRD_PLAYLISTCOMMANDS_TYPE_CHOICEITEM,
                        aId,
                        props);
  },
  
  insertChoiceMenuItemAfter: function 
    PlaylistCommandsBuilder_insertChoiceMenuItemAfter(aParentSubMenuId, 
                                                      aAfterId,
                                                      aId,
                                                      aLabel, 
                                                      aTooltipText, 
                                                      aTriggerCallback)
  {
    var props = { m_Name            : aLabel,
                  m_Tooltip         : aTooltipText,
                  m_TriggerCallback : aTriggerCallback };

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

  appendCustomItem: function 
    PlaylistCommandsBuilder_appendCustomItem(aParentSubMenuId, 
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
  
  insertCustomItemBefore: function 
    PlaylistCommandsBuilder_insertCustomItemBefore(aParentSubMenuId, 
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
  
  insertCustomItemAfter: function 
    PlaylistCommandsBuilder_insertCustomItemAfter(aParentSubMenuId, 
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

  appendPlaylistCommands: function 
    PlaylistCommandsBuilder_appendPlaylistCommands(aParentSubMenuId, 
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
  
  insertPlaylistCommandsBefore: function 
    PlaylistCommandsBuilder_insertPlaylistCommandsBefore(aParentSubMenuId, 
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
  
  insertPlaylistCommandsAfter: function 
    PlaylistCommandsBuilder_insertPlaylistCommandsAfter(aParentSubMenuId, 
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

  setCommandShortcut: function 
    PlaylistCommandsBuilder_setCommandShortcut(aParentSubMenuId, 
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

  setCommandEnabledCallback: function 
    PlaylistCommandsBuilder_setCommandEnabledCallback(aParentSubMenuId, 
                                                      aCommandId, 
                                                      aEnabledCallback)
  {
    var props = { m_EnabledCallback : aEnabledCallback };

    this._setCommandProperties(aParentSubMenuId, aCommandId, props);
  },                             

  setCommandVisibleCallback: function 
    PlaylistCommandsBuilder_setCommandVisibleCallback(aParentSubMenuId, 
                                                      aCommandId, 
                                                      aVisibleCallback)
  {
    var props = { m_VisibleCallback : aVisibleCallback };

    this._setCommandProperties(aParentSubMenuId, aCommandId, props);
  },                             

  setInitCallback: function 
    PlaylistCommandsBuilder_setInitCallback(aInitCallback)
  {
    this.m_InitCallback = aInitCallback;
  },

  setShutdownCallback: function 
    PlaylistCommandsBuilder_setShutdownCallback(aShutdownCallback)
  {
    this.m_ShutdownCallback = aShutdownCallback;
  },

  setVisibleCallback: function 
    PlaylistCommandsBuilder_setVisibleCallback(aVisibleCallback)
  {
    this.m_VisibleCallback = aVisibleCallback;
  },
  
// ----------------------------------------------------------------------------  

  removeCommand: function 
    PlaylistCommandsBuilder_removeCommand(aParentSubMenuId,
                                          aCommandId)
  {
    var menu = this._getMenu(aParentSubMenuId);
    var index = this._getCommandIndex(menu, aCommandId);
    this._removeCommandTree(menu, index);
  },

  removeAllCommands: function 
    PlaylistCommandsBuilder_removeAllCommands(aParentSubMenuId)
  {
    var menu = this._getMenu(aParentSubMenuId);
    while (menu.length) {
      this._removeCommandTree(menu, 0);
    }
    if (!aParentSubMenuId) {
      // after removing all commands, the only remaining menu is the root menu
      if (this.m_menus.length != 1) {
        throw new Error("Menu list is not empty after removeAllCommands (item 1 = " + this.m_menus[1].m_Id + ")");
      }
    }
  },

  shutdown: function PlaylistCommandsBuilder_shutdown()
  {
    // empty arrays, remove any remaining sbIPlaylistCommands reference
    // this also removes all references to item callback functions
    this.removeAllCommands();
    // reset references for all callback functions for the entire command set
    this.m_InitCallback = null;
    this.m_ShutdownCallback = null;
    this.m_VisibleCallback = null;
    // and forget context
    this.m_Context.playlist = null;
    this.m_Context.medialist = null;
    this.m_Context.window = null;
    this.m_Context.commands = this;
    this.m_Context = null;
  },

// ----------------------------------------------------------------------------  

  _insertCommand: function 
    PlaylistCommandsBuilder__insertCommand(aInsertHow,
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

  _insertCommandInArray: function 
    PlaylistCommandsBuilder__insertCommandInArray(aMenu, 
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
        if (aType == SONGBIRD_PLAYLISTCOMMANDS_TYPE_CHOICE) {
          menuitem.m_ItemCallback = aPropArray.m_ItemCallback;
        }
        this.m_menus.push(menuitem);
        break;
    }
    
    // insert the item in the menu at the requested position
    if (aInsertionIndex < 0 || aInsertionIndex >= aMenu.length)
      aMenu.push(item);
    else
      aMenu.splice(aInsertionIndex, 0, item);
  },
  
  _removeCommandTree: function 
    PlaylistCommandsBuilder__removeCommandTree(aMenu, aIndex) {
    if (aIndex >= aMenu.length || aIndex < 0) {
      throw new Error("Invalid index " + aIndex + " in _removeCommandTree");
    }

    var item = aMenu[aIndex];
    
    if (item.m_Type == SONGBIRD_PLAYLISTCOMMANDS_TYPE_MENU ||
        item.m_Type == SONGBIRD_PLAYLISTCOMMANDS_TYPE_CHOICE) {
      var submenu = this._getMenu(item.m_Id);
      if (submenu && submenu != this.m_root_commands) {
        while(submenu.length > 0) {
          this._removeCommandTree(submenu, 0);
        }
        var idx = this._getMenuIndex(item.m_Id);
        if (idx > 0 && idx < this.m_menus.length) { // yes, '>', we do not want the root menu
          // discard the item callback in case this is a choicemenu
          this.m_menus[idx].m_ItemCallback = null;
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

  _getMenu: function PlaylistCommandsBuilder__getMenu(aSubMenuId)
  {
    // null is the root menu
    if (!aSubMenuId) return this.m_root_commands;
    for (var i=0;i<this.m_menus.length;i++) {
      if (this.m_menus[i].m_Id == aSubMenuId) {
        return this.m_menus[i].m_Menu;
      }
    }
    throw new Error("Submenu " + aSubMenuId + " not found");
  },

  _getMenuIndex: function PlaylistCommandsBuilder__getMenuIndex(aSubMenuId)
  {
    if (!aSubMenuId) return 0;
    for (var i=0;i<this.m_menus.length;i++) {
      if (this.m_menus[i].m_Id == aSubMenuId) {
        return i;
      }
    }
    throw new Error("Submenu " + aSubMenuId + " not found");
    return -1;
  },
  
  _getCommandIndex: function 
    PlaylistCommandsBuilder__getCommandIndex(aSubMenu, aCommandId)
  {
    if (!aSubMenu) return -1;
    for (var i=0;i<aSubMenu.length;i++) {
      if (aSubMenu[i].m_Id == aCommandId) {
        return i;
      }
    }
    return -1;
  },
  
  _getCommandProperty: function 
    PlaylistCommandsBuilder__getCommandProperty(aSubMenuId, 
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
      this.LOG("_getCommandProperty(" + aSubMenuId + ", " + aIndex + " (" + 
               menu[aIndex].m_Id + "), " + aHost + ", " + aProperty + ", " + 
               aDefault + ") - " + e);
    }
    return aDefault;
  },
  
  _getMenuProperty: function 
    PlaylistCommandsBuilder__getMenuProperty(aMenu, aHost, aProperty, aDefault)
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

  _setCommandProperties: function 
    PlaylistCommandsBuilder__setCommandProperties(aSubMenuId, aCommandId, aPropArray)
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

  // not used for now, this may eventually be needed if we want to allow
  // changing the item callback for a choicemenu
  _setMenuProperties: function 
    PlaylistCommandsBuilder__setMenuProperties(aMenu, aPropArray)
  {
    for (var i in aPropArray) {
      aMenu[i] = aPropArray[i];
    }
  },

/**
 * ----------------------------------------------------------------------------
 * sbIPlaylistCommands Implementation
 * ----------------------------------------------------------------------------
 */
 
  getVisible: function PlaylistCommandsBuilder_getVisible( aHost )
  {
    if (this.m_VisibleCallback &&
        typeof(this.m_VisibleCallback) == "object" && 
        this.m_VisibleCallback.handleCallback) {
      return this.m_VisibleCallback.handleCallback(this.m_Context, aHost, "Visible");
    }
    return true;
  },

  getNumCommands: function 
    PlaylistCommandsBuilder_getNumCommands( aSubMenuId, aHost )
  {
    var menu = this._getMenu(aSubMenuId);
    return menu.length;
  },
  
  getCommandId: function 
    PlaylistCommandsBuilder_getCommandId( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_Id", 
                                    "");
  },
  
  getCommandType: function 
    PlaylistCommandsBuilder_getCommandType( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_Type", 
                                    "");
  },

  getCommandText: function 
    PlaylistCommandsBuilder_getCommandText( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_Name", 
                                    "");
  },

  getCommandFlex: function 
    PlaylistCommandsBuilder_getCommandFlex( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_Flex", 
                                    false);
  },

  getCommandToolTipText: function 
    PlaylistCommandsBuilder_getCommandToolTipText( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_Tooltip", 
                                    "");
  },

  getCommandValue: function 
    PlaylistCommandsBuilder_getCommandValue( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_ValueCallback", 
                                    "");
  },

  instantiateCustomCommand: function 
    PlaylistCommandsBuilder_instantiateCustomCommand( aSubMenuId, 
                                                      aIndex, 
                                                      aHost, 
                                                      aId, 
                                                      aDocument ) 
  {
    var menu = this._getMenu(aSubMenuId);
    if (aIndex >= menu.length || aIndex < 0) {
      throw new Error("Invalid index " + index + " in instantiateCustomCommand");
    }
    var cb = menu[aIndex].m_InstantiationCallback;
    if (cb) return cb.handleCallback(this.m_Context,
                                     menu[aIndex].m_ParentMenu,
                                     menu[aIndex].m_Id,
                                     aHost,
                                     aDocument);
    throw new Error("No custom command instantiation callback defined for command " + aId);
  },

  refreshCustomCommand: function 
    PlaylistCommandsBuilder_refreshCustomCommand( aSubMenuId, 
                                                  aIndex, 
                                                  aHost, 
                                                  aId, 
                                                  aElement ) 
  {
    var menu = this._getMenu(aSubMenuId);
    if (aIndex >= menu.length || aIndex < 0) {
      throw new Error("Invalid index " + index + " in refreshCustomCommand");
    }
    var cb = menu[aIndex].m_RefreshCallback;
    if (cb) cb.handleCallback(this.m_Context,
                              menu[aIndex].m_ParentMenu,
                              menu[aIndex].m_Id,
                              aHost,
                              aElement);
  },

  getCommandVisible: function 
    PlaylistCommandsBuilder_getCommandVisible( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_VisibleCallback", 
                                    true);
  },

  getCommandFlag: function 
    PlaylistCommandsBuilder_getCommandFlag( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_ValueCallback", 
                                    false);
  },

  getCommandChoiceItem: function 
    PlaylistCommandsBuilder_getCommandChoiceItem( aChoiceMenu, aHost )
  {
    var menuindex = this._getMenuIndex(aChoiceMenu);
    if (menuindex < 0 || menuindex >= this.m_menus.length) {
      throw new Error("Choice menu id not found : " + aChoiceMenu + " in getCommandChoiceItem");
    }
    var menu = this.m_menus[menuindex];
    if (menu.m_Type != SONGBIRD_PLAYLISTCOMMANDS_TYPE_CHOICE) return "";
    return this._getMenuProperty(menu, aHost, "m_ItemCallback", "");
  },

  getCommandEnabled: function 
    PlaylistCommandsBuilder_getCommandEnabled( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_EnabledCallback", 
                                    true);
  },

  getCommandShortcutModifiers: function 
    PlaylistCommandsBuilder_getCommandShortcutModifiers( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_ShortcutModifiers", 
                                    "");
  },

  getCommandShortcutKey: function 
    PlaylistCommandsBuilder_getCommandShortcutKey( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_ShortcutKey", 
                                    "");
  },

  getCommandShortcutKeycode: function 
    PlaylistCommandsBuilder_getCommandShortcutKeycode( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_ShortcutKeycode", 
                                    "");
  },

  getCommandShortcutLocal: function 
    PlaylistCommandsBuilder_getCommandShortcutLocal( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_ShortcutIsLocal", 
                                    false);
  },

  getCommandSubObject: function 
    PlaylistCommandsBuilder_getCommandSubObject( aSubMenuId, aIndex, aHost )
  {
    return this._getCommandProperty(aSubMenuId, 
                                    aIndex, 
                                    aHost, 
                                    "m_CommandSubObject", 
                                    null);
  },

  onCommand: function 
    PlaylistCommandsBuilder_onCommand(aSubMenuId, aIndex, aHost, id, value)
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
  
  duplicate: function PlaylistCommandsBuilder_duplicate()
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
    obj.m_Context = this.dupObject(this.m_Context);
        
    // return the copy
    return obj;
  },
  
  dupObject: function PlaylistCommandsBuilder_dupObject(obj) {
    var r = {};
    for ( var i in obj )
    {
      r[ i ] = obj[ i ];
    }
    return r;
  },

  initCommands: function PlaylistCommandsBuilder_initCommands(aHost) { 
    for (var i=0;i<this.m_menus.length;i++) {
      for (var j=0;j<this.m_menus[i].m_Menu.length;j++) {
        if (this.m_menus[i].m_Menu[j].m_CommandSubObject) {
          this.m_menus[i].m_Menu[j].m_CommandSubObject.initCommands(aHost);
        }
      }
    }
    if (this.m_InitCallback &&
        typeof(this.m_InitCallback) == "object" && 
        this.m_InitCallback.handleCallback) {
      this.m_InitCallback.handleCallback(this.m_Context, aHost, null);
    }
  },

  shutdownCommands: function PlaylistCommandsBuilder_shutdownCommands() { 
    for (var i=0;i<this.m_menus.length;i++) {
      for (var j=0;j<this.m_menus[i].m_Menu.length;j++) {
        if (this.m_menus[i].m_Menu[j].m_CommandSubObject) {
          this.m_menus[i].m_Menu[j].m_CommandSubObject.shutdownCommands();
        }
      }
    }
    if (this.m_ShutdownCallback &&
        typeof(this.m_ShutdownCallback) == "object" && 
        this.m_ShutdownCallback.handleCallback) {
      this.m_ShutdownCallback.handleCallback(this.m_Context, null, null);
    }
    this.shutdown();
  },
  
  setContext: function PlaylistCommandsBuilder_setContext( context )
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

  LOG: function PlaylistCommandsBuilder_LOG(str) {
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

