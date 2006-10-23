/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright? 2006 POTI, Inc.
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

// get the songbird components directory and register the components in it.
function do_load_songbird () {
  try {
    // this returns the xulrunner directory
    var compDir = Components.classes["@mozilla.org/file/directory_service;1"]
                             .getService(Components.interfaces.nsIProperties)
                             .get("CurProcD", Components.interfaces.nsIFile);

    // our components are down the hall to the left
    compDir.append("..");
    compDir.append("components");

    dump("hey! we're over there:" + compDir.path + "\n");

    // unfortunately right now this only ends up adding the classes to
    // compreg.dat and not into xpti.dat    
    Components.manager.nsIComponentRegistrar.autoRegister(compDir);

    // iterate over component classes
    for(var contractID in Components.classes)
    {
      if ( contractID.indexOf("songbird") != -1 )
        dump("Component.classes[" + contractID + "]\n");
      //Components.classes[contractID].createInstance(this.m_interfaceID);
    }

    // iterate over interfaces
    for(var contractID in Components.interfaces)
    {
      //if ( contractID.indexOf("songbird") != -1 )
        dump("Component.interfaces[" + contractID + "]\n");
      //Components.classes[contractID].createInstance(this.m_interfaceID);
    }
  }
  catch (err) {
    dump("ERROR: (do_load_songbird) " + err + "\n");
  }
}
//do_load_songbird();

