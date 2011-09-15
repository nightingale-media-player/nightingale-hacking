/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2011 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

/**
 * \brief Playlist Commands Unit Test File
 */

// ============================================================================
// GLOBALS & CONSTS
// ============================================================================

const NUMRECURSE = 2;
const TESTLENGTH = 500;
const REGISTRATION_PARAM_COMBOS = [
  ["medialist", "guid", "servicepane"],
  ["medialist", "guid", "menu"],
  ["medialist", "type", "servicepane"],
  ["medialist", "type", "menu"],
  ["library", "guid", "servicepane"],
  ["library", "guid", "menu"]
]

const PlaylistCommandsBuilder = new Components.
  Constructor("@songbirdnest.com/Songbird/PlaylistCommandsBuilder;1",
              "sbIPlaylistCommandsBuilder", "init");

var gCmdMgr =
  Components.classes["@songbirdnest.com/Songbird/PlaylistCommandsManager;1"]
            .createInstance(Components.interfaces.sbIPlaylistCommandsManager);

var gTestLibrary = null;
var gPrimarySimpleList = null;
var gSecondarySimpleList = null;
var gSmartList = null;

/* An instruction construct that has all the information necessary to
 * perform, validate, and reverse a playlist command registration
 */
function RegistrationInstruction(aMediaListOrLibrary,
                                 aTypeOrGUID,
                                 aServicePaneOrMenu,
                                 aRegisterString,
                                 aCommand) {
  // Register this command to a medialist or a whole library?
  this.mediaListOrLibrary = aMediaListOrLibrary;

  // Register this command to a medialist by guid or type?
  // For library commands we always set this to 'guid'.
  this.typeOrGUID = aTypeOrGUID;

  // Register this command to appear in the 'menu', i.e. the mediaitem context
  // menu or toolbar, or the 'servicepane'?
  this.servicePaneOrMenu = aServicePaneOrMenu;

  // Depending on if guid or type was specified above this will contain the
  // guid or type to register to respectively
  this.registerString = aRegisterString;

  // The command to register
  this.command = aCommand;
}

RegistrationInstruction.prototype.toString = function() {
    var str = "Command '" + this.command.id + "' to '" + this.mediaListOrLibrary;
    str += "' by '" + this.typeOrGUID + "' in the '" + this.servicePaneOrMenu;
    str += "' of '" + this.registerString + "'";
    return str;
}

// ============================================================================
// ENTRY POINT
// ============================================================================

function runTest () {
  if (typeof(SBProperties) == "undefined") {
    Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
    if (!SBProperties)
      throw new Error("Import of sbProperties module failed!");
  }

  log("Testing Playlist Commands:");
  gTestPrefix = "PlaylistCommandsBuilder";
  testCommandsBuilder();

  gTestPrefix = "Playlist Commands Registration";
  testRegistration();
  log("OK");
}

// ============================================================================
// TESTS
// ============================================================================

function testCommandsBuilder() {

  var builder = new PlaylistCommandsBuilder("test-command-01");

  testAppendInsertRemove(builder, null, TESTLENGTH, 0);
  testCommandCallbacksAndShortcuts(builder, null, TESTLENGTH, 0);
  testSubmenus(builder, null, NUMRECURSE, 0);

  builder.shutdown();
}

