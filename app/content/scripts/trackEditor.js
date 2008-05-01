if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;

Components.utils.import("resource://app/jsmodules/SBJobUtils.jsm");

// TODO: clean up trackeditor into two classes
//       probably move button-ey/listen-ey things out from other logic
Components.utils.import("resource://app/components/sbProperties.jsm");

var TrackEditor = {
  _propertyManager: Cc["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
                      .getService(Ci.sbIPropertyManager),
  
  onLoadTrackEditor: function() {
    var that = this;
    
    // Get the main window.
    var windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"]
                          .getService(Ci.nsIWindowMediator);

    var songbirdWindow = windowMediator.getMostRecentWindow("Songbird:Main"); 
    this.gBrowser = songbirdWindow.gBrowser;
    
    this.gBrowser.addEventListener(
      'TabContentChange', 
      function() {
        that.onTabContentChange();
      }, 
      false
    );
    
    this.populateAdvancedTab();
    this.watchTextboxesBlur();
    this.onTabContentChange();
  },

  populateAdvancedTab: function() {
    // Create elements for the properties in the Advanced Property Tab
    var advancedTab = document.getElementById("advanced");

    var advancedContainer = document.createElement("vbox");
    advancedContainer.id = "advanced-contents";
    advancedContainer.setAttribute("flex", "1");

    var propertiesEnumerator = this._propertyManager.propertyIDs;
    while (propertiesEnumerator.hasMore()) {
      var property = propertiesEnumerator.getNext();
      var propInfo = this._propertyManager.getPropertyInfo(property);
      
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
  },

  onTabContentChange: function() {
    if(this.mediaListView) {
      this.mediaListView.selection.removeListener(this);
      this.mediaListView.mediaList.removeListener(this);
    }
    this.mediaListView = this.gBrowser.currentMediaListView;
    this.mediaListView.selection.addListener(this);

    //this.mediaListView.mediaList.addListener(this); 
    // disabled for demo purposes. this watches for changes from the playlist widget.

    // update the autocomplete parameters so we point
    // at the right library
    this.setTextboxesAutoComplete();
    
    this.onSelectionChanged();
  },

  setTextboxesAutoComplete: function() {
    // Set the autocompletesearchparam attribute of all
    // our autocomplete textboxes to 'property;guid', where
    // property is the mediaitem property that the textbox
    // edits, and guid is the guid of the currently displayed
    // library. We could omit the guid and set only the property
    // in the xul, but the suggestions would then come
    // from all the libraries in the system instead of only
    // the one whom the displayed list belongs to.
    var library = this.mediaListView.mediaList.library;
    if (!library) 
      return;
    var libraryGuid = library.guid;
    var textboxes = document.getElementsByTagName("textbox");
    for (var i = 0; i < textboxes.length; i++) {
      var textbox = textboxes[i];
      // verify that this is an autocomplete textbox, and that
      // it belongs to us: avoid changing the param for a
      // textbox that autocompletes to something else than
      // distinct props. Note that we look for the search id
      // anywhere in the attribute, because we could be getting
      // suggestions from multiple suggesters
      if (textbox.getAttribute("type") == "autocomplete" &&
          textbox.getAttribute("autocompletesearch")
                 .indexOf("library-distinct-properties") >= 0) {
          var property = textbox.getAttribute("property");
          textbox.setAttribute("autocompletesearchparam", property + ";" + libraryGuid);
      }
    }
  },

  onItemUpdated: function(aMediaList, aMediaItem, aOldPropertiesArray) {
    // we have to treat this just like a selection change
    // so that multiple-selection values are correctly updated
    // this might actually be a punch in the face because there are a
    // a bunch of user-uneditable properties that change when playback starts
    // hmm    
    for (var i = 0; i < aOldPropertiesArray.length; i++) {
      // fuck me this is weak
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
  },
  
  onSelectionChanged: function() {
    var somethings = document.getElementsByAttribute("property", "*");
    var numSelected = this.mediaListView.selection.count;

    // update the notificationbox at the top.
    var notificationBox = document.getElementById("trackeditor-notification");
    
    notificationBox.removeAllNotifications();
    if (numSelected > 1) {
        notificationBox.appendNotification("Selected " + numSelected + " tracks.", 
                                           "multi-select", null, 1);
    }
    
    // set all textbox values based on the number of items selected
    for (var i = 0; i < somethings.length; i++) {
      var elt = somethings[i];
      var property = elt.getAttribute("property");
      var propInfo = this._propertyManager.getPropertyInfo(property);

      if(elt.hasAttribute("property-type") && elt.getAttribute("property-type") == "label") {
        var value = propInfo.displayName;
      }
      else if (numSelected == 0) {
        var value = propInfo.displayName;
        elt.setAttribute("disabled", true);
      }
      else if (numSelected == 1) {
        var mediaItem = this.mediaListView.selection.currentMediaItem;
        var value = this.getPropertyValue(property, mediaItem);
        elt.removeAttribute("disabled");
        elt.clickSelectsAll = true;
      }
      else { // >1 selected
        var value = this.determineMultipleValue(elt);
        elt.removeAttribute("disabled");
        elt.clickSelectsAll = true;
      }
      this.applyMediaItem(elt, value);
    }
  },

  getPropertyValue: function(property, mediaItem) {
    var propInfo = this._propertyManager.getPropertyInfo(property);
    var value = mediaItem.getProperty(property);

    // on an image, format truncates, then returns!
    // we don't want that.
    // KILL IAN FOR THIS?
    if (property == SBProperties.primaryImageURL) {
      return value;
    }
    
    // oh great. this doesn't work either.
    if (value == "" || value == null) {
      return value;
    }
    
    // formatting can fail. :(
    try { 
      value = propInfo.format( value ); 
    }
    catch (e) { 
      dump("TrackEditor::getPropertyValue("+property+") - "+value+": " + e +"\n");
    }
    return value;
  },

  determineMultipleValue: function(elt) {
    var property = elt.getAttribute("property");
    var propInfo = this._propertyManager.getPropertyInfo(property);
    
    if(elt.hasAttribute("property-type")) {
      if(elt.getAttribute("property-type") == "label") {
        return propInfo.displayName;
      }
    }

    // otherwise, we have a real value
    var sIMI = this.mediaListView.selection.selectedIndexedMediaItems;

    var sharedValue = this.mediaListView
                          .selection
                          .currentMediaItem
                          .getProperty(property);

    while (sIMI.hasMoreElements()) {
      var mI = sIMI.getNext()
                   .QueryInterface(Components.interfaces.sbIIndexedMediaItem)
                   .mediaItem;
      if (mI.getProperty(property) != sharedValue) {
        return "* Various *";
      }
    }
    
    return sharedValue;
  },

  applyMediaItem: function(elt, value) {
    if (elt.nodeName == "label" || elt.nodeName == "textbox") {
      elt.value = value;
      elt.defaultValue = value;
    }
    else if (elt.nodeName == "image") {
      elt.setAttribute("src", value);
    }
  },
  
  onCurrentIndexChanged: function() {
    // foo bar baz
    // don't do nothin
    // yet...
    // maybe we ought to make a multi-select use the 
    // current item for some kind of hinting?
  },
  
  onUnloadTrackEditor: function() {
    // break the cycles
    this.gBrowser.removeEventListener("TabContentChange", this, false);
    this.mediaListView.selection.removeListener(this);
    this.mediaListView.mediaList.removeListener(this);
    this.mediaListView = null;
  },

  watchTextboxesBlur: function() {
    var somethings = document.getElementsByAttribute("property", "*");
    for (var i = 0; i < somethings.length; i++) {
      somethings[i].setAttribute("onchange", "TrackEditor.onTextboxChange(this)");
    }
  },

  onTextboxChange: function(elt) {
    var property = elt.getAttribute("property");
    var propInfo;
    try {
      propInfo = this._propertyManager.getPropertyInfo(property);
    }
    catch (e) {
      // propInfo fails when there is no property of that name
      // indcating we don't need to care about this blur.
      return;
    }

    // be sure the value set was good, and if not, reset it, notifying the user
    // TODO: blank values should be legal for all fields and right now they don't necessarily validate...
    if (elt.value != "" && !propInfo.validate(elt.value)) {
      alert("Bad value for " + propInfo.displayName);
      elt.reset();
    }
    
    // synchronize textboxes elsewhere in the dialog
    var syncers = document.getElementsByAttribute("property", property);
    for(var i = 0; i < syncers.length; i++) {
      if(syncers[i].getAttribute("property-type") == "label") {
        continue; // skip labels
      }
      else {
        syncers[i].value = elt.value;
      }
    }

    var trackEditor = this;
    var notificationBox = document.getElementById("trackeditor-notification");
    var applyNotification = notificationBox.getNotificationWithValue("apply-changes");
    
    if (elt.value != elt.defaultValue && !applyNotification) {
      notificationBox
        .appendNotification(
                            "Apply These Changes?",
                            "apply-changes",
                            null, // no image
                            3, // info, high priority
                            [
                             { 
                               accessKey: "A",
                               callback: function() { TrackEditor.apply() },
                               label: "Apply"
                             },
                             {
                               accessKey: "R",
                               callback: function() { TrackEditor.reset() },
                               label: "Reset"
                             }
                           ]
                          );
    }
    // TODO: this *sucks* for performance (not that it matters yet)
    //       still, maybe we should store all this state somewhere 
    else if (applyNotification) {
      var somethings = document.getElementsByTagName("textbox");
      for (var i = 0; i < somethings.length; i++) {
        if (!somethings[i].hasAttribute("property")) {
          continue;
        }
        if (somethings[i].value != somethings[i].defaultValue) {
          // STOP processing the values, we found one that is changed!
          return;
        }
      }
      // well, i guess there are no other changes left.
      notificationBox.removeNotification(applyNotification);
    }                                        
  },

  next: function() {
    var idx = this.mediaListView.selection.currentIndex;

    if (idx == null || idx == undefined) { return; }
    
    idx = idx+1;
    if (idx >= this.mediaListView.length) { idx = 0; }

    this.mediaListView.selection.selectOnly(idx);
  },

  prev: function() {
    var idx = this.mediaListView.selection.currentIndex;

    if (idx == null || idx == undefined) { return; }
    
    idx = idx-1;
    if (idx < 0) { idx = this.mediaListView.length - 1; }

    this.mediaListView.selection.selectOnly(idx);
  },
  reset: function() {
    var somethings = document.getElementsByTagName("textbox");
    for (var i = 0; i < somethings.length; i++) {
      somethings[i].reset();
    }
  },
  apply: function() {
    if (this.mediaListView.selection.count < 0) {
      alert("Apply what? Select some tracks, foo!");
      return;
    }
    var somethings = document.getElementsByTagName("textbox");

    // todo: move this somewhere more sensible
    var blacklist = {}
    blacklist[SBProperties.trackName] = "ask-multiple";
    blacklist[SBProperties.trackNumber] = "ask-multiple";

    if (this.mediaListView.selection.count > 1) {
      for (var i = 0; i < somethings.length; i++) {
        var tb = somethings[i];
        var property = tb.getAttribute("property");
        if(tb.value != tb.defaultValue && blacklist[property] == "ask-multiple") {
          blacklist[property] == "already-asked";
          // it's cool, this array gets created each time.
          // todo: "Don't ask me again."
          // todo: nonugly title
          // todo: make into a notifcationbox(?)
          var propInfo = this._propertyManager.getPropertyInfo(property);
          if (!confirm("Are you sure you want to change the "+propInfo.displayName
                       +" for all selected tracks?")) {
            tb.reset();
            return;
          }
          else {
            break; // only ask once
          }
        }
      }
    }

    var needsWriting = new Array(this.mediaListView.selection.count);
    // we use a counter for noting items that need writing in the while loop below. 
    // (we can't decorate XPCOM objects, and can't index into selection (i think))

    for (var i = 0; i < somethings.length; i++) {
      var tb = somethings[i];
      if(tb.defaultValue != tb.value) {
        // update the default value. we know these values validate,
        // so they should go into the media items okay
        var property = tb.getAttribute("property");
        tb.defaultValue = tb.value;

        // go through the list setting properties and queuing items
        var sIMI = this.mediaListView.selection.selectedIndexedMediaItems;
        var j = 0;
        while (sIMI.hasMoreElements()) {
          j++;          
          var mI = sIMI.getNext()
            .QueryInterface(Ci.sbIIndexedMediaItem)
            .mediaItem;

          if (mI.getProperty(property) != tb.value) {
            mI.setProperty(property, tb.value);
            needsWriting[j] = true; // keep track of these suckers
          }
        }
      }
    }
    
    var mediaItemArray = Cc["@mozilla.org/array;1"]
                        .createInstance(Ci.nsIMutableArray);
    // go through the list setting properties and queuing items
    var sIMI = this.mediaListView.selection.selectedIndexedMediaItems;
    var j = 0;
    while (sIMI.hasMoreElements()) {
      j++;          
      var mI = sIMI.getNext()
        .QueryInterface(Ci.sbIIndexedMediaItem)
        .mediaItem;
      
      if (needsWriting[j]) {
        mediaItemArray.appendElement(mI, false);
      }
    }
    var manager = Cc["@songbirdnest.com/Songbird/MetadataJobManager;1"]
                      .getService(Ci.sbIMetadataJobManager);
    var job = manager.newJob(mediaItemArray, 5, Ci.sbIMetadataJob.JOBTYPE_WRITE);

    SBJobUtils.showProgressDialog(job, null);
  }
};
