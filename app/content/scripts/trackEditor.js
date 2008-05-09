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


/******************************************************************************
 *
 * \file trackEditor.js 
 * \brief Dialog and UI controllers for viewing and modifying metadata
 *        associated with the main window sbIMediaListViewSelection
 *
 *****************************************************************************/


if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;

Components.utils.import("resource://app/jsmodules/SBJobUtils.jsm");
Components.utils.import("resource://app/components/sbProperties.jsm");



/******************************************************************************
 *
 * \class TrackEditorState 
 * \brief Wraps an sbIMediaListViewSelection interface, adding accessors
 *        and listener support.
 *
 * This class maintains the state of the track editor.  UI widgets read from, 
 * write to, and observe the instance of this class available at TrackEditor.state.
 *
 * Since the track editor has no knowledge of the widgets extension authors 
 * are free to customize the UI.
 *
 *****************************************************************************/
function TrackEditorState() {
  this._propertyListeners = {};
  this._selectionListeners = [];
  this._properties = {};
}
TrackEditorState.prototype = {
  
  _propertyManager: Cc["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
                      .getService(Ci.sbIPropertyManager),
  
  // Array of media items that the track editor is operating on
  _selectedItems: null,
    
  // Internal data structure
  // Example:
  // {
  //   propertyName: { 
  //     hasMultiple: false,
  //     edited: false,
  //     value: "",
  //     originalValue: ""
  //   }
  // }
  _properties: null,
  
  // Cached count of items with userEditable == true
  _writableItemCount: null,
  
  // Map of properties to listener arrays
  _propertyListeners: null,
  
  // Array of listeners to be notified only when the selection is reinited
  _selectionListeners: null,
  
  /** 
   * Initialize the state object around an sbIMediaListViewSelection.
   * May be called again to reinitialize with a new selection.
   */
  setSelection: function TrackEditorState_setSelection(mediaListSelection) {
    this._selectedItems = [];
    var items = mediaListSelection.selectedIndexedMediaItems;
    while (items.hasMoreElements()) {
      var item = items.getNext()
                      .QueryInterface(Components.interfaces.sbIIndexedMediaItem)
                      .mediaItem;
      this._selectedItems.push(item);
    }
    
    this._properties = {};
    this._writableItemCount = null;

    this._notifySelectionListeners();
    this._notifyPropertyListeners();
  },
  
  
  /**
   * If there is nothing to write, there is nothing to edit
   */
  get isDisabled() {    
    return this.writableItemCount == 0;
  },
  
  /**
   * Figure out how many items can be edited and cache the value
   */
  get writableItemCount() {
    if (this._writableItemCount == null) {    
      this._writableItemCount = 0;
      if (this._selectedItems && this._selectedItems.length > 0) {
        
        // TODO do we want to handle 200,000 selected tracks?
        for each (var item in this._selectedItems) {
          if (item.userEditable) {
            this._writableItemCount++;
          }
        }
      }
    }
    return this._writableItemCount;
  },
  
  /** 
   * JS array of sbIMediaItems that the track editor
   * is currently targeting
   */
  get selectedItems() {
    return this._selectedItems;
  },
  
  /** 
   * Get a formatted, displayable value for the given property
   */
  getPropertyValue: function(property) {
    this._ensurePropertyData(property);
    return this._properties[property].value;
  },

  /** 
   * Restore the original value for the given
   * property
   */  
  resetPropertyValue: function(property) {
    if (property in this._properties) {
      delete this._properties[property];
    }
    this._ensurePropertyData(property);
    this._notifyPropertyListeners(property);
  },
  
  /** 
   * Returns true if there is more than one value between the
   * selected media items for the given property.
   *
   * Note that this value will be false as soon as 
   * the property has been edited by the user.
   */
  hasMultipleValuesForProperty: function(property) {
    this._ensurePropertyData(property);
    return this._properties[property].hasMultiple;
  },
  
  /** 
   * Save/broadcast a new value for the given property.
   * Note that this does not write back to the selected media items.
   */
  setPropertyValue: function(property, value) {
    this._ensurePropertyData(property);
    this._properties[property].value = value;
    this._properties[property].edited = 
        this._properties[property].value != this._properties[property].originalValue;

    // Multiple values no longer matter
    this._properties[property].hasMultiple = false;
    
    this._notifyPropertyListeners(property);
  },
  
  /** 
   * Returns true if the value for the given property
   * differs from the original media item value
   */
  isPropertyEdited: function(property) {
    this._ensurePropertyData(property);
    return this._properties[property].edited;
  },
  
  /** 
   * Returns an array of the property names that have
   * been modified by the track editor
   */
  getEditedProperties: function() {
    var editedList = [];
    for (var propertyName in this._properties) {
      if (this._properties[propertyName].edited) {
        editedList.push(propertyName);
      }
    }
    return editedList;
  },
  
  /** 
   * Makes sure that the _properties hash contains a record for 
   * the given property
   */
  _ensurePropertyData: function TrackEditorState__ensurePropertyData(property) {
    // If the data has already been created, nothing to do
    if (property in this._properties) {
      return;
    }
    
    // Set up the empty structure
    this._properties[property] = {
      edited: false,
      hasMultiple: false,
      value: "",
      originalValue: ""
    }
    
    // Populate the structure
    if (this._selectedItems && this._selectedItems.length > 0) {      
      var value = this._selectedItems[0].getProperty(property);

      if (this._selectedItems.length > 1) {
        // Look through the selection to see if all items have the same value
        // for this property
        for each (var item in this._selectedItems) {
          if (value != item.getProperty(property)) {
            this._properties[property].hasMultiple = true;
            value = "";
            break;
          }
        }
      }

      // Format the string if needed
      // Note that on an image, format truncates, then returns!
      if (value && property != SBProperties.primaryImageURL &&
          !this._properties[property].hasMultiple) 
      {
        var propInfo = this._propertyManager.getPropertyInfo(property);
        
        // Formatting can fail. :(
        try { 
          value = propInfo.format(value); 
        }
        catch (e) { 
          Components.utils.reportError("TrackEditor::getPropertyValue("+property+") - "+value+": " + e +"\n");
        }
      }

      this._properties[property].value = value;
      this._properties[property].originalValue = value;
    }
  },
  
  /** 
   * Add an object to be notified when the editor state for the given
   * property changes.
   * 
   * The listener object must provide a onTrackEditorPropertyChange function
   */
  addPropertyListener: function(property, listener) {
    if (!("onTrackEditorPropertyChange" in listener)) {
      throw new Error("Listener must provide a onTrackEditorPropertyChange function");
    }
    
    if (!(property in this._propertyListeners)) {
      this._propertyListeners[property] = [listener];
    } else {
      this._propertyListeners[property].push(listener);
    }
  },
    
  /** 
   * Remove a previously added property listener
   */
  removePropertyListener: function(property, listener) {
    if (property in this._propertyListeners) {
      var array = this._propertyListeners[property];
      var index = array.indexOf(listener);
      if (index > -1) {
        array.splice(index,1);
      }
    }
  },
  
  /** 
   * Add an object to be notified when the editor is updated with a 
   * new selection state.
   * 
   * The listener object must provide a onTrackEditorSelectionChange function
   */
  addSelectionListener: function(listener) {
    if (!("onTrackEditorSelectionChange" in listener)) {
      throw new Error("Listener must provide a onTrackEditorSelectionChange function");
    }
    this._selectionListeners.push(listener);
  },
  
  /** 
   * Remove a previously added selection listener
   */
  removeSelectionListener: function(listener) {
    var index = this._selectionListeners.indexOf(listener);
    if (index > -1) {
      this._selectionListeners.splice(index,1);
    }
  },
  
  /** 
   * Broadcast change notification to listeners interested in the 
   * given property, or all if property is null
   */
  _notifyPropertyListeners: function TrackEditorState__notifyPropertyListeners(property) {
    // If no property was specified, notify everyone
    if (property == null) {
      for each (var listenerArray in this._propertyListeners) {
        for each (var listener in listenerArray) {
          listener.onTrackEditorPropertyChange(property);
        }
      }
    } else {
      // Otherwise just notify listeners interested in 
      // this specific property
      var listeners = this._propertyListeners[property];
      if (listeners) {
        for each (var listener in listeners) {
          listener.onTrackEditorPropertyChange(property);
        }
      }
    }
  },
  
  /** 
   * Broadcast the fact that a new selection has been set
   */
  _notifySelectionListeners: function TrackEditorState__notifySelectionListeners() {
    for each (var listener in this._selectionListeners) {
      listener.onTrackEditorSelectionChange();
    }
  },
}






/******************************************************************************
 *
 * \class TrackEditor 
 * \brief Base controller for track editor windows, including trackEditor.xul.
 *
 * Responsible for setting up default UI elements and maintaining the 
 * state object 
 *
 *****************************************************************************/
var TrackEditor = {
  
  
  _propertyManager: Cc["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
                      .getService(Ci.sbIPropertyManager),
             
  // TrackEditorState object
  state: null,
  
  // Hash of ID to widget objects
  _elements: {},
    
  // TODO consolidate?
  _browser: null,
  mediaListView: null,
  
  /**
   * Called when the dialog loads
   */
  onLoadTrackEditor: function TrackEditor_onLoadTrackEditor() {
    
    // Get the main window.
    var windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"]
                          .getService(Ci.nsIWindowMediator);

    var songbirdWindow = windowMediator.getMostRecentWindow("Songbird:Main"); 
    this._browser = songbirdWindow.gBrowser;
    
    this.state = new TrackEditorState();  
    
    // Prepare the UI
    this._setUpDefaultWidgets();
    
    // note that this code USED to watch tabContent and selection and library
    // changes, but we've implemented track editor as a modal dialog, so there's
    // no need for any of that now. still, the names are there, so it can be
    // brought back if the desire arises
    this.onTabContentChange();
  },
  
  /**
   * Find and configure known DOM elements
   */
  _setUpDefaultWidgets: function TrackEditor__setUpDefaultWidgets() {

    // TODO: profiling shows that setting the value of a textbox can
    // take up to 200ms.  Multiply that by 40 textboxes and performance
    // can suffer.  Do some more tests and determine the impact of
    // the advanced tab
    
    // Since the edit pane is all that is required for launch we  
    // are disabling all other panes.  They can be selectively
    // re-enabled once they've been polished.  
    
    // TODO/TEMP for now use the advanced editing pref to show 
    // the unpolished tabs
    
    // Set up the advanced tab
    var enableAdvanced = Application.prefs.getValue(
                           "songbird.trackeditor.enableAdvancedTab", false);
    if (enableAdvanced) {
      // this code assumes that the number of tabs and tabpanels is aligned
      var tabbox = document.getElementById("trackeditor-tabbox");
      this._elements.push(new TrackEditorAdvancedTab(tabbox));
    } else {
      // TODO remove this
      var tabBox = document.getElementById("trackeditor-tabbox");
      var tabs = tabBox.parentNode.getElementsByTagName("tabs")[0];
      tabBox.selectedIndex = 1;
      tabs.hidden = true;
    }
    
    // Add an additional layer of control to all elements with a property
    // attribute.  This will cause the elements to be updated when the
    // selection model changes, and vice versa.
    
    this._elements["anonymous"] = []; // Keep elements without ids in a list
    
    // Potentially anonymous elements that need wrapping
    var elements = document.getElementsByAttribute("property", "*");
    for each (var element in elements) {
      var wrappedElement = null;
      if (element.tagName == "label") {
        wrappedElement = new TrackEditorLabel(element);
      } else if (element.tagName == "textbox") {
        wrappedElement = new TrackEditorTextbox(element);
      } else if (element.tagName == "sb-rating") {
        wrappedElement = new TrackEditorRating(element);
      }
      
      if (wrappedElement) {
        if (element.id) {
          this._elements[element.id] = wrappedElement;
        } else {
          this._elements["anonymous"].push(wrappedElement);
        }
      }
    }
    
    // Known elements we're going to want to use.
    elements = ["notification_box", "notification_text",
                "prev-button", "next-button"];
    // I'd love to do this in the previous loop, but getElementsByAttribute
    // returns an HTMLCollection, not an array.
    for each (var elementName in elements) {
      var element = document.getElementById(elementName);
      if (element) {
        this._elements[elementName] = element;
      } 
    }

    this._elements["notification_box"].hidden = true;
  },
  
  
  /**
   * Show/hide warning messages as needed in the header of the dialog
   */
  _updateNotificationBox: function TrackEditor__updateNotificationBox() {
    this.state.selectedItems.length
    
    var itemCount = this.state.selectedItems.length;
    var writableCount = this.state.writableItemCount;
    
    var message;
    var class = "notification-warning";
    
    // TODO localize!
    if (itemCount > 1) {
      if (writableCount == itemCount) {
        message = "Editing metadata for " + itemCount + " media items.";
        class = "notification-info";
      } else if (writableCount >= 1) {
        message = "Metadata cannot be edited for " + (itemCount - writableCount) +
          " of the " + itemCount + " selected items.";
      } else {
        message = "Metadata for the selected items cannot be edited.";
      }
    } else if (writableCount == 0) {
      message = "Metadata for the selected item cannot be edited.";
    }
      
    if (message) {
      this._elements["notification_box"].className = class;
      this._elements["notification_text"].textContent = message;
      this._elements["notification_box"].hidden = false;
    } else {
      this._elements["notification_box"].hidden = true;
    }
  },

  /**
   * Called when the state of the main window browser changes.
   * TODO: or at least it would be if we were a non-modal dialog
   */
  onTabContentChange: function TrackEditor_onTabContentChange() {
    // We don't listen to nobody.
    //if(this.mediaListView) {
      //this.mediaListView.mediaList.removeListener(this);
    //}
    
    this.mediaListView = this._browser.currentMediaListView;
    
    //this.mediaListView.mediaList.addListener(this); 
    //we're assuming a modal dialog for now, so don't reflect changes.
    
    this.onSelectionChanged();
  },

  // TODO this isn't hooked up to anything!
  // TODO should be fixed, as playback may modify metadata while
  // we are editing it
  /*
  onItemUpdated: function(aMediaList, aMediaItem, aOldPropertiesArray) {
    // we have to treat this just like a selection change
    // so that multiple-selection values are correctly updated
    // this might actually be a punch in the face because there are a
    // a bunch of user-uneditable properties that change when playback starts
    // hmm    
    for (var i = 0; i < aOldPropertiesArray.length; i++) {
      // this is weak
      var property = aOldPropertiesArray.getPropertyAt(i);
      var propInfo = this._propertyManager.getPropertyInfo(property.id);
      
      //dump("Property change logged for "+aMediaItem.getProperty(SBProperties.trackName)+ ":\n" 
      //     + property.value + "\n" + aMediaItem.getProperty(property.id) 
      //     + "\neditable: " + propInfo.userEditable);
      if (property.value != aMediaItem.getProperty(property.id) && propInfo.userEditable) {
        this.onSelectionChanged();
        return;
      }
    }
  },*/
  
  /**
   * Called when the media item selection in the main player window changes
   */
  onSelectionChanged: function TrackEditor_onSelectionChanged() {
    
    // TODO build a data structure
    this.state.setSelection(this.mediaListView.selection);    
    
    // TODO consider a separate function
   
    // Special case the title and track number
    // It doesn't make sense to apply the same title and track number
    // to multiple tracks, so just disable the option. 
    // If the user really wants it they can use the advanced tab.
    var disable = this.state.selectedItems.length > 1;
    this._elements["infotab_trackname_textbox"].disabled = disable;
    this._elements["infotab_tracknumber_textbox"].disabled = disable;
 
    // DISABLE NEXT/PREV BUTTONS AT TOP/BOTTOM OF LIST
    // TODO: komi suggests wraparound as a pref
    var idx = this.mediaListView.selection.currentIndex;
    var atStart = (idx == 0)
    var atEnd   = (idx == this.mediaListView.length - 1);
    this._elements["prev-button"].setAttribute("disabled", (atStart ? "true" : "false"));
    this._elements["next-button"].setAttribute("disabled", (atEnd ? "true" : "false"));

    this._updateNotificationBox();
  },
  
  /**
   * Called when the media item selection in the main player window changes
   * TODO not being run at the moment?
   */
  onUnloadTrackEditor: function() {
    // break the cycles
    //this._browser.removeEventListener("TabContentChange", this, false);
    this.mediaListView.selection.removeListener(this);
    this.mediaListView.mediaList.removeListener(this);
    this.mediaListView = null;
  },

  /**
   * Called by the right arrow button.  Increments the selection
   * index in the main player window
   */
  next: function() {
    var idx = this.mediaListView.selection.currentIndex;

    if (idx == null || idx == undefined) { return; }
    
    idx = idx+1;
    if (idx >= this.mediaListView.length) {
      //idx = 0; // wrap around
      idx = this.mediaListView.length - 1 // bump
    }

    this.mediaListView.selection.selectOnly(idx);
    // called explicitly since we aren't watching
    this.onSelectionChanged();
  },

  /**
   * Called by the left arrow button.  Decrements the selection
   * index in the main player window
   */
  prev: function() {
    var idx = this.mediaListView.selection.currentIndex;

    if (idx == null || idx == undefined) { return; }
    
    idx = idx-1;
    if (idx < 0) {
      //idx = this.mediaListView.length - 1; // wrap around
      idx = 0; // bump
    }

    this.mediaListView.selection.selectOnly(idx);
    // called explicitly since we aren't watching
    this.onSelectionChanged();
  },
  
  /**
   * Purge any edits made by the user. 
   * TODO Currently unused.
   */
  reset: function() {
    // Reiniting with the existing selection will
    // refresh all UI
    onSelectionChanged();
  },

  /**
   * Write property changes back to the selected media items
   * and if possible to the on disk files.
   */  
  apply: function() {
    
    var properties = this.state.getEditedProperties();
    var items = this.state.selectedItems;
    if (items.length == 0 || properties.length == 0) {
      return;
    }
    
    // Apply each modified property back onto the selected items,
    // keeping track of which items have been modified
    var needsWriting = new Array(items.length);
    for each (property in properties) {
      for (var i = 0; i < items.length; i++) {
        var value = TrackEditor.state.getPropertyValue(property);
        var item = items[i];
        if (value != item.getProperty(property)) {
          item.setProperty(property, value);
          needsWriting[i] = true;
        }
      }
    }
      
    
  /* TODO: finish or nix this
    // isPartOfCompilation gets special treatment because
    // this is our only user-exposed boolean property right now
    // TODO: generalize this to be more like the textboxes above
    //       boy, it would be really nice if it were really boolean instead of
    //       a 1/0 clamped number...
    var property = SBProperties.isPartOfCompilation;
    var compilation = document.getElementsByAttribute("property", property)[0];
    if (compilation.checked) {
      // go through the list setting properties and queuing items
        var sIMI = this.mediaListView.selection.selectedIndexedMediaItems;
        var j = 0;
        while (sIMI.hasMoreElements()) {
          j++;          
          var mI = sIMI.getNext()
            .QueryInterface(Ci.sbIIndexedMediaItem)
            .mediaItem;

          if (mI.getProperty(property) != tb.value) {
            mI.setProperty(property, (tb.value ? "1" : "0"));
            needsWriting[j] = true; // keep track of these suckers
          }
        }
    }
  */
    
    // Add all items that need writing into an array 
    // and hand them off to the metadata manager
    var mediaItemArray = Cc["@mozilla.org/array;1"]
                        .createInstance(Ci.nsIMutableArray);
    for (var i = 0; i < items.length; i++) {
      if (needsWriting[i] && items[i].userEditable) {
        mediaItemArray.appendElement(items[i], false);
      }
    }
    if (mediaItemArray.length > 0) {
      var manager = Cc["@songbirdnest.com/Songbird/MetadataJobManager;1"]
                        .getService(Ci.sbIMetadataJobManager);
      var job = manager.newJob(mediaItemArray, 5, Ci.sbIMetadataJob.JOBTYPE_WRITE);
      
      SBJobUtils.showProgressDialog(job, window);
    }
  }
};





/******************************************************************************
 *
 * \class TrackEditorWidgetBase 
 * \brief Base wrapper for UI elements that need to observe a track editor
 *        property value
 *
 * To be wrapped by this function an element must have a property attribute
 * and .value accessor.
 *
 * Note that to add UI to the track editor you DO NOT need to use
 * this or other wrapper classes.  Use this, use XBL, use your own script, just
 * observe TrackEditor.state and post back user changes.
 *
 *****************************************************************************/
function TrackEditorWidgetBase(element) {
  this._element = element;
  
  TrackEditor.state.addPropertyListener(this.property, this);
}
TrackEditorWidgetBase.prototype = {  
  // The DOM element that this object is responsible for
  _element: null,
  
  get property() {
    return this._element.getAttribute("property");
  },
  
  get element() {
    return this._element;
  },
  
  get disabled() {
    return this._element.disabled;
  },
  
  set disabled(val) {
    this._element.disabled = val;
  },
  
  onTrackEditorPropertyChange: function TrackEditorWidgetBase_onTrackEditorPropertyChange() {
    var value = TrackEditor.state.getPropertyValue(this.property);
    if (value != this._element.value) {
      this._element.value = value;
    }
  }
}





/******************************************************************************
 *
 * \class TrackEditorLabel 
 * \brief Extends TrackEditorWidgetBase for label elements that should 
 *        reflect propert information
 *
 * If the label has a property-type="label" attribute it will receive the 
 * title associated with the property attribute. Otherwise the label will
 * receive the current editor display value for property.
 *
 *****************************************************************************/
function TrackEditorLabel(element) {
  
  // If requested, just show the title of the property
  if (element.hasAttribute("property-type") && 
      element.getAttribute("property-type") == "label") {
        
    var propMan = Cc["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
                   .getService(Ci.sbIPropertyManager);
    var propInfo = propMan.getPropertyInfo(element.getAttribute("property"));
    element.setAttribute("value", propInfo.displayName);   
    
  } else {
    // Otherwise, this label should show the value of the 
    // property, so call the parent constructor
    TrackEditorWidgetBase.call(this, element);
  }
}
TrackEditorLabel.prototype = {
  __proto__: TrackEditorWidgetBase.prototype,
}





/******************************************************************************
 *
 * \class TrackEditorInputWidget 
 * \brief Extends TrackEditorWidgetBase to provide a base for widgets that
 *        need to read from AND write back to the track editor state.
 *
 * Wraps the given element to manage the readonly attribute based on
 * track editor state and a checkbox.  Synchronizes readonly state between 
 * all input elements for a given property.
 * 
 * Assumes widgets with .value and .readonly accessors and a 
 * property attribute.
 *
 *****************************************************************************/
function TrackEditorInputWidget(element) {
  
  // Otherwise, this label should show the value of the 
  // property, so call the parent constructor
  TrackEditorWidgetBase.call(this, element);  
  
  TrackEditor.state.addSelectionListener(this);
  
  // Create preceding checkbox to enable/disable when 
  // multiple tracks are selected
  this._createCheckbox();
}
TrackEditorInputWidget.prototype = {
  __proto__: TrackEditorWidgetBase.prototype, 
  
  // Checkbox used to enable/disable the input widget
  // when multiple tracks are selected
  _checkbox: null,
  
  _createCheckbox: function() {
    var hbox = document.createElement("hbox");
    this._element.parentNode.replaceChild(hbox, this._element);
    this._checkbox = document.createElement("checkbox");
    
    // In order for tabbing to work in the desired order
    // we need to apply tabindex to all elements.
    if (this._element.hasAttribute("tabindex")) {
      var value = parseInt(this._element.getAttribute("tabindex")) - 1;
      this._checkbox.setAttribute("tabindex", value);
    }
    this._checkbox.hidden = true;

    var self = this;
    this._checkbox.addEventListener("command", 
      function() { self.onCheckboxCommand(); }, false);
    
    hbox.appendChild(this._checkbox);
    hbox.appendChild(this._element);
  },
  
  set disabled(val) {
    this._checkbox.disabled = val;
    this._element.disabled = val;
  },
  
  onCheckboxCommand: function() {
    if (this._checkbox.checked) {
      this._element.disabled = false;
      
      if (TrackEditor.state.hasMultipleValuesForProperty(this.property)) {
        // Clear out the *Various* text
        TrackEditor.state.setPropertyValue(this.property, "");
      }
      
      this._element.focus();
    } else {
      TrackEditor.state.resetPropertyValue(this.property);
      this._checkbox.checked = false;
      this._element.disabled = true;
    }
  },
  
  onTrackEditorSelectionChange: function() {
    this._checkbox.hidden = TrackEditor.state.selectedItems.length <= 1; 
    
    // If none of the selected items can be modified, disable all editing
    if (TrackEditor.state.isDisabled) {
      this._checkbox.disabled = true;
      this._element.setAttribute("readonly", "true");
    } else {
      this._checkbox.disabled = false;
      this._element.disabled = false;
      this._element.removeAttribute("readonly");
    }
  },
  
  onTrackEditorPropertyChange: function TrackEditorInputWidget_onTrackEditorPropertyChange() {
    
    TrackEditorWidgetBase.prototype.onTrackEditorPropertyChange.call(this);
    
    this._checkbox.checked = !TrackEditor.state.hasMultipleValuesForProperty(this.property);
    if (!this._checkbox.checked) {
      this._element.disabled = true;
    } else if (!this._checkbox.disabled) { 
      this._element.disabled = false;
    }
  }
}






/******************************************************************************
 *
 * \class TrackEditorTextbox 
 * \brief Extends TrackEditorInputWidget to add textbox specific details
 *
 * Binds the given textbox to track editor state for a property, and mananges
 * input, updates, edited attribute, and autocomplete configuration.
 *
 *****************************************************************************/
function TrackEditorTextbox(element) {
  
  TrackEditorInputWidget.call(this, element);  
  
  var self = this;
  element.addEventListener("input",
          function() { self.onUserInput(); }, false);
}
TrackEditorTextbox.prototype = {
  __proto__: TrackEditorInputWidget.prototype,
  
  onUserInput: function() {
    TrackEditor.state.setPropertyValue(this.property, this._element.value);
    
    // TODO VALIDATION!
    // When to validate? oninput? onchange?
    /*
    if (elt.value != "" && !propInfo.validate(elt.value)) {
      elt.reset();
    }*/
  },
  
  onTrackEditorSelectionChange: function TrackEditorTextbox_onTrackEditorSelectionChange() {
    TrackEditorInputWidget.prototype.onTrackEditorSelectionChange.call(this);

    if (this._element.getAttribute("type") == "autocomplete") {
      this._configureAutoComplete();
    }
  },
  
  onTrackEditorPropertyChange: function TrackEditorTextbox_onTrackEditorPropertyChange() {
    var property = this.property;
    
    // Indicate if this property has been edited
    if (TrackEditor.state.isPropertyEdited(this.property)) {
      if (!this._element.hasAttribute("edited")) {
        this._element.setAttribute("edited", "true");
      }
    } else if (this._element.hasAttribute("edited")) {
      this._element.removeAttribute("edited"); 
    } 

    TrackEditorInputWidget.prototype.onTrackEditorPropertyChange.call(this);
  },
  
  _configureAutoComplete: function TrackEditorTextbox__configureAutoComplete() {
    // Set the autocompletesearchparam attribute of the
    // autocomplete textbox to 'property;guid', where
    // property is the mediaitem property that the textbox
    // edits, and guid is the guid of the currently displayed
    // library. We could omit the guid and set only the property
    // in the xul, but the suggestions would then come
    // from all the libraries in the system instead of only
    // the one whom the displayed list belongs to.
    // In addition, textboxes that have a defaultdistinctproperties
    // attribute need to have that value appended to the
    // search param attribute as well. 
    if (!TrackEditor.mediaListView) 
      return;
    var library = TrackEditor.mediaListView.mediaList.library;
    if (!library) 
      return;
    var libraryGuid = library.guid;

    // verify that this is an autocomplete textbox, and that
    // it belongs to us: avoid changing the param for a
    // textbox that autocompletes to something else than
    // distinct props. Note that we look for the search id
    // anywhere in the attribute, because we could be getting
    // suggestions from multiple suggesters
    if (this._element.getAttribute("autocompletesearch")
                     .indexOf("library-distinct-properties") >= 0) {
      var defvals = this._element.getAttribute("defaultdistinctproperties");
      var param = this.property + ";" + libraryGuid;
      if (defvals && defvals != "") {
        param += ";" + defvals;
      }
      this._element.setAttribute("autocompletesearchparam", param);       
    }
    
    // Grr, autocomplete textboxes don't handle tabindex, so we have to 
    // get out hands dirty.  Filed as Moz Bug 432886.
    this._element.inputField.tabIndex = this._element.tabIndex;
  }
}






/******************************************************************************
 *
 * \class TrackEditorRating
 * \brief Extends TrackEditorInputWidget to add rating specific details
 *
 * Binds the given sb-rating to track editor state for a property, and manages
 * input, updates, and edited attribute
 *
 *****************************************************************************/
function TrackEditorRating(element) {
  
  TrackEditorInputWidget.call(this, element);  
  
  var self = this;
  element.addEventListener("input",
          function() { self.onUserInput(); }, false);
}

TrackEditorRating.prototype = {
  __proto__: TrackEditorInputWidget.prototype,
  
  onUserInput: function() {
    var value = this._element.value;
    TrackEditor.state.setPropertyValue(this.property, value);
  },
  
  onTrackEditorPropertyChange: function TrackEditorRating_onTrackEditorPropertyChange() {
    var property = this.property;
    
    // Indicate if this property has been edited
    if (TrackEditor.state.isPropertyEdited(this.property)) 
    {
      if (!this._element.hasAttribute("edited")) {
        this._element.setAttribute("edited", "true");
      }
    } else if (this._element.hasAttribute("edited")) {
      this._element.removeAttribute("edited"); 
    } 

    TrackEditorInputWidget.prototype.onTrackEditorPropertyChange.call(this);
  },
}






/******************************************************************************
 *
 * \class TrackEditorAdvancedTab 
 * \brief A tab panel that displays all properties in the system
 *
 *****************************************************************************/
function TrackEditorAdvancedTab(tabBox) {
  var tabs = tabBox.getElementsByTagName("tabs")[0];
  var tabPanels = tabBox.getElementsByTagName("tabpanels")[0];
  var tab = document.createElement("tab");
  
  // TODO localize
  tab.setAttribute("label", "Advanced");
  tabs.appendChild(tab);
  
  var panel = document.createElement("vbox");
  panel.setAttribute("flex", "1");
  tabPanels.appendChild(panel);
  
  this._populateTab(panel);
}
TrackEditorAdvancedTab.prototype = {
  
  /**
   * Create grid of labels and text boxes for all known properties
   */
  _populateTab: function TrackEditorAdvancedTab__populateTab(advancedTab) {
    // Create elements for the properties in the Advanced Property Tab
    var label = document.createElement("label");
    label.setAttribute("id", "advanced-warning");
    var labelText = document.createTextNode(
                      "WARNING: Editing these values could ruin Christmas.");
    label.appendChild(labelText);
    
    var advancedContainer = document.createElement("vbox");
    advancedContainer.id = "advanced-contents";
    advancedContainer.setAttribute("flex", "1");

    var propMan = Cc["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
                   .getService(Ci.sbIPropertyManager);

    var propertiesEnumerator = propMan.propertyIDs;
    while (propertiesEnumerator.hasMore()) {
      var property = propertiesEnumerator.getNext();
      var propInfo = propMan.getPropertyInfo(property);
      
      if (propInfo.userViewable) {
        var container = document.createElement("hbox");
        advancedContainer.appendChild(container);
        
        var label = document.createElement("label");
        label.setAttribute("class", "advanced-label");
        label.setAttribute("property", property);
        label.setAttribute("property-type", "label");
        container.appendChild(label);

        if (propInfo.userEditable) {
          var textbox = document.createElement("textbox");
          textbox.setAttribute("property", property);
          container.appendChild(textbox);
        }
        else {
          var label = document.createElement("label");
          label.setAttribute("property", property);
          container.appendChild(label);
        }
      }
    }
    // we add this at the end so that there is just a single reflow
    advancedTab.appendChild(advancedContainer);
  }
}