function testRegistration() {
  _log("Testing Playlist Command Registration");

  // Create our gTestLibrary.  We have to register it so the playlist commands
  // manager can find it for registration.
  var libraryManager = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                         .getService(Ci.sbILibraryManager);
  var databaseGUID = "test_playlistcommands_registration";
  gTestLibrary = createLibrary(databaseGUID, false);

  libraryManager.registerLibrary(gTestLibrary, false);

  // Create our test medialists.  We will only add commands directly to
  // gPrimarySimpleList, the others are to confirm that a command wasn't
  // added somewhere it shouldn't have been.
  gPrimarySimpleList = gTestLibrary.createMediaList("simple");
  gSecondarySimpleList = gTestLibrary.createMediaList("simple");
  gSmartList = gTestLibrary.createMediaList("smart");

  // And create a very simple command with one action
  var newCommandId = "test-command-02";
  var newCommand = PlaylistCommandsBuilder(newCommandId);

  var newActionId = newCommandId + "-action"
  var triggerCallback = makeTriggerCallback(newActionId);
  newCommand.appendAction(null,
                          newActionId,
                          "label goes here",
                          "tooltip goes here",
                          triggerCallback);

  // Construct a list of instructions that is an exhaustive list of
  // the ways commands can be registered.
  var instructions = constructRegistrationInstructions(gTestLibrary,
                                                       gPrimarySimpleList,
                                                       newCommand);

  // Perform, validate, reverse, and validate the reversal of each instruction
  handleInstructions(instructions);

  // Cleanup
  libraryManager.unregisterLibrary(gTestLibrary);
  newCommand.shutdownCommands();
  _log("Playlist Command Registration Test Complete");
}

/* This method constructs an array of RegistrationInstructions based on
 * REGISTRATION_PARAM_COMBOS which is an exhaustive list of the
 * "medialist" or "library", "guid" or "type", and "servicepane" or "menu"
 * combinations that represent all the potential methods of
 * registering a playlist command
 */
function constructRegistrationInstructions(aLibrary, aMediaList, aCommand) {
  _log("Constructing Test Instructions");
  var instructions = [];

  var len = REGISTRATION_PARAM_COMBOS.length
  for (var i = 0; i < len; i++) {
    var params = REGISTRATION_PARAM_COMBOS[i];

    // a guid or type that will target our instruction
    var registerString = "";

    if (params[0] == "library") {
       // if we are targetting a library we know we'll use the guid
      registerString = aLibrary.guid;
    }
    else if (params[1] == "guid") {
      // if we are targetting a medialist by its guid, save the guid
      registerString = aMediaList.guid;
    }
    else {
      // if we are targetting a medialist by its type, save the type
      registerString = aMediaList.type;
    }

    var newInstruction = new RegistrationInstruction(params[0], // medialist or library
                                                     params[1], // guid or type
                                                     params[2], // servicepane or menu
                                                     registerString,
                                                     aCommand);
    instructions.push(newInstruction);
  }
  return instructions;
}

/* Handle each of the instructions that we created individually by performing
 * the instruction (registering the command), validating that it was
 * performed correctly, reversing the instruction (unregistering the command),
 * and validating that it was reversed correctly.
 */
function handleInstructions(aInstructions) {
  _log("Enacting Instructions");

  var len = aInstructions.length
  for (var i = 0; i < len; i++) {
    var currInstruction = aInstructions[i];
    performInstruction(currInstruction);
    assertInstruction(currInstruction);
    reverseInstruction(currInstruction);
    assertReversedInstruction(currInstruction);
  }
}

/* This method enacts a RegistrationInstruction, registering a playlist command
 * to a library, medialist, or medialist type.
 */
function performInstruction(aInstruction) {
  // We use different registration methods if we are targetting the servicepane
  var targetServicePane = (aInstruction.servicePaneOrMenu == "servicepane");

  if (aInstruction.mediaListOrLibrary == "library") {
    gCmdMgr.registerPlaylistCommandsForLibrary(targetServicePane,
                                               gTestLibrary,
                                               aInstruction.command);

  }
  else {
    // Registering to a medialist not a library.
    // We must use registerPlaylistCommandsMediaItem if our target is a menu.
    // We must use registerPlaylistCommandsMediaList if it's the servicepane.
    var registerFunction = (targetServicePane ?
                            gCmdMgr.registerPlaylistCommandsMediaList :
                            gCmdMgr.registerPlaylistCommandsMediaItem);

    // One of guid or type will be null while the other will take on our
    // saved value depending on if the instructions wants us to
    // target by guid or type.
    var guid = (aInstruction.typeOrGUID == "guid" ? aInstruction.registerString :
                                                    "");

    var type = (aInstruction.typeOrGUID == "type" ? aInstruction.registerString :
                                                    "");

    // Perform the registration
    registerFunction(guid, type, aInstruction.command);
  }
}

