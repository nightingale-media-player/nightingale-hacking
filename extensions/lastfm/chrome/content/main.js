/*
Copyright (c) 2010, Pioneers of the Inevitable, Inc.
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

Cu.import("resource://app/jsmodules/DOMUtils.jsm");

// Make a namespace.
if (typeof LastFm == 'undefined') {
  var LastFm = {};
}

if (typeof(gBrowser) == "undefined") {
  var gBrowser = Cc['@mozilla.org/appshell/window-mediator;1']
                   .getService(Ci.nsIWindowMediator)
                   .getMostRecentWindow('Songbird:Main')
                   .window.gBrowser;
}

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
LastFm.URL_PASSWORD = 'http://www.last.fm/settings/lostpassword';


// Called when the window finishes loading
LastFm.onLoad = function() {
  // the window has finished loading
  this._strings = document.getElementById("lastfmStrings");

  // get the XPCOM service as a JS object
  this._service = Components.classes['@songbirdnest.com/lastfm;1']
                                    .getService().wrappedJSObject;
  // listen to events from our Last.fm service
  this._service.listeners.add(this);

  // get metrics service
  this.metrics = Cc['@songbirdnest.com/Songbird/Metrics;1']
    .getService(Ci.sbIMetrics);

  // get references to our pieces of ui

  // menu items
  this._menuLogin = document.getElementById('lastfmMenuLogin');
  this._menuLogout = document.getElementById('lastfmMenuLogout');
  this._menuEnableScrobbling =
    document.getElementById('lastfmMenuEnableScrobbling');

  // statusbar icon
  this._statusIcon = document.getElementById('lastfmStatusIcon');

  // the panel binding
  this._panelBinding = document.getElementById('lastfmLoginPanel');

  // the panel
  this._panel = this._getElement(this._panelBinding, 'loginPanel');
  this._tagPanel = document.getElementById('lastfmTagPanel');

  // the deck
  this._deck = this._getElement(this._panelBinding, 'loginDeck');

  // login page of the deck
  this._login = this._getElement(this._panelBinding, 'loginBox');
  // login username field
  this._username = this._getElement(this._panelBinding, 'username');
  // login password field
  this._password = this._getElement(this._panelBinding, 'password');
  // login auto sign in checkbox
  this._loginAutoLogin = this._getElement(this._panelBinding,
                                          'loginAutoLogin');
  // abort button
  this._abortButton = this._getElement(this._panelBinding,
                                       'abortButton');
  // login button
  this._loginButton = this._getElement(this._panelBinding, 'loginButton');
  // login error description 
  this._loginError = this._getElement(this._panelBinding, 'loginError');
  // new account groupbox
  this._newAccountGroupbox = this._getElement(this._panelBinding,
                                              'newAccountGroupbox');
  // signup button
  this._signupButton = this._getElement(this._panelBinding, 'signupButton');
  // forgot password link
  this._forgotpass = this._getElement(this._panelBinding, 'forgotpass');
  this._forgotpass.textContent =
         this._strings.getString('lastfm.forgotpass.label');

  // the logging-in page of the deck
  this._loggingIn = this._getElement(this._panelBinding, 'loginProgressBox');

  // the logged-in / profile page of the deck
  this._profile = this._getElement(this._panelBinding, 'profileBox');
  // profile image
  this._image = this._getElement(this._panelBinding, 'image');
  // profile real name
  this._realname = this._getElement(this._panelBinding, 'realname');
  // profile tracks
  this._tracks = this._getElement(this._panelBinding, 'profileDescription');
  // enable-scrobbling checkbox
  this._scrobble = this._getElement(this._panelBinding, 'profileCheckbox');
  // profile auto sign in checkbox
  this._profileAutoLogin = this._getElement(this._panelBinding,
                                            'profileAutoLogin');

  // Create a DOM event listener set.
  this._domEventListenerSet = new DOMEventListenerSet();

  var self = this;
  // wire up UI events for the menu items
  var onMenuLogin = function(event) {
    self.metrics.metricsInc('lastfm', 'menu', 'login');
    self.showPanel();
  };
  this._domEventListenerSet.add(this._menuLogin,
                                'command',
                                onMenuLogin,
                                false,
                                false);

  var onMenuLogout = function(event) {
    self.metrics.metricsInc('lastfm', 'menu', 'logout');
    self.showPanel();
  };
  this._domEventListenerSet.add(this._menuLogout,
                                'command',
                                onMenuLogout,
                                false,
                                false);

  var onMenuEnableScrobbling = function(event) {
    self.metrics.metricsInc('lastfm', 'menu', 'scrobble');
    self.toggleShouldScrobble();
  };
  this._domEventListenerSet.add(this._menuEnableScrobbling,
                                'command',
                                onMenuEnableScrobbling,
                                false,
                                false);

  // wire up click event for the status icon
  var onStatusIconClicked = function(event) {
    // only the left button
    if (event.button != 0) return;

    self.metrics.metricsInc('lastfm', 'icon', 'click');

    if (self._service.loggedIn) {
      // if we're logged in, toggle the scrobble state
      self.toggleShouldScrobble();
    } else {
      // otherwise show the panel
      self.showPanel();
    }
  };
  this._domEventListenerSet.add(this._statusIcon,
                                'click',
                                onStatusIconClicked,
                                false,
                                false);

  // and the contextmenu event
  var onStatusIconContextMenu = function(event) {
    self.metrics.metricsInc('lastfm', 'icon', 'context');
    self.showPanel();
  };
  this._domEventListenerSet.add(this._statusIcon,
                                'contextmenu',
                                onStatusIconContextMenu,
                                false,
                                false);

  // wire up the abort login link
  var onAbortButtonClicked = function(event) { self._panel.hidePopup(); };
  this._domEventListenerSet.add(this._abortButton,
                                'click',
                                onAbortButtonClicked,
                                false,
                                false);


  // wire up the signup link
  var onSignupButtonClicked = function(event) {
    self.loadURI(self.URL_SIGNUP, event);
  };
  this._domEventListenerSet.add(this._signupButton,
                                'click',
                                onSignupButtonClicked,
                                false,
                                false);

  // wire up the forgot password link
  var onForgotpass = function(event) {
    self.loadURI(self.URL_PASSWORD, event);
  };
  this._domEventListenerSet.add(this._forgotpass,
                                'click',
                                onForgotpass,
                                false,
                                false);

  // wire up UI events for the profile links
  var onProfileUrlClicked = function(event) {
    self.loadURI(self._service.profileurl, event);
  };
  this._domEventListenerSet.add(this._image,
                                'click',
                                onProfileUrlClicked,
                                false,
                                false);
  this._domEventListenerSet.add(this._realname,
                                'click',
                                onProfileUrlClicked,
                                false,
                                false);

  var onTracksUrlClicked = function(event) {
    self.loadURI('http://www.last.fm/user/' +
                 self._service.username + '/charts/', event);
  };
  this._domEventListenerSet.add(this._tracks,
                                'click',
                                onTracksUrlClicked,
                                false,
                                false);

  // ui events for the auto sign in checkbox
  var onAutoLoginToggled = function(event) { self.toggleAutoLogin(); };
  this._domEventListenerSet.add(this._loginAutoLogin,
                                'command',
                                onAutoLoginToggled,
                                false,
                                false);
  this._domEventListenerSet.add(this._profileAutoLogin,
                                'command',
                                onAutoLoginToggled,
                                false,
                                false);

  // ui event for the should-scrobble checkbox
  var onScrobbleToggled = function(event) { self.toggleShouldScrobble(); };
  this._domEventListenerSet.add(this._scrobble,
                                'command',
                                onScrobbleToggled,
                                false,
                                false);

  var onButtonClicked = function(event) { self._handleUIEvents(event); };
  this._domEventListenerSet.add(this._panelBinding,
                                'login-button-clicked',
                                onButtonClicked,
                                false,
                                false);
  this._domEventListenerSet.add(this._panelBinding,
                                'cancel-button-clicked',
                                onButtonClicked,
                                false,
                                false);
  this._domEventListenerSet.add(this._panelBinding,
                                'logout-button-clicked',
                                onButtonClicked,
                                false,
                                false);

  // copy the username & password out of the service into the UI
  this._username.value = this._service.username;
  this._password.value = this._service.password;

  // Initially disable the login button if no username or password value
  if (!this._username.value || !this._password.value) {
    this._loginButton.disabled = true;
  }

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

    var onFaceplateLoveClicked = function(event) {
      self.metrics.metricsInc('lastfm', 'faceplate', 'love');
      if (self._service.loveTrack && self._service.love) {
        /* if we have a loved track, then unlove */
        self._service.loveBan(null, false);
      } else {
        /* otherwise, love */
        self._service.loveBan(gMM.sequencer.currentItem, true);
      }
    };
    this._domEventListenerSet.add(this._faceplateLove,
                                  'click',
                                  onFaceplateLoveClicked,
                                  false,
                                  false);
    this._faceplate.appendChild(this._faceplateLove);

    this._faceplateBan = document.createElement('image');
    this._faceplateBan.setAttribute('id', 'lastfmFaceplateBan');
    this._faceplateBan.setAttribute('mousethrough', 'never');
    this._faceplateBan.setAttribute('tooltiptext',
        this._strings.getString('lastfm.faceplate.ban.tooltip'));

    var onFaceplateBanClicked = function(event) {
      self.metrics.metricsInc('lastfm', 'faceplate', 'ban');
      if (self._service.loveTrack && !self._service.love) {
        /* if we have a banned track, then unban */
        self._service.loveBan(null, false);
      } else {
        /* otherwise, ban */
        self._service.loveBan(gMM.sequencer.currentItem, false);
        gMM.sequencer.next();
      }
    };
    this._domEventListenerSet.add(this._faceplateBan,
                                  'click',
                                  onFaceplateBanClicked,
                                  false,
                                  false);
    this._faceplate.appendChild(this._faceplateBan);

    this._faceplateTag = document.createElement('image');
    this._faceplateTag.setAttribute('id', 'lastfmFaceplateTag');
    this._faceplateTag.setAttribute('mousethrough', 'never');
    this._faceplateTag.setAttribute('tooltiptext',
        this._strings.getString('lastfm.faceplate.tag.tooltip'));

    var onFaceplateTagClicked = function(event) {
      self.metrics.metricsInc('lastfm', 'faceplate', 'tag');
      self._tagPanel.openPopup(event.target);
      var globalTags = document.getElementById("global-tags");
      var userTags = document.getElementById("user-tags");
      // clear out the tag boxes
      while (userTags.firstChild)
        userTags.removeChild(userTags.firstChild);
      while (globalTags.firstChild)
        globalTags.removeChild(globalTags.firstChild);

      // grab the tags from the service
      for (var tag in self._service.userTags) {
        var removable = self._service.userTags[tag];
        var hbox = self.showOneMoreTag(tag, removable);
          userTags.appendChild(hbox);
      }
      for (var tag in self._service.globalTags) {
        var removable = self._service.globalTags[tag];
        var hbox = self.showOneMoreTag(tag, removable);
        globalTags.appendChild(hbox);
      }

      if (!userTags.firstChild) {
        document.getElementById("label-user-tags")
                .style.visibility = "collapse";
      }
      if (!globalTags.firstChild) {
        document.getElementById("label-global-tags")
                .style.visibility = "collapse";
      }
    };
    this._domEventListenerSet.add(this._faceplateTag,
                                  'click',
                                  onFaceplateTagClicked,
                                  false,
                                  false);
    this._faceplate.appendChild(this._faceplateTag);

    // Add a preferences observer
    this.prefs = Cc["@mozilla.org/preferences-service;1"]
                   .getService(Ci.nsIPrefService)
                   .getBranch("extensions.lastfm.")
                   .QueryInterface(Ci.nsIPrefBranch2);
    this.prefs.addObserver("", this.prefObserver, false);
  }

  // Add a listener to toggle visibility of the love/ban faceplate toolbar
  Cc['@songbirdnest.com/Songbird/Mediacore/Manager;1']
    .getService(Ci.sbIMediacoreManager)
    .addListener(this);

  // clear the login error message
  this.setLoginError(null);
  // update the ui with the should-auto-login state
  this.onAutoLoginChanged(this._service.autoLogin);
  // update the ui with the should-scrobble state
  this.onShouldScrobbleChanged(this._service.shouldScrobble);
  // update the ui with the logged-in state
  this.onLoggedInStateChanged();
  // update the ui with the love/ban state
  this.onLoveBan();

  // if we have a username & password then try to log in
  if (this._service.shouldAutoLogin()) {
    this._service.login();
  }

  // Attach our listener to the ShowCurrentTrack event issue by the faceplate
  this._domEventListenerSet.add(window,
                                'ShowCurrentTrack',
                                this.showCurrentTrack,
                                true,
                                false);
}

