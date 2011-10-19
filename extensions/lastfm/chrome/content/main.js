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
  * Neither the name of Pioneers of the Inevitable, Nightingale, nor the names
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

// Make a namespace.
if (typeof LastFm == 'undefined') {
  var LastFm = {};
}

if (typeof(gBrowser) == "undefined")
	var gBrowser = Cc['@mozilla.org/appshell/window-mediator;1']
        .getService(Ci.nsIWindowMediator).getMostRecentWindow('Nightingale:Main')
        .window.gBrowser;

LastFm.Icons = {
  busy: 'chrome://sb-lastfm/skin/busy.png',
  disabled: 'chrome://sb-lastfm/skin/disabled.png',
  disabled_libre: 'chrome://sb-lastfm/skin/librefm_disabled.png',
  logged_in_libre: 'chrome://sb-lastfm/skin/librefm.png',
  logged_in: 'chrome://sb-lastfm/skin/as.png',
  logged_out: 'chrome://sb-lastfm/skin/disabled.png',
  error: 'chrome://sb-lastfm/skin/error.png',
  login_error: 'chrome://sb-lastfm/skin/error.png'
};

LastFm.URL_SIGNUP = 'http://www.last.fm/join/';


// Called when the window finishes loading
LastFm.onLoad = function() {
  // the window has finished loading
  this._strings = document.getElementById("lastfmStrings");

  // get the XPCOM service as a JS object
  this._service = Components.classes['@getnightingale.com/lastfm;1']
    .getService().wrappedJSObject
  // listen to events from our Last.fm service
  this._service.listeners.add(this);

  // get metrics service
  this.metrics = Cc['@getnightingale.com/Nightingale/Metrics;1']
    .getService(Ci.sbIMetrics);

  // get references to our pieces of ui

  // menu items
  this._menuLogin = document.getElementById('lastfmMenuLogin');
  this._menuLogout = document.getElementById('lastfmMenuLogout');
  this._menuEnableScrobbling =
    document.getElementById('lastfmMenuEnableScrobbling');

  // statusbar icon
  this._statusIcon = document.getElementById('lastfmStatusIcon');

  // the panel
  this._panel = document.getElementById('lastfmPanel');
  this._tagPanel = document.getElementById('lastfmTagPanel');

  // the deck
  this._deck = document.getElementById('lastfmDeck');

  // login page of the deck
  this._login = document.getElementById('lastfmLogin');
  // login username field
  this._username = document.getElementById('lastfmUsername');
  // login password field
  this._password = document.getElementById('lastfmPassword');
  // login button
  this._loginError = document.getElementById('lastfmLoginError');
  // login button
  this._loginButton = document.getElementById('lastfmLoginButton');
  // signup link
  this._signup = document.getElementById('lastfmSignup');

  // the logging-in page of the deck
  this._loggingIn = document.getElementById('lastfmLoggingIn');
  // login cancel button
  this._cancelButton = document.getElementById('lastfmCancelButton');

  // the logged-in / profile page of the deck
  this._profile = document.getElementById('lastfmProfile');
  // logout button
  this._logoutButton = document.getElementById('lastfmLogoutButton');
  // profile image
  this._image = document.getElementById('lastfmImage');
  // profile real name
  this._realname = document.getElementById('lastfmRealname');
  // profile tracks
  this._tracks = document.getElementById('lastfmTracks');
  // enable-scrobbling checkbox
  this._scrobble = document.getElementById('lastfmScrobble');
  // the currently playing element
  this._currently = document.getElementById('lastfmCurrently');

  // wire up UI events for the menu items
  this._menuLogin.addEventListener('command',
      function(event) {
        LastFm.metrics.metricsInc('lastfm', 'menu', 'login');
        LastFm.showPanel();
      }, false);
  this._menuLogout.addEventListener('command',
      function(event) {
        LastFm.metrics.metricsInc('lastfm', 'menu', 'logout');
        LastFm.onLogoutClick(event);
      }, false);
  this._menuEnableScrobbling.addEventListener('command',
      function(event) {
        LastFm.metrics.metricsInc('lastfm', 'menu', 'scrobble');
        LastFm.toggleShouldScrobble();
      }, false);

  // wire up click event for the status icon
  this._statusIcon.addEventListener('click',
      function(event) {
        // only the left button
        if (event.button != 0) return;

        LastFm.metrics.metricsInc('lastfm', 'icon', 'click');

        if (!LastFm._service.loggedIn) {
          // if we're not logged in, show the login panel
          LastFm._deck.selectedPanel = LastFm._login;
          LastFm.showPanel();
        } else {
          // otherwise toggle the scrobble state
          LastFm.toggleShouldScrobble();
        }
      }, false);
  // and the contextmenu event
  this._statusIcon.addEventListener('contextmenu',
      function(event) {
        LastFm.metrics.metricsInc('lastfm', 'icon', 'context');
        LastFm.showPanel();
      }, false);

  // wire up UI events for the buttons
  this._loginButton.addEventListener('command',
      function(event) { LastFm.onLoginClick(event); }, false);
  this._cancelButton.addEventListener('command',
      function(event) { LastFm.onCancelClick(event); }, false);
  this._logoutButton.addEventListener('command',
      function(event) { LastFm.onLogoutClick(event); }, false);

  // wire up the signup link
  this._signup.addEventListener('click',
      function(event) { LastFm.loadURI(LastFm.URL_SIGNUP, event); }, false);

  // wire up UI events for the profile links
  this._image.addEventListener('click',
      function(event) { LastFm.loadURI(LastFm._service.profileurl, event); },
      false);
  this._realname.addEventListener('click',
      function(event) { LastFm.loadURI(LastFm._service.profileurl, event); },
      false);
  this._tracks.addEventListener('click',
      function(event) {
        LastFm.loadURI('http://www.last.fm/user/' +
                       LastFm._service.username + '/charts/', event);
      }, false);

  // ui event for the should-scrobble checkbox
  this._scrobble.addEventListener('command',
      function(event) { LastFm.toggleShouldScrobble(); }, false);

  // the popupshown event on the panel
  this._panel.addEventListener('popupshown',
      function(event) {
        if (LastFm._deck.selectedPanel == LastFm._login) {
          // if the login panel is showing then focus & select the username
          // field
          LastFm._username.focus();
          LastFm._username.select();
        }
      }, false);

  // copy the username & password out of the service into the UI
  this._username.value = this._service.username;
  this._password.value = this._service.password;

  // react to changes in the login form
  function loginFormChanged(event) {
    if (LastFm._username.value.length &&
        LastFm._password.value.length) {
      // we have a username & password, make sure 'login' button is enabled
      LastFm._loginButton.disabled = false;
    } else {
      // we're missing username or password, disable the 'login' button
      LastFm._loginButton.disabled = true;
    }
  }
  this._username.addEventListener('input', loginFormChanged, false);
  this._password.addEventListener('input', loginFormChanged, false);
  loginFormChanged();
  // react to keypresses
  function loginFormKeypress(event) {
    // enter or return = login, if we're not disabled
    if (event.keyCode == KeyEvent.DOM_VK_RETURN ||
        event.keyCode == KeyEvent.DOM_VK_ENTER) {
      if (!LastFm._loginButton.disabled) {
        LastFm.onLoginClick(event);
      }
    }
  }
  this._username.addEventListener('keypress', loginFormKeypress, false);
  this._password.addEventListener('keypress', loginFormKeypress, false);

  // create elements for the faceplate
  var faceplateParent = document.getElementById('faceplate-tool-bar');
  if (faceplateParent) {
    this._faceplate = document.createElement('hbox');
    this._faceplate.setAttribute('id', 'lastfmFaceplate');
    faceplateParent.insertBefore(this._faceplate, faceplateParent.firstChild);
    this._faceplateLove = document.createElement('image');
    this._faceplateLove.setAttribute('id', 'lastfmFaceplateLove');
    this._faceplateLove.setAttribute('mousethrough', 'never');
    this._faceplateLove.setAttribute('tooltiptext',
        this._strings.getString('lastfm.faceplate.love.tooltip'));
    this._faceplateLove.addEventListener('click', function(event) {
          LastFm.metrics.metricsInc('lastfm', 'faceplate', 'love');
          if (LastFm._service.loveTrack && LastFm._service.love) {
            /* if we have a loved track, then unlove */
            LastFm._service.loveBan(null, false);
          } else {
            /* otherwise, love */
            LastFm._service.loveBan(gMM.sequencer.currentItem, true);
          }
        }, false);
    this._faceplate.appendChild(this._faceplateLove);
    this._faceplateBan = document.createElement('image');
    this._faceplateBan.setAttribute('id', 'lastfmFaceplateBan');
    this._faceplateBan.setAttribute('mousethrough', 'never');
    this._faceplateBan.setAttribute('tooltiptext',
        this._strings.getString('lastfm.faceplate.ban.tooltip'));
    this._faceplateBan.addEventListener('click', function(event) {
          LastFm.metrics.metricsInc('lastfm', 'faceplate', 'ban');
          if (LastFm._service.loveTrack && !LastFm._service.love) {
            /* if we have a banned track, then unban */
            LastFm._service.loveBan(null, false);
          } else {
            /* otherwise, ban */
            LastFm._service.loveBan(gMM.sequencer.currentItem, false);
            gMM.sequencer.next();
          }
        }, false);
    this._faceplate.appendChild(this._faceplateBan);

	this._faceplateTag = document.createElement('image');
	this._faceplateTag.setAttribute('id', 'lastfmFaceplateTag');
    this._faceplateTag.setAttribute('mousethrough', 'never');
    this._faceplateTag.setAttribute('tooltiptext',
        this._strings.getString('lastfm.faceplate.tag.tooltip'));
    this._faceplateTag.addEventListener('click', function(event) {
          LastFm.metrics.metricsInc('lastfm', 'faceplate', 'tag');
		  LastFm._tagPanel.openPopup(event.target);
		  var globalTags = document.getElementById("global-tags");
		  var userTags = document.getElementById("user-tags");
		  // clear out the tag boxes
		  while (userTags.firstChild)
		  	userTags.removeChild(userTags.firstChild);
		  while (globalTags.firstChild)
		  	globalTags.removeChild(globalTags.firstChild);
		  
		  // grab the tags from the service
		  for (var tag in LastFm._service.userTags) {
			var removable = LastFm._service.userTags[tag];
			var hbox = LastFm.showOneMoreTag(tag, removable);
			userTags.appendChild(hbox);
		  }
		  for (var tag in LastFm._service.globalTags) {
			var removable = LastFm._service.globalTags[tag];
			var hbox = LastFm.showOneMoreTag(tag, removable);
			globalTags.appendChild(hbox);
		  }
		  
		  if (!userTags.firstChild)
			  document.getElementById("label-user-tags")
				  .style.visibility = "collapse";
		  if (!globalTags.firstChild)
			  document.getElementById("label-global-tags")
				  .style.visibility = "collapse";
        }, false);
    this._faceplate.appendChild(this._faceplateTag);

    // Add a preferences observer
    this.prefs = Cc["@mozilla.org/preferences-service;1"]
		.getService(Ci.nsIPrefService).getBranch("extensions.lastfm.")
		.QueryInterface(Ci.nsIPrefBranch2);
    this.prefs.addObserver("", this.prefObserver, false);
  }

  // Add a listener to toggle visibility of the love/ban faceplate toolbar
  Cc['@getnightingale.com/Nightingale/Mediacore/Manager;1']
    .getService(Ci.sbIMediacoreManager)
    .addListener(this);

  // clear the login error message
  this.setLoginError(null);
  // update the ui with the should-scrobble state
  this.onShouldScrobbleChanged(this._service.shouldScrobble);
  // update the ui with the logged-in state
  this.onLoggedInStateChanged();
  // update the ui with the love/ban state
  this.onLoveBan();
  // update the ui to match the current status
  this.updateStatus();

  // if we have a username & password then try to log in
  if (this._service.shouldAutoLogin()) {
    this._service.login();
  }

  // Attach our listener to the ShowCurrentTrack event issue by the faceplate
  window.addEventListener("ShowCurrentTrack", LastFm.showCurrentTrack, true);
}

