/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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
 
/************************************************************************************
 * Firefox Browser Compatibility Functions. 
 * See http://lxr.mozilla.org/seamonkey/source/browser/base/content/utilityOverlay.js
 ***********************************************************************************/

/*
 * Songbird Dependencies:
 *    gBrowser
 *    About() in playerOpen.js
 */

if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;

var gBidiUI = false;

function getBrowserURL()
{
  throw("browserUtilities.js: :  getBrowserURL() is not implemented in Songbird!");
}

function goToggleToolbar( id, elementID )
{
  var toolbar = document.getElementById(id);
  var element = document.getElementById(elementID);
  if (toolbar)
  {
    var isHidden = toolbar.hidden;
    toolbar.hidden = !isHidden;
    document.persist(id, 'hidden');
    if (element) {
      element.setAttribute("checked", isHidden ? "true" : "false");
      document.persist(elementID, 'checked');
    }
  }
}

function getTopWin()
{
  var windowManager = Components.classes['@mozilla.org/appshell/window-mediator;1']
                                .getService(Components.interfaces.nsIWindowMediator);
  return windowManager.getMostRecentWindow("Songbird:Main");
}

function openTopWin( url )
{
  openUILink(url, {})
}

function getBoolPref ( prefname, def )
{
  try { 
    var pref = Components.classes["@mozilla.org/preferences-service;1"]
                       .getService(Components.interfaces.nsIPrefBranch);
    return pref.getBoolPref(prefname);
  }
  catch(er) {
    return def;
  }
}

// Change focus for this browser window to |aElement|, without focusing the
// window itself.
function focusElement(aElement) {
  // This is a redo of the fix for jag bug 91884
  var ww = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                     .getService(Components.interfaces.nsIWindowWatcher);
  if (window == ww.activeWindow)
    aElement.focus();
  else {
    // set the element in command dispatcher so focus will restore properly
    // when the window does become active
    var cmdDispatcher = document.commandDispatcher;
    if (aElement instanceof Window) {
      cmdDispatcher.focusedWindow = aElement;
      cmdDispatcher.focusedElement = null;
    }
    else if (aElement instanceof Element) {
      cmdDispatcher.focusedWindow = aElement.ownerDocument.defaultView;
      cmdDispatcher.focusedElement = aElement;
    }
  }
}

// openUILink handles clicks on UI elements that cause URLs to load.
function openUILink( url, e, ignoreButton, ignoreAlt, allowKeywordFixup, postData, referrerUrl )
{
  var where = whereToOpenLink(e, ignoreButton, ignoreAlt);
  openUILinkIn(url, where, allowKeywordFixup, postData, referrerUrl);
}


/* whereToOpenLink() looks at an event to decide where to open a link.
 *
 * The event may be a mouse event (click, double-click, middle-click) or keypress event (enter).
 *
 * On Windows, the modifiers are:
 * Ctrl        new tab, selected
 * Shift       new window
 * Ctrl+Shift  new tab, in background
 * Alt         save
 *
 * You can swap Ctrl and Ctrl+shift by toggling the hidden pref
 * browser.tabs.loadBookmarksInBackground (not browser.tabs.loadInBackground, which
 * is for content area links).
 *
 * Middle-clicking is the same as Ctrl+clicking (it opens a new tab) and it is
 * subject to the shift modifier and pref in the same way.
 *
 * Exceptions: 
 * - Alt is ignored for menu items selected using the keyboard so you don't accidentally save stuff.  
 *    (Currently, the Alt isn't sent here at all for menu items, but that will change in bug 126189.)
 * - Alt is hard to use in context menus, because pressing Alt closes the menu.
 * - Alt can't be used on the bookmarks toolbar because Alt is used for "treat this as something draggable".
 * - The button is ignored for the middle-click-paste-URL feature, since it's always a middle-click.
 */
