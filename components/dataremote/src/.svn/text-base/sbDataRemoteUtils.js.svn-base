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
//   bool - Optionally assign the data as BOOLEAN data (true/false props, "true"/"false" attrs)
//   not  - Optionally assign the data as a boolean NOT of the value
//   eval - Optionally apply an eval string where `value = eval( eval_string );`

Components.utils.import("resource://app/jsmodules/SBDataRemoteUtils.jsm");

const SB_NewDataRemote = new Components.Constructor("@songbirdnest.com/Songbird/DataRemote;1", "sbIDataRemote", "init");

/**
 * \brief Create a DataRemote and bind an Element's property to the data.
 * This method creates a DataRemote associated with the key passed in and
 *   binds the element's property to the data. Changes to the value of the
 *   data will automatically be reflected in the value of the property.
 *
 * \param aKey - The string identifier for the data to be changed.
 * \param aElement - A nsIDOMElement whose property is to be bound, or the
 *                   name of an element in the current document
 * \param aProperty - A property of aElement that will be bound.
 * \param aIsBool - Is the attribute (and therefore the data) a true/false value?
 * \param aIsNot - Should the attributes value be the opposite of the data.
 *                 Only used if the aIsBool is true.
 * \param aEvalString - A string of javascript to be executed to determine the
 *                      value of the property. It can use the variable "value"
 *                      in order to take action on the value of the data.
 * \return The newly created and bound DataRemote.
 * \sa DatatRemote
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
      Components.utils.reportError( "SBDataBindElementProperty: Can't find " + aElement );
    }
  }
  catch ( err ) {
    Components.utils.reportError( err );
  }
  return retval;  
}

/**
 * \brief Create a DataRemote and bind an Element's attribute to the data.
 * This method creates a DataRemote associated with the key passed in and
 *   binds the element's attribute to the data. Changes to the value of the
 *   data will automatically be reflected in the value of the attribute.
 *
 * \param aKey - The string identifier for the data to be changed.
 * \param aElement - A nsIDOMElement whose attribute is to be bound, or the
 *                   name of an element in the current document
 * \param aAttribute - An attribute of aElement that will be bound
 * \param aIsBool - Is the attribute (and therefore the data) a true/false value
 * \param aIsNot - Should the attributes value be the opposite of the data.
 *                 Only used if the aIsBool is true.
 * \param aEvalString - A string of javascript to be executed to determine the
 *                      value of the attribute. It can use the variable "value"
 *                      in order to take action on the value of the data.
 * \return The newly created and bound DataRemote.
 * \sa DatatRemote
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
      Components.utils.reportError( "SBDataBindElementAttribute: Can't find " + aElement );
    }
  }
  catch ( err ) {
    Components.utils.reportError( "ERROR! Binding attribute did not succeed.\n" + err + "\n" );
  }
  return retval;  
}

