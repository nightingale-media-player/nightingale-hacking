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
 * \brief Playlist Commands Unit Test File
 */

// ============================================================================
// GLOBALS & CONSTS
// ============================================================================
 
var triggered = false; 
var triggeredval = null;
var refreshed = false;
var initialized = false;
var shutdown = false;

var dummyNode = {
  QueryInterface  : function(iid) {
    if (iid.equals(Components.interfaces.nsIDOMNode) ||
        iid.equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
};

var dummyDocument = {
  QueryInterface  : function(iid) {
    if (iid.equals(Components.interfaces.nsIDOMDocument) ||
        iid.equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
};

const NUMRECURSE = 2;

const PlaylistCommandsBuilder = new Components.
  Constructor("@songbirdnest.com/Songbird/PlaylistCommandsBuilder;1", 
              "sbIPlaylistCommandsBuilder");

// ============================================================================
// ENTRY POINT
// ============================================================================

function runTest () {

  log("Testing Playlist Commands:");
  testCommandsBuilder();
  log("OK");
}

// ============================================================================
// TESTS
// ============================================================================

function testCommandsBuilder() {

  var builder = new PlaylistCommandsBuilder();
  
  testAppendInsertRemove(builder, null);
  testSubmenus(builder, null, NUMRECURSE);
  testInitShutdownCB(builder);
  testVisibleCB(builder);
  
  builder.shutdown();
}

function testSubmenus(builder, menuid, recursecount) {
  _log("Testing PlaylistCommandsBuilder - appendSubmenu", menuid);
  builder.appendSubmenu(menuid, "submenu1id", "Submenu 1 Label", "Submenu 1 Tooltip");
  builder.appendSubmenu(menuid, "submenu2id", "Submenu 2 Label", "Submenu 2 Tooltip");
  builder.appendSubmenu(menuid, "submenu3id", "Submenu 3 Label", "Submenu 3 Tooltip");
  builder.appendSubmenu(menuid, "submenu4id", "Submenu 4 Label", "Submenu 4 Tooltip");
  var commands = builder.QueryInterface(Components.interfaces.sbIPlaylistCommands);
  assertNumCommands(commands, menuid, "test", 4);
  assertCommand(commands, 0, menuid, "submenu", "submenu1id", "Submenu 1 Label", "Submenu 1 Tooltip", "test");
  assertCommand(commands, 1, menuid, "submenu", "submenu2id", "Submenu 2 Label", "Submenu 2 Tooltip", "test");
  assertCommand(commands, 2, menuid, "submenu", "submenu3id", "Submenu 3 Label", "Submenu 3 Tooltip", "test");
  assertCommand(commands, 3, menuid, "submenu", "submenu4id", "Submenu 4 Label", "Submenu 4 Tooltip", "test");
  builder.removeAllCommands(menuid);

  _log("Testing PlaylistCommandsBuilder - insertSubmenuBefore/insertSubmenuAfter", menuid);
  builder.appendSubmenu(menuid, "submenu3id", "Submenu 3 Label", "Submenu 3 Tooltip");
  builder.insertSubmenuBefore(menuid, "submenu3id", "submenu1id", "Submenu 1 Label", "Submenu 1 Tooltip");
  builder.insertSubmenuAfter(menuid, "submenu1id", "submenu2id", "Submenu 2 Label", "Submenu 2 Tooltip");
  builder.insertSubmenuAfter(menuid, "submenu3id", "submenu4id", "Submenu 4 Label", "Submenu 4 Tooltip");
  assertNumCommands(commands, menuid, "test", 4);
  assertCommand(commands, 0, menuid, "submenu", "submenu1id", "Submenu 1 Label", "Submenu 1 Tooltip", "test");
  assertCommand(commands, 1, menuid, "submenu", "submenu2id", "Submenu 2 Label", "Submenu 2 Tooltip", "test");
  assertCommand(commands, 2, menuid, "submenu", "submenu3id", "Submenu 3 Label", "Submenu 3 Tooltip", "test");
  assertCommand(commands, 3, menuid, "submenu", "submenu4id", "Submenu 4 Label", "Submenu 4 Tooltip", "test");
  builder.removeAllCommands(menuid);

  var submenuid = "submenutestid" + (NUMRECURSE-recursecount+1);
  builder.appendSubmenu(menuid, submenuid, "Submenu Test Label", "Submenu Test Tooltip");
  if (recursecount > 0) {
    testSubmenus(builder, submenuid, recursecount-1);
  }
  testAppendInsertRemove(builder, submenuid);
  builder.removeCommand(menuid, submenuid);
}

function testAppendInsertRemove(builder, menuid) {
  // ACTIONS
  _log("Testing PlaylistCommandsBuilder - appendAction", menuid);
  builder.appendAction(menuid, "action1id", "Action 1 Label", "Action 1 Tooltip", action1_ontrigger);
  builder.appendAction(menuid, "action2id", "Action 2 Label", "Action 2 Tooltip", action2_ontrigger);
  builder.appendAction(menuid, "action3id", "Action 3 Label", "Action 3 Tooltip", action3_ontrigger);
  builder.appendAction(menuid, "action4id", "Action 4 Label", "Action 4 Tooltip", action4_ontrigger);
  var commands = builder.QueryInterface(Components.interfaces.sbIPlaylistCommands);
  assertNumCommands(commands, menuid, "test", 4);
  assertCommand(commands, 0, menuid, "action", "action1id", "Action 1 Label", "Action 1 Tooltip", "test");
  assertCommand(commands, 1, menuid, "action", "action2id", "Action 2 Label", "Action 2 Tooltip", "test");
  assertCommand(commands, 2, menuid, "action", "action3id", "Action 3 Label", "Action 3 Tooltip", "test");
  assertCommand(commands, 3, menuid, "action", "action4id", "Action 4 Label", "Action 4 Tooltip", "test");
  
  _log("Testing PlaylistCommandsBuilder - removeCommand", menuid);
  builder.removeCommand(menuid, "action1id");
  assertNumCommands(commands, menuid, "test", 3);
  assertCommand(commands, 0, menuid, "action", "action2id", "Action 2 Label", "Action 2 Tooltip", "test");
  builder.removeCommand(menuid, "action3id");
  assertNumCommands(commands, menuid, "test", 2);
  assertCommand(commands, 1, menuid, "action", "action4id", "Action 4 Label", "Action 4 Tooltip", "test");

  _log("Testing PlaylistCommandsBuilder - removeAllCommands", menuid);
  builder.removeAllCommands(menuid);
  assertNumCommands(commands, menuid, "test", 0);

  _log("Testing PlaylistCommandsBuilder - insertActionBefore/insertActionAfter", menuid);
  builder.appendAction(menuid, "action3id", "Action 3 Label", "Action 3 Tooltip", action3_ontrigger);
  builder.insertActionBefore(menuid, "action3id", "action1id", "Action 1 Label", "Action 1 Tooltip", action1_ontrigger);
  builder.insertActionAfter(menuid, "action1id", "action2id", "Action 2 Label", "Action 2 Tooltip", action2_ontrigger);
  builder.insertActionAfter(menuid, "action3id", "action4id", "Action 4 Label", "Action 4 Tooltip", action4_ontrigger);
  assertNumCommands(commands, menuid, "test", 4);
  assertCommand(commands, 0, menuid, "action", "action1id", "Action 1 Label", "Action 1 Tooltip", "test");
  assertCommand(commands, 1, menuid, "action", "action2id", "Action 2 Label", "Action 2 Tooltip", "test");
  assertCommand(commands, 2, menuid, "action", "action3id", "Action 3 Label", "Action 3 Tooltip", "test");
  assertCommand(commands, 3, menuid, "action", "action4id", "Action 4 Label", "Action 4 Tooltip", "test");

  _log("Testing PlaylistCommandsBuilder - setCommandVisibleCallback", menuid);
  builder.setCommandVisibleCallback(menuid, "action1id", action1_getvisible);
  builder.setCommandVisibleCallback(menuid, "action2id", action2_getvisible);
  builder.setCommandVisibleCallback(menuid, "action3id", action3_getvisible);
  builder.setCommandVisibleCallback(menuid, "action4id", action4_getvisible);
  assertVisible(commands, menuid, 0, "test", "action1id", true);
  assertVisible(commands, menuid, 1, "test", "action2id", false);
  assertVisible(commands, menuid, 2, "test", "action3id", true);
  assertVisible(commands, menuid, 3, "test", "action4id", false);

  _log("Testing PlaylistCommandsBuilder - setCommandEnabledCallback", menuid);
  builder.setCommandEnabledCallback(menuid, "action1id", action1_getenabled);
  builder.setCommandEnabledCallback(menuid, "action2id", action2_getenabled);
  builder.setCommandEnabledCallback(menuid, "action3id", action3_getenabled);
  builder.setCommandEnabledCallback(menuid, "action4id", action4_getenabled);
  assertEnabled(commands, menuid, 0, "test", "action1id", false);
  assertEnabled(commands, menuid, 1, "test", "action2id", true);
  assertEnabled(commands, menuid, 2, "test", "action3id", false);
  assertEnabled(commands, menuid, 3, "test", "action4id", true);

  _log("Testing PlaylistCommandsBuilder - setCommandShortcuts", menuid);
  builder.setCommandShortcut(menuid, "action1id", "key1", "keycode1", "modifiers1", true);
  builder.setCommandShortcut(menuid, "action2id", "key2", "keycode2", "modifiers2", false);
  builder.setCommandShortcut(menuid, "action3id", "key3", "keycode3", "modifiers3", true);
  builder.setCommandShortcut(menuid, "action4id", "key4", "keycode4", "modifiers4", false);
  assertShortcut(commands, menuid, 0, "test", "action1id", "key1", "keycode1", "modifiers1", true);
  assertShortcut(commands, menuid, 1, "test", "action2id", "key2", "keycode2", "modifiers2", false);
  assertShortcut(commands, menuid, 2, "test", "action3id", "key3", "keycode3", "modifiers3", true);
  assertShortcut(commands, menuid, 3, "test", "action4id", "key4", "keycode4", "modifiers4", false);

  _log("Testing PlaylistCommandsBuilder - onCommand for Actions", menuid);
  assertTrigger(commands, menuid, 0, "test", "action1id", null);
  assertTrigger(commands, menuid, 1, "test", "action2id", null);
  assertTrigger(commands, menuid, 2, "test", "action3id", null);
  assertTrigger(commands, menuid, 3, "test", "action4id", null);
  builder.removeAllCommands(menuid);

  // SEPARATORS
  _log("Testing PlaylistCommandsBuilder - appendSeparator", menuid);
  builder.appendSeparator(menuid, "sep1id");
  builder.appendSeparator(menuid, "sep2id");
  builder.appendSeparator(menuid, "sep3id");
  builder.appendSeparator(menuid, "sep4id");
  assertNumCommands(commands, menuid, "test", 4);
  assertCommand(commands, 0, menuid, "separator", "sep1id", "*separator*", "", "test");
  assertCommand(commands, 1, menuid, "separator", "sep2id", "*separator*", "", "test");
  assertCommand(commands, 2, menuid, "separator", "sep3id", "*separator*", "", "test");
  assertCommand(commands, 3, menuid, "separator", "sep4id", "*separator*", "", "test");
  builder.removeAllCommands(menuid);

  _log("Testing PlaylistCommandsBuilder - insertSeparatorBefore/insertSeparatorAfter", menuid);
  builder.appendSeparator(menuid, "sep3id");
  builder.insertSeparatorBefore(menuid, "sep3id", "sep1id");
  builder.insertSeparatorAfter(menuid, "sep1id", "sep2id");
  builder.insertSeparatorAfter(menuid, "sep3id", "sep4id");
  assertNumCommands(commands, menuid, "test", 4);
  assertCommand(commands, 0, menuid, "separator", "sep1id", "*separator*", "", "test");
  assertCommand(commands, 1, menuid, "separator", "sep2id", "*separator*", "", "test");
  assertCommand(commands, 2, menuid, "separator", "sep3id", "*separator*", "", "test");
  assertCommand(commands, 3, menuid, "separator", "sep4id", "*separator*", "", "test");
  builder.removeAllCommands(menuid);

  // FLAGS
  _log("Testing PlaylistCommandsBuilder - appendFlag", menuid);
  builder.appendFlag(menuid, "flag1id", "Flag 1 Label", "Flag 1 Tooltip", flag1_ontrigger, flag1_ongetvalue);
  builder.appendFlag(menuid, "flag2id", "Flag 2 Label", "Flag 2 Tooltip", flag2_ontrigger, flag2_ongetvalue);
  builder.appendFlag(menuid, "flag3id", "Flag 3 Label", "Flag 3 Tooltip", flag3_ontrigger, flag3_ongetvalue);
  builder.appendFlag(menuid, "flag4id", "Flag 4 Label", "Flag 4 Tooltip", flag4_ontrigger, flag4_ongetvalue);
  assertNumCommands(commands, menuid, "test", 4);
  assertCommand(commands, 0, menuid, "flag", "flag1id", "Flag 1 Label", "Flag 1 Tooltip", "test");
  assertCommand(commands, 1, menuid, "flag", "flag2id", "Flag 2 Label", "Flag 2 Tooltip", "test");
  assertCommand(commands, 2, menuid, "flag", "flag3id", "Flag 3 Label", "Flag 3 Tooltip", "test");
  assertCommand(commands, 3, menuid, "flag", "flag4id", "Flag 4 Label", "Flag 4 Tooltip", "test");
  builder.removeAllCommands(menuid);

  _log("Testing PlaylistCommandsBuilder - insertFlagBefore/insertFlagAfter", menuid);
  builder.appendFlag(menuid, "flag3id", "Flag 3 Label", "Flag 3 Tooltip", flag3_ontrigger, flag3_ongetvalue);
  builder.insertFlagBefore(menuid, "flag3id", "flag1id", "Flag 1 Label", "Flag 1 Tooltip", flag1_ontrigger, flag1_ongetvalue);
  builder.insertFlagAfter(menuid, "flag1id", "flag2id", "Flag 2 Label", "Flag 2 Tooltip", flag2_ontrigger, flag2_ongetvalue);
  builder.insertFlagAfter(menuid, "flag3id", "flag4id", "Flag 4 Label", "Flag 4 Tooltip", flag4_ontrigger, flag4_ongetvalue);
  assertNumCommands(commands, menuid, "test", 4);
  assertCommand(commands, 0, menuid, "flag", "flag1id", "Flag 1 Label", "Flag 1 Tooltip", "test");
  assertCommand(commands, 1, menuid, "flag", "flag2id", "Flag 2 Label", "Flag 2 Tooltip", "test");
  assertCommand(commands, 2, menuid, "flag", "flag3id", "Flag 3 Label", "Flag 3 Tooltip", "test");
  assertCommand(commands, 3, menuid, "flag", "flag4id", "Flag 4 Label", "Flag 4 Tooltip", "test");

  _log("Testing PlaylistCommandsBuilder - onCommand for Flags", menuid);
  assertTrigger(commands, menuid, 0, "test", "flag1id", null);
  assertTrigger(commands, menuid, 1, "test", "flag2id", null);
  assertTrigger(commands, menuid, 2, "test", "flag3id", null);
  assertTrigger(commands, menuid, 3, "test", "flag4id", null);
  
  _log("Testing PlaylistCommandsBuilder - getCommandFlag", menuid);
  assertGetFlag(commands, menuid, 0, "test", "flag1id", true);
  assertGetFlag(commands, menuid, 1, "test", "flag2id", false);
  assertGetFlag(commands, menuid, 2, "test", "flag3id", true);
  assertGetFlag(commands, menuid, 3, "test", "flag4id", false);

  builder.removeAllCommands(menuid);

  // VALUES
  _log("Testing PlaylistCommandsBuilder - appendValue", menuid);
  builder.appendValue(menuid, "value1id", "Value 1 Label", "Value 1 Tooltip", value1_onsetvalue, value1_ongetvalue);
  builder.appendValue(menuid, "value2id", "Value 2 Label", "Value 2 Tooltip", value2_onsetvalue, value1_ongetvalue);
  builder.appendValue(menuid, "value3id", "Value 3 Label", "Value 3 Tooltip", value3_onsetvalue, value1_ongetvalue);
  builder.appendValue(menuid, "value4id", "Value 4 Label", "Value 4 Tooltip", value4_onsetvalue, value1_ongetvalue);
  assertNumCommands(commands, menuid, "test", 4);
  assertCommand(commands, 0, menuid, "value", "value1id", "Value 1 Label", "Value 1 Tooltip", "test");
  assertCommand(commands, 1, menuid, "value", "value2id", "Value 2 Label", "Value 2 Tooltip", "test");
  assertCommand(commands, 2, menuid, "value", "value3id", "Value 3 Label", "Value 3 Tooltip", "test");
  assertCommand(commands, 3, menuid, "value", "value4id", "Value 4 Label", "Value 4 Tooltip", "test");
  builder.removeAllCommands(menuid);

  _log("Testing PlaylistCommandsBuilder - insertValueBefore/insertValueAfter", menuid);
  builder.appendValue(menuid, "value3id", "Value 3 Label", "Value 3 Tooltip", value3_onsetvalue, value3_ongetvalue);
  builder.insertValueBefore(menuid, "value3id", "value1id", "Value 1 Label", "Value 1 Tooltip", value1_onsetvalue, value1_ongetvalue);
  builder.insertValueAfter(menuid, "value1id", "value2id", "Value 2 Label", "Value 2 Tooltip", value2_onsetvalue, value2_ongetvalue);
  builder.insertValueAfter(menuid, "value3id", "value4id", "Value 4 Label", "Value 4 Tooltip", value4_onsetvalue, value4_ongetvalue);
  assertNumCommands(commands, menuid, "test", 4);
  assertCommand(commands, 0, menuid, "value", "value1id", "Value 1 Label", "Value 1 Tooltip", "test");
  assertCommand(commands, 1, menuid, "value", "value2id", "Value 2 Label", "Value 2 Tooltip", "test");
  assertCommand(commands, 2, menuid, "value", "value3id", "Value 3 Label", "Value 3 Tooltip", "test");
  assertCommand(commands, 3, menuid, "value", "value4id", "Value 4 Label", "Value 4 Tooltip", "test");

  _log("Testing PlaylistCommandsBuilder - onCommand for Values", menuid);
  assertTrigger(commands, menuid, 0, "test", "value1id", "value1");
  assertTrigger(commands, menuid, 1, "test", "value2id", "value2");
  assertTrigger(commands, menuid, 2, "test", "value3id", "value3");
  assertTrigger(commands, menuid, 3, "test", "value4id", "value4");
  
  _log("Testing PlaylistCommandsBuilder - getCommandValue", menuid);
  assertGetValue(commands, menuid, 0, "test", "value1id", "value1");
  assertGetValue(commands, menuid, 1, "test", "value2id", "value2");
  assertGetValue(commands, menuid, 2, "test", "value3id", "value3");
  assertGetValue(commands, menuid, 3, "test", "value4id", "value4");
  builder.removeAllCommands(menuid);
  
  // CHOICE MENUS

  _log("Testing PlaylistCommandsBuilder - appendChoiceMenu", menuid);
  builder.appendChoiceMenu(menuid, "choicemenu1id", "Choicemenu 1 Label", "Choicemenu 1 Tooltip", choicemenu_ongetitem);
  builder.appendChoiceMenu(menuid, "choicemenu2id", "Choicemenu 2 Label", "Choicemenu 2 Tooltip", choicemenu_ongetitem);
  builder.appendChoiceMenu(menuid, "choicemenu3id", "Choicemenu 3 Label", "Choicemenu 3 Tooltip", choicemenu_ongetitem);
  builder.appendChoiceMenu(menuid, "choicemenu4id", "Choicemenu 4 Label", "Choicemenu 4 Tooltip", choicemenu_ongetitem);
  assertNumCommands(commands, menuid, "test", 4);
  assertCommand(commands, 0, menuid, "choice", "choicemenu1id", "Choicemenu 1 Label", "Choicemenu 1 Tooltip", "test");
  assertCommand(commands, 1, menuid, "choice", "choicemenu2id", "Choicemenu 2 Label", "Choicemenu 2 Tooltip", "test");
  assertCommand(commands, 2, menuid, "choice", "choicemenu3id", "Choicemenu 3 Label", "Choicemenu 3 Tooltip", "test");
  assertCommand(commands, 3, menuid, "choice", "choicemenu4id", "Choicemenu 4 Label", "Choicemenu 4 Tooltip", "test");
  builder.removeAllCommands(menuid);

  _log("Testing PlaylistCommandsBuilder - insertChoicemenuBefore/insertChoicemenuAfter", menuid);
  builder.appendChoiceMenu(menuid, "choicemenu3id", "Choicemenu 3 Label", "Choicemenu 3 Tooltip", choicemenu_ongetitem);
  builder.insertChoiceMenuBefore(menuid, "choicemenu3id", "choicemenu1id", "Choicemenu 1 Label", "Choicemenu 1 Tooltip", choicemenu_ongetitem);
  builder.insertChoiceMenuAfter(menuid, "choicemenu1id", "choicemenu2id", "Choicemenu 2 Label", "Choicemenu 2 Tooltip", choicemenu_ongetitem);
  builder.insertChoiceMenuAfter(menuid, "choicemenu3id", "choicemenu4id", "Choicemenu 4 Label", "Choicemenu 4 Tooltip", choicemenu_ongetitem);
  assertNumCommands(commands, menuid, "test", 4);
  assertCommand(commands, 0, menuid, "choice", "choicemenu1id", "Choicemenu 1 Label", "Choicemenu 1 Tooltip", "test");
  assertCommand(commands, 1, menuid, "choice", "choicemenu2id", "Choicemenu 2 Label", "Choicemenu 2 Tooltip", "test");
  assertCommand(commands, 2, menuid, "choice", "choicemenu3id", "Choicemenu 3 Label", "Choicemenu 3 Tooltip", "test");
  assertCommand(commands, 3, menuid, "choice", "choicemenu4id", "Choicemenu 4 Label", "Choicemenu 4 Tooltip", "test");
  builder.removeAllCommands(menuid);

  // CHOICE MENUS ITEMS
  _log("Testing PlaylistCommandsBuilder - appendChoiceMenuItem", menuid);
  builder.appendChoiceMenu(menuid, "choicemenu", "Choicemenu Label", "Choicemenu Tooltip", choicemenu_ongetitem);
  var _menu = "choicemenu";
  builder.appendChoiceMenuItem(_menu, "choiceitem1id", "ChoiceMenuItem 1 Label", "ChoiceMenuItem 1 Tooltip", choiceitem1_ontrigger);
  builder.appendChoiceMenuItem(_menu, "choiceitem2id", "ChoiceMenuItem 2 Label", "ChoiceMenuItem 2 Tooltip", choiceitem2_ontrigger);
  builder.appendChoiceMenuItem(_menu, "choiceitem3id", "ChoiceMenuItem 3 Label", "ChoiceMenuItem 3 Tooltip", choiceitem3_ontrigger);
  builder.appendChoiceMenuItem(_menu, "choiceitem4id", "ChoiceMenuItem 4 Label", "ChoiceMenuItem 4 Tooltip", choiceitem4_ontrigger);
  assertNumCommands(commands, _menu, "test", 4);
  assertCommand(commands, 0, _menu, "choiceitem", "choiceitem1id", "ChoiceMenuItem 1 Label", "ChoiceMenuItem 1 Tooltip", "test");
  assertCommand(commands, 1, _menu, "choiceitem", "choiceitem2id", "ChoiceMenuItem 2 Label", "ChoiceMenuItem 2 Tooltip", "test");
  assertCommand(commands, 2, _menu, "choiceitem", "choiceitem3id", "ChoiceMenuItem 3 Label", "ChoiceMenuItem 3 Tooltip", "test");
  assertCommand(commands, 3, _menu, "choiceitem", "choiceitem4id", "ChoiceMenuItem 4 Label", "ChoiceMenuItem 4 Tooltip", "test");
  builder.removeAllCommands(_menu);

  _log("Testing PlaylistCommandsBuilder - insertChoiceMenuItemBefore/insertChoiceMenuItemAfter", menuid);
  builder.appendChoiceMenuItem(_menu, "choiceitem3id", "ChoiceMenuItem 3 Label", "ChoiceMenuItem 3 Tooltip", choiceitem3_ontrigger);
  builder.insertChoiceMenuItemBefore(_menu, "choiceitem3id", "choiceitem1id", "ChoiceMenuItem 1 Label", "ChoiceMenuItem 1 Tooltip", choiceitem1_ontrigger);
  builder.insertChoiceMenuItemAfter(_menu, "choiceitem1id", "choiceitem2id", "ChoiceMenuItem 2 Label", "ChoiceMenuItem 2 Tooltip", choiceitem2_ontrigger);
  builder.insertChoiceMenuItemAfter(_menu, "choiceitem3id", "choiceitem4id", "ChoiceMenuItem 4 Label", "ChoiceMenuItem 4 Tooltip", choiceitem4_ontrigger);
  assertNumCommands(commands, _menu, "test", 4);
  assertCommand(commands, 0, _menu, "choiceitem", "choiceitem1id", "ChoiceMenuItem 1 Label", "ChoiceMenuItem 1 Tooltip", "test");
  assertCommand(commands, 1, _menu, "choiceitem", "choiceitem2id", "ChoiceMenuItem 2 Label", "ChoiceMenuItem 2 Tooltip", "test");
  assertCommand(commands, 2, _menu, "choiceitem", "choiceitem3id", "ChoiceMenuItem 3 Label", "ChoiceMenuItem 3 Tooltip", "test");
  assertCommand(commands, 3, _menu, "choiceitem", "choiceitem4id", "ChoiceMenuItem 4 Label", "ChoiceMenuItem 4 Tooltip", "test");

  _log("Testing PlaylistCommandsBuilder - onCommand for ChoiceMenuItems", menuid);
  assertTrigger(commands, _menu, 0, "test", "choiceitem1id", null);
  assertTrigger(commands, _menu, 1, "test", "choiceitem2id", null);
  assertTrigger(commands, _menu, 2, "test", "choiceitem3id", null);
  assertTrigger(commands, _menu, 3, "test", "choiceitem4id", null);

  _log("Testing PlaylistCommandsBuilder - getCommandChoiceItem", menuid);
  assertGetChoiceItem(commands, "choicemenu", "test", "choiceitem1id");

  builder.removeAllCommands(menuid);

  // CUSTOM ITEMS
  builder.appendCustomItem(menuid, "customitem1id", customitem1_oninstantiate, customitem1_onrefresh);
  builder.appendCustomItem(menuid, "customitem2id", customitem2_oninstantiate, customitem2_onrefresh);
  builder.appendCustomItem(menuid, "customitem3id", customitem3_oninstantiate, customitem3_onrefresh);
  builder.appendCustomItem(menuid, "customitem4id", customitem4_oninstantiate, customitem4_onrefresh);
  assertNumCommands(commands, menuid, "test", 4);
  assertCommand(commands, 0, menuid, "custom", "customitem1id", "", "", "test");
  assertCommand(commands, 1, menuid, "custom", "customitem2id", "", "", "test");
  assertCommand(commands, 2, menuid, "custom", "customitem3id", "", "", "test");
  assertCommand(commands, 3, menuid, "custom", "customitem4id", "", "", "test");
  builder.removeAllCommands(menuid);
  
  _log("Testing PlaylistCommandsBuilder - insertCustomItemBefore/insertCustomItemAfter", menuid);
  builder.appendCustomItem(menuid, "customitem3id", customitem3_oninstantiate, customitem3_onrefresh);
  builder.insertCustomItemBefore(menuid, "customitem3id", "customitem1id", customitem1_oninstantiate, customitem1_onrefresh);
  builder.insertCustomItemAfter(menuid, "customitem1id", "customitem2id", customitem2_oninstantiate, customitem2_onrefresh);
  builder.insertCustomItemAfter(menuid, "customitem3id", "customitem4id", customitem4_oninstantiate, customitem4_onrefresh);
  assertNumCommands(commands, menuid, "test", 4);
  assertCommand(commands, 0, menuid, "custom", "customitem1id", "", "", "test");
  assertCommand(commands, 1, menuid, "custom", "customitem2id", "", "", "test");
  assertCommand(commands, 2, menuid, "custom", "customitem3id", "", "", "test");
  assertCommand(commands, 3, menuid, "custom", "customitem4id", "", "", "test");

  _log("Testing PlaylistCommandsBuilder - instantiateCustomCommand", menuid);
  assertInstantiate(commands, menuid, 0, "test", "customitem1id", dummyDocument);
  assertInstantiate(commands, menuid, 1, "test", "customitem2id", dummyDocument);
  assertInstantiate(commands, menuid, 2, "test", "customitem3id", dummyDocument);
  assertInstantiate(commands, menuid, 3, "test", "customitem4id", dummyDocument);
  
  _log("Testing PlaylistCommandsBuilder - refreshCustomCommand", menuid);
  assertRefreshed(commands, menuid, 0, "test", "customitem1id", dummyNode);
  assertRefreshed(commands, menuid, 1, "test", "customitem2id", dummyNode);
  assertRefreshed(commands, menuid, 2, "test", "customitem3id", dummyNode);
  assertRefreshed(commands, menuid, 3, "test", "customitem4id", dummyNode);
  builder.removeAllCommands(menuid);
  
  // SUB PLAYLISTCOMMANDS
  _log("Testing PlaylistCommandsBuilder - appendPlaylistCommands", menuid);
  var secbuilder1 = new PlaylistCommandsBuilder();
  var secbuilder2 = new PlaylistCommandsBuilder();
  var secbuilder3 = new PlaylistCommandsBuilder();
  var secbuilder4 = new PlaylistCommandsBuilder();
  builder.appendPlaylistCommands(menuid, "seccommand1id", secbuilder1);
  builder.appendPlaylistCommands(menuid, "seccommand2id", secbuilder2);
  builder.appendPlaylistCommands(menuid, "seccommand3id", secbuilder3);
  builder.appendPlaylistCommands(menuid, "seccommand4id", secbuilder4);
  assertNumCommands(commands, menuid, "test", 4);
  builder.removeAllCommands(menuid);

  secbuilder1.appendAction(null, "secbuilder1action", "Secondary Builder 1 Action Label", "Secondary Builder 1 Action Tooltip", action1_ontrigger);
  secbuilder2.appendAction(null, "secbuilder2action", "Secondary Builder 2 Action Label", "Secondary Builder 2 Action Tooltip", action2_ontrigger);
  secbuilder3.appendAction(null, "secbuilder3action", "Secondary Builder 3 Action Label", "Secondary Builder 3 Action Tooltip", action3_ontrigger);
  secbuilder4.appendAction(null, "secbuilder4action", "Secondary Builder 4 Action Label", "Secondary Builder 4 Action Tooltip", action4_ontrigger);

  _log("Testing PlaylistCommandsBuilder - insertPlaylistCommandsBefore/insertPlaylistCommandsAfter", menuid);
  builder.appendPlaylistCommands(menuid, "seccommand3id", secbuilder3);
  builder.insertPlaylistCommandsBefore(menuid, "seccommand3id", "seccommand1id", secbuilder1);
  builder.insertPlaylistCommandsAfter(menuid, "seccommand1id", "seccommand2id", secbuilder2);
  builder.insertPlaylistCommandsAfter(menuid, "seccommand3id", "seccommand4id", secbuilder4);
  assertNumCommands(commands, menuid, "test", 4);

  assertSubCommand(commands, 0, menuid, "action", "secbuilder1action", "Secondary Builder 1 Action Label", "Secondary Builder 1 Action Tooltip", "test");
  assertSubCommand(commands, 1, menuid, "action", "secbuilder2action", "Secondary Builder 2 Action Label", "Secondary Builder 2 Action Tooltip", "test");
  assertSubCommand(commands, 2, menuid, "action", "secbuilder3action", "Secondary Builder 3 Action Label", "Secondary Builder 3 Action Tooltip", "test");
  assertSubCommand(commands, 3, menuid, "action", "secbuilder4action", "Secondary Builder 4 Action Label", "Secondary Builder 4 Action Tooltip", "test");
  secbuilder1.shutdown();
  secbuilder2.shutdown();
  secbuilder3.shutdown();
  secbuilder4.shutdown();

  builder.removeAllCommands(menuid);
}

function testInitShutdownCB(builder) {
  builder.setInitCallback(cmds_init);
  builder.setShutdownCallback(cmds_shutdown);
  initialized = false;
  shutdown = false;
  var commands = builder
                  .QueryInterface(Components.interfaces.sbIPlaylistCommands)
                  .duplicate();
  commands.initCommands("");
  assertBool(initialized, "initCommands did not call init callback!");
  commands.shutdownCommands();
  assertBool(shutdown, "shutdownCommands did not call shutdown callback!");
}

function testVisibleCB(builder) {
  builder.setVisibleCallback(cmds_getvisible);
  var commands = builder.QueryInterface(Components.interfaces.sbIPlaylistCommands);
  var visible = commands.getVisible("test");
  assertBool(!visible, "getVisible should have returned false instead of " + visible + "!");
}

// ============================================================================
// COMMAND CALLBACKS
// ============================================================================

function cmds_init(context, host, data) {
  var implementorContext = {
    testString: "test",
    toString: function() {
      return this.testString;
    },
    QueryInterface: function(iid) {
      if (iid.equals(Ci.nsISupportsString) ||
          iid.equals(Ci.nsISupports))
        return this;
      throw Cr.NS_NOINTERFACE;
    }
  };
  context.implementorContext = implementorContext;
  initialized = true;
}

function cmds_shutdown(context, host, data) {
  assertBool((context != null), "shutdown callback not provided context");
  assertBool((context.implementorContext != null),
             "shutdown callback not provided implementor context");
  var testString =
                context.implementorContext.QueryInterface(Ci.nsISupportsString);
  assertValue(testString.toString(),
              "test",
              "shutdown callback implementor context invalid");
  context.implementorContext = null;
  shutdown = true;
}

function cmds_getvisible(context, host, data) {
  return false;
}

function action1_ontrigger(context, submenu, commandid, host) {
  triggered = true;
  assertValue(commandid, "action1id", "CommandId should be action1id in action1_ontrigger, not " + commandid + "!");
}
function action2_ontrigger(context, submenu, commandid, host) {
  triggered = true;
  assertValue(commandid, "action2id", "CommandId should be action2id in action2_ontrigger, not " + commandid + "!");
}
function action3_ontrigger(context, submenu, commandid, host) {
  triggered = true;
  assertValue(commandid, "action3id", "CommandId should be action3id in action3_ontrigger, not " + commandid + "!");
}
function action4_ontrigger(context, submenu, commandid, host) {
  triggered = true;
  assertValue(commandid, "action4id", "CommandId should be action4id in action4_ontrigger, not " + commandid + "!");
}

function action1_getvisible(context, submenu, commandid, host) {
  assertValue(commandid, "action1id", "CommandId should be action1id in action1_getvisible, not " + commandid + "!");
  return true;
}

function action2_getvisible(context, submenu, commandid, host) {
  assertValue(commandid, "action2id", "CommandId should be action2id in action2_getvisible, not " + commandid + "!");
  return false;
}

function action3_getvisible(context, submenu, commandid, host) {
  assertValue(commandid, "action3id", "CommandId should be action3id in action3_getvisible, not " + commandid + "!");
  return true;
}

function action4_getvisible(context, submenu, commandid, host) {
  assertValue(commandid, "action4id", "CommandId should be action4id in action4_getvisible, not " + commandid + "!");
  return false;
}

function action1_getenabled(context, submenu, commandid, host) {
  assertValue(commandid, "action1id", "CommandId should be action1id in action1_getenabled, not " + commandid + "!");
  return false;
}

function action2_getenabled(context, submenu, commandid, host) {
  assertValue(commandid, "action2id", "CommandId should be action2id in action2_getenabled, not " + commandid + "!");
  return true;
}

function action3_getenabled(context, submenu, commandid, host) {
  assertValue(commandid, "action3id", "CommandId should be action3id in action3_getenabled, not " + commandid + "!");
  return false;
}

function action4_getenabled(context, submenu, commandid, host) {
  assertValue(commandid, "action4id", "CommandId should be action4id in action4_getenabled, not " + commandid + "!");
  return true;
}

function flag1_ontrigger(context, submenu, commandid, host, val) {
  triggered = true;
  assertValue(commandid, "flag1id", "CommandId should be flag1id in flag1_ontrigger, not " + commandid + "!");
}
function flag2_ontrigger(context, submenu, commandid, host, val) {
  triggered = true;
  assertValue(commandid, "flag2id", "CommandId should be flag2id in flag2_ontrigger, not " + commandid + "!");
}
function flag3_ontrigger(context, submenu, commandid, host, val) {
  triggered = true;
  assertValue(commandid, "flag3id", "CommandId should be flag3id in flag3_ontrigger, not " + commandid + "!");
}
function flag4_ontrigger(context, submenu, commandid, host, val) {
  triggered = true;
  assertValue(commandid, "flag4id", "CommandId should be flag4id in flag4_ontrigger, not " + commandid + "!");
}
function flag1_ongetvalue(context, submenu, commandid, host, data) {
  assertValue(commandid, "flag1id", "CommandId should be flag1id in flag1_ongetvalue, not " + commandid + "!");
  return true;
}
function flag2_ongetvalue(context, submenu, commandid, host, data) {
  assertValue(commandid, "flag2id", "CommandId should be flag1id in flag2_ongetvalue, not " + commandid + "!");
  return false;
}
function flag3_ongetvalue(context, submenu, commandid, host, data) {
  assertValue(commandid, "flag3id", "CommandId should be flag1id in flag3_ongetvalue, not " + commandid + "!");
  return true;
}
function flag4_ongetvalue(context, submenu, commandid, host, data) {
  assertValue(commandid, "flag4id", "CommandId should be flag1id in flag4_ongetvalue, not " + commandid + "!");
  return false;
}

function value1_onsetvalue(context, submenu, commandid, host, val) {
  triggeredval = val;
  assertValue(commandid, "value1id", "CommandId should be value1id in value1_onsetvalue, not " + commandid + "!");
}
function value2_onsetvalue(context, submenu, commandid, host, val) {
  triggeredval = val;
  assertValue(commandid, "value2id", "CommandId should be value2id in value2_onsetvalue, not " + commandid + "!");
}
function value3_onsetvalue(context, submenu, commandid, host, val) {
  triggeredval = val;
  assertValue(commandid, "value3id", "CommandId should be value3id in value3_onsetvalue, not " + commandid + "!");
}
function value4_onsetvalue(context, submenu, commandid, host, val) {
  triggeredval = val;
  assertValue(commandid, "value4id", "CommandId should be value4id in value4_onsetvalue, not " + commandid + "!");
}
function value1_ongetvalue(context, submenu, commandid, host, data) {
  assertValue(commandid, "value1id", "CommandId should be value1id in value1_ongetvalue, not " + commandid + "!");
  return "value1";
}
function value2_ongetvalue(context, submenu, commandid, host, data) {
  assertValue(commandid, "value2id", "CommandId should be value1id in value2_ongetvalue, not " + commandid + "!");
  return "value2";
}
function value3_ongetvalue(context, submenu, commandid, host, data) {
  assertValue(commandid, "value3id", "CommandId should be value1id in value3_ongetvalue, not " + commandid + "!");
  return "value3";
}
function value4_ongetvalue(context, submenu, commandid, host, data) {
  assertValue(commandid, "value4id", "CommandId should be value1id in value4_ongetvalue, not " + commandid + "!");
  return "value4";
}

function choiceitem1_ontrigger(context, submenu, commandid, host, val) {
  triggered = true;
  assertValue(commandid, "choiceitem1id", "CommandId should be choiceitem1id in choiceitem1_ontrigger, not " + commandid + "!");
}
function choiceitem2_ontrigger(context, submenu, commandid, host, val) {
  triggered = true;
  assertValue(commandid, "choiceitem2id", "CommandId should be choiceitem2id in choiceitem2_ontrigger, not " + commandid + "!");
}
function choiceitem3_ontrigger(context, submenu, commandid, host, val) {
  triggered = true;
  assertValue(commandid, "choiceitem3id", "CommandId should be choiceitem3id in choiceitem3_ontrigger, not " + commandid + "!");
}
function choiceitem4_ontrigger(context, submenu, commandid, host, val) {
  triggered = true;
  assertValue(commandid, "choiceitem4id", "CommandId should be choiceitem4id in choiceitem4_ontrigger, not " + commandid + "!");
}

function choicemenu_ongetitem(context, submenu, commandid, host, val) {
  return "choiceitem1id";
}

function customitem1_oninstantiate(context, submenu, commandid, host, doc) {
  assertBool((doc != null), "instantiateCustomCommand did not forward the document!");
  assertValue(commandid, "customitem1id", "CommandId should be customitem1id in customitem1_oninstantiate, not " + commandid + "!");
  return dummyNode;
}
function customitem2_oninstantiate(context, submenu, commandid, host, doc) {
  assertBool((doc != null), "instantiateCustomCommand did not forward the document!");
  assertValue(commandid, "customitem2id", "CommandId should be customitem2id in customitem2_oninstantiate, not " + commandid + "!");
  return dummyNode;
}
function customitem3_oninstantiate(context, submenu, commandid, host, doc) {
  assertBool((doc != null), "instantiateCustomCommand did not forward the document!");
  assertValue(commandid, "customitem3id", "CommandId should be customitem3id in customitem3_oninstantiate, not " + commandid + "!");
  return dummyNode;
}
function customitem4_oninstantiate(context, submenu, commandid, host, doc) {
  assertBool((doc != null), "instantiateCustomCommand did not forward the document!");
  assertValue(commandid, "customitem4id", "CommandId should be customitem4id in customitem4_oninstantiate, not " + commandid + "!");
  return dummyNode;
}

function customitem1_onrefresh(context, submenu, commandid, host, element) {
  assertNode(element, "refreshCustomCommand failed to forward the element!");
  assertValue(commandid, "customitem1id", "CommandId should be customitem1id in customitem1_oninstantiate, not " + commandid + "!");
  refreshed = true;
}
function customitem2_onrefresh(context, submenu, commandid, host, element) {
  assertNode(element, "refreshCustomCommand failed to forward the element!");
  assertValue(commandid, "customitem2id", "CommandId should be customitem2id in customitem2_oninstantiate, not " + commandid + "!");
  refreshed = true;
}
function customitem3_onrefresh(context, submenu, commandid, host, element) {
  assertNode(element, "refreshCustomCommand failed to forward the element!");
  assertValue(commandid, "customitem3id", "CommandId should be customitem3id in customitem3_oninstantiate, not " + commandid + "!");
  refreshed = true;
}
function customitem4_onrefresh(context, submenu, commandid, host, element) {
  assertNode(element, "refreshCustomCommand failed to forward the element!");
  assertValue(commandid, "customitem4id", "CommandId should be customitem4id in customitem4_oninstantiate, not " + commandid + "!");
  refreshed = true;
}

// ============================================================================
// TEST FUNCTIONS
// ============================================================================

function assertEquals(s1, s2, msgprefix) {
  if (s1 != s2) {
    fail(msgprefix + " should be " + s2 + ", not " + s1 + "!");
  }
}

function assertNumCommands(cmds, menuid, host, num) {
  var actual = cmds.getNumCommands(menuid, host);
  if (actual != num) {
    fail("Number of commands should be " + num + ", not " + actual + "!");
  }
}

function assertCommand(cmds, index, menuid, type, id, label, tooltip, host) {
  assertEquals(cmds.getCommandType(menuid, index, host), type, "Type");
  assertEquals(cmds.getCommandId(menuid, index, host), id, "Id");
  assertEquals(cmds.getCommandText(menuid, index, host), label, "Label");
  assertEquals(cmds.getCommandToolTipText(menuid, index, host), tooltip, "Tooltip");
}

function assertBool(boolval, msg) {
  if (!boolval) {
    fail(msg);
  }
}

function assertValue(value, expected, msg) {
  if (value != expected) {
    fail(msg);
  }
}

function assertTrigger(cmds, menuid, index, host, id, val) {
  triggeredval = null;
  triggered = false;
  cmds.onCommand(menuid, index, host, id, val);
  if (!val) assertBool(triggered, "onCommand did not trigger the action!");
  if (val) assertValue(triggeredval, val, "onCommand did not forward the value for command " + id + "!");
}

function assertVisible(cmds, menuid, index, host, id, val) {
  var visible = cmds.getCommandVisible(menuid, index, host, id);
  assertValue(visible, val, "getCommandVisible should have returned " + val + " instead of " + visible + "!");
}

function assertEnabled(cmds, menuid, index, host, id, val) {
  var enabled = cmds.getCommandEnabled(menuid, index, host, id);
  assertValue(enabled, val, "getCommandEnabled should have returned " + val + " instead of " + enabled + "!");
}

function assertShortcut(cmds, menuid, index, host, id, key, code, mod, local) {
  var _key = cmds.getCommandShortcutKey(menuid, index, host);
  var _code = cmds.getCommandShortcutKeycode(menuid, index, host);
  var _mod = cmds.getCommandShortcutModifiers(menuid, index, host);
  var _local = cmds.getCommandShortcutLocal(menuid, index, host);
  assertValue(_key, key, "getCommandShortcutKey should have returned " + key + ", instead of " + _key + "!");
  assertValue(_code, code, "getCommandShortcutKeycode should have returned " + code + ", instead of " + _code + "!");
  assertValue(_mod, mod, "getCommandShortcutModifiers should have returned " + mod + ", instead of " + _mod + "!");
  assertValue(_local, local, "getCommandShortcutLocal should have returned " + local + ", instead of " + _local+ "!");
}


function assertGetFlag(cmds, menuid, index, host, id, val) {
  var getval = cmds.getCommandFlag(menuid, index, host, id, val);
  assertValue(getval, val, "getCommandFlag should have returned " + val + ", not " + getval);
}

function assertGetValue(cmds, menuid, index, host, id, val) {
  var getval = cmds.getCommandValue(menuid, index, host, id, val);
  assertValue(getval, val, "getCommandValue should have returned " + val + ", not " + getval);
}

function assertGetChoiceItem(cmds, menuid, host, val) {
  var getval = cmds.getCommandChoiceItem(menuid, host);
  assertValue(getval, val, "getCommandChoiceItem should have returned " + val + ", not " + getval);
}

function assertInstantiate(cmds, menuid, index, host, id, doc) {
  var element = cmds.instantiateCustomCommand(menuid, index, host, id, doc);
  assertNode(element, dummyNode, "instantiateCustomCommand should have returned dummyNode!");
}

function assertRefreshed(cmds, menuid, index, host, id, element) {
  refreshed = false;
  cmds.refreshCustomCommand(menuid, index, host, id, element);
  assertBool(refreshed, "refreshCustomCommand did not trigger a refresh!");
}

function assertNode(element, msg) {
  var el = element.QueryInterface(Components.interfaces.nsIDOMNode);
  assertBool((el != null), "refreshCustomCommand did not trigger a refresh!");
}

function assertSubCommand(cmds, index, menuid, type, id, label, tooltip, host) {
  var subcmd = cmds.getCommandSubObject(menuid, index, host);
  assertBool(subcmd != null, "getCommandSubObject should not have returned null!");
  assertEquals(subcmd.getCommandType(null, 0, host), type, "Type");
  assertEquals(subcmd.getCommandId(null, 0, host), id, "Id");
  assertEquals(subcmd.getCommandText(null, 0, host), label, "Label");
  assertEquals(subcmd.getCommandToolTipText(null, 0, host), tooltip, "Tooltip");
}

// ============================================================================
// UTILS
// ============================================================================

function _log(msg, menuid) {
  log(msg + (menuid ? " in submenus (" + menuid + ")" : "") + "...");
}
