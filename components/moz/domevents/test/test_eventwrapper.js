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
 * \brief test the event wrapper to carry a payload
 */

function runTest() {
  var retval = false;
  
  function listener(event) {
    if (event instanceof Components.interfaces.sbIDOMEventWrapper)
      retval = event.data.wrappedJSObject.value;
  }

  var w = Components.classes["@songbirdnest.com/moz/domevents/wrapper;1"]
                    .createInstance(Components.interfaces.sbIDOMEventWrapper);
  
  var document = Components.classes["@mozilla.org/xml/xml-document;1"]
                           .createInstance(Components.interfaces.nsIDOMEventTarget)
                           .QueryInterface(Components.interfaces.nsIDOMDocumentEvent);
  
  // make an underlying event and wrap it with a payload
  var e = document.createEvent("events");
  e.initEvent("events", false, false);
  w.init(e, #1={value: true, wrappedJSObject: #1# });

  document.addEventListener("events", listener, false);
  document.dispatchEvent(w);
  document.removeEventListener("events", listener, false);
  
  assertTrue(retval);
}
