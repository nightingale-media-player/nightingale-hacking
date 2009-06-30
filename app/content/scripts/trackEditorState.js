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