/* This method reverses a RegistrationInstruction, unregistering a playlist
 * command from a library, medialist, or medialist type.
 */
function reverseInstruction(aInstruction) {
  var targetServicePane = (aInstruction.servicePaneOrMenu == "servicepane");

  if (aInstruction.mediaListOrLibrary == "library") {
    gCmdMgr.unregisterPlaylistCommandsForLibrary(targetServicePane,
                                                 gTestLibrary,
                                                 aInstruction.command);

  }
  else {
    // registering to a medialist not a library
    // use registerPlaylistCommandsMediaItem if target is a menu
    // use registerPlaylistCommandsMediaList if target is the servicepane
    var unregisterFunction = (targetServicePane ?
                            gCmdMgr.unregisterPlaylistCommandsMediaList :
                            gCmdMgr.unregisterPlaylistCommandsMediaItem);
    var guid = (aInstruction.typeOrGUID == "guid" ? aInstruction.registerString :
                                                   "");

    var type = (aInstruction.typeOrGUID == "type" ? aInstruction.registerString :
                                                   "");

    unregisterFunction(guid, type, aInstruction.command);
  }
}

/* An entry point into our instruction checking.
 * We expect to find our command as this instruction was just performed so
 * send aExpectedToFind = true for either assert function.
 */
function assertInstruction(aInstruction) {
  if (aInstruction.mediaListOrLibrary == "library") {
    assertLibraryInstruction(aInstruction, true);
  }
  else {
    assertMediaListInstruction(aInstruction, true);
  }
}

/* An entry point into our instruction reversal checking.
 * We don't expect to find our command as this instruction was just reversed so
 * send aExpectedToFind = false for either assert function.
 */
function assertReversedInstruction(aInstruction) {
  if (aInstruction.mediaListOrLibrary == "library") {
    assertLibraryInstruction(aInstruction, false);
  }
  else {
    assertMediaListInstruction(aInstruction, false);
  }
}

/* This method verifies that an instruction that targetted the library was
 * performed or reversed successfully.
 *
 * If the insruction was just performed, this method should be called with
 * aExpectedToFind as true.  However, if an instruction was just reversed
 * aExpectedToFind should be false as it should have been removed.
 *
 * When this test is called with aExpectedToFind as true it confirms
 * that the instruction's command was added to all of the existing
 * medialists of the library, then that it gets added to any new medialist,
 * and finally that it gets removed from medialists when they are removed.
 *
 * When aExpectedToFind is false, it performs the same analysis but throws if
 * it does find the command anywhere.
 */
