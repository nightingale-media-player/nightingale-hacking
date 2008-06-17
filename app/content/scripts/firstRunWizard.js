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
  //----------------------------------------------------------------------------
  //
  // Event handling services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle a wizard finish event.
   */

  doFinish: function firstRunWizard_doFinish() {
    // Indicate that the wizard is complete and should not be restarted.
    window.arguments[0].onComplete(false);
  },


  /**
   * Handle a wizard cancel event.
   */

  doCancel: function firstRunWizard_doCancel() {
    // Indicate that the wizard is complete and should not be restarted.
    window.arguments[0].onComplete(false);
  },


  /**
   * Handle a wizard page show event.
   */

  doPageShow: function firstRunWizard_doPageShow() {
    // Get the current wizard page.
    var wizardElem = document.getElementById("first_run_wizard");
    var currentPage = wizardElem.currentPage;

    // Hide or show the back button.
    var backButton = wizardElem.getButton("back");
    if (currentPage.getAttribute("hideback") == "true") {
      backButton.setAttribute("hidden", "true");
    } else {
      backButton.removeAttribute("hidden");
    }

    // Hide or show the cancel button.
    var cancelButton = wizardElem.getButton("cancel");
    if (currentPage.getAttribute("hidecancel") == "true") {
      cancelButton.setAttribute("hidden", "true");
    } else {
      cancelButton.removeAttribute("hidden");
    }
  }
};

