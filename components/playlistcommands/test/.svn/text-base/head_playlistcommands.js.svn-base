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

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

// Current test being run
var gTestPrefix = "PlaylistCommandsBuilder";

// globals used to indicate successful function calls
var gTriggered = false;
var gRefreshed = false;
var gValueChecker = null;

/* an increasing counter used to generate ids, labels, and tooltips that
 * are unique among all subobjects added in these tests */
var gIDCounter = 0;

// Dummy DOM node and document used as returns
var dummyNode = {
  QueryInterface : XPCOMUtils.generateQI([Ci.nsIDOMNode])
};
var dummyDocument = {
  QueryInterface : XPCOMUtils.generateQI([Ci.nsIDOMDocument])
};

// PlaylistCommands subobject types.
// These constants are mirrored from sbPlaylistCommandsBuilder.js
var FLAG = {
  EXISTING: "existing",
  SUBMENU: "submenu",
  ACTION: "action",
  SEPARATOR: "separator",
  FLAG: "flag",
  VALUE: "value",
  CHOICEMENU: "choice",
  CHOICEMENUITEM: "choiceitem",
  CUSTOM: "custom",
  COMMAND: "subobject"
}

// const used by addChoiceMenuItems
var NUM_CHOICEMENUITEMS = 10;

// ============================================================================
// TEST FUNCTIONS
// ============================================================================

/* This log is central to the implementation of this test.
 * This test primarily deals with one command object and the subobjects
 * that are added to it.  Each subobjects entry in the log keeps track
 * of enough information about that subobject to verify its state
 * at any time through assertLog.  Objects like child sbIPlaylistCommands,
 * submenus, and choicemenus maintain information about their children in
 * sub-logs that are functionally similar to this, the outermost log. */
var gRootLog = new Array();

/* The command log in use at the moment.  Changed only through setLog(),
 * this will usually be gRootLog unless we are examining a subobject with
 * children, in which case the current command log, represented by
 * gActiveCommandLog, will be changed to the command log relevant to that
 * subobject.
 * Every test that can operate in or on a submenu must call setLog(submenuId)
 * at the start of the test to ensure that the gActiveCommandLog
 * is set appropriately. */
var gActiveCommandLog = gRootLog;

/* TestAppendInsertRemove
 * Test appending, inserting, and removing all types of sbIPlaylistCommands
 * subobjects to a menu within aCommand.
 * aMenuID indicates which menu within aCommand to direct the test to.
 * To specify the root menu aMenuID is passed as null, otherwise aMenuID
 * represents a submenu of aCommand that the test will be performed in.
 *
 * First this tests appending, removing, and inserting 'action'
 * subobjects  with previously added subobjects present.
 * Second, the same tests are run after clearing all child objects from the
 * command.
 * Lastly, we run testAppendAndInsertAllTypes which tests appending and
 * inserting other command subobject types.
 *
 * It is expected that the applicable command log is empty when this test begins
 *
 *@param aCommand The sbIPlaylistCommands object that will have subobjects
 *                appended to, inserted in, and removed from.
 *@param aMenuID The submenu of aCommand which the test will be performed in.
 *               null indicates the root menu of aCommand should be used
 *@param aTestLength The number of subobjects to be appended.  Half this number
 *                   will then be removed.  Then subobjects will be inserted
 *                   until aTestLength number of subobjects are present again.
 *@param aNumSubCommands The number of subobjects within aCommand under the
 *                       menu described by aMenuID when this test is begun.
 */
function testAppendInsertRemove(aCommand, aMenuID, aTestLength, aNumSubCommands)
{
  /* The correct log to verify subobjects depends on the menu that we are
   * currently in.  If we are not in a submenu, aMenuID is null, then we
   * use gRootLog.  Otherwise, we use the sublog that is attached to the
   * log object for the submenu specified by aMenuID */
  setLog(aMenuID);

  // the active command log is expected to be empty when this test begins
  assertEqual(gActiveCommandLog.length, 0);

  // sanity checks
  assertTrue((aNumSubCommands >= 0), "aNumSubCommands can't be negative");
  assertTrue((aTestLength > 0), "A test length > 0 must be specified");

  // make sure that we were told the correct number of existing commands
  assertNumCommands(aCommand, aMenuID, aNumSubCommands);

  /* Put an entry in the log for every existing subobject.  We don't know
   * anything about them, so just log where they are. */
  for (var i = 0; i < aNumSubCommands; i++)
  {
    var cmdID = aCommand.getCommandId(aMenuID,
                                      i,
                                      "test" /* host string */);
    gActiveCommandLog[i] = {CHECK_FLAG: FLAG.EXISTING,
                            CHECK_ID: cmdID};
  }

  /* Test appending, removing, and inserting actions in a command with
   * already-present subobjects */
  // First, append aTestLength number of actions
  _log("appendAction with existing commands", aMenuID);
  testAppendActions(aCommand, aMenuID, aTestLength, aNumSubCommands);

  // Second, remove 1/2 * aTestLength number of actions.
  // 1/2 was chosen arbitrarily
  _log("removeAction with existing commands", aMenuID);
  testRemoveSubobjects(aCommand, aMenuID, 0.5 * aTestLength, aNumSubCommands);

  // Then, insert 1/2 * aTestLength number of actions
  // 1/2 was chosen arbitrarily, but needs to match the number removed
  _log("insertAction with existing commands", aMenuID);
  testInsertActions(aCommand, aMenuID, 0.5 * aTestLength, aNumSubCommands);

  // ensure that we have aTestLength more commands than we started with
  assertNumCommands(aCommand, aMenuID, aTestLength + aNumSubCommands);

  // clear aCommand of subobjects and empty the active command log
  aCommand.removeAllCommands(aMenuID);
  assertNumCommands(aCommand, aMenuID, 0);
  gActiveCommandLog.length = 0;

  // Test appending, removing, and inserting actions in an empty command.
  // First, append aTestLength number of actions
  _log("appendAction without existing commands", aMenuID);
  testAppendActions(aCommand, aMenuID, aTestLength, 0);

  // Second, remove 1/2 * aTestLength number of actions
  // 1/2 was chosen arbitrarily
  _log("removeAction without existing commands", aMenuID);
  testRemoveSubobjects(aCommand, aMenuID, 0.5 * aTestLength, 0);

  // Then, insert 1/2 * aTestLength number of actions
  // 1/2 was chosen arbitrarily but needs to match the number removed
  _log("insertAction without existing commands", aMenuID);
  testInsertActions(aCommand, aMenuID, 0.5 * aTestLength, 0);

  // ensure that we have aTestLength commands
  assertNumCommands(aCommand, aMenuID, aTestLength);

  // clear aCommand of subobjects and empty the active command log
  aCommand.removeAllCommands(aMenuID);
  gActiveCommandLog.length = 0;
  assertNumCommands(aCommand, aMenuID, 0);

  // test appending and inserting all sub object types
  _log("append other types of command elements", aMenuID);
  testAppendAndInsertAllTypes(aCommand, aMenuID, aTestLength, 0);

  // Remove 1/2 * aTestLength number of subobjects
  // 1/2 was chosen arbitrarily
  _log("remove various types of command elements", aMenuID);
  testRemoveSubobjects(aCommand, aMenuID, 0.5 * aTestLength, 0);

  // clear aCommand of subobjects and empty the active command log
  aCommand.removeAllCommands(aMenuID);
  gActiveCommandLog.length = 0;
  assertNumCommands(aCommand, aMenuID, 0);
}

