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

/**
 * \file  sbTestDisplayPaneProvider.js
 * \brief Component implementing sbIDisplayPaneContentInfo and using
 *        "display-pane-provider" category to target content at display panes.
 *        This component is only built and registered when tests are enabled.
 */

const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function TestDisplayPaneProvider() {
}

TestDisplayPaneProvider.prototype = {

  classDescription: "Songbird Test Display Pane Content Info Provider",
  classID:          Components.ID("{34b1c7d0-1dd2-11b2-9e33-9086cae69f99}"),
  contractID:       "@songbirdnest.com/Songbird/DisplayPane/test-provider;1",
  _xpcom_categories:
    [{
      category: "display-pane-provider",
      entry: "test-provider"
    }],

  contentUrl: "provider_component_contentUrl",
  contentTitle: "provider_component_contentTitle",
  contentIcon: "provider_component_contentIcon",
  defaultWidth: 220,
  defaultHeight: 340,
  suggestedContentGroups: "provider_component_suggestedContentGroups",

  QueryInterface:
      XPCOMUtils.generateQI([Ci.sbIDisplayPaneContentInfo])
};

var NSGetModule = XPCOMUtils.generateNSGetFactory([TestDisplayPaneProvider]);