LastFm.onMediacoreEvent = function(aEvent) {
  switch(aEvent.type) {
		case Ci.sbIMediacoreEvent.TRACK_CHANGE:
      // Hide the faceplate controls for love/ban/tag if it's not audio
      var mediaItem = aEvent.data;
      if (mediaItem.getProperty(SBProperties.contentType) != "audio") {
        document.getElementById("lastfmFaceplate").hidden = true;
      } else {
        document.getElementById("lastfmFaceplate").hidden = false;
      }
      break;
  }
}

LastFm.showOneMoreTag = function(tagName, removable) {
	var hbox = document.createElement('hbox');
	hbox.setAttribute("align", "center");
	var delTag = document.createElement('image');
	delTag.setAttribute("mousethrough", "never");
	delTag.setAttribute('id', tagName);
	hbox.appendChild(delTag);
	if (removable) {
		delTag.addEventListener('click', function(event) {
			var tagName = event.target.id;
			dump("removing tag: " + tagName + "\n");
			LastFm._service.removeTag(gMM.sequencer.currentItem, tagName);
			this.parentNode.parentNode.removeChild(this.parentNode);
			if (!this.parentNode.parentNode.firstChild)
				document.getElementById("label-user-tags")
					  .style.visibility = "collapse";
		}, false);
		delTag.setAttribute('class', 'tag-remove');
	} else {
		delTag.addEventListener('click', function(event) {
			var tagName = event.target.id;
			dump("adding tag: " + tagName + "\n");
			LastFm.addThisTag(gMM.sequencer.currentItem, tagName);
		}, false);
		delTag.setAttribute('class', 'tag-add');
}
	var label = document.createElement('label');
	label.setAttribute('value', tagName);
	label.setAttribute('class', 'tag-link');
	if (removable) 
		label.setAttribute('href', '/user/' + LastFm._service.username +
				'/tags/' + tagName);
	else
		label.setAttribute('href', '/tag/' + tagName);
	label.addEventListener('click', function(event) {
			dump("loading: " + this.getAttribute('href'));
			gBrowser.loadOneTab("http://www.last.fm"+this.getAttribute('href'));
		}, false);
	hbox.appendChild(label);
	return hbox;
}