LastFm._getElement = function(aWidget, aElementID) {
  return document.getAnonymousElementByAttribute(aWidget, "sbid", aElementID);
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
  var onDelTagClicked = null;
  if (removable) {
    onDelTagClicked = function(event) {
      var tagName = event.target.id;
      dump("removing tag: " + tagName + "\n");
      self._service.removeTag(gMM.sequencer.currentItem, tagName);
      this.parentNode.parentNode.removeChild(this.parentNode);
      if (!this.parentNode.parentNode.firstChild) {
        document.getElementById("label-user-tags")
                .style.visibility = "collapse";
      }
    };

    this._domEventListenerSet.add(delTag,
                                  'click',
                                  onDelTagClicked,
                                  false,
                                  false);
    delTag.setAttribute('class', 'tag-remove');
  } else {
    onDelTagClicked = function(event) {
      var tagName = event.target.id;
      dump("adding tag: " + tagName + "\n");
      self.addThisTag(gMM.sequencer.currentItem, tagName);
    };
    this._domEventListenerSet.add(delTag,
                                  'click',
                                  onDelTagClicked,
                                  false,
                                  false);
    delTag.setAttribute('class', 'tag-add');
  }

  var label = document.createElement('label');
  label.setAttribute('value', tagName);
  label.setAttribute('class', 'tag-link');
  if (removable) {
    label.setAttribute('href', '/user/' + LastFm._service.username +
                       '/tags/' + tagName);
  } else {
    label.setAttribute('href', '/tag/' + tagName);
  }

  var onLabelClicked = function(event) {
    dump("loading: " + this.getAttribute('href'));
    gBrowser.loadOneTab("http://www.last.fm"+this.getAttribute('href'));
  };
  this._domEventListenerSet.add(label,
                                'click',
                                onLabelClicked,
                                false,
                                false);

  hbox.appendChild(label);
  return hbox;
}

