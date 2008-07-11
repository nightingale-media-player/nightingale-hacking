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

function runTest() {
  var stringTransform = Cc["@songbirdnest.com/Songbird/Intl/StringTransform;1"]
                          .createInstance(Ci.sbIStringTransform);
                          
  var normalizeTestIn = "àäâéçîïë l'été est génial";
  var normalizeTestExpectedOut = "aaaeciie l'ete est genial";
  
  var normalizeTestOut = stringTransform.normalizeString("", 
                                  Ci.sbIStringTransform.TRANSFORM_IGNORE_NONSPACE,
                                  normalizeTestIn);
                                  
  log("Pre-normalized string: " + normalizeTestIn);
  log("Normalized string: " + normalizeTestOut);
  assertEqual(normalizeTestOut, normalizeTestExpectedOut);
  
  return;
}
