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
 * \brief Common service pane helper functions
 */

/**
 * \brief Creates a string representation of the nodes tree. Example output:
 *        [node foo="bar" id="node"][node id="child"/][/node]
 *        Attributes are sorted by name.
 * \param aNode root node for serialization
 */
function serializeTree(aNode) {
  let result = "[node";

  let attributes = aNode.attributes;
  let attrList = [];
  while (attributes.hasMore()) {
    let attr = attributes.getNext();
    attrList.push(" " + attr + '="' + aNode.getAttribute(attr) + '"');
  }
  attrList.sort();
  result += attrList.join("");

  let childNodes = aNode.childNodes;
  if (childNodes.hasMoreElements()) {
    // Node has children
    result += "]";
    while (childNodes.hasMoreElements()) {
      let child = childNodes.getNext().QueryInterface(Ci.sbIServicePaneNode);
      result += serializeTree(child);
    }
    result += "[/node]";
  }
  else {
    // No children, close "tag" immediately
    result += "/]";
  }

  return result;
}
