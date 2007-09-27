/*
	Copyright (c) 2007, POTI Inc. All Rights Reserved.

	Redistribution and use in source and binary forms, with or without 
	modification, are permitted provided that the following conditions are met:
	
	* Redistributions of source code must retain the above copyright notice, 
	this list of conditions and the following disclaimer.
	
	* Redistributions in binary form must reproduce the above copyright notice, 
	this list of conditions and the following disclaimer in the documentation 
	and/or other materials provided with the distribution.
	
	* Neither the name of POTI Inc. or Songbird nor the names of its contributors 
	may be used to endorse or promote products derived from this software without 
	specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
	A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
	CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
	EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
	PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
	PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
	LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
	NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 *
 * Helper objects to build a music store with the Songbird Web Page API
 *
 */

if(typeof MusicStore=="undefined") {
	var MusicStore={};
}

/*
 * Return true if user agent supports web page api
 */
MusicStore.webPageAPISupported = function() {
	return (typeof(songbird) == "object");
}

/*
 * Return true if user agent is Songbird
 */
MusicStore.isSongbird = function() {
	return navigator.userAgent.match(/Songbird\/([\d\.]+)(\S+)/) != null;
}

/*
 * Return Songbird version, null if not Songbird
 */
MusicStore.getVersion = function() {
	var version = navigator.userAgent.match(/Songbird\/([\d\.]+)(\S+)/);
	if (version != null) {
		return version[1];
	} else {
		return null;
	}	
}

/*
 * Return a full url based on a relative path and page location
 */
MusicStore.fullUrl = function(relative_path) {
	var pathname = window.location.pathname.toString();
	return "http://" + window.location.host + pathname.slice(0,pathname.lastIndexOf("/")+1) + relative_path;	
}

/*
 * Compute duration in microseconds from hours, minutes, seconds
 */
MusicStore.toMicroseconds = function(hours, minutes, seconds) {
	return 1000000 * (seconds + 60*minutes + 60*60*hours);
}

/*
 * Format track duration from microseconds to hours:minute:seconds
 */
MusicStore.formatDuration = function(duration) {
	seconds = duration / 1000000;
	hours = parseInt(seconds / (60*60));
	seconds = seconds - (hours * 60 * 60);
	minutes = parseInt(seconds / 60);
	seconds = seconds - (minutes * 60);
	if (seconds < 10) {
		seconds = "0" + seconds;
	}
	if (minutes < 10 && hours > 0) {
		minutes = "0" + minutes;
	}
	if (hours > 0) {
		return hours + ":" + minutes + ":" + seconds;
	} else {
		return minutes + ":" + seconds;
	}
}

/*
 * Return the download playlist from the main library
 */
MusicStore.getDownloadPlayList = function() {
	var mediaList = null;
	var listener = {
		onEnumerationBegin: function() {
			return true;
		},
		onEnumeratedItem: function(list, item) {
			mediaList = item;
			return false;
		},
		onEnumerationEnd: function() {
			return true;
		}
	};
	
	/*
	 * If the user does not grant permission for the site to look in the local library, this will raise an exception
	 *
	 * TODO: Refactor this when there is a method to retrieve the download play list
	 * http://bugzilla.songbirdnest.com/show_bug.cgi?id=4769
	 */
	try {
		songbird.mainLibrary.enumerateItemsByProperty("http://songbirdnest.com/data/1.0#mediaListName",  
				"&chrome://songbird/locale/songbird.properties#device.download", listener, 0);	
	} catch(e) {}
	
	return new MusicStore.PlayList(songbird.mainLibrary, mediaList);
};

/*
 * Create or retrieve a playlist in the site library.
 * This playlist is only accesible to this site (domain and path) and not directly visible to the end-user.
 */
MusicStore.getSitePlayList = function(name) {
	var mediaList;
	/*
	 * Specify the scope of the playlist. By default will be limited to originating domain of this page.
	 * Set the path to be accessible to everything under root.
	 */
	var library = songbird.siteLibrary("", "/");
	try {
		mediaList = library.getMediaListByName(name);
	} catch (e) {
	}
	if (mediaList == null || name == null) {
		mediaList = library.createMediaList("simple");
	} 	
	mediaList.name = name;
	return new MusicStore.PlayList(library, mediaList);
} 

/*
 * Create or retrieve a playlist in the main library.
 * This playlist will be visible to the end-user.
 */
MusicStore.getMainPlayList = function(guid, name) {
	var mediaList;
	/*
	 * Specify the main library
	 */
	var library = songbird.mainLibrary;
	try {
		/*
		 * Playlist in the main library can only be accessed by their GUID. This is because their name is not
		 * guaranteed to be unique and can be changed by the user. It's best practice to use the GUID to retrieve
		 * a specifc object from the library.
		 */
		mediaList = library.getItemByGuid(guid);
	} catch (e) {
	}
	if (mediaList == null) {
		mediaList = library.createMediaList("simple");
		mediaList.name = name;
	} 	
	return new MusicStore.PlayList(library, mediaList);	
} 

