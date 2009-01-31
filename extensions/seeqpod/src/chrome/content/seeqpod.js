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

const SEEQPOD_KEY = 'a31e98d0b885ca220c9cd6d411f3ace54486f4c6';
const SEEQPOD_URL = 'http://www.seeqpod.com/api/v0.2/'+SEEQPOD_KEY;

/**
 * Provide calls to SeeqPod API
 */
var SeeqPod = {
    /**
     * The currently active XMLHttpRequest
     */
    xhr: null,

    /**
     * Return xml of search results
     */
    search : function(term, start, maxItems, complete) {
        var req = new XMLHttpRequest();
        req.overrideMimeType('text/xml');
        req.onload = function() {
            SeeqPod.xhr = null;
            var result;
            try {
                result = req.responseXML.getElementsByTagName("trackList")[0]
                    .getElementsByTagName("track");
            } catch(e) {
                Components.utils.reportError(e);
                complete();
                return;
            }
            complete(result);
        }
        req.onerror = function() {
            SeeqPod.xhr = null;
            complete();
        }
        // SeeqPod seems to freak out on "/"
        term = term.replace('/', ' ');

        var url = SEEQPOD_URL + "/music/search/" + encodeURIComponent(term) + 
          "/" + start + "/" + maxItems;
        req.open("GET", url, true);
        this.xhr = req;
        req.send(null);
    },

    /**
     * Return an artist name if there is a mach higher than threshold or return null
     */
    spellcheck: function(term, threshold, complete) {
        var req = new XMLHttpRequest();
        req.overrideMimeType('text/xml');
        req.onload = function() {
            try {
                var results = req.responseXML
                    .getElementsByTagName("resultList")[0]
                    .getElementsByTagName("result");
                for (var i=0; i < results.length; i++) {
                    var artist = results[i].getElementsByTagName("artist")[0]
                        .textContent;
                    var score = results[i].getElementsByTagName("score")[0]
                        .textContent;
                    if (score >= threshold) {
                        complete(artist);
                        return;
                    }
                }
            } catch(e) {
                Components.utils.reportError(e);
            }
            complete(null);
        }
        req.open("GET", SEEQPOD_URL + "/music/spellcheck/" +
                encodeURIComponent(term), true);
        req.send(null);
    },


    /**
     * Cancel search
     */
    cancel: function() {
        if (this.xhr) {
            this.xhr.abort();
            this.xhr = null;
        }
    }
}
