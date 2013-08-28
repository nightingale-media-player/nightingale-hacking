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

    onLoad: function() {
        var mainwindow = window.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIWebNavigation)
                               .QueryInterface(Ci.nsIDocShellTreeItem).rootTreeItem
                               .QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindow);
        this.mmService = Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"].getService(Ci.sbIMediacoreManager);
        this.strConv = Cc["@mozilla.org/intl/scriptableunicodeconverter"].getService(Ci.nsIScriptableUnicodeConverter);
        this.lnNotifsService = Cc["@getnightingale.com/Nightingale/lnNotifs;1"].getService(Ci.ILNNotifs);
        this.wm = Cc["@mozilla.org/appshell/window-mediator;1"].getService(Ci.nsIWindowMediator);
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

                switch (event.type) {
                    case Ci.sbIMediacoreEvent.TRACK_CHANGE:
                        // Check if notifications are enabled
                        var notifsEnabled = lnNotifs.prefs.getBoolPref("enableNotifications");
                        lnNotifs.lnNotifsService.EnableNotifications(notifsEnabled);

                        var curItem = that.mmService.sequencer.currentItem;
                        // Don't notify for mediacore sequencing oddities
                        if (that.lastItem == curItem) return;
                        else that.lastItem = curItem;

                        var resourceURL = curItem.getProperty(SBProperties.primaryImageURL);
                        var artist = that.strConv.ConvertFromUnicode(curItem.getProperty(SBProperties.artistName));
                        var album = that.strConv.ConvertFromUnicode(curItem.getProperty(SBProperties.albumName));
                        var track = that.strConv.ConvertFromUnicode(curItem.getProperty(SBProperties.trackName));
                        var timeout = that.prefs.getIntPref("notificationTimeout");
                        that.downloadFileToTemp(resourceURL, function (coverFilePath) {
                            that.lnNotifsService.TrackChangeNotify(track, artist, album, coverFilePath, timeout);
                        });
                        break;
                    default:
                        break;
                }
            }
        });
    },

    onUnload: function() {

    },

    downloadFileToTemp: function (aWebURL, aCallback) {
        if (!aWebURL || aWebURL == "") {
            aCallback(null);
            return;
        }
        
        // The tempFile we are saving to.
        var tempFile = Cc["@mozilla.org/file/directory_service;1"]
                         .getService(Ci.nsIProperties)
                         .get("TmpD", Ci.nsIFile);
        
        var webProgressListener = {
            QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener]),

            /* nsIWebProgressListener methods */
            // No need to implement anything in these functions
            onLocationChange : function (a, b, c) { },
            onProgressChange : function (a, b, c, d, e, f) { },
            onSecurityChange : function (a, b, c) { },
            onStatusChange : function (a, b, c, d) { },
            onStateChange : function (aWebProgress, aRequest, aStateFlags, aStatus) {
                // when the transfer is complete...
                if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
                    if (aStatus == 0) {
                        aCallback(tempFile.path);
                    } else { }
                }
            }
        };
        
        var fileName = aWebURL.substr(aWebURL.lastIndexOf("/")+1);
        tempFile.append(fileName);
        
        try {
            tempFile.create(Ci.nsIFile.NORMAL_FILE_TYPE, 0644);
        } catch (e) {
            if (e.name == "NS_ERROR_FILE_ALREADY_EXISTS")
                aCallback(tempFile.path);
            else
                aCallback(null);
                
            return;
        };
        
        // Make sure it is deleted when we shutdown
        var registerFileForDelete = Cc["@mozilla.org/uriloader/external-helper-app-service;1"]
                                     .getService(Ci.nsPIExternalAppLauncher);
        registerFileForDelete.deleteTemporaryFileOnExit(tempFile);
        
        // Download the file
        var ioService = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
        var webDownloader = Cc['@mozilla.org/embedding/browser/nsWebBrowserPersist;1'].createInstance(Ci.nsIWebBrowserPersist);
        webDownloader.persistFlags = Ci.nsIWebBrowserPersist.PERSIST_FLAGS_NONE;
        webDownloader.progressListener = webProgressListener;
        webDownloader.saveURI(ioService.newURI(aWebURL, null, null), // URL
                              null,
                              null,
                              null,
                              null,
                              tempFile);  // File to save to
    }
};

window.addEventListener("load",   function(e) { lnNotifs.onLoad(); },   false);
window.addEventListener("unload", function(e) { lnNotifs.onUnload(); }, false);
