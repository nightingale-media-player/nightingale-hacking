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
 * \file  GeneratorThread.jsm
 * \brief Javascript source for the generator thread services.
 */

//------------------------------------------------------------------------------
//
// Generator thread configuration.
//
//------------------------------------------------------------------------------

EXPORTED_SYMBOLS = [ "GeneratorThread" ];


//------------------------------------------------------------------------------
//
// Generator thread defs.
//
//------------------------------------------------------------------------------

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;


//------------------------------------------------------------------------------
//
// GeneratorThread class.
//
//   This class implements pseudo-threads using Javascript generators.  These
// threads provide support for cooperative multitasking within the main thread.
//   The entry point for a generator thread is a generator-iterator.  The
// generator-iterator is scheduled for execution on a periodic basis.  The
// scheduling period is set with the thread period field.
//   The thread generator-iterator may use the yield keyword to allow other
// threads to run.  In this case, no value should be provided with the yield
// keyword.
//   If the generator-iterator entry point provides another generator-iterator
// with the yield keyword, the new generator-iterator will be pushed onto a
// thread generator-iterator stack.  When the thread is scheduled for execution,
// the generator-iterator on the top of the stack will be run.
//   The generator-iterators on the stack will be run until a generator-iterator
// yields with no new generator-iterator or until the stack is empty.
//   The generator-iterator should use the yieldIfShould class method to yield
// if the thread's allocated CPU percentage has been consumed.  The maximum CPU
// percentage can be set with the thread maxPctCPU field.  The yieldIfShould
// method is a generator, so it should be used as follows:
//
//   yield GeneratorThread.yieldIfShould();
//
//   If the thread should not yield, the generator-iterator will continue to be
// run.
//
//   For more on generators, see
// http://developer.mozilla.org/en/New_in_JavaScript_1.7
//
//------------------------------------------------------------------------------

/**
 * Construct an GeneratorThread object with the thread entry point
 * generator-iterator specified by aEntryPoint.
 *
 * \param aEntryPoint           Thread entry point.
 */

function GeneratorThread(aEntryPoint) {
  // Push the entry point onto the generator-iterator stack.
  this._genIterStack = [];
  this._genIterStack.push(aEntryPoint);
}


//
// Pseudo-thread class fields.
//
//   currentThread              Currently scheduled thread.
//

GeneratorThread.currentThread = null;


/**
 * Terminate the current thread.
 */

GeneratorThread.terminateCurrentThread =
  function GeneratorThread_terminateCurrentThread() {
  if (this.currentThread)
    this.currentThread.terminate();
}


/**
 * Return true if the current thread should yield.
 *
 * \return True if current thread should yield.
 */

GeneratorThread.shouldYield = function GeneratorThread_shouldYield() {
  // Get the current thread info.
  var yieldThreshold = this.currentThread._yieldThreshold;
  var scheduleStartTime = this.currentThread._scheduleStartTime;

  // Get the current time.
  var currentTime = (new Date()).getTime();

  // Determine whether the thread should yield.
  if ((currentTime - scheduleStartTime) >= yieldThreshold)
    return true;
  return false;
}


/**
 * Yield if the current thread should yield.
 */

GeneratorThread.yieldIfShould = function GeneratorThread_yieldIfShould() {
  if (GeneratorThread.shouldYield())
    yield;
}