/* TestAppendActions
 * Test to append aTestLength number of actions and ensure that they have the
 * proper information and ordering.
 *
 * There are no expectations for gActiveCommandLog when this test begins.
 * When this test is over, aCommand will have aTestLength number of new
 * actions and gActiveCommandLog will have a record for each.
 *
 *@param aCommand The sbIPlaylistCommands object that subobjects will be
 *                appended to
 *@param aMenuID The submenu of aCommand which the test will be performed in.
 *               null indicates the root menu of aCommand should be used
 *@param aTestLength The number of subobjects to be appended.
 *@param aNumSubCommands The number of subobjects within aCommand under the
 *                       menu described by aMenuID when this test's parent
 *                       test began.
 */
function testAppendActions(aCommand, aMenuID, aTestLength, aNumSubCommands)
{
  /* The correct log to verify subobjects depends on the menu that we are
   * currently in.  If we are not in a submenu, aMenuID is null, then we
   * use gRootLog.  Otherwise, we use the sublog that is attached to the
   * log object for the submenu specified by aMenuID */
  setLog(aMenuID);

  // sanity checks
  assertTrue((aTestLength > 0), "A test length > 0 must be specified");
  assertTrue((aNumSubCommands >= 0), "aNumSubCommands can't be negative");

  // make sure all the present commands are accounted for
  assertNumCommands(aCommand, aMenuID, aNumSubCommands);

  // create a list of append action instructions
  var instructions = new Array();
  for (var i = 0; i < aTestLength; i++)
  {
    /* for this test we only append actions, so all of our instructions are the
     * same.  We want an action type object, added using append */
    instructions.push({ type: FLAG.ACTION,
                        action: "append%s"});
  }

  // execute those instructions
  ExecuteInstructions(aCommand, instructions, aMenuID);
  assertEqual(aTestLength + aNumSubCommands,
              gActiveCommandLog.length,
              "The length of the log should be "+ (aTestLength +
              aNumSubCommands) + ", not " + gActiveCommandLog.length);

  assertNumCommands(aCommand, aMenuID, gActiveCommandLog.length);
  assertLog(aCommand);
}

/* TestRemoveSubobjects
 * Test to remove aTestLength number of subobjects.  aTestLength number of
 * command subobjects must be present when this test is called, usually
 * that means calling this immediately after testAppendActions.
 *
 * gActiveCommandLog is expected to have at least aTestLength number of
 * entries more than aNumSubCommands when this test begins.
 * When this test is over, aCommand will have aTestLength fewer subobjects and
 * the corresponding records will be removed from gActiveCommandLog.
 *
 * The first aNumSubCommands sub objects of aCommand, will not be affected
 * by this test.  Removal begins with the last of the subobjects and proceeds
 * to the front, removing every other subobject.  When aNumSubCommands is
 * reached, we start over at the end rather than remove one of those
 * pre-existing commands.
 *
 *@param aCommand The sbIPlaylistCommands object that subobjects will be
 *                removed from
 *@param aMenuID The submenu of aCommand which the test will be performed in.
 *               null indicates the root menu of aCommand should be used
 *@param aTestLength The number of subobjects to be removed.
 *@param aNumSubCommands The number of subobjects within aCommand under the
 *                       menu described by aMenuID when this test's parent
 *                       test began.  These objects will not be removed.
 */
function testRemoveSubobjects(aCommand, aMenuID, aTestLength, aNumSubCommands)
{
  /* The correct log to verify subobjects depends on the menu that we are
   * currently in.  If we are not in a submenu, aMenuID is null, then we
   * use gRootLog.  Otherwise, we use the sublog that is attached to the
   * log object for the submenu specified by aMenuID */
  setLog(aMenuID);

  // sanity checks
  assertTrue((aTestLength > 0), "A test length > 0 must be specified");
  assertTrue((aNumSubCommands >= 0), "aNumSubCommands can't be negative");

  // check that there are enough commands to remove.
  assertNumCommands(aCommand, aMenuID, gActiveCommandLog.length);
  assertTrue((gActiveCommandLog.length >= aTestLength + aNumSubCommands),
             "testRemoveSubobjects expected more subobjects to be present." +
              "Have " + gActiveCommandLog.length + " but need at least " +
              (aTestLength + aNumSubCommands));

  /* We will remove every other subobject starting from the end.  If
   * we make a full pass of the array, removing every other one, and
   * still need to remove more, then we start over at the end.
   * Also, we don't remove from the first numSubCommands representing
   * existing subobjects from before this test's parent test was run */
  var index = gActiveCommandLog.length - 1;
  for (var i = 0; i < aTestLength; i++)
  {
    var checkObject = gActiveCommandLog[index];

    // remove the command object and its log record
    aCommand.removeCommand(aMenuID, checkObject.CHECK_ID);
    gActiveCommandLog.splice(index, 1);

    assertNumCommands(aCommand, aMenuID, gActiveCommandLog.length);

    // remove every other command subobject
    index = index - 2;

    // protect the existing commands by starting over at the end
    if (index < aNumSubCommands)
    {
      index = gActiveCommandLog.length - 1;
    }
  }

  assertLog(aCommand);
}

/* TestInsertActions
 * Test to insert aTestLength number of actions in aCommand under the menu
 * described by aMenuID (if aMenuID is null we consider the root menu of
 * aCommand).
 *
 * This function creates an instruction list that oscillates between
 * inserting before and after.  When those instructions are executed,
 * the insert location is determined, and those insert locations are meant to
 * jump around within aCommand.
 *
 * This test relies on aCommand having at least one command already present
 * under the submenu aMenuID.  This is necessary so that we know there
 * is always something to insert commands after or before.
 *
 * When this test finishes, aCommand will have aTestLength more subobjects
 *
 *@param aCommand The sbIPlaylistCommands object that subobjects will be
 *                inserted into
 *@param aMenuID The submenu of aCommand which the test will be performed in.
 *               null indicates the root menu of aCommand should be used
 *@param aTestLength The number of subobjects to be inserted.
 *@param aNumSubCommands The number of subobjects within aCommand under the
 *                       menu described by aMenuID when this test's parent
 *                       test began.
 */
function testInsertActions(aCommand, aMenuID, aTestLength, aNumSubCommands)
{
  /* The correct log to verify subobjects depends on the menu that we are
   * currently in.  If we are not in a submenu, aMenuID is null, then we
   * use gRootLog.  Otherwise, we use the sublog that is attached to the
   * log object for the submenu specified by aMenuID */
  setLog(aMenuID);

  // sanity checks
  assertTrue((aTestLength > 0), "A test length > 0 must be specified");
  assertTrue((aNumSubCommands >= 0), "aNumSubCommands can't be negative");
  assertNumCommands(aCommand, aMenuID, gActiveCommandLog.length);
  assertTrue((gActiveCommandLog.length > 0),
             "testInsertActions requires at least 1 command before running");

  var numAdded = 0;
  var index = 0;

  var instructions = new Array();
  for (var i = 0; i < aTestLength; i++)
  {
    /* Create a list of instructions that alternate between inserting actions
     * before and after. The insert location is handled by a separate counter */
    if (i % 2 == 0)
    {
      instructions.push({ type: FLAG.ACTION,
                          action: "insert%sBefore"});
    }
    else {
      instructions.push({ type: FLAG.ACTION,
                          action: "insert%sAfter"});
    }
  }

  ExecuteInstructions(aCommand, instructions, aMenuID);

  assertNumCommands(aCommand, aMenuID, gActiveCommandLog.length);
  assertLog(aCommand);
}

