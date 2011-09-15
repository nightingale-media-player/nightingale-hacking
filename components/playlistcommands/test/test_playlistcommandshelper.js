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
 * \brief Playlist Commands Helper Unit Test File
 */

// ============================================================================
// GLOBALS & CONSTS
// ============================================================================

// enable or disable debug output
const DEBUG = false;

// determines the depth of submenus created within a helper-constructed command
const NUMRECURSE = 2;

/* determines the number of command subobjects that will be added to and removed
 * from a helper-constructed commands */
const TESTLENGTH = 500;

Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");

var cmdHelper = Cc["@songbirdnest.com/Songbird/PlaylistCommandsHelper;1"]
                  .getService(Ci.sbIPlaylistCommandsHelper);

var cmdMgr = Cc["@songbirdnest.com/Songbird/PlaylistCommandsManager;1"]
               .createInstance(Ci.sbIPlaylistCommandsManager);

const PlaylistCommandsBuilder = new Components.
  Constructor("@songbirdnest.com/Songbird/PlaylistCommandsBuilder;1",
              "sbIPlaylistCommandsBuilder");

/* If a playlist command is registered with RegisterPlaylistCommandsMediaItem
 * and does not have a targetFlags (i.e. it doesn't know where it is) we
 * use this flag to indicate that in the log.
 * This relies on TARGET_TOOLBAR being the last of the targetFlags. */
const INCOMPLETE_TARGET_FLAGS = cmdHelper.TARGET_TOOLBAR << 1;

/* Maps of playlist command id -> location flags indicating where that playlist
 * should be present.  Serves as a log to verify that all playlist command
 * additions and removals are performed correctly. */
var typeLocMap = new Object();
var guidLocMap = new Object();

// a medialist that we'll create for the test
var testMediaList = null;
// ============================================================================
// ENTRY POINT
// ============================================================================

function runTest () {
  log("Testing Playlist Commands Helper:");
  gTestPrefix = "PlaylistCommandsHelper";

  /* first test that a command created with the playlistCommandsHelper is fully
   * functional */
  testCreateCommandObjectForAction();

  /* next test all of the other playlistCommandsHelper functions that can
   * add, remove, or get existing playlist commands */
  testAddAndRemoveCommandObject();
  log("OK");
}

// ============================================================================
// TESTS
// ============================================================================

/* This runs a series of tests that verify the sanity of helper-constructed
 * sbIPlaylistCommands objects.  In particular, verifying that subcommands can
 * be added to and removed from helper-constructed playlist commands, checking
 * that all callbacks and shortcuts work properly, and that submenus can be
 * used within helper-constructed commands.
 *
 * The three tests; testAppendInsertRemove, testCommandCallbacksAndShortcuts,
 * and testSubmenus; can be found in head_playlistcommands.js
 */
function testCreateCommandObjectForAction() {

  _log("- Testing createCommandObject with appending, inserting, " +
       "and removing subobjects");
  var newCommandID = "test-id-1";
  var actionFn = makeTriggerCallback(newCommandID);
  var newCommand = cmdHelper.createCommandObjectForAction
                            (newCommandID, // an id for the command and action
                             "test-label-1", // the user-facing label
                             "test-tooltip-1", // a tooltip
                             actionFn /* the fn to call on click */);
  testAppendInsertRemove(newCommand, null, TESTLENGTH, 1);
  newCommand.shutdown();

  _log("- Testing createCommandObject with command callbacks and keyboard " +
       "shortcuts");
  newCommandID = "test-id-2";
  actionFn = makeTriggerCallback(newCommandID);
  newCommand = cmdHelper.createCommandObjectForAction
                            (newCommandID,
                             "test-label-2",
                             "test-tooltip-2",
                             actionFn);
  testCommandCallbacksAndShortcuts(newCommand, null, TESTLENGTH, 1);
  newCommand.shutdown();

  _log("- Testing createCommandObject with submenus");
  newCommandID = "test-id-3";
  actionFn = makeTriggerCallback(newCommandID);
  newCommand = cmdHelper.createCommandObjectForAction
                            (newCommandID,
                             "test-label-3",
                             "test-tooltip-3",
                             actionFn);
  testSubmenus(newCommand, null, NUMRECURSE, 1);
  newCommand.shutdown();
}

