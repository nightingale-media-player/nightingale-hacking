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
 * \file  importLibrary.js
 * \brief Javascript source for the import library dialog.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Import library dialog.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Import library dialog defs.
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


//------------------------------------------------------------------------------
//
// Import library dialog services.
//
//------------------------------------------------------------------------------

var importLibrary = {
  //----------------------------------------------------------------------------
  //
  // Event handling services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle a load event.
   */

  doLoad: function importLibrary_doLoad() {
    // Set the user query text.
    //XXXeps don't hardcode iTunes
    //XXXeps set query based on dialog params
    var queryDescElem = document.getElementById("import_library_query");
    var userQuery = SBFormattedString
                      ("import_library.dialog_query.manual_first_import",
                       [ "iTunes" ]);
    var userQueryNode = document.createTextNode(userQuery);
    queryDescElem.appendChild(userQueryNode);
  }
}