/* TestAppendAndInsertAllTypes
 * Test appending, inserting, and removing all types of sbIPlaylistCommands
 * subobjects.  This test creates a list of aTestLength number of instructions,
 * pairs of method operation types (ie "append", "insertBefore", or
 * "insertAfter") and subObject types, that are executed at the end of this
 * function.
 *
 * The command log for the submenu described by aMenuID is expected to be
 * empty when this test begins, and aCommand should have only aNumSubCommands
 * number of subobjects.
 *
 * The instructions, though guaranteed to represent all relevant
 * subobject types, select randomly from combinations of the three operation
 * types.
 *
 *@param aCommand The sbIPlaylistCommands object that subobjects will be
 *                appended and inserted into
 *@param aMenuID The submenu of aCommand which the test will be performed in.
 *               null indicates the root menu of aCommand should be used
 *@param aTestLength The number of subobjects to insert and append.
 *@param aNumSubCommands The number of subobjects within aCommand under the
 *                       menu described by aMenuID when this test begins.
 */
function testAppendAndInsertAllTypes(aCommand,
                                     aMenuID,
                                     aTestLength,
                                     aNumSubCommands)
{
  setLog(aMenuID);

  // sanity checks
  assertTrue((aTestLength > 0), "A test length > 0 must be specified");
  assertTrue((aNumSubCommands >= 0), "aNumSubCommands can't be negative");
  assertTrue((gActiveCommandLog.length == 0),
             "The relevant log for the menu '" + aMenuID + "' is expected to " +
              "be empty when testAppendAndInsertAllTypes begins.  It has a " +
              "length of " + gActiveCommandLog.length);

  /* Put an entry in the log for every existing subobject.  We don't know
   * anything about them, so just log what and where they are. */
  for (var i = 0; i < aNumSubCommands; i++)
  {
    var cmdID = aCommand.getCommandId(aMenuID,
                                      i,
                                      "test" /* host string */);
    gActiveCommandLog[i] = {CHECK_FLAG: FLAG.EXISTING,
                            CHECK_ID: cmdID};
  }

  /* this test expects aNumSubCommands subobjects to be present when this test
   * begins, so that means gActiveCommandLog.length should match the number
   * of subobjects now */
   assertNumCommands(aCommand, aMenuID, gActiveCommandLog.length);

  /* We generate a list of instructions that indicate what type of subobject
   * to add and how to add it. */
  var instructions = new Array();

  /* The possible combinations of all the ways to add a subobject to aCommand.
   * One of these arrays is selected randomly for a subobject type and an
   * instruction is then created for each of the actions in the array paired
   * with that type.  For example, if the 4th element is selected for the
   * "separator" subobject type, then two instructions will be created: one
   * to append a separator and the other to insert a separator with
   * 'insertBefore' */
  var actions = [["append%s"],
                 ["insert%sBefore"],
                 ["insert%sAfter"],
                 ["append%s", "insert%sBefore"],
                 ["append%s", "insert%sAfter"],
                 ["insert%sBefore", "insert%sAfter"],
                 ["append%s", "insert%sBefore", "insert%sAfter"]
                ];

  /* We make the first action an appendAction so that we know there will be at
   * least one subobject present before the randomly picked instructions are
   * executed.  We do this in case the first random instruction is an insert,
   * in which case it needs another object to be present to insert before
   * or after */
  instructions.push({type: FLAG.ACTION,
                     action: actions[0][0]});

  // make sure we add aTestLength number
  while (instructions.length < aTestLength)
  {
    // for each subobject type
    for (var flag in FLAG)
    {
      // we don't concern ourselves with some of the flags that don't apply here
      if (FLAG[flag] == FLAG.EXISTING ||
          FLAG[flag] == FLAG.CHOICEMENUITEM ||
          FLAG[flag] == FLAG.SUBMENU)
      {
        continue;
      }

      /* generate a random num between 0 and (actions.length - 1) to pick
       * an instruction combination */
      var num = Math.floor(Math.random() * actions.length);

      // get the instruction combination, an array of actions, that we selected
      var currActions = actions[num];

      /* push an instruction for each of the actions in the combination we
       * selected */
      for each (var a in currActions)
      {
        instructions.push({ type: FLAG[flag],
                            action: a});

        // if we have aTestLength number of instructions, we are done
        if (instructions.length >= aTestLength)
          break;
      }

      // if we have aTestLength number of instructions, we are done
      if (instructions.length >= aTestLength)
        break;
    }
  }

  /* perform the list of instructions that we just created */
  ExecuteInstructions(aCommand, instructions, aMenuID);
  assertNumCommands(aCommand, aMenuID, gActiveCommandLog.length);
  assertLog(aCommand);
}

/* ExecuteInstructions
 * This functions takes a list of instructions, pairs of operation
 * types (ie "append", "insertBefore", or "insertAfter") and subObject types,
 * and executes the appropriate operation.
 *
 * There must be at least one subobject present in aCommand under the menu
 * described by aMenuID if the first instruction is an insert operation,
 * otherwise there will be nothing to insert before or after.  This should be
 * handled by the caller.
 *
 * Valid instructions are of the form { type: t, action: a} where t is one of
 * the FLAGs and a is one of 'append&s', 'insert%sAfter', or 'insert%sBefore'
 *
 * For each of the instructions, the functionName is derived from the operation
 * and subObject types. Then, the params for the execute call are pulled
 * together in the params array.
 * Finally, the function described by functionName is executed with the
 * params array.
 *
 *@param aCommand The sbIPlaylistCommands object that subobjects will be
 *                appended and inserted into
 *@param aInstructions The array of instruction objects that indicate which
 *                     operations to perform for which subobject types.
 *                     Each instruction must be of the form {type: t, action: a}
 *@param aMenuID The submenu of aCommand which the test will be performed in.
 *               null indicates the root menu of aCommand should be used
 */
