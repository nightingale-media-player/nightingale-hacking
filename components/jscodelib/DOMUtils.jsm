/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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

/**
 * \file  DOMUtils.jsm
 * \brief Javascript source for the DOM utility services.
 */

//------------------------------------------------------------------------------
//
// DOM utility JSM configuration.
//
//------------------------------------------------------------------------------

EXPORTED_SYMBOLS = [ "DOMUtils", "sbDOMHighlighter", "DOMEventListenerSet" ];


//------------------------------------------------------------------------------
//
// DOM utility defs.
//
//------------------------------------------------------------------------------

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results
const Cu = Components.utils


//------------------------------------------------------------------------------
//
// DOM utility services.
//
//------------------------------------------------------------------------------

var DOMUtils = {
  //----------------------------------------------------------------------------
  //
  // DOM utility services.
  //
  //----------------------------------------------------------------------------

  /**
   * Load a document using the URI string specified by aDocumentURI and return
   * it as an nsIDOMDocument.
   *
   * \param aDocumentURI        Document URI string.
   *
   * \return                    An nsIDOMDocument document.
   */

  loadDocument: function DOMUtils_loadDocument(aDocumentURI) {
    // Open the document.
    var ioSvc = Cc["@mozilla.org/network/io-service;1"]
                  .getService(Ci.nsIIOService);
    var channel = ioSvc.newChannel(aDocumentURI, null, null);
    var inputStream = channel.open();

    // Load and return the document.
    var domParser = Cc["@mozilla.org/xmlextras/domparser;1"]
                      .createInstance(Ci.nsIDOMParser);
    return domParser.parseFromStream(inputStream,
                                     null,
                                     channel.contentLength,
                                     "text/xml");
  },


  /**
   *   Import the child elements of the parent element with the ID specified by
   * aSrcParentID within the document specified by aSrcDocument.  Import them
   * as child elements of the node specified by aDstNode.
   *   For each imported child, set attributes as specified by the
   * aChildAttrList object.  Use the name of each field in aChildAttrList as the
   * name of an attribute, and set the attribute value to the field value.
   *   Note that only child elements are imported.  Non-element child nodes such
   * as text nodes are are not imported.  The descendents of all imported child
   * elements are also imported, including non-element descendents.
   *
   * \param aDstNode            Destination node into which to import child
   *                            elements.
   * \param aSrcDocument        Document from which to import child elements.
   * \param aSrcParentID        ID of parent node from which to import child
   *                            elements.
   * \param aChildAttrList      List of child attributes to set.
   */

  importChildElements: function DOMUtils_importChildElements(aDstNode,
                                                             aSrcDocument,
                                                             aSrcParentID,
                                                             aChildAttrList) {
    // Get the destination document and the list of source children.
    var dstDoc = aDstNode.ownerDocument;
    var srcChildList = aSrcDocument.getElementById(aSrcParentID).childNodes;

    // Import the source elements.
    for (var i = 0; i < srcChildList.length; i++) {
      // Get the next source child.  Skip if not an element.
      var srcChild = srcChildList[i];
      if (srcChild.nodeType != Ci.nsIDOMNode.ELEMENT_NODE)
        continue;

      // Import the source child into the destination document.
      var dstChild = dstDoc.importNode(srcChild, true);

      // Add the child to the destination node.
      aDstNode.appendChild(dstChild);

      // Add the child attributes.
      for (var attrName in aChildAttrList) {
        dstChild.setAttribute(attrName, aChildAttrList[attrName]);
      }
    }
  },


  /**
   * Copy the attributes specified by aAttributeList from the element specified
   * by aSrcElem to the element specified by aDstElem.  If an attribute is not
   * set in aSrcElem, do not change that attribute in aDstElem unless
   * aRemoveAttributes is true; if aRemoveAttributes is true, remove the
   * attribute from aDstElem.
   *
   * \param aSrcElem            Source element.
   * \param aDstElem            Destination element.
   * \param aAttributeList      Array of attribute names.
   * \param aRemoveAttributes   Remove attributes from aDstElem that aren't set
   *                            in aSrcElem.
   */

  copyAttributes: function DOMUtils_copyAttributes(aSrcElem,
                                                   aDstElem,
                                                   aAttributeList,
                                                   aRemoveAttributes) {
    // Copy the attributes.
    for (var i = 0; i < aAttributeList.length; i++) {
      // Get attribute name.
      var attribute = aAttributeList[i];

      // If source element does not have the attribute, do nothing or remove
      // the attribute as specified.
      if (!aSrcElem.hasAttribute(attribute)) {
        if (aRemoveAttributes)
          aDstElem.removeAttribute(attribute);
        continue;
      }

      // Copy the attribute from the source element to the destination if the
      // attribute values differ.
      var srcAttributeVal = aSrcElem.getAttribute(attribute);
      var dstAttributeVal = aDstElem.getAttribute(attribute);
      if (srcAttributeVal != dstAttributeVal)
        aDstElem.setAttribute(attribute, srcAttributeVal);
    }
  },


  /**
   * Search the children of the element specified by aRootElem for elements
   * with the attribute specified by aAttrName set to the value specified by
   * aAttrValue.  Return an array containing all matching elements.
   * If aAttrValue is not specified, return all elements with the specified
   * attribute, regardless of value.
   *
   * \param aRootElem           Root element from which to start searching.
   * \param aAttrName           Attribute name to search for.
   * \param aAttrValue          Attribute value to search for.
   *
   * \return                    Array containing found elements.
   */

  getElementsByAttribute: function DOMUtils_getElementsByAttribute(aRootElem,
                                                                   aAttrName,
                                                                   aAttrValue) {
    // Start searching for elements from the root.
    var matchList = [];
    this._getElementsByAttribute(aRootElem, aAttrName, aAttrValue, matchList);

    return matchList;
  },

  _getElementsByAttribute: function
                             DOMUtils__getElementsByAttribute(aRootElem,
                                                              aAttrName,
                                                              aAttrValue,
                                                              aMatchList) {
    // Search each of the children.
    var childList = aRootElem.childNodes;
    for (var i = 0; i < childList.length; i++) {
      // Check the child node for a match.
      var child = childList[i];
      if (child.hasAttributes() && child.hasAttribute(aAttrName)) {
        if (!aAttrValue || (child.getAttribute(aAttrName) == aAttrValue))
          aMatchList.push(child);
      }

      // Check each of the child's children.
      this._getElementsByAttribute(child, aAttrName, aAttrValue, aMatchList);
    }
  },


  /**
   * Encode the text specified by aText for use in an XML document and return
   * the result (e.g., encode "this & that" to "this &amp; that".
   *
   * \param aText               Text to encode.
   *
   * \return                    Text encoded for XML.
   */

  encodeTextForXML: function DOMUtils_encodeTextForXML(aText) {
    // Create an XML text node.
    var xmlDocument = Cc["@mozilla.org/xml/xml-document;1"]
                        .createInstance(Ci.nsIDOMDocument);
    var xmlTextNode = xmlDocument.createTextNode(aText);

    // Produce the encoded XML text by serializing the XML text node.
    var domSerializer = Cc["@mozilla.org/xmlextras/xmlserializer;1"]
                          .createInstance(Ci.nsIDOMSerializer);
    var encodedXMLText = domSerializer.serializeToString(xmlTextNode);

    return encodedXMLText;
  },


  //----------------------------------------------------------------------------
  //
  // DOM class attribute services.
  //
  //----------------------------------------------------------------------------

  /**
   * Set the class specified by aClass within the class attribute of the element
   * specified by aElem.
   *
   * \param aElem               Element for which to set class.
   * \param aClass              Class to set.
   */

  setClass: function DOMUtils_setClass(aElem, aClass) {
    // Get the class attribute.  If it's empty, just set the class and return.
    var classAttr = aElem.getAttribute("class");
    if (!classAttr) {
      aElem.setAttribute("class", aClass);
      return;
    }

    // Set class if not already set.
    var classList = classAttr.split(" ");
    if (classList.indexOf(aClass) < 0) {
      classList.push(aClass);
      aElem.setAttribute("class", classList.join(" "));
    }
  },


  /**
   * Clear the class specified by aClass from the class attribute of the element
   * specified by aElem.
   *
   * \param aElem               Element for which to clear class.
   * \param aClass              Class to clear.
   */

  clearClass: function DOMUtils_clearClass(aElem, aClass) {
    // Get the class attribute.  If it's empty, just return.
    var classAttr = aElem.getAttribute("class");
    if (!classAttr)
      return;

    // Clear specified class from class attribute.
    var classList = classAttr.split(" ");
    var classCount = classList.length;
    for (var i = classCount - 1; i >= 0; i--) {
      if (classList[i] == aClass)
        classList.splice(i, 1);
    }
    if (classList.length < classCount)
      aElem.setAttribute("class", classList.join(" "));
  },


  /**
   * Return true if the class specified by aClass is set within the class
   * attribute of the element specified by aElem.
   *
   * \param aElem               Element for which to check if class is set.
   * \param aClass              Class for which to check.
   *
   * \return                    True if class is set.
   */

  isClassSet: function DOMUtils_isClassSet(aElem, aClass) {
    var classAttr = aElem.getAttribute("class");
    var classList = classAttr.split(" ");
    if (classList.indexOf(aClass) < 0)
      return false;

    return true;
  },


  //----------------------------------------------------------------------------
  //
  // DOM node destruction services.
  //
  //   When a node is removed from a DOM tree, any XBL bindings bound to that
  // node or its children are not detached.  Thus, the binding destructors are
  // not called.  The XBL bindings are not detached until the owner document is
  // destroyed.  This can lead to memory leaks if XBL bound nodes are
  // dynamically added and removed from a document.
  //   The DOM node destruction services provide support for releasing XBL
  // binding resources before the XBL binding is detached.  An XBL binding may
  // add a destroy function by calling addNodeDestroyFunc.  Multiple destroy
  // functions may be added for an XBL binding (e.g., for extended bindings).
  //   Before a node is removed from a document, destroyNode should be called.
  // This function recursively calls the destroy functions for all of the nodes
  // children, including the anonymous children.  Note that destroyNode must be
  // called before node removal since node removal removes XBL binding content.
  // If destroyNode is called after node removal, anonymous child nodes will not
  // be destroyed.
  //
  //----------------------------------------------------------------------------

  /**
   * Add the destroy function specified by aFunc to the list of destroy
   * functions for the node specified by aNode.  The destroy function is only
   * called once and is removed from the destroy function list when called.
   *
   * \param aNode               Node for which to add destroy function.
   * \param aFunc               Destroy function.
   */

  addNodeDestroyFunc: function DOMUtils_addNodeDestroyFunc(aNode,
                                                           aFunc) {
    // Ensure the destroy function list exists.
    if (!aNode.destroyFuncList)
      aNode.destroyFuncList = [];

    // Push the destroy function on the end of the list.
    aNode.destroyFuncList.push(aFunc);
  },


  /**
   * Recursively call the destroy functions for all child nodes, including
   * anonymous nodes, for the node specified by aNode.
   *
   * \param aNode               Node to destroy.
   */

  destroyNode: function DOMUtils_destroyNode(aNode) {
    /* Can only destroy element nodes. */
    if (aNode.nodeType != Ci.nsIDOMNode.ELEMENT_NODE)
      return;

    // Destroy all of the node's children, including the anonymous children.
    var nodeDocument = aNode.ownerDocument;
    this.destroyNodeList(nodeDocument.getAnonymousNodes(aNode));
    this.destroyNodeList(aNode.childNodes);

    // Call the node destroy functions.
    while (aNode.destroyFuncList) {
      // Pop the next destroy function from the end of the list and call it.
      var destroyFunc = aNode.destroyFuncList.pop();
      try {
        destroyFunc();
      } catch (ex) {
        var msg = "Exception: " + ex +
                  " at " + ex.fileName +
                  ", line " + ex.lineNumber;
        Cu.reportError(msg);
      }

      // If the destroy function list is empty, remove it.
      if (!aNode.destroyFuncList.length)
        aNode.destroyFuncList = null;
    }
  },


  /**
   * Destroy all nodes in the list specified by aNodeList.
   *
   * \param aNodeList           Array list of nodes to destroy.
   */

  destroyNodeList: function DOMUtils_destroyNodeList(aNodeList) {
    if (!aNodeList)
      return;

    for (var i = 0; i < aNodeList.length; i++) {
      this.destroyNode(aNodeList[i]);
    }
  }
};


