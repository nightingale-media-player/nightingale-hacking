/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

/**
 * \file  importMediaPrefs.js
 * \brief Javascript source for the import media preference pane.
 */

//------------------------------------------------------------------------------
//
// Import media preference pane services defs.
//
//------------------------------------------------------------------------------

// Component manager defs.
if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;

// DOM defs.
if (typeof(XUL_NS) == "undefined")
  var XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Import media preference pane services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

var importMediaPrefsPane = {
  //----------------------------------------------------------------------------
  //
  // Event handling services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle the pane load event specified by aEvent.
   *
   * \param aEvent              Pane load event.
   */

  doPaneLoad: function importMediaPrefs_doPaneLoad(aEvent) {
    // Forward the event to all import media preference tab panels.
    var tabPanelsElem = document.getElementById("import_media_tabpanels");
    var tabPanelList = tabPanelsElem.getElementsByTagNameNS(XUL_NS, "tabpanel");
    for (var i = 0; i < tabPanelList.length; i++) {
      var tabPanel = tabPanelList[i];
      this._fireEvent(tabPanel, aEvent.type, aEvent.bubbles, aEvent.cancelable);
    }
    var tabBox = document.getElementById("import_media_tabbox");
    if (tabBox.hasAttribute("selectedIndex")) {
      tabBox.selectedIndex = tabBox.getAttribute("selectedIndex");
    }
  },


  //----------------------------------------------------------------------------
  //
  // Internal services.
  //
  //----------------------------------------------------------------------------

  /**
   * Create and initialize a DOM event as specified by aEventType, aBubbles,
   * and aCancelable and dispatch it to the DOM event target specified by
   * aTarget.
   *
   * \param aTarget             Target of event.
   * \param aEventType          Type of event.
   * \param aBubbles            If true, event bubbles.
   * \param aCancelable         If true, event is cancelable.
   */

  _fireEvent: function importMediaPrefs__fireEvent(aTarget,
                                                   aEventType,
                                                   aBubbles,
                                                   aCancelable) {
    // Report any exceptions from firing event.
    try {
      // Create and initialized the event.
      var event = document.createEvent("Events");
      event.initEvent(aEventType, aBubbles, aCancelable);

      // Dispatch the event to the target.
      var cancel = !aTarget.dispatchEvent(event);

      // Dispatch the event to the target attribute event handler.
      var eventHandlerAttrName = "on" + aEventType;
      if (aTarget.hasAttribute(eventHandlerAttrName)) {
        var func = new Function("event",
                                aTarget.getAttribute(eventHandlerAttrName));
        var result = func.call(aTarget, event);
        if (result == false)
          cancel = true;
      }

      return !cancel;
    } catch (ex) {
      Cu.reportError(ex);
    }

    return false;
  }
}

