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
 * \file  SBUtils.jsm
 * \brief Javascript source for the general Songbird utilities.
 */

//------------------------------------------------------------------------------
//
// Songbird utilities configuration.
//
//------------------------------------------------------------------------------

EXPORTED_SYMBOLS = [ "SBUtils" ];


//------------------------------------------------------------------------------
//
// Songbird utilities defs.
//
//------------------------------------------------------------------------------

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;


//------------------------------------------------------------------------------
//
// Songbird utility services.
//
//------------------------------------------------------------------------------

var SBUtils = {
  /**
   * Return true if the first-run has completed.
   *
   * \return                    True if first-run has completed.
   */

  hasFirstRunCompleted: function SBUtils_hasFirstRunCompleted() {
    // Read the first-run has completed preference.
    var Application = Cc["@mozilla.org/fuel/application;1"]
                        .getService(Ci.fuelIApplication);
    var hasCompleted = Application.prefs.getValue("songbird.firstrun.check.0.3",
                                                  false);

    return hasCompleted;
  }
};


