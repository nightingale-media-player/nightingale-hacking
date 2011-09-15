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

EXPORTED_SYMBOLS = [ "ServicePaneHelper" ];

/**
 * \file ServicePaneHelper.jsm
 * \brief helper functions for working with the service pane
 */

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Ce = Components.Exception;
const Cu = Components.utils;

/**
 * \brief Helper function to retrieve visible badges for a service pane node.
 *
 * \param node sbIServicePaneNode to check
 * \return array of badge IDs (empty if no badges visible)
 */
function getBadgesForNode(node)
{
  let attr = node.getAttribute("badges");
  if (attr)
    return attr.split(/\s+/);
  else
    return [];
}

/**
 * \brief counter used to generate unique IDs for badges where no ID is
 *        supplied.
 */
var badgeIndex = 0;

/**
 * \class ServicePaneBadge
 *
 * This is a tear-off object, only created when badge data needs to be
 * manipulated. It doesn't store any data, real data is stored in node
 * attributes.
 *
 * \param node node that the badge is tied to (becomes the node property
 *        of the object)
 * \param id badge ID (becomes the id property of the object)
 */
function ServicePaneBadge(node, id) {
  if (!id)
    throw "Badge ID is a mandatory parameter";
  if (id && /\s/.test(id))
    throw "Spaces are not allowed in badge ID";

  this.__defineGetter__("node", function() node);
  this.__defineGetter__("id", function() id);
}
ServicePaneBadge.prototype = {
  /**
   * \brief Retrieves and changes the displayed label of the badge.
   */
  get label() {
    return this.node.getAttribute("badge_" + this.id + "_label");
  },
  set label(value) {
    this.node.setAttribute("badge_" + this.id + "_label", value);
    return value;
  },

  /**
   * \brief Retrieves and changes the image URL to be displayed for the badge.
   */
  get image() {
    return this.node.getAttribute("badge_" + this.id + "_image");
  },
  set image(value) {
    this.node.setAttribute("badge_" + this.id + "_image", value);
    return value;
  },

  /**
   * \brief Retrieves and changes the visibility of the badge.
   *
   * Changing this property to true will have the same result as calling
   * append(). Changing it to false will make the badge invisible, its
   * attributes will not be cleared however.
   */
  get visible() {
    let badges = getBadgesForNode(this.node);
    return (badges.indexOf(this.id) >= 0);
  },
  set visible(val) {
    let badges = getBadgesForNode(this.node);
    let currentIndex = badges.indexOf(this.id);
    if (val) {
      if (currentIndex < 0)
        this.append();
    }
    else {
      if (currentIndex >= 0) {
        badges.splice(currentIndex, 1);
        this.node.setAttribute("badges", badges.length ? badges.join(" ") : null);
      }
    }
    return false;
  },

  /**
   * \brief Makes the current badge visible as the last badge of the node. Will
   *        move the badge if it is already visible.
   */
  append: function()
  {
    this.insertBefore(null);
  },

  /**
   * \brief Makes the current badge visible by inserting it before another
   *        badge. Will move the badge if it is already visible.
   */
  insertBefore: function(id) {
    if (id && /\s/.test(id))
      throw "No spaces allowed in badge ID";

    let badges = getBadgesForNode(this.node);

    let currentIndex = badges.indexOf(this.id);
    if (currentIndex >= 0)
      badges.splice(currentIndex, 1);

    let newIndex = (id ? badges.indexOf(id) : -1);
    if (newIndex < 0)
      newIndex = badges.length;
    badges.splice(newIndex, 0, this.id);

    this.node.setAttribute("badges", badges.join(" "));
  },

  /**
   * \brief Removes the current badge and all node attributes associated with
   *        it.
   *
   * Note: this will also unset the properties of the current badge object. If
   * you only want to hide the badge use badge.visible = false.
   */
  remove: function() {
    this.visible = false;
    this.node.removeAttribute("badge_" + this.id + "_label");
    this.node.removeAttribute("badge_" + this.id + "_image");
  }
};

var ServicePaneHelper = {
  /**
   * \brief Creates a new ServicePaneBadge object to help manipulate a badge
   *        on a particular service pane node.
   *
   * The object returned is a temporary tear-off object, references to is
   * should be released one it is no longer needed to allow garbage collection
   * to pick it up. Note that two subsequent calls to getBadge() will not return
   * the same object even though all properties will return the same values.
   *
   * If the badge doesn't exist yet a new one will be created. In this case a
   * call to badge.append() or badge.insertBefore() is necessary to make it
   * visible.
   *
   * \param node node that the badge is tied to
   * \param id (optional) ID of the badge. If missing
   *        an ID will be generated automatically. Supplied ID should not
   *        contain any spaces or NUL characters.
   * \return ServicePaneBadge object for the badge
   */
  getBadge: function(node, badgeID) {
    if (!badgeID) {
      // Generate badge ID automatically if none given, use NUL character to
      // avoid clashes with user-supplied IDs
      badgeID = "badge\x00" + badgeIndex++;
    } else if (/\x00/.test(badgeID)) {
      // NUL characters are reserved for generated IDs
      throw "No NUL characters allowed in badge ID";
    }

    return new ServicePaneBadge(node, badgeID);
  },

  /**
   * \brief Retrieves all visible badges of a node.
   *
   * The order in which the badges are returned is the order in which they
   * should be displayed.
   *
   * \param node the node for which the badges should be retrieved
   * \return iterator returning ServicePaneBadge objects
   */
  getAllBadges: function(node) {
    let badges = getBadgesForNode(node);
    return (new ServicePaneBadge(node, badgeID) for each (badgeID in badges));
  }
};
