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
 * \file  PlatformUtils.jsm
 * \brief Javascript source for the platform utility services.
 */

//------------------------------------------------------------------------------
//
// Platform utility JSM configuration.
//
//------------------------------------------------------------------------------

EXPORTED_SYMBOLS = ["PlatformUtils"];


//------------------------------------------------------------------------------
//
// Platform utility imported services.
//
//------------------------------------------------------------------------------

Components.utils.import("resource://app/jsmodules/SBDataRemoteUtils.jsm");
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");


//------------------------------------------------------------------------------
//
// Platform utility defs.
//
//------------------------------------------------------------------------------

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results

var PlatformUtils = {
  get platformString() {
    var platform = Cc["@mozilla.org/xre/runtime;1"].
                       getService(Ci.nsIXULRuntime).
                       OS;
    switch(platform) {
      case "WINNT":
        platform = "Windows_NT";
      break;
    }
    
    return platform;
  }
};
