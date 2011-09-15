/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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
                     "SBBrandedString",
                     "SBFormattedString",
                     "SBBrandedFormattedString",
                     "SBFormattedCountString",
                     "SBStringBrandShortName",
                     "SBStringGetDefaultBundle",
                     "SBStringGetBrandBundle",
                     "SBStringBundle",
                     "StringSet" ];


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
var gSBStringBrandBundle = null;


//------------------------------------------------------------------------------
//
// String utility localization services.
//
//------------------------------------------------------------------------------

/**
 *   Get and return the localized string with the key specified by aKey using
 * the string bundle specified by aStringBundle.  If the string cannot be found,
 * return the default string specified by aDefault; if aDefault is not
 * specified or is null, return aKey.
 *   If aStringBundle is not specified, use the main Songbird string bundle.
 *
 * \param aKey                  Localized string key.
 * \param aDefault              Optional default string.
 * \param aStringBundle         Optional string bundle.
 *
 * \return                      Localized string.
 */

function SBString(aKey, aDefault, aStringBundle) {
  // Use default Songbird string bundle utility object if no bundle specified.
  if (!aStringBundle)
    return SBStringGetDefaultBundle().get(aKey, aDefault);

  // Set the default value.
  var value = aKey;
  if ((typeof(aDefault) != "undefined") && (aDefault !== null))
    value = aDefault;

  // Try getting the string from the bundle.
  try {
    value = aStringBundle.GetStringFromName(aKey);
  } catch(ex) {}

  return value;
}


/**
 *   Get and return the localized, branded string with the bundle key specified
 * by aKey using the string bundle specified by aStringBundle.  If the string
 * cannot be found, return the string specified by aDefault; if aDefault is not
 * specified, return aKey.
 *   If aStringBundle is not specified, use the main Songbird string bundle.
 *   The bundle string will be treated as a formatted string, and the first
 * parameter will be set to the brand short name string.
 *
 * \param aKey                  String bundle key.
 * \param aDefault              Default string value.
 * \param aStringBundle         Optional string bundle.
 *
 * \return                      Localized string.
 */

function SBBrandedString(aKey, aDefault, aStringBundle) {
  return SBFormattedString(aKey,
                           [ SBStringBrandShortName() ],
                           aDefault,
                           aStringBundle);
}


/**
 *   Get the formatted localized string with the key specified by aKey using the
 * format parameters specified by aParams and the string bundle specified by
 * aStringBundle.  If the string cannot be found, return the default string
 * specified by aDefault; if aDefault is not specified or is null, return aKey.
 *   If no string bundle is specified, get the string from the Songbird bundle.
 * If a string cannot be found, return aKey.
 *
 * \param aKey                  Localized string key.
 * \param aParams               Format params array.
 * \param aDefault              Optional default string.
 * \param aStringBundle         Optional string bundle.
 *
 * \return                      Localized string.
 */

function SBFormattedString(aKey, aParams, aDefault, aStringBundle) {
  // Use default Songbird string bundle utility object if no bundle specified.
  if (!aStringBundle)
    return SBStringGetDefaultBundle().format(aKey, aParams, aDefault);

  // Set the default value.
  var value = aKey;
  if ((typeof(aDefault) != "undefined") && (aDefault !== null))
    value = aDefault;

  // Try formatting string from bundle.
  try {
    value = aStringBundle.formatStringFromName(aKey, aParams, aParams.length);
  } catch(ex) {}

  return value;
}


/**
 *   Get the branded, formatted localized string with the key specified by aKey
 * using the format parameters specified by aParams and the string bundle
 * specified by aStringBundle.  If the string cannot be found, return the
 * default string specified by aDefault; if aDefault is not specified, return
 * aKey.
 *   If no string bundle is specified, get the string from the Songbird bundle.
 * If a string cannot be found, return aKey.
 *   The brand short name string will be appended to the formatted string
 * parameter list.
 *
 * \param aKey                  Localized string key.
 * \param aParams               Format params array.
 * \param aStringBundle         Optional string bundle.
 * \param aDefault              Optional default string.
 *
 * \return                      Localized string.
 */

function SBBrandedFormattedString(aKey, aParams, aDefault, aStringBundle) {
  return SBFormattedString(aKey,
                           aParams.concat(SBStringBrandShortName()),
                           aDefault,
                           aStringBundle);
}