function whereToOpenLink( e, ignoreButton, ignoreAlt )
{
  if (!e)
    e = { shiftKey:false, ctrlKey:false, metaKey:false, altKey:false, button:0 };

  var shift = e.shiftKey;
  var ctrl =  e.ctrlKey;
  var meta =  e.metaKey;
  var alt  =  e.altKey && !ignoreAlt;

  // ignoreButton allows "middle-click paste" to use function without always opening in a new window.
  var middle = !ignoreButton && e.button == 1;
  var middleUsesTabs = getBoolPref("browser.tabs.opentabfor.middleclick", true);

  // Don't do anything special with right-mouse clicks.  They're probably clicks on context menu items.
  var sysInfo =
    Components.classes["@mozilla.org/system-info;1"]
              .getService(Components.interfaces.nsIPropertyBag2);
  var os = sysInfo.getProperty("name");

  var modifier = (os == "Darwin") ? meta : ctrl;
  if (modifier || (middle && middleUsesTabs)) {
    if (shift)
      return "tabshifted";
    else
      return "tab";
  }
  else if (alt) {
    return "save";
  }
  else if (shift || (middle && !middleUsesTabs)) {
    return "window";
  }
  else {
    return "current";
  }
}

/* openUILinkIn opens a URL in a place specified by the parameter |where|.
 *
 * |where| can be:
 *  "current"     current tab            (if there aren't any browser windows, then in a new window instead)
 *  "tab"         new tab                (if there aren't any browser windows, then in a new window instead)
 *  "tabshifted"  same as "tab" but in background if default is to select new tabs, and vice versa
 *  "window"      new window
 *  "save"        save to disk (with no filename hint!)
 *
 * allowThirdPartyFixup controls whether third party services such as Google's
 * I Feel Lucky are allowed to interpret this URL. This parameter may be
 * undefined, which is treated as false.
 */
function openUILinkIn( url, where, allowThirdPartyFixup, postData, referrerUrl )
{
  if (!where || !url)
    return;

  if (where == "save") {
    saveURL(url, null, null, true, null, referrerUrl);
    return;
  }


  var w = getTopWin();
  //if (!w || where == "window") {
  //  openDialog(getBrowserURL(), "_blank", "chrome,all,dialog=no", url,
  //             null, referrerUrl, postData, allowThirdPartyFixup);
  //  return;
  //}
  if (where == "window") {
    dump("browserUtilities.js.openUILinkIn() Warning: Songbird does not support where=window.\n");
  }


  var loadInBackground = getBoolPref("browser.tabs.loadBookmarksInBackground", false);
 
  var browser = w.gBrowser;
 
  // Make sure we have a browser to talk to
  // TODO: Look for other windows?
  if (typeof(browser) == "undefined") {
    dump("\n\n\nbrowserUtilities.openUILinkIn() Error: no browser available!\n\n\n");  
    return;
  }

  // Songbird defaults to tab
  switch (where) {
  case "current":
    browser.loadURI(url, referrerUrl, postData, allowThirdPartyFixup);
    break;
  case "tabshifted":
    loadInBackground = !loadInBackground;
    // fall through
  case "tab":
  default:
    browser.loadOneTab(url, referrerUrl, null, postData, loadInBackground,
                       allowThirdPartyFixup || false);
    break;
  }

  // Call focusElement(w.content) instead of w.content.focus() to make sure
  // that we don't raise the old window, since the URI we just loaded may have
  // resulted in a new frontmost window (e.g. "javascript:window.open('');").
  focusElement(w.content);
}

// Used as an onclick handler for UI elements with link-like behavior.
// e.g. onclick="checkForMiddleClick(this, event);"
function checkForMiddleClick(node, event)
{
  // We should be using the disabled property here instead of the attribute,
  // but some elements that this function is used with don't support it (e.g.
  // menuitem).
  if (node.getAttribute("disabled") == "true")
    return; // Do nothing

  if (event.button == 1) {
    /* Execute the node's oncommand.
     *
     * XXX: we should use node.oncommand(event) once bug 246720 is fixed.
     */
    var fn = new Function("event", node.getAttribute("oncommand"));
    fn.call(node, event);

    // If the middle-click was on part of a menu, close the menu.
    // (Menus close automatically with left-click but not with middle-click.)
    closeMenus(event.target);
  }
}

// Closes all popups that are ancestors of the node.
function closeMenus(node)
{
  if ("tagName" in node) {
    if (node.namespaceURI == "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul"
    && (node.tagName == "menupopup" || node.tagName == "popup"))
      node.hidePopup();

    closeMenus(node.parentNode);
  }
}

