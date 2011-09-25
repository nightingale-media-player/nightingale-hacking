/*
//
// BEGIN NIGHTINGALE GPL
// 
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
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

const ARTWORK_NO_COVER = "chrome://nightingale/skin/album-art/drop-target.png";

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
  _replaceLabel: null,
  _addLabel: null,
  _dragoverlay: null,
  _elementStack: null,
  // Menu popup and items 
  _menuPopup: null,
  _menuCut: null,
  _menuCopy: null,
  _menuPaste: null,
  _menuClear: null,
  
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
    this._elementStack.setAttribute("class", "art");
    if (this._element.hasAttribute("tabindex")) {
      this._elementStack.setAttribute("tabindex",
                                      this._element.getAttribute("tabindex"));
      this._element.removeAttribute("tabindex");
    }
    this._element.parentNode.replaceChild(this._elementStack, this._element);

    // Label for Drag here text
    var dragLabel = document.createElement("label");
    dragLabel.setAttribute("class", "drop-message");

    // Text value to be added to the label as a child
    var dragLabelValue
    dragLabelValue = document.createTextNode(SBString("trackeditor.artwork.drag"));
    dragLabel.appendChild(dragLabelValue);

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
    this._menuCut = document.createElement("menuitem");
    this._menuCopy = document.createElement("menuitem");
    this._menuPaste = document.createElement("menuitem");
    this._menuClear = document.createElement("menuitem");
    var menuSeparatorPaste = document.createElement("menuseparator");
    
    var self = this;
    this._menuCut.setAttribute("label", SBString("trackeditor.artwork.menu.cut"));
    this._menuCut.addEventListener("command",
                                 function() { self.onCut();}, false);

    this._menuCopy.setAttribute("label", SBString("trackeditor.artwork.menu.copy"));
    this._menuCopy.addEventListener("command",
                                  function() { self.onCopy();}, false);

    this._menuPaste.setAttribute("label", SBString("trackeditor.artwork.menu.paste"));
    this._menuPaste.addEventListener("command",
                                   function() { self.onPaste();}, false);

    this._menuClear.setAttribute("label", SBString("trackeditor.artwork.menu.clear"));
    this._menuClear.addEventListener("command",
                                   function() { self.onClear();}, false);
    this._menuPopup.appendChild(this._menuCut);
    this._menuPopup.appendChild(this._menuCopy);
    this._menuPopup.appendChild(this._menuPaste);
    this._menuPopup.appendChild(menuSeparatorPaste);
    this._menuPopup.appendChild(this._menuClear);

    this._menuPopup.addEventListener("popupshowing",
          function(evt) { self.onPopupShowing(evt); }, false);

    this._element.parentNode.appendChild(this._menuPopup);
  },
  
  /**
   * \brief Handles context menu actions.
   */
  onContextMenu: function TrackEditorArtwork_onContextMenu(aEvent) {
    // Make sure we are focused (could be a right-click that fired this)
    this._elementStack.focus();
    
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
   * \brief onPopupShowing - Adjusts the menu before displaying based on if the
   *                         items are editable or not.
   * \param aEvent - event for this action
   */
  onPopupShowing: function TrackEditorArtwork_onPopupShowing(aEvent) {
    var curImageUrl = TrackEditor.state.getPropertyValue(this.property);

    // Get the clipboard image.
    var sbClipboard = Cc["@getnightingale.com/moz/clipboard/helper;1"]
                        .createInstance(Ci.sbIClipboardHelper);
    var mimeType = {};
    var imageData = sbClipboard.copyImageFromClipboard(mimeType, {});
    mimeType = mimeType.value;

    // Validate image as valid album art.
    var isValidAlbumArt = false;
    if (imageData && (imageData.length > 0)) {
      var artService = Cc["@getnightingale.com/Nightingale/album-art-service;1"]
                         .getService(Ci.sbIAlbumArtService);
      isValidAlbumArt = artService.imageIsValidAlbumArt(mimeType,
                                                        imageData,
                                                        imageData.length);
    }
    
    if (!curImageUrl || curImageUrl == ARTWORK_NO_COVER) {
      this._menuCut.setAttribute("disabled", true);
      this._menuCopy.setAttribute("disabled", true);
      this._menuClear.setAttribute("disabled", true);
    } else {
      this._menuCut.removeAttribute("disabled");
      this._menuCopy.removeAttribute("disabled");
      this._menuClear.removeAttribute("disabled");
    }
    
    if (!isValidAlbumArt) {
      this._menuPaste.setAttribute("disabled", true);
    } else {
      this._menuPaste.removeAttribute("disabled");
    }

    // Disable everything except copy for readonly items.
    if (TrackEditor.state.isDisabled) {
      this._menuCut.setAttribute("disabled", true);
      this._menuClear.setAttribute("disabled", true);
      this._menuPaste.setAttribute("disabled", true);
    }     
  },
 
  /**
   * \brief Handles clicks from user on the album artwork widget
   */
  onClick: function TrackEditorArtwork_onClick(aEvent) {
    // Focus the element so we can respond to context menus and commands
    this._elementStack.focus();
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
          if (!TrackEditor.state.isDisabled)
            this.onCut();
        break;
        case 99:  // COPY
          this.onCopy();
        break;
        case 118: // PASTE
          if (!TrackEditor.state.isDisabled)
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
    var sbClipboard = Cc["@getnightingale.com/moz/clipboard/helper;1"]
                        .createInstance(Ci.sbIClipboardHelper);
    var mimeType = {};
    var imageData = sbClipboard.copyImageFromClipboard(mimeType, {});
    if (sbCoverHelper.isImageSizeValid(null, imageData.length)) {
      
      var artService =
                        Cc["@getnightingale.com/Nightingale/album-art-service;1"]
                          .getService(Ci.sbIAlbumArtService);

      var newURI = artService.cacheImage(mimeType.value,
                                         imageData,
                                         imageData.length);
      if (newURI) {
        this._imageSrcChange(newURI.spec);
      }
    }
  },
  
  /**
   * \brief handle the copy command (invoked either from the keyboard or
   *        context menu)
   */
  onCopy: function TrackEditorArtwork_onCopy() {
    var sbClipboard = Cc["@getnightingale.com/moz/clipboard/helper;1"]
                        .createInstance(Ci.sbIClipboardHelper);

    // Load up the file (Properties are stored as URL Strings)
    var imageFilePath = TrackEditor.state.getPropertyValue(this.property);
    var ioService = Cc["@mozilla.org/network/io-service;1"]
                      .getService(Ci.nsIIOService);

    // Convert the URL to a URI
    var imageURI = null;
    try {
      imageURI = ioService.newURI(imageFilePath, null, null);
    } catch (err) {
      Cu.reportError("trackEditor: Unable to convert to URI: [" +
                     imageFilePath + "] - " + err);
      return false;
    }
    
    // Check if this is a local file
    if (!(imageURI instanceof Ci.nsIFileURL)) {
      Cu.reportError("trackEditor: Not a local file [" +
                     imageFilePath + "]");
      return false;
    }
    
    imageFile = imageURI.file; // The .file is a nsIFile
    var imageData;
    var mimetype;
    [imageData, mimeType] = sbCoverHelper.readImageData(imageFile);
    
    try {
      sbClipboard.pasteImageToClipboard(mimeType,
                                        imageData,
                                        imageData.length);
    } catch (err) {
      Cu.reportError("trackEditor: Unable to copy from clipboard - " + err);
      return false;
    }
    
    return true;
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

  /**   * \brief handle the cut command (invoked either from the keyboard or
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
    if (TrackEditor.state.isDisabled) {
      return;
    }
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
    this._elementStack.focus();
    
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
    
    var XLINK_NS = "http://www.w3.org/1999/xlink";
    var SVG_NS = "http://www.w3.org/2000/svg";
    // The SVG image is somewhat unique in that it requires an SVG element to wrap it.
    // Thus, we put the property on the wrapper, and reach in to grab the image and set its
    // href from the outside.
    var imageElement = this._element.getElementsByTagNameNS(SVG_NS, "image")[0];
    if (value && value == imageElement.getAttributeNS(XLINK_NS, "href")) {
      // Nothing has changed so leave it as is.
      return;
    }
    
    // Check if we have multiple values for this property
    var allMatch = true;
    if (TrackEditor.state.selectedItems.length > 1) {
      allMatch = !TrackEditor.state.hasMultipleValuesForProperty(this.property);
    }
     
    // Check if we can edit all the items in the list
    var canEdit = (TrackEditor.state.writableItemCount ==
                   TrackEditor.state.selectedItems.length);

    // check if we should hide the drag here label
    if (value && value != "") {
      // There is an image so hide the label
      this._dragoverlay.hidden = true;
    } else {
      this._dragoverlay.hidden = false;
    }
    
    // Lets check if this item is missing a cover
    if( (!value || value == "") && allMatch ) {
      value = ARTWORK_NO_COVER;
    }

    // Update the button to the correct text
    this._button.label = (value == ARTWORK_NO_COVER ? this._addLabel : this._replaceLabel);
    
    // Update the image depending on if we have multiple items or not.
    imageElement.setAttributeNS(XLINK_NS, "href", (allMatch ? value : ""));
    
    // Indicate if this property has been edited
    if (TrackEditor.state.isPropertyEdited(this.property)) {
      this._elementStack.setAttribute("edited", true);
    } else {
      this._elementStack.removeAttribute("edited");
    }
    
    // update the check box to indicate we haved edited the property
    this._checkbox.checked = TrackEditor.state.isPropertyEdited(this.property);

    // Toggle the "Add..." button depending on the read-only state.
    if (TrackEditor.state.isDisabled) {
      this._button.setAttribute("disabled", "true");
    }
    else {
      this._button.removeAttribute("disabled");
    }
  }
}