LastFm.addTags = function(event) {
  // call the API to add the tags
  var textbox = event.target;
  var tagString = textbox.value;
  LastFm.addThisTag(gMM.sequencer.currentItem, tagString, function() {
    textbox.value = "";
  }, function() {});
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

    if (tagBox.firstChild) {
      document.getElementById("label-user-tags").style.visibility = "visible";
    }
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
  this.prefs.removeObserver("", this.prefObserver, false);

  // Remove DOM event listeners.
  if (this._domEventListenerSet) {
    this._domEventListenerSet.removeAll();
    this._domEventListenerSet = null;
  }

  // Add a listener to toggle visibility of the love/ban faceplate toolbar
  Cc['@songbirdnest.com/Songbird/Mediacore/Manager;1']
    .getService(Ci.sbIMediacoreManager)
    .removeListener(this);
}

LastFm.showCurrentTrack = function(e) {
  var gMM = Cc['@songbirdnest.com/Songbird/Mediacore/Manager;1']
      .getService(Ci.sbIMediacoreManager);
  var item = gMM.sequencer.currentItem;
  var artistPage =
      item.getProperty("http://www.songbirdnest.com/lastfm#artistPage");
  if (artistPage) {
    dump("artist page: " + artistPage + "\n");
    var mainWin =
      Components.classes['@mozilla.org/appshell/window-mediator;1']
      .getService(Components.interfaces.nsIWindowMediator)
      .getMostRecentWindow('Songbird:Main');
    if (mainWin && mainWin.gBrowser)
      mainWin.gBrowser.loadOneTab(artistPage);
    e.preventDefault();
    e.stopPropagation();
  }
}