function ExecuteInstructions(aCommand, aInstructions, aMenuID)
{
  setLog(aMenuID);

  // sanity checks
 assertTrue(aCommand instanceof Ci.sbIPlaylistCommands,
             "aCommand is not a sbIPlaylistCommands");

  // index indicating where to perform the next insert operation
  var insertIndex = 0;

  /* this bool makes sure that the first choice menu we create gets
   * choice items as subobjects to be sure we test the choice items */
  var firstChoiceMenu = true;

  for each (var instruction in aInstructions)
  {
    assertTrue("type" in instruction,
               "The instruction being executed doesn't have a subobject type");
    assertTrue("action" in instruction,
              "The instruction being executed doesn't have an action type");

    /* the instruction.action is an incomplete functionName with %s in place of
     * the type */
    var functionName = instruction.action;
    var type = instruction.type;

    /* fill out the method name which currently has %s instead of the subobject
     * type to be added */
    functionName = completeFunctionName(type, functionName);

    /* create an array for the params that will be sent to the function
     * described by functionName */
    var params = [aMenuID];

    /* Create a log object that keeps track of information about the object
     * being added so that we can ensure all operations were succesful.  We will
     * add more to this object as we learn more about the subobject being added.
     */
    var checkObject = {CHECK_FLAG: type,
                       CHECK_SUBMENUID: aMenuID};

    // figure out where to put the log object in the command log.
    // first figure out if this is an insert or append operation
    if (/^insert/.test(functionName))
    {
      // this is an insert so we'll need to splice into the log
      var insertId = gActiveCommandLog[insertIndex].CHECK_ID;

      params.push(insertId);
      if (/Before$/.test(functionName))
      {
        // this is an insertBefore operation
        gActiveCommandLog.splice(insertIndex, 0, checkObject);
      }
      else if (/After$/.test(functionName)) {
        // this is an insertAfter operation
        gActiveCommandLog.splice(insertIndex + 1, 0, checkObject);
      }
      else {
        doFail("Instruction had invalid action in ExecuteInstructions. Tried " +
                "to execute '" + functionName + "'");
      }

      // jump the insertIndex around with +3 chosen arbitrarily
      insertIndex = (insertIndex + 3) % gActiveCommandLog.length;
    }
    else if (functionName.search(/append/) != -1 ) {
      // this is an append so we can just push onto the back of the log
      gActiveCommandLog.push(checkObject);
    }
    else {
      fail("Received an instruction with an unrecognized function name");
    }

    /* Get an id, label, and tooltip for the new subobject.  In all cases we
     * need the id, so push that now, the label and tooltip we may not need. */
    var [currId, currLabel, currTooltip] = getCommandData(type);
    params.push(currId);
    checkObject.CHECK_ID = currId;

    // separators, custom objects, and subcommands don't have a label or tooltip
    if (type != FLAG.SEPARATOR &&
        type != FLAG.CUSTOM &&
        type != FLAG.COMMAND)
    {
      params.push(currLabel, currTooltip);
      checkObject.CHECK_LABEL = currLabel;
      checkObject.CHECK_TOOLTIP = currTooltip;
    }

    // the remaining params are unique enough to warrant individual handling
    switch(type)
    {
    case FLAG.ACTION:
      // action objects need a trigger callback
      params.push(makeTriggerCallback(currId));
      break;
    case FLAG.FLAG:
      // Flag objects need a trigger and value callback.
      params.push(makeTriggerCallback(currId));

      // Value callback for flag types returns a bool.
      checkObject.CHECK_VALUECALLBACK = ((gIDCounter % 2) == 0);
      params.push(makeValueCallback(currId, checkObject.CHECK_VALUECALLBACK));
      break;
    case FLAG.VALUE:
      // value objects need setting and getting value callbacks
      params.push(makeValueSetter(currId));

      // Value callback for value types returns a string.
      checkObject.CHECK_VALUECALLBACK = FLAG.VALUE + gIDCounter;
      params.push(makeValueCallback(currId, checkObject.CHECK_VALUECALLBACK));
      break;
    case FLAG.CHOICEMENU:
      // jhawk this should probably check getCommandChoiceItem, but I can't
      //       figure out how that works
      checkObject.CHECK_VALUECALLBACK = FLAG.CHOICEMENU + gIDCounter;
      params.push(makeValueCallback(currId, checkObject.CHECK_VALUECALLBACK));
      break;
    case FLAG.CUSTOM:
      // custom objects need an instantiation and refresh callback each
      params.push(makeInstantiationCallback(currId));
      params.push(makeRefreshCallback(currId));
      break;
    case FLAG.COMMAND:
      // we add some actions (0-4) to each subcommand
      var newCommand = PlaylistCommandsBuilder(currId);
      checkObject.CHECK_SUBACTIONS = new Array();

      // select a random number between 0 and 4 and add that many subactions
      var rand = Math.floor(Math.random() * 5);
      for (var p = 0; p < rand; p++)
      {
        let subId = "subaction" + p + "id-in-" + currId;
        let subLabel = "subaction" + p + "label-in-" + currId;
        let subTooltip = "subaction" + p + "tooltip-in-" + currId;
        let callback = makeTriggerCallback(subId);
        newCommand.appendAction(null,
                                subId,
                                subLabel,
                                subTooltip,
                                callback);
        checkObject.CHECK_SUBACTIONS
                   .push({CHECK_FLAG: FLAG.ACTION,
                          CHECK_ID: subId,
                          CHECK_LABEL: subLabel,
                          CHECK_TOOLTIP: subTooltip,
                          CHECK_SUBMENUID: null});
      }
      params.push(newCommand);
      break;
    }

    // here we do the operation we've been constructing in this function
    aCommand[functionName].apply(this, params);

    /* at this point if this instruction is for a choice menu, it has been added
     * so it might be time to add choicemenuitems to it */
    if (type == FLAG.CHOICEMENU && (gIDCounter % 2 == 0 || firstChoiceMenu))
    {
      addChoiceMenuItems(aCommand, currId, checkObject)
      firstChoiceMenu = false;
    }
  }
}

/* replace the missing portions of a functionName depending on the type of
 * subobject and return the completed functionName */
function completeFunctionName(aType, aFunctionName)
{
  switch(aType)
  {
  case FLAG.ACTION:
    aFunctionName = aFunctionName.replace(/%s/, "Action");
    break;
  case FLAG.SEPARATOR:
    aFunctionName = aFunctionName.replace(/%s/, "Separator");
    break;
  case FLAG.FLAG:
    aFunctionName = aFunctionName.replace(/%s/, "Flag");
    break;
  case FLAG.VALUE:
    aFunctionName = aFunctionName.replace(/%s/, "Value");
    break;
  case FLAG.CHOICEMENU:
    aFunctionName = aFunctionName.replace(/%s/, "ChoiceMenu");
    break;
  case FLAG.CUSTOM:
    aFunctionName = aFunctionName.replace(/%s/, "CustomItem");
    break;
  case FLAG.COMMAND:
    aFunctionName = aFunctionName.replace(/%s/, "PlaylistCommands");
    break;
  default:
    fail("unrecognized instruction type: " + type);
  }
  return aFunctionName;
}

/* Adds NUM_CHOICEMENUITEMS number of choicemenuitems to the choicemenu under
 * the param command with an id of choiceMenuId.  Additions are made to the
 * CHECK_CHOICEMENUITEMS array of the param checkObject to serve as a mini-log
 * of the added choicemenuitems.
 */
function addChoiceMenuItems(aCommand, aChoiceMenuId, aCheckObject)
{
  aCheckObject.CHECK_CHOICEMENUITEMS = new Array();
  for (var c = 0; c < NUM_CHOICEMENUITEMS; c++)
  {
    assertNumCommands(aCommand, aChoiceMenuId, c);
    var currId = "choiceitem" + c;
    var currLabel = "choiceitem" + c + "label";
    var currTooltip = "choiceitem" + c + "tooltip";
    var triggerCallback = makeTriggerCallback(currId);
    aCommand.appendChoiceMenuItem(aChoiceMenuId,
                                  currId,
                                  currLabel,
                                  currTooltip,
                                  triggerCallback);
    aCheckObject.CHECK_CHOICEMENUITEMS
                .push({CHECK_FLAG: FLAG.CHOICEMENUITEM,
                       CHECK_ID: currId,
                       CHECK_LABEL: currLabel,
                       CHECK_TOOLTIP: currTooltip,
                       CHECK_SUBMENUID: aChoiceMenuId});
  }
  assertEqual(aCheckObject.CHECK_CHOICEMENUITEMS.length,
              NUM_CHOICEMENUITEMS,
              "The length of the choicemenuitem log should be "+
               NUM_CHOICEMENUITEMS + ", not " +
               aCheckObject.CHECK_CHOICEMENUITEMS.length);
  assertNumCommands(aCommand, aChoiceMenuId, NUM_CHOICEMENUITEMS);
}