/**
 *   Get and return the formatted localized count string with the key base
 * specified by aKeyBase using the count specified by aCount.  If the count is
 * one, get the string using the singular string key; otherwise, get the
 * formatted string using the plural string key and count.
 *   The singular string key is the key base with the suffix "_1".  The plural
 * string key is the key base with the suffix "_n".
 *   Use the format parameters specified by aParams.  If aParams is not
 * specified, use the count as the single format parameter.
 *   Use the string bundle specified by aStringBundle.  If the string bundle is
 * not specified, use the main Songbird string bundle.
 *   If the string cannot be found, return the default string specified by
 * aDefault; if aDefault is not specified or is null, return aKeyBase.
 *
 * \param aKeyBase              Localized string key base.
 * \param aCount                Count value for string.
 * \param aParams               Format params array.
 * \param aDefault              Optional default string.
 * \param aStringBundle         Optional string bundle.
 *
 * \return                      Localized string.
 */

function SBFormattedCountString(aKeyBase,
                                aCount,
                                aParams,
                                aDefault,
                                aStringBundle) {
  // Use default Songbird string bundle utility object if no bundle specified.
  if (!aStringBundle) {
    return SBStringGetDefaultBundle().formatCountString(aKeyBase,
                                                        aCount,
                                                        aParams,
                                                        aDefault);
  }

  // Get the format parameters.
  var params;
  if (aParams)
    params = aParams;
  else
    params = [ aCount ];

  // Produce the string key.
  var key = aKeyBase;
  if (aCount == 1)
    key += "_1";
  else
    key += "_n";

  // Set the default value.
  var value = aKeyBase;
  if ((typeof(aDefault) != "undefined") && (aDefault !== null))
    value = aDefault;

  // Try formatting the string from the bundle.
  try {
    value = aStringBundle.formatStringFromName(key, params, params.length);
  } catch(ex) {}

  return value;
}


/**
 * Return the Songbird brand short name localized string.
 *
 * \return Songbird brand short name.
 */

function SBStringBrandShortName() {
  return SBString("brandShortName", "Songbird", SBStringGetBrandBundle());
}


//------------------------------------------------------------------------------
//
// Internal string utility services.
//
//------------------------------------------------------------------------------

/**
 * Return the default Songbird string bundle utility object.
 *
 * \return Default Songbird string bundle utility object.
 */

function SBStringGetDefaultBundle() {
  if (!gSBStringDefaultBundle)
    gSBStringDefaultBundle = new SBStringBundle();

  return gSBStringDefaultBundle;
}


/**
 * Return the Songbird branding localized string bundle.
 *
 * \return Songbird branding localized string bundle.
 */

function SBStringGetBrandBundle() {
  if (!gSBStringBrandBundle) {
    gSBStringBrandBundle =
      Cc["@mozilla.org/intl/stringbundle;1"]
        .getService(Ci.nsIStringBundleService)
        .createBundle("chrome://branding/locale/brand.properties");
  }

  return gSBStringBrandBundle;
}


//------------------------------------------------------------------------------
//
// Songbird string bundle utility object.
//
//   The Songbird string bundle utility object provides an expanded set of
// string bundle services.  In particular, this object allows string bundles to
// include other string bundles and to include bundle strings in other bundle
// strings.
//   To include one or more string bundles in a top-level string bundle, define
// the string "include_bundle_list" in the top-level bundle.  This string should
// consist of a comma separated list of included string bundle URI's.  When
// the Songbird string bundle utility object looks up a string, it will look in
// the top-level string bundle and all included string bundles.  The included
// string bundles can include additional string bundles too.
//   To include a bundle string in another bundle string, encapsulate the
// included bundle string in "&" and ";", much like XML entities.  Use "&amp;"
// for a literal "&".
//
// Example:
//
// include_bundle_list=chrome://bundles1.properties,chrome://bundle2.properties
//
// string1=World
// string2=Hello &string1; &amp; Everyone Else
//
// string2 evaluates to "Hello World & Everyone Else".
//
//------------------------------------------------------------------------------

/**
 *   Construct a Songbird string bundle utility object using the base string
 * bundle specified by aBundle.  If aBundle is a string, it is treated as a
 * URI for a string bundle; otherwise, it is treated as a string bundle object.
 * If aBundle is not specified, the default Songbird string bundle is used.
 *
 *   ==> aBundle                Base string bundle.
 */

