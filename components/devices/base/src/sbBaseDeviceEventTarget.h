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
#ifndef SBBASEDEVICEEVENTTARGET_H
#define SBBASEDEVICEEVENTTARGET_H

#include "sbIDeviceEventTarget.h"

#include <nsCOMArray.h>
#include <nsDeque.h>
#include <nsWeakReference.h>
#include <prmon.h>

class sbIDeviceEventListener;

/**
 * sbBaseDeviceEventTarget is a helper class to implement a basic
 * sbIDeviceEventTarget.  All events will be proxied onto the main thread and
 * dispatched there.
 */

class sbBaseDeviceEventTarget : public sbIDeviceEventTarget
{
public:
  NS_DECL_SBIDEVICEEVENTTARGET;
  sbBaseDeviceEventTarget();

protected:
  virtual ~sbBaseDeviceEventTarget();

protected:
  nsresult DispatchEventInternal(nsCOMPtr<sbIDeviceEvent> aEvent);

protected:
  // the listener array should always be accessed from the main thread
  nsCOMArray<sbIDeviceEventListener> mListeners;

  struct DispatchState {
    // these are indices into mListeners
    PRInt32 index; // the currently processing listener index
    PRInt32 length;  // the number of listeners at the start of this dispatch
  };
  // our stack of states (holds *pointers* to DispatchStates)
  nsDeque mStates;
  friend class sbDeviceEventTargetRemovalHelper;

  nsCOMPtr<nsIWeakReference> mParentEventTarget;

private:
  PRMonitor* mMonitor;
};

#endif