/* This is an exhaustive test of playlistCommandsHelper.addCommandObject
 * and removeCommandObject (and less directly getCommandObject) for all
 * combinations of added and removed locations, with all combinations of
 * registering by guid and/or type.
 *
 * This test proceeds by establishing some necessary elements, recording
 * all existing playlist commands in the maps that we'll use for later
 * verification that our operations were successful (guidLocMap and typeLocMap),
 * and then testing.
 *
 * Testing proceeds by selecting one of the combinations of added locations
 * and, for each combination of registering for type and/or guid, adding
 * a command object to those locations.  Next, we select a combination of
 * locations to remove that command object from and attempt to remove the
 * added command from those locations for both type and guid.  We do not check
 * that the command was actually added for that type, guid, or locations
 * as removal should fail gracefully and we will verify there were no strange
 * side effects using the maps.
 *
 * Once we verify that those adds and removals occurred properly, we remove the
 * command object from every place that it was added to cause it to unregister
 * completely so we can add it again on the next pass.
 *
 * The same operations performed for type and guid are interleaved to ensure
 * there is no breakdown in the strict separation of actions performed on
 * type and guid.
 */
function testAddAndRemoveCommandObject() {
  _log("- Testing addCommandObject and removeCommandObject");

  // this is all combinations of locations for a command object
  var locationsArray = [(cmdHelper.TARGET_SERVICEPANE_MENU |
                         cmdHelper.TARGET_TOOLBAR |
                         cmdHelper.TARGET_MEDIAITEM_CONTEXT_MENU),
                        (cmdHelper.TARGET_SERVICEPANE_MENU |
                         cmdHelper.TARGET_TOOLBAR),
                        (cmdHelper.TARGET_SERVICEPANE_MENU |
                         cmdHelper.TARGET_MEDIAITEM_CONTEXT_MENU),
                        (cmdHelper.TARGET_MEDIAITEM_CONTEXT_MENU |
                         cmdHelper.TARGET_TOOLBAR),
                        (cmdHelper.TARGET_MEDIAITEM_CONTEXT_MENU),
                        (cmdHelper.TARGET_TOOLBAR),
                        (cmdHelper.TARGET_SERVICEPANE_MENU)];

  // create a medialist to perform our test with
  var databaseGUID = "test_playlistcommandshelper";
  var testLibrary = createLibrary(databaseGUID, false);
  testMediaList = testLibrary.createMediaList("simple");

  /* before we start, we should make sure we have a log entry for all playlist
   * commands that are already registered */
  logExistingCommands(testMediaList);

  // for every combination of locations we can add to
  for (var cmdIndex = 0; cmdIndex < locationsArray.length; cmdIndex++)
  {
    // make a new command to test
    var newCommandID = "test-command-" + cmdIndex;
    var actionFn = makeTriggerCallback(newCommandID); // from head_playlistcommands
    var newCommandGUID = cmdHelper.createCommandObjectForAction
                              (newCommandID,
                               "test-label-" + cmdIndex,
                               "test-tooltip-" + cmdIndex,
                               actionFn);

    /* we need a duplicate so we have one to register to medialist guid and
     * another to register to medialist type */
    var newCommandType = newCommandGUID.duplicate();

    var addLocs = locationsArray[cmdIndex];

    // for each combination of operations on type and/or guid
    for (var p = 0; p < 3; p ++)
    {
      var addByType = false;
      var addByGUID = false;
      switch (p)
      {
      case 0:
        addByType = true;
        addByGUID = true;
        break;
      case 1:
        addByType = true;
        break;
      case 2:
        addByGUID = true;
        break;
      default:
        continue;
      }

      // for each combination of locations that we can remove from
      for (var j = 0; j < locationsArray.length; j++)
      {
        if (DEBUG)
        {
          log("\nperforming " + ((addByGUID) ? "addByGUID " : "") +
              ((addByType) ? "addByType " : "") + "with '" + newCommandID +
              "'\n" +"Adding to     - " + flagsToString(addLocs) + "\n" +
              "Removing from - " + flagsToString(locationsArray[j]) + "");
        }

        // add our command to the chosen locations
        if (addByGUID)
          addCommandToLocations(newCommandGUID, testMediaList, addLocs, "guid");

        if (addByType)
          addCommandToLocations(newCommandType, testMediaList, addLocs, "type");

        // select which locations we'll remove that command from, and remove it
        var removeLocs = locationsArray[j];

        /* Attempt to remove the commmand from the selected remove locations
         * irregardless of whether the command was actually added to those
         * locations or was registered to type or guid.
         * These calls will fail in those situations, but they should fail
         * gracefully and we'll check that is the case using our log maps */
        removeCommandFromLocations(newCommandGUID,
                                   testMediaList,
                                   removeLocs,
                                   "guid");
        removeCommandFromLocations(newCommandType,
                                   testMediaList,
                                   removeLocs,
                                   "type");

        /* Remove the command from everywhere that we added it so that it
         * unregisters and we can use it again.  If something goes wrong and
         * the command is not unregistered correctly, we'll get a
         * 'Duplicate Command in menu' error on the next pass. */
        if (addByGUID && guidLocMap[newCommandID] > 0)
        {
          if (DEBUG) _log("    complete removing " + newCommandID +
                          " from guid");
          cmdHelper.removeCommandObjectForGUID(addLocs,
                                               testMediaList.guid,
                                               newCommandGUID);

          // and blank the mapping to reflect that this command is now nowhere
          guidLocMap[newCommandID] = 0;
          assertCommandLocations(testMediaList);
        }
        if (addByType && typeLocMap[newCommandID] > 0)
        {
          if (DEBUG) _log("    complete removing " + newCommandID +
                          " from type");
          cmdHelper.removeCommandObjectForType(addLocs,
                                               testMediaList.type,
                                               newCommandType);

          // and blank the mapping to reflect that this command is now nowhere
          typeLocMap[newCommandID] = 0;
          assertCommandLocations(testMediaList);
        }
      }

      // our commands should have been removed, now we can shut them down
      if (DEBUG) _log("    shutting down " + newCommandID + " for guid");
      newCommandGUID.shutdownCommands();
      if (DEBUG) _log("    shutting down " + newCommandID + " for type");
      newCommandType.shutdownCommands();
    }
  }

  // we're done, cleanup our medialist
  testLibrary.remove(testMediaList);
}