//------------------------------------------------------------------------------
//
// DOM highlighter services.
//
//   These services may be used to apply highlighting effects to DOM elements.
//   Highlighting is applied by setting a DOM element attribute to a particular
// value.
//   The highlighting effect blinks the highlighting for a specified period of
// time and then leaves the highlight applied.  When the user clicks anywhere,
// the highlighting is removed.
//
//------------------------------------------------------------------------------

/**
 *   Construct a DOM highlighter object to highlight the element specified by
 * aElement within the window specified by aWindow and return the highlighter
 * object.
 *   The element is highlighted by setting one of its attributes to a particular
 * value.  A CSS rule can then be set up to apply highlight styling to the
 * element based on the attribute and value.
 *   The attribute to set is specified by aAttribute.  The attribute is set to
 * the value specified by aValue.
 *   If the attribute is "class", the value is added to the class attribute
 * instead of replacing it entirely.
 *   If aAttribute is not specified, the "class" attribute is used as the
 * default.
 *   If aValue is not specified, a default value is also used.  If the "class"
 * attribute is being used, "highlight" is used as the value to add to the
 * "class" attribute.  Otherwise, the attribute is set to the value "true".
 *   Blink the highlighting for the duration specified by aBlinkDuration.  If
 * the duration is not specified, choose a default duration.  If the duration is
 * negative, do not blink.
 *
 * \param aWindow               Window containing element.
 * \param aElement              Element to highlight.
 * \param aValue                Value to set highlight.
 * \param aAttribute            Attribute to use to highlight.
 * \param aBlinkDuration        Duration of blinking in milliseconds.
 */

