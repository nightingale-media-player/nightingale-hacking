//TODO: update license, though I don't think we want to 
//remove the list of contributors.

/*
  Originally from /browser/base/content/nsContextMenu.js
  Forked on April 10, 2008 to accomodate for Songbird's needs
*/

/*
 ***** BEGIN LICENSE BLOCK *****
 Version: MPL 1.1/GPL 2.0/LGPL 2.1

 The contents of this file are subject to the Mozilla Public License Version
 1.1 (the "License"); you may not use this file except in compliance with
 the License. You may obtain a copy of the License at
 http://www.mozilla.org/MPL/

 Software distributed under the License is distributed on an "AS IS" basis,
 WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 for the specific language governing rights and limitations under the
 License.

 The Original Code is mozilla.org code.

 The Initial Developer of the Original Code is
 Netscape Communications Corporation.
 Portions created by the Initial Developer are Copyright (C) 1998
 the Initial Developer. All Rights Reserved.

 Contributor(s):
   Blake Ross <blake@cs.stanford.edu>
   David Hyatt <hyatt@mozilla.org>
   Peter Annema <disttsc@bart.nl>
   Dean Tessman <dean_tessman@hotmail.com>
   Kevin Puetz <puetzk@iastate.edu>
   Ben Goodger <ben@netscape.com>
   Pierre Chanial <chanial@noos.fr>
   Jason Eager <jce2@po.cwru.edu>
   Joe Hewitt <hewitt@netscape.com>
   Alec Flett <alecf@netscape.com>
   Asaf Romano <mozilla.mano@sent.com>
   Jason Barnabe <jason_barnabe@fastmail.fm>
   Peter Parente <parente@cs.unc.edu>
   Giorgio Maone <g.maone@informaction.com>
   Tom Germeau <tom.germeau@epigoon.com>
   Jesse Ruderman <jruderman@gmail.com>
   Joe Hughes <joe@retrovirus.com>
   Pamela Greene <pamg.bugs@gmail.com>
   Michael Ventnor <ventnors_dogs234@yahoo.com.au>
   Simon Bünzli <zeniko@gmail.com>
   Gijs Kruitbosch <gijskruitbosch@gmail.com>
   Ehsan Akhgari <ehsan.akhgari@gmail.com>
   Dan Mosedale <dmose@mozilla.org>

 Alternatively, the contents of this file may be used under the terms of
 either the GNU General Public License Version 2 or later (the "GPL"), or
 the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 in which case the provisions of the GPL or the LGPL are applicable instead
 of those above. If you wish to allow use of your version of this file only
 under the terms of either the GPL or the LGPL, and not to allow others to
 use your version of this file under the terms of the MPL, indicate your
 decision by deleting the provisions above and replace them with the notice
 and other provisions required by the GPL or the LGPL. If you do not delete
 the provisions above, a recipient may use your version of this file under
 the terms of any one of the MPL, the GPL or the LGPL.

 ***** END LICENSE BLOCK *****
*/

if (!window.DNDUtils)
  Components.utils.import("resource://app/jsmodules/DropHelper.jsm");
if (!window.LibraryUtils) 
  Components.utils.import("resource://app/jsmodules/LibraryUtils.jsm");

function ContentAreaContextMenu(aXulMenu, aBrowser) {
  this.target            = null;
  this.tabbrowser        = null;
  this.menu              = null;
  this.isFrameImage      = false;
  this.onTextInput       = false;
  this.onKeywordField    = false;
  this.onImage           = false;
  this.onLoadedImage     = false;
  this.onCompletedImage  = false;
  this.onCanvas          = false;
  this.onLink            = false;
  this.onMailtoLink      = false;
  this.onSaveableLink    = false;
  this.onMetaDataItem    = false;
  this.onMathML          = false;
  this.onMedia           = false;
  this.link              = false;
  this.linkURL           = "";
  this.linkURI           = null;
  this.linkProtocol      = null;
  this.inFrame           = false;
  this.hasBGImage        = false;
  this.isTextSelected    = false;
  this.isContentSelected = false;
  this.inDirList         = false;
  this.shouldDisplay     = true;
  this.isDesignMode      = false;
  this.possibleSpellChecking = false;
  this.ellipsis = "\u2026";
  try {
    this.ellipsis = gPrefService.getComplexValue("intl.ellipsis",
                                                 Ci.nsIPrefLocalizedString).data;
  } catch (e) { }

  // Initialize new menu.
  this.initMenu(aXulMenu, aBrowser);
}

