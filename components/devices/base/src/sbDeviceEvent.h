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

#include <nsIVariant.h>
#include "sbIDeviceEvent.h"

#include <nsCOMPtr.h>
#include "sbIDeviceEventTarget.h"

class nsIVariant;

#define SBDEVICEEVENT_IID \
  {0xad7c89b5, 0x52f8, 0x487f, \
    { 0x90, 0xf7, 0x2f, 0x79, 0xab, 0x8a, 0x84, 0x6b } }

class sbDeviceEvent : public sbIDeviceEvent
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICEEVENT

  nsresult InitEvent(PRUint32 aType,
                     nsIVariant *aData,
                     nsISupports *aOrigin,
                     PRUint32 aDeviceState,
                     PRUint32 aDeviceSubState);
  static sbDeviceEvent* CreateEvent();

protected:
  sbDeviceEvent();
  ~sbDeviceEvent();

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(SBDEVICEEVENT_IID)
  virtual void Dispatch() { mWasDispatched = PR_TRUE; }
  virtual PRBool WasDispatched() { return mWasDispatched; }
  virtual nsresult SetTarget(sbIDeviceEventTarget* aTarget);

  // XXXAus: This static CreateEvent method is necessary
  // to accommodate our static linking of the CRT. Otherwise
  // the event would get deallocated with the wrong allocator.
  static nsresult CreateEvent(PRUint32 aType,
                              nsIVariant *aData,
                              nsISupports *aOrigin,
                              PRUint32 aDeviceState,
                              PRUint32 aDeviceSubState,
                              sbIDeviceEvent **_retval);

protected:
  PRUint32 mType;
  nsCOMPtr<nsIVariant> mData;
  nsCOMPtr<sbIDeviceEventTarget> mTarget;
  nsCOMPtr<nsISupports> mOrigin;
  PRBool mWasDispatched;
  PRUint32 mDeviceState;
  PRUint32 mDeviceSubState;
};

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_SBDEVICEEVENT(_to) \
  void Dispatch() { NS_ASSERTION(_to, "_to is null"); if (_to) _to->Dispatch(); } \
  PRBool WasDispatched() { NS_ASSERTION(_to, "_to is null"); return !_to ? PR_FALSE : _to->WasDispatched(); } \
  nsresult SetTarget(sbIDeviceEventTarget* aTarget) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetTarget(aTarget); }

NS_DEFINE_STATIC_IID_ACCESSOR(sbDeviceEvent, SBDEVICEEVENT_IID)
