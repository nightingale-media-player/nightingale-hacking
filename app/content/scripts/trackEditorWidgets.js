/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
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

    var propMan = Cc["@getnightingale.com/Nightingale/Properties/PropertyManager;1"]
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
    var flex = this._element.getAttribute("flex");
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
    if (flex) {
      hbox.setAttribute("flex", flex);
    }
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
      this._elementStack.focus();
    }
  },
  
  onTrackEditorSelectionChange: function() {
    this._checkbox.hidden = TrackEditor.state.selectedItems.length <= 1; 
    
    // If none of the selected items can be modified, disable all editing
    if (TrackEditor.state.isDisabled) {
      this._checkbox.disabled = true;
      this._element.setAttribute("readonly", "true");
      this._element.setAttribute("tooltiptext", 
                                 SBString("trackeditor.tooltip.readonly"));
    } else {
      this._checkbox.disabled = false;
      this._element.disabled = false;
      this._element.removeAttribute("readonly");
      this._element.removeAttribute("tooltiptext");
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

  var propMan = Cc["@getnightingale.com/Nightingale/Properties/PropertyManager;1"]
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

    // Set a tooltip text for differing multi-value textboxes.
    if (TrackEditor.state.hasMultipleValuesForProperty(this.property)) {
      this._element.setAttribute("tooltiptext", SBString("trackeditor.tooltip.multiple"));
    }
    else if (this._element.getAttribute("tooltiptext") == 
             SBString("trackeditor.tooltip.multiple")) {
      this._element.removeAttribute("tooltiptext");
    }

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
        this._element.setAttribute("tooltiptext", SBString("trackeditor.tooltip.invalid"));
      }
    } else if (this._element.hasAttribute("invalid")) {
      this._element.removeAttribute("invalid");
      this._element.removeAttribute("tooltiptext");
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
      var param = this.property;

      // Get content type for genre property and set to auto complete param.
      if (this.property == SBProperties.genre) {
        var LSP = Cc["@getnightingale.com/servicepane/library;1"]
                    .getService(Ci.sbILibraryServicePaneService);
        var type =
          LSP.getNodeContentTypeFromMediaListView(TrackEditor.mediaListView);
        if (type)
          param += "$" + type;
      }
      param += ";" + libraryGuid;
      if (defvals && defvals != "") {
        param += ";" + defvals;
      }
      this._element.setAttribute("autocompletesearchparam", param);
    }
    
    // Grr, autocomplete textboxes don't handle tabindex, so we have to 
    // get our hands dirty.  Filed as Moz Bug 432886.
    this._element.inputField.tabIndex = this._element.tabIndex;
    
    // And we need to fix the dropdown not showing up...
    // (Nightingale bug 9149, same moz bug)
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

    // Override the rating widget to never be disabled.
    this._checkbox.disabled = false;
    this._element.disabled = false;
    this._element.removeAttribute("readonly");
    this._element.removeAttribute("tooltiptext");
  }
}