// Gather all descendent text under given document node.
function gatherTextUnder ( root ) 
{
  var text = "";
  var node = root.firstChild;
  var depth = 1;
  while ( node && depth > 0 ) {
    // See if this node is text.
    if ( node.nodeType == Node.TEXT_NODE ) {
      // Add this text to our collection.
      text += " " + node.data;
    } else if ( node instanceof HTMLImageElement) {
      // If it has an alt= attribute, use that.
      var altText = node.getAttribute( "alt" );
      if ( altText && altText != "" ) {
        text = altText;
        break;
      }
    }
    // Find next node to test.
    // First, see if this node has children.
    if ( node.hasChildNodes() ) {
      // Go to first child.
      node = node.firstChild;
      depth++;
    } else {
      // No children, try next sibling.
      if ( node.nextSibling ) {
        node = node.nextSibling;
      } else {
        // Last resort is our next oldest uncle/aunt.
        node = node.parentNode.nextSibling;
        depth--;
      }
    }
  }
  // Strip leading whitespace.
  text = text.replace( /^\s+/, "" );
  // Strip trailing whitespace.
  text = text.replace( /\s+$/, "" );
  // Compress remaining whitespace.
  text = text.replace( /\s+/g, " " );
  return text;
}

function getShellService()
{
  var shell = null;
  try {
    shell = Components.classes["@mozilla.org/browser/shell-service;1"]
      .getService(Components.interfaces.nsIShellService);
  } catch (e) {dump("*** e = " + e + "\n");}
  return shell;
}

function isBidiEnabled() {
  var rv = false;

  try {
    var localeService = Components.classes["@mozilla.org/intl/nslocaleservice;1"]
                                  .getService(Components.interfaces.nsILocaleService);
    var systemLocale = localeService.getSystemLocale().getCategory("NSILOCALE_CTYPE").substr(0,3);

    switch (systemLocale) {
      case "ar-":
      case "he-":
      case "fa-":
      case "ur-":
      case "syr":
        rv = true;
    }
  } catch (e) {}

  // check the overriding pref
  if (!rv)
    rv = getBoolPref("bidi.browser.ui");

  return rv;
}


function openAboutDialog()
{
  // *sigh*
  About();
}

function openPreferences(paneID)
{
  SBOpenPreferences(paneID);
}

/**
 * Opens the release notes page for this version of the application.
 * @param   event
 *          The DOM Event that caused this function to be called, used to
 *          determine where the release notes page should be displayed based
 *          on modifiers (e.g. Ctrl = new tab)
 */
function openReleaseNotes(event)
{
  throw("browserUtilities.js: openReleaseNotes() is not implemented in Songbird!");
}
  
/**
 * Opens the update manager and checks for updates to the application.
 */
function checkForUpdates()
{
  var um = 
      Components.classes["@mozilla.org/updates/update-manager;1"].
      getService(Components.interfaces.nsIUpdateManager);
  var prompter = 
      Components.classes["@mozilla.org/updates/update-prompt;1"].
      createInstance(Components.interfaces.nsIUpdatePrompt);

  // If there's an update ready to be applied, show the "Update Downloaded"
  // UI instead and let the user know they have to restart the browser for
  // the changes to be applied. 
  if (um.activeUpdate && um.activeUpdate.state == "pending")
    prompter.showUpdateDownloaded(um.activeUpdate);
  else
    prompter.checkForUpdates();
}

function isElementVisible(aElement)
{
  // * When an element is hidden, the width and height of its boxObject
  //   are set to 0
  // * css-visibility (unlike css-display) is inherited.
  var bo = aElement.boxObject;
  return (bo.height != 0 && bo.width != 0 &&
          document.defaultView
                  .getComputedStyle(aElement, null).visibility == "visible");
}

function getBrowserFromContentWindow(aContentWindow)
{
  var browsers = gBrowser.browsers;
  for (var i = 0; i < browsers.length; i++) {
    if (browsers[i].contentWindow == aContentWindow)
      return browsers[i];
  }
  return null;
}


