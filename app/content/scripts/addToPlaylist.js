/*
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

const VK_ESCAPE = 27;
const VK_ENTER = 13;

const PROPERTYKEY_ISLIST = "http://songbirdnest.com/data/1.0#isList";

const CONTRACTID_LIBRARYMANAGER      = "@songbirdnest.com/Songbird/library/Manager;1";
const CONTRACTID_PROMPTSERVICE       = "@mozilla.org/embedcomp/prompt-service;1";
const CONTRACTID_STRINGBUNDLESERVICE = "@mozilla.org/intl/stringbundle;1";

const STRINGKEY_NEWPLAYLIST_TITLE = "newPlaylist.title";
const STRINGKEY_NEWPLAYLIST_PROMPT = "newPlaylist.prompt";
const STRINGKEY_NEWPLAYLIST_DEFAULTNAME = "newPlaylist.defaultName";

const URL_SONGBIRD_PROPERTIES = "chrome://songbird/locale/songbird.properties";

const nsIDOMKeyEvent = Components.interfaces.nsIDOMKeyEvent;
const nsIPromptService = Components.interfaces.nsIPromptService;
const nsIStringBundleService = Components.interfaces.nsIStringBundleService;

const sbILibrary = Components.interfaces.sbILibrary;
const sbILibraryManager = Components.interfaces.sbILibraryManager;
const sbIMediaList = Components.interfaces.sbIMediaList;

function onLoad() {

  PushBackscanPause();

  var mainLibrary = getMainLibrary();
  
  var list = document.getElementById("medialist_listbox");
  
  var listener = {
    index: 1,

    onEnumerationBegin: function() {
      return true;
    },

    onEnumeratedItem: function(mediaList, item) {
      var listItem = document.createElement("listitem");
      listItem.setAttribute("label", item.name);
      listItem.value = item.guid;

      list.appendChild(listItem);

      return true;
    },

    onEnumerationEnd: function() {
      return true;
    }
  };
  
  mainLibrary.enumerateItemsByProperty(PROPERTYKEY_ISLIST, "1", listener,
                                       sbIMediaList.ENUMERATIONTYPE_SNAPSHOT);

  list.focus();
}

function onUnload() {
  PopBackscanPause();
}

function onKeyDown(event) {
  switch (event.keyCode) {
    case nsIDOMKeyEvent.DOM_VK_ESCAPE:
      onCancel();
      break;
    case nsIDOMKeyEvent.DOM_VK_ENTER:
      onOK();
      break;
  }
}

function enableOK() {
  document.getElementById("ok_button").removeAttribute("disabled");
}

function onOK() {
  var list = document.getElementById("medialist_listbox");
  
  window.arguments[0].mediaListGUID = list.selectedItem.value;
  
  onExit();
}

function onCancel() {
  onExit();
}

function onNewPlaylist()
{
  var promptService = Components.classes[CONTRACTID_PROMPTSERVICE]
                                .getService(nsIPromptService);

  var stringBundle = Components.classes[CONTRACTID_STRINGBUNDLESERVICE]
                               .getService(nsIStringBundleService)
                               .createBundle(URL_SONGBIRD_PROPERTIES);

  var input =
    {value: stringBundle.GetStringFromName(STRINGKEY_NEWPLAYLIST_DEFAULTNAME)};

  var title = stringBundle.GetStringFromName(STRINGKEY_NEWPLAYLIST_TITLE);
  var prompt = stringBundle.GetStringFromName(STRINGKEY_NEWPLAYLIST_PROMPT);

  if (promptService.prompt(window, title, prompt, input, null, {})) {
    var mainLibrary = getMainLibrary();
    
    var mediaList = mainLibrary.createMediaList("simple");
    mediaList.name = input.value;
    mediaList.write();
    
    var listItem = document.createElement("listitem");
    listItem.setAttribute("label", mediaList.name);
    listItem.value = mediaList.guid;

    var list = document.getElementById("medialist_listbox");
    list.appendChild(listItem);
    
    list.ensureElementIsVisible(listItem);
    list.selectItem(listItem);
    list.focus();
  }
}

function getMainLibrary() {
  var libraryManager = Components.classes[CONTRACTID_LIBRARYMANAGER]
                                 .getService(sbILibraryManager);
  return libraryManager.mainLibrary;
}
