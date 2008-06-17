/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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

/**
* \file  firstRunWizard.js
* \brief Javascript source for the first-run wizard dialog.
*/

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// First-run wizard dialog.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// First-run wizard dialog services.
//
//------------------------------------------------------------------------------

var firstRunWizard = {
  //
  // First-run wizard object fields.
  //
  //   _currentPanel            Current wizard panel.
  //   _panelCount              Number of wizard panels.
  //

  _currentPanel: 0,
  _panelCount: 0,


  //----------------------------------------------------------------------------
  //
  // Event handling services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle a window load event.
   */

  doLoad: function firstRunWizard_doLoad() {
    // Get the number of wizard panels.
    var wizardDeckElem = document.getElementById("first_run_wizard_deck");
    this._panelCount = parseInt(wizardDeckElem.getAttribute("panelcount"));

    // Update the first-run wizard UI.
    this._update();
  },


  /**
   * Handle a window unload event.
   */

  doUnload: function firstRunWizard_doUnload() {
    // Indicate that the wizard is complete and should not be restarted.
    window.arguments[0].onComplete(false);
  },


  /**
   * Handle the command event specified by aEvent.
   *
   * \param aEvent              Event to handle.
   */

  doCommand: function firstRunWizard_doCommand(aEvent) {
    // Process the command.
    var commandID = aEvent.target.id
    switch (commandID) {
      case "cmd_back" :
        if (this._currentPanel > 0)
          this._currentPanel--;
        break;

      case "cmd_continue" :
        if (this._currentPanel < (this._panelCount - 1))
          this._currentPanel++;
        break;

      case "cmd_finish" :
        onExit();
        break;

      case "cmd_cancel" :
        onExit();
        break;

      default :
        break;
    }

    // Update the UI.
    this._update();
  },


  //----------------------------------------------------------------------------
  //
  // Internal first-run wizard services.
  //
  //----------------------------------------------------------------------------

  /**
   * Update the first-run wizard UI.
   */

  _update: function firstRunWizard__update() {
    // Hide back command on EULA and panel after EULA.
    var backCommandElem = document.getElementById("cmd_back");
    if (this._currentPanel < 2)
      backCommandElem.setAttribute("hidden", "true");
    else
      backCommandElem.removeAttribute("hidden");

    // Hide cancel command on EULA panel.
    var cancelCommandElem = document.getElementById("cmd_cancel");
    if (this._currentPanel < 1)
      cancelCommandElem.setAttribute("hidden", "true");
    else
      cancelCommandElem.removeAttribute("hidden");

    // Hide finish command on all but the last panel.
    var finishCommandElem = document.getElementById("cmd_finish");
    if (this._currentPanel < (this._panelCount - 1))
      finishCommandElem.setAttribute("hidden", "true");
    else
      finishCommandElem.removeAttribute("hidden");

    // Hide continue command on the last panel.
    var continueCommandElem = document.getElementById("cmd_continue");
    if (this._currentPanel == (this._panelCount - 1))
      continueCommandElem.setAttribute("hidden", "true");
    else
      continueCommandElem.removeAttribute("hidden");

    // Update the wizard deck element selected index.
    var wizardDeckElem = document.getElementById("first_run_wizard_deck");
    wizardDeckElem.setAttribute("selectedIndex", this._currentPanel);
  }
};