/* TestCommandCallbacksAndShortcuts
 * This tests the command enabled and visible callbacks that determine where/
 * when/if a subobject appears, as well as the command shortcuts (keyboard
 * shortcuts) and action triggers that occur when the user activates and action.
 *
 * It is expected that the relevant gActiveCommandLog is empty when this test
 * is run
 */
function testCommandCallbacksAndShortcuts(aCommand,
                                          aMenuID,
                                          aTestLength,
                                          aNumSubCommands)
{
  setLog(aMenuID);

  // the active command log is expected to be empty when this test begins
  assertEqual(gActiveCommandLog.length, 0);

  // sanity checks
  assertTrue((aNumSubCommands >= 0), "aNumSubCommands can't be negative");
  assertTrue((aTestLength > 0), "A test length > 0 must be specified");

  // make sure that we were told the correct number of existing commands
  assertNumCommands(aCommand, aMenuID, aNumSubCommands);

  /* Put an entry in the log for every existing subobject.  We don't know
   * anything about them, so just log where they are. */
  for (var i = 0; i < aNumSubCommands; i++)
  {
    var cmdID = aCommand.getCommandId(aMenuID,
                                      i,
                                      "test" /* host string */);
    gActiveCommandLog[i] = {CHECK_FLAG: FLAG.EXISTING,
                            CHECK_ID: cmdID};
  }

  // first append some logged objects to the commands for use in the test
  testAppendActions(aCommand, aMenuID, aTestLength, aNumSubCommands);

  _log("setCommandVisibleCallback and setCommandEnabledCallback",
       aMenuID);
  testCommandVisibleAndEnabled(aCommand, aMenuID, aTestLength, aNumSubCommands);

  _log("setCommandShortcut", aMenuID);
  testCommandShortcut(aCommand, aMenuID, aTestLength, aNumSubCommands);

  _log("command action triggers", aMenuID);
  testTriggerActions(aCommand, aMenuID, aTestLength, aNumSubCommands);

  _log("initiation and shutdown callbacks", aMenuID);
  testInitShutdownCB(aCommand);

  aCommand.removeAllCommands(aMenuID);
  gActiveCommandLog.length = 0;
  assertNumCommands(aCommand, aMenuID, 0);
}

/* TestInitShutdownCB
 * This tests the initialization and shutdown callbacks of an
 * sbIPlaylistCommands.
 */
function testInitShutdownCB(aCommand) {

  // we'll use these variables to determine success
  var isInitialized = false;
  var isShutdown = false;

  // definte the callbacks that we'll need
  function cmds_init(context, host, data) {
    var implementorContext = {
      testString: "test",
      toString: function() {
        return this.testString;
      },
      QueryInterface: XPCOMUtils.generateQI([Ci.nsISupportsString])
    };
    context.implementorContext = implementorContext;
    isInitialized = true;
  }

  function cmds_shutdown(context, host, data) {
    assertTrue((context),
               "shutdown callback not provided context");
    assertTrue((context.implementorContext),
               "shutdown callback not provided implementor context");
    var testString =
        context.implementorContext.QueryInterface(Ci.nsISupportsString);
    assertEqual(testString.toString(),
                "test",
                "shutdown callback implementor context invalid");
    context.implementorContext = null;
    isShutdown = true;
  }

  // set the callbacks that we want
  aCommand.setInitCallback(cmds_init);
  aCommand.setShutdownCallback(cmds_shutdown);

  // copy the command because we are going to shut it down in this test
  var commandDup = aCommand.duplicate();

  // try the initialize
  commandDup.initCommands("");
  assertTrue(isInitialized,
             "initCommands did not call init callback!");

  // try the shutdown
  commandDup.shutdownCommands();
  assertTrue(isShutdown,
             "shutdownCommands did not call shutdown callback!");
}

/* TestCommandVisibleAndEnabled
 * This tests the command enabled and visible callbacks that determine where/
 * when/if a subobject appears.  Every other command is made visible, while
 * every third is made enabled.  If CHECK_VIS or CHECK_ENABLED is defined
 * in a log object, the values are compared to the result from getCommandVisible
 * and getCommandEnabled respectively.
 */
function testCommandVisibleAndEnabled(aCommand,
                                      aMenuID,
                                      aTestLength,
                                      aNumSubCommands)
{
  setLog(aMenuID);
  assertTrue((aTestLength > 0), "A test length > 0 must be specified");
  assertTrue((aNumSubCommands >= 0), "aNumSubCommands can't be negative");

  for (var i = aNumSubCommands; i < aTestLength + aNumSubCommands; i++)
  {
    var checkObject = gActiveCommandLog[i];

    checkObject.CHECK_VIS = ((i % 2) == 0);
    var visCallback = makeValueCallback(checkObject.CHECK_ID,
                                        checkObject.CHECK_VIS);
    aCommand.setCommandVisibleCallback(aMenuID, checkObject.CHECK_ID, visCallback);

    checkObject.CHECK_ENABLED = ((i % 3) == 0);
    var enabledCallback = makeValueCallback(checkObject.CHECK_ID,
                                            checkObject.CHECK_ENABLED);
    aCommand.setCommandEnabledCallback(aMenuID, checkObject.CHECK_ID, enabledCallback);
  }
  assertLog(aCommand);
}

/* TestCommandShortcut
 * This tests the the command shortcuts (keyboard shortcuts).  Every subobject
 * is given a shortcut based on its index in the gActiveCommandLog, with the local
 * boolean alternating between true and false.  If CHECK_SHORTCUT is
 * defined in log object, the command shortcuts will be checked.
 */
function testCommandShortcut(aCommand, aMenuID, aTestLength, aNumSubCommands)
{
  setLog(aMenuID);

  // sanity checks
  assertTrue((aTestLength > 0), "A test length > 0 must be specified");
  assertTrue((aNumSubCommands >= 0), "aNumSubCommands can't be negative");

  for (var i = aNumSubCommands; i < aTestLength + aNumSubCommands; i++)
  {
    var checkObject = gActiveCommandLog[i];
    aCommand.setCommandShortcut(aMenuID,
                                checkObject.CHECK_ID,
                                "key" + i,
                                "keycode" + i,
                                "modifier" + i,
                                (i % 2) == 0);
    checkObject.CHECK_SHORTCUT = ((i % 2) == 0);
  }

  assertLog(aCommand);
}

/* TestTriggerActions
 * Tests that every action trigger is functional by calling onCommand on the
 * subobject and ensuring that the gTriggered boolean changes from
 * false to true (the only operation performed by the test trigger callback)
 */
function testTriggerActions(aCommand, aMenuID, aTestLength, aNumSubCommands)
{
  setLog(aMenuID);

  // sanity checks
  assertTrue((aTestLength > 0), "A test length > 0 must be specified");
  assertTrue((aNumSubCommands >= 0), "aNumSubCommands can't be negative");

  for (var i = aNumSubCommands; i < aTestLength + aNumSubCommands; i++)
  {
    assertTriggerCallback(aCommand, gActiveCommandLog[i], i);
  }
}

/* TestSubmenus
 * Test submenus by adding aTestLength number of submenus.  We cycle through
 * appending, insertingAfter, and insertingBefore.  Finally, we run
 * testAppendAndInsertAllTypes within the first submenu added.
 */