function SBStringBundle(aBundle)
{
  // Get the string bundle service. */
  this._stringBundleService = Cc["@mozilla.org/intl/stringbundle;1"]
                                .getService(Ci.nsIStringBundleService);

  // Use the default Songbird string bundle if none specified.
  if (!aBundle)
    aBundle = "chrome://songbird/locale/songbird.properties";

  // Initialize the bundle list.
  this._bundleList = [];

  // Load the string bundle.
  this._loadBundle(aBundle);
}

// Define the class.
SBStringBundle.prototype = {
  // Set the constructor.
  constructor: SBStringBundle,

  //
  // Songbird string bundle utility object fields.
  //
  //   _stringBundleService     String bundle service object.
  //   _bundleList              List of string bundles.
  //

  _stringBundleService: null,
  _bundleList: null,


  /**
   *   Get and return the string with the bundle key specified by aKey.  If the
   * string cannot be found, return the string specified by aDefault; if
   * aDefault is not specified or is null, return aKey.
   *
   * \param aKey                  String bundle key.
   * \param aDefault              Default string value.
   *
   * \return                      Bundle string.
   */

  get: function SBStringBundle_get(aKey, aDefault) {
    // Set the default string value.
    var value = aKey;
    if ((typeof(aDefault) != "undefined") && (aDefault !== null))
      value = aDefault;

    // Get the string from the bundle list.
    for (var i = 0; i < this._bundleList.length; i++) {
      var bundle = this._bundleList[i];
      try {
        value = bundle.GetStringFromName(aKey);
        break;
      } catch (ex) {}
    }

    // Apply string substitutions.
    value = this._applySubstitutions(value);

    return value;
  },


  /**
   *   Get and return the formatted string with the bundle key specified by aKey
   * and the parameters specified by aParams.  If the string cannot be found,
   * return the string specified by aDefault; if aDefault is not specified or is
   * null, return aKey.
   *
   * \param aKey                  String bundle key.
   * \param aParams               Bundle string format parameters.
   * \param aDefault              Default string value.
   *
   * \return                      Bundle string.
   */

  format: function SBStringBundle_format(aKey, aParams, aDefault) {
    // Set the default string value.
    var value = aKey;
    if ((typeof(aDefault) != "undefined") && (aDefault !== null))
      value = aDefault;

    // Get the string from the bundle list.
    for (var i = 0; i < this._bundleList.length; i++) {
      var bundle = this._bundleList[i];
      try {
        value = bundle.formatStringFromName(aKey, aParams, aParams.length);
        break;
      } catch (ex) {}
    }

    // Apply string substitutions.
    value = this._applySubstitutions(value);

    return value;
  },


  /**
   *   Get and return the formatted count string with the key base specified by
   * aKeyBase using the count specified by aCount.  If the count is one, get the
   * string using the singular string key; otherwise, get the formatted string
   * using the plural string key and count.
   *   The singular string key is the key base with the suffix "_1".  The plural
   * string key is the key base with the suffix "_n".
   *   Use the format parameters specified by aParams.  If aParams is not
   * specified, use the count as the single format parameter.
   *   If the string cannot be found, return the default string specified by
   * aDefault; if aDefault is not specified or is null, return aKeyBase.
   *
   * \param aKeyBase              String bundle key base.
   * \param aCount                Count value for string.
   * \param aParams               Format params array.
   * \param aDefault              Default string value.
   *
   * \return                      Bundle string.
   */

  formatCountString: function SBStringBundle_formatCountString(aKeyBase,
                                                               aCount,
                                                               aParams,
                                                               aDefault) {
    // Get the format parameters.
    var params;
    if (aParams)
      params = aParams;
    else
      params = [ aCount ];

    // Produce the string key.
    var key = aKeyBase;
    if (aCount == 1)
      key += "_1";
    else
      key += "_n";

    // Set the default string value.
    var value = aKeyBase;
    if ((typeof(aDefault) != "undefined") && (aDefault !== null))
      value = aDefault;

    // Get the string from the bundle list.
    for (var i = 0; i < this._bundleList.length; i++) {
      var bundle = this._bundleList[i];
      try {
        value = bundle.formatStringFromName(key, params, params.length);
        break;
      } catch (ex) {}
    }

    // Apply string substitutions.
    value = this._applySubstitutions(value);

    return value;
  },


  //----------------------------------------------------------------------------
  //
  // Internal Songbird string bundle utility object services.
  //
  //----------------------------------------------------------------------------

  /**
   *   Load the string bundle specified by aBundle and all of its included
   * bundles.  If aBundle is a string, it is treated as a URI for the string
   * bundle; otherwise, it is treated as a string bundle object.
   *
   *   ==> aBundle              Bundle to load.
   */

  _loadBundle: function SBStringBundle__loadBundle(aBundle) {
    var bundle = aBundle;

    // If the bundle is specified as a URI spec string, create a bundle from it.
    if (typeof(bundle) == "string")
      bundle = this._stringBundleService.createBundle(bundle);

    // Add the string bundle to the list of string bundles.
    this._bundleList.push(bundle);

    // Get the list of included string bundles.
    var includeBundleList;
    try {
      includeBundleList =
        bundle.GetStringFromName("include_bundle_list").split(",");
    } catch (ex) {
      includeBundleList = [];
    }

    // Load each of the included string bundles.
    for (var i = 0; i < includeBundleList.length; i++) {
        this._loadBundle(includeBundleList[i]);
    }
  },


  /*
   * Apply any string bundle substitutions to the string specified by aString.
   *
   * \param aString               String to which to apply substitutions.
   */

  _applySubstitutions: function SBStringBundle__applySubstitutions(aString) {
    // Set up a replacement function.
    var _this = this;
    var replaceFunc = function(aMatch, aKey) {
      if (aKey == "amp")
        return "&";
      return _this.get(aKey, "");
    }

    // Apply all string substitutions and return result.
    return aString.replace(/&([^&;]*);/g, replaceFunc);
  }
};


