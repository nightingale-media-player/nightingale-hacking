/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://getnightingale.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
//
// Software distributed under the License is distributed
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
// END NIGHTINGALE GPL
//
*/

/**
 * \file  WindowUtils.jsm
 * \brief Javascript source for the window utility services.
 */

//------------------------------------------------------------------------------
//
// Window utility JSM configuration.
//
//------------------------------------------------------------------------------

EXPORTED_SYMBOLS = ["WindowUtils"];


//------------------------------------------------------------------------------
//
// Window utility imported services.
//
//------------------------------------------------------------------------------

Components.utils.import("resource://app/jsmodules/DOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/SBDataRemoteUtils.jsm");
Components.utils.import("resource://app/jsmodules/SBUtils.jsm");
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");


//------------------------------------------------------------------------------
//
// Window utility defs.
//
//------------------------------------------------------------------------------

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results


//------------------------------------------------------------------------------
//
// Window utility services.
//
//------------------------------------------------------------------------------

var WindowUtils = {
  /**
   * \brief Open the modal dialog specified by aURL.  The parent window of the
   *        dialog is specified by aParent.  The name of the dialog is specified
   *        by aName.  The dialog options are specified by aOptions.  Pass the
   *        arguments specified by the array aInArgs in an nsIDialogParamBlock
   *        to the dialog.  The arguments may be strings or nsISupports.
   *        Strings may be specified as locale string bundle keys.  In addition,
   *        if an array of strings is specified as an argument, the first string
   *        is the locale key for a formatted string, and the remaining strings
   *        are the format arguments.  Strings are interpreted as locale keys if
   *        they're wrapped in "&;" (e.g., "&local.string;"); otherwise, they're
   *        interpreted as literal strings.
   *        Set the value field of the objects within the array aOutArgs to the
   *        arguments returned by the dialog.
   *        A locale string bundle may be optionally specified by aLocale.  If
   *        one is not specified, the Nightingale locale bundle is used.
   *        Return true if the dialog was accepted.
   *
   * \param aParent             Dialog parent window.
   * \param aURL                URL of dialog chrome.
   * \param aName               Dialog name.
   * \param aOptions            Dialog options.
   * \param aInArgs             Array of arguments passed into dialog.
   * \param aOutArgs            Array of argments returned from dialog.
   * \param aLocale             Optional locale string bundle.
   *
   * \return                    True if dialog accepted.
   */

  openModalDialog: function WindowUtils_openModalDialog(aParent,
                                                        aURL,
                                                        aName,
                                                        aOptions,
                                                        aInArgs,
                                                        aOutArgs,
                                                        aLocale) {
    return this.openDialog(aParent,
                           aURL,
                           aName,
                           aOptions,
                           true,
                           aInArgs,
                           aOutArgs,
                           aLocale);
  },


  /**
   * \brief Open the dialog specified by aURL.  The parent window of the
   *        dialog is specified by aParent.  The name of the dialog is specified
   *        by aName.  The dialog options are specified by aOptions.  Pass the
   *        arguments specified by the array aInArgs in an nsIDialogParamBlock
   *        to the dialog.  The arguments may be strings or nsISupports.
   *        Strings may be specified as locale string bundle keys.  In addition,
   *        if an array of strings is specified as an argument, the first string
   *        is the locale key for a formatted string, and the remaining strings
   *        are the format arguments.  Strings are interpreted as locale keys if
   *        they're wrapped in "&;" (e.g., "&local.string;"); otherwise, they're
   *        interpreted as literal strings.
   *        Set the value field of the objects within the array aOutArgs to the
   *        arguments returned by the dialog.
   *        A locale string bundle may be optionally specified by aLocale.  If
   *        one is not specified, the Nightingale locale bundle is used.
   *        Return true if the dialog was accepted.
   *
   * \param aParent             Dialog parent window.
   * \param aURL                URL of dialog chrome.
   * \param aName               Dialog name.
   * \param aOptions            Dialog options.
   * \param aModal              True if the dialog should be modal
   * \param aInArgs             Array of arguments passed into dialog.
   * \param aOutArgs            Array of argments returned from dialog.
   * \param aLocale             Optional locale string bundle.
   *
   * \return                    True if dialog accepted.
   */

  openDialog: function WindowUtils_openDialog(aParent,
                                              aURL,
                                              aName,
                                              aOptions,
                                              aModal,
                                              aInArgs,
                                              aOutArgs,
                                              aLocale) {
    // Set the dialog arguments.
    var dialogPB = null;
    if (aInArgs)
      dialogPB = this._setArgs(aInArgs, aLocale);

    // Get the options.
    var options = aOptions.split(",");

    // Add options for a dialog.
    options.push("dialog");
    if (aModal) {
      options.push("modal=yes");
    }
    options.push("resizable=no");

    // Set accessibility options.
    if (SBDataGetBoolValue("accessibility.enabled"))
      options.push("titlebar=yes");
    else
      options.push("titlebar=no");

    // Convert options back to a string.
    options = options.join(",");

    // Create a dialog watcher.
    var dialogWatcher = new sbDialogWatcher();

    // Open the dialog and check for acceptance.
    var prompter = Cc["@getnightingale.com/Nightingale/Prompter;1"]
                     .createInstance(Ci.sbIPrompter);
    var window = prompter.openDialog(aParent, aURL, aName, options, dialogPB);
    var accepted = dialogWatcher.getAccepted(window);

    // Finalize the dialog watcher.
    dialogWatcher.finalize();

    // Get the dialog arguments.
    if (aOutArgs)
      this._getArgs(dialogPB, aOutArgs);

    return accepted;
  },


  /**
   * Invoke sizeToContent on the window specified by aWindow with workarounds
   * for the following bugs:
   *
   *   https://bugzilla.mozilla.org/show_bug.cgi?id=230959
   *   http://bugzilla.getnightingale.com/show_bug.cgi?id=20969.
   *
   * \param aWindow             Window to size to content.
   */

  sizeToContent: function WindowUtils_sizeToContent(aWindow) {
    // Don't resize if not needed.
    // See https://bugzilla.mozilla.org/show_bug.cgi?id=230959
    if ((aWindow.innerHeight >= 10) &&
        (aWindow.innerHeight ==
         aWindow.document.documentElement.boxObject.height)) {
      return;
    }

    // Defer resizing until after the current event completes.
    // See http://bugzilla.getnightingale.com/show_bug.cgi?id=20969.
    SBUtils.deferFunction(function() { aWindow.sizeToContent(); });
  },


  /**
   * \brief Attempt to restart the application.
   */

  restartApp: function WindowUtils_restartApp() {
    // Notify all windows that an application quit has been requested.
    var os = Cc["@mozilla.org/observer-service;1"]
               .getService(Ci.nsIObserverService);
    var cancelQuit = Cc["@mozilla.org/supports-PRBool;1"]
                       .createInstance(Ci.nsISupportsPRBool);
    os.notifyObservers(cancelQuit, "quit-application-requested", "restart");

    // Something aborted the quit process.
    if (cancelQuit.data)
      return;

    // attempt to restart
    var as = Cc["@mozilla.org/toolkit/app-startup;1"]
               .getService(Ci.nsIAppStartup);
    as.quit(Ci.nsIAppStartup.eRestart | Ci.nsIAppStartup.eAttemptQuit);
  },


  /**
   * \brief Create and return a dialog parameter block set with the arguments
   *        contained in the array specified by aArgs.  Look up string arguments
   *        as keys in the locale string bundle specified by aLocale; use the
   *        Nightingale locale string bundle if aLocale is not specified.
   *
   * \param aArgs               Array of dialog arguments to set.
   * \param aLocale             Optional locale string bundle.
   */

  _setArgs: function WindowUtils__setArgs(aArgs, aLocale) {
    // If |aArgs| is already a |nsIArray|, just use it instead.
    if (aArgs instanceof Ci.nsIArray) {
      return aArgs;
    }
    
    // Get a dialog param block.
    var dialogPB = Cc["@mozilla.org/embedcomp/dialogparam;1"]
                     .createInstance(Ci.nsIDialogParamBlock);

    /* Put arguments into param block. */
    var stringArgNum = 0;
    for (var i = 0; i < aArgs.length; i++) {
      // Get the next argument.
      var arg = aArgs[i];

      // If arg is an nsISupports, add it to the objects field.  Otherwise, add
      // it to the string list.
      if (arg instanceof Ci.nsISupports) {
        if (!dialogPB.objects) {
          dialogPB.objects = Cc["@getnightingale.com/moz/xpcom/threadsafe-array;1"]
                               .createInstance(Ci.nsIMutableArray);
        }
        dialogPB.objects.appendElement(arg, false);
      } else {
        dialogPB.SetString(stringArgNum, this._getArgString(arg, aLocale));
        stringArgNum++;
      }
    }

    return dialogPB;
  },


  /**
   * \brief Get the dialog arguments from the parameter block specified by
   *        aDialogPB and return them in the value field of the objects within
   *        the array specified by aArgs.
   *
   * \param aDialogPB           Dialog parameter block.
   * \param aArgs               Array of dialog arguments to get.
   */

  _getArgs: function WindowUtils__getArgs(aDialogPB, aArgs) {
    // Get arguments from param block.
    for (var i = 0; i < aArgs.length; i++)
        aArgs[i].value = aDialogPB.GetString(i);
  },


  /**
   * \brief Parse the argument specified by aArg and return a formatted,
   *        localized string using the locale string bundle specified by
   *        aLocale.  If aLocale is not specified, use the Nightingale locale
   *        string bundle.
   *
   * \param aArg                Argument to parse.
   * \param aLocale             Optional locale string bundle.
   *
   * \return                    Formatted, localized string.
   */

  _getArgString: function WindowUtils__getArgString(aArg, aLocale) {
    if (aArg instanceof Array) {
      var localeKeyMatch = aArg[0].match(/^&(.*);$/);
      return SBFormattedString(localeKeyMatch[1], aArg.slice(1), null, aLocale);
    }
    else {
      var localeKeyMatch = aArg.match(/^&(.*);$/);
      if (localeKeyMatch)
        return SBString(localeKeyMatch[1], null, aLocale);
      else
        return aArg;
    }
  }
};


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Dialog watcher services.
//
//   These service provide support for watching dialog windows and determining
// whether a dialog was accepted.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * Construct a dialog watcher object.
 */

