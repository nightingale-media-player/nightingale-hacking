/*
 * CDDL HEADER START
 *       
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 * 
 * You can obtain a copy of the license in LICENSE.TXT or at
 * http://www.opensource.org/licenses/cddl1.php
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at LICENSE.TXT
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

var Cc = Components.classes;
var Ci = Components.interfaces;

const shoutcastCheckBitRate = "extensions.shoutcast-radio.limit-bit-rate";
const shoutcastCheckListeners = "extensions.shoutcast-radio.limit-listeners";

var radioPrefs = {
  openPreferences: function() {
    var windowMediator;
        var window;

        windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"].
                            getService(Ci.nsIWindowMediator);
        window = windowMediator.getMostRecentWindow("Songbird:Main");

        /* Open the Radio preferences pane. */
        window.SBOpenPreferences("radioPrefsPane");
  },

  handleLoad: function() {
  },

  updateStates: function() {
    var cBitRateBox = document.getElementById("checkBitRatePref");
    var bitRateMenu = document.getElementById("bitRateMenu");
    var listenersText = document.getElementById("textListenersPref");
    var cListenersBox = document.getElementById("checkListenersPref");
    Application.prefs.setValue(shoutcastCheckBitRate, cBitRateBox.checked);
    Application.prefs.setValue(shoutcastCheckListeners,
        cListenersBox.checked);
    if (cBitRateBox.checked)
      bitRateMenu.disabled = false;
    else
      bitRateMenu.disabled = true;
    if (cListenersBox.checked)
      listenersText.disabled = false;
    else
      listenersText.disabled = true;

  }
}