/**
 * openNewTabWith: opens a new tab with the given URL.
 *
 * @param aURL
 *        The URL to open (as a string).
 * @param aDocument
 *        The document from which the URL came, or null. This is used to set the
 *        referrer header and to do a security check of whether the document is
 *        allowed to reference the URL. If null, there will be no referrer
 *        header and no security check.
 * @param aPostData
 *        Form POST data, or null.
 * @param aEvent
 *        The triggering event (for the purpose of determining whether to open
 *        in the background), or null.
 * @param aAllowThirdPartyFixup
 *        If true, then we allow the URL text to be sent to third party services
 *        (e.g., Google's I Feel Lucky) for interpretation. This parameter may
 *        be undefined in which case it is treated as false.
 */ 
function openNewTabWith(aURL, aDocument, aPostData, aEvent,
                        aAllowThirdPartyFixup)
{
  if (aDocument)
    urlSecurityCheck(aURL, aDocument.nodePrincipal);

  var prefSvc = Components.classes["@mozilla.org/preferences-service;1"]
                          .getService(Components.interfaces.nsIPrefService);
  prefSvc = prefSvc.getBranch(null);

  // should we open it in a new tab?
  var loadInBackground = true;
  try {
    loadInBackground = prefSvc.getBoolPref("browser.tabs.loadInBackground");
  }
  catch(ex) {
  }

  if (aEvent && aEvent.shiftKey)
    loadInBackground = !loadInBackground;

  // TODO: Does Songbird need this?
 
  // As in openNewWindowWith(), we want to pass the charset of the
  // current document over to a new tab. 
  //var wintype = document.documentElement.getAttribute("windowtype");
  var originCharset;
  //if (wintype == "navigator:browser")
  //  originCharset = window.content.document.characterSet;

  // open link in new tab
  var referrerURI = aDocument ? aDocument.documentURIObject : null;
  if (typeof(gBrowser) == "undefined") {
    dump("\n\n\nbrowserUtilities.openNewTabWith() Error: no browser available!\n\n\n");  
    return;
  }
  gBrowser.loadOneTab(aURL, referrerURI, originCharset, aPostData,
                     loadInBackground, aAllowThirdPartyFixup || false);
}

function openNewWindowWith(aURL, aDocument, aPostData, aAllowThirdPartyFixup)
{
  dump("\n\n\nbrowserUtilities.openNewWindowWith() Warning: Songbird does not support new windows.\n\n\n");  
}


// From browser.js
function getBrowser()
{
  return window.gBrowser;
}

// From browser.js
function loadURI(uri, referrer, postData, allowThirdPartyFixup)
{
  try {
    if (postData === undefined)
      postData = null;
    var flags = Components.interfaces.nsIWebNavigation.LOAD_FLAGS_NONE;
    if (allowThirdPartyFixup) {
      flags = Components.interfaces.nsIWebNavigation.LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP;
    }
    getWebNavigation().loadURI(uri, flags, referrer, postData, null);
  } catch (e) {
  }
}

// From browser.js
function getWebNavigation()
{
  try {
    return gBrowser.selectedBrowser.webNavigation;
  } catch (e) {
    return null;
  }
}

// From browser.js
function toOpenWindowByType(inType, uri, features)
{
  const WM_CID = "@mozilla.org/appshell/window-mediator;1";
  const nsIWM = Components.interfaces.nsIWindowMediator;

  var windowManager = Components.classes[WM_CID].getService();
  var windowManagerInterface = windowManager.QueryInterface(nsIWM);
  var topWindow = windowManagerInterface.getMostRecentWindow(inType);
  var winFeatures = features ? features : "chrome,extrachrome,menubar," +
                                          "resizable,scrollbars,status," +
                                          "toolbar";

  if (topWindow)
      topWindow.focus();
  else
      window.open(uri, "_blank", winFeatures);
}

// From browser.js
function BrowserForward(aEvent, aIgnoreAlt)
{
  var where = whereToOpenLink(aEvent, false, aIgnoreAlt);

  if (where == "current") {
    try {
      getWebNavigation().goForward();
    }
    catch(ex) {
    }
  }
  else {
    var sessionHistory = getWebNavigation().sessionHistory;
    var currentIndex = sessionHistory.index;
    var entry = sessionHistory.getEntryAtIndex(currentIndex + 1, false);
    var url = entry.URI.spec;
    openUILinkIn(url, where);
  }
}

