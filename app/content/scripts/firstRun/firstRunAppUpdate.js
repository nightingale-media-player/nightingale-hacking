/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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


/**
 * \file  firstRunAppUpdate.js
 * \brief Javascript source for the first-run wizard application update service.
 *
 * This is intended to trigger an application update check during the first
 * run experience.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// First-run wizard application update widget services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

// Component manager defs.
if (!("Cc" in this))
  this["Cc"] = Components.classes;
if (!("Ci" in this))
  this["Ci"] = Components.interfaces;
if (!("Cr" in this))
  this["Cr"] = Components.results;
if (!("Cu" in this))
  this["Cu"] = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/DebugUtils.jsm");

//------------------------------------------------------------------------------
//
// First-run wizard application update widget services configuration.
//
//------------------------------------------------------------------------------

var firstRunAppUpdate = {
  /***** public getters *****/
  get updateCheckState() this.mUpdateCheckState,
  
  get update() this.mUpdate,
  set update(val) this.mUpdate = val,
  
  /***** member variables *****/
  
  /**
   * the application update service (nsIApplicationUpdateService)
   */
  mUpdateService: null,
  
  /**
   * an update checker instance (nsIUpdateChecker)
   */
  mUpdateChecker: null,
  
  /**
   * The found (and selected) update, if any
   */
  mUpdate: null,
  
  /**
   * The update string bundle (from XULRunner)
   */
  mStringBundle: null,
  
  /**
   * The update checking state
   * This takes one of the K_HAS_UPDATE_STATE_* values
   */
  mUpdateCheckState: null,
  K_HAS_UPDATE_STATE_FALSE: 0,    // Check complete, no update
  K_HAS_UPDATE_STATE_TRUE: 1,     // Check complete, has update
  K_HAS_UPDATE_STATE_UNKNOWN: 2,  // Not started check yet
  K_HAS_UPDATE_STATE_CHECKING: 3, // Check in progress
  
  /***** public functions *****/

  LOG:   DebugUtils.generateLogFunction("firstRunAppUpdate", 3),
  TRACE: DebugUtils.generateLogFunction("firstRunAppUpdate", 5),

  /**
   * String localization
   * @param key The key in the string bundle to get
   * @param args [optional] Array of parameters for formatting
   */
  getString: function firstRunAppUpdate_getString(key, args) {
    if (args) {
      return this.mStringBundle.format(key, args);
    }
    return this.mStringBundle.get(key);
  },

  /**
   * initialize the first run app update object
   */
  init: function firstRunAppUpdate_init() {
    this.TRACE(arguments.callee.name);
    this.mUpdateCheckState = this.K_HAS_UPDATE_STATE_UNKNOWN;
    this.wrappedJSObject = this;
    addEventListener("pageshow", this.onPageShow, false);
    addEventListener("unload", this.onUnload, false);
  },
  
  onUnload: function firstRunAppUpdate_onUnload(aEvent) {
    removeEventListener("unload", arguments.callee, false);
    removeEventListener("pageshow", firstRunAppUpdate.onPageShow, false);
  },
  
  /**
   * Dispatcher for individual pages getting shown
   */
  onPageShow: function firstRunAppUpdate_onPageShow(event) {
    let currentPage = document.documentElement.currentPage;
    let id = currentPage.id.replace(/_(.)/g,
                                    function(x, y) y.toUpperCase());
    if (("onPageShow_" + id) in firstRunAppUpdate) {
      firstRunAppUpdate.TRACE("on page show: " + id);
      firstRunAppUpdate["onPageShow_" + id]();
    }
  },
  
  /**
   * Various functions on page show
   */
  onPageShow_firstRunWelcomePage: function firstRunAppUpdate_WelcomePage() {
    this.TRACE(arguments.callee.name);
    let doCheck =
      Application.prefs.getValue("songbird.firstrun.check.app-update", false);
    if (!doCheck) {
      // checking for app updates on first run is disabled
      this.mUpdateCheckState = this.K_HAS_UPDATE_STATE_FALSE;
    }
    else if (this.mUpdateCheckState == this.K_HAS_UPDATE_STATE_UNKNOWN) {
      this.checkForUpdate();
    }
  },

  /**
   * Check if an update is available.  Does not display any UI.
   *
   * This is expected to be started from the welcome screen and fire off an app
   * update check in the background.
   */
  checkForUpdate: function firstRunAppUpdate_checkForUpdate() {
    this.TRACE(arguments.callee.name);
    if (!this.mUpdateService.canApplyUpdates) {
      // we can't actually apply any updates anyway :|
      this.TRACE("Can't apply updates, skipping");
      this.mUpdateCheckState = this.K_HAS_UPDATE_STATE_FALSE;
      this.onUpdateStateChange();
    }
    else {
      this.TRACE("Checking for updates...");
      this.mUpdateCheckState = this.K_HAS_UPDATE_STATE_CHECKING;
      this.mUpdateChecker.checkForUpdates(this, true);
    }
  },

  /***** XPCOM interfaces *****/
  
  /**
   * nsIUpdateCheckListener
   */
  onProgress: function firstRunAppUpdate_onProgress(aRequest,
                                                    aPosition,
                                                    aTotal)
  {
    /* ignore */
  },
  
  onCheckComplete: function firstRunAppUpdate_onCheckComplete(aRequest,
                                                              aUpdates,
                                                              aUpdateCount)
  {
    this.LOG("update check complete: " + aUpdateCount + " updates found");
    var update = this.mUpdateService.selectUpdate(aUpdates, aUpdateCount);
    if (update) {
      this.mUpdate = update;
      this.mUpdateCheckState = this.K_HAS_UPDATE_STATE_TRUE;
    }
    else {
      this.mUpdateCheckState = this.K_HAS_UPDATE_STATE_FALSE;
    }
    this.onUpdateStateChange();
  },
  
  onError: function firstRunAppUpdate_onError(aRequest, aUpdate) {
    this.LOG("update check error");
    // flip the state back to unknown so we can re-check if we end up going
    // to the check for updates page again
    this.mUpdateCheckState = this.K_HAS_UPDATE_STATE_UNKNOWN;
    // If we're on the checking page, advance
    this.onUpdateStateChange();
  },
  
  /* helper for nsIUpdateCheckListener */
  onUpdateStateChange: function firstRunAppUpdate_onUpdateStateChange() {
    let currentPage = document.documentElement.currentPage;
    if (currentPage.id != "first_run_app_update_check_page") {
      return;
    }
    this.mUpdateChecker = null;
    let nextId = null;
    if (this.mUpdateCheckState == this.K_HAS_UPDATE_STATE_TRUE) {
      this.TRACE("Update state change: found update");
      nextId = currentPage.getAttribute("next");
    }
    else {
      // no update
      this.TRACE("Update state change: no update");
      nextId = currentPage.getAttribute("next_skip");
    }
    // always use goTo rather than advance because we never want rewind() to
    // get back to this page
    document.documentElement.goTo(nextId);
  },
  
  /***** XPCOM goop *****/
  QueryInterface:XPCOMUtils.generateQI([Ci.nsIUpdateCheckListener])
};

/**
 * various lazy getters
 * they should all be documented in the object above.
 */
XPCOMUtils.defineLazyServiceGetter(firstRunAppUpdate,
                                   "mUpdateService",
                                   "@mozilla.org/updates/update-service;1",
                                   "nsIApplicationUpdateService2");
XPCOMUtils.defineLazyGetter(firstRunAppUpdate,
                            "mUpdateChecker",
                            function() {
                              return Cc["@mozilla.org/updates/update-checker;1"]
                                       .createInstance(Ci.nsIUpdateChecker);
                            });
XPCOMUtils.defineLazyGetter(firstRunAppUpdate,
                            "mStringBundle",
                            function() {
                              return new SBStringBundle("chrome://mozapps/locale/update/updates.properties");
                            });

/**
 * event listener to trigger initialization
 */
addEventListener("load",
                 function(aEvent) {
                   removeEventListener(aEvent.type, arguments.callee, false);
                   firstRunAppUpdate.init();
                 },
                 false);