function testSubmenus(aCommand, aMenuID, aTestLength, aNumSubCommands)
{
  setLog(aMenuID);

  // sanity checks
  assertTrue((aTestLength > 0), "A test length > 0 must be specified");
  assertTrue((aNumSubCommands >= 0),
             "aNumSubCommands can't be negative, got: " + aNumSubCommands);

  _log("adding submenus", aMenuID);

  /* Put an entry in the log for every existing subobject.  We don't know
   * anything about them, so just log where they are. */
  for (var i = 0; i < aNumSubCommands; i++)
  {
    var cmdID = aCommand.getCommandId(aMenuID,
                                      i,
                                      "test" /* host string */);
    gActiveCommandLog[i] = {CHECK_FLAG: FLAG.EXISTING,
                            CHECK_ID: cmdID};
  }

  // marker to keep track of a location for inserting before and after
  let insertCounter = 0;

  // add aTestLength number of submenus
  for (var i = 0; i < aTestLength; i++)
  {
    var currId = FLAG.SUBMENU + i + "id";
    var currLabel = FLAG.SUBMENU + i + "label";
    var currTooltip = FLAG.SUBMENU + i + "tooltip";

    // an object for the log that keeps track of this submenu's information
    var checkObject = {CHECK_FLAG: FLAG.SUBMENU,
                       CHECK_ID: currId,
                       CHECK_LABEL: currLabel,
                       CHECK_TOOLTIP: currTooltip,
                       CHECK_SUBMENUID: aMenuID};

    // we cycle through append, insertAfter, and insertBefore
    switch (i % 3)
    {
    case 0:
      // add submenu to the end, and add the log object to the end of the log
      aCommand.appendSubmenu(aMenuID, currId, currLabel, currTooltip);
      gActiveCommandLog.push(checkObject);
      break;
    case 1:
      // add submenu as insertAfter, and splice the log object into the log
      var insertId = gActiveCommandLog[insertCounter].CHECK_ID;
      aCommand.insertSubmenuAfter(aMenuID,
                                  insertId,
                                  currId,
                                  currLabel,
                                  currTooltip)
      gActiveCommandLog.splice(insertCounter + 1, 0 , checkObject);

      // jump the insertIndex around with +3 chosen arbitrarily
      insertCounter = (insertCounter + 3) % gActiveCommandLog.length;
      break;
    case 2:
      // add submenu as insertBefore, and splice the log object into the log
      var insertId = gActiveCommandLog[insertCounter].CHECK_ID;
      aCommand.insertSubmenuBefore(aMenuID,
                                   insertId,
                                   currId,
                                   currLabel,
                                   currTooltip)
      gActiveCommandLog.splice(insertCounter, 0 , checkObject);

      // jump the insertIndex around with +4 chosen arbitrarily
      insertCounter = (insertCounter + 4) % gActiveCommandLog.length;
      break;
    default:
      break;
    }
  }

  // confirm that the command object's contents match the log
  assertNumCommands(aCommand, aMenuID, gActiveCommandLog.length);
  assertLog(aCommand);

  // run testAppendAndInsertAllTypes within the first submenu
  var submenuId = gActiveCommandLog[aNumSubCommands].CHECK_ID;
  _log("adding subobjects of all types to submenu", submenuId);
  testAppendAndInsertAllTypes(aCommand, submenuId, aTestLength, 0);

  // clear the command of subobjects and empty the log
  aCommand.removeAllCommands(aMenuID);
  gActiveCommandLog.length = 0;
  assertNumCommands(aCommand, aMenuID, 0);
}

// ============================================================================
// COMMAND CALLBACKS
// ============================================================================

// makes a test trigger callback that sets gTriggered to true
function makeTriggerCallback(aActionID)
{
  return function TriggerCallback(context, submenu, commandid, host)
  {
    gTriggered = true;
    assertEqual(commandid,
                aActionID,
                "CommandId passed to action callback should be " + aActionID +
                 ", not " + commandid + "!");
  };
}

// makes a test value callback that returns the param callbackval
function makeValueCallback(aActionID, aCallbackval)
{
  return function ValueCallback(context, submenu, commandid, host)
  {
    assertEqual(commandid,
                aActionID,
                "CommandId should be " + aActionID + "in visible callback, not " +
                 commandid + "!");
    return aCallbackval;
  };
}

// makes a value setter function that sets gValueChecker
function makeValueSetter(aActionID)
{
  return function ValueSetter(context, submenu, commandid, host, val)
  {
    gValueChecker = val;
    assertEqual(commandid,
                aActionID,
                "CommandId should be " + aActionID + "in visible callback, not " +
                 commandid + "!");
  };
}

// makes an instantiation callback that is used by custom commands
function makeInstantiationCallback(aActionID)
{
  return function InstantiationCallback(context, submenu, commandid, host, doc)
  {
    assertTrue((doc instanceof Ci.nsIDOMDocument),
               "instantiateCustomCommand did not forward the document");
    assertEqual(commandid,
                aActionID,
                "CommandId should be " + aActionID + " in instantiation callback" +
                 ", not " + commandid + "!");
    return dummyNode;
  };
}

// makes a refresh callback that is used by custom commands
function makeRefreshCallback(aActionID)
{
  return function RefreshCallback(context, submenu, commandid, host, element)
  {
    assertEqual(commandid,
                aActionID,
                "CommandId should be " + aActionID + " in instantiation callback" +
                 ", not " + commandid + "!");
    assertTrue((element instanceof Ci.nsIDOMNode),
               "refreshCustomCommand did not trigger a refresh or failed to " +
                "forward a DOM node");
    gRefreshed = true;
  };
}

// ============================================================================
// ASSERTS
// ============================================================================

/* This function compares the currently active gActiveCommandLog with the contents
 * of aCommandObject to ensure that all subobjects are complete and were
 * added correctly.
 */
function assertLog(aCommandObject)
{
  // we handle the gActiveCommandLog in reverse order because we can
  for (var j = gActiveCommandLog.length-1; j >= 0; j--)
  {
    var checkObject = gActiveCommandLog[j];
    // make sure a submenuid is specified.  null refers to the root
    if (typeof(checkObject.CHECK_SUBMENUID) == "undefined")
      checkObject.CHECK_SUBMENUID = null;

    /* make sure a host string is specifed.  This is largely unimportant for our
     * operations */
    if (typeof(checkObject.CHECK_HOST) == "undefined")
      checkObject.CHECK_HOST = "";

    /* all subobjects must have an id, and we need to confirm that the
     * type of subobject is what we think it is */
    assertType(aCommandObject, checkObject, j);
    assertId(aCommandObject, checkObject, j);

    /* only separators, custom objects, and subcommands don't have labels
     * and tooltips */
    if (checkObject.CHECK_FLAG != FLAG.SEPARATOR &&
        checkObject.CHECK_FLAG != FLAG.CUSTOM &&
        checkObject.CHECK_FLAG != FLAG.COMMAND)
    {
      assertLabel(aCommandObject, checkObject, j);
      assertTooltip(aCommandObject, checkObject, j);
    }

    /* perform more specialized checking depending on the type of object
     * being checked */
    switch(checkObject.CHECK_FLAG)
    {
    case FLAG.ACTION:
      /* for actions we check the ui callbacks (like the action trigger) and
       * the keyboard command shortcuts */
      assertUICallbacks(aCommandObject, checkObject, j);
      assertShortcuts(aCommandObject, checkObject, j);
      break;
    case FLAG.FLAG:
      // for flags we check the get value returned by the value callback
      assertValueCallback(aCommandObject, checkObject, j);
      break;
    case FLAG.VALUE:
      // for values we check the value getter and setters
      assertValueCallback(aCommandObject, checkObject, j);
      assertValueSetter(aCommandObject, checkObject, j);
      break;
    case FLAG.CHOICEMENU:
      // jhawk we probably also need a getCommandChoiceItem check,
      //       but I can't figure out how that works

      // if a choice menu has choicemenuitems, we check those too
      if ("CHECK_CHOICEMENUITEMS" in checkObject)
      {
        assertChoiceMenuItems(aCommandObject, checkObject, j);
      }
      break;
    case FLAG.CUSTOM:
      // for custom objects we check the instantiate and refresh callbacks
      assertInstantiate(aCommandObject, checkObject, j);
      assertRefresh(aCommandObject, checkObject, j);
      break;
    case FLAG.COMMAND:
      // for subcommands we check that all subactions are sane as well
      assertSubActions(aCommandObject, checkObject, j);
      break;
    case FLAG.SEPARATOR:
     // separators only really have an id and type so this is checked already
    case FLAG.SUBMENU:
      // submenus only have id, type, label, and tooltip so this is checkalready
    case FLAG.EXISTING:
      // we don't know enough about existing objects to check them properly
    default:
      break;
    }
  }
}