function sbDOMHighlighter(aWindow,
                          aElement,
                          aValue,
                          aAttribute,
                          aBlinkDuration) {
  // Get the attribute to highlight, defaulting to "class".
  var attribute = aAttribute;
  if (!attribute)
    attribute = "class";

  // Get the attribute value with which to highlight.  Choose an appropriate
  // default if no value was specified.
  var value = aValue;
  if (!value) {
    if (attribute == "class") {
      value = "highlight";
    }
    else {
      value = "true";
    }
  }

  // Get the blink duration.
  var blinkDuration = aBlinkDuration;
  if (!blinkDuration)
    blinkDuration = this.defaultBlinkDuration;

  // Save parameters.
  this._window = aWindow;
  this._element = aElement;
  this._value = value;
  this._attribute = attribute;
  this._blinkDuration = blinkDuration;
}


/**
 *   Highlight the element specified by aElement within the window specified by
 * aWindow and return the highlighter object.
 *   The element is highlighted by setting one of its attributes to a particular
 * value.  A CSS rule can then be set up to apply highlight styling to the
 * element based on the attribute and value.
 *   The attribute to set is specified by aAttribute.  The attribute is set to
 * the value specified by aValue.
 *   If the attribute is "class", the value is added to the class attribute
 * instead of replacing it entirely.
 *   If aAttribute is not specified, the "class" attribute is used as the
 * default.
 *   If aValue is not specified, a default value is also used.  If the "class"
 * attribute is being used, "highlight" is used as the value to add to the
 * "class" attribute.  Otherwise, the attribute is set to the value "true".
 *   Blink the highlighting for the duration specified by aBlinkDuration.  If
 * the duration is not specified, choose a default duration.  If the duration is
 * negative, do not blink.
 *
 * \param aWindow               Window containing element.
 * \param aElement              Element to highlight.
 * \param aValue                Value to set highlight.
 * \param aAttribute            Attribute to use to highlight.
 * \param aBlinkDuration        Duration of blinking in milliseconds.
 *
 * \return                      Highlighter object.
 */