function assertLibraryInstruction(aInstruction, aExpectedToFind) {
  // We need to use a different get method if we are targetting the servicepane
  var getFunction = (aInstruction.servicePaneOrMenu == "servicepane" ?
                     gCmdMgr.getPlaylistCommandsMediaList :
                     gCmdMgr.getPlaylistCommandsMediaItem);

  // First we check if our command was added to all existing
  // medialists in the library, including the library's medialist itself
  var mediaLists = gTestLibrary.getItemsByProperty(SBProperties.isList, "1");
  mediaLists = mediaLists.QueryInterface(Ci.nsIMutableArray);
  mediaLists.appendElement(gTestLibrary, false);

  var len = mediaLists.length
  for (var i = 0; i < len; i++) {
    var guid = mediaLists.queryElementAt(i, Ci.sbIMediaList).guid;
    var rootCommand = getFunction(guid, "");

    var found = (rootCommand != null) &&
                isSubCommandPresent(rootCommand, aInstruction.command);
    assertTrue(aExpectedToFind == found,
               "We expected this instruction on an existing list to " +
               (aExpectedToFind ? "work and it didn't" : "be reversed and it wasn't") +
               ":\n  " + aInstruction.toString());
  }

  // Next we create a new list and see if our command was added to it
  var newList = gTestLibrary.createMediaList("simple");
  var newListRootCommand = getFunction(newList.guid, "");

  found = (newListRootCommand != null) &&
          isSubCommandPresent(newListRootCommand, aInstruction.command);
  assertTrue(aExpectedToFind == found,
             "We expected this instruction on an added list to " +
             (aExpectedToFind ? "work and it didn't" : "be reversed and it wasn't") +
             ":\n  " + aInstruction.toString());

  // Lastly we remove the list and see if our command is present
  gTestLibrary.remove(newList);
  newListRootCommand = getFunction(newList.guid, "");
  found = (newListRootCommand != null) &&
          isSubCommandPresent(newListRootCommand, aInstruction.command);

  assertFalse(found,
              "Instruction's command present on added and removed list: " +
               aInstruction.toString());
}

/* This method verifies that an instruction that targetted a medialist or
 * medialist type was performed or reversed successfully.
 *
 * If the insruction was just performed, this method should be called with
 * aExpectedToFind as true.  However, if an instruction was just reversed
 * aExpectedToFind should be false as it should have been removed.
 *
 * When this test is called with aExpectedToFind as true it confirms
 * that the instruction's command was added to all of the existing
 * medialists of the library, then that it gets added to any new medialist,
 * and finally that it gets removed from medialists when they are removed.
 *
 * When aExpectedToFind is false, it performs the same analysis but throws if
 * it does find the command anywhere.
 */
function assertMediaListInstruction(aInstruction, aExpectedToFind) {
  // We need to use a different get method if we are targetting the servicepane
  var getFunction = (aInstruction.servicePaneOrMenu == "servicepane" ?
                     gCmdMgr.getPlaylistCommandsMediaList :
                     gCmdMgr.getPlaylistCommandsMediaItem);

  // Retrieve the root command that we are interested in
  var guid = (aInstruction.typeOrGUID == "guid" ? aInstruction.registerString :
                                                 "");

  var type = (aInstruction.typeOrGUID == "type" ? aInstruction.registerString :
                                                 "");

  var primaryRootCommand = getFunction(guid, type);

  // Search the root command for our instruction's command
  var found = (primaryRootCommand != null) &&
              isSubCommandPresent(primaryRootCommand, aInstruction.command);
  assertTrue(aExpectedToFind == found,
             "We expected this instruction to " +
             (aExpectedToFind ? "work and it didn't" : "be reversed and it wasn't") +
             ":\n  " + aInstruction.toString());

  // Next we look for our command somewhere that it should not be.
  var secondaryRootCommand;
  if (guid.length > 0) {
    // if we registered the command to guid, ensure that it isn't present for
    // a command with the same type, but a different guid
    secondaryRootCommand = getFunction(gSecondarySimpleList.guid, "");
  }
  else {
    // if we registered the command to type, ensure that it isn't present for
    // a command with a different type
    secondaryRootCommand = getFunction("", gSmartList.type);
  }

  found = (secondaryRootCommand != null) &&
          isSubCommandPresent(secondaryRootCommand, aInstruction.command);

  // We would never expect the command to be found here so aExpectedtoFind
  // is irrelevant.
  assertFalse(found,
              "Instruction affected a type or guid it shouldn't have: " +
                aInstruction.toString());
}

/* A helper method to determine if any of the direct children of
 * aRootCommand is aSubCommand.
 */
function isSubCommandPresent(aRootCommand, aSubCommand) {
  for (var i = 0; i < aRootCommand.getNumCommands("",""); i++) {
    var currCommand = aRootCommand.getCommandSubObject("", i, "");
    if (currCommand == aSubCommand) {
      return true;
    }
  }
  return false;
}