//-----------------------------------------------------------------------------
/* The following functions check that the X attribute in the
 * param log object, aCheckObject, matches the X attribute of the subobject
 * of aCommandObject at index.
 * X is specified in the function name of the form assertX
 */
function assertType(aCommandObject, aCheckObject, aIndex)
{
  // we can't check properties of an existing command, because we don't
  // know what they should be.
  if (aCheckObject.CHECK_FLAG == FLAG.EXISTING)
  {
    return;
  }
  var type = aCommandObject.getCommandType(aCheckObject.CHECK_SUBMENUID,
                                          aIndex,
                                          aCheckObject.CHECK_HOST);
  assertEqual(type,
              aCheckObject.CHECK_FLAG,
              "Type should be " + aCheckObject.CHECK_FLAG + ", not " +
              type);
}
function assertId(aCommandObject, aCheckObject, aIndex)
{
  // we can't check properties of an existing command, because we don't
  // know what they should be.
  if (aCheckObject.CHECK_FLAG == FLAG.EXISTING)
  {
    return;
  }
  var id = aCommandObject.getCommandId(aCheckObject.CHECK_SUBMENUID,
                                      aIndex,
                                      aCheckObject.CHECK_HOST);
  assertEqual(id,
              aCheckObject.CHECK_ID,
              "Id should be " + aCheckObject.CHECK_ID + ", not " + id);
}
function assertLabel(aCommandObject, aCheckObject, aIndex)
{
  // we can't check properties of an existing command, because we don't
  // know what they should be.
  if (aCheckObject.CHECK_FLAG == FLAG.EXISTING)
  {
    return;
  }
  var label = aCommandObject.getCommandText(aCheckObject.CHECK_SUBMENUID,
                                           aIndex,
                                           aCheckObject.CHECK_HOST);
  assertEqual(label,
              aCheckObject.CHECK_LABEL,
              "Label should be " + aCheckObject.CHECK_LABEL + ", not " +
               label);

}
function assertTooltip(aCommandObject, aCheckObject, aIndex)
{
  // we can't check properties of an existing command, because we don't
  // know what they should be.
  if (aCheckObject.CHECK_FLAG == FLAG.EXISTING)
  {
    return;
  }
  var tooltip = aCommandObject.getCommandToolTipText(aCheckObject.CHECK_SUBMENUID,
                                                    aIndex,
                                                    aCheckObject.CHECK_HOST);
  assertEqual(tooltip,
              aCheckObject.CHECK_TOOLTIP,
              "Tooltip should be " + aCheckObject.CHECK_TOOLTIP + ", not " +
               tooltip);
}

//-----------------------------------------------------------------------------

/* Checks that the enabled and visible callbacks for the subobject of
 * aCommandObject at aIndex return the correct values as specified by
 * aCheckObject */
function assertUICallbacks(aCommandObject, aCheckObject, aIndex)
{
  if ("CHECK_VIS" in aCheckObject)
  {
    var visible = aCommandObject.getCommandVisible(aCheckObject.CHECK_SUBMENUID,
                                                   aIndex,
                                                   aCheckObject.CHECK_HOST);
    assertEqual(visible,
                aCheckObject.CHECK_VIS,
                "getCommandVisible should have returned " + aCheckObject.CHECK_VIS +
                 " instead of " + visible + "!");
  }

  if ("CHECK_ENABLED" in aCheckObject)
  {
    var enabled = aCommandObject.getCommandEnabled(aCheckObject.CHECK_SUBMENUID,
                                                  aIndex,
                                                  aCheckObject.CHECK_HOST);
    assertEqual(enabled,
                aCheckObject.CHECK_ENABLED,
                "getCommandEnabled should have returned " +
                 aCheckObject.CHECK_ENABLED + " instead of " + enabled + "!");
  }
}

/* Checks that the keyboard shortcuts for the subobject of aCommandObject at
 * aIndex are correctly reported, matching the values in aCheckObject */
function assertShortcuts(aCommandObject, aCheckObject, aIndex)
{
  if ("CHECK_SHORTCUT" in aCheckObject)
  {
    var key = aCommandObject.getCommandShortcutKey(aCheckObject.CHECK_SUBMENUID,
                                                   aIndex,
                                                   aCheckObject.CHECK_HOST);
    var code = aCommandObject.getCommandShortcutKeycode
                             (aCheckObject.CHECK_SUBMENUID,
                              aIndex,
                              aCheckObject.CHECK_HOST);
    var mod = aCommandObject.getCommandShortcutModifiers
                            (aCheckObject.CHECK_SUBMENUID,
                             aIndex,
                             aCheckObject.CHECK_HOST);
    var local = aCommandObject.getCommandShortcutLocal
                              (aCheckObject.CHECK_SUBMENUID,
                               aIndex,
                               aCheckObject.CHECK_HOST);
    assertEqual(key,
                "key" + aIndex,
                "getCommandShortcutKey should have returned " +
                 ("key" + aIndex) + ", instead of " + key + "!");
    assertEqual(code,
                "keycode" + aIndex,
                "getCommandShortcutKeycode should have returned " +
                 ("keycode" + aIndex) + ", instead of " + code + "!");
    assertEqual(mod,
                "modifier" + aIndex,
                "getCommandShortcutModifiers should have returned " +
                 ("modifier" + aIndex) + ", instead of " + mod + "!");
    assertEqual(local,
                aCheckObject.CHECK_SHORTCUT,
                "getCommandShortcutLocal should have returned " +
                 aCheckObject.CHECK_SHORTCUT + ", instead of " + local+ "!");
  }
}

/* Checks that the get value callback for the subobject of aCommandObject at
 * aIndex returns the correct value, matching the value saved in aCheckObject.
 * The value callback was specified by makeValueCallback */
