/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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
//  these data remotes should will function as globals across 
//  the application.  This is both powerful and dangerous, and
//  while this interface is available to all, everyone should
//  be very careful to properly namespace their data strings.
//
//  SBDataBindElementProperty Param List:
//   key  - The data ID to bind upon
//   elem - The element ID to be bound
//   attr - The name of the property or attribute to change
//   bool - Optionally assign the data as BOOLEAN data (true/false props, "true"/"false" attrs)
//   not  - Optionally assign the data as a boolean NOT of the value
//   eval - Optionally apply an eval string where `value = eval( eval_string );`

//
// XXXredfive - clean up function names after getting them working.
//

/**
 *
 */
function SBDataBindElementProperty( aKey, aElement, aProperty, aIsBool, aIsNot, aEvalString )
{
  var retval = null;
  try {
    var obj = ( typeof(aElement) == "object" ) ?  aElement : document.getElementById( aElement );
    if ( obj ) {
      retval = SB_NewDataRemote( aKey, null );
      retval.bindProperty( obj, aProperty, aIsBool, aIsNot, aEvalString );
    }
    else {
      Components.utils.reportError( "Can't find " + aElement );
    }
  }
  catch ( err ) {
    Components.utils.reportError( err );
  }
  return retval;  
}

/**
 *
 */
function SBDataBindElementAttribute( aKey, aElement, aAttribute, aIsBool, aIsNot, aEvalString )
{
  var retval = null;
  try {
    var obj = ( typeof(aElement) == "object" ) ?  aElement : document.getElementById( aElement );
    if ( obj ) {
      retval = SB_NewDataRemote( aKey, null );
      retval.bindAttribute( obj, aAttribute, aIsBool, aIsNot, aEvalString );
    }
    else {
      Components.utils.reportError( "Can't find " + aElement );
    }
  }
  catch ( err ) {
    Components.utils.reportError( "ERROR! Binding attribute did not succeed.\n" + err + "\n" );
  }
  return retval;  
}

// Note:
// Some XBL contexts do not create an object properly outside this script.
/**
 * XXXredfive - remove me
//function SBDataGetValue( aKey )
{
  var data = SB_NewDataRemote( aKey, null );
  var ret = data.stringValue;
  data.unbind();
  return ret;
}
 */

/**
 *
 */
function SBDataGetStringValue( aKey )
{
  //dump("XXXredfive - SBDataGetStringValue(" + aKey + ")");
 try {
  var data = SB_NewDataRemote( aKey, null );
  var ret = data.stringValue;
  data.unbind();
 } catch (err) {
   dump("SBDataGetStringValue() ERROR:\n" + err + "\n");
 }
  //dump( "      stringValue is:" + ret + "\n");
  return ret;
}

/**
 *
 */
function SBDataGetIntValue( aKey )
{
  //dump("XXXredfive - SBDataGetIntValue(" + aKey + ")");
 try {
  var data = SB_NewDataRemote( aKey, null );
  var ret = data.intValue;
  data.unbind();
 } catch (err) {
   dump("SBDataGetIntValue() ERROR:\n" + err + "\n");
 }
  //dump( ":" + ret + "\n");
  return ret;
}

/**
 *
 */
function SBDataGetBoolValue( aKey )
{
  //dump("XXXredfive - SBDataGetBoolValue(" + aKey + ")");
 try {
  var data = SB_NewDataRemote( aKey, null );
  var ret = data.boolValue;
  data.unbind();
 } catch (err) {
   dump("SBDataGetBoolValue() ERROR:\n" + err + "\n");
 }
  //dump( ":" + ret + "\n");
  return ret;
}

/**
 * XXXredfive - remove me
//function SBDataSetValue( aKey, value )
{
  var data = SB_NewDataRemote( aKey, null );
  var ret = data.stringValue = value;
  data.unbind();
  return ret;
}
 */

/**
 *
 */
function SBDataSetStringValue( aKey, aStringValue )
{
  var data = SB_NewDataRemote( aKey, null );
  var ret = data.stringValue = aStringValue;
  data.unbind();
  return ret;
}

/**
 *
 */
function SBDataSetBoolValue( aKey, aBoolValue )
{
  var data = SB_NewDataRemote( aKey, null );
  var ret = data.boolValue = aBoolValue;
  data.unbind();
  return ret;
}

/**
 *
 */
function SBDataSetIntValue( aKey, aIntValue )
{
  var data = SB_NewDataRemote( aKey, null );
  var ret = data.intValue = aIntValue;
  data.unbind();
  return ret;
}

/**
 *
 */
function SBDataFireEvent( aKey )
{
  var data = SB_NewDataRemote( aKey, null );
  var ret = data.intValue += 1;
  data.unbind();
  return ret;
}