function addCommandToLocations(newCommand, testMediaList, locations, guidOrType)
{
  if (guidOrType == "type")
  {
    if (DEBUG)
    {
      _log("    adding " + newCommand.id + " for type: " +
           flagsToString(addLocs));
    }

    cmdHelper.addCommandObjectForType(locations,
                                      testMediaList.type,
                                      newCommand);

    // log in our map where the command should be now
    typeLocMap[newCommand.id] = locations;

    // check to make sure our added command is where we expect it to be
    assertCommandLocations(testMediaList);
  }
  else {
    if (DEBUG)
    {
      _log("    adding " + newCommand.id + " for guid: " +
           flagsToString(addLocs));
    }

    cmdHelper.addCommandObjectForGUID(locations,
                                      testMediaList.guid,
                                      newCommand);

    // log in our map where the command should be now
    guidLocMap[newCommand.id] = locations;

    // check to make sure our added command is where we expect it to be
    assertCommandLocations(testMediaList);
  }
}

function removeCommandFromLocations(newCommand,
                                    testMediaList,
                                    locations,
                                    guidOrType)
{
  if (guidOrType == "type")
  {
    if (DEBUG)
    {
      _log("    removing " + newCommand.id + " from type: " +
           flagsToString(locations));
    }
    cmdHelper.removeCommandObjectForType(locations,
                                         testMediaList.type,
                                         newCommand);

    // update our map to track what the command's location should be
    if (typeLocMap[newCommand.id] > 0)
      typeLocMap[newCommand.id] = (typeLocMap[newCommand.id] & ~locations);
    assertCommandLocations(testMediaList);
  }
  else {
    if (DEBUG)
    {
      _log("    removing " + newCommand.id + " from guid: " +
           flagsToString(locations));
    }
    cmdHelper.removeCommandObjectForGUID(locations,
                                         testMediaList.guid,
                                         newCommand);
    // update our map to track what the command's location should be
    if (guidLocMap[newCommand.id] > 0)
      guidLocMap[newCommand.id] = (guidLocMap[newCommand.id] & ~locations);
    assertCommandLocations(testMediaList);
  }
}


/* this function confirms that all playlistcommands are in the locations
 * logged by guidLocMap and typeLocMap */
function assertCommandLocations(testMediaList)
{
  for (var id in guidLocMap)
  {
    let foundLocs = findCommandLocations(id, "guid", testMediaList);
    let checkLocs = guidLocMap[id];
    assertTrue(checkLocationFlags(foundLocs, checkLocs),
               "Command with id '" + id + "' is not where it was expected " +
               "when registered to medialist guid '" + testMediaList.guid +
               "'.\n" + "Expected at - " + flagsToString(checkLocs) + "\n" +
               "Found at    - " + flagsToString(foundLocs) + "\n");
  }

  for (var id in typeLocMap)
  {
    let foundLocs = findCommandLocations(id, "type", testMediaList);
    let checkLocs = typeLocMap[id];
    assertTrue(checkLocationFlags(foundLocs, checkLocs),
               "Command with id '" + id + "' is not where it was expected " +
               "when registered to medialist type '" + testMediaList.type +
               "'.\n" + "Expected at -" + flagsToString(checkLocs) + "\n" +
               "Found at    -" + flagsToString(foundLocs) + "\n")
  }
}

