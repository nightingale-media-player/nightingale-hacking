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

EXPORTED_SYMBOLS = ["SBTimer"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results

//------------------------------------------------------------------------------
//
// SBTimer class.
//
//   SBTimer objects provide a wrapper for XPCOM nsITimer objects to allow
// functions with closures to be called when the timer expires, much like
// window.setInterval.  However, SBTimer objects have the same precision as
// nsITimer and can be used from XPCOM components.
//
//------------------------------------------------------------------------------

/**
 * \brief Construct an SBTimer object with the timer type specified by aType
 *        that will expire after the delay specified in milliseconds by aDelay.
 *        Call the function specified by aFunc when the timer expires.
 *        The timer type values are the same as for nsITimer.
 *
 * \param aFunc                 Function to call upon timer expiration.
 * \param aDelay                Timer expiration delay in milliseconds.
 * \param aType                 Type of timer (see nsITimer).
 */

function SBTimer(aFunc, aDelay, aType) {
  // Create the timer XPCOM object.
  this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

  // Set up the timer object fields.
  this._timerFunc = aFunc;

  // Initialize the timer XPCOM object.
  this._timer.init(this, aDelay, aType);
}

SBTimer.prototype = {
  //
  // Timer object fields.
  //
  //   _timer                   Timer nsITimer XPCOM object.
  //   _timerFunc               Function to call upon timer expiration.
  //

  _timer: null,
  _timerFunc: null,


  //----------------------------------------------------------------------------
  //
  // Public timer functions.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Cancel the timer.
   */

  cancel: function SBTimer_cancel() {
    this._timer.cancel();
    this._timer = null;
    this._timerFunc = null;
  },


  /**
   * \brief Set the timer delay to the value specified by aDelay.
   *
   * \param aDelay              Delay in milliseconds.
   */

  setDelay: function SBTimer_setDelay(aDelay) {
    this._timer.delay = aDelay;
  },


  //----------------------------------------------------------------------------
  //
  // Timer nsIObserver functions.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Observe timer events.  Ignore arguments.
   */

  observe: function SBTimer_observe(aSubject, aTopic, aData) {
    // Handle the timer event.
    this._timerFunc.apply(null);
  }
};

