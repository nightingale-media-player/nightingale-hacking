/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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
 * \file  ObserverUtils.jsm
 * \brief Javascript source for the observer utilities.
 */

//------------------------------------------------------------------------------
//
// Observer utilities configuration.
//
//------------------------------------------------------------------------------

EXPORTED_SYMBOLS = [ "ObserverSet" ];


//------------------------------------------------------------------------------
//
// Observer utility defs.
//
//------------------------------------------------------------------------------

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results
const Cu = Components.utils


//------------------------------------------------------------------------------
//
// Observer set services.
//
//   These services may be used to maintain a set of observers and to facilitate
// the removal of them.
//
//------------------------------------------------------------------------------

/**
 * Construct an observer set object.
 */

function ObserverSet()
{
  // Get the observer services.
  this._observerSvc = Cc["@mozilla.org/observer-service;1"]
                        .getService(Ci.nsIObserverService);

  // Initialize some fields.
  this._observerList = {};
}

// Define the class.
ObserverSet.prototype = {
  // Set the constructor.
  constructor: ObserverSet,

  //
  // Observer set fields.
  //
  //   _observerSvc             Mozilla observer service.
  //   _observerList            List of observers.
  //   _nextObserverID          Next observer ID.
  //

  _observerSvc: null,
  _observerList: null,
  _nextObserverID: 0,


  /**
   * Add the observer object specified by aObserver as an observer of the event
   * topic specified by aTopic with weak ownership as specified by aOwnsWeak.
   * If aIsOneShot is true, the observer is removed after the first event is
   * observed.
   * Return an observer ID that may be used with remove.
   *
   * \param aObserver           Observer object.
   * \param aTopic              Observer topic.
   * \param aOwnsWeak           Weak ownership.
   * \param aIsOneShot          True if observer should be removed after the
   *                            first event.
   *
   * \return                    Observer ID.
   */

  add: function ObserverSet_add(aObserver,
                                aTopic,
                                aOwnsWeak,
                                aIsOneShot) {
    // Create the observer set entry object.
    var observerEntry = {};
    observerEntry.id = this._nextObserverID++;
    observerEntry.observer = aObserver;
    observerEntry.topic = aTopic;
    observerEntry.ownsWeak = aOwnsWeak;
    observerEntry.isOneShot = aIsOneShot;

    // Use one-shot observer wrapper if observer is one-shot.
    var observer = observerEntry.observer;
    if (observerEntry.isOneShot) {
      var _this = this;
      observer = {
        observe: function(aSubject, aTopic, aData) {
          return _this._doOneShot(observerEntry, aSubject, aTopic, aData);
        }
      };
    }
    observerEntry.addedObserver = observer;

    // Add the observer.
    this._observerSvc.addObserver(observerEntry.addedObserver,
                                  observerEntry.topic,
                                  observerEntry.ownsWeak);
    this._observerList[observerEntry.id] = observerEntry;

    return observerEntry.id;
  },


  /**
   * Remove the observer specified by aObserverID.
   *
   * \param aObserverID         ID of observer to remove.
   */

  remove: function ObserverSet_remove(aObserverID) {
    // Get the observer set entry.  Do nothing if not found.
    var observerEntry = this._observerList[aObserverID];
    if (!observerEntry)
      return;

    // Remove the observer.
    this._observerSvc.removeObserver(observerEntry.addedObserver,
                                     observerEntry.topic,
                                     observerEntry.ownsWeak);
    delete this._observerList[aObserverID];
  },


  /**
   * Remove all observers.
   */

  removeAll: function ObserverSet_removeAll() {
    // Remove all observers.
    for (var id in this._observerList) {
      this.remove(id);
    }
    this._observerList = {};
  },


  //----------------------------------------------------------------------------
  //
  // Internal observer set services.
  //
  //----------------------------------------------------------------------------

  /**
   * Dispatch the observed event specified by aSubject, aTopic, and aData to the
   * one-shot observer entry specified aObserverEntry and remove the observer.
   *
   * \param aObserverEntry      One-shot observer entry.
   * \param aSubject            Event subject.
   * \param aTopic              Event topic.
   * \param aData               Event data.
   */

  _doOneShot: function ObserverSet__doOneShot(aObserverEntry,
                                              aSubject,
                                              aTopic,
                                              aData) {
    // Dispatch event to observer.
    return aObserverEntry.observer.observe(aSubject, aTopic, aData);
  }
};


