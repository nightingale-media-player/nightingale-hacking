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
* \brief Javascript source for the first run wizard dialog.
*/

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// First run wizard dialog.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// First run wizard dialog services.
//
//------------------------------------------------------------------------------

var firstRunWizard = {
  //----------------------------------------------------------------------------
  //
  // Event handling services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle a window unload event.
   */

  doUnload: function firstRunWizard_doUnload() {
    // Indicate that the wizard is complete and should not be restarted.
    window.arguments[0].onComplete(false);
  },


  /**
   * Handle the action event specified by aEvent.
   *
   * \param aEvent              Event to handle.
   */

  doAction: function firstRunWizard_doAction(aEvent) {
    // Get the number of panels and the currently selected panel.
    var deckElem = document.getElementById("first_run_wizard_deck");
    var panelCount = deckElem.getAttribute("panelcount");
    var selectedIndex = deckElem.selectedIndex;
    var newSelectedIndex = deckElem.selectedIndex;

    // Process the action.
    var action = aEvent.target.getAttribute("action");
    switch (action) {
      case "back" :
        if (newSelectedIndex > 0)
          newSelectedIndex--;
        break;

      case "continue" :
        if (newSelectedIndex < (panelCount - 1))
          newSelectedIndex++;
        break;

      case "cancel" :
        onExit();
        break;

      default :
        break;
    }

    // Update the selected index.
    if (newSelectedIndex != selectedIndex)
      deckElem.setAttribute("selectedIndex", newSelectedIndex);
  }
};

