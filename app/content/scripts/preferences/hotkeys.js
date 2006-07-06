// todo: internationalize 

var gHotkeysPane = {

  _list: null,
  _remove: null,
  _set: null,
  _action: null,
  _hotkey: null,
  _binding_enabled: null,
  _enabled: null,

  init: function ()
  {
    var jsLoader = Components.classes["@mozilla.org/moz/jssubscript-loader;1"].getService(Components.interfaces.mozIJSSubScriptLoader);
    jsLoader.loadSubScript( "chrome://songbird/content/scripts/messagebox.js", this );
    
    document.defaultView.addEventListener("unload", onHotkeysUnload, true);
    
    this._binding_enabled = SBDataBindElementAttribute("globalhotkeys.enabled", "hotkeys.enabled", "checked", true);

    this._list = document.getElementById("hotkey.list");
    this._set = document.getElementById("hotkey.set");
    this._remove = document.getElementById("hotkey.remove");
    this._action = document.getElementById("hotkey.actions");
    this._hotkey = document.getElementById("hotkey.hotkey");
    this._enabled = document.getElementById("hotkeys.enabled");

    this.loadHotkeys();
  },
  
  onUnload: function()
  {
    this._binding_enabled.unbind();
  },
  
  loadHotkeys: function()
  {
    var count = SBDataGetIntValue("globalhotkeys.count");
    for (var i=0;i<count;i++) {
      // Read hotkey binding from user preferences
      var root = "globalhotkey." + i + ".";
      var keycombo = SBDataGetStringValue(root + "key");
      var keydisplay = SBDataGetStringValue(root + "key.readable");
      var action = SBDataGetStringValue(root + "action");
      // make list items accordingly
      this._addItem(keycombo, keydisplay, action, -1);
    }
    this.updateButtons();
  },
  
  _addItem: function(keycombo, keydisplay, action, replace)
  {
    // if an index to replace was specified, figure out the corresponding item to use as a marker for insertBefore
    var before = (replace == -1) ? null : this._list.getItemAtIndex(replace);
    // make list item
    var listitem = document.createElement("listitem");
    listitem.keycombo = keycombo;
    var listcell_action = document.createElement("listcell");
    listcell_action.setAttribute("label", action);
    var listcell_keydisplay = document.createElement("listcell");
    listcell_keydisplay.setAttribute("label", keydisplay.toUpperCase());
    listitem.appendChild(listcell_action);
    listitem.appendChild(listcell_keydisplay);
    if (before) {
      // insert the item at the correct spot
      this._list.insertBefore(listitem, before);
      // and remove the item we were supposed to replace
      this._list.removeChild(before);
    } else {
      // append at the end of the list (replace was -1)
      this._list.appendChild(listitem);
    }
  },
  
  saveHotkeys: function()
  {
    // Save all hotkeys to prefs, extract them from the list itself
    var n = this._list.getRowCount();
    SBDataSetIntValue("globalhotkeys.count", n);
    for (var i=0;i<n;i++)
    {
      var root = "globalhotkey." + i + ".";
      var item = this._list.getItemAtIndex(i);
      var keycombo = item.keycombo;
      var actioncell = item.firstChild;
      var keydisplaycell = actioncell.nextSibling;
      var actionstr = actioncell.getAttribute("label");
      var keydisplay = keydisplaycell.getAttribute("label");
      SBDataSetStringValue(root + "key", keycombo);
      SBDataSetStringValue(root + "key.readable", keydisplay);
      SBDataSetStringValue(root + "action", actionstr);
    }
    SBDataFireEvent("globalhotkeys.changed");
  },
  
  onSelectHotkey: function()
  {
    this.updateButtons();
    var selected = this._list.selectedIndex;
    if (selected >= 0) {
      var item = this._list.getItemAtIndex(selected);
      var actioncell = item.firstChild;
      var keydisplaycell = actioncell.nextSibling;
      // load action in menulist
      var nodes = this._action.firstChild.childNodes;
      var actionstr = actioncell.getAttribute("label");
      for (var i=0;i<nodes.length;i++) {
        if (nodes[i].getAttribute("label") == actionstr) {
          this._action.selectedItem = nodes[i]
          break;
        }
      }
      // load hotkey in hotkey control
      this._hotkey.setHotkey(item.keycombo, keydisplaycell.getAttribute("label"));
    }
  },
  
  updateButtons: function()
  {
    // disable set & remove when no item is selected
    var disabled = (this._list.selectedIndex== -1);
    this._remove.setAttribute("disabled", disabled);
    this._set.setAttribute("disabled", disabled);
  },
  
  addHotkey: function()
  {
    // add the hotkey to the list
    var keycombo = this._hotkey.getHotkey(false);
    if (this._checkComboExists(keycombo, -1)) return;
    var keydisplay = this._hotkey.getHotkey(true);
    var action = this._action.selectedItem.getAttribute("label");
    if (action == "" || keycombo == "" || keydisplay == "") return;
    this._addItem(keycombo, keydisplay, action, -1);
    // select the last (newly added) item
    this._list.selectedIndex = this._list.getRowCount()-1;
    this.saveHotkeys();
  },
  
  setHotkey: function()
  {
    // change the hotkey item that's currently selected
    var selected = this._list.selectedIndex;
    var keycombo = this._hotkey.getHotkey(false);
    if (this._checkComboExists(keycombo, selected)) return;
    var keydisplay = this._hotkey.getHotkey(true);
    var action = this._action.selectedItem.getAttribute("label");
    if (action == "" || keycombo == "" || keydisplay == "") return;
    this._addItem(keycombo, keydisplay, action, selected);
    // reselect the item after it's been changed
    this._list.selectedIndex = selected;
    this.saveHotkeys();
  },
  
  _checkComboExists: function(keycombo, ignoreentry)
  {
    // checks wether the key combination already exists
    for (var i=0;i<this._list.getRowCount();i++) 
    {
      if (i == ignoreentry) continue;
      var item = this._list.getItemAtIndex(i);
      if (item.keycombo == keycombo) {
        this.sbMessageBox_strings("hotkeys.hotkeyexists.title", "hotkeys.hotkeyexists.msg", "Hotkey", "This hotkey is already taken", false);
        return true;
      }
    }
    return false;
  },
  
  removeHotkey: function()
  {
    // remove the selected item from the list
    var index = this._list.selectedIndex;
    var item = this._list.getItemAtIndex(index);
    this._list.removeChild(item);
    // select the next item, or the previous one if that was the last
    if (index >= this._list.getRowCount()) index = this._list.getRowCount()-1;
    this._list.selectedIndex = index;
    this.saveHotkeys();
  },
  
  onEnableDisable: function()
  {
    SBDataSetBoolValue("globalhotkeys.enabled", (this._enabled.getAttribute("checked") == "true"));
  }
  
};

function onHotkeysUnload() 
{
  gHotkeysPane.onUnload();
}