LastFm.addTags = function(event) {
	// call the API to add the tags
	var textbox = event.target;
	var tagString = textbox.value;
	LastFm.addThisTag(gMM.sequencer.currentItem, tagString, function() {
		textbox.value = "";
	}, function() {
	});
}

LastFm.addThisTag = function(mediaItem, tagString, success, failure) {
	// need to handle adding a global tag that already exists as a user tag
	this._service.addTags(mediaItem, tagString, function() {
		// add them to the tag panel
		var tagBox = document.getElementById("user-tags");
		var tags = tagString.split(",");
		for (var i in tags) {
			var tag = tags[i].replace(/^\s*/, "").replace(/\s*$/, "");
			var hbox = LastFm.showOneMoreTag(tag, true);
			tagBox.appendChild(hbox);
			LastFm._service.userTags[tag] = true;
		}
		if (tagBox.firstChild)
		    document.getElementById("label-user-tags")
				  .style.visibility = "visible";
		if (typeof(success) == "function")
			success();
	}, function() {
		alert(LastFm._strings.getString('lastfm.tags.add_fail'));
		if (typeof(failure) == "function")
			failure();
	});
}

LastFm.onUnload = function() {
  // the window is about to close
  this._service.listeners.remove(this);
  window.removeEventListener("ShowCurrentTrack", LastFm.showCurrentTrack, true);
  this.prefs.removeObserver("", this.prefObserver, false);

  // Add a listener to toggle visibility of the love/ban faceplate toolbar
  Cc['@getnightingale.com/Nightingale/Mediacore/Manager;1']
    .getService(Ci.sbIMediacoreManager)
    .removeListener(this);
}

