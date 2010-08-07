/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function runTest() {

  const prop = "http://songbirdnest.com/data/1.0#imageLabelLink";

  var builder =
    Cc["@songbirdnest.com/Songbird/Properties/Builder/ImageLabelLink;1"]
      .createInstance(Ci.sbIImageLabelLinkPropertyBuilder)
      .QueryInterface(Ci.sbIClickablePropertyBuilder);

  builder.propertyID = prop;
  builder.displayName = "Display";

  const K_DATA = {
    "dummy": { label: "Dummy",
               image: "http://0/dummy.png" },
    "two":   { label: "\u01d5\u0144\u0131\u00e7\u00f8\u0111\u0115",
               image: "http://0/two.png" },
    "":      { label: "Default",
               image: "http://0/default.png" }
  };
  for (let key in K_DATA) {
    builder.addImage(key ? key : null, K_DATA[key].image);
    builder.addLabel(key ? key : null, K_DATA[key].label);
  }

  function onclick(aPropertyInfo, aItem, aEvent, aContext) {
    assertEqual(aPropertyInfo, pi);
    assertEqual(aItem.wrappedJSObject, item);
    assertEqual(aEvent.wrappedJSObject, event);
    assertEqual(aContext.wrappedJSObject, context);
    testFinished();
  }
  builder.addClickHandler(onclick);

  var pi = builder.get();
  assertTrue(pi instanceof Ci.sbITreeViewPropertyInfo,
             "image label link property info should be a sbITreeViewPropertyInfo!");
  assertTrue(pi instanceof Ci.sbIClickablePropertyInfo,
             "image label link property info should be a sbIClickablePropertyInfo!");

  assertEqual(pi.type, "image");
  assertEqual(pi.id, prop);
  assertEqual(pi.displayName, "Display");

  for (let key in K_DATA) {
    assertEqual(pi.format(key), K_DATA[key].label,
                "Expecting to display [" + K_DATA[key].label +
                  "] for property value [" + key + "]");
    assertEqual(pi.getImageSrc(key), K_DATA[key].image,
                "Expecting to get image [" + K_DATA[key].image +
                  "] for property value [" + key + "]");
  }
  assertEqual(pi.format("missing"), K_DATA[""].label,
              "Expecting to use default label [" + K_DATA[""].label +
                "] for missing key [missing]");
  assertEqual(pi.getImageSrc("missing"), K_DATA[""].image,
              "Expecting to use default image [" + K_DATA[""].image +
                "] for missing key [missing]");

  var item = #1= {
    QueryInterface: XPCOMUtils.generateQI([Ci.sbIMediaItem]),
    wrappedJSObject: #1#
  };
  var event = #2= { wrappedJSObject: #2# };
  var context = #3= { wrappedJSObject: #3# };
  pi.onClick(item, event, context);
  testPending();
}