function assertValueCallback(aCommandObject, aCheckObject, aIndex)
{
  var getval = aCommandObject.getCommandValue(aCheckObject.CHECK_SUBMENUID,
                                             aIndex,
                                             aCheckObject.CHECK_HOST);
  /* Some callbacks are setup to return booleans while others return strings
   * This check can handle both if we stringify any boolean returned */
  getval = String(getval);
  aCheckObject.CHECK_VALUECALLBACK = String(aCheckObject.CHECK_VALUECALLBACK);

  assertEqual(getval,
              aCheckObject.CHECK_VALUECALLBACK,
              "getCommandValue should have returned " +
               aCheckObject.CHECK_VALUECALLBACK + ", instead of " + getval +
               "!");
}

/* Checks that the set value callback for the subobject of aCommandObject at
 * aIndex can alter gValueChecker properly.
 * The value setter was specified by makeValueSetter */
function assertValueSetter(aCommandObject, aCheckObject, aIndex)
{
  gValueChecker = null;
  aCommandObject.onCommand(aCheckObject.CHECK_SUBMENUID,
                           aIndex,
                           aCheckObject.CHECK_HOST,
                           aCheckObject.CHECK_ID,
                           "setvalue" + aCheckObject.CHECK_ID);
  assertEqual(gValueChecker,
              "setvalue" + aCheckObject.CHECK_ID,
              "onCommand failed to trigger a value setter!");
}

/* Checks the choicemenuitems of a choice menu subobject to ensure that
 * they match the recorded values in aCheckObject. */
function assertChoiceMenuItems(aCommandObject, aCheckObject, aIndex)
{
  // CHECK_CHOICEMENUITEMS is a mini-log that corresponds to the choicemenuitems
  var checkItemArray = aCheckObject.CHECK_CHOICEMENUITEMS;

  // set the submenu to be the choicemenu containing the choicemenuitems
  var choiceMenuId = checkItemArray[0].CHECK_SUBMENUID;

  // ensure there are the expected number of choicemenuitems
  assertNumCommands(aCommandObject,
                    choiceMenuId,
                    checkItemArray.length);

  // use the checkItemArray minilog to check the contents of the choicemenuitems
  for (var i = 0; i < checkItemArray.length; i++)
  {
    var currItemCheckObject = checkItemArray[i];
    assertType(aCommandObject, currItemCheckObject, i);
    assertId(aCommandObject, currItemCheckObject, i);
    assertLabel(aCommandObject, currItemCheckObject, i);
    assertTooltip(aCommandObject, currItemCheckObject, i);
    assertTriggerCallback(aCommandObject, currItemCheckObject, i);
  }
}

/* Checks the actions appended to a sub-sbIPlaylistCommands object, itself under
 * aCommandObject at aIndex.  */
function assertSubActions(aCommandObject, aCheckObject, aIndex)
{
  var subcmd = aCommandObject.getCommandSubObject(aCheckObject.CHECK_SUBMENUID,
                                                  aIndex,
                                                  aCheckObject.CHECK_HOST);
  assertTrue((subcmd),
             "getCommandSubObject should not have returned null!");

  // CHECK_SUBACTIONS is a mini-log that corresponds to the subactions
  var checkItemArray = aCheckObject.CHECK_SUBACTIONS;

  // ensure there are the expected number of actions under the subcommand
  assertNumCommands(subcmd,
                    null,
                    checkItemArray.length);

  // use the checkItemArray minilog to check the contents of the subactions
  for (var i = 0; i < aCheckObject.CHECK_SUBACTIONS; i++)
  {
    assertType(subcmd, aCheckObject.CHECK_SUBACTIONS[i], i);
    assertId(subcmd, aCheckObject.CHECK_SUBACTIONS[i], i);
    assertLabel(subcmd, aCheckObject.CHECK_SUBACTIONS[i], i);
    assertTooltip(subcmd, aCheckObject.CHECK_SUBACTIONS[i], i);
  }
}

/* Checks that the action trigger callback for the subobject of aCommandObject
 * at aIndex can alter gTriggered properly.
 * The trigger callback was specified by makeTriggerCallback */
function assertTriggerCallback(aCommandObject, aCheckObject, aIndex)
{
  gTriggered = false;
  var host = (typeof(aCheckObject.CHECK_HOST) == "undefined") ?
             "" : aCheckObject.CHECK_HOST;
  var unusedVal = null;
  aCommandObject.onCommand(aCheckObject.CHECK_SUBMENUID,
                           aIndex,
                           host,
                           aCheckObject.CHECK_ID,
                           unusedVal);
  assertTrue(gTriggered, "onCommand failed to trigger an action!");
}

/* Checks that the instantiation callback for the subobject of aCommandObject
 * at aIndex returns the dummy node.
 * The instantiation callback was specified by makeInstantiationCallback */
function assertInstantiate(aCommandObject, aCheckObject, aIndex)
{
  var element = aCommandObject.instantiateCustomCommand
                              (aCheckObject.CHECK_SUBMENUID,
                               aIndex,
                               aCheckObject.CHECK_HOST,
                               aCheckObject.CHECK_ID,
                               dummyDocument);
  element = element.QueryInterface(Ci.nsIDOMNode);
  assertTrue((element),
             "instantiateCustomCommand should have returned dummyNode!");
}

/* Checks that the refresh callback for the subobject of aCommandObject
 * at aIndex returns the can correctly modify gRefreshed.
 * The instantiation callback was specified by makeRefreshCallback */
function assertRefresh(aCommandObject, aCheckObject, aIndex)
{
  gRefreshed = false;
  aCommandObject.refreshCustomCommand(aCheckObject.CHECK_SUBMENUID,
                                      aIndex,
                                      aCheckObject.CHECK_HOST,
                                      aCheckObject.CHECK_ID,
                                      dummyNode);
  assertTrue(gRefreshed,
             "refreshCustomCommand did not trigger a refresh!");
}

/* Checks that the number of commands in cmds, under the submenu specified by
 * aMenuID, is equal to the param num */
function assertNumCommands(aCmds, aMenuID, aNum)
{
  // the host string, "test", is unimportant here
  var actual = aCmds.getNumCommands(aMenuID, "test");
  assertEqual(actual,
              aNum,
              "Number of commands should be " + aNum + ", not " + actual + "!");
}

// ============================================================================
// UTILS
// ============================================================================

/* Utility method to display a log message that is informed of the submenu
 * that is currently relevant */
function _log(aMsg, aMenuID)
{
  log(gTestPrefix + " - " + aMsg + (aMenuID ? " (" + aMenuID + ")" : "") );
}

/* The correct log to use depends on the current submenu being investigated.
 * Usually, the gRootLog is the correct one (used whenever not within a
 * another commands submenu), but if we are in a submenu this function
 * finds the correct log and makes it the active one.
 */
function setLog(aMenuID)
{
  if (aMenuID == null)
  {
    gActiveCommandLog = gRootLog;
  }
  else {
    for (var i = 0; i < gActiveCommandLog.length; i++)
    {
      if (gActiveCommandLog[i].CHECK_ID == aMenuID)
      {
        if ((typeof(gActiveCommandLog[i].SUBMENU_LOG) == "undefined"))
        {
          gActiveCommandLog[i].SUBMENU_LOG = new Array();
        }
        gActiveCommandLog = gActiveCommandLog[i].SUBMENU_LOG;
      }
    }
  }
}

/* This is a utility function to uniquely generate an id, label, and tooltip for
 * a new command subobject.  The attributes are returned in an array of the form
 * [id, label, tooltip]. */
function getCommandData(type)
{
  var id = type + gIDCounter + "id";
  var label = type + gIDCounter + "label";
  var tooltip = type + gIDCounter + "tooltip"
  gIDCounter++;

  return [id, label, tooltip];
}
