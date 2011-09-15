/* vim: set sw=2 : */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");

const FeedHandler = {
  NS_XLINK: "http://www.w3.org/1999/xlink",

  init: function FeedHandler_init() {
    window.removeEventListener("load", FeedHandler, false);
    getBrowser().addEventListener("DOMLinkAdded", this, false);
    getBrowser().addProgressListener(this,
                                     Ci.nsIWebProgress.NOTIFY_STATE_REQUEST |
                                     Ci.nsIWebProgress.NOTIFY_STATE_DOCUMENT);
  },

  handleEvent: function FeedHandler_handleEvent(aEvent) {
    switch(aEvent.type) {
      case "load":
        this.init();
        break;
      case "DOMLinkAdded":
        this.onLinkAdded(aEvent);
        break;
    }
  },

  onLinkAdded: function FeedHandler_onLinkAdded(aEvent) {
    /* forked from mozilla/browser/base/content/browser.js DOMLinkHandler */
    var link = aEvent.originalTarget;
    var rel = link.rel && link.rel.toLowerCase();
    if (!link || !link.ownerDocument || !rel || !link.href)
      return;

    var iconAdded = false;
    var searchAdded = false;
    var relStrings = rel.split(/\s+/);
    var rels = {};
    for (let i = 0; i < relStrings.length; i++)
      rels[relStrings[i]] = true;

    for (let relVal in rels) {
      switch (relVal) {
        case "feed":
        case "alternate":
          if (!rels.feed && rels.alternate && rels.stylesheet)
            break;

          if (this.isValidFeed(link,
                               link.ownerDocument.nodePrincipal,
                               rels.feed))
          {
            this.detectedFeed(link, link.ownerDocument);
            feedAdded = true;
          }
          break;
        case "icon":
          if (!iconAdded) {
            if (!Application.prefs.getValue("browser.chrome.site_icons", true))
              break;

            var targetDoc = link.ownerDocument;
            var uri = makeURI(link.href, targetDoc.characterSet);

            if (("isFailedIcon" in gBrowser) && gBrowser.isFailedIcon(uri))
              break;

            // Verify that the load of this icon is legal.
            // error pages can load their favicon, to be on the safe side,
            // only allow chrome:// favicons
            const aboutNeterr = /^about:neterror\?/;
            const aboutBlocked = /^about:blocked\?/;
            const aboutCert = /^about:certerror\?/;
            if (!(aboutNeterr.test(targetDoc.documentURI) ||
                  aboutBlocked.test(targetDoc.documentURI) ||
                  aboutCert.test(targetDoc.documentURI)) ||
                !uri.schemeIs("chrome")) {
              var ssm = Cc["@mozilla.org/scriptsecuritymanager;1"].
                        getService(Ci.nsIScriptSecurityManager);
              try {
                ssm.checkLoadURIWithPrincipal(targetDoc.nodePrincipal, uri,
                                              Ci.nsIScriptSecurityManager.DISALLOW_SCRIPT);
              } catch(e) {
                break;
              }
            }

            try {
              var contentPolicy = Cc["@mozilla.org/layout/content-policy;1"].
                                  getService(Ci.nsIContentPolicy);
            } catch(e) {
              break; // Refuse to load if we can't do a security check.
            }

            // Security says okay, now ask content policy
            if (contentPolicy.shouldLoad(Ci.nsIContentPolicy.TYPE_IMAGE,
                                         uri, targetDoc.documentURIObject,
                                         link, link.type, null)
                                         != Ci.nsIContentPolicy.ACCEPT)
              break;

            var browserIndex = gBrowser.getBrowserIndexForDocument(targetDoc);
            // no browser? no favicon.
            if (browserIndex == -1)
              break;

            var tab = gBrowser.mTabContainer.childNodes[browserIndex];
            gBrowser.setIcon(tab, link.href);
            iconAdded = true;
          }
          break;
        case "search":
          if (!searchAdded) {
            var type = link.type && link.type.toLowerCase();
            type = type.replace(/^\s+|\s*(?:;.*)?$/g, "");

            if (type == "application/opensearchdescription+xml" && link.title &&
                /^(?:https?|ftp):/i.test(link.href)) {
              var engine = { title: link.title, href: link.href };
              BrowserSearch.addEngine(engine, link.ownerDocument);
              searchAdded = true;
            }
          }
          break;
      }
    }
  },

  /**
   * isValidFeed: checks whether the given data represents a valid feed.
   *
   * @param  aLink
   *         An object representing a feed with title, href and type.
   * @param  aPrincipal
   *         The principal of the document, used for security check.
   * @param  aIsFeed
   *         Whether this is already a known feed or not, if true only a security
   *         check will be performed.
   * @return The content-type of the feed, or null if it's invalid
   */
  isValidFeed: function FeedHandler_isValidFeed(aLink, aPrincipal, aIsFeed) {
    /* forked from mozilla/browser/base/content/utilityOverlay.js isValidFeed */
    if (!aLink || !aPrincipal)
      return null;

    var type = aLink.type.toLowerCase().replace(/^\s+|\s*(?:;.*)?$/g, "");
    if (!aIsFeed) {
      aIsFeed = (type == "application/rss+xml" ||
                 type == "application/atom+xml");
    }

    if (aIsFeed) {
      try {
        urlSecurityCheck(aLink.href, aPrincipal,
                         Components.interfaces.nsIScriptSecurityManager.DISALLOW_INHERIT_PRINCIPAL);
        return type || "application/rss+xml";
      }
      catch(ex) {
      }
    }

    return null;
  },

  detectedFeed: function FeedHandler_detectedFeed(link, targetDoc) {
    // find which tab this is for, and set the attribute on the browser
    var browserForLink = gBrowser.getBrowserForDocument(targetDoc);
    if (!browserForLink) {
      // ignore feeds loaded in subframes (see mozbug 305472)
      return;
    }

    var feed = { href: link.href, title: link.title };

    // grab the feed and see if it's a podcast
    var ios = Cc["@mozilla.org/network/io-service;1"]
                .getService(Ci.nsIIOService);
    var uri = ios.newURI(feed.href, null, null);

    var processor = Cc["@mozilla.org/feed-processor;1"]
                      .createInstance(Ci.nsIFeedProcessor);
    processor.listener = {
      self: this,
      handleResult: function FeedHandler_handleFeedResult(aResult) {
        if (aResult.doc.title) {
          feed.title = aResult.doc.title.plainText();
        }
        feed.author = ArrayConverter.JSArray(aResult.doc.authors)
                                    .map(function(x) x.QueryInterface(Ci.nsIFeedPerson).name)
                                    .join(", ");
        feed.result = aResult;
        this.self.addFeed(feed, browserForLink);
      },
      QueryInterface: XPCOMUtils.generateQI([Ci.nsIFeedResultListener])
    };

    processor.parseAsync(null, uri);

    var channel = ios.newChannelFromURI(uri);
    channel.asyncOpen(processor, null);
  },

  addFeed: function FeedHandler_addFeed(feed, browserForLink) {
    if (!browserForLink.feeds)
      browserForLink.feeds = [];

    const iTunesNS = "http://www.itunes.com/dtds/podcast-1.0.dtd";
    let feedElem = feed.result.doc.QueryInterface(Ci.nsIFeed);
    if (!feedElem.attributes) {
      // feed has no attributes?  not valid.
      return;
    }
    let hasItunesNS = false;
    for (let i = 0; i < feedElem.attributes.length; ++i) {
      if (feedElem.attributes.getURI(i) != "http://www.w3.org/2000/xmlns/") {
        // not a xml namespace
        continue;
      }
      if (feedElem.attributes.getValue(i) == iTunesNS) {
        hasItunesNS = true;
        break;
      }
    }
    if (!hasItunesNS) {
      // not a podcast feed
      return;
    }

    browserForLink.feeds.push(feed);

    if (browserForLink == gBrowser.mCurrentBrowser) {
      var feedButton = document.getElementById("feed-button");
      if (feedButton)
        feedButton.collapsed = false;
    }
  },

  /**
   * Update the browser UI to show whether or not feeds are available when
   * a page is loaded or the user switches tabs to a page that has feeds.
   */
  updateFeeds: function() {
    var feedButton = document.getElementById("feed-button");
    if (!this._feedMenuitem)
      this._feedMenuitem = document.getElementById("subscribeToPageMenuitem");
    if (!this._feedMenupopup)
      this._feedMenupopup = document.getElementById("subscribeToPageMenupopup");

    var feeds = gBrowser.mCurrentBrowser.feeds;
    if (!feeds || feeds.length == 0) {
      if (feedButton) {
        feedButton.collapsed = true;
        feedButton.removeAttribute("feed");
      }
      /*
      this._feedMenuitem.setAttribute("disabled", "true");
      this._feedMenupopup.setAttribute("hidden", "true");
      this._feedMenuitem.removeAttribute("hidden");
      */
    } else {
      if (feedButton)
        feedButton.collapsed = false;

      if (feeds.length > 1) {
        /*
        this._feedMenuitem.setAttribute("hidden", "true");
        this._feedMenupopup.removeAttribute("hidden");
        */
        if (feedButton)
          feedButton.removeAttribute("feed");
      } else {
        if (feedButton)
          feedButton.setAttribute("feed", feeds[0].href);

        /*
        this._feedMenuitem.setAttribute("feed", feeds[0].href);
        this._feedMenuitem.removeAttribute("disabled");
        this._feedMenuitem.removeAttribute("hidden");
        this._feedMenupopup.setAttribute("hidden", "true");
        */
      }
    }
  },

  updateFeedPanel: function FeedHandler_updateFeedPanel(aEvent) {
    function getElem(s) { return document.getElementById("feed-panel-" + s);}

    if (aEvent.target != document.getElementById("feed-panel")) {
      // this is triggered by the inner menulist showing; ignore the event
      return;
    }

    var feeds = getBrowser().selectedBrowser.feeds || [];

    getElem("listbox").removeAllItems();
    for (let i = 0; i < feeds.length; ++i) {
      getElem("listbox").appendItem(feeds[i].title, i, feeds[i].href);
    }
    getElem("listbox").collapsed = (feeds.length <= 1);

    let feed = feeds[0];
    let feedElem = feed.result.doc.QueryInterface(Ci.nsIFeed);
    getElem("title").value = feedElem.title.plainText();
    getElem("author").value = feed.author;
    var fields = feedElem.fields;

    // try to find a tag that looks like the description
    getElem("description").collapsed = true;
    for each (let key in ["media:description", "itunes:summary", "description"]) {
      if (fields.hasKey(key)) {
        getElem("description").collapsed = false;
        getElem("description").textContent = fields.getProperty(key);
        break;
      }
    }
    var image = feedElem.image;
    if ((image instanceof Ci.nsIPropertyBag) && image.hasKey("url")) {
      getElem("image").setAttributeNS(this.NS_XLINK,
                                      "href",
                                      image.getProperty("url"));
      getElem("image").collapsed = false;
    } else {
      getElem("image").collapsed = true;
    }
  },

  closeFeedPanel: function FeedHandler_closeFeedPanel() {
    document.getElementById("feed-panel").hidePopup();
  },

  /*** nsIWebProgressListener ***/
  onStateChange: function FeedHandler_onStateChange(aWebProgress,
                                                    aRequest,
                                                    aStateFlags,
                                                    aStatus)
  {

  },

  onProgressChange: function FeedHanlder_onProgressChange(aWebProgress,
                                                          aRequest,
                                                          aCurSelfProgress,
                                                          aMaxSelfProgress,
                                                          aCurTotalProgress,
                                                          aMaxTotalProgress)
  {

  },

  onLocationChange: function FeedHandler_onLocationChange(aWebProgress,
                                                          aRequest,
                                                          aLocation)
  {
    this.updateFeeds();
  },

  onStatusChange: function FeedHandler_onStatusChange(aWebProgress,
                                                      aRequest,
                                                      aStatus,
                                                      aMessage)
  {
    this.updateFeeds();
  },

  onSecurityChange: function FeedHandler_onSecurityChange(aWebProgress,
                                                          aRequest,
                                                          aState)
  {

  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMEventListener,
                                         Ci.nsIWebProgressListener])

};

window.addEventListener("load", FeedHandler, false);