// Define the class instance services.
GeneratorThread.prototype = {
  // Set the constructor.
  constructor: GeneratorThread,

  //
  // Public object fields.
  //
  //   period                   Thread scheduling period in milliseconds.
  //   maxPctCPU                Maximum thread CPU execution percentage (0 -
  //                            100).
  //

  period: 50,
  maxPctCPU: 50,


  //
  // Internal object fields.
  //
  //   _genIterStack            Thread generator-iterator stack.
  //   _timer                   Thread scheduling timer.
  //   _yieldThreshold          Execution time threshold in milliseconds after
  //                            which thread should yield.
  //   _scheduleStartTime       Time when thread scheduling started.
  //

  _genIterStack: null,
  _timer: null,
  _yieldThreshold: 0,
  _scheduleStartTime: 0,


  //----------------------------------------------------------------------------
  //
  // Public services.
  //
  //----------------------------------------------------------------------------

  /**
   * Start running the thread.
   */

  start: function GeneratorThread_start() {
    // Compute the yield time threshold.
    this._yieldThreshold = (this.period * this.maxPctCPU) / 100;

    // Create a thread scheduling timer.
    this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

    // Set up to periodically run the thread.
    var _this = this;
    var func = function() { _this._run(); };
    this._timer.initWithCallback(func,
                                 this.period,
                                 Ci.nsITimer.TYPE_REPEATING_SLACK);
  },


  /**
   * Terminate the thread.
   */

  terminate: function GeneratorThread_terminate() {
    // Close all of the generators in the stack.
    var genIter;
    while (genIter = this._genIterStack.pop()) {
      genIter.close();
    }

    // Dispose of the scheduling timer.
    if (this._timer) {
      this._timer.cancel();
      this._timer = null;
    }

    // Remove object references.
    this._genIterStack = [];
  },


  //----------------------------------------------------------------------------
  //
  // Internal services.
  //
  //----------------------------------------------------------------------------

  /**
   * This function runs the thread.
   */

  _run: function GeneratorThread__run() {
    // Schedule the thread.
    this._scheduleStartTime = (new Date()).getTime();
    GeneratorThread.currentThread = this;

    // Run the generator stack.
    this._runStack();

    // Unschedule the thread.
    GeneratorThread.currentThread = null;

    // Terminate the thread if the generator-iterator stack is empty.
    if (this._genIterStack.length == 0)
      this.terminate();

    // Adjust the thread timer period.
    this._adjustTimer();
  },


  /**
   * Run the generator-iterator stack.
   *
   * Continue running the generator-iterator on the top of the stack until one
   * yields without a new generator-iterator or until the stack is empty.
   */

  _runStack: function GeneratorThread__runStack() {
    // Start running with the generator-iterator on the top of the stack.
    var nextGenIter = this._genIterStack[this._genIterStack.length - 1];

    // Keep running while there's a generator-iterator to run.
    //XXXeps ideally, we'd check to see if the thread should yield.  However,
    //XXXeps the check is expensive since creating a new Date object is
    //XXXeps expensive.  Instead, we rely upon the thread to make periodic
    //XXXeps checks.
    var exception = null;
    while (nextGenIter) {
      // Get the next generator-iterator to run.
      var genIter = nextGenIter;
      nextGenIter = null;

      // Run the generator-iterator.
      try {
        // Run the generator-iterator.  If the generator-iterator yields a new
        // generator-iterator, run it next.  If an unhandled exception occured,
        // propagate it to the generator-iterator.
        if (!exception) {
          nextGenIter = genIter.next();
        } else {
          var throwException = exception;
          exception = null;
          nextGenIter = genIter.throw(throwException);
        }

        // If the generator-iterator yielded a new generator-iterator, push it
        // onto the stack.
        if (nextGenIter)
          this._genIterStack.push(nextGenIter);
      } catch (ex) {
        // Pop completed generator-iterator off of stack and run the next
        // generator-iterator on the top of the stack.
        this._genIterStack.pop();
        if (this._genIterStack.length > 0)
          nextGenIter = this._genIterStack[this._genIterStack.length - 1];
        else
          nextGenIter = null;

        // Propagate non-StopIteration exceptions.
        if (!(ex instanceof StopIteration))
          exception = ex;
      }
    }

    // Report unhandled exceptions.
    if (exception) {
      var msg = "GeneratorThread exception: " + exception +
                " at " + exception.fileName +
                ", line " + exception.lineNumber;
      Cu.reportError(msg);
      dump(msg + "\n");
    }
  },


  /**
   * Adjust the thread timer to maintain thread maximum CPU percentage.
   */

  _adjustTimer: function GeneratorThread__adjustTimer() {
    // Do nothing if no timer.
    if (!this._timer)
      return;

    // Get the thread scheduling end time.
    var scheduleEndTime = (new Date()).getTime();

    // Adjust the scheduling period by the amount of excess time over the
    // threshold.
    var adjTime = scheduleEndTime -
                  this._scheduleStartTime -
                  this._yieldThreshold;
    if (adjTime < 0)
      adjTime = 0;

    // Limit the adjustment.
    if (adjTime > 1000)
      adjTime = 1000;

    // Update the scheduling timer period.
    var newDelay = this.period + adjTime;
    if (this._timer.delay != newDelay)
      this._timer.delay = newDelay;
  }
};