//------------------------------------------------------------------------------
//
// Songbird string set utility object.
//
//   A string set is a string containing a set of strings, separated by a
// delimiter (e.g., " ").  A string set does not contain duplicates.  A string
// set could be used, for example, with the class attribute of a DOM element.
//
//------------------------------------------------------------------------------

var StringSet = {
  /**
   * Add the string specified by aString to the string set specified by
   * aStringSet using the delimiter specified by aDelimiter.  If aDelimiter is
   * not specified, use " ".  Return resulting string set.
   *
   * \param aStringSet          String set to which to add string.
   * \param aString             String to add to string set.
   * \param aDelimiter          String set delimiter.  Defaults to " ".
   *
   * \return                    New string set.
   */

  add: function StringSet_add(aStringSet, aString, aDelimiter) {
    // Get the delimiter.
    var delimiter = aDelimiter;
    if (!delimiter)
      delimiter = " ";

    // Split the string set into an array and add string if it's not already
    // present.
    var stringSet = aStringSet.split(delimiter);
    if (stringSet.indexOf(aString) < 0)
      stringSet.push(aString);

    // Return new string set.
    return stringSet.join(delimiter);
  },


  /**
   * Remove the string specified by aString from the string set specified by
   * aStringSet using the delimiter specified by aDelimiter.  If aDelimiter is
   * not specified, use " ".  Return resulting string set.
   *
   * \param aStringSet          String set from which to remove string.
   * \param aString             String to remove from string set.
   * \param aDelimiter          String set delimiter.  Defaults to " ".
   *
   * \return                    New string set.
   */

  remove: function StringSet_remove(aStringSet, aString, aDelimiter) {
    // Get the delimiter.
    var delimiter = aDelimiter;
    if (!delimiter)
      delimiter = " ";

    // Split the string set into an array and remove all instances of the
    // string.
    var stringSet = aStringSet.split(delimiter);
    var stringCount = stringSet.length;
    for (var i = stringCount - 1; i >= 0; i--) {
      if (stringSet[i] == aString)
        stringSet.splice(i, 1);
    }

    // Return new string set.
    return stringSet.join(delimiter);
  },


  /**
   * Return true if the string specified by aString is contained in the string
   * set specified by aStringSet using the delimiter specified by aDelimiter.
   * If aDelimiter is not specified, use " ".
   *
   * \param aStringSet          String set to check.
   * \param aString             String for which to check.
   * \param aDelimiter          String set delimiter.  Defaults to " ".
   *
   * \return                    True if string set contains string.
   */

  contains: function StringSet_contains(aStringSet, aString, aDelimiter) {
    // Get the delimiter.
    var delimiter = aDelimiter;
    if (!delimiter)
      delimiter = " ";

    // Split the string set into an array and return whether the string set
    // contains the string.
    var stringSet = aStringSet.split(delimiter);
    return (stringSet.indexOf(aString) >= 0);
  }
};