function sbDialogWatcher() {
  this.initialize();
}

// Define the object.
sbDialogWatcher.prototype = {
  // Set the constructor.
  constructor: sbDialogWatcher,

  //
  // Dialog watcher fields.
  //
  //   _windowInfoList          List of information about windows being watched.
  //   _windowWatcher           Window watcher services.
  //

  _windowInfoList: null,
  _windowWatcher: null,


  //----------------------------------------------------------------------------
  //
  // Dialog watcher services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the dialog watcher services.  This is called automatically from
   * the dialog watcher constructor.
   */

  initialize: function sbDialogWatcher_initialize() {
    // Initialize the watched window info list.
    this._windowInfoList = [];

    // Register for window notifications.
    this._windowWatcher = Cc["@mozilla.org/embedcomp/window-watcher;1"]
                            .getService(Ci.nsIWindowWatcher);
    this._windowWatcher.registerNotification(this);
  },


  /**
   * Finalize the dialog watcher services.
   */

  finalize: function sbDialogWatcher_finalize() {
    // Unregister for window notifications.
    this._windowWatcher.unregisterNotification(this);

    // Remove all windows.
    this._removeAllWindows();

    // Clear object fields.
    this._windowWatcher = null;
  },


  /**
   * Return true if the dialog with the window specified by aWindow was
   * accepted.
   *
   * \param aWindow             Window for which to check for acceptance.
   *
   * \return                    True if the dialog was accepted.
   */

  getAccepted: function sbDialogWatcher_getAccepted(aWindow) {
    // Get the window info.
    var windowInfo = this._getWindowInfo(aWindow);

    return windowInfo ? windowInfo.accepted : false;
  },


  //----------------------------------------------------------------------------
  //
  // Dialog watcher nsIObserver services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle the observed event specified by aSubject, aTopic, and aData.
   *
   * \param aSubject            Event subject.
   * \param aTopic              Event topic.
   * \param aData               Event data.
   */

  observe: function sbDialogWatcher_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "domwindowopened" :
        this._addWindow(aSubject);
        break;

      case "domwindowclosed" :
        this._doWindowClosed(aSubject);
        break;

      default :
        break;
    }
  },


  //----------------------------------------------------------------------------
  //
  // Dialog watcher event services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle the load event for the window specified by aWindowInfo.
   *
   * \param aWindowInfo         Window info pertaining to the load event.
   */

  _doLoad: function sbDialogWatcher__doLoad(aWindowInfo) {
    var _this = this;
    var func;

    // Listen for accept and cancel events.
    var documentElement = aWindowInfo.window.document.documentElement;
    func = function() { _this._doAccept(aWindowInfo); };
    aWindowInfo.domEventListenerSet.add(documentElement,
                                        "dialogaccept",
                                        func,
                                        false);
    func = function() { _this._doCancel(aWindowInfo); };
    aWindowInfo.domEventListenerSet.add(documentElement,
                                        "dialogcancel",
                                        func,
                                        false);
  },


  /**
   * Handle the window closed event for the window specified by aWindow.
   *
   * \param aWindow             Window pertaining to the closed event.
   */

  _doWindowClosed: function sbDialogWatcher__doWindowClosed(aWindow) {
    // Get the window info.  Do nothing if no window info.
    var windowInfo = this._getWindowInfo(aWindow);
    if (!windowInfo)
      return;

    // Remove DOM event listeners.
    windowInfo.domEventListenerSet.removeAll();
    windowInfo.domEventListenerSet = null;
  },


  /**
   * Handle the accept event for the window specified by aWindowInfo.
   *
   * \param aWindowInfo         Window info pertaining to the accept event.
   */

  _doAccept: function sbDialogWatcher__doAccept(aWindowInfo) {
    // Set the window as accepted.
    aWindowInfo.accepted = true;
  },


  /**
   * Handle the cancel event for the window specified by aWindowInfo.
   *
   * \param aWindowInfo         Window info pertaining to the cancel event.
   */

  _doCancel: function sbDialogWatcher__doCancel(aWindowInfo) {
    // Set the window as not accepted.
    aWindowInfo.accepted = false;
  },


  //----------------------------------------------------------------------------
  //
  // Internal dialog watcher services.
  //
  //----------------------------------------------------------------------------

  /**
   * Add the window specified by aWindow to the list of windows being watched.
   *
   * \param aWindow             Window to add.
   */

  _addWindow: function sbDialogWatcher__addWindow(aWindow) {
    // Do nothing if window has already been added.
    if (this._getWindowInfo(aWindow))
      return;

    // Add window to window list.
    var windowInfo = { window: aWindow, accepted: false };
    this._windowInfoList.push(windowInfo);

    // Create a window DOM event listener set.
    windowInfo.domEventListenerSet = new DOMEventListenerSet();

    // Listen once for window load events.
    var _this = this;
    var func = function() { _this._doLoad(windowInfo); };
    windowInfo.domEventListenerSet.add(aWindow, "load", func, false, true);
  },


  /**
   * Remove all windows from the list of windows being watched.
   */

  _removeAllWindows: function sbDialogWatcher__removeAllWindows() {
    // Remove all windows.
    for (var i = 0; i < this._windowInfoList.length; i++) {
      // Get the window info.
      var windowInfo = this._windowInfoList[i];

      // Remove window DOM event listeners.
      if (windowInfo.domEventListenerSet) {
        windowInfo.domEventListenerSet.removeAll();
        windowInfo.domEventListenerSet = null;
      }
    }
    this._windowInfoList = null;
  },


  /**
   * Return the window info for the window specified by aWindow.
   *
   * \param aWindow             Window for which to return information.
   *
   * \retrurn                   Window information object.
   */

  _getWindowInfo: function sbDialogWatcher__getWindowInfo(aWindow) {
    // Find the window information.
    var windowInfo = null;
    for (var i = 0; i < this._windowInfoList.length; i++) {
      if (this._windowInfoList[i].window == aWindow) {
        windowInfo = this._windowInfoList[i];
        break;
      }
    }

    return windowInfo;
  }
};

