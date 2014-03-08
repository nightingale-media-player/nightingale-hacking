/*
 * BEGIN NIGHTINGALE GPL
 * 
 * This file is part of the Nightingale Media Player.
 *
 * Copyright(c) 2013
 * http://getnightingale.com
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

if (typeof(Ci) == "undefined")
    var Ci = Components.interfaces;
if (typeof(Cc) == "undefined")
    var Cc = Components.classes;
if (typeof(Cr) == "undefined")
    var Cr = Components.results;
if (typeof(Cu) == "undefined")
    var Cu = Components.utils;

try {
    Cu.import("resource://gre/modules/XPCOMUtils.jsm");
    Cu.import("resource://app/jsmodules/sbProperties.jsm");
    Cu.import("resource://app/jsmodules/sbCoverHelper.jsm");
} catch (error) {
    alert("lnNotifs: Unexpected error - module import error\n\n" + error)
}

lnNotifs = {
    mmService: null,
    strConv: null,
    lastItem: null,
    wm: null,
    mainwindow: null,
    prefs: null,
    lnNotifsService: null,
    ios: null,

    onLoad: function() {
        var mainwindow = window.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIWebNavigation)
                               .QueryInterface(Ci.nsIDocShellTreeItem).rootTreeItem
                               .QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindow);
        this.mmService = Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"].getService(Ci.sbIMediacoreManager);
        this.strConv = Cc["@mozilla.org/intl/scriptableunicodeconverter"].getService(Ci.nsIScriptableUnicodeConverter);
        this.lnNotifsService = Cc["@getnightingale.com/Nightingale/lnNotifs;1"].getService(Ci.ILNNotifs);
        this.wm = Cc["@mozilla.org/appshell/window-mediator;1"].getService(Ci.nsIWindowMediator);
        this.ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);

        this.mainwindow = this.wm.getMostRecentWindow("Songbird:Main");
        var windowTitle = this.mainwindow.document.getElementById("mainplayer").getAttribute("title");

        this.prefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefService).getBranch("extensions.libnotify-notifs.");

        this.lnNotifsService.InitNotifs(windowTitle);

        this.strConv.charset = 'utf-8';
        var mm = this.mmService;
        var that = this;

        this.mmService.addListener({
            onMediacoreEvent: function(event) {
                // Get mediacore event
                if (that.mmService.sequencer.view == null) return;

                if (event.type == Ci.sbIMediacoreEvent.TRACK_CHANGE) {
                    // Check if notifications are enabled
                    var notifsEnabled = lnNotifs.prefs.getBoolPref("enableNotifications");
                    lnNotifs.lnNotifsService.EnableNotifications(notifsEnabled);

                    var curItem = that.mmService.sequencer.currentItem;
                    // Don't notify for mediacore sequencing oddities
                    if (that.lastItem == curItem) return;
                    else that.lastItem = curItem;

                    var resourceURL = curItem.getProperty(SBProperties.primaryImageURL);
                    var fileURI = null;
                    if(resourceURL) {
                        try{
                            fileURI = that.ios.newURI(decodeURI(resourceURL), null, null).QueryInterface(Ci.nsIFileURL).file.path;
                        }
                        catch(e) {
                            Cu.reportError(e);
                        }
                    }

                    var artist = that.strConv.ConvertFromUnicode(curItem.getProperty(SBProperties.artistName));
                    var album = that.strConv.ConvertFromUnicode(curItem.getProperty(SBProperties.albumName));
                    var track = that.strConv.ConvertFromUnicode(curItem.getProperty(SBProperties.trackName));
                    var timeout = that.prefs.getIntPref("notificationTimeout");

                    that.lnNotifsService.TrackChangeNotify(track, artist, album, fileURI, timeout);
                }
            }
        });
    },

    onUnload: function() {

    }
};

window.addEventListener("load",   function(e) { lnNotifs.onLoad(); },   false);
window.addEventListener("unload", function(e) { lnNotifs.onUnload(); }, false);