// From browser.js
function BrowserBack(aEvent, aIgnoreAlt)
{
  var where = whereToOpenLink(aEvent, false, aIgnoreAlt);

  if (where == "current") {
    try {
      getWebNavigation().goBack();
    }
    catch(ex) {
    }
  }
  else {
    var sessionHistory = getWebNavigation().sessionHistory;
    var currentIndex = sessionHistory.index;
    var entry = sessionHistory.getEntryAtIndex(currentIndex - 1, false);
    var url = entry.URI.spec;
    openUILinkIn(url, where);
  }
} 

// From browser.js
function BrowserHomeClick(aEvent)
{
  if (aEvent.button == 2) // right-click: do nothing
    return;

  var homePage = gBrowser.homePage;
  var where = whereToOpenLink(aEvent, false, true);
  var urls;

  // openUILinkIn in utilityOverlay.js doesn't handle loading multiple pages
  switch (where) {
  case "save":
    urls = homePage.split("|");
    saveURL(urls[0], null, null, true);  // only save the first page
    break;
  case "current":
    loadOneOrMoreURIs(homePage);
    break;
  case "tabshifted":
  case "tab":
  case "window": // because songbird does not support new browser windows
    urls = homePage.split("|");
    var loadInBackground = getBoolPref("browser.tabs.loadBookmarksInBackground", false);
    gBrowser.loadTabs(urls, loadInBackground);
    break;
  /* case "window":
    OpenBrowserWindow();
    break; */
  }
}

// From browser.js
function loadOneOrMoreURIs(aURIString)
{
  // This function throws for certain malformed URIs, so use exception handling
  // so that we don't disrupt startup
  try {
    gBrowser.loadTabs(aURIString.split("|"), false, true);
  } 
  catch (e) {
  }
}
 

/**
 * This is a hacked version of nsBrowserStatusHandler
 * from firefox/browser.js.
 *
 * nsBrowserStatusHandler is registered as window.XULBrowserWindow,
 * which allows the C++ browser back-end to update window status
 * and other UI.
 */
function nsBrowserStatusHandler()
{
  this.typeSniffer = 
    Components.classes["@songbirdnest.com/Songbird/Mediacore/TypeSniffer;1"]
              .createInstance(Components.interfaces.sbIMediacoreTypeSniffer);
  this.ios = Components.classes["@mozilla.org/network/io-service;1"]
                       .getService(Components.interfaces.nsIIOService);
}

nsBrowserStatusHandler.prototype =
{
  // Stored Status, Link and Loading values
  status : "",
  defaultStatus : "",
  jsStatus : "",
  jsDefaultStatus : "",
  overLink : "",
  statusText: "",
  typeSniffer: null,
  ios: null,

  QueryInterface : function(aIID)
  {
    if (aIID.equals(Ci.nsIWebProgressListener) ||
        aIID.equals(Ci.nsIWebProgressListener2) ||
        aIID.equals(Ci.nsISupportsWeakReference) ||
        aIID.equals(Ci.nsIXULBrowserWindow) ||
        aIID.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_NOINTERFACE;
  },

  setJSStatus : function(status)
  {
    this.jsStatus = status;
    this.updateStatusField();
  },

  setJSDefaultStatus : function(status)
  {
    this.jsDefaultStatus = status;
    this.updateStatusField();
  },

  setDefaultStatus : function(status)
  {
    this.defaultStatus = status;
    this.updateStatusField();
  },

  setOverLink : function(link, b)
  {
    // Encode bidirectional formatting characters.
    // (RFC 3987 sections 3.2 and 4.1 paragraph 6)
    this.overLink = link.replace(/[\u200e\u200f\u202a\u202b\u202c\u202d\u202e]/g,
                                 encodeURIComponent);
    this.updateStatusField();
  },

  updateStatusField : function()
  {
    var text = this.overLink || this.status || this.jsStatus || this.jsDefaultStatus || this.defaultStatus;

    // check the current value so we don't trigger an attribute change
    // and cause needless (slow!) UI updates
    if (this.statusText != text) {
      SBDataSetStringValue( "faceplate.status.text", text);

      if (this.overLink) {
        var uri = null;
        try {
          uri = this.ios.newURI(this.overLink, null, null);
        } catch (ex) {}

        if (uri && (this.typeSniffer.isValidMediaURL(uri) ||
                    this.typeSniffer.isValidPlaylistURL(uri))) {
          SBDataSetStringValue( "faceplate.status.type", "playable");
        } else {
          SBDataSetStringValue( "faceplate.status.type", "normal");
        }
      }

      this.statusText = text;
    }
  }
}

// initialize observers and listeners
// and give C++ access to gBrowser
window.XULBrowserWindow = new nsBrowserStatusHandler();
window.QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIWebNavigation)
      .QueryInterface(Ci.nsIDocShellTreeItem).treeOwner
      .QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIXULWindow)
      .XULBrowserWindow = window.XULBrowserWindow;


