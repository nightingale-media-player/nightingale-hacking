/*
Copyright (c) 2008-2009, Pioneers of the Inevitable, Inc.
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

    var metrics = Components.classes['@songbirdnest.com/Songbird/Metrics;1']
      .getService(Ci.sbIMetrics);

    // do evil to override gSearchHandler.onSearchEvent
    var orig_onSearchEvent = gSearchHandler.onSearchEvent;
    gSearchHandler.onSearchEvent = function(event) {
      dump('onSearchEvent\n\n\n\n');
      // if it's our handler that's being called, then load our search ui
      var widget = this._getSearchEventTarget(event);
      if (widget && widget.currentEngine &&
          widget.currentEngine.alias == 'songbird-seeqpod-search') {
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
  }
}

window.addEventListener("load", function(e) { Seeqpod.Controller.onLoad(e); }, false);
