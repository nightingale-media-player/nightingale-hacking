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

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

// Class constructor.
function PartnerDistHelperInitializer() {
}

// Class definition.
PartnerDistHelperInitializer.prototype = {
    // XPCOM goop
    classDescription: "PartnerDistHelperInitializer XPCOM Component",
    
    classID:          Components.ID("{fe6eec71-ab2c-483c-8696-662cbbcd8cd3}"),
    contractID:       "@example.com/PartnerDistHelperInitializer;1",
    
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

    _xpcom_categories: [
        {
            category: "app-startup"
        }
    ],

    // nsIObsever    
    observe: function(aSubject, aTopic, aData) {
        var env = Cc["@mozilla.org/process/environment;1"]
                    .getService(Ci.nsIEnvironment);

        // This is the addon's directory (which contains install.rdf)
        var addonDir = __LOCATION__.parent.parent;

        // set DISTHELPER_DISTINI to point to our add-on's distribution.ini
        var distIniFile = addonDir.clone();
        distIniFile.append("distribution");
        distIniFile.append("distribution.ini");
        
        env.set("DISTHELPER_DISTINI", distIniFile.path);
        dump("setting DISTHELPER_DISTINI: " + distIniFile.path + "\n");
        Cu.reportError("setting DISTHELPER_DISTINI: " + distIniFile.path + "\n");
        
        // set another environment variable to get passed to disthelper
        env.set("DISTHELPER_FOOFOO", Math.random());
    }
};

// XPCOM registration of class.
var components = [PartnerDistHelperInitializer];
function NSGetModule(compMgr, fileSpec) {
    return XPCOMUtils.generateModule(components);
}
