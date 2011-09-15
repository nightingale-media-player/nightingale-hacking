/**
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

Components.utils.import("resource://app/jsmodules/sbSmartMediaListColumnSpecUpdater.jsm");

var sbILocalDatabaseSmartMediaList =
  Ci.sbILocalDatabaseSmartMediaList;

var SB_NS = "http://songbirdnest.com/data/1.0#";
var SB_PROPERTY_UILIMITTYPE = SB_NS + "uiLimitType";

var USECS_PER_MINUTE = 60 * 1000 * 1000;
var USECS_PER_HOUR   = USECS_PER_MINUTE * 60;
var BYTES_PER_MB     = 1000 * 1000;
var BYTES_PER_GB     = BYTES_PER_MB * 1000;

var selectByList = [
  {
    value: "album",
    property: SB_NS + "albumName",
    direction: true
  },
  {
    value: "artist",
    property: SB_NS + "artistName",
    direction: true
  },
  {
    value: "genre",
    property: SB_NS + "genre",
    direction: true
  },
  {
    value: "title",
    property: SB_NS + "trackName",
    direction: true
  },
  {
    value: "high_rating",
    property: SB_NS + "rating",
    direction: false
  },
  {
    value: "low_rating",
    property: SB_NS + "rating",
    direction: true
  },
  {
    value: "most_recent",
    property: SB_NS + "lastPlayTime",
    direction: false
  },
  {
    value: "least_recent",
    property: SB_NS + "lastPlayTime",
    direction: true
  },
  {
    value: "most_often",
    property: SB_NS + "playCount",
    direction: false
  },
  {
    value: "least_often",
    property: SB_NS + "playCount",
    direction: true
  },
  {
    value: "most_added",
    property: SB_NS + "created",
    direction: false
  },
  {
    value: "least_added",
    property: SB_NS + "created",
    direction: true
  }
];

function updateOkButton() {
  var dialog = document.documentElement;
  var ok = dialog.getButton("accept");
  ok.disabled = !isConfigurationValid();
}

function updateMatchControls() {
  var check = document.getElementById("smart_match_check");
  var checkwhat = document.getElementById("smart_any_list");
  var following = document.getElementById("smart_following_label");
  
  var smartConditions = document.getElementById("smart_conditions");
  var conditions = smartConditions.conditions;
  
  if (conditions.length == 1) {
    check.setAttribute("label", check.getAttribute("labelsingle"));
    checkwhat.hidden = true;
    following.hidden = true;
  } else {
    check.setAttribute("label", check.getAttribute("labelmultiple"));
    checkwhat.hidden = false;
    following.hidden = false;
  }
}

function onAddCondition() {
  updateMatchControls();
  updateOkButton();
}

function onRemoveCondition() {
  updateMatchControls();
  updateOkButton();
}

function onUserInput() {
  updateOkButton();
}

function doLoad()
{
  setTimeout(loadConditions);

  var smartConditions = document.getElementById("smart_conditions");
  smartConditions.addEventListener("input",  onUserInput, false);
  smartConditions.addEventListener("select", onUserInput, false);
  smartConditions.addEventListener("additem", onAddCondition, false);
  smartConditions.addEventListener("removeitem", onRemoveCondition, false);

  var limit = document.getElementById("smart_songs_count");
  limit.addEventListener("blur",  onLimitBlur, false);
}

function doUnLoad()
{
  var smartConditions = document.getElementById("smart_conditions");
  smartConditions.removeEventListener("input",  onUserInput, false);
  smartConditions.removeEventListener("select", onUserInput, false);
  smartConditions.removeEventListener("additem", onAddCondition, false);
  smartConditions.removeEventListener("removeitem", onRemoveCondition, false);

  var limit = document.getElementById("smart_songs_count");
  limit.removeEventListener("blur",  onLimitBlur, false);
}

function loadConditions()
{
  var list = window.arguments[0];
  
  if (list instanceof sbILocalDatabaseSmartMediaList) {

    // Set up conditions
    var smartConditions = document.getElementById("smart_conditions");
    if (list.conditionCount > 0) {
      var conditions = [];
      for (var i = 0; i < list.conditionCount; i++) {
        var condition = list.getConditionAt(i);
        conditions.push({
          metadata: condition.propertyID,
          condition: condition.operator.operator,
          value: condition.leftValue,
          value2: condition.rightValue,
          unit: condition.displayUnit,
          listguid: list.guid,
          source: list.sourceLibraryGuid
        });
      }
      smartConditions.conditions = conditions;
    }
    else {
      smartConditions.newCondition();
    }

    // Set match type
    var matchSomething = document.getElementById("smart_match_check");
    var matchAnyAll = document.getElementById("smart_any_list");
    switch(list.matchType) {
      case sbILocalDatabaseSmartMediaList.MATCH_TYPE_ANY:
        matchAnyAll.value = "any";
        matchSomething.checked = true;
      break;
      case sbILocalDatabaseSmartMediaList.MATCH_TYPE_ALL:
        matchAnyAll.value = "all";
        matchSomething.checked = true;
      break;
      case sbILocalDatabaseSmartMediaList.MATCH_TYPE_NONE:
        matchAnyAll.value = "any";
        matchSomething.checked = false;
      break;
    }

    // Set limit.  Get the "ui" limit from a list property
    var uiLimitType = list.getProperty(SB_PROPERTY_UILIMITTYPE) || "songs";

    // Set the limit based on the ui limit.  Convert the units from the smart
    // playlist to the units needed to display the ui limit

    var limit = document.getElementById("smart_songs_check");
    var count = document.getElementById("smart_songs_count");
    var limitType = document.getElementById("smart_songs_list");
    if (list.limitType == sbILocalDatabaseSmartMediaList.LIMIT_TYPE_NONE) {
      limit.checked = false;
      count.value = "0";
      limitType.value = "songs";
    }
    else {
      var mismatch = false;
      switch(uiLimitType) {
        case "songs":
          if (list.limitType != sbILocalDatabaseSmartMediaList.LIMIT_TYPE_ITEMS) {
            mismatch = true;
            break;
          }
          count.value = list.limit;
        break;
        case "minutes":
          if (list.limitType != sbILocalDatabaseSmartMediaList.LIMIT_TYPE_USECS) {
            mismatch = true;
            break;
          }
          count.value = list.limit / USECS_PER_MINUTE;
        break;
        case "hours":
          if (list.limitType != sbILocalDatabaseSmartMediaList.LIMIT_TYPE_USECS) {
            mismatch = true;
            break;
          }
          count.value = list.limit / USECS_PER_HOUR;
        break;
        case "MB":
          if (list.limitType != sbILocalDatabaseSmartMediaList.LIMIT_TYPE_BYTES) {
            mismatch = true;
            break;
          }
          count.value = list.limit / BYTES_PER_MB;
        break;
        case "GB":
          if (list.limitType != sbILocalDatabaseSmartMediaList.LIMIT_TYPE_BYTES) {
            mismatch = true;
            break;
          }
          count.value = list.limit / BYTES_PER_GB;
        break;
      }
      if (mismatch) {
        limit.checked = false;
        count.value = "0";
        limitType.value = "songs";
      }
      else {
        limit.checked = true;
        limitType.value = uiLimitType;
      }
    }

    // Set select by
    var selectBy = document.getElementById("smart_selected_list");
    if (list.randomSelection) {
      selectBy.value = "random";
    }
    else {
      var value = getValueForSelectBy(list);
      selectBy.value = value;
    }

    // Set autoupdate
    var autoUpdate = document.getElementById("smart_autoupdate_check");
    autoUpdate.checked = 
      list.autoUpdate == true;

  } else { // if (list instanceof smart)
    
    // defaults for new lists
    
    var smartConditions = document.getElementById("smart_conditions");

    smartConditions.newCondition();

    var matchSomething = document.getElementById("smart_match_check");
    var matchAnyAll = document.getElementById("smart_any_list");

    matchAnyAll.value = "all";
    matchSomething.checked = true;

    var limit = document.getElementById("smart_songs_check");
    var count = document.getElementById("smart_songs_count");
    var limitType = document.getElementById("smart_songs_list");

    limit.checked = false;
    count.value = "25";
    limitType.value = "songs";

    var selectBy = document.getElementById("smart_selected_list");
    selectBy.value = "artist";
    
    var autoUpdate = document.getElementById("smart_autoupdate_check");
    autoUpdate.checked = true;
  }

  // immediately update the match controls, so we don't have to wait for the drawer items
  updateMatchControls();
  // immediately update ok button, in case one of the loaded setting was actually invalid
  updateOkButton();
}

function isConfigurationValid() {
  var smartConditions = document.getElementById("smart_conditions");
  var check = document.getElementById("smart_match_check");
  var check_limit = document.getElementById("smart_songs_check");
  var count = document.getElementById("smart_songs_count");
  var check_match = document.getElementById("smart_match_check");
  return ((smartConditions.isValid || !check.checked) &&
         (!check_limit.checked || parseInt(count.value) > 0) &&
         (check_limit.checked || check_match.checked));
}

function doOK()
{
  checkLimit();
  if (!isConfigurationValid())
    return true;

  var smart_conditions = document.getElementById("smart_conditions");
  var conditions = smart_conditions.conditions;
  var check = document.getElementById("smart_match_check");

  // the rules themselves are valid, but some values do not make sense to
  // make playlists for (eg. contains "" ?), so we take care of this here,
  // by showing those fields as invalid after the user clicks ok.
  if (check.checked &&
      !testAdditionalRestrictions(conditions, smart_conditions)) {
    updateOkButton();
    return false;
  }

  // Get the smart playlist, creating one if necessary
  var list = window.arguments[0];
  var paramObject = null;
  var newSmartPlaylist = null;
  if (!(list instanceof sbILocalDatabaseSmartMediaList)) {
    // Get the window parameters object
    paramObject = list;

    // If a new smart playlist has already been created, use it.  Otherwise,
    // create a new one.
    if (paramObject.newSmartPlaylist)
      list = paramObject.newSmartPlaylist;
    else
      list = paramObject.newPlaylistFunction();
    newSmartPlaylist = list;
  }

  // Try configuring the smart playlist
  var success;
  try {
    success = configureList(list);
  } catch (ex) {
    success = false;
  }

  // Remove new smart playlist on failure
  if (!success && newSmartPlaylist) {
    newSmartPlaylist.library.remove(newSmartPlaylist);
    newSmartPlaylist = null;
  }

  // Return new smart playlist
  if (paramObject)
    paramObject.newSmartPlaylist = newSmartPlaylist;

  return success;
}

function configureList(list)
{
  var pm = Components
             .classes["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
             .getService(Ci.sbIPropertyManager);

  // Save conditions
  var smart_conditions = document.getElementById("smart_conditions");
  var conditions = smart_conditions.conditions;
    
  list.clearConditions();
  var check = document.getElementById("smart_match_check");
  if (check.checked) {
    conditions.forEach(function(condition) {
      var info = pm.getPropertyInfo(condition.metadata);
      // access specialized operators
      switch (info.type) {
        case "datetime":
          info.QueryInterface(Ci.sbIDatetimePropertyInfo);
          break;
        case "boolean":
          info.QueryInterface(Ci.sbIBooleanPropertyInfo);
          break;
      }
      var op = info.getOperator(condition.condition);
      var unit;
      var leftValue;
      var rightValue;
      if (op.operator != info.OPERATOR_ISTRUE &&
          op.operator != info.OPERATOR_ISFALSE &&
          op.operator != info.OPERATOR_ISSET &&
          op.operator != info.OPERATOR_ISNOTSET)
        leftValue = condition.value;
      if (op.operator == info.OPERATOR_BETWEEN ||
          op.operator == info.OPERATOR_BETWEENDATES)
        rightValue = condition.value2;
      if (condition.useunits)
        unit = condition.unit;
      list.appendCondition(condition.metadata,
                           op,
                           leftValue,
                           rightValue,
                           unit);
    });
  }
    
  // Save match
  var matchSomething = document.getElementById("smart_match_check");
  var matchAnyAll = document.getElementById("smart_any_list");
  if (matchSomething.checked) {
    if (matchAnyAll.value == "all") {
      list.matchType = sbILocalDatabaseSmartMediaList.MATCH_TYPE_ALL;
    }
    else {
      list.matchType = sbILocalDatabaseSmartMediaList.MATCH_TYPE_ANY;
    }
  }
  else {
   list.matchType = sbILocalDatabaseSmartMediaList.MATCH_TYPE_NONE;
  }

  // Save limit
  var limit = document.getElementById("smart_songs_check");
  var count = document.getElementById("smart_songs_count");
  var limitType = document.getElementById("smart_songs_list");

  list.setProperty(SB_PROPERTY_UILIMITTYPE, limitType.value);

  if (limit.checked) {
    switch(limitType.value) {
      case "songs":
        list.limitType = sbILocalDatabaseSmartMediaList.LIMIT_TYPE_ITEMS;
        list.limit = count.value;
      break;
      case "minutes":
        list.limitType = sbILocalDatabaseSmartMediaList.LIMIT_TYPE_USECS;
        list.limit = count.value * USECS_PER_MINUTE;
      break;
      case "hours":
        list.limitType = sbILocalDatabaseSmartMediaList.LIMIT_TYPE_USECS;
        list.limit = count.value * USECS_PER_HOUR;
      break;
      case "MB":
        list.limitType = sbILocalDatabaseSmartMediaList.LIMIT_TYPE_BYTES;
        list.limit = count.value * BYTES_PER_MB;
      break;
      case "GB":
        list.limitType = sbILocalDatabaseSmartMediaList.LIMIT_TYPE_BYTES;
        list.limit = count.value * BYTES_PER_GB;
      break;
    }
  }
  else {
    list.limitType = sbILocalDatabaseSmartMediaList.LIMIT_TYPE_NONE;
    list.limit = 0;
  }

  // Save select by
  var selectBy = document.getElementById("smart_selected_list");
  if (selectBy.value == "random") {
    list.randomSelection = true;
    list.selectPropertyID = "";
  }
  else {
    list.randomSelection = false;
    setSelectBy(list, selectBy.value);
  }

  var autoUpdate = document.getElementById("smart_autoupdate_check");
  list.autoUpdate = autoUpdate.checked;
    
  SmartMediaListColumnSpecUpdater.update(list);

  list.rebuild();
  
  return true;
}

function doCancel() {
  return true;
}

function getValueForSelectBy(list) {

  var value;
  selectByList.forEach(function(e) {
    if (e.property == list.selectPropertyID &&
        e.direction == list.selectDirection) {
      value = e.value;
    }
  });
  
  if (!value) value = "random";

  return value;
}

function setSelectBy(list, value) {

  selectByList.forEach(function(e) {
    if (e.value == value) {
      list.selectPropertyID = e.property;
      list.selectDirection = e.direction;
    }
  });

}

// we're not using radio controls because both checkboxes can be checked
// at the same time, but we have to keep at least one of them checked at
// all times

function onCheckMatch(evt) {
  updateOkButton();
}

function onCheckLimit(evt) {
  var check_limit = document.getElementById("smart_songs_check");
  if (check_limit.checked) {
    checkIfCanAutoUpdate();
  }
  updateOkButton();
}

function onSelectSelectedBy(evt) {
  checkIfCanAutoUpdate();
}

function checkIfCanAutoUpdate() {
  var selectBy = document.getElementById("smart_selected_list");
  var limit = document.getElementById("smart_songs_check");
  var autoUpdate = document.getElementById("smart_autoupdate_check");
  if (limit.checked && 
      selectBy.value == "random") {
    autoUpdate.checked = false;
    autoUpdate.disabled = true;
  } else {
    autoUpdate.removeAttribute("disabled");
  }
}

function testAdditionalRestrictions(aConditions, aConditionsDrawer) {
  var pm = Components.classes["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
                             .getService(Ci.sbIPropertyManager);
  var firstFailure = -1;
  for (var i=0; i<aConditions.length; i++) {
    var condition = aConditions[i];
    var info = pm.getPropertyInfo(condition.metadata)
    if (condition.condition == info.OPERATOR_CONTAINS ||
        condition.condition == info.OPERATOR_NOTCONTAINS ||
        condition.condition == info.OPERATOR_BEGINSWITH ||
        condition.condition == info.OPERATOR_NOTBEGINSWITH ||
        condition.condition == info.OPERATOR_ENDSWITH ||
        condition.condition == info.OPERATOR_NOTENDSWITH) {
      if (!condition.value) {
        aConditionsDrawer.makeInvalid(i);
        if (firstFailure == -1)
          firstFailure = i;
      }
    }
  }
  if (firstFailure != -1) {
    aConditionsDrawer.focusInput(firstFailure);
    return false;
  }
  return true;
}

function onLimitBlur(evt) {
  checkLimit();
}

function checkLimit() {
  var limit = document.getElementById("smart_songs_count");
  // Constrain the value to avoid conversion problems between huge numbers and
  // 64bits integers. Keep the value between 0 and 99,999,999,999,999, the
  // largest integer made of only '9's that fits in 64 bits. We could constrain
  // to 2^64 instead but that number (18446744073709551615) looks arbitrary to
  // the user and does not suggest to him that he's reached the maximum value.
  
  // For each limit type, this means the maximums are:
  
  // - By minutes:       190,258,752 years
  // - By hours:      11,415,525,114 years
  // - By megabytes:      95,367,432 petabytes
  // - By gigabytes:  97,656,250,000 petabytes
  // - By items:  99,999,999,999,999 items
  
  // This "ought to be enough for anybody".
  
  limit.value = Math.min(limit.value, 99999999999999);
  limit.value = Math.max(limit.value, 0);
}