/* This function does the actual check to ensure the locations we found a
 * command in match those we expect to find it in.
 *
 * We cannot just check their equality because commands that are registered
 * not using the playlistCommandsHelper API do not have targetFlags, so we
 * can't distinguish if those commands are in the mediaitem context menu,
 * the toolbar, or both.  If we are handling such a command we check what we
 * can.
 */
function checkLocationFlags(foundLocs, checkLocs)
{
  if (foundLocs != checkLocs)
  {
    /* Things don't match up right, but that could be because we don't have
     * enough information in our checkLocs because the playlist command in
     * question didn't have targetFlags.
     * If the command didn't have target flags, we would have set the
     * INCOMPLETE_TARGET_FLAGS flag. */
    if (checkLocs & INCOMPLETE_TARGET_FLAGS)
    {
      /* This is a playlist command that didn't have target flags.
       * Our information about the servicepane menu, however, is complete,
       * so make sure those flags match.
       * Also, we know that it must be in at least one of the toolbar or
       * mediaitem context menu, so check for either of those flags. */
      if ((foundLocs & cmdHelper.TARGET_SERVICEPANE_MENU ==
           checkLocs & cmdHelper.TARGET_SERVICEPANE_MENU) &&
          ((foundLocs & cmdHelper.TARGET_MEDIAITEM_CONTEXT_MENU) ||
           (foundLocs & cmdHelper.TARGET_TOOLBAR)))
      {
        /* If the servicepane flags match and we know the command is in either
         * of the mediaitem context menu or toolbar, then that's the best
         * checking we can do with incomplete target flags. */
        return true;
      }
    }

   /* The found flags and the check flags don't match, and we didn't get
    * a pass because of incomplete target flags, so something must be wrong */
    return false;
  }
  else {
    // the flags match so we pass this check
    return true;
  }
}

/* A utility function to return location flags for those places where the
 * command whose id is cmdID is registered.
 *
 * param cmdID: the id of the command object we want to find the locations of
 * param guidOrType: either "type" or "guid" indicating to search in the
 *                   commands registered to type or guid
 * param testMediaList: the medialist this test is being performed within.
 */
function findCommandLocations(cmdID, guidOrType, testMediaList)
{
  var foundLocs = 0;
  if (guidOrType == "type")
  {
    // first check the service pane for the command object
    var spFound = cmdHelper.getCommandObjectForType
                           (cmdHelper.TARGET_SERVICEPANE_MENU,
                            testMediaList.type,
                            cmdID);

    if (spFound)
      foundLocs = foundLocs | cmdHelper.TARGET_SERVICEPANE_MENU;

    // next check the mediaitem context menu
    var cmFound = cmdHelper.getCommandObjectForType
                           (cmdHelper.TARGET_MEDIAITEM_CONTEXT_MENU,
                            testMediaList.type,
                            cmdID);
    if (cmFound)
      foundLocs = foundLocs | cmdHelper.TARGET_MEDIAITEM_CONTEXT_MENU;

    // last, check the toolbar
    var tbFound = cmdHelper.getCommandObjectForType
                           (cmdHelper.TARGET_TOOLBAR,
                            testMediaList.type,
                            cmdID);
    if (tbFound)
      foundLocs = foundLocs | cmdHelper.TARGET_TOOLBAR;
  }
  else {
    // first check the service pane for the command object
    var spFound = cmdHelper.getCommandObjectForGUID
                           (cmdHelper.TARGET_SERVICEPANE_MENU,
                            testMediaList.guid,
                            cmdID);
    if (spFound)
      foundLocs = foundLocs | cmdHelper.TARGET_SERVICEPANE_MENU;

    // next check the mediaitem context menu
    var cmFound = cmdHelper.getCommandObjectForGUID
                           (cmdHelper.TARGET_MEDIAITEM_CONTEXT_MENU,
                            testMediaList.guid,
                            cmdID);
    if (cmFound)
      foundLocs = foundLocs | cmdHelper.TARGET_MEDIAITEM_CONTEXT_MENU;

    // last, check the toolbar
    var tbFound = cmdHelper.getCommandObjectForGUID
                           (cmdHelper.TARGET_TOOLBAR,
                            testMediaList.guid,
                            cmdID);
    if (tbFound)
      foundLocs = foundLocs | cmdHelper.TARGET_TOOLBAR;
  }

  return foundLocs;
}