LastFm.showCurrentTrack = function(e) {
  var gMM = Cc['@getnightingale.com/Nightingale/Mediacore/Manager;1']
      .getService(Ci.sbIMediacoreManager);
  var item = gMM.sequencer.currentItem;
  var artistPage =
      item.getProperty("http://www.nightingalenest.com/lastfm#artistPage");
  if (artistPage) {
    dump("artist page: " + artistPage + "\n");
    var mainWin =
      Components.classes['@mozilla.org/appshell/window-mediator;1']
      .getService(Components.interfaces.nsIWindowMediator)
      .getMostRecentWindow('Nightingale:Main');
    if (mainWin && mainWin.gBrowser)
      mainWin.gBrowser.loadOneTab(artistPage);
    e.preventDefault();
    e.stopPropagation();
  }
}

LastFm.showPanel = function LastFm_showPanel() {
  this._panel.openPopup(this._statusIcon);
}


// button event handlers
LastFm.onLoginClick = function(event) {
  this._service.userLoggedOut = false;
  this._service.username = this._username.value;
  this._service.password = this._password.value;
  // call login, telling the service to ignore any saved sessions
  this._service.login(true);
}
LastFm.onCancelClick = function(event) {
  this._service.cancelLogin();
}
LastFm.onLogoutClick = function(event) {
  this._service.userLoggedOut = true;
  this._service.logout();
  this._deck.selectedPanel = this._login;
  this.setLoginError(null);
  this.updateStatus();
}

