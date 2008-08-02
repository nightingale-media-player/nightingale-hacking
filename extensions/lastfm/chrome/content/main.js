// Make a namespace.
if (typeof LastFm == 'undefined') {
  var LastFm = {};
}

LastFm.Icons = {
  busy: 'chrome://lastfm/skin/busy.gif',
  disabled: 'chrome://lastfm/skin/disabled.png',
  logged_in: 'chrome://lastfm/skin/as.png',
  logged_out: 'chrome://lastfm/skin/disabled.png',
  error: 'chrome://lastfm/skin/error.png',
  login_error: 'chrome://lastfm/skin/error.png'
};

LastFm.URL_SIGNUP = 'http://www.last.fm/join/';


// Called when the window finishes loading
LastFm.onLoad = function() {
  // the window has finished loading
  this._strings = document.getElementById("lastfmStrings");

  // get the XPCOM service as a JS object
  this._service = Components.classes['@songbirdnest.com/lastfm;1']
    .getService().wrappedJSObject
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

  // the panel
  this._panel = document.getElementById('lastfmPanel');

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
            LastFm._service.loveBan(gPPS.currentGUID, true);
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
            LastFm._service.loveBan(gPPS.currentGUID, false);
          }
        }, false);
    this._faceplate.appendChild(this._faceplateBan);
  }

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
  if (this._service.username && this._service.password) {
    this._service.login();
  }
}


LastFm.onUnload = function() {
  // the window is about to close
  this._service.listeners.remove(this);
}


LastFm.showPanel = function LastFm_showPanel() {
  this._panel.openPopup(this._statusIcon);
}


// button event handlers
LastFm.onLoginClick = function(event) {
  this._service.username = this._username.value;
  this._service.password = this._password.value;
  this._service.login();
}
LastFm.onCancelClick = function(event) {
  this._service.cancelLogin();
}
LastFm.onLogoutClick = function(event) {
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
    this._menuLogin.parentNode.removeChild(this._menuLogin);
    // enable the "enable scrobbling" menu item
    this._menuEnableScrobbling.disabled = false;

    // main screen turn on
    this._deck.selectedPanel = this._profile;
  } else {
    // logged out

    // remove the "log out" menu item
    this._menuLogout.parentNode.removeChild(this._menuLogout);
    // insert the "log in" menu item
    this._menuEnableScrobbling.parentNode.insertBefore(this._menuLogin,
        this._menuEnableScrobbling);
    // disable the "enable scrobbling" menu item
    this._menuEnableScrobbling.disabled = true;

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
  this._image.setAttribute('src', this._service.avatar);
  if (this._service.realname && this._service.realname.length) {
    this._realname.textContent = this._service.realname;
  } else {
    this._realname.textContent = this._service.username;
  }
  this._tracks.textContent = this._service.playcount;
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
        stateName = 'logged_in';
      } else {
        stateName = 'disabled';
      }
    } else {
      stateName = 'logged_out';
    }
  }
  this.setStatusIcon(this.Icons[stateName]);
  this.setStatusTextId('lastfm.state.'+stateName);

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
    this._loginError.style.display = '-moz-box';
  } else {
    this._loginError.textContent = '';
    this._loginError.style.display = 'none';
  }
}

LastFm.onErrorChanged = function LastFm_onErrorChanged(aError) {
  this.setLoginError(aError);
  this.updateStatus();
}

// Love & Ban support
LastFm.onLoveBan = function LastFm_onLoveBan() {
  if (this._service.loveTrack) {
    // the current track is loved or banned
    this._faceplate.setAttribute('loveban', this._service.love?'love':'ban');
  } else {
    this._faceplate.removeAttribute('loveban');
  }
}

window.addEventListener("load", function(e) { LastFm.onLoad(e); }, false);
window.addEventListener("unload", function(e) { LastFm.onUnload(e); }, false);
