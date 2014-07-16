/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");

if (typeof(Cc) == "undefined")
  this.Cc = Components.classes;
if (typeof(Ci) == "undefined")
  this.Ci = Components.interfaces;

function runTest() {
    var mutablePreset = Cc["@getnightingale.com/equalizer-presets/mutable;1"]
                        .createInstance(Ci.ngIMutableEqualizerPreset);
    assertEqual(mutablePreset.name, null, "Name is already defined");
    assertEqual(mutablePreset.values, null, "Values are already defined");

    mutablePreset.setName("test");
    assertEqual(mutablePreset.name, "test", "Name not set correctly");

    mutablePreset.setValues(ArrayConverter.nsIArray([0,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9]));
    assertTrue(mutablePreset.values, "Values not defined even though previously set");
    assertEqual(mutablePreset.values.length, 10, "Not all values were written into the array");

    return;
}