/* A function to add records for those command objects already registered to
 * testMediaList's type or guid to typeLocMap and guidLocMap so that they can be
 * accounted for in the test checks
 */
function logExistingCommands(testMediaList)
{
  /* We need to handle things registered with RegisterPlaylistCommandsMediaList
   * and RegisterPlaylistCommandsMediaItem.
   * Here we get the root item for each, then get there children, and add
   * each to the appropriate log */
  var testGUID = testMediaList.guid;

  // those registered with RegisterPlaylistCommandsMediaItem
  var rootGUIDItem = cmdMgr.getPlaylistCommandsMediaItem(testGUID, "");
  if (rootGUIDItem)
  {
    var guidItemChildrenEnum = rootGUIDItem.getChildrenCommandObjects();
    while(guidItemChildrenEnum.hasMoreElements())
    {
      addExistingToLocMap(guidItemChildrenEnum.getNext(),
                          guidLocMap,
                          "mediaitem");
    }
  }

  // those registered with RegisterPlaylistCommandsMediaList
  var rootGUIDList = cmdMgr.getPlaylistCommandsMediaList(testGUID, "");
  if (rootGUIDList)
  {
    var guidListChildrenEnum = rootGUIDList.getChildrenCommandObjects();
    while(guidListChildrenEnum.hasMoreElements())
    {
      addExistingToLocMap(guidListChildrenEnum.getNext(),
                          guidLocMap,
                          "medialist");
    }
  }

  // do the same but for the type
  var testType = testMediaList.type;

  // those registered with RegisterPlaylistCommandsMediaItem
  var rootTypeItem = cmdMgr.getPlaylistCommandsMediaItem("", testType);
  if (rootTypeItem)
  {
    var typeItemChildrenEnum = rootTypeItem.getChildrenCommandObjects();
    while(typeItemChildrenEnum.hasMoreElements())
    {
      addExistingToLocMap(typeItemChildrenEnum.getNext(),
                          typeLocMap,
                          "mediaitem");
    }
  }

  // those registered with RegisterPlaylistCommandsMediaList
  var rootTypeList = cmdMgr.getPlaylistCommandsMediaList("", testType);
  if (rootTypeList)
  {
    var typeListChildrenEnum = rootTypeList.getChildrenCommandObjects();
    while(typeListChildrenEnum.hasMoreElements())
    {
      addExistingToLocMap(typeListChildrenEnum.getNext(),
                          typeLocMap,
                          "medialist");
    }
  }
}

function addExistingToLocMap(playlistCommand, locMap, registrationLoc)
{
  playlistCommand = playlistCommand.QueryInterface(Ci.sbIPlaylistCommands);
  var id = playlistCommand.id;

  var knownLoc;
  if (registrationLoc == "medialist")
  {
    /* if this playlistcommand was registered with
     * RegisterPlaylistCommandsMediaList then it must be in the servicepane menu
     * so we can just set that as our known location */
    knownLoc = cmdHelper.TARGET_SERVICEPANE_MENU;
  }
  else {
    /* if this playlistcommands was registered with
     * RegisterPlaylistCommandsMediaItem then we know it is in both or either of
     * the toolbar and/or the mediaitem context menu.  We need targetFlags
     * to know for more specifically.
     */
    knownLoc = playlistCommand.targetFlags
  }

  if (!knownLoc || knownLoc == 0)
  {
    /* This playlist command does not know where it is.  In this situation
     * we use INCOMPLETE_TARGET_FLAGS to indicate to the function that checks
     * the map that this playlistCommand could be in both or either of the
     * toolbar and mediaitem context menu */
    locMap[id] = (locMap[id] ?
                  (locMap[id] | INCOMPLETE_TARGET_FLAGS) :
                  INCOMPLETE_TARGET_FLAGS);
  }
  else {
    //  this playlist command does know where it is so record that
    locMap[id] = (locMap[id] ? (locMap[id] | knownLoc) : knownLoc);
  }
}

// utility function to print the flags in human-readable form
function flagsToString(flags)
{
  var str = "";
  str += "ServicePane:" +
         ((flags & cmdHelper.TARGET_SERVICEPANE_MENU) > 0) + ", ";
  if (flags & INCOMPLETE_TARGET_FLAGS)
  {
    str += "Present in Toolbar and/or Context Menu, but incomplete info";
  }
  else {
    str += "Toolbar:" + ((flags & cmdHelper.TARGET_TOOLBAR) > 0) + ", ";
    str += "Context Menu:" +
           ((flags & cmdHelper.TARGET_MEDIAITEM_CONTEXT_MENU) > 0);
  }
  return str;
}
