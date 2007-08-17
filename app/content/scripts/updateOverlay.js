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

/**
 * \file updateOverlay.js
 * \brief Contains all support functions required by the overlay applied to the auto update dialog.
 * \internal
 */
 
/**
 * \brief Make text links use openURLExternal instead of the default action.
 * \internal 
 */
function fixTextLink(evt) {
  var node = evt.target;
  var nodeClass = node.getAttribute("class");
  if(nodeClass == "text-link")
  {
    openURLExternal(node.href);
    evt.preventDefault();
  }
}

/**
 * \brief Open a URL in the default OS web browser.
 * \internal
 */
function openURLExternal(url) {
  try {
    var externalLoader = (Components
          .classes["@mozilla.org/uriloader/external-protocol-service;1"]
          .getService(Components.interfaces.nsIExternalProtocolService));
    var nsURI = (Components
          .classes["@mozilla.org/network/io-service;1"]
          .getService(Components.interfaces.nsIIOService)
          .newURI(url, null, null));
    externalLoader.loadURI(nsURI, null);
  } catch(err) { alert(err); }
  
}

window.addEventListener("click", fixTextLink, true);
