/** vim: ts=2 sw=2 expandtab ai
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
 * \brief Basic service pane unit tests
 */


function runTest () {
  var SPS = Components.classes['@songbirdnest.com/servicepane/service;1'].getService(Components.interfaces.sbIServicePaneService);
  
  // okay, did we get it?
  assertNotEqual(SPS, null);

  var test_root_id = 'SB:UnitTestRoot';
  var test_root;

  test_root = SPS.getNode(test_root_id);
  if (test_root) {
    // there's leftover unit testing cruft. lets throw it away
    test_root.parentNode.removeChild(test_root);
  }


  // create a node
  test_root = SPS.addNode(test_root_id, SPS.root, true);
  assertNotEqual(test_root, null);

  // check that its rdf id is exposed correctly
  assertEqual(test_root.id, test_root_id);
  assertEqual(test_root.resource.ValueUTF8, test_root_id);

  // check that it was correctly created as a container
  assertEqual(test_root.isContainer, true);
  assertNotEqual(test_root.childNodes, null);
  assertEqual(test_root.childNodes.hasMoreElements(), false);

  // check that it was added at the end of the root
  assertEqual(test_root.parentNode.id, SPS.root.id);
  assertEqual(test_root.nextSibling, null);

  // set some properties
  var test_root_properties = {
    name: 'Unit Test Root',
    hidden: false,
    tooltip: 'This is where the unit tests run'
  }
  var k;
  for (k in test_root_properties) {
    dump ('about to set '+k+'\n');
    test_root[k] = test_root_properties[k];
  }

  // test that they were set right
  for (k in test_root_properties) {
    dump ('about to check '+k+'\n');
    assertEqual(test_root[k], test_root_properties[k]);
    dump ('ok\n');
  }

  // create a node inside the root
  var test_node = SPS.addNode(null, test_root, false);
  assertNotEqual(test_node, null);

  var test_node_id = test_node.id

  // make sure it is not a container
  assertEqual(test_node.isContainer, false);

  // make sure the resource and id match
  assertEqual(test_node.id, test_node.resource.ValueUTF8);

  // check to see it has been correctly placed into the root
  assertEqual(test_node.parentNode.id, test_root.id);
  assertEqual(test_node.id, test_root.firstChild.id);
  assertEqual(test_node.id, test_root.lastChild.id);
  assertEqual(test_node.previousSibling, null);
  assertEqual(test_node.nextSibling, null);
  var en = test_root.childNodes;
  assertEqual(en.hasMoreElements(), true);
  assertEqual(en.getNext().id, test_node.id);
  assertEqual(en.hasMoreElements(), false);

  // add a second node
  var test_node_2 = SPS.addNode(null, test_root, false);
  assertNotEqual(test_node_2, null);

  // check to see it has been correctly placed into the root
  assertEqual(test_node_2.parentNode.id, test_root.id);
  assertEqual(test_node.id, test_root.firstChild.id);
  assertEqual(test_node_2.id, test_root.lastChild.id);
  assertEqual(test_node.previousSibling, null);
  assertEqual(test_node.nextSibling.id, test_node_2.id);
  assertEqual(test_node_2.previousSibling.id, test_node.id);
  assertEqual(test_node_2.nextSibling, null);
  var en = test_root.childNodes;
  assertEqual(en.hasMoreElements(), true);
  assertEqual(en.getNext().id, test_node.id);
  assertEqual(en.hasMoreElements(), true);
  assertEqual(en.getNext().id, test_node_2.id);
  assertEqual(en.hasMoreElements(), false);


  // add a second container
  var test_container = SPS.addNode(null, SPS.root, true);

  // move the second node into the second container
  test_container.appendChild(test_node_2);

  // ensure that that's where it is
  assertEqual(test_node_2.parentNode.id, test_container.id);

  // insert it before the other node
  test_root.insertBefore(test_node_2, test_node);
  assertEqual(test_node_2.nextSibling.id, test_node.id);

  // test replaceChild
  test_container.appendChild(test_node);
  var test_node_2_id = test_node_2.id;
  test_root.replaceChild(test_node, test_node_2);
  assertEqual(test_node.parentNode.id, test_root.id);
  assertEqual(test_node.previousSibling, null);
  assertEqual(test_node.nextSibling, null);
  assertEqual(SPS.getNode(test_node_2_id), null);

  // test removeChild
  test_node_2 = SPS.addNode(null, test_root, false);
  test_node_2_id = test_node_2.id;
  test_root.removeChild(test_node_2);
  assertEqual(test_node.parentNode.id, test_root.id);
  assertEqual(test_node.previousSibling, null);
  assertEqual(test_node.nextSibling, null);
  assertEqual(SPS.getNode(test_node_2_id), null);


  // clean up 
  dump ('cleaning up unit test nodes\n');
  test_root.parentNode.removeChild(test_root);
  test_container.parentNode.removeChild(test_container);

  // make sure the nodes are gone
  assertEqual(SPS.getNode(test_root_id), null);
  assertEqual(SPS.getNode(test_node_id), null);

  SPS.save();
}