// load an URL from an event in the panel
LastFm.loadURI= function(uri, event) {
  gBrowser.loadURI(uri, null, null, event, '_blank');
  this._panel.hidePopup();
}

LastFm.toggleShouldScrobble = function() {
  this._service.shouldScrobble = !this._service.shouldScrobble;
}

// last.fm event handlers for login events
LastFm.onLoggedInStateChanged = function LastFm_onLoggedInStateChanged() {
  if (this._service.loggedIn) {
    // logged in

    // insert the "log out" menu item
    this._menuEnableScrobbling.parentNode.insertBefore(this._menuLogout,
        this._menuEnableScrobbling);
    // remove the "log in" menu item
    if (this._menuLogin.parentNode) {
        this._menuLogin.parentNode.removeChild(this._menuLogin);
    }
    // enable the "enable scrobbling" menu item
    this._menuEnableScrobbling.disabled = false;

    // main screen turn on
    this._deck.selectedPanel = this._profile;

    // show the last.fm faceplate if we're logged in 
    document.getElementById("lastfmFaceplate").hidden = false;
  } else {
    // logged out
    if (this._service.userLoggedOut) {
      // remove the "log out" menu item
      this._menuLogout.parentNode.removeChild(this._menuLogout);
      // insert the "log in" menu item
      this._menuEnableScrobbling.parentNode.insertBefore(this._menuLogin,
        this._menuEnableScrobbling);
      // disable the "enable scrobbling" menu item
      this._menuEnableScrobbling.disabled = true;
      
      // don't show the last.fm faceplate if we're logged out
      document.getElementById("lastfmFaceplate").hidden = true;
    }

    // switch back to the login panel
    this._deck.selectedPanel = this._login;
  }

  this.updateStatus();
}
LastFm.onLoginBegins = function LastFm_onLoginBegins() {
  this._deck.selectedPanel = this._loggingIn;
  this.setStatusIcon(this.Icons.busy);
  this.setStatusTextId('lastfm.state.logging_in');
}
LastFm.onLoginCancelled = function LastFm_onLoginCancelled() {
  // clear the login error
  this.setLoginError(null);

}
LastFm.onLoginFailed = function LastFm_onLoginFailed() {
  // set the login error message
  this.setLoginError(this._strings.getString('lastfm.error.login_failed'));

  // set the status icon
  this.updateStatus();
}
LastFm.onLoginSucceeded = function LastFm_onLoginSucceeded() {
  // clear the login error
  this.setLoginError(null);
}

// last.fm profile changed
LastFm.onProfileUpdated = function LastFm_onProfileUpdated() {
  var avatar = 'chrome://sb-lastfm/skin/default-avatar.png';
  if (this._service.avatar) {
    avatar = this._service.avatar;
  }
  this._image.setAttributeNS('http://www.w3.org/1999/xlink', 'href', avatar);
  if (this._service.realname && this._service.realname.length) {
    this._realname.textContent = this._service.realname;
  } else {
    this._realname.textContent = this._service.username;
  }
  this._tracks.textContent = 
      this._strings.getFormattedString('lastfm.profile.numtracks', 
          [this._service.playcount]);
}

// shouldScrobble changed
LastFm.onShouldScrobbleChanged = function LastFm_onShouldScrobbleChanged(val) {
  if (val) {
    this._menuEnableScrobbling.setAttribute('checked', 'true');
    this._scrobble.setAttribute('checked', 'true');
  } else {
    this._menuEnableScrobbling.removeAttribute('checked');
    this._scrobble.removeAttribute('checked');
  }
  this.updateStatus();
}

