/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

/**
 * \brief ServicePaneHelper.jsm unit tests
 */

Components.utils.import("resource://app/jsmodules/ServicePaneHelper.jsm");

function DBG(s) { log('DBG:test_servicepanehelper: '+s); }

function testBadges(SPS, aRoot) {
  function getBadgeIDs() {
    return [badge.id for (badge in ServicePaneHelper.getAllBadges(aRoot))];
  }

  assertArraysEqual(getBadgeIDs(), []);

  let badge = ServicePaneHelper.getBadge(aRoot, "testBadge");
  assertTrue(badge, "Getting a non-existent badge doesn't create it");
  assertEqual(badge.id, "testBadge", "getBadge didn't initialize badge ID");
  assertEqual(badge.node, aRoot, "getBadge didn't initialize badge node");
  assertFalse(badge.visible, "Badge isn't invisible by default");

  assertArraysEqual(getBadgeIDs(), []);

  badge.label = "label";
  badge.image = "chrome://example/skin/image.png";

  assertEqual(badge.label, "label", "Setting badge label didn't work");
  assertEqual(badge.image, "chrome://example/skin/image.png",
                           "Setting badge image didn't work");

  assertArraysEqual(getBadgeIDs(), []);

  badge = ServicePaneHelper.getBadge(aRoot, "testBadge");
  assertEqual(badge.label, "label", "Re-getting badge cleared badge label");
  assertEqual(badge.image, "chrome://example/skin/image.png",
                           "Re-getting badge cleared badge image");

  assertArraysEqual(getBadgeIDs(), []);

  badge.append();
  assertArraysEqual(getBadgeIDs(), ["testBadge"]);

  assertEqual(badge.id, "testBadge", "getBadge didn't initialize badge ID");
  assertEqual(badge.node, aRoot, "getBadge didn't initialize badge node");
  assertTrue(badge.visible, "Badge isn't invisible by default");
  assertEqual(badge.label, "label", "Appending badge cleared badge label");
  assertEqual(badge.image, "chrome://example/skin/image.png",
                           "Appending badge cleared badge image");

  badge = ServicePaneHelper.getBadge(aRoot, "testBadge");
  assertTrue(badge.visible, "Re-getting badge cleared visibility flag");
  assertEqual(badge.label, "label", "Re-getting badge cleared badge label");
  assertEqual(badge.image, "chrome://example/skin/image.png",
                           "Re-getting badge cleared badge image");

  let badge2 = ServicePaneHelper.getBadge(aRoot);
  assertTrue(badge2, "Getting badge without ID doesn't create it");
  assertTrue(badge2.id, "getBadge didn't generate badge ID");

  assertArraysEqual(getBadgeIDs(), ["testBadge"]);

  badge2.insertBefore("testBadge");
  assertArraysEqual(getBadgeIDs(), [badge2.id, "testBadge"]);

  assertEqual(badge2.label, null, "Badge label isn't null by default");
  assertEqual(badge2.image, null, "Badge image isn't null by default");

  badge2.label = "label2";
  assertEqual(badge2.label, "label2",
                            "Changing label of visible badge didn't persist");
  assertEqual(badge.label, "label", "Label of unrelated badge was modified");

  badge.insertBefore(badge2.id);
  assertArraysEqual(getBadgeIDs(), ["testBadge", badge2.id]);

  badge.visible = false;
  assertArraysEqual(getBadgeIDs(), [badge2.id]);
  assertFalse(badge.visible, "Badge is still visible after hiding");
  assertEqual(badge.label, "label", "Hiding badge cleared badge label");
  assertEqual(badge.image, "chrome://example/skin/image.png",
                           "Hiding badge cleared badge image");

  badge.remove();
  assertArraysEqual(getBadgeIDs(), [badge2.id]);
  assertEqual(badge.label, null, "Removing badge didn't clear badge label");
  assertEqual(badge.image, null, "Removing badge didn't clear badge image");

  badge.remove();
  assertArraysEqual(getBadgeIDs(), [badge2.id]);

  badge2.remove();
  assertArraysEqual(getBadgeIDs(), []);
  assertFalse(badge2.visible, "Badge is still visible after removing");

  // Root node should still have no attributes after the test
  assertEqual(serializeTree(aRoot), '[node/]');
}

function runTest() {
  let SPS = Cc["@getnightingale.com/servicepane/service;1"]
              .getService(Ci.sbIServicePaneService);
  
  // okay, did we get it?
  assertNotEqual(SPS, null, "Failed to get service pane service");

  // Can we create test node?
  let testRoot = SPS.createNode();
  assertTrue(testRoot instanceof Ci.sbIServicePaneNode,
             "created node is not a service pane node");

  SPS.root.appendChild(testRoot);

  // Test badges
  testBadges(SPS, testRoot);

  // Clean up
  SPS.root.removeChild(testRoot);
}