LastFm.showPanel = function LastFm_showPanel() {
  this._panel.openPopup(this._statusIcon);
}


// UI event handlers
LastFm._handleUIEvents = function(aEvent) {
  switch (aEvent.type) {
    case "login-button-clicked":
      this._service.userLoggedOut = false;
      this._service.username = this._username.value;
      this._service.password = this._password.value;
      // call login, telling the service to ignore any saved sessions
      this._service.login(true);
      break;
    case "cancel-button-clicked":
      this._service.cancelLogin();
      this._service.userLoggedOut = true;
      break;
    case "logout-button-clicked":
      this.onLogoutClick(aEvent);
      break;
    default:
      break;
  }

  if (this._service.userLoggedOut) {
    this._newAccountGroupbox.removeAttribute('loggedOut');
  } else {
    this._newAccountGroupbox.setAttribute('loggedOut', 'false');
  }
}

LastFm.onLogoutClick = function(event) {
  this._service.userLoggedOut = true;
  this._service.logout();
  this.setLoginError(null);
  this.updateStatus();
}

// load an URL from an event in the panel
LastFm.loadURI = function(uri, event) {
  gBrowser.loadURI(uri, null, null, event, '_blank');
  this._panel.hidePopup();
}

LastFm.toggleAutoLogin = function() {
  this._service.autoLogin = !this._service.autoLogin;
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

// autoLogin changed
LastFm.onAutoLoginChanged = function LastFm_onAutoLoginChanged(val) {
  if (val) {
    this._loginAutoLogin.setAttribute('checked', 'true');
    this._profileAutoLogin.setAttribute('checked', 'true');
  } else {
    this._loginAutoLogin.removeAttribute('checked');
    this._profileAutoLogin.removeAttribute('checked');
  }
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
        {
          stateName = 'logged_in_libre'
        } else {
          stateName = 'logged_in';
        }
      } else {
        if (Application.prefs.getValue("extensions.lastfm.auth_url", "")
                       .indexOf("libre.fm") >= 0)
        {
          stateName = 'disabled_libre';
        } else {
          stateName = 'disabled';
        }
      }
    } else {
      stateName = 'logged_out';
    }
  }
  this.setStatusIcon(this.Icons[stateName]);
  this.setStatusTextId('lastfm.state.'+stateName.replace("_libre", ""));

  if (stateName == 'logged_in') {
    this._faceplate.removeAttribute('hidden');
  } else {
    this._faceplate.setAttribute('hidden', 'true');
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
        data == "show_radio_node")
    {
      var lastFmNode =
            Cc['@songbirdnest.com/servicepane/service;1']
              .getService(Ci.sbIServicePaneService)
              .getNodeForURL("chrome://sb-lastfm/content/tuner2.xhtml");

      if (lastFmNode != null) {
        lastFmNode.hidden = !Application.prefs.getValue(
                              "extensions.lastfm.show_radio_node", true);

        // Hide the "Radio" node if it's empty
        var radioNode = lastFmNode.parentNode;
        var enum = radioNode.childNodes;
        var visibleFlag = false;

        // Iterate through elements and check if one is visible
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
