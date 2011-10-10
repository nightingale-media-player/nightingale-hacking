/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.getnightingale.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END NIGHTINGALE GPL
 */

/**
 * \file  SBDataRemoteUtils.jsm
 * \brief Javascript source for the Nightingale data remote utility services.
 */

//------------------------------------------------------------------------------
//
// Nightingale data remote utility JSM configuration.
//
//------------------------------------------------------------------------------

EXPORTED_SYMBOLS =
[
  "SBNewDataRemote",
  "SBDataGetStringValue",
  "SBDataGetIntValue",
  "SBDataGetBoolValue",
  "SBDataSetStringValue",
  "SBDataSetBoolValue",
  "SBDataSetIntValue",
  "SBDataIncrementValue",
  "SBDataDecrementValue",
  "SBDataToggleBoolValue",
  "SBDataFireEvent",
  "SBDataDeleteBranch"
];


//------------------------------------------------------------------------------
//
// Nightingale data remote utility defs.
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
// sbIDataRemote wrapper
//
//  This object provides the ability to set key-value pairs
//  into a global "data store," and to have various callback
//  effects occur when anyone changes an observed value.
//
//  The callback binding can be placed on a dom element to
//  change its properties or attributes based upon the value
//  (either directly, or as a boolean, and/or as the result
//  of a given evaluation expression).
//
//  The mozilla preferences system is used as the underlying
//  data storage layer to power this interface.  Because the
//  preferences are available to all open windows in xulrunner,
//  these data remotes should function as globals across
//  the application.  This is both powerful and dangerous, and
//  while this interface is available to all, everyone should
//  be very careful to properly namespace their data strings.
//
//  SBDataBindElementProperty Param List:
//   key  - The data ID to bind upon
//   elem - The element ID to be bound
//   attr - The name of the property or attribute to change
//   bool - Optionally assign the data as BOOLEAN data (true/false props,
//          "true"/"false" attrs)
//   not  - Optionally assign the data as a boolean NOT of the value
//   eval - Optionally apply an eval string where `value = eval( eval_string );`
//XXXeps add data remote DOM utilities.
//
//------------------------------------------------------------------------------

/**
 * \brief Create a new data remote object.
 *
 * \param aKey - The string identifier for the data to watch
 * \param aRoot - OPTIONAL - If present this defines a prefix to the key
 *
 * \return - A data remote object.
 * \internal
 * \note This function is not exported.
 */

function SB_NewDataRemote( aKey, aRoot )
{
  var dataRemote = Cc["@getnightingale.com/Nightingale/DataRemote;1"]
                     .createInstance(Ci.sbIDataRemote);
  dataRemote.init( aKey, aRoot );
  return dataRemote;
}

/**
 * \brief Create a new data remote object
 * \param aKey - The string identifier for the data to watch
 * \param aRoot - OPTIONAL - If present this defines a prefix to the key
 *
 * \return - A data remote object.
 */
function SBNewDataRemote( aKey, aRoot )
{
  return SB_NewDataRemote( aKey, aRoot );
}

/**
 * \brief Get the value of the data in string format.
 *
 * \param aKey - The string identifier for the data to be changed.
 * \return - The string value of the data. If the data has not been set the
 *           return value will be the empty string ("");
 * \sa DatatRemote
 */
function SBDataGetStringValue( aKey )
{
  var data = SB_NewDataRemote( aKey, null );
  return data.stringValue;
}

/**
 * \brief Get the value of the data in integer format
 *
 * \param aKey - The string identifier for the data to be changed.
 * \return - The integer value of the data. If the data has not been set or
 *           the data is not convertible to an integer the return value will
 *           be NaN.
 * \sa DatatRemote
 */
function SBDataGetIntValue( aKey )
{
  var data = SB_NewDataRemote( aKey, null );
  return data.intValue;
}

/**
 * \brief Get the value of the data in boolean format.
 *
 * \param aKey - The string identifier for the data to be retrieved.
 * \return - The boolean value of the data.
 * \sa DatatRemote
 */
function SBDataGetBoolValue( aKey )
{
  var data = SB_NewDataRemote( aKey, null );
  return data.boolValue;
}

