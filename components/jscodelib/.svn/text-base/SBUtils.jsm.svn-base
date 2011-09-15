/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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
 * \file  SBUtils.jsm
 * \brief Javascript source for the general Songbird utilities.
 */

//------------------------------------------------------------------------------
//
// Songbird utilities configuration.
//
//------------------------------------------------------------------------------

EXPORTED_SYMBOLS = [ "SBUtils", "SBFilteredEnumerator" ];


//------------------------------------------------------------------------------
//
// Songbird utilities defs.
//
//------------------------------------------------------------------------------

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;


//------------------------------------------------------------------------------
//
// Songbird utilities imports.
//
//------------------------------------------------------------------------------

// Mozilla imports.
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");


//------------------------------------------------------------------------------
//
// Songbird utility services.
//
//------------------------------------------------------------------------------

var SBUtils = {
  /**
   * Return true if the first-run has completed.
   *
   * \return                    True if first-run has completed.
   */

  hasFirstRunCompleted: function SBUtils_hasFirstRunCompleted() {
    // Read the first-run has completed preference.
    var Application = Cc["@mozilla.org/fuel/application;1"]
                        .getService(Ci.fuelIApplication);
    var hasCompleted = Application.prefs.getValue("songbird.firstrun.check.0.3",
                                                  false);

    return hasCompleted;
  },


  /**
   *   Defer invocation of the function specified by aFunction by queueing an
   * invocation event on the current thread event queue.
   *   Use deferFunction instead of a zero-delay timer for deferring function
   * invocation.
   *
   * \param aFunction           Function for which to defer invocation.
   */

  deferFunction: function SBUtils_deferFunction(aFunction) {
    // Get the current thread.
    var threadManager = Cc["@mozilla.org/thread-manager;1"]
                            .getService(Ci.nsIThreadManager);
    var currentThread = threadManager.currentThread;

    // Queue an event to call the function.
    var runnable = { run: aFunction };
    currentThread.dispatch(runnable, Ci.nsIEventTarget.DISPATCH_NORMAL);
  },


  /**
   * Set the text for the XUL description element specified by aDescriptionElem
   * to the text specified by aText.
   *
   * \param aDescriptionElem    XUL description element for which to set text.
   * \param aText               Text to set for XUL description element.
   */

  setDescriptionText: function SBUtils_setDescriptionText(aDescriptionElem,
                                                          aText) {
    // Remove all description element children.
    while (aDescriptionElem.firstChild)
      aDescriptionElem.removeChild(aDescriptionElem.firstChild);

    // Add a text node with the text to the description element.
    var textNode = aDescriptionElem.ownerDocument.createTextNode(aText);
    aDescriptionElem.appendChild(textNode);
  }
};


//------------------------------------------------------------------------------
//
// Filtered enumerator services.
//
//   These services provide support for adding a filter to an nsIEnumerator.
//
// Example:
//
// // Return an enumerator that contains all even elements of aEnumerator.
// function GetEvenEnumerator(aEnumerator) {
//   var elementCount = 0;
//   var func = function(aElement) {
//     var isEven = (elementCount % 2) == 0 ? true : false;
//     elementCount++;
//     return isEven;
//   }
//
//   return new SBFilteredEnumerator(aEnumerator, func);
// }
//
//------------------------------------------------------------------------------

/**
 * Construct a filtered enumerator for the enumerator specified by aEnumerator
 * using the filter function specified by aFilterFunc.  The filter function is
 * provided an element from the base enumerator and should return true if the
 * element should not be filtered out.
 *
 * \param aEnumerator           Base enumerator to filter.
 * \param aFilterFunc           Filter function.
 */

function SBFilteredEnumerator(aEnumerator, aFilterFunc)
{
  this._enumerator = aEnumerator;
  this._filterFunc = aFilterFunc;
}

// Define to class.
SBFilteredEnumerator.prototype = {
  // Set the constructor.
  constructor: SBFilteredEnumerator,

  //
  // Internal object fields.
  //
  //   _enumerator              Base enumerator.
  //   _filterFunc              Filter function to apply to base enumerator.
  //   _nextElement             Next available element.
  //

  _enumerator: null,
  _filterFunc: null,
  _nextElement: null,


  //----------------------------------------------------------------------------
  //
  // nsISimpleEnumerator services.
  //
  //----------------------------------------------------------------------------

  /**
   * Called to determine whether or not the enumerator has
   * any elements that can be returned via getNext(). This method
   * is generally used to determine whether or not to initiate or
   * continue iteration over the enumerator, though it can be
   * called without subsequent getNext() calls. Does not affect
   * internal state of enumerator.
   *
   * @see getNext()
   * @return PR_TRUE if there are remaining elements in the enumerator.
   *         PR_FALSE if there are no more elements in the enumerator.
   */

  hasMoreElements: function SBFilteredEnumerator_hasMoreElements() {
    if (this._getNext())
      return true;
  },


  /**
   * Called to retrieve the next element in the enumerator. The "next"
   * element is the first element upon the first call. Must be
   * pre-ceeded by a call to hasMoreElements() which returns PR_TRUE.
   * This method is generally called within a loop to iterate over
   * the elements in the enumerator.
   *
   * @see hasMoreElements()
   * @return NS_OK if the call succeeded in returning a non-null
   *               value through the out parameter.
   *         NS_ERROR_FAILURE if there are no more elements
   *                          to enumerate.
   * @return the next element in the enumeration.
   */

  getNext: function SBFilteredEnumerator_getNext() {
    // Get the next element.  Throw an exception if none is available.
    var nextElement = this._getNext();
    if (!nextElement)
      throw Components.results.NS_ERROR_FAILURE;

    // Consume the next element and return it.
    this._nextElement = null;
    return nextElement;
  },


  //----------------------------------------------------------------------------
  //
  // nsISupports services.
  //
  //----------------------------------------------------------------------------

  QueryInterface: XPCOMUtils.generateQI([ Ci.nsISimpleEnumerator ]),


  //----------------------------------------------------------------------------
  //
  // Internal services.
  //
  //----------------------------------------------------------------------------

  /**
   * Get and return the next element that passes the filter.  Store the element
   * internally and continue returning it until it's consumed.
   *
   * \return                    Next available element.
   */

  _getNext: function SBFilteredEnumerator__getNext() {
    // If next element is already available, simply return it.
    if (this._nextElement)
      return this._nextElement;

    // Get the next element that passes the filter.
    while (this._enumerator.hasMoreElements()) {
      var nextElement = this._enumerator.getNext();
      if (this._filterFunc(nextElement)) {
        this._nextElement = nextElement;
        break;
      }
    }

    return this._nextElement;
  }
}

