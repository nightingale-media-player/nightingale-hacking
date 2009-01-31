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

if (typeof Library == 'undefined') {
    var Library = {};
}

/**
 * Provide methods to manipulate medialists in the library
 */
Library = {
    library : null,
    list: null,
    columnSpec: 'http://songbirdnest.com/data/1.0#ordinal 28 ' +
        'http://songbirdnest.com/data/1.0#trackName 200 ' +
        'http://songbirdnest.com/data/1.0#artistName 169 a ' +
        'http://songbirdnest.com/data/1.0#albumName 159 ' +
        'http://songbirdnest.com/data/1.0#duration 38 ' +
        'http://songbirdnest.com/data/1.0#downloadButton 83 ' +
        'http://songbirdnest.com/data/1.0#bitRate 80',
    seeqpodQuery: 'http://songbirdnest.com/seeqpod#query',
    /**
     * Initialize our library, playlist, etc.
     */
    init : function() {
	// Get the web library
        this.library = this._getWebLibrary();

	// Create our playlist if it doesn't exist
	this.list = this._retrieveOrCreateMediaList('Seeqpod',
						    'seeqpod.lastSearch');
	this.list.setProperty(SBProperties.isReadOnly, '1');

	// Create our property if it doesn't exist
	var pMgr = Cc["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
		.getService(Ci.sbIPropertyManager);
	if (!pMgr.hasProperty(Library.seeqpodQuery)) {
	    var pI = Cc["@songbirdnest.com/Songbird/Properties/Info/Text;1"]
		    .createInstance(Ci.sbITextPropertyInfo);
	    pI.id = Library.seeqpodQuery;
      pI.displayName = 'SeeqPod Query';
	    pI.userEditable = false;
	    pI.userViewable = false;
	    pMgr.addPropertyInfo(pI);
	}
    },

    /**
     * Return the web library
     */
    _getWebLibrary: function() {
        var prefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch2);
        var guid = prefs.getComplexValue("songbird.library.web", Components.interfaces.nsISupportsString);
        var libraryManager = Components.classes["@songbirdnest.com/Songbird/library/Manager;1"].getService(Components.interfaces.sbILibraryManager);
        return libraryManager.getLibrary(guid);
    },

    /**
     * Retrieve a media list from the library or create it
     */
    _retrieveOrCreateMediaList: function(name, customType) {
        var mediaList = null;
        var propertyExists = true;
        var a;

        // Try to get our list by its custom property
        try {
            a = this.library.getItemsByProperty(SBProperties.customType, customType);
        } catch (e) {
            propertyExists = false;
        }

        // Got something?
        if (propertyExists && a.length > 0) {
            mediaList = a.queryElementAt(0, Components.interfaces.sbIMediaList);
        } else {
            // Create it
            mediaList = this.library.createMediaList("simple");
            mediaList.setProperty(SBProperties.hidden, "0");
            mediaList.name = name;
            mediaList.setProperty(SBProperties.isReadOnly, "0");
            mediaList.setProperty(SBProperties.customType, customType);
        }
        return mediaList;
    },
}