/**
 * \brief Set a string value.
 * Changes the value of the data remote to the boolean passed in, regardless
 *   of its value before.
 *
 * \param aKey - The string identifier for the data to be changed.
 * \param aBoolValue - A boolean value.
 * \return - The new value of the data.
 * \sa DatatRemote
 */
function SBDataSetStringValue( aKey, aStringValue )
{
  var data = SB_NewDataRemote( aKey, null );
  data.stringValue = aStringValue;
  return aStringValue;
}

/**
 * \brief Set a boolean value.
 * Changes the value of the data remote to the boolean passed in, regardless
 *   of its value before.
 *
 * \param aKey - The string identifier for the data to be changed.
 * \param aBoolValue - A boolean value.
 * \return - The new value of the data.
 * \sa DatatRemote
 */
function SBDataSetBoolValue( aKey, aBoolValue )
{
  var data = SB_NewDataRemote( aKey, null );
  data.boolValue = aBoolValue;
  return aBoolValue;
}

/**
 * \brief Set an integer value.
 * Changes the value of the data remote to the integer passed in, regardless
 *   of its value before.
 *
 * \param aKey - The string identifier for the data to be changed.
 * \param aIntValue - An integer (or string convertable to an integer) value.
 * \return - The new value of the data.
 * \sa DatatRemote
 */
function SBDataSetIntValue( aKey, aIntValue )
{
  var data = SB_NewDataRemote( aKey, null );
  data.intValue = aIntValue;
  return aIntValue;
}

/**
 * \brief Increment the integer value.
 *  Increment the integer value associated with the key passed in. If a ceiling
 *    value is passed in the new value will be no greater than the ceiling.
 *
 * \param aKey - The string identifier for the data to be changed.
 * \param aCeiling - Optional, if specified the data will be at most, this value
 * \return - The new value of the data.
 * \sa DatatRemote
 */
function SBDataIncrementValue( aKey, aCeiling )
{
  // if no ceiling is given use the *ceiling*
  if ( aCeiling == null || isNaN(aCeiling) )
    aCeiling = Number.MAX_VALUE;

  var data = SB_NewDataRemote( aKey, null );
  var newVal = (data.intValue + 1);  // getter call
  if ( newVal > aCeiling )
    newVal = aCeiling;
  data.intValue = newVal;            // setter call
  return newVal;
}

/**
 * \brief Decrement the integer value.
 *  Decrement the integer value associated with the key passed in. If a floor
 *    value is passed in the new value will be no less than the floor.
 *
 * \param aKey - The string identifier for the data to be changed.
 * \param aFloor - Optional, if specified the data will be no less than this
 *                 value
 * \return - The new value of the data.
 * \sa DatatRemote
 */
function SBDataDecrementValue( aKey, aFloor )
{
  // if no floor is given, use the *floor*
  if ( aFloor == null || isNaN(aFloor) )
    aFloor = -Number.MAX_VALUE;

  var data = SB_NewDataRemote(aKey, null);
  var newVal = (data.intValue - 1);  // getter call
  if ( newVal < aFloor )
    newVal = aFloor;
  data.intValue = newVal;            // setter call
  return newVal;
}

/**
 * \brief Change the boolean value.
 * The true/false value of the data associated with the key will be reversed.
 *
 * \param aKey - The string identifier for the data to be changed.
 * \return - The new value of the data.
 * \sa DatatRemote
 */
function SBDataToggleBoolValue( aKey )
{
  var data = SB_NewDataRemote(aKey, null);
  var newVal = !data.boolValue;
  data.boolValue = newVal;
  return newVal;
}

/**
 * \brief Cause a notification to be fired.
 * The data associated with the key will be modified so that observers will
 *   be called about the change. The actual value of the data should not be
 *   counted on.
 *
 * \param aKey - The data about which the event is being fired
 * \param aKey - The string identifier for the data about which the event is
 *               being fired.
 * \return - The new value of the data.
 * \sa DatatRemote
 */
function SBDataFireEvent( aKey )
{
  var data = SB_NewDataRemote( aKey, null );
  return ++data.intValue;
}

/**
 * \brief Called to remove the data remote specified by aKey and all its
 *        children.
 *
 * \param aKey - The data remove to remove.
 */
function SBDataDeleteBranch( aKey )
{
  var data = SB_NewDataRemote( aKey, null );
  data.deleteBranch();
}

