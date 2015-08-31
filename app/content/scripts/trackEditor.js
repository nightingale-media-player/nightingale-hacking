/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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
Cu.import("resource://app/jsmodules/sbMetadataUtils.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

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

    // load the tabBox and individuals tabs     
    var tabBox = document.getElementById("trackeditor-tabbox");
    var tabs = tabBox.tabs;   
     
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
        var property = element.getAttribute("property");
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
      } else if (element.localName == "svg") {
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

    // load the advanced tab
    // get ngales property manager
    var propMan = Cc["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
                       .getService(Ci.sbIPropertyManager);                   
    // get all row elements from the advanced tab
    var advancedRowItemsToLoad = document.getElementsByAttribute("class", "advTabRowElements");
    
    // get the menupopup element from the advanced tab
    var advancedMenupopup = document.getElementById("menupopup");

    // Populate the advanced tab's menupopup
    for (var i = 0; i < advancedRowItemsToLoad.length; i++){
      var singleRowItem = advancedRowItemsToLoad[i];
      var property = singleRowItem.getAttribute("property");
      var propInfo = propMan.getPropertyInfo(property);
      // create menuitems
      var menuitem = document.createElement("menuitem");
      menuitem.setAttribute("type", "checkbox");
      menuitem.setAttribute("class", "advTabMenuItems");
      menuitem.setAttribute("property", property);
      menuitem.setAttribute("label", propInfo.displayName);
      // stop the menuitem(s) from triggering the menupopup to close.
      menuitem.setAttribute("closemenu", "none");
      advancedMenupopup.appendChild(menuitem);
    }

    // addEventListener to the menupopup
    advancedMenupopup.addEventListener("popuphidden", function(){TrackEditor.onpopupclose()}, false);

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
    
    var trackeditorTabs = document.getElementById("trackeditor-tabs");
    var tabCount = 0;
    for (var tab = trackeditorTabs.firstChild; tab; tab = tab.nextSibling) {
      if (("hidden" in tab) && (!tab.hidden)) {
        ++tabCount;
      }
    }
    if (tabCount > 1) {
      trackeditorTabs.hidden = false;
    }

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
    
    // get the new item's content type
    var itemContentType = this.mediaListView.selection.currentMediaItem.contentType;

    if (itemContentType == "audio"){
      // load the users audio preference
      var userPreferenceProperty = JSON.parse(Application.prefs.getValue(
                                        "songbird.trackeditor.audioAdvancedTags", "")); 
            
      // update the advanced tab with the proper elements
      TrackEditor.updateAdvancedTab(userPreferenceProperty);
    }
    else if (itemContentType == "video"){
      // load the users audio preference
      var userPreferenceProperty = JSON.parse(Application.prefs.getValue(
                                        "songbird.trackeditor.videoAdvancedTags", ""));
      
      // update the advanced tab with the proper elements
      TrackEditor.updateAdvancedTab(userPreferenceProperty);
    }

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
    
    var batchSetter = {
      runBatched: function(aUserData) {
        for each (property in properties) {
          if (!TrackEditor.state.isPropertyEdited(property)) {
            continue;
          }
          // Add the property to our list so only the changed ones get written
          writeProperties.push(property);

          for (var i = 0; i < items.length; i++) {
            var value = TrackEditor.state.getPropertyValue(property);
            var item = items[i];
            // don't modify values for non-user-editable items (except rating)
            if (!LibraryUtils.canEditMetadata(item)
                && property != SBProperties.rating) {
              continue;
            }

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
      }
    };

    this.mediaListView.mediaList.runInBatchMode(batchSetter, null);
    
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
    var mediaItemArray = [];
    for (var i = 0; i < items.length; i++) {
      if (needsWriting[i] && LibraryUtils.canEditMetadata(items[i])) {
        mediaItemArray.push(items[i]);
      }
    }
    if (mediaItemArray.length > 0 && writeProperties.length > 0) {
      sbMetadataUtils.writeMetadata(mediaItemArray,
                                    writeProperties,
                                    window,
                                    this.mediaListView.mediaList);
    }
    
    return true;
  },

  /**
   * onpopupclose function handles the event when the user closes the advanced
   * tab's menupopup.  It collects all the menuitems in the menupopup that are 
   * checked and show/hides the row elements in the advanced tab that have the
   * same property attribute.  It also saves the users selection of checked 
   * items for later use inside the checkedPropertyStrings, which is written to 
   * the users respective preference.
   */  
  onpopupclose: function() {
    // get all menuitems from the advanced tab
    var advancedMenuItems = document.getElementsByAttribute("class", "advTabMenuItems");
    var checkedPropertyStrings = [];
    for (var i = 0; i < advancedMenuItems.length; i++){
      if (advancedMenuItems[i].getAttribute("checked", "true")){
        var propertyString = advancedMenuItems[i].getAttribute("property");
        checkedPropertyStrings.push(propertyString);
      }
    }

    // update the advanced tab
    TrackEditor.updateAdvancedTab(checkedPropertyStrings);

    // get current items content type
    var itemContentType = this._browser.currentMediaListView.selection.currentMediaItem.contentType;
    
    // write the list of property strings to the users preferences
    if (itemContentType == "audio"){
      Application.prefs.setValue("songbird.trackeditor.audioAdvancedTags", 
                                  JSON.stringify(checkedPropertyStrings));
    } 
    else if (itemContentType == "video"){
      Application.prefs.setValue("songbird.trackeditor.videoAdvancedTags",
                                  JSON.stringify(checkedPropertyStrings));
    }
  },

  /**
   * updateAdvancedTab function show/hides the row elements in the advanced tab
   * and updates the proper "checked" state of the menuitems that belong to
   * the advanced tab's menupopup element.
   * anArrayofPropertyStrings - a list of property strings that correspond to row elements
   *                            in the advanced tab the user wants to show/hide on screen.
   */
  updateAdvancedTab: function(anArrayOfPropertyStrings){
    // get all menuitems from the advanced tab
    var advancedMenuItems = document.getElementsByAttribute("class", "advTabMenuItems");
    // get all row elements from the advanced tab
    var rowElementsAdvancedTab = document.getElementsByAttribute("class", "advTabRowElements");
    
    /**
     * Go through all of the advanced tab's row elements. If its property attribute is found within 
     * anArrayOfPropertyStrings,  we show the row element on screen by removing the hidden attribute 
     * and we hide the row element by setting the attribute to true.
     */

    for (var i = 0; i < rowElementsAdvancedTab.length; i++){
      var property = rowElementsAdvancedTab[i].getAttribute("property");
      var matchStringProperty = anArrayOfPropertyStrings.indexOf(property,0);
      if(matchStringProperty > -1){
        rowElementsAdvancedTab[i].removeAttribute("hidden");
      } else {
        rowElementsAdvancedTab[i].setAttribute("hidden", true);
      }
    }

    /**
     * Go through all of the advanced tab's menuitems elements. If its property attribute is found within 
     * anArrayOfPropertyStrings,  we "check" the menuitem by setting its "checked" attribute to true and   
     * remove the attribute to "un-check" the menuitem.
     */

    for (var i = 0; i < advancedMenuItems.length; i++){
      var property = advancedMenuItems[i].getAttribute("property");
      var matchStringProperty = anArrayOfPropertyStrings.indexOf(property,0);
      if(matchStringProperty > -1){
        advancedMenuItems[i].setAttribute("checked", "true");
      } else {
        advancedMenuItems[i].removeAttribute("checked");
      }
    }
  },
};