sbDOMHighlighter.highlightElement =
  function sbDOMHighlighter_highlightElement(aWindow,
                                             aElement,
                                             aValue,
                                             aAttribute,
                                             aBlinkDuration) {
  // Create and start a highlighter.
  var highlighter = new sbDOMHighlighter(aWindow,
                                         aElement,
                                         aValue,
                                         aAttribute,
                                         aBlinkDuration);
  highlighter.start();

  return highlighter;
}


// Define the class.
sbDOMHighlighter.prototype = {
  // Set the constructor.
  constructor: sbDOMHighlighter,

  //
  // DOM highlighter configuration.
  //
  //   blinkPeriod              Blink period in milliseconds.
  //   defaultBlinkDuration     Default blink duration in milliseconds.
  //

  blinkPeriod: 250,
  defaultBlinkDuration: 2000,


  //
  // DOM highlighter fields.
  //
  //   _window                  Window containing element.
  //   _element                 Element to highlight.
  //   _attribute               Attribute to use to highlight.
  //   _value                   Value to set highlight.
  //   _blinkDuration           Duration of blinking.
  //   _domEventListenerSet     Set of DOM event listeners.
  //   _blinkTimer              Timer used for blinking.
  //   _blinkStart              Timestamp of start of blinking.
  //   _isHighlighted           If true, element is currently highlighted.
  //

  _window: null,
  _element: null,
  _attribute: null,
  _value: null,
  _blinkDuration: -1,
  _domEventListenerSet: null,
  _blinkTimer: null,
  _blinkStart: 0,
  _isHighlighted: false,


  /**
   * Start the highlight effect.
   */

  start: function sbDOMHighlighter_start() {
    var self = this;
    var func;

    // Create a DOM event listener set.
    this._domEventListenerSet = new DOMEventListenerSet();

    // Listen to document click and window unload events.
    func = function _onClickWrapper() { self._onClick(); };
    this._domEventListenerSet.add(this._element.ownerDocument,
                                  "click",
                                  func,
                                  true);
    if (this._window) {
      func = function _onUnloadWrapper() { self._onUnload(); };
      this._domEventListenerSet.add(this._window, "unload", func, false);
    }

    // Highlight element.
    this.setHighlight();

    // Start blinking.
    this._blinkStart = Date.now();
    var self = this;
    var func = function _blinkWrapper() { self._blink(); };
    this._blinkTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this._blinkTimer.initWithCallback(func,
                                      this.blinkPeriod,
                                      Ci.nsITimer.TYPE_REPEATING_SLACK);
  },


  /**
   * Cancel the highlighter.
   */

  cancel: function sbDOMHighlighter_cancel() {
    // Remove DOM event listeners.
    if (this._domEventListenerSet)
      this._domEventListenerSet.removeAll();

    // Cancel blink timer.
    if (this._blinkTimer)
      this._blinkTimer.cancel();

    // Remove object references.
    this._element = null;
    this._domEventListenerSet = null;
    this._blinkTimer = null;
  },


  /**
   * Set the highlight on the element.
   */

  setHighlight: function sbDOMHighlighter_setHighlight() {
    // Apply highlight.
    if (this._attribute == "class") {
      DOMUtils.setClass(this._element, this._value);
    }
    else {
      this._element.setAttribute(this._attribute, this._value);
    }

    // Indicate that the element is now highlighted.
    this._isHighlighted = true;
  },


  /**
   * Clear the highlight on the element.
   */

  clearHighlight: function sbDOMHighlighter_clearHighlight() {
    // Remove highlight.
    if (this._attribute == "class") {
      DOMUtils.clearClass(this._element, this._value);
    }
    else {
      this._element.removeAttribute(this._attribute);
    }

    // Indicate that the element is no longer highlighted.
    this._isHighlighted = false;
  },


  //----------------------------------------------------------------------------
  //
  // Internal DOM highlighter services.
  //
  //----------------------------------------------------------------------------

  /**
   * Blink the element highlight.
   */

  _blink: function sbDOMHighlighter__blink() {
    // If the blink duration has been reached, set the highlight and stop the
    // blinking.
    var blinkTime = Date.now() - this._blinkStart;
    if (blinkTime >= this._blinkDuration) {
      this._blinkTimer.cancel();
      this._blinkTimer = null;
      this.setHighlight();
      return;
    }

    // Blink the element highlight.
    if (this._isHighlighted) {
      this.clearHighlight();
    }
    else {
      this.setHighlight();
    }
  },


  /**
   * Handle a click event.
   */

  _onClick: function sbDOMHighlighter__onClick() {
    // Clear highlight and cancel highlighter.
    this.clearHighlight();
    this.cancel();
  },


  /**
   * Handle an unload event.
   */

  _onUnload: function sbDOMHighlighter__onUnload() {
    // Cancel highlighter.
    this.cancel();
  }
};