function makeURLAbsolute(aBase, aUrl)
{
  // Note:  makeURI() will throw if aUri is not a valid URI
  return makeURI(aUrl, null, makeURI(aBase)).spec;
}

function getBrowserSelection(aCharLen) {
 // selections of more than 150 characters aren't useful
 const kMaxSelectionLen = 150;
 const charLen = Math.min(aCharLen || kMaxSelectionLen, kMaxSelectionLen);

 var focusedWindow = document.commandDispatcher.focusedWindow;
 var selection = focusedWindow.getSelection().toString();

 if (selection) {
   if (selection.length > charLen) {
     // only use the first charLen important chars. see bug 221361
     var pattern = new RegExp("^(?:\\s*.){0," + charLen + "}");
     pattern.test(selection);
     selection = RegExp.lastMatch;
   }

   selection = selection.replace(/^\s+/, "")
                        .replace(/\s+$/, "")
                        .replace(/\s+/g, " ");

   if (selection.length > charLen)
     selection = selection.substr(0, charLen);
 }
 return selection;
}

function mimeTypeIsTextBased(aMimeType)
{
  return /^text\/|\+xml$/.test(aMimeType) ||
         aMimeType == "application/javascript" ||
         aMimeType == "application/javascript" ||
         aMimeType == "application/xml" ||
         aMimeType == "mozilla.application/cached-xul";
}

function BrowserReload() 
{
  const reloadFlags = nsIWebNavigation.LOAD_FLAGS_NONE;
  return BrowserReloadWithFlags(reloadFlags);
}

function BrowserReloadSkipCache()
{
  // Bypass proxy and cache.
  const reloadFlags = nsIWebNavigation.LOAD_FLAGS_BYPASS_PROXY | nsIWebNavigation.LOAD_FLAGS_BYPASS_CACHE;
  return BrowserReloadWithFlags(reloadFlags);
}

function BrowserReloadWithFlags(reloadFlags)
{
  /* First, we'll try to use the session history object to reload so
   * that framesets are handled properly. If we're in a special
   * window (such as view-source) that has no session history, fall
   * back on using the web navigation's reload method.
   */

  var webNav = getWebNavigation();
  try {
    var sh = webNav.sessionHistory;
    if (sh)
      webNav = sh.QueryInterface(nsIWebNavigation);
  } catch (e) {
  }

  try {
    webNav.reload(reloadFlags);
  } catch (e) {
  }
}

function formatURL(aFormat, aIsPref) {
  var formatter = Cc["@mozilla.org/toolkit/URLFormatterService;1"]
                    .getService(Ci.nsIURLFormatter);
  return aIsPref ? 
    formatter.formatURLPref(aFormat) : formatter.formatURL(aFormat);
}

function HandleAppCommandEvent(evt)
{
  evt.stopPropagation();
  switch (evt.command) {
  case "Back":
    gBrowser.goBack();
    break;
  case "Forward":
    gBrowser.goForward();
    break;
  case "Reload":
    BrowserReloadSkipCache();
    break;
  case "Stop":
    gBrowser.stop();
    break;
  case "Search":
    BrowserSearch.webSearch();
    break;
  case "Home":
    gBrowser.goHome();
    break;
  default:
    break;
  }
}