// userLoggedOut changed
LastFm.onUserLoggedOutChanged = function LastFm_onUserLoggedOutChanged(val) {
  // we don't care
}

// update the status icon's icon
LastFm.setStatusIcon = function LastFm_setStatusIcon(aIcon) {
  this._statusIcon.setAttribute('src', aIcon);
}

// update the status icon based on the current service state
LastFm.updateStatus = function LastFm_updateStatus() {
  var stateName = '';
  if (this._service.error) {
    if (this._service.loggedIn) {
      stateName = 'error';
    } else {
      stateName = 'login_error';
    }
  } else {
    if (this._service.loggedIn) {
      if (this._service.shouldScrobble) {
		if (Application.prefs.getValue("extensions.lastfm.auth_url", "")
				.indexOf("libre.fm") >= 0)
		  stateName = 'logged_in_libre'
		else
		  stateName = 'logged_in';
      } else {
		if (Application.prefs.getValue("extensions.lastfm.auth_url", "")
				.indexOf("libre.fm") >= 0)
          stateName = 'disabled_libre';
		else
          stateName = 'disabled';
      }
    } else {
      stateName = 'logged_out';
    }
  }
  this.setStatusIcon(this.Icons[stateName]);
  this.setStatusTextId('lastfm.state.'+stateName.replace("_libre", ""));

  if (stateName == 'logged_in') {
    this._faceplate.removeAttribute('hidden');
    // switch to profile panel
    this._deck.selectedPanel = this._profile;
  } else {
    this._faceplate.setAttribute('hidden', 'true');
    if (stateName == 'error' || stateName == 'login_error') {
      // switch back to the login panel
      this._deck.selectedPanel = this._login;
    }
  }
}

// update the status icon's text
LastFm.setStatusText = function LastFm_setStatusText(aText) {
  this._statusIcon.setAttribute('tooltiptext', aText);
}

// update the status icon's text from the properties file by id
LastFm.setStatusTextId = function LastFm_setStatusTextId(aId) {
  this.setStatusText(this._strings.getString(aId));
}

// update the login error - pass null to clear the error message
LastFm.setLoginError = function LastFm_setLoginError(aText) {
  if (aText) {
    this._loginError.textContent = aText;
    this._loginError.style.visibility = 'visible';
  } else {
    this._loginError.textContent = '';
    this._loginError.style.visibility = 'collapse';
  }
}

LastFm.onErrorChanged = function LastFm_onErrorChanged(aError) {
  this.setLoginError(aError);

  this.updateStatus();
}

// Love & Ban support
LastFm.onLoveBan = function LastFm_onLoveBan(aItem, love) {
  if (this._service.loveTrack) {
    // the current track is loved or banned
    this._faceplate.setAttribute('loveban', this._service.love?'love':'ban');
  } else {
    this._faceplate.removeAttribute('loveban');
  }
}

LastFm.onAuthorisationSuccess = function LastFm_onAuthorisationSuccess() { }

LastFm.prefObserver = {
	observe: function(subject, topic, data) {
                if (subject instanceof Components.interfaces.nsIPrefBranch &&
			data == "show_radio_node") {
			var lastFmNode = Cc['@getnightingale.com/servicepane/service;1']
				.getService(Ci.sbIServicePaneService).
				getNodeForURL("chrome://sb-lastfm/content/tuner2.xhtml");

			if (lastFmNode != null) {
				lastFmNode.hidden =
					!Application.prefs.getValue("extensions.lastfm.show_radio_node", true);
	
				// Hide the "Radio" node if it's empty
				var radioNode = lastFmNode.parentNode;
				var enum = radioNode.childNodes;
				var visibleFlag = false;

				// Iterate through elements and check if one is
				// visible
				while (enum.hasMoreElements()) {
					var node = enum.getNext();
					if (!node.hidden) {
						visibleFlag = true;
						break;
					}
				}

				radioNode.hidden = !visibleFlag;
			}
                }
	}
}

window.addEventListener("load", function(e) { LastFm.onLoad(e); }, false);
window.addEventListener("unload", function(e) { LastFm.onUnload(e); }, false);
