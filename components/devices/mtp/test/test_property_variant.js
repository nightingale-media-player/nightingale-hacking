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

function runTest () 
{
  var variant = Components.classes["@songbirdnest.com/Songbird/PropertyVariant;1"]
                          .createInstance(Components.interfaces.nsIWritableVariant);

  assertTrue(variant, "Failed to create property variant instance");

  variant.setAsInt8(42);
  assertTrue(variant.QueryInterface(Components.interfaces.nsIVariant) == "42", "Int8 failed returned " + variant.QueryInterface(Components.interfaces.nsIVariant));
  variant.setAsInt16(4200);
  assertTrue(variant.QueryInterface(Components.interfaces.nsIVariant) == "4200", "Int16 failed returned " + variant.QueryInterface(Components.interfaces.nsIVariant));
  variant.setAsInt32(420000);
  assertTrue(variant.QueryInterface(Components.interfaces.nsIVariant) == "420000", "Int32 failed returned " + variant.QueryInterface(Components.interfaces.nsIVariant));
  variant.setAsInt64(420000000);
  assertTrue(variant.QueryInterface(Components.interfaces.nsIVariant) == "420000000", "Int64 failed returned " + variant.QueryInterface(Components.interfaces.nsIVariant));
  variant.setAsUint8(200);
  assertTrue(variant.QueryInterface(Components.interfaces.nsIVariant) == "200", "Uint8 failed returned " + variant.QueryInterface(Components.interfaces.nsIVariant));
  variant.setAsUint16(42000);
  assertTrue(variant.QueryInterface(Components.interfaces.nsIVariant) == "42000", "Uint16 failed returned " + variant.QueryInterface(Components.interfaces.nsIVariant));
  variant.setAsUint32(420000);
  assertTrue(variant.QueryInterface(Components.interfaces.nsIVariant) == "420000", "Uint32 failed returned " + variant.QueryInterface(Components.interfaces.nsIVariant));
  variant.setAsUint64(420000000);
  assertTrue(variant.QueryInterface(Components.interfaces.nsIVariant) == "420000000", "Uint8 failed returned " + variant.QueryInterface(Components.interfaces.nsIVariant));
  variant.setAsFloat(42.0);
  assertTrue(variant.QueryInterface(Components.interfaces.nsIVariant) == "42", "Float failed returned " + variant.QueryInterface(Components.interfaces.nsIVariant));
  variant.setAsDouble(420000000.42);
  assertTrue(variant.QueryInterface(Components.interfaces.nsIVariant) == "420000000.42", "Double failed returned " + variant.QueryInterface(Components.interfaces.nsIVariant));
  variant.setAsBool(true);
  assertTrue(variant.QueryInterface(Components.interfaces.nsIVariant), "Bool failed returned " + variant.QueryInterface(Components.interfaces.nsIVariant));
  variant.setAsWChar("X");
  assertTrue(variant.QueryInterface(Components.interfaces.nsIVariant) == "88", "WChar failed returned " + variant.QueryInterface(Components.interfaces.nsIVariant));
  variant.setAsChar("Y");
  assertTrue(variant.QueryInterface(Components.interfaces.nsIVariant) == "89", "Char failed returned " + variant.QueryInterface(Components.interfaces.nsIVariant));
  variant.setAsAString("AString");
  assertTrue(variant.QueryInterface(Components.interfaces.nsIVariant) == "AString", "AString failed returned " + variant.QueryInterface(Components.interfaces.nsIVariant));
  variant.setAsDOMString("DOMString");
  assertTrue(variant.QueryInterface(Components.interfaces.nsIVariant) == "DOMString", "DOMString failed returned " + variant.QueryInterface(Components.interfaces.nsIVariant));
  variant.setAsACString("ACString");
  assertTrue(variant.QueryInterface(Components.interfaces.nsIVariant) == "ACString", "ACString failed returned " + variant.QueryInterface(Components.interfaces.nsIVariant));
  variant.setAsAUTF8String("AUTF8String");
  assertTrue(variant.QueryInterface(Components.interfaces.nsIVariant) == "AUTF8String", "UTF8String failed returned " + variant.QueryInterface(Components.interfaces.nsIVariant));
  variant.setAsWString("WString");
  assertTrue(variant.QueryInterface(Components.interfaces.nsIVariant) == "WString", "WString failed returned " + variant.QueryInterface(Components.interfaces.nsIVariant));
}