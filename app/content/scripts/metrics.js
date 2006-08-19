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

// Metrics

// Not assuming sbDataRemoteUtils.js is loaded because this file may be used by a js component

function getValue(key)
{
  var v = 0;
  try
  {
    var pref = Components.classes["@mozilla.org/preferences-service;1"]
                      .getService(Components.interfaces.nsIPrefBranch);
    v = parseInt(pref.getCharPref(key));                  
  }
  catch (e)
  {
  }
  return v;
}
 
function setValue(key, n)
{
  try
  {
    var pref = Components.classes["@mozilla.org/preferences-service;1"]
                      .getService(Components.interfaces.nsIPrefBranch);
    pref.setCharPref(key, parseInt(n));
  }
  catch (e)
  {
  }
}
 

function metrics_add( category, unique_id, extra, intvalue )
{
  try 
  {
    var enabled = getValue("app.metrics.enabled");
    if (!enabled) return;

    // only integers allowed
    intvalue = parseInt(intvalue);

    var key = "metrics." + category + "." + unique_id;
    if (extra != null && extra != "") key = key + "." + extra;
    var cur = getValue( key );
    var newval = cur + intvalue;
    setValue( key, newval );
    //var consoleService = Components.classes['@mozilla.org/consoleservice;1']
    //                        .getService(Components.interfaces.nsIConsoleService);
    //consoleService.logStringMessage(key + " is now " + newval);
  }
  catch(e)
  {
  }
}

function metrics_inc( category, unique_id, extra )
{
  metrics_add(category, unique_id, extra, 1);
}

