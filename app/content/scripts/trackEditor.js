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
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;

Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/sbCoverHelper.jsm");
Cu.import("resource://app/jsmodules/SBJobUtils.jsm");
Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const ARTWORK_NO_COVER = "chrome://songbird/skin/album-art/default-cover.png";


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

  _metadataImageScannerService:
                        Cc["@songbirdnest.com/Songbird/Metadata/ImageScanner;1"]
                          .getService(Ci.sbIMetadataImageScanner),
  
  
  // Array of media items that the track editor is operating on
  _selectedItems: null,
    
  // Internal data structure
  // Example:
  // {
  //   propertyName: { 
  //     hasMultiple: false,  (multiple values between selected items)
  //     edited: false,
  //     enabled: false,      (allowed to be saved)
  //     value: "",
  //     originalValue: "".
  //     knownInvalid: false  (verified to be ivalid)
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
                      .QueryInterface(Ci.sbIIndexedMediaItem)
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
          if (LibraryUtils.canEditMetadata(item)) {
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
    this._properties[property].edited = true;

    // Multiple values no longer matter
    this._properties[property].hasMultiple = false;
    
    // The property has changed, and may be valid.
    // We don't want to validate on every change,
    // so wait until someone calls validatePropertyValue()
    this.validatePropertyValue(property);
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
   * Returns true if the given property has been
   * enabled for editing.  In some cases
   * attempting to edit a value may automatically
   * enabled editing.
   */
  isPropertyEnabled: function(property) {
    this._ensurePropertyData(property);
    return this._properties[property].enabled;
  },
  
  /** 
   * Set/unset the enabled flag for the given property.
   * Used to control whether changes will be written
   * out to the selected media items.
   */
  setPropertyEnabled: function(property, enabled) {
    this._ensurePropertyData(property);
    this._properties[property].enabled = enabled;
    this._notifyPropertyListeners(property);
  },
  
  /** 
   * Returns true if the value has been verified
   * as invalid.  Note that validation
   * only happens on request, not on set.
   */
  isPropertyInvalidated: function(property) {
    this._ensurePropertyData(property);
    return this._properties[property].knownInvalid;
  },
  
  /** 
   * Performs validation on the value of the given
   * property, and then broadcasts a notification.
   * Check the result via isPropertyInvalidated.
   */
  validatePropertyValue: function(property) {
    var needsNotification = true;

    this._ensurePropertyData(property);

    // If no changes were made, don't bother invalidating
    if (this._properties[property].edited) {  

      var value = this._properties[property].value;
      if (value == "") {
        value = null;
      }

      var valid = this._properties[property].propInfo.validate(value);
      this._properties[property].knownInvalid = !valid;

      // XXXAus: Because of bug 9045, we have to tie the totalTracks and 
      // totalDiscs properties to trackNumber and discNumber and ensure
      // that denominator is not smaller than the numerator.
      if(valid) {
        // Track number greater than total tracks, invalid value.
        if(property == SBProperties.totalTracks ||
           property == SBProperties.trackNumber) {
          
          this._properties[SBProperties.totalTracks].knownInvalid = 
           !isNaN(this._properties[SBProperties.totalTracks].value) &&
           !isNaN(this._properties[SBProperties.trackNumber].value) &&
           (parseInt(this._properties[SBProperties.totalTracks].value) < 
            parseInt(this._properties[SBProperties.trackNumber].value))
          
          this._properties[SBProperties.trackNumber].knownInvalid = 
            !isNaN(this._properties[SBProperties.totalTracks].value) &&
            !isNaN(this._properties[SBProperties.trackNumber].value) &&
            (parseInt(this._properties[SBProperties.trackNumber].value) > 
             parseInt(this._properties[SBProperties.totalTracks].value))

          needsNotification = false;

          this._notifyPropertyListeners(SBProperties.totalTracks);
          this._notifyPropertyListeners(SBProperties.trackNumber);
        }
        // Disc number greater than total discs, invalid value.
        if(property == SBProperties.totalDiscs ||
           property == SBProperties.discNumber) {
          
          this._properties[SBProperties.totalDiscs].knownInvalid =
            !isNaN(this._properties[SBProperties.totalDiscs].value) &&
            !isNaN(this._properties[SBProperties.discNumber].value) &&
            (parseInt(this._properties[SBProperties.discNumber].value) > 
             parseInt(this._properties[SBProperties.totalDiscs].value));
          
          this._properties[SBProperties.discNumber].knownInvalid = 
            !isNaN(this._properties[SBProperties.totalDiscs].value) &&
            !isNaN(this._properties[SBProperties.discNumber].value) &&
            (parseInt(this._properties[SBProperties.totalDiscs].value) <
             parseInt(this._properties[SBProperties.discNumber].value));
             
          needsNotification = false;
          
          this._notifyPropertyListeners(SBProperties.totalDiscs);
          this._notifyPropertyListeners(SBProperties.discNumber);
        }
      }
    }

    if(needsNotification) {
      this._notifyPropertyListeners(property);
    }
  },
  
  /** 
   * Returns true if at least one enabled property has been
   * determined to be invalid.  
   * Note that validation must be explicitly requested
   * via validatePropertyValue().
   */
  isKnownInvalid: function() {
    for (var propertyName in this._properties) {
      if (this._properties[propertyName].knownInvalid && 
          this._properties[propertyName].enabled) {
        return true;
      }
    }
    return false;
  },
  
  /** 
   * Returns an array of property names that have been
   * explicitly enabled for writing.  
   * Note that the values may or may not have been edited.
   */
  getEnabledProperties: function() {
    var enabledList = [];
    for (var propertyName in this._properties) {
      if (this._properties[propertyName].enabled) {
        enabledList.push(propertyName);
      }
    }
    return enabledList;
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
      enabled: false,
      hasMultiple: false,
      value: "",
      originalValue: "",
      propInfo: this._propertyManager.getPropertyInfo(property),
      knownInvalid: false
    };
    
    // Populate the structure
    if (this._selectedItems && this._selectedItems.length > 0) {      
      var value = this._selectedItems[0].getProperty(property);

      if (property == SBProperties.primaryImageURL) {
        // See if we need to manually load extra track image data.
        var MAX_ITEMS_TO_CHECK = 30; // keep this from taking forever.
        var selectedItems = TrackEditor.state.selectedItems;
        var numToCheck = Math.min(MAX_ITEMS_TO_CHECK, selectedItems.length);
        
        var seedValue;
        for (var i = 0; i < numToCheck; i++) {
          value = selectedItems[i].getProperty(property);
          // Note: "" implies the file was scanned and found empty.
          //       null implies that the file has not been checked for data.
          //       Both are false, so if you don't care, just use if(!value).
          if (value == null) { 
            value = this._metadataImageScannerService
                        .fetchCoverForMediaItem(selectedItems[i]);
            selectedItems[i].setProperty(property, value);
          }
          
          // Break out early if we determine we don't need to scan any further items.
          if (i == 0) {
            seedValue = value;
          }
          if (seedValue != value) {
            value = "";
            break;
          }
        }
      }

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
      if (value) 
      {
        if (property == SBProperties.primaryImageURL) {
          // can't format. for an image, format truncates the value to ""!
        }
        else if (this._properties[property].hasMultiple) {
          // we keep multiple values as "".
        }
        else {
          // Formatting can fail. :(
          try { 
            value = this._properties[property].propInfo.format(value); 
          }
          catch (e) { 
            Components.utils.reportError("TrackEditor::getPropertyValue("+property+") - "+value+": " + e +"\n");
          }
        }
      }
      
      if (value == null) {
        value = "";
      }

      this._properties[property].value = value;
      
      // If there are multiple value for this property,
      // default to edited as "", but disabled.
      // This way the user can null out the multiple values
      // simply by enabling the field.
      if (this._properties[property].hasMultiple) {
        this._properties[property].edited = true;
        this._properties[property].originalValue = null;
      } else {
        this._properties[property].originalValue = value;
      }
    }
  },
  
  /** 
   * Add an object to be notified when the editor state for the given
   * property changes. Pass "all" to be notified of all property changes.
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
      
      // Notify those who explicitly want to hear about all changes.
      if (property != "all") {
        var listeners = this._propertyListeners["all"];
        if (listeners) {
          for each (var listener in listeners) {
            listener.onTrackEditorPropertyChange(property);
          }
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
  }
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
    
    var initialTab = window.arguments[0];
    if (initialTab) {
      var tabBox = document.getElementById("trackeditor-tabbox");
      var tabs = tabBox.parentNode.getElementsByTagName("tab");
      for each (var tab in tabs) {
        if (tab.id == initialTab) {
          tabBox.selectedTab = tab;
        }
      }
    }
  },
  
  /**
   * Find and configure known DOM elements
   */
  _setUpDefaultWidgets: function TrackEditor__setUpDefaultWidgets() {

    // TODO: profiling shows that setting the value of a textbox can
    // take up to 200ms.  Multiply that by 40 textboxes and performance
    // can suffer.  Do some more tests and determine the impact of
    // the advanced tab

    // Set up the advanced tab
    var enableAdvanced = Application.prefs.getValue(
                           "songbird.trackeditor.enableAdvancedTab", false);
    if (enableAdvanced) {
      // this code assumes that the number of tabs and tabpanels is aligned
      var tabbox = document.getElementById("trackeditor-tabbox");
      this._elements["advancedTab"] = new TrackEditorAdvancedTab(tabbox);
    }
    else {

      var tabBox = document.getElementById("trackeditor-tabbox");
      var tabs = tabBox.tabs;
      tabs.setAttribute("hidden", "true");
      /*    
      // FIXME: hid summary and lyrics tabs halfheartedly
      //        (you can probably still keyboard shortcut to them)
      tabs.getItemAtIndex(0).setAttribute("hidden", "true");
      tabs.getItemAtIndex(2).setAttribute("hidden", "true");
      tabBox.selectedIndex = 1;*/
    }
 
    // Add an additional layer of control to all elements with a property
    // attribute.  This will cause the elements to be updated when the
    // selection model changes, and vice versa.
    
    this._elements["anonymous"] = []; // Keep elements without ids in a list
    
    // Potentially anonymous elements that need wrapping
    var elements = document.getElementsByAttribute("property", "*");
    for each (var element in elements) {
      var wrappedElement = null;
      if (element.tagName == "label" ||
          (element.tagName == "textbox" && /\bplain\b/(element.className)))
      {
        var property = element.getAttribute("property")
        var propertyInfo = this._propertyManager.getPropertyInfo(property);
        
        if (element.getAttribute("property-type") != "label" &&
            propertyInfo.type == "uri") {
          wrappedElement = new TrackEditorURILabel(element);
        }
        else if (element.getAttribute("property-type") != "label" &&
                 property == SBProperties.originPage) {
          // FIXME: originPage should be a uri type, but isn't!
          wrappedElement = new TrackEditorOriginLabel(element);
        }
        else {
          wrappedElement = new TrackEditorLabel(element);
        }
      } else if (element.tagName == "textbox") {
        wrappedElement = new TrackEditorTextbox(element);
      } else if (element.tagName == "sb-rating") {
        wrappedElement = new TrackEditorRating(element);
      } else if (element.tagName == "image") {
        // Setup a TrackEditorArtwork wrappedElement
        wrappedElement = new TrackEditorArtwork(element);
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
                "prev_button", "next_button", "infotab_trackname_label",
                "ok_button", "cancel_button"];
    // I'd love to do this in the previous loop, but getElementsByAttribute
    // returns an HTMLCollection, not an array.
    for each (var elementName in elements) {
      var element = document.getElementById(elementName);
      if (element) {
        this._elements[elementName] = element;
      } 
    }

    this._elements["notification_box"].hidden = true;
    
    
    // Monitor all changes in order to update the dialog controls
    this.state.addPropertyListener("all", this);
  },
  
  
  /**
   * Show/hide warning messages as needed in the header of the dialog
   */
  updateNotificationBox: function TrackEditor__updateNotificationBox() {
    
    var itemCount = this.state.selectedItems.length;
    var writableCount = this.state.writableItemCount;
    
    var message;
    var notificationClass = "dialog-notification notification-warning";

    if (itemCount > 1) {
      if (writableCount == itemCount) {
        message = SBFormattedString("trackeditor.notification.editingmultiple", [itemCount]);                
        notificationClass = "dialog-notification notification-info";
      } else if (writableCount >= 1) {
        message = SBFormattedString("trackeditor.notification.somereadonly",
                                    [(itemCount - writableCount), itemCount]);
      } else {
        message = SBString("trackeditor.notification.multiplereadonly");
      }
    } else if (writableCount == 0) {
      message = SBString("trackeditor.notification.singlereadonly");
    }
      
    if (message) {
      this._elements["notification_box"].className = notificationClass;
      this._elements["notification_text"].textContent = message;
      this._elements["notification_box"].hidden = false;
    } else {
      this._elements["notification_box"].hidden = true;
    }
  },
  
  /**
   * Update the dialog controls based on the current state
   */
  updateControls: function TrackEditor__updateControls() {
   
   // If the user has entered invalid data, disable all 
   // UI that would cause an apply()
   var hasErrors = this.state.isKnownInvalid();
   
   this._elements["ok_button"].disabled = hasErrors;
   
   // Disable next/prev at top/bottom of list, and when
   // editing multiple items
   var idx = this.mediaListView.selection.currentIndex;
   var atStart = (idx == 0)
   var atEnd   = (idx == this.mediaListView.length - 1);
   var hasMultiple = this.mediaListView.selection.count > 1;
   this._elements["prev_button"].setAttribute("disabled", 
      (atStart || hasErrors || hasMultiple ? "true" : "false"));
   this._elements["next_button"].setAttribute("disabled", 
      (atEnd  || hasErrors || hasMultiple ? "true" : "false"));
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
    
    this.state.setSelection(this.mediaListView.selection);    
    
    // Disable summary page if multiple items are selected.
    // TODO summary page is commented out.
    /*
    var tabBox = document.getElementById("trackeditor-tabbox");
    var tabs = tabBox.tabs;
    
    if (this.state.selectedItems.length > 1) {
      // warning: assumes summary page is at tabs[0]
      if (tabBox.selectedIndex == 0) {
        tabBox.selectedIndex = 1;
      }
      tabs.getItemAtIndex(0).setAttribute("disabled", "true");
    }
    */
    
    this.updateNotificationBox();
    this.updateControls();
 
    // Hacky special case to hide the title when editing multiple items.
    // We assume you don't want to set multiple titles at once.
    var hidden = this.state.selectedItems.length > 1;
    this._elements["infotab_trackname_textbox"].hidden = hidden;
    this._elements["infotab_trackname_label"].hidden = hidden;
  },
  
  /**
   * Called any time a property value is modified by the UI
   */
  onTrackEditorPropertyChange: function TrackEditor_onTrackEditorPropertyChange(property) {
    // Update dialog controls, since the validation state may have changed.
    // TODO: this may hurt performance, since it is executed on every keystroke!
    this.updateControls();
  },
  
  /**
   * Called when the media item selection in the main player window changes
   * TODO not being run at the moment?
   */
  onUnloadTrackEditor: function() {
    // break the cycles
    //this._browser.removeEventListener("TabContentChange", this, false);
    this.mediaListView.selection.removeListener(this);

    //this.mediaListView.mediaList.removeListener(this);
    //we're assuming a modal dialog for now, so don't reflect changes.
    
    this.mediaListView = null;
  },

  /**
   * Called by the right arrow button.  Increments the selection
   * index in the main player window
   */
  next: function() {
    if (!this.apply()) {
      return;
    }

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
    if (!this.apply()) {
      return;
    }
    
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
   * Attempt to write out property changes and close the dialog
   */
  closeAndApply: function() {
    if (this.apply()) {
      window.close();
    }
  },

  /**
   * Write property changes back to the selected media items
   * and if possible to the on disk files.
   */  
  apply: function() {
    
    if (this.state.isKnownInvalid()) {
      Components.utils.reportError("TrackEditor: attempt to call apply() with known invalid state");
      return false;
    }
    
    var properties = this.state.getEnabledProperties();
    var items = this.state.selectedItems;
    if (items.length == 0 || properties.length == 0) {
      return true;
    }
    
    var enableRatingWrite = Application.prefs.getValue("songbird.metadata.ratings.enableWriting", false);
    
    // Properties we want to write
    var writeProperties = [];
    
    // Apply each modified property back onto the selected items,
    // keeping track of which items have been modified
    var needsWriting = new Array(items.length);
    for each (property in properties) {
      if (!TrackEditor.state.isPropertyEdited(property)) {
        continue;
      }
      // Add the property to our list so only the changed ones get written
      writeProperties.push(property);

      for (var i = 0; i < items.length; i++) {
        var value = TrackEditor.state.getPropertyValue(property);
        var item = items[i];
        if (value != item.getProperty(property)) {
          // Completely remove empty properties
          // HACK for 0.7: primaryImageURL likes to be set to ""
          //               this says "i am scanned and empty"
          //               setting it to null says "rescan me"
          if (value == "" && property != SBProperties.primaryImageURL) {
            value = null;
          }
          
          item.setProperty(property, value);
          
          // Flag the item as needing a metadata-write job.
          // Do not start a write-job if all that has changed is the 
          // rating, and rating-write isn't enabled.
          if (property != SBProperties.rating || enableRatingWrite) {
            needsWriting[i] = true;
          }
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
    var mediaItemArray = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                        .createInstance(Ci.nsIMutableArray);
    for (var i = 0; i < items.length; i++) {
      if (needsWriting[i] && LibraryUtils.canEditMetadata(items[i])) {
        mediaItemArray.appendElement(items[i], false);
      }
    }
    if (mediaItemArray.length > 0 && writeProperties.length > 0) {
      var metadataService = Cc["@songbirdnest.com/Songbird/FileMetadataService;1"]
                              .getService(Ci.sbIFileMetadataService);      
      try {
	// This will write out the properties in propArray for each item.
        var propArray = ArrayConverter.stringEnumerator(writeProperties);
        var job = metadataService.write(mediaItemArray, propArray);
      
        SBJobUtils.showProgressDialog(job, window);
      } catch (e) {
        // Job will fail if writing is disabled by the pref
        Components.utils.reportError(e);
      }
    }
    
    return true;
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
  __proto__: TrackEditorWidgetBase.prototype
}


/******************************************************************************
 *
 * \class TrackEditorURILabel 
 * \brief Extends TrackEditorLabel for label elements containing URLs
 *
 * If the label has a property-type="label" attribute it will receive the 
 * title associated with the property attribute. Otherwise the label will
 * receive the current editor display value for property.
 *
 *****************************************************************************/
function TrackEditorURILabel(element) {
  TrackEditorLabel.call(this, element);
  
  this._button = document.createElement("button");
  this._button.setAttribute("class", "goto-url");
  
  var self = this;
  this._button.addEventListener("command", 
      function() { self.onButtonCommand(); }, false);
  
  element.parentNode.insertBefore(this._button, element.nextSibling);
}
TrackEditorURILabel.prototype = {
  __proto__: TrackEditorLabel.prototype,
  _ioService: Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService),
  
  onTrackEditorPropertyChange: function TrackEditorWidgetBase_onTrackEditorPropertyChange() {
    var value = TrackEditor.state.getPropertyValue(this.property);
    
    var formattedValue = value;
    try {
      // reformat the URI as a native file path
      var ioService = Cc["@mozilla.org/network/io-service;1"]
                        .getService(Ci.nsIIOService); 
      var uri = ioService.newURI(value, null, null);
      if (uri.scheme == "file") {
        formattedValue = uri.QueryInterface(Ci.nsIFileURL).file.path;
      }
    }
    catch(e) {
      /* it's okay, we just leave formattedValue == value in this case */
    }
    
    if (formattedValue != this._element.value) {
      this._element.value = formattedValue;
    }
  
    if (value != this._button.value) {
      this._button.value = value;
    }
  },
  
  onButtonCommand: function()
  {
    this.loadOrRevealURI(this._button.value);
  },
  
  loadOrRevealURI: function(uriLocation)
  {
    var uri = this._ioService.newURI(uriLocation, null, null);
    if (uri.scheme == "file") {
      this.revealURI(uri);
    }
    else {
      this.promptAndLoadURI(uriLocation);
    }
  },
  
  revealURI: function(uri) {
    // Cribbed from mozilla/toolkit/mozapps/downloads/content/downloads.js 
    var f = uri.QueryInterface(Ci.nsIFileURL).file.QueryInterface(Ci.nsILocalFile);
    
    try {
      // Show the directory containing the file and select the file
      f.reveal();
    } catch (e) {
      // If reveal fails for some reason (e.g., it's not implemented on unix or
      // the file doesn't exist), try using the parent if we have it.
      let parent = f.parent.QueryInterface(Ci.nsILocalFile);
      if (!parent)
        return;
  
      try {
        // "Double click" the parent directory to show where the file should be
        parent.launch();
      } catch (e) {
        // If launch also fails (probably because it's not implemented), let the
        // OS handler try to open the parent
        openExternal(parent);
      }
    }
  },
  
  promptAndLoadURI: function(uriLocation) {
    var properties = TrackEditor.state.getEnabledProperties();
    
    var edits = 0;
    for (var i = 0; i < properties.length; i++) {
      if (TrackEditor.state.isPropertyEdited(properties[i])) {
        edits++;
      }
    }
    
    var items = TrackEditor.state.selectedItems;
    if (items.length == 0 || edits == 0) {
      // No need to muck about with prompts. Just banish the window.
      TrackEditor._browser.loadOneTab(uriLocation, null, null, null, false, false);
      window.close();
      return;
    }
    
    // This is not a local file, so open it in a new tab and dismiss
    // the track editor. (Prompt first.)
    
    var titleMessage = SBString("trackeditor.closewarning.title");
    var dialogMessage = SBString("trackeditor.closewarning.message");
    var prompts = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                      .getService(Components.interfaces.nsIPromptService);
    var flags = prompts.BUTTON_TITLE_DONT_SAVE * prompts.BUTTON_POS_0 +
          prompts.BUTTON_TITLE_CANCEL          * prompts.BUTTON_POS_1 +
          prompts.BUTTON_TITLE_OK              * prompts.BUTTON_POS_2;
    
    try {
      var button = prompts.confirmEx(window, titleMessage, dialogMessage, flags, 
           null, null, null, null, {});
      switch (button) {
        case 0:
          TrackEditor._browser.loadOneTab(uriLocation, null, null, null, true, false);
          window.close();
        case 1: return;
        case 2:
          TrackEditor._browser.loadOneTab(uriLocation, null, null, null, true, false);
          TrackEditor.closeAndApply();
      }
    } catch(e) {
      Components.utils.reportError(e);
    }
  }
};

/******************************************************************************
 *
 * \class TrackEditorOriginLabel 
 * \brief Extends TrackEditorURILabel for the originPage property.
 *        Basically the same, except with originPage{Title, Image} for display.
 *
 *****************************************************************************/
function TrackEditorOriginLabel(element) {
  TrackEditorURILabel.call(this, element);
  TrackEditor.state.addPropertyListener(SBProperties.originPageTitle, this);
}
TrackEditorOriginLabel.prototype = {
  __proto__: TrackEditorURILabel.prototype,
  
  onTrackEditorPropertyChange: function (property) {
    // NB: no property value ==> all properties liable to have changed
    if (property == SBProperties.originPage      || !property) {
      this.onTrackEditorPagePropertyChange();
    }
    if (property == SBProperties.originPageTitle || !property) {
      this.onTrackEditorPageTitlePropertyChange();
    }
    if (property == SBProperties.originPageImage || !property) {
      this.onTrackEditorPageImagePropertyChange();
    }
  },
  
  onTrackEditorPagePropertyChange: function() {
    var value = TrackEditor.state.getPropertyValue(this.property);
    
    var formattedValue = value;
    try {
      // reformat the URI as a native file path
      var ioService = Cc["@mozilla.org/network/io-service;1"]
                        .getService(Ci.nsIIOService); 
      var uri = ioService.newURI(value, null, null);
      if (uri.scheme == "file") {
        formattedValue = uri.QueryInterface(Ci.nsIFileURL).file.path;
      }
    }
    catch(e) {
      /* it's okay, we just leave formattedValue == value in this case */
    }
    
    if (value != this._button.value) {
      this._button.value = value;
    }
  },
  
  onTrackEditorPageTitlePropertyChange: function() {
    var title = TrackEditor.state.getPropertyValue(SBProperties.originPageTitle);
    if (title != this._element.title) {
      this._element.value = title;
    }
  },
  
  onTrackEditorPageImagePropertyChange: function() {
    // TODO: this
  }
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
  
  get hidden() {
    return this._element.parentNode.hidden;
  },
  
  set hidden(val) {
    this._element.parentNode.hidden = val;
  },
  
  onCheckboxCommand: function() {
    TrackEditor.state.setPropertyEnabled(this.property, this._checkbox.checked);
    if (this._checkbox.checked) {
      this._element.focus();
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
    
    this._checkbox.checked = TrackEditor.state.isPropertyEnabled(this.property);
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

  var propMan = Cc["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
                 .getService(Ci.sbIPropertyManager);
  var propInfo = propMan.getPropertyInfo(this.property);
  if (propInfo instanceof Ci.sbINumberPropertyInfo || 
      this._element.getAttribute("isnumeric") == "true") {
    this._isNumeric = true;
    this._minValue = propInfo.minValue;
    this._maxValue = propInfo.maxValue;
    this._maxDigits = new String(this._maxValue).length;
    
    element.addEventListener("keypress",
            function(evt) { self.onKeypress(evt); }, false);
  }
}
TrackEditorTextbox.prototype = {
  __proto__: TrackEditorInputWidget.prototype,
  
  // If set true, filter out non-numeric input
  _isNumeric: false, 
  _minValue: 0, 
  _maxValue: 0,
  _maxDigits: 0,
  
  onUserInput: function() {
    var self = this;
    // Defer updating the model by a tick.  This is necessary because
    // html:input elements (inside of textbox) send oninput too early
    // when undoing from empty state to a previous value.  html:textarea
    // sends oninput with value==previous, where html:input sends oninput
    // with value=="".  This bug has been filed as BMO 433574.
    setTimeout(function() { 
        var value = self._element.value;
        TrackEditor.state.setPropertyValue(self.property, value);
        
        // Auto-enable property write-back
        if (!TrackEditor.state.isPropertyEnabled(self.property)) {
          TrackEditor.state.setPropertyEnabled(self.property, true);
        }
      }, 0 );
  },
  
  onKeypress: function(evt) {    
    // If numeric, suppress all other keys.
    // TODO/NOTE: THIS DOES NOT WORK FOR NEGATIVE VALUES!
    if (this._isNumeric) {
      if (!evt.ctrlKey && !evt.metaKey && !evt.altKey && evt.charCode) {
        if (evt.charCode < 48 || evt.charCode > 57) {
          evt.preventDefault();
        // If typing the key would make the value too long, prevent keypress
        } else if (this._element.value.length + 1 > this._maxDigits &&
                   this._element.selectionStart == this._element.selectionEnd) {
          evt.preventDefault();
        }
      }
    }
  },
  
  onTrackEditorSelectionChange: function TrackEditorTextbox_onTrackEditorSelectionChange() {
    TrackEditorInputWidget.prototype.onTrackEditorSelectionChange.call(this);

    if (this._element.getAttribute("type") == "autocomplete") {
      this._configureAutoComplete();
    }
  },
  
  onTrackEditorPropertyChange: function TrackEditorTextbox_onTrackEditorPropertyChange() {
    TrackEditorInputWidget.prototype.onTrackEditorPropertyChange.call(this);

    var property = this.property;
    
    // Indicate if this property has been edited
    if (TrackEditor.state.isPropertyEdited(this.property)) {
      if (!this._element.hasAttribute("edited")) {
        this._element.setAttribute("edited", "true");
      }
    } else {
      if (this._element.hasAttribute("edited")) {
        this._element.removeAttribute("edited"); 
      }

      // If this is the original un-edited value, set it as the
      // default for the textbox and reset the undo history
      var value = TrackEditor.state.getPropertyValue(property);
      if (this._element.defaultValue != value) {
        this._element.defaultValue = value;
        this._element.reset();
      }
    }
    
    // Indicate if this property is known to be invalid
    if (TrackEditor.state.isPropertyInvalidated(this.property)) {
      if (!this._element.hasAttribute("invalid")) {
        this._element.setAttribute("invalid", "true");
      }
    } else if (this._element.hasAttribute("invalid")) {
      this._element.removeAttribute("invalid"); 
    }
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
    // get our hands dirty.  Filed as Moz Bug 432886.
    this._element.inputField.tabIndex = this._element.tabIndex;
    
    // And we need to fix the dropdown not showing up...
    // (Songbird bug 9149, same moz bug)
    var marker = this._element
                     .ownerDocument
                     .getAnonymousElementByAttribute(this._element,
                                                     "anonid",
                                                     "historydropmarker");
    if ("showPopup" in marker) {
      // only hack the method if it's the same

      // here is the expected function...
      function showPopup() {
        var textbox = document.getBindingParent(this);
        textbox.showHistoryPopup();
      };
      
      // and we compare it with uneval to make sure the existing one is expected
      if (uneval(showPopup) == uneval(marker.showPopup)) {
        // the existing function looks like what we expect
        marker.showPopup = function showPopup_hacked() {
          var textbox = document.getBindingParent(this);
          setTimeout(function(){
            textbox.showHistoryPopup();
          }, 0);
        }
      } else {
        // the function does not match; this should not happen in practice
        dump("trackEditor - autocomplete marker showPopup mismatch!\n");
      }
    }
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

    // Auto-enable property write-back
    if (!TrackEditor.state.isPropertyEnabled(this.property)) {
      TrackEditor.state.setPropertyEnabled(this.property, true);
    }
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
  }
}

/******************************************************************************
 *
 * \class TrackEditorArtwork
 * \brief Extends TrackEditorInputWidget to add an artwork editor
 *
 * Binds the given image element
 *
 *****************************************************************************/
function TrackEditorArtwork(element) {
  
  TrackEditorInputWidget.call(this, element);
  
  this._metadataImageScannerService =
              Cc["@songbirdnest.com/Songbird/Metadata/ImageScanner;1"]
                .getService(Ci.sbIMetadataImageScanner);
  
  this._replaceLabel = SBString("trackeditor.artwork.replace");
  this._addLabel = SBString("trackeditor.artwork.add");
  this._createButton();
  this._createDragOverlay();
  this._createContextMenu();

  var self = this;
  this._elementStack.addEventListener("click",
          function(evt) { self.onClick(evt); }, false);
  this._elementStack.addEventListener("keypress",
          function(evt) { self.onKeyPress(evt); }, false);
  this._elementStack.addEventListener("contextmenu",
          function(evt) { self.onContextMenu(evt); }, false);
  // Drag and drop for the album art image
  // We need to have over in order to get the getSupportedFlavours called when
  // the user drags an item over us.
  this._elementStack.addEventListener("dragover",
          function(evt) { nsDragAndDrop.dragOver(evt, self); }, false);
  this._elementStack.addEventListener("dragdrop",
          function(evt) { nsDragAndDrop.drop(evt, self); }, false);
  this._elementStack.addEventListener("draggesture",
          function(evt) { nsDragAndDrop.startDrag(evt, self); }, false);
}
TrackEditorArtwork.prototype = {
  __proto__: TrackEditorInputWidget.prototype,
  _button: null,
  _menuPopup: null,
  _replaceLabel: null,
  _addLabel: null,
  _dragoverlay: null,
  _elementStack: null,
  
  /**
   * \brief Changes the value of the image property only if it different.
   * \param newValue string of the uri to set this property to.
   */
  _imageSrcChange: function TrackEditorArtwork__imageSrcChange(newValue) {
    var oldValue = TrackEditor.state.getPropertyValue(this.property);
    
    if (newValue != oldValue) {
      
      // This will call onTrackEditorPropertyChange
      TrackEditor.state.setPropertyValue(this.property, newValue);

      // Auto-enable property write-back
      if (!TrackEditor.state.isPropertyEnabled(this.property)) {
        TrackEditor.state.setPropertyEnabled(this.property, true);
      }
    }
  },

  /**
   * \brief Creates a button attached to this image so that the user can
   *        select an image from the file system
   */
  _createButton: function TrackEditorArtwork__createButton() {
    this._button = document.createElement("button");
    var vbox = document.createElement("vbox");
    this._element.parentNode.replaceChild(vbox, this._element);
    
    // In order for tabbing to work in the desired order
    // we need to apply tabindex to all elements.
    if (this._element.hasAttribute("tabindex")) {
      var value = parseInt(this._element.getAttribute("tabindex")) + 1;
      this._button.setAttribute("tabindex", value);
    }

    var self = this;
    this._button.addEventListener("command", 
      function() { self.onButtonCommand(); }, false);
    
    vbox.appendChild(this._element);
    vbox.appendChild(this._button);
  },
  
  /**
   * \brief Creates a stack so we can display a drag image here label over top
   *        of the default image if the item does not have artwork.
   */
  _createDragOverlay: function TrackEditorArtwork__createDragOverlay() {
    // Outter stack for overlaying the label on the image
    this._elementStack = document.createElement("stack");
    this._element.parentNode.replaceChild(this._elementStack, this._element);

    // Label for Drag here text
    var dragLabel = document.createElement("label");
    dragLabel.setAttribute("class", "artDragLabel");
    dragLabel.setAttribute("value", SBString("trackeditor.artwork.drag"));

    // Create a wrapper box around the image for the stack
    var imageVBox = document.createElement("vbox");
    imageVBox.setAttribute("class", "artWrapperBox");
    imageVBox.appendChild(this._element);
    
    // Create a wrapper box around the label for the stack
    this._dragoverlay = document.createElement("vbox");
    this._dragoverlay.setAttribute("class", "artWrapperBox");
    this._dragoverlay.appendChild(dragLabel);
    
    // Append the two boxes
    this._elementStack.appendChild(imageVBox);
    this._elementStack.appendChild(this._dragoverlay)
  },
  
  /**
   * \brief Creates a context menu for the image that allows the user to copy,
   *        paste, cut and clear the artwork for an item or items.
   */
  _createContextMenu: function TrackEditorArtwork__createContextMenu() {
    this._menuPopup = document.createElement("menupopup");
    var menuItemCut = document.createElement("menuitem");
    var menuItemCopy = document.createElement("menuitem");
    var menuItemPaste = document.createElement("menuitem");
    var menuItemClear = document.createElement("menuitem");
    
    var self = this;
    menuItemCut.setAttribute("label", SBString("trackeditor.artwork.menu.cut"));
    menuItemCut.addEventListener("command",
                                 function() { self.onCut();}, false);

    menuItemCopy.setAttribute("label", SBString("trackeditor.artwork.menu.copy"));
    menuItemCopy.addEventListener("command",
                                  function() { self.onCopy();}, false);

    menuItemPaste.setAttribute("label", SBString("trackeditor.artwork.menu.paste"));
    menuItemPaste.addEventListener("command",
                                   function() { self.onPaste();}, false);

    menuItemClear.setAttribute("label", SBString("trackeditor.artwork.menu.clear"));
    menuItemClear.addEventListener("command",
                                   function() { self.onClear();}, false);
    this._menuPopup.appendChild(menuItemCut);
    this._menuPopup.appendChild(menuItemCopy);
    this._menuPopup.appendChild(menuItemPaste);
    this._menuPopup.appendChild(menuItemClear);

    this._element.parentNode.appendChild(this._menuPopup);
  },
  
  /**
   * \brief Handles context menu actions.
   */
  onContextMenu: function TrackEditorArtwork_onContextMenu(aEvent) {
    // Make sure we are focused (could be a right-click that fired this)
    this._element.focus();
    
    // Default to assuming we are invoked by alternative methods to right-click
    var xPos = 0;                       // No position needed
    var yPos = 0;
    var anchor = "after_start";         // Anchor to the bottom left
    var anchor_element = this._element; // Anchor to the element
    
    // Check if it was a right-click
    if (aEvent.button == 2) {
      // Since we were invoked by mouse we do not anchor the menu
      anchor_element = null;
      anchor = "";
      xPos = aEvent.clientX;
      yPos = aEvent.clientY;
    }
    
    this._menuPopup.openPopup(anchor_element, // Anchor to the art work box
                              anchor,         // position it on the bottom
                              xPos,           // x position/offset of menu
                              yPos,           // y position/offset of menu
                              true,           // context menu
                              false);         // no attributes override
  },
  
  /**
   * \brief Handles clicks from user on the album artwork widget
   */
  onClick: function TrackEditorArtwork_onClick(aEvent) {
    // Focus the element so we can respond to context menus and commands
    this._element.focus();
  },

  /**
   * \brief Handles keypress from user when album artwork widget is focused.
   *        We have to handle each OS natively.
   */
  onKeyPress: function TrackEditorArtwork_onKeyPress(aEvent) {
    var validMetaKeys = false;
    
    if (aEvent.keyCode == 46 || aEvent.keyCode == 8) {
      this.onClear();
    }
    
    if (getPlatformString() == "Darwin") {
      // Mac (Uses cmd)
      validMetaKeys = (aEvent.metaKey && !aEvent.ctrlKey && !aEvent.altKey);
    } else {
     // Windows, Linux (Use ctrl)
      validMetaKeys = (!aEvent.metaKey && aEvent.ctrlKey && !aEvent.altKey);
    }
    
    if (validMetaKeys) {
      switch (aEvent.charCode) {
        case 120: // CUT
          // Relies on copy
          this.onCut();
        break;
        case 99:  // COPY
          this.onCopy();
        break;
        case 118: // PASTE
          this.onPaste();
        break;
      }
    }
  },

  /**
   * \brief handle the paste command (invoked either from the keyboard or
   *        context menu)
   */
  onPaste: function TrackEditorArtwork_onPaste() {
    var sbClipboard = Cc["@songbirdnest.com/moz/clipboard/helper;1"]
                        .createInstance(Ci.sbIClipboardHelper);
    var mimeType = {};
    var imageData = sbClipboard.copyImageFromClipboard(mimeType, {});
    if (sbCoverHelper.isImageSizeValid(null, imageData.length)) {
      var metadataImageScannerService =
                        Cc["@songbirdnest.com/Songbird/Metadata/ImageScanner;1"]
                          .getService(Ci.sbIMetadataImageScanner);

      var newFile = metadataImageScannerService
                      .saveImageDataToFile(imageData,
                                           imageData.length,
                                           mimeType.value);
      if (newFile) {
        this._imageSrcChange(newFile);
      }
    }
  },
  
  /**
   * \brief handle the copy command (invoked either from the keyboard or
   *        context menu)
   */
  onCopy: function TrackEditorArtwork_onCopy() {
    var sbClipboard = Cc["@songbirdnest.com/moz/clipboard/helper;1"]
                        .createInstance(Ci.sbIClipboardHelper);

    // Load up the file
    var imageFilePath = TrackEditor.state.getPropertyValue(this.property);
    var ioService = Cc["@mozilla.org/network/io-service;1"]
                      .getService(Ci.nsIIOService);
    var normalPath = decodeURI(ioService.newURI(imageFilePath, null, null).path);
    var imageFile = Cc["@mozilla.org/file/local;1"]
                      .createInstance(Ci.nsILocalFile);
    try {
      imageFile.initWithPath(normalPath);
    } catch(err) {
      return false;
    }
    
    var imageData;
    var mimetype;
    [imageData, mimeType] = sbCoverHelper.readImageData(imageFile);
    
    var copyOk = true;
    try {
      sbClipboard.pasteImageToClipboard(mimeType, imageData, imageData.length);
    } catch (err) {
      copyOk = false;
    }
    
    return copyOk;
  },
  
  /**
   * \brief handle the cut command (invoked either from the keyboard or
   *        context menu)
   */
  onCut: function TrackEditorArtwork_onCut() {
    if (this.onCopy()) {
      this.onClear();
    }
  },

  /**
   * \brief handle the cut command (invoked either from the keyboard or
   *        context menu)
   */
  onClear: function TrackEditorArtwork_onClear() {
    this._imageSrcChange("");
  },

  /**
   * Drag and Drop functions
   */
  getSupportedFlavours : function TrackEditorArtwork_getSupportedFlavours() {
    var flavours = new FlavourSet();
    return sbCoverHelper.getFlavours(flavours);
  },

  onDragOver: function(event, flavour, session) {
    // No need to do anything here, for UI we should set the
    // .art:-moz-drag-over style.
  },
  
  onDrop: function TrackEditorArtwork_onDrop(aEvent, aDropData, aSession) {
    var self = this;
    sbCoverHelper.handleDrop(function (newFile) {
      if (newFile) {
        self._imageSrcChange(newFile);
      }
    }, aDropData);
  },

  onDragStart: function TrackEditorArtwork_onDragStart(aEvent, 
                                                       aTransferData,
                                                       aAction) {
    var imageURL = TrackEditor.state.getPropertyValue(this.property);
    aTransferData.data = new TransferData();
    sbCoverHelper.setupDragTransferData(aTransferData, imageURL);
  },


  /**
   * \brief Called when the user clicks the button, we then pop up a file
   *        picker for them to choose an image file.
   */
  onButtonCommand: function TrackEditorArtwork_onButtonCommand() {
    this._element.focus();
    
    // Open the file picker
    var filePicker = Cc["@mozilla.org/filepicker;1"]
                       .createInstance(Ci.nsIFilePicker);
    var windowTitle = SBString("trackeditor.filepicker.title");
    filePicker.init( window, windowTitle, Ci.nsIFilePicker.modeOpen);
    filePicker.appendFilters(Ci.nsIFilePicker.filterImages);
    var fileResult = filePicker.show();
    if (fileResult == Ci.nsIFilePicker.returnOK) {
      var ioService = Cc["@mozilla.org/network/io-service;1"]
                        .getService(Ci.nsIIOService);
      var fileURL = ioService.newFileURI(filePicker.file).spec;
      if (sbCoverHelper.isImageSizeValid(fileURL)) {
        this._imageSrcChange(fileURL);
      }
    }
  },
  
  onTrackEditorPropertyChange: function TrackEditorArtwork_onTrackEditorPropertyChange() {
    var value = TrackEditor.state.getPropertyValue(this.property);
    
    if (value && value == this._element.getAttribute("src")) {
      // Nothing has changed so leave it as is.
      return;
    }
    
    // Check if we have multiple values for this property
    var allMatch = true;
    if (TrackEditor.state.selectedItems.length > 1) {
      allMatch = !TrackEditor.state.hasMultipleValuesForProperty(this.property);
    }
    
    // check if we should hide the drag here label
    if (value && value != "") {
      // There is an image so hide the label
      this._dragoverlay.setAttribute("hidden", true);
    } else {
      this._dragoverlay.removeAttribute("hidden");
    }
    
    // Lets check if this item is missing a cover
    if( (!value || value == "") && allMatch ) {
      value = ARTWORK_NO_COVER;
    }

    // Update the button to the correct text
    if (value == ARTWORK_NO_COVER) {
      this._button.setAttribute("label", this._addLabel);
    } else {
      this._button.setAttribute("label", this._replaceLabel);
    }

    // Update the image depending on if we have multiple items or not.
    if (allMatch) {
      // All the items match on this property (or if there is only one)
      this._element.setAttribute("src", value);
    } else {
      // Multiple items that do not match show nothing
      this._element.setAttribute("src", "");
    }
    
    // Indicate if this property has been edited
    if (TrackEditor.state.isPropertyEdited(this.property)) {
      if (!this._element.hasAttribute("edited")) {
        this._element.setAttribute("edited", "true");
      }
    } else if (this._element.hasAttribute("edited")) {
      this._element.removeAttribute("edited"); 
    }

    this._checkbox.checked = TrackEditor.state.isPropertyEnabled(this.property);
  }
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
  
  tab.setAttribute("label", SBString("trackeditor.tab.advanced"));
  tab.setAttribute("id", "advanced");
  tabs.appendChild(tab);
  
  var panel = document.createElement("vbox");
  panel.style.overflow = "scroll"
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
    // TODO remove or localize
    var labelText = document.createTextNode(
                      "WARNING: Editing these values could ruin Christmas.");
    label.appendChild(labelText);
    
    var advancedGrid = document.createElement("grid");
    advancedGrid.id = "advanced-contents";
    advancedGrid.style.MozBoxFlex = 1;
    var columns = document.createElement("columns");
    var column = document.createElement("column");
    column.setAttribute("id", "advanced-column-key");
    columns.appendChild(column);
    column = document.createElement("column");
    column.setAttribute("id", "advanced-column-value");
    column.style.MozBoxFlex = "1";
    columns.appendChild(column);
    advancedGrid.appendChild(columns);
    var advancedRows = document.createElement("rows");
    advancedGrid.appendChild(advancedRows);

    var propMan = Cc["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
                   .getService(Ci.sbIPropertyManager);

    var propertiesEnumerator = propMan.propertyIDs;
    while (propertiesEnumerator.hasMore()) {
      var property = propertiesEnumerator.getNext();
      var propInfo = propMan.getPropertyInfo(property);
      
      if (propInfo.userViewable) {
        var container = document.createElement("row");
        advancedRows.appendChild(container);
        
        var label = document.createElement("label");
        label.setAttribute("class", "advanced-label");
        label.setAttribute("property", property);
        label.setAttribute("property-type", "label");
        container.appendChild(label);

        if (propInfo.userEditable) {
          var textbox = document.createElement("textbox");
          textbox.setAttribute("property", property);
          textbox.setAttribute("flex", "1");
          container.appendChild(textbox);
        }
        else {
          var label = document.createElement("textbox");
          label.setAttribute("readonly", true);
          label.className = "plain";
          label.setAttribute("property", property);
          container.appendChild(label);
        }
      }
    }
    // we add this at the end so that there is just a single reflow
    advancedTab.appendChild(advancedGrid);
  }
}


