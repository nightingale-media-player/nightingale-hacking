/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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
//  sbIRestartBox
//

const SB_RESTARTBOX_RESTARTLATER = 0;
const SB_RESTARTBOX_RESTARTNOW   = 1;
const SB_RESTARTBOX_RESTARTEOP   = 2;

function sbRestartBox( title, message )
{
  try
  {
    var restartbox_data = new Object();
    restartbox_data.title = title;
    restartbox_data.message = message;
    restartbox_data.playing = gPPS.playing;
    restartbox_data.ret = 0;
    SBOpenModalDialog( "chrome://songbird/content/xul/restartBox.xul", "restartbox", "chrome,centerscreen", restartbox_data ); 
    var restartOnPlaybackEnd = SB_NewDataRemote( "restart.onplaybackend", null );
    switch (restartbox_data.ret)
    {
      case SB_RESTARTBOX_RESTARTLATER: // restart later
              restartOnPlaybackEnd.boolValue = false;
              break; 
      case SB_RESTARTBOX_RESTARTNOW: // restart now
              restartOnPlaybackEnd.boolValue = false;
              SBDataSetBoolValue("restart.restartnow", true);
              break;
      case SB_RESTARTBOX_RESTARTEOP: // restart on end of playback
              restartOnPlaybackEnd.boolValue = true;
              break;
    }
    return restartbox_data.ret;
  }
  catch ( err )
  {
    alert("sbRestartBox - " + err);
  }
  return -1;
}

function sbRestartBox_strings(titlestring, msgstring, defaulttitle, defaultmsg) {
  var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
  var prop = sbs.createBundle("chrome://songbird/locale/songbird.properties");
  var msg = defaultmsg;
  var title = defaulttitle;
  try {
    // These can throw if the strings don't exist.
    msg = prop.GetStringFromName(msgstring);
    title = prop.GetStringFromName(titlestring);
  } catch (e) { }
  return sbRestartBox(title, msg);
}

