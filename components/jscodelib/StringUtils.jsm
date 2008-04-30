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
 * \file  StringUtils.jsm
 * \brief Javascript source for the string utility services.
 */

//------------------------------------------------------------------------------
//
// String utility JSM configuration.
//
//------------------------------------------------------------------------------

EXPORTED_SYMBOLS = [ "SBString",
                     "SBFormattedString",
                     "SBFormattedCountString" ];


//------------------------------------------------------------------------------
//
// String utility defs.
//
//------------------------------------------------------------------------------

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results


//------------------------------------------------------------------------------
//
// String utility globals.
//
//------------------------------------------------------------------------------

var gSBStringDefaultBundle = null;


//------------------------------------------------------------------------------
//
// String utility localization services.
//
//------------------------------------------------------------------------------

/**
 *   Get and return the localized string with the key specified by aKey using
 * the string bundle specified by aStringBundle.  If the string cannot be found,
 * return the default string specified by aDefault; if aDefault is not
 * specified, return aKey.
 *   If aStringBundle is not specified, use the main Songbird string bundle.
 *
 * \param aKey                  Localized string key.
 * \param aDefault              Optional default string.
 * \param aStringBundle         Optional string bundle.
 *
 * \return                      Localized string.
 */

function SBString(aKey, aDefault, aStringBundle) {
  // Get the string bundle.
  var stringBundle = aStringBundle ? aStringBundle : SBStringGetDefaultBundle();

  // Set the default value.
  var value = aDefault ? aDefault : aKey;

  // Try getting the string from the bundle.
  try {
    value = stringBundle.GetStringFromName(aKey);
  } catch(ex) {}

  return value;
}


/**
 *   Get the formatted localized string with the key specified by aKey using the
 * format parameters specified by aParams and the string bundle specified by
 * aStringBundle.
 *   If no string bundle is specified, get the string from the Songbird bundle.
 * If a string cannot be found, return aKey.
 *
 * \param aKey                  Localized string key.
 * \param aParams               Format params array.
 * \param aStringBundle         Optional string bundle.
 *
 * \return                      Localized string.
 */

function SBFormattedString(aKey, aParams, aStringBundle) {
  // Get the string bundle.
  var stringBundle = aStringBundle ? aStringBundle : SBStringGetDefaultBundle();

  // Set the default value.
  var value = aKey;

  // Try formatting string from bundle.
  try {
    value = stringBundle.formatStringFromName(aKey, aParams, aParams.length);
  } catch(ex) {}

  return value;
}


/**
 *   Get and return the formatted localized count string with the key base
 * specified by aKeyBase using the count specified by aCount.  If the count is
 * one, get the string using the singular string key; otherwise, get the
 * formatted string using the plural string key and count.
 *   The singular string key is the key base with the suffix "_1".  The plural
 * string key is the key base with the suffix "_n".
 *   Use the string bundle specified by aStringBundle.  If the string bundle is
 * not specified, use the main Songbird string bundle.
 *   If the string cannot be found, return the default string specified by
 * aDefault; if aDefault is not specified, return aKey.
 *
 * \param aKeyBase              Localized string key base.
 * \param aCount                Count value for string.
 * \param aDefault              Optional default string.
 * \param aStringBundle         Optional string bundle.
 *
 * \return                      Localized string.
 */

function SBFormattedCountString(aKeyBase, aCount, aDefault, aStringBundle) {
  // Get the string bundle.
  var stringBundle = aStringBundle ? aStringBundle : SBStringGetDefaultBundle();

  // Produce the string key.
  var key = aKeyBase;
  if (aCount == 1)
    key += "_1";
  else
    key += "_n";

  // Set the default value.
  var value = aDefault ? aDefault : aKeyBase;

  // Try formatting the string from the bundle.
  try {
    value = stringBundle.formatStringFromName(key, [ aCount ], 1);
  } catch(ex) {}

  return value;
}


//------------------------------------------------------------------------------
//
// Internal string utility services.
//
//------------------------------------------------------------------------------

/**
 * Return the default Songbird localized string bundle.
 *
 * \return Default Songbird localized string bundle.
 */

function SBStringGetDefaultBundle() {
  if (!gSBStringDefaultBundle) {
    gSBStringDefaultBundle =
      Cc["@mozilla.org/intl/stringbundle;1"]
        .getService(Ci.nsIStringBundleService)
        .createBundle("chrome://songbird/locale/songbird.properties");
  }

  return gSBStringDefaultBundle;
}

