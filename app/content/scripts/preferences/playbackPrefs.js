/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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

/**
 * \file  playbackPrefs.js
 * \brief Javascript source for the playback preferences UI.
 */

Components.utils.import("resource://app/jsmodules/DOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/SBJobUtils.jsm");
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");

if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;

var playbackPrefsUI = {
  _outputBufferMin: 100,
  _outputBufferMax: 10000,
  _outputBufferMaxDigits: 5,

  _streamingBufferMin: 32,
  _streamingBufferMax: 10240,
  _streamingBufferMaxDigits: 5,
  
  _outputBufferTextbox: null,
  _streamingBufferTextbox: null,

  initialize: function playbackPrefsUI_initialize() {
    this._outputBufferTextbox = document.getElementById("playback_output_buffer_textbox");
    this._streamingBufferTextbox = document.getElementById("playback_streaming_buffer_textbox");
    
    this._update();
  },
  
  doOnOutputBufferChange: function playbackPrefsUI_doOnOutputBufferChange() {
    var bufferVal = this._getPrefElemValue(this._getPrefElem("output_buffer_pref"));
    this._verifyIntValueBounds(this._outputBufferTextbox, 
                               bufferVal, 
                               this._outputBufferMin, 
                               this._outputBufferMax);
  },
  
  doOnOutputBufferKeypress: function playbackPrefsUI_doOnOutputBufferKeypress(aEvent) {
    this._verifyIntInputValue(aEvent,
                              this._outputBufferTextbox,
                              this._outputBufferMin, 
                              this._outputBufferMax, 
                              this._outputBufferMaxDigits);
  },
  
  doOnStreamingBufferChange: function playbackPrefsUI_doOnStreamingBufferChange() {
    var streamingVal = this._getPrefElemValue(this._getPrefElem("streaming_buffer_pref"));
    this._verifyIntValueBounds(this._streamingBufferTextbox,
                               streamingVal, 
                               this._streamingBufferMin,
                               this._streamingBufferMax);
  },
  
  doOnStreamingBufferKeypress: function playbackPrefsUI_doOnStreamingBufferKeypress(aEvent) {
    this._verifyIntInputValue(aEvent,
                              this._streamingBufferTextbox,
                              this._streamingBufferMin, 
                              this._streamingBufferMax, 
                              this._streamingBufferMaxDigits);
  },

  doOnNormalizationEnabledChange: 
  function playbackPrefsUI_doOnNormalizationEnabledChange() {
    var normalize = 
      !this._getPrefElemValue(this._getPrefElem("normalization_enabled_pref"));
    
    var normalizeList = document.getElementById("playback_normalization_preferred_gain_list");
    var normalizeLabel = document.getElementById("playback_normalization_preferred_gain_label");
    
    normalizeList.setAttribute("disabled", normalize);
    normalizeLabel.setAttribute("disabled", normalize);
  },
  
  _update: function playbackPrefsUI__update() {
    this.doOnOutputBufferChange();
    this.doOnNormalizationEnabledChange();
  },


  /**
   * Return the preference element with the preference ID specified by aPrefID.
   *
   * \param aPrefID             ID of preference element to get.
   *
   * \return                    Preference element.
   */

  _getPrefElem: function playback__getPrefElem(aPrefID) {
    // Get the requested element.
    var prefElemList = DOMUtils.getElementsByAttribute(document,
                                                       "prefid",
                                                       aPrefID);
    if (!prefElemList || (prefElemList.length == 0))
      return null;

    return prefElemList[0];
  },


  /**
   * Return the preference value of the element specified by aElement.
   *
   * \param aElement            Element for which to get preference value.
   *
   * \return                    Preference value of element.
   */

  _getPrefElemValue: function playbackPrefsUI__getPrefElemValue(aElement) {
    // Return the checked value for checkbox preferences.
    if (aElement.localName == "checkbox")
      return aElement.checked;

    return aElement.value;
  },
  
  _verifyIntValueBounds: function playbackPrefsUI__verifyIntValueBounds(aElement, 
                                                                        aValue,
                                                                        aMin,
                                                                        aMax) {
    if(aValue < aMin || 
       aValue > aMax) {
      aElement.setAttribute("invalid", true);
    }
    else if(aElement.hasAttribute("invalid")){
      aElement.removeAttribute("invalid");
    }
  },
  
  _verifyIntInputValue: function playbackPrefsUI__verifyIntInputValue(aEvent, 
                                                                      aElement,
                                                                      aMaxDigits) {
    if (!aEvent.ctrlKey && !aEvent.metaKey && !aEvent.altKey && aEvent.charCode) {
      if (aEvent.charCode < 48 || aEvent.charCode > 57) {
        aEvent.preventDefault();
      // If typing the key would make the value too long, prevent keypress
      } else if (aElement.value.length + 1 > aMaxDigits &&
                 aElement.selectionStart == aElement.selectionEnd) {
        aEvent.preventDefault();
      }
    }
    
    
  }
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Playback preference pane services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

var playbackPrefsPane = {
  doPaneLoad: function playbackPrefsPane_doPaneLoad() {
    playbackPrefsUI.initialize();
  }
}

