/*
Copyright (c) 2008, Pioneers of the Inevitable, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

	* Redistributions of source code must retain the above copyright notice,
	  this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.
	* Neither the name of Pioneers of the Inevitable, Songbird, nor the names
	  of its contributors may be used to endorse or promote products derived
	  from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

const SEEQPOD_PROFILE_VERSION = 1

if (typeof Seeqpod == 'undefined') {
  var Seeqpod = {};
}

/**
 * Call this to initiate a search
 */
Seeqpod.search = function(query) {
  SBDataSetStringValue('seeqpod.query', query);
  SBDataSetBoolValue('seeqpod.searchnow', true);
  var tab = gBrowser.loadURI('chrome://seeqpod/content/search.xul',
                             null, null, null, '_media');
  if (tab) {
    gBrowser.selectedTab = tab;
  }
}

/**
 * UI controller that is loaded into the main player window
 */
Seeqpod.Controller = {
  /**
   * Called when the window finishes loading
   */
  onLoad: function() {
    this._initialized = true;
    this._strings = document.getElementById("seeqpod-strings");

    // Perform extra actions the first time the extension is run, or on upgrade
    if (Application.prefs.get("extensions.seeqpod.firstrun").value ||
        Application.prefs.get("extensions.seeqpod.profileversion").value <
            SEEQPOD_PROFILE_VERSION) {
      Application.prefs.setValue("extensions.seeqpod.firstrun", false);
      Application.prefs.setValue("extensions.seeqpod.profileversion",
                                 SEEQPOD_PROFILE_VERSION);
      this._firstRunSetup();
    }

    // This allows to clean up in case we get uninstalled
    Seeqpod.uninstallObserver.register();

    var metrics = Components.classes['@songbirdnest.com/Songbird/Metrics;1']
      .getService(Ci.sbIMetrics);

    // do evil to override gSearchHandler.onSearchEvent
    var orig_onSearchEvent = gSearchHandler.onSearchEvent;
    gSearchHandler.onSearchEvent = function(event) {
      dump('onSearchEvent\n\n\n\n');
      // if it's our handler that's being called, then load our search ui
      var widget = this._getSearchEventTarget(event);
      if (widget && widget.currentEngine &&
          widget.currentEngine.name == 'SeeqPod') {
        metrics.metricsInc('seeqpod', 'searchbox', '');
        Seeqpod.search(widget.value);
        return;
      }

      // call the original handler
      orig_onSearchEvent.apply(this, [event]);
    }
  },

  /**
   * Called when the window is about to close
   */
  onUnLoad: function() {
    this._initialized = false;
  },

  /**
   * Perform extra setup the first time the extension is run
   */
  _firstRunSetup : function() {
    // add and remove service pane nodes
    ServicePane.configure();

    // update search engine
    var search = Components.classes['@mozilla.org/browser/search-service;1']
      .getService(Components.interfaces.nsIBrowserSearchService);

    // remove the old engine if it's there
    var searchEngine = search.getEngineByName('SeeqPod');
    if (searchEngine) {
      search.removeEngine(searchEngine);
    }
    var searchEngine = search.getEngineByName('Seeqpod');
    if (searchEngine) {
      search.removeEngine(searchEngine);
    }

    // add the new engine
    search.addEngine('chrome://seeqpod/content/seeqpod-search.xml',
        Components.interfaces.nsISearchEngine.DATA_XML,
        'http://www.seeqpod.com/favicon.ico', false);
    function setUpSearchEngine() {
      // make it the default after half a second
      var searchEngine = search.getEngineByName("SeeqPod");
      if (!searchEngine) {
        setTimeout(setUpSearchEngine, 500);
        return;
      }
      searchEngine.hidden = false;
      search.moveEngine(searchEngine, 1);
      gSearchHandler._previousSearchEngine = searchEngine;
    }
    setUpSearchEngine();
  }
}

/**
 * Observer shutdown events to detect if we are being uninstalled and perform clean up
 */
Seeqpod.uninstallObserver = {
    _uninstall : false,

    observe : function(subject, topic, data) {
        if (topic == "em-action-requested") {
            // Extension has been flagged to be uninstalled
            subject.QueryInterface(Components.interfaces.nsIUpdateItem);
            if (subject.id == "{ab4bf7ab-0100-b748-a82c-a0a467773cca}") {
                if (data == "item-uninstalled" || data == "item-disabled") {
                    this._uninstall = true;
                } else if (data == "item-cancel-action") {
                    this._uninstall = false;
                }
            }
        } else if (topic == "quit-application-granted") {
            // We're shutting down, so check to see if we were flagged for uninstall - if we were, then cleanup here
            if (this._uninstall) {
                // clean up the service pane
                ServicePane.removeAllNodes();

                // remove the search engine
                try {
                  var search =
                    Components.classes['@mozilla.org/browser/search-service;1']
                    .getService(Components.interfaces.nsIBrowserSearchService)
                  var searchEngine = search.getEngineByName("SeeqPod");
                   search.removeEngine(searchEngine);
                } catch (e) { }

                // clear the first-run pref
                try {
                  Application.prefs.get("extensions.seeqpod.firstrun").reset();
                } catch(e) { }

            }
            this.unregister();
        }
    },

    register : function() {
        var observerService = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
        observerService.addObserver(this, "em-action-requested", false);
        observerService.addObserver(this, "quit-application-granted", false);
    },

    unregister : function() {
        var observerService = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
        observerService.removeObserver(this, "em-action-requested");
        observerService.removeObserver(this, "quit-application-granted");
    }
}

window.addEventListener("load", function(e) { Seeqpod.Controller.onLoad(e); }, false);
