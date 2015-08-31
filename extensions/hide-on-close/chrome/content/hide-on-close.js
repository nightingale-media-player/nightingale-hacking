/*
 * BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale Media Player.
 *
 * Copyright(c) 2014
 * http://www.getnightingale.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the "GPL").
 *
 * Software distributed under the License is distributed
 * on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END NIGHTINGALE GPL
 */

try {
    Components.utils.import("resource://gre/modules/XPCOMUtils.jsm"); 
    Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
} catch (error) {alert("Hide on close: module import error\n\n" + error) }

if (typeof HideOnClose == 'undefined') {
    var HideOnClose = {};
};

HideOnClose = {
    xulAppInfo: null,
    wm: null,
    ngHideOnClose: null,
    observerService: null,
    mainwindow: null,
    prefs: null,

    onLoad: function () {
        var mainwindow = window.QueryInterface(Components.interfaces.nsIInterfaceRequestor).getInterface(Components.interfaces.nsIWebNavigation)
                               .QueryInterface(Components.interfaces.nsIDocShellTreeItem).rootTreeItem
                               .QueryInterface(Components.interfaces.nsIInterfaceRequestor).getInterface(Components.interfaces.nsIDOMWindow);

        this.xulAppInfo = Components.classes["@mozilla.org/xre/app-info;1"].getService(Components.interfaces.nsIXULAppInfo);
        this.stringConverter = Components.classes['@mozilla.org/intl/scriptableunicodeconverter'].getService(Components.interfaces.nsIScriptableUnicodeConverter);
        this.wm = Components.classes["@mozilla.org/appshell/window-mediator;1"].getService(Components.interfaces.nsIWindowMediator);
        this.mainwindow = this.wm.getMostRecentWindow("Songbird:Main");
        this.prefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefService).getBranch("extensions.hide-on-close.");
        this.prefs.QueryInterface(Components.interfaces.nsIPrefBranch2);

        HideOnClose.preferencesObserver.register();

        this.ngHideOnClose = Components.classes["@getnightingale.com/Nightingale/ngHideOnClose;1"].getService(Components.interfaces.ngIHideOnClose);

        var windowTitle = mainwindow.document.getElementById("mainplayer").getAttribute("title");

        if (this.xulAppInfo.name == "Nightingale")
            this.ngHideOnClose.InitializeFor("nightingale.desktop", windowTitle);
        else {
            alert("Hide on close failed to initialize")
            return;
        }

        HideOnClose.preferencesObserver.observe(null, "needInit", "enabled");

        this.observerService = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
    },

    onUnLoad: function () {
        HideOnClose.preferencesObserver.unregister();
    },

    registerOnClose: function (force) {
        if (this.prefs.getBoolPref("enabled")) {
            var that = this;
            this.mainwindow.onclose = function () {
                that.ngHideOnClose.HideWindow(); 
                return false;
            }
        } else if (force) {
            this.mainwindow.onclose = "";
        }
    },

    preferencesObserver: {
        register: function () {
            HideOnClose.prefs.addObserver("", this, false);
        },

        unregister: function () {
            HideOnClose.prefs.removeObserver("", this);
        },

        observe: function (aSubject, aTopic, aData) {
            if(aTopic != "nsPref:changed" && aTopic != "needInit") return;

            if (aData == "enabled")
                HideOnClose.registerOnClose(true);
        }
    }
};

window.addEventListener("load",   function(e) { HideOnClose.onLoad(); },   false);
window.addEventListener("unload", function(e) { HideOnClose.onUnLoad(); }, false);