// Prototype for ContentAreaContextMenu "class."
ContentAreaContextMenu.prototype = {
  // onDestroy is a no-op at this point.
  onDestroy: function () {
  },

  // Initialize context menu.
  initMenu: function CM_initMenu(aPopup, aBrowser) {
    this.menu = aPopup;
    this.tabbrowser = aBrowser;

    this.isFrameImage = document.getElementById("isFrameImage");

    // Get contextual info.
    this.setTarget(document.popupNode, document.popupRangeParent,
                   document.popupRangeOffset);

    this.isTextSelected = this.isTextSelection();
    this.isContentSelected = this.isContentSelection();

    // Initialize (disable/remove) menu items.
    this.initItems();
  },

  initItems: function CM_initItems() {
    this.initOpenItems();
    this.initOpenExternalItems();
    this.initNavigationItems();
    this.initViewItems();
    this.initMiscItems();
    this.initSpellingItems();
    this.initSaveItems();
    this.initClipboardItems();
    this.initMediaItems();
  },

  initOpenItems: function CM_initOpenItems() {
    var shouldShow = this.onSaveableLink ||
                     (this.inDirList && this.onLink);
    this.showItem("context-openlinkintab", shouldShow);
    this.showItem("context-sep-open", shouldShow);
  },

  initOpenExternalItems: function CM_initOpenExternalItems() {
    var showLink = this.onSaveableLink ||
                     (this.inDirList && this.onLink);
    var showPage = !showLink && !showImage && !InlineSpellCheckerUI.canSpellCheck;
    var showImage = this.onImage;

    this.showItem("context-openlinkexternal", showLink);
    this.showItem("context-openpageexternal", showPage);
    this.showItem("context-sep-openexternal", showLink || showPage);

    this.showItem("context-openimageexternal", showImage);
    this.showItem("context-sep-openimageexternal", showImage);
  },

  initNavigationItems: function CM_initNavigationItems() {
    var shouldShow = !(this.onLink || this.onImage ||
                       this.onCanvas || this.onTextInput);
    this.showItem("context-back", shouldShow);
    this.showItem("context-forward", shouldShow);
    this.showItem("context-reload", shouldShow);
    this.showItem("context-stop", shouldShow);
    this.showItem("context-sep-stop", shouldShow);
  },

  initSaveItems: function CM_initSaveItems() {
    var shouldShow = !(this.inDirList || this.onTextInput || this.onLink ||
                       this.onImage || this.onCanvas);
    this.showItem("context-savepage", shouldShow);
    this.showItem("context-sep-savepage", shouldShow);

    // Save link depends on whether we're in a link.
    this.showItem("context-savelink", this.onSaveableLink);

    // Save image depends on whether we're on a loaded image, or a canvas.
    this.showItem("context-saveimage", this.onLoadedImage || this.onCanvas);
  },

  initViewItems: function CM_initViewItems() {
    // View source is always OK, unless in directory listing.
    var shouldShow = !(this.inDirList || this.onImage || 
                       this.onLink || this.onTextInput);
    this.showItem("context-viewsource", shouldShow);

    this.showItem("context-sep-properties", false);

    // Set as Desktop background depends on whether an image was clicked on,
    // and only works if we have a shell service.
    var haveSetDesktopBackground = false;

    // Show image depends on an image that's not fully loaded
    this.showItem("context-showimage", (this.onImage && !this.onCompletedImage));

    // View image depends on having an image that's not standalone
    // (or is in a frame), or a canvas.
    this.showItem("context-viewimage", (this.onImage &&
                  (!this.onStandaloneImage || this.inFrame)) || this.onCanvas);

    this.showItem("context-sep-viewbgimage", false);
  },

  initMiscItems: function CM_initMiscItems() {
    this.showItem("context-searchselect", this.isTextSelected);
    //xxxlone: note that if we ever want to change this to show the 
    // frame items at the same time as the image and/or link items, 
    // it'll be be problematic, because we'll then need to decide on 
    // whether the final separator needs to be shown or not according
    // to the visibility status of a different type of item (link, 
    // or image), which is not ideal as it makes the code more complex.
    // firefox deals with this by always having a final item that is
    // never used with a trailing separator (eg, 'properties', 
    // 'inspect', ...).
    var showFrame = this.inFrame && !this.onImage && !this.onLink;
    this.showItem("frame", showFrame);
    this.showItem("frame-sep", false);
    this.showItem("frame-sep-after", showFrame);

    // Hide menu entries for images, show otherwise
    if (this.inFrame) {
      if (mimeTypeIsTextBased(this.target.ownerDocument.contentType))
        this.isFrameImage.removeAttribute('hidden');
      else
        this.isFrameImage.setAttribute('hidden', 'true');
    }

    // BiDi UI
    this.showItem("context-sep-bidi", top.gBidiUI);
    this.showItem("context-bidi-text-direction-toggle",
                  this.onTextInput && top.gBidiUI);
    this.showItem("context-bidi-page-direction-toggle",
                  !this.onTextInput && top.gBidiUI);

    if (this.onImage) {
      var blockImage = document.getElementById("context-blockimage");

      var uri = this.target
                    .QueryInterface(Ci.nsIImageLoadingContent)
                    .currentURI;

      // this throws if the image URI doesn't have a host (eg, data: image URIs)
      // see bug 293758 for details
      var hostLabel = "";
      try {
        hostLabel = uri.host;
      } catch (ex) { }

      if (hostLabel) {
        var shortenedUriHost = hostLabel.replace(/^www\./i,"");
        if (shortenedUriHost.length > 15)
          shortenedUriHost = shortenedUriHost.substr(0,15) + this.ellipsis;
        blockImage.label = gNavigatorBundle.getFormattedString("blockImages", [shortenedUriHost]);

        if (this.isImageBlocked())
          blockImage.setAttribute("checked", "true");
        else
          blockImage.removeAttribute("checked");
      }
    }

    // Only show the block image item if the image can be blocked
    this.showItem("context-blockimage", this.onImage && hostLabel);
  },

  initSpellingItems: function() {
    var canSpell = InlineSpellCheckerUI.canSpellCheck;
    var onMisspelling = InlineSpellCheckerUI.overMisspelling;
    this.showItem("spell-check-enabled", canSpell);
    this.showItem("spell-separator", false);
    if (canSpell) {
      document.getElementById("spell-check-enabled")
              .setAttribute("checked", InlineSpellCheckerUI.enabled);
    }

    this.showItem("spell-add-to-dictionary", onMisspelling);

    // suggestion list
    this.showItem("spell-suggestions-separator", onMisspelling);
    if (onMisspelling) {
      var menu = document.getElementById("contentAreaContextMenu");
      var suggestionsSeparator =
        document.getElementById("spell-add-to-dictionary");
      var numsug = InlineSpellCheckerUI.addSuggestionsToMenu(menu, suggestionsSeparator, 5);
      this.showItem("spell-no-suggestions", numsug == 0);
    }
    else
      this.showItem("spell-no-suggestions", false);

    // dictionary list
    this.showItem("spell-dictionaries", InlineSpellCheckerUI.enabled);
    if (canSpell) {
      var dictMenu = document.getElementById("spell-dictionaries-menu");
      var dictSep = document.getElementById("spell-language-separator");
      InlineSpellCheckerUI.addDictionaryListToMenu(dictMenu, dictSep);
      this.showItem("spell-add-dictionaries-main", false);
    }
    else if (this.possibleSpellChecking) {
      // when there is no spellchecker but we might be able to spellcheck
      // add the add to dictionaries item. This will ensure that people
      // with no dictionaries will be able to download them
      this.showItem("spell-add-dictionaries-main", true);
    }
    else
      this.showItem("spell-add-dictionaries-main", false);
  },

  initClipboardItems: function() {
    // Copy depends on whether there is selected text.
    // Enabling this context menu item is now done through the global
    // command updating system
    // this.setItemAttr( "context-copy", "disabled", !this.isTextSelected() );
    goUpdateGlobalEditMenuItems();

    this.showItem("context-undo", this.onTextInput);
    this.showItem("context-sep-undo", this.onTextInput);
    this.showItem("context-cut", this.onTextInput);
    this.showItem("context-copyselected", this.isContentSelected);
    this.showItem("context-sep-selected", this.isContentSelected);
    this.showItem("context-copy", this.onTextInput);
    this.showItem("context-paste", this.onTextInput);
    this.showItem("context-delete", this.onTextInput);
    this.showItem("context-sep-paste", this.onTextInput);
    var showSelectAll = !(this.onLink || this.onImage) || this.isDesignMode;
    this.showItem("context-selectall", showSelectAll);
    this.showItem("context-sep-selectall", showSelectAll);

    // XXX dr
    // ------
    // nsDocumentViewer.cpp has code to determine whether we're
    // on a link or an image. we really ought to be using that...

    // Copy link location depends on whether we're on a non-mailto link.
    this.showItem("context-copylink", this.onLink && !this.onMailtoLink);
    this.showItem("context-sep-copylink", this.onLink && this.onImage);

    // Copy image contents depends on whether we're on an image.
    this.showItem("context-copyimage-contents", this.onImage);

    // Copy image location depends on whether we're on an image.
    this.showItem("context-copyimage", this.onImage);
    this.showItem("context-sep-copyimage", this.onImage);
  },
  
  initMediaItems: function () {
    this.showItem("context-playmedia", this.onMedia);
    this.showItem("context-downloadmedia", this.onMedia);
    this.showItem("context-addmediatoplaylist", this.onMedia);
    this.showItem("context-sep-media", this.onMedia);
    
    if (this.onMedia) {
      this.updateAddToPlaylist();
    }
    
    /*
    var showSubscribe = SBDataGetBoolValue("browser.cansubscription") &&
                        !this.onLink &&
                        !this.onImage &&
                        !this.onTextInput;
    this.showItem("context-subscribemedia-page", showSubscribe);
    this.showItem("context-subscribemedia-frame", showSubscribe);
    */
  },

  // Set various context menu attributes based on the state of the world.
  setTarget: function (aNode, aRangeParent, aRangeOffset) {
    const xulNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    if (aNode.namespaceURI == xulNS ||
        this.isTargetAFormControl(aNode)) {
      this.shouldDisplay = false;
    }

    // Initialize contextual info.
    this.onImage           = false;
    this.onLoadedImage     = false;
    this.onCompletedImage  = false;
    this.onStandaloneImage = false;
    this.onCanvas          = false;
    this.onMetaDataItem    = false;
    this.onTextInput       = false;
    this.onKeywordField    = false;
    this.imageURL          = "";
    this.onLink            = false;
    this.linkURL           = "";
    this.linkURI           = null;
    this.linkProtocol      = "";
    this.onMathML          = false;
    this.onMedia           = false;
    this.inFrame           = false;
    this.hasBGImage        = false;
    this.bgImageURL        = "";
    this.possibleSpellChecking = false;

    // Clear any old spellchecking items from the menu, this used to
    // be in the menu hiding code but wasn't getting called in all
    // situations. Here, we can ensure it gets cleaned up any time the
    // menu is shown. Note: must be before uninit because that clears the
    // internal vars
    InlineSpellCheckerUI.clearSuggestionsFromMenu();
    InlineSpellCheckerUI.clearDictionaryListFromMenu();

    InlineSpellCheckerUI.uninit();

    // Remember the node that was clicked.
    this.target = aNode;

    // First, do checks for nodes that never have children.
    if (this.target.nodeType == Node.ELEMENT_NODE) {
      // See if the user clicked on an image.
      if (this.target instanceof Ci.nsIImageLoadingContent &&
          this.target.currentURI) {
        this.onImage = true;
        this.onMetaDataItem = true;
                
        var request =
          this.target.getRequest(Ci.nsIImageLoadingContent.CURRENT_REQUEST);
        if (request && (request.imageStatus & request.STATUS_SIZE_AVAILABLE))
          this.onLoadedImage = true;
        if (request && (request.imageStatus & request.STATUS_LOAD_COMPLETE))
          this.onCompletedImage = true;

        this.imageURL = this.target.currentURI.spec;
        if (this.target.ownerDocument instanceof ImageDocument)
          this.onStandaloneImage = true;
      }
      else if (this.target instanceof HTMLCanvasElement) {
        this.onCanvas = true;
      }
      else if (this.target instanceof HTMLInputElement ) {
        this.onTextInput = this.isTargetATextBox(this.target);
        // allow spellchecking UI on all writable text boxes except passwords
        if (this.onTextInput && ! this.target.readOnly &&
            this.target.type != "password") {
          this.possibleSpellChecking = true;
          InlineSpellCheckerUI.init(this.target.QueryInterface(Ci.nsIDOMNSEditableElement).editor);
          InlineSpellCheckerUI.initFromEvent(aRangeParent, aRangeOffset);
        }
        this.onKeywordField = this.isTargetAKeywordField(this.target);
      }
      else if (this.target instanceof HTMLTextAreaElement) {
        this.onTextInput = true;
        if (!this.target.readOnly) {
          this.possibleSpellChecking = true;
          InlineSpellCheckerUI.init(this.target.QueryInterface(Ci.nsIDOMNSEditableElement).editor);
          InlineSpellCheckerUI.initFromEvent(aRangeParent, aRangeOffset);
        }
      }
      else if (this.target instanceof HTMLHtmlElement) {
        var bodyElt = this.target.ownerDocument.body;
        if (bodyElt) {
          var computedURL = this.getComputedURL(bodyElt, "background-image");
          if (computedURL) {
            this.hasBGImage = true;
            this.bgImageURL = makeURLAbsolute(bodyElt.baseURI,
                                              computedURL);
          }
        }
      }
      else if ("HTTPIndex" in content &&
               content.HTTPIndex instanceof Ci.nsIHTTPIndex) {
        this.inDirList = true;
        // Bubble outward till we get to an element with URL attribute
        // (which should be the href).
        var root = this.target;
        while (root && !this.link) {
          if (root.tagName == "tree") {
            // Hit root of tree; must have clicked in empty space;
            // thus, no link.
            break;
          }

          if (root.getAttribute("URL")) {
            // Build pseudo link object so link-related functions work.
            this.onLink = true;
            this.link = { href : root.getAttribute("URL"),
                          getAttribute: function (aAttr) {
                            if (aAttr == "title") {
                              return root.firstChild.firstChild
                                         .getAttribute("label");
                            }
                            else
                              return "";
                           }
                         };

            // If element is a directory, then you can't save it.
            this.onSaveableLink = root.getAttribute("container") != "true";
          }
          else
            root = root.parentNode;
        }
      }
    }

    // Second, bubble out, looking for items of interest that can have childen.
    // Always pick the innermost link, background image, etc.
    const XMLNS = "http://www.w3.org/XML/1998/namespace";
    var elem = this.target;
    while (elem) {
      if (elem.nodeType == Node.ELEMENT_NODE) {
        // Link?
        if (!this.onLink &&
             ((elem instanceof HTMLAnchorElement && elem.href) ||
              (elem instanceof HTMLAreaElement && elem.href) ||
              elem instanceof HTMLLinkElement ||
              elem.getAttributeNS("http://www.w3.org/1999/xlink", "type") == "simple")) {
            
          // Target is a link or a descendant of a link.
          this.onLink = true;
          this.onMetaDataItem = true;

          // xxxmpc: this is kind of a hack to work around a Gecko bug (see bug 266932)
          // we're going to walk up the DOM looking for a parent link node,
          // this shouldn't be necessary, but we're matching the existing behaviour for left click
          var realLink = elem;
          var parent = elem.parentNode;
          while (parent) {
            try {
              if ((parent instanceof HTMLAnchorElement && parent.href) ||
                  (parent instanceof HTMLAreaElement && parent.href) ||
                  parent instanceof HTMLLinkElement ||
                  parent.getAttributeNS("http://www.w3.org/1999/xlink", "type") == "simple")
                realLink = parent;
            } catch (e) { }
            parent = parent.parentNode;
          }
          
          // Remember corresponding element.
          this.link = realLink;
          this.linkURL = this.getLinkURL();
          this.linkURI = this.getLinkURI();
          this.linkProtocol = this.getLinkProtocol();
          this.onMailtoLink = (this.linkProtocol == "mailto");
          this.onSaveableLink = this.isLinkSaveable( this.link );
        }

        // Metadata item?
        if (!this.onMetaDataItem) {
          // We display metadata on anything which fits
          // the below test, as well as for links and images
          // (which set this.onMetaDataItem to true elsewhere)
          if ((elem instanceof HTMLQuoteElement && elem.cite) ||
              (elem instanceof HTMLTableElement && elem.summary) ||
              (elem instanceof HTMLModElement &&
               (elem.cite || elem.dateTime)) ||
              (elem instanceof HTMLElement &&
               (elem.title || elem.lang)) ||
              elem.getAttributeNS(XMLNS, "lang")) {
            this.onMetaDataItem = true;
          }
        }

        // Background image?  Don't bother if we've already found a
        // background image further down the hierarchy.  Otherwise,
        // we look for the computed background-image style.
        if (!this.hasBGImage) {
          var bgImgUrl = this.getComputedURL( elem, "background-image" );
          if (bgImgUrl) {
            this.hasBGImage = true;
            this.bgImageURL = makeURLAbsolute(elem.baseURI,
                                              bgImgUrl);
          }
        }
      }

      elem = elem.parentNode;
    }
    
    // See if the user clicked on MathML
    const NS_MathML = "http://www.w3.org/1998/Math/MathML";
    if ((this.target.nodeType == Node.TEXT_NODE &&
         this.target.parentNode.namespaceURI == NS_MathML)
         || (this.target.namespaceURI == NS_MathML))
      this.onMathML = true;

    // See if the user clicked in a frame.
    var docDefaultView = this.target.ownerDocument.defaultView;
    if (docDefaultView != docDefaultView.top)
      this.inFrame = true;

    // if the document is editable, show context menu like in text inputs
    var win = this.target.ownerDocument.defaultView;
    if (win) {
      var isEditable = false;
      try {
        var editingSession = win.QueryInterface(Ci.nsIInterfaceRequestor)
                                .getInterface(Ci.nsIWebNavigation)
                                .QueryInterface(Ci.nsIInterfaceRequestor)
                                .getInterface(Ci.nsIEditingSession);
        if (editingSession.windowIsEditable(win) &&
            this.getComputedStyle(this.target, "-moz-user-modify") == "read-write") {
          isEditable = true;
        }
      }
      catch(ex) {
        // If someone built with composer disabled, we can't get an editing session.
      }

      // linkURL is empty (and therefore false) when not over a link
      this.onMedia = this.linkURL &&
                     gTypeSniffer.isValidMediaURL(makeURI(this.linkURL, null, null));

      if (isEditable) {
        this.onTextInput       = true;
        this.onKeywordField    = false;
        this.onImage           = false;
        this.onLoadedImage     = false;
        this.onCompletedImage  = false;
        this.onMetaDataItem    = false;
        this.onMathML          = false;
        this.onMedia           = false;
        this.inFrame           = false;
        this.hasBGImage        = false;
        this.isDesignMode      = true;
        this.possibleSpellChecking = true;
        InlineSpellCheckerUI.init(editingSession.getEditorForWindow(win));
        var canSpell = InlineSpellCheckerUI.canSpellCheck;
        InlineSpellCheckerUI.initFromEvent(aRangeParent, aRangeOffset);
        this.showItem("spell-check-enabled", canSpell);
        this.showItem("spell-separator", false);
      }
    }
  },

  // Returns the computed style attribute for the given element.
  getComputedStyle: function(aElem, aProp) {
    return aElem.ownerDocument
                .defaultView
                .getComputedStyle(aElem, "").getPropertyValue(aProp);
  },

  // Returns a "url"-type computed style attribute value, with the url() stripped.
  getComputedURL: function(aElem, aProp) {
    var url = aElem.ownerDocument
                   .defaultView.getComputedStyle(aElem, "")
                   .getPropertyCSSValue(aProp);
    return url.primitiveType == CSSPrimitiveValue.CSS_URI ?
           url.getStringValue() : null;
  },

  // Returns true if clicked-on link targets a resource that can be saved.
  isLinkSaveable: function(aLink) {
    // We don't do the Right Thing for news/snews yet, so turn them off
    // until we do.
    return this.linkProtocol && !(
             this.linkProtocol == "mailto"     ||
             this.linkProtocol == "javascript" ||
             this.linkProtocol == "news"       ||
             this.linkProtocol == "snews"      );
  },

  openLinkInDefaultBrowser: function() {
    SBOpenURLInDefaultBrowser(this.linkURL);
  },

  openPageInDefaultBrowser: function() {
    SBOpenURLInDefaultBrowser(gBrowser.currentURI.spec);
  },

  openFrameInDefaultBrowser: function() {
    var doc = this.target.ownerDocument;
    var frameURL = doc.documentURIObject.spec;
    SBOpenURLInDefaultBrowser(frameURL);
  },
  openImageInDefaultBrowser: function() {
    SBOpenURLInDefaultBrowser(this.imageURL);
  },

  // Open linked-to URL in a new tab.
  openLinkInTab: function() {
    // Try special handling for playlists and media items before opening a tab
    if (!gBrowser.handleMediaURL(this.linkURL, false, false))
      openNewTabWith(this.linkURL, this.target.ownerDocument, null, null, false);
  },

  // Open frame in a new tab.
  openFrameInTab: function() {
    var doc = this.target.ownerDocument;
    var frameURL = doc.documentURIObject.spec;
    var referrer = doc.referrer;

    openNewTabWith(frameURL, null, null, null, false,
                   referrer ? makeURI(referrer) : null);
  },

  // Reload clicked-in frame.
  reloadFrame: function() {
    this.target.ownerDocument.location.reload();
  },

  // Open clicked-in frame in the same window.
  showOnlyThisFrame: function() {
    var doc = this.target.ownerDocument;
    var frameURL = doc.documentURIObject.spec;

    urlSecurityCheck(frameURL, this.tabbrowser.selectedBrowser.contentPrincipal,
                     Ci.nsIScriptSecurityManager.DISALLOW_SCRIPT);
    var referrer = doc.referrer;
    this.tabbrowser.loadURI(frameURL, referrer ? makeURI(referrer) : null);
  },

  // Open new "view source" window with the frame's URL.
  viewFrameSource: function() {
    BrowserViewSourceOfDocument(this.target.ownerDocument);
  },

  showImage: function(e) {
    urlSecurityCheck(this.imageURL,
                     this.tabbrowser.selectedBrowser.contentPrincipal,
                     Ci.nsIScriptSecurityManager.DISALLOW_SCRIPT);

    if (this.target instanceof Ci.nsIImageLoadingContent)
      this.target.forceReload();
  },

  // Change current window to the URL of the image.
  viewImage: function(e) {
    var viewURL;

    if (this.onCanvas)
      viewURL = this.target.toDataURL();
    else {
      viewURL = this.imageURL;
      urlSecurityCheck(viewURL,
                       this.tabbrowser.selectedBrowser.contentPrincipal,
                       Ci.nsIScriptSecurityManager.DISALLOW_SCRIPT);
    }

    var doc = this.target.ownerDocument;
    openUILink(viewURL, e, null, null, null, null, doc.documentURIObject );
  },

  // Save URL of clicked-on frame.
  saveFrame: function () {
    saveDocument(this.target.ownerDocument);
  },

  // Save URL of clicked-on link.
  saveLink: function() {
    // canonical def in nsURILoader.h
    const NS_ERROR_SAVE_LINK_AS_TIMEOUT = 0x805d0020;
    
    var doc =  this.target.ownerDocument;
    urlSecurityCheck(this.linkURL, doc.nodePrincipal);
    var linkText = this.linkText();
    var linkURL = this.linkURL;


    // an object to proxy the data through to
    // nsIExternalHelperAppService.doContent, which will wait for the
    // appropriate MIME-type headers and then prompt the user with a
    // file picker
    function saveAsListener() {}
    saveAsListener.prototype = {
      extListener: null, 

      onStartRequest: function saveLinkAs_onStartRequest(aRequest, aContext) {

        // if the timer fired, the error status will have been caused by that,
        // and we'll be restarting in onStopRequest, so no reason to notify
        // the user
        if (aRequest.status == NS_ERROR_SAVE_LINK_AS_TIMEOUT)
          return;

        timer.cancel();

        // some other error occured; notify the user...
        if (!Components.isSuccessCode(aRequest.status)) {
          try {
            const sbs = Cc["@mozilla.org/intl/stringbundle;1"].
                        getService(Ci.nsIStringBundleService);
            const bundle = sbs.createBundle(
                    "chrome://mozapps/locale/downloads/downloads.properties");
            
            const title = bundle.GetStringFromName("downloadErrorAlertTitle");
            const msg = bundle.GetStringFromName("downloadErrorGeneric");
            
            const promptSvc = Cc["@mozilla.org/embedcomp/prompt-service;1"].
                              getService(Ci.nsIPromptService);
            promptSvc.alert(doc.defaultView, title, msg);
          } catch (ex) {}
          return;
        }

        var extHelperAppSvc = 
          Cc["@mozilla.org/uriloader/external-helper-app-service;1"].
          getService(Ci.nsIExternalHelperAppService);
        var channel = aRequest.QueryInterface(Ci.nsIChannel);
        this.extListener = 
          extHelperAppSvc.doContent(channel.contentType, aRequest, 
                                    doc.defaultView, true);
        this.extListener.onStartRequest(aRequest, aContext);
      }, 

      onStopRequest: function saveLinkAs_onStopRequest(aRequest, aContext, 
                                                       aStatusCode) {
        if (aStatusCode == NS_ERROR_SAVE_LINK_AS_TIMEOUT) {
          // do it the old fashioned way, which will pick the best filename
          // it can without waiting.
          saveURL(linkURL, linkText, null, true, false, doc.documentURIObject);
        }
        if (this.extListener)
          this.extListener.onStopRequest(aRequest, aContext, aStatusCode);
      },
       
      onDataAvailable: function saveLinkAs_onDataAvailable(aRequest, aContext,
                                                           aInputStream,
                                                           aOffset, aCount) {
        this.extListener.onDataAvailable(aRequest, aContext, aInputStream,
                                         aOffset, aCount);
      }
    }

    // in case we need to prompt the user for authentication
    function callbacks() {}
    callbacks.prototype = {
      getInterface: function sLA_callbacks_getInterface(aIID) {
        if (aIID.equals(Ci.nsIAuthPrompt) || aIID.equals(Ci.nsIAuthPrompt2)) {
          var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
                   getService(Ci.nsIPromptFactory);
          return ww.getPrompt(doc.defaultView, aIID);
        }
        throw Cr.NS_ERROR_NO_INTERFACE;
      } 
    }

    // if it we don't have the headers after a short time, the user 
    // won't have received any feedback from their click.  that's bad.  so
    // we give up waiting for the filename. 
    function timerCallback() {}
    timerCallback.prototype = {
      notify: function sLA_timer_notify(aTimer) {
        channel.cancel(NS_ERROR_SAVE_LINK_AS_TIMEOUT);
        return;
      }
    }

    // set up a channel to do the saving
    var ioService = Cc["@mozilla.org/network/io-service;1"].
                    getService(Ci.nsIIOService);
    var channel = ioService.newChannelFromURI(this.getLinkURI());
    channel.notificationCallbacks = new callbacks();
    channel.loadFlags |= Ci.nsIRequest.LOAD_BYPASS_CACHE |
                         Ci.nsIChannel.LOAD_CALL_CONTENT_SNIFFERS;
    if (channel instanceof Ci.nsIHttpChannel)
      channel.referrer = doc.documentURIObject;

    // fallback to the old way if we don't see the headers quickly 
    var timeToWait = 0;
    try {
      timeToWait = 
        gPrefService.getIntPref("browser.download.saveLinkAsFilenameTimeout");
    } catch (e) {
    }
    var timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    timer.initWithCallback(new timerCallback(), timeToWait,
                           timer.TYPE_ONE_SHOT);

    // kick off the channel with our proxy object as the listener
    channel.asyncOpen(new saveAsListener(), null);
  },

  // Save URL of clicked-on image.
  saveImage: function() {
    var doc =  this.target.ownerDocument;
    if (this.onCanvas) {
      // Bypass cache, since it's a data: URL.
      saveImageURL(this.target.toDataURL(), "canvas.png", "SaveImageTitle",
                   true, false, doc.documentURIObject);
    }
    else {
      urlSecurityCheck(this.imageURL, doc.nodePrincipal);
      saveImageURL(this.imageURL, null, "SaveImageTitle", false,
                   false, doc.documentURIObject);
    }
  },

  toggleImageBlocking: function(aBlock) {
    var permissionmanager = Cc["@mozilla.org/permissionmanager;1"].
                            getService(Ci.nsIPermissionManager);

    var uri = this.target.QueryInterface(Ci.nsIImageLoadingContent).currentURI;

    if (aBlock)
      permissionmanager.add(uri, "image", Ci.nsIPermissionManager.DENY_ACTION);
    else
      permissionmanager.remove(uri.host, "image");

    var brandBundle = document.getElementById("bundle_brand");
    var app = brandBundle.getString("brandShortName");
    var bundle_browser = document.getElementById("bundle_browser");
    var message = bundle_browser.getFormattedString(aBlock ?
     "imageBlockedWarning" : "imageAllowedWarning", [app, uri.host]);

    var notificationBox = this.tabbrowser.getNotificationBox();
    var notification = notificationBox.getNotificationWithValue("images-blocked");

    if (notification)
      notification.label = message;
    else {
      var self = this;
      var buttons = [{
        label: bundle_browser.getString("undo"),
        accessKey: bundle_browser.getString("undo.accessKey"),
        callback: function() { self.toggleImageBlocking(!aBlock); }
      }];
      const priority = notificationBox.PRIORITY_WARNING_MEDIUM;
      notificationBox.appendNotification(message, "images-blocked",
                                         "chrome://browser/skin/Info.png",
                                         priority, buttons);
    }

    // Reload the page to show the effect instantly
    BrowserReload();
  },

  isImageBlocked: function() {
    var permissionmanager = Cc["@mozilla.org/permissionmanager;1"].
                            getService(Ci.nsIPermissionManager);

    var uri = this.target.QueryInterface(Ci.nsIImageLoadingContent).currentURI;

    return permissionmanager.testPermission(uri, "image") == Ci.nsIPermissionManager.DENY_ACTION;
  },

  ///////////////
  // Utilities //
  ///////////////

  // Show/hide one item (specified via name or the item element itself).
  showItem: function(aItemOrId, aShow) {
    var item = aItemOrId.constructor == String ?
      document.getElementById(aItemOrId) : aItemOrId;
    if (item)
      item.hidden = !aShow;
  },

  // Set given attribute of specified context-menu item.  If the
  // value is null, then it removes the attribute (which works
  // nicely for the disabled attribute).
  setItemAttr: function(aID, aAttr, aVal ) {
    var elem = document.getElementById(aID);
    if (elem) {
      if (aVal == null) {
        // null indicates attr should be removed.
        elem.removeAttribute(aAttr);
      }
      else {
        // Set attr=val.
        elem.setAttribute(aAttr, aVal);
      }
    }
  },

  // Set context menu attribute according to like attribute of another node
  // (such as a broadcaster).
  setItemAttrFromNode: function(aItem_id, aAttr, aOther_id) {
    var elem = document.getElementById(aOther_id);
    if (elem && elem.getAttribute(aAttr) == "true")
      this.setItemAttr(aItem_id, aAttr, "true");
    else
      this.setItemAttr(aItem_id, aAttr, null);
  },

  // Temporary workaround for DOM api not yet implemented by XUL nodes.
  cloneNode: function(aItem) {
    // Create another element like the one we're cloning.
    var node = document.createElement(aItem.tagName);

    // Copy attributes from argument item to the new one.
    var attrs = aItem.attributes;
    for (var i = 0; i < attrs.length; i++) {
      var attr = attrs.item(i);
      node.setAttribute(attr.nodeName, attr.nodeValue);
    }

    // Voila!
    return node;
  },

  // Generate fully qualified URL for clicked-on link.
  getLinkURL: function() {
    var href = this.link.href;  
    if (href)
      return href;

    href = this.link.getAttributeNS("http://www.w3.org/1999/xlink",
                                    "href");

    if (!href || !href.match(/\S/)) {
      // Without this we try to save as the current doc,
      // for example, HTML case also throws if empty
      throw "Empty href";
    }

    return makeURLAbsolute(this.link.baseURI, href);
  },
  
  getLinkURI: function() {
    var ioService = Cc["@mozilla.org/network/io-service;1"].
                    getService(Ci.nsIIOService);
    try {
      return ioService.newURI(this.linkURL, null, null);
    }
    catch (ex) {
     // e.g. empty URL string
    }

    return null;
  },
  
  getLinkProtocol: function() {
    if (this.linkURI)
      return this.linkURI.scheme; // can be |undefined|

    return null;
  },

  // Get text of link.
  linkText: function() {
    var text = gatherTextUnder(this.link);
    if (!text || !text.match(/\S/)) {
      text = this.link.getAttribute("title");
      if (!text || !text.match(/\S/)) {
        text = this.link.getAttribute("alt");
        if (!text || !text.match(/\S/))
          text = this.linkURL;
      }
    }

    return text;
  },

  // Get selected text. Only display the first 15 chars.
  isTextSelection: function() {
    // Get 16 characters, so that we can trim the selection if it's greater
    // than 15 chars
    var selectedText = getBrowserSelection(16);

    if (!selectedText)
      return false;

    if (selectedText.length > 15)
      selectedText = selectedText.substr(0,15) + this.ellipsis;

    // Use the current engine if the search bar is visible, the default
    // engine otherwise.
    var engineName = "";
    var ss = Cc["@mozilla.org/browser/search-service;1"].
             getService(Ci.nsIBrowserSearchService);
    if (isElementVisible(BrowserSearch.getSearchBar()))
      engineName = ss.currentEngine.name;
    else
      engineName = ss.defaultEngine.name;

    // format "Search <engine> for <selection>" string to show in menu
    var menuLabel = gNavigatorBundle.getFormattedString("contextMenuSearchText",
                                                        [engineName,
                                                         selectedText]);
    document.getElementById("context-searchselect").label = menuLabel;

    return true;
  },

  // Returns true if anything is selected.
  isContentSelection: function() {
    return !document.commandDispatcher.focusedWindow.getSelection().isCollapsed;
  },

  toString: function () {
    return "contextMenu.target     = " + this.target + "\n" +
           "contextMenu.onImage    = " + this.onImage + "\n" +
           "contextMenu.onLink     = " + this.onLink + "\n" +
           "contextMenu.link       = " + this.link + "\n" +
           "contextMenu.inFrame    = " + this.inFrame + "\n" +
           "contextMenu.hasBGImage = " + this.hasBGImage + "\n";
  },

  // Returns true if aNode is a from control (except text boxes).
  // This is used to disable the context menu for form controls.
  isTargetAFormControl: function(aNode) {
    if (aNode instanceof HTMLInputElement)
      return (aNode.type != "text" && aNode.type != "password");

    return (aNode instanceof HTMLButtonElement) ||
           (aNode instanceof HTMLSelectElement) ||
           (aNode instanceof HTMLOptionElement) ||
           (aNode instanceof HTMLOptGroupElement);
  },

  isTargetATextBox: function(node) {
    if (node instanceof HTMLInputElement)
      return (node.type == "text" || node.type == "password")

    return (node instanceof HTMLTextAreaElement);
  },

  isTargetAKeywordField: function(aNode) {
    var form = aNode.form;
    if (!form)
      return false;

    var method = form.method.toUpperCase();

    // These are the following types of forms we can create keywords for:
    //
    // method   encoding type       can create keyword
    // GET      *                                 YES
    //          *                                 YES
    // POST                                       YES
    // POST     application/x-www-form-urlencoded YES
    // POST     text/plain                        NO (a little tricky to do)
    // POST     multipart/form-data               NO
    // POST     everything else                   YES
    return (method == "GET" || method == "") ||
           (form.enctype != "text/plain") && (form.enctype != "multipart/form-data");
  },

  // Determines whether or not the separator with the specified ID should be
  // shown or not by determining if there are any non-hidden items between it
  // and the previous separator.
  shouldShowSeparator: function (aSeparatorID) {
    var separator = document.getElementById(aSeparatorID);
    if (separator) {
      var sibling = separator.previousSibling;
      while (sibling && sibling.localName != "menuseparator") {
        if (!sibling.hidden)
          return true;
        sibling = sibling.previousSibling;
      }
    }
    return false;
  },

  addDictionaries: function() {
    var uri;
    try {
      uri = formatURL("browser.dictionaries.download.url", true);
    } catch (e) {
      Components.utils.reportError("Dictionnary URL not found in prefs");
    }

    var locale = "-";
    try {
      locale = gPrefService.getComplexValue("intl.accept_languages",
                                            Ci.nsIPrefLocalizedString).data;
    }
    catch (e) { }

    var version = "-";
    try {
      version = Cc["@mozilla.org/xre/app-info;1"].
                getService(Ci.nsIXULAppInfo).version;
    }
    catch (e) { }

    uri = uri.replace(/%LOCALE%/, escape(locale)).replace(/%VERSION%/, version);

    //xxxlone no new window in songbird, force new tab
    //var newWindowPref = gPrefService.getIntPref("browser.link.open_newwindow");
    //var where = newWindowPref == 3 ? "tab" : "window";
    var where = "tab";

    openUILinkIn(uri, where);
  },

  savePageAs: function CM_savePageAs() {
    saveDocument(this.tabbrowser.selectedBrowser.contentDocument);
  },

  switchPageDirection: function CM_switchPageDirection() {
    SwitchDocumentDirection(this.tabbrowser.selectedBrowser.contentWindow);
  },
  
  // recreate the list of menuitems for 'add to playlist'
  updateAddToPlaylist: function CM_updateAddToPlaylist() {
    var sep = document.getElementById("context-sep-playlists");
    var popup = sep.parentNode;
    var elements = document.getElementsByAttribute("type", "addtoplaylist");
    while (elements.length > 0) {
      popup.removeChild(elements[0]);
    }
    var libraryManager = Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
                         .getService(Components.interfaces.sbILibraryManager);
    var libs = libraryManager.getLibraries();
    var nadded = 0;
    while (libs.hasMoreElements()) {
      var library = libs.getNext();
      nadded += this.updateAddToPlaylistForLibrary(library);
    }
    
    // show the 'no playlist' item only if we didn't add any ourselves
    this.showItem("context-addmediatoplaylist-noplaylist", (nadded == 0));
  },
  
  // create all the menuitems for 'add to playlist' for a particular library
  updateAddToPlaylistForLibrary: function CM_addToPlayListForLibrary(aLibrary) {
    // we insert all the items before the separator
    var sep = document.getElementById("context-sep-playlists");
    var popup = sep.parentNode;
    var nadded = 0;
    // listener receives all the playlists for this library
    var listener = {
      obj: this,
      items: [],
      _downloadListGUID: null,
      _libraryServicePane: null,
      onEnumerationBegin: function() {
        var ddh = Components.classes["@songbirdnest.com/Songbird/DownloadDeviceHelper;1"]
                            .getService(Components.interfaces.sbIDownloadDeviceHelper);
        var downloadMediaList = ddh.getDownloadMediaList();
        if (downloadMediaList)
          this._downloadListGUID = downloadMediaList.guid;

        this._libraryServicePane = 
          Components.classes['@songbirdnest.com/servicepane/library;1']
          .getService(Components.interfaces.sbILibraryServicePaneService);
      },
      onEnumerationEnd: function() { },
      onEnumeratedItem: function(list, item) {
        // discard hidden and non-simple playlists
        var hidden = item.getProperty("http://songbirdnest.com/data/1.0#hidden");
        if (hidden == "1" ||
            item.type != "simple") {
          return Components.interfaces.sbIMediaListEnumerationListener.CONTINUE;
        }
        
        // discard more playlists

        // XXXlone use policy system when bug 4017 is fixed
        if (item.guid == this._downloadListGUID) {
          return Components.interfaces.sbIMediaListEnumerationListener.CONTINUE;
        }
        
        // XXXlone use policy system when bug 4017 is fixed
        function isHidden(node) {
          while (node) {
            if (node.hidden) return true;
            node = node.parentNode;
          }
          return false;
        }
        var node = this._libraryServicePane.getNodeForLibraryResource(item);
        if (!node || isHidden(node)) {
          return Components.interfaces.sbIMediaListEnumerationListener.CONTINUE;
        }

        // we want this playlist in the menu, make an item for it
        var menuitem = document.createElement("menuitem");
        menuitem.setAttribute("type", "addtoplaylist");
        menuitem.setAttribute("library", aLibrary.guid);
        menuitem.setAttribute("playlist", item.guid);
        if (!item.name ||
             item.name == "") {
          songbird_bundle = document.getElementById("songbird_strings");
          menuitem.setAttribute("label", songbird_bundle.getString("addMediaToPlaylistCmd.unnamedPlaylist"));
        } else {
          menuitem.setAttribute("label", item.name);
        }
        menuitem.setAttribute("oncommand", "gContextMenu.addMediaToPlaylist(event);");
        popup.insertBefore(menuitem, sep);
        
        // count the number of items we created
        nadded++;

        // keep enumerating please
        return Components.interfaces.sbIMediaListEnumerationListener.CONTINUE;
      }
    };

    // start the enumeration
    aLibrary.enumerateItemsByProperty("http://songbirdnest.com/data/1.0#isList", "1",
                                        listener );
    
    // return the number of items we created
    return nadded;
  },
  
  // called when an 'add to playlist' menu item has been clicked
  addMediaToPlaylist: function CM_addMediaToPlaylist(aEvent) {
    // get the playlist that is targeted by the menu that triggered the event
    var libraryguid = aEvent.target.getAttribute("library");
    var playlistguid = aEvent.target.getAttribute("playlist");

    var libraryManager = Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
                         .getService(Components.interfaces.sbILibraryManager);
    var library = libraryManager.getLibrary(libraryguid);
    if (library) {
      var playlist = library.getMediaItem(playlistguid);
      if (playlist) {
        // add the item to that playlist
        this._addMediaToPlaylist(playlist);
        return;
      }
    }
    // couldn't find either the library or the playlist, that's bad.
    throw new Error("addMediaToPlaylist invoked with invalid playlist");
  },
  
  // called when the 'new playlist' menuitem has been clicked
  addMediaToNewPlaylist: function CM_addMediaToNewPlaylist() {
    // create a new playlist, the servicepane will take over and
    // enter edition mode, but the playlist will be created
    // immediately, only with a temporary name
    var newMediaList = window.makeNewPlaylist("simple");
    this._addMediaToPlaylist(newMediaList);
  },

  // add the context link to a playlist
  _addMediaToPlaylist: function CM__addMediaToPlaylist(aPlaylist) {
    // we use the drop helper code because it does everything we need,
    // including starting a metadata scanning job, and reporting the
    // result of the action on the statusbar
    ExternalDropHandler.dropUrlsOnList(window, [this.linkURL], aPlaylist, -1, null);
  },
  
  downloadMedia: function CM_downloadMedia() {
    // find the item in the web library, it should be there because the user
    // right clicked on a url to get here, so it should have been handled by
    // the scraper.
    var item = 
      getFirstItemByProperty(LibraryUtils.webLibrary, 
                             "http://songbirdnest.com/data/1.0#contentURL", 
                             this.linkURL);
    if (!item)                         
      item = 
        getFirstItemByProperty(LibraryUtils.webLibrary, 
                               "http://songbirdnest.com/data/1.0#originURL", 
                               this.linkURL);
    // still, it's possible to right click on a media url without having a corresponding 
    // item in the web library, because maybe the user has disabled the scraper somehow, 
    // or it failed, or there is another tab open with the web library and the user has
    // deleted the track from it without reloading the current webpage, or something.
    // so:
    if (!item) {
      // if the item does not exist, drop it into the web library
      var dropHandlerListener = {
        onDropComplete: function(aTargetList,
                                 aImportedInLibrary,
                                 aDuplicates,
                                 aInsertedInMediaList,
                                 aOtherDropsHandled) {
          // do not show the standard report on the status bar
          return false;
        },
        onFirstMediaItem: function(aTargetList, aFirstMediaItem) { },
      };
      ExternalDropHandler.dropUrlsOnList(window, [this.linkURL], LibraryUtils.webLibrary, -1, dropHandlerListener);
      // we only gave a single item, it'll be dropped synchronously.
      // look for the item again
      item = getFirstItemByProperty(LibraryUtils.webLibrary, 
                                    "http://songbirdnest.com/data/1.0#contentURL", 
                                    this.linkURL);
      if (!item) {
        throw new Error("Failed to find media item after dropping it in the web library");
      }
    }
    
    // start downloading the item
    var ddh = Components.classes["@songbirdnest.com/Songbird/DownloadDeviceHelper;1"]
                                .getService(Components.interfaces.sbIDownloadDeviceHelper);
    ddh.downloadItem(item);
  },
  
  playMedia: function CM_playMedia() {
    gBrowser.handleMediaURL(this.linkURL, true, false);
  },
  
  subscribeMediaPage: function CM_subscribeToPage() {
    this._subscribeMedia(gBrowser.currentURI);
  },
  
  subscribeMediaFrame: function CM_subscribeToFrame() {
    var doc = this.target.ownerDocument;
    this._subscribeMedia(doc.documentURIObject);
  },
  
  _subscribeMedia: function CM_subscribeToURL(aURI) {
    SBSubscribe(null, aURI);
  }

};

