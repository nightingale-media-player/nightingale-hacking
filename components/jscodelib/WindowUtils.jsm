/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://songbirdnest.com
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
// END SONGBIRD GPL
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

Components.utils.import("resource://app/jsmodules/SBDataRemoteUtils.jsm");
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
   *        one is not specified, the Songbird locale bundle is used.
   *
   * \param aParent             Dialog parent window.
   * \param aURL                URL of dialog chrome.
   * \param aName               Dialog name.
   * \param aOptions            Dialog options.
   * \param aInArgs             Array of arguments passed into dialog.
   * \param aOutArgs            Array of argments returned from dialog.
   * \param aLocale             Optional locale string bundle.
   */

  openModalDialog: function WindowUtils_openModalDialog(aParent,
                                                        aURL,
                                                        aName,
                                                        aOptions,
                                                        aInArgs,
                                                        aOutArgs,
                                                        aLocale) {
    // Set the dialog arguments.
    var dialogPB = null;
    if (aInArgs)
      dialogPB = this._setArgs(aInArgs, aLocale);

    // Get the options.
    var options = aOptions.split(",");

    // Add options for a modal dialog.
    options.push("modal=yes");
    options.push("resizable=no");

    // Set accessibility options.
    if (SBDataGetBoolValue("accessibility.enabled"))
      options.push("titlebar=yes");
    else
      options.push("titlebar=no");

    // Convert options back to a string.
    options = options.join(",");

    // Open the dialog.
    var prompter = Cc["@songbirdnest.com/Songbird/Prompter;1"]
                     .createInstance(Ci.sbIPrompter);
    prompter.openDialog(aParent, aURL, aName, options, dialogPB);

    // Get the dialog arguments.
    if (aOutArgs)
      this._getArgs(dialogPB, aOutArgs);
  },


  /**
   * \brief Create and return a dialog parameter block set with the arguments
   *        contained in the array specified by aArgs.  Look up string arguments
   *        as keys in the locale string bundle specified by aLocale; use the
   *        Songbird locale string bundle if aLocale is not specified.
   *
   * \param aArgs               Array of dialog arguments to set.
   * \param aLocale             Optional locale string bundle.
   */

  _setArgs: function WindowUtils__setArgs(aArgs, aLocale) {
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
          dialogPB.objects = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
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
   *        aLocale.  If aLocale is not specified, use the Songbird locale
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
      return SBFormattedString(localeKeyMatch[1], aArg.slice(1), aLocale);
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

