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


/* wait for completion global object */

var gWaitForCompletion = {};
gWaitForCompletion.mDeviceManager = null;
gWaitForCompletion.mDevices = [];

/* window load event handler */
gWaitForCompletion.onLoad = function()
{
  /* Get the dialog element. */
  var dialog = document.getElementById("dialog");

  /* Hide the cancel button. */
  var cancelButton = document.getAnonymousElementByAttribute(dialog, "dlgtype",
     "cancel");
  cancelButton.setAttribute("hidden", "true");

  /* get the device manager */
  this.mDeviceManager = 
    Components.classes['@songbirdnest.com/Songbird/DeviceManager;2']
    .getService(Components.interfaces.sbIDeviceManager2);

  /* listen to all device events */
  this.mDeviceManager.addEventListener(this);
}

/* window unload event handler */
gWaitForCompletion.onUnload = function()
{
  // remove ourselves as a device event listener
  if (this.mDeviceManager) {
    this.mDeviceManager.removeEventListener(this);
  }
}

/* sbIDeviceEventListener callback */
gWaitForCompletion.onDeviceEvent = function(aDeviceEvent)
{
  /* some event occurred. can we disconnect yet? */
  if (this.mDeviceManager.canDisconnect) {
    /* close the window */
    window.close();
  }
}

/* just enough nsISupports to make everyone happy */
gWaitForCompletion.QueryInterface = function(aInterfaceID)
{
  /* Check for supported interfaces. */
  if (!aInterfaceID.equals(Components.interfaces.nsISupports) && 
      !aInterfaceID.equals(Components.interfaces.sbIDeviceEventListener)) { 
    throw(Components.results.NS_ERROR_NO_INTERFACE);
  }

  return this;
}