/**
 * Copied from Firefox's browser.js -- Original note follows:
 *
 * Content area tooltip.
 * XXX - this must move into XBL binding/equiv! Do not want to pollute
 *       browser.js with functionality that can be encapsulated into
 *       browser widget. TEMPORARY!
 *
 * NOTE: Any changes to this routine need to be mirrored in ChromeListener::FindTitleText()
 *       (located in mozilla/embedding/browser/webBrowser/nsDocShellTreeOwner.cpp)
 *       which performs the same function, but for embedded clients that
 *       don't use a XUL/JS layer. It is important that the logic of
 *       these two routines be kept more or less in sync.
 *       (pinkerton)
 **/
function FillInHTMLTooltip(tipElement)
{
  var retVal = false;
  if (tipElement.namespaceURI == "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul")
    return retVal;

  const XLinkNS = "http://www.w3.org/1999/xlink";


  var titleText = null;
  var XLinkTitleText = null;
  var direction = tipElement.ownerDocument.dir;

  while (!titleText && !XLinkTitleText && tipElement) {
    if (tipElement.nodeType == Node.ELEMENT_NODE) {
      titleText = tipElement.getAttribute("title");
      XLinkTitleText = tipElement.getAttributeNS(XLinkNS, "title");
      var defView = tipElement.ownerDocument.defaultView;
      // XXX Work around bug 350679:
      // "Tooltips can be fired in documents with no view".
      if (!defView)
        return retVal;
      direction = defView.getComputedStyle(tipElement, "")
        .getPropertyValue("direction");
    }
    tipElement = tipElement.parentNode;
  }

  var tipNode = document.getElementById("aHTMLTooltip");
  tipNode.style.direction = direction;
  
  for each (var t in [titleText, XLinkTitleText]) {
    if (t && /\S/.test(t)) {

      // Per HTML 4.01 6.2 (CDATA section), literal CRs and tabs should be
      // replaced with spaces, and LFs should be removed entirely.
      // XXX Bug 322270: We don't preserve the result of entities like &#13;,
      // which should result in a line break in the tooltip, because we can't
      // distinguish that from a literal character in the source by this point.
      t = t.replace(/[\r\t]/g, ' ');
      t = t.replace(/\n/g, '');

      tipNode.setAttribute("label", t);
      retVal = true;
    }
  }

  return retVal;
}

var gPrefService;
var gNavigatorBundle;

function onInitBrowserUtilities() {
  window.removeEventListener("load", onInitBrowserUtilities, false);
  window.addEventListener("unload", onShutdownBrowserUtilities, false);
  gNavigatorBundle = document.getElementById("bundle_browser");
  var gPrefService = Components.classes["@mozilla.org/preferences-service;1"]
                               .getService(Components.interfaces.nsIPrefBranch2);
  gBidiUI = isBidiEnabled();

  window.addEventListener("AppCommand", HandleAppCommandEvent, true);  
}

function onShutdownBrowserUtilities() {
  window.removeEventListener("unload", onShutdownBrowserUtilities, false);
  window.removeEventListener("AppCommand", HandleAppCommandEvent, true);  
}

window.addEventListener("load", onInitBrowserUtilities, false);

// Function from Mozilla's browser.js. Accel + #1 through 8 will send it to
// that tab. Accel + 9 always sends it to the last tab.
function BrowserNumberTabSelection(event, index) {
  if (index == 8) {
    index = gBrowser.tabContainer.childNodes.length - 1;
  } else if (index >= gBrowser.tabContainer.childNodes.length) {
    return;
  }
 
  var oldTab = gBrowser.selectedTab;
  var newTab = gBrowser.tabContainer.childNodes[index];
  if (newTab != oldTab) {
    gBrowser.selectedTab = newTab;
  }
 
  event.preventDefault();
  event.stopPropagation();
}

function BrowserHandleBackspace()
{
  switch (Application.prefs.getValue("browser.backspace_action", 2)) {
  case 0:
    gBrowser.goBack();
    break;
  case 1:
    goDoCommand("cmd_scrollPageUp");
    break;
  }
}

function BrowserHandleShiftBackspace()
{
  switch (Application.prefs.getValue("browser.backspace_action", 2)) {
  case 0:
    gBrowser.goForward();
    break;
  case 1:
    goDoCommand("cmd_scrollPageDown");
    break; 
  }
}