/*
 * Wrapper object to expose some convinience methods to manipulate playlists
 */
MusicStore.PlayList = function(library, mediaList) {
	this.library = library;
	/*
	 * This will prevent the scanning of meta data and overwrite values that are set programatically.
	 */
	try {
		this.library.scanMediaOnCreation = false;
	} catch(e) {
		
	}
	this.mediaList = mediaList;
}

MusicStore.PlayList.prototype.constructor = MusicStore.PlayList;

MusicStore.PlayList.prototype = {
	addColumn: function(type, key, name, buttonLabel, before, hidden, viewable, editable, nullSortType, width) {
		songbird.webPlaylist.addColumn(type, key, name, buttonLabel, before, hidden, viewable, editable, nullSortType, width); 
	},

	addTextColumn: function(key, name, width) {
		this.addColumn("text", key, name, "", "", false, true, false, 0, width);
	},

	addDateTimeColumn: function(key, name, width) {
		this.addColumn("datetime", key, name, "",  "", false, true, false,  0, width);
	},

	addUriColumn: function(key, name, width) {
		this.addColumn("uri", key, name, "", "", false, true, false,  0, width);
	},

	addNumberColumn: function(key, name, width) {
		this.addColumn("number", key, name, "", "", false, true, false,  0, width);
	},

	addButton: function(key, name, buttonLabel, width) {
		this.addColumn("button", key, name, buttonLabel, "", false, false, false,  0, width);
	},

	addImageColumn: function(key, name, width) {
		this.addColumn("image", key, name, "", "", false, true, false,  0, width);
	},
	
	addDownloadButtonColumn: function(key, name, width) {
		this.addColumn("downloadbutton", key, name, "", "", false, true, false,  0, width);
	},
		
	hideColumn: function(key) {
		songbird.webPlaylist.hideColumn(key);
	},

	showColumn: function(key) {
		songbird.webPlaylist.showColumn(key);
	},
	
	clear: function() {
	    try {
			this.mediaList.clear();
	  	} catch (e) {
	  	}					
	},
	
	destroy: function (){
	    try {
			this.mediaList.clear();
			this.library.remove(this.mediaList);							
	  	} catch (e) {
	  	}			
		this.library = null;
		this.mediaList = null;
	},
	
	/*
	 *	Add a track to the play list
	 */
	add: function (url, prop) {
		try {
			var item = this.library.createMediaItem(url);
		} catch (e) {
			throw "Can't create media item for: " + url + " (" + e + ")";
		}
		
		// Set properties
		for (var key in prop) {
			try {
				item.setProperty(key, prop[key]);
			} catch (e) {
				throw "Could not set property: " + key + "=" + prop[key];
			}
		}

		this.mediaList.add(item);		
	},
	
	/* 
	 * Makes the playlist visible
	 */
	show: function(){
		songbird.webPlaylist.mediaList = this.mediaList;		
	},
	
	/*
	 * Remove tracks from the play list based on a property and value
	 */
	remove: function(propertyKey, value) {
		var toDelete = new Array();
		var listener = {
		   onEnumerationBegin: function() {
		     return true;
		   },
		   onEnumeratedItem: function(list, item) {
			if (item.getProperty(propertyKey) == value) {
				toDelete.push(item);
			}
			return true;
		   },
		   onEnumerationEnd: function() {
		     return true;
		   }
		};
		this.mediaList.enumerateAllItems(listener, 1);    
		for (var i=0; i < toDelete.length; i++) {
			try {
		 		this.mediaList.remove(toDelete[i]);
			} catch (e) {
			}		
		}
	},
	
	setName: function(name) {
		mediaList.name = name;
	},
	
	getName: function() {
		return mediaList.name;
	},

	/*
	 * Returns true if an item has property key of a given value. Stops after first one is found.
	 */
	contains: function(propertyKey, value) {
		var found = false;
		var listener = {
		   onEnumerationBegin: function() {
		     return true;
		   },
		   onEnumeratedItem: function(list, item) {
			found = true;
			return false;
		   },
		   onEnumerationEnd: function() {
		     return true;
		   }
		};
		this.mediaList.enumerateItemsByProperty(propertyKey, value, listener, 1);    		
		return found;
	},
	
	getGUID: function() {
		return this.mediaList.guid;
	},
	
	download: function() {
		try {
			songbird.downloadList(this.mediaList);
	 	} catch(e) {		
			throw "Could not download media list (" + e + ")";
		}
	}
};




MusicStore.writeCookie = function(name, value) {
	document.cookie = name + "=" + value + "; path=/";
};

MusicStore.readCookie = function(name) {
	var cookieName = name + "=";
	var values = document.cookie.split(';');
	for (var i=0; i < values.length; i++) {
		var c = values[i];
		while (c.charAt(0) == ' ') {
			c = c.substring(1, c.length);
		}
		if (c.indexOf(cookieName) == 0) {
 			return c.substring(cookieName.length,c.length);
		}
	}
	return null;
}

MusicStore.deleteCookie = function(name) {
	writeCookie(name,"");
}