//------------------------------------------------------------------------------
//
// DOM event listener set services.
//
//   These services may be used to maintain a set of DOM event listeners and to
// facilitate the removal of DOM event listeners.
//
//------------------------------------------------------------------------------

/**
 * Construct a DOM event listener set object.
 */

function DOMEventListenerSet()
{
  // Initialize some fields.
  this._eventListenerList = {};
}

// Set constructor.
DOMEventListenerSet.prototype.constructor = DOMEventListenerSet;

// Define the class.
DOMEventListenerSet.prototype = {
  //
  // DOM event listener set fields.
  //
  //   _eventListenerList       List of event listeners.
  //   _nextEventListenerID     Next event listener ID.
  //

  _eventListenerList: null,
  _nextEventListenerID: 0,


  /**
   * Add an event listener for the element specified by aElement with the
   * parameters specified by aType, aListener, and aUseCapture.  If aOneShot is
   * true, remove listener after it's called once.  Return an ID that may be
   * used to reference the added listener.
   *
   * \param aElement            Element for which to add an event listener.
   * \param aType               Type of event for which to listen.
   * \param aListener           Listener function.
   * \param aUseCapture         True if event capture should be used.
   * \param aOneShot            True if listener is a one-shot listener.
   *
   * \return                    Event listener ID.
   */

  add: function DOMEventListenerSet_add(aElement,
                                        aType,
                                        aListener,
                                        aUseCapture,
                                        aOneShot) {
    // Create the event listener object.
    var eventListener = {};
    eventListener.id = this._nextEventListenerID++;
    eventListener.element = aElement;
    eventListener.type = aType;
    eventListener.listener = aListener;
    eventListener.useCapture = aUseCapture;
    eventListener.oneShot = aOneShot;

    // Use one-shot function if listener is a one-shot listener.
    var listenerFunc = eventListener.listener;
    if (eventListener.oneShot) {
      var _this = this;
      listenerFunc =
        function(aEvent) { return _this._doOneShot(aEvent, eventListener); };
    }
    eventListener.addedListener = listenerFunc;

    // Add the event listener.
    eventListener.element.addEventListener(eventListener.type,
                                           eventListener.addedListener,
                                           eventListener.useCapture);
    this._eventListenerList[eventListener.id] = eventListener;

    return (eventListener.id);
  },


  /**
   * Remove the event listener specified by aEventListenerID.
   *
   * \param aEventListenerID    ID of event listener to remove.
   */

  remove: function DOMEventListenerSet_remove(aEventListenerID) {
    // Get the event listener.  Do nothing if not found.
    var eventListener = this._eventListenerList[aEventListenerID];
    if (!eventListener)
      return;

    // Remove the event listener.
    eventListener.element.removeEventListener(eventListener.type,
                                              eventListener.addedListener,
                                              eventListener.useCapture);
    delete this._eventListenerList[aEventListenerID];
  },


  /**
   * Remove all event listeners.
   */

  removeAll: function DOMEventListenerSet_removeAll() {
    // Remove all event listeners.
    for (var id in this._eventListenerList) {
      this.remove(id);
    }
    this._eventListenerList = {};
  },


  //----------------------------------------------------------------------------
  //
  // Internal DOM event listener set services.
  //
  //----------------------------------------------------------------------------

  /**
   * Dispatch the event specified by aEvent to the one-shot listener specified
   * by aEventListener and remove the listener.
   *
   * \param aEvent              Event to dispatch.
   * \param aEventListener      Event listener to which to dispatch event.
   */

  _doOneShot: function DOMEventListenerSet__doOneShot(aEvent, aEventListener) {
    // Remove event listener if one-shot.
    if (aEventListener.oneShot)
      this.remove(aEventListener.id);

    // Dispatch event to listener.
    return aEventListener.listener(aEvent);
  }
};

