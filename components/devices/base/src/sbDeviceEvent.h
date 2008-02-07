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
#include "sbBaseDeviceAPI.h"
#include "sbIDeviceEventTarget.h"

class nsIVariant;

#define SBDEVICEEVENT_IID \
  {0xad7c89b5, 0x52f8, 0x487f, \
    { 0x90, 0xf7, 0x2f, 0x79, 0xab, 0x8a, 0x84, 0x6b } }

class SB_API sbDeviceEvent : public sbIDeviceEvent
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICEEVENT

  sbDeviceEvent();
  nsresult InitEvent(PRUint32 aType,
                     nsIVariant *aData,
                     nsISupports *aOrigin);

protected:
  ~sbDeviceEvent();

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(SBDEVICEEVENT_IID)
  void Dispatch() { mWasDispatched = PR_TRUE; }
  PRBool WasDispatched() { return mWasDispatched; }
  nsresult SetTarget(sbIDeviceEventTarget* aTarget);
  
protected:
  PRUint32 mType;
  nsCOMPtr<nsIVariant> mData;
  nsCOMPtr<sbIDeviceEventTarget> mTarget;
  nsCOMPtr<nsISupports> mOrigin;
  PRBool mWasDispatched;
};

NS_DEFINE_STATIC_IID_ACCESSOR(sbDeviceEvent, SBDEVICEEVENT_IID)
