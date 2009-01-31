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

if (typeof Services == 'undefined') {
    var Services = {};
}

/**
 * Convinience class for services
 */
Services = {
    libraryServicePaneService : null,
    servicePaneService : null,
    bookmarksService : null,
    searchService :null,

    libraryServicePane: function() {
        if (this.libraryServicePaneService == null) {
            this.libraryServicePaneService = Components.classes['@songbirdnest.com/servicepane/library;1'].getService(Components.interfaces.sbILibraryServicePaneService);
        }
        return this.libraryServicePaneService;
    },

    servicePane: function() {
        if (this.servicePaneService == null) {
            this.servicePaneService = Components.classes['@songbirdnest.com/servicepane/service;1'].getService(Components.interfaces.sbIServicePaneService);
        }
        return this.servicePaneService;
    },

    bookmarks: function() {
        if (this.bookmarksService == null) {
            this.bookmarksService = Components.classes['@songbirdnest.com/servicepane/bookmarks;1'].getService(Components.interfaces.sbIBookmarks);
        }
        return this.bookmarksService;
    },

    search: function() {
        if (this.searchService == null) {
            this.searchService = Components.classes["@mozilla.org/browser/search-service;1"].getService(Components.interfaces.nsIBrowserSearchService);
        }
        return this.searchService;
    }
}
