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

#include "sbDeviceEventTesterRemoval.h"

/**
 * Testing adding and removing listeners
 *
 * Sequence diagram-like thing:
 *
 * + listener A
 *    set flag
 * + listener B
 * + listener C
 * dispatch
 *      (A)   - listener B
 *            + listener D
 *      (B has been removed)
 *      (C)   - listener A
 *            set some flag
 *            dispatch
 *                 (A has been removed)
 *                 (B has been removed)
 *                 (C)   do nothing (flag set)
 *                 (D)   set some other flag
 *      (D is not called)
 * dispatch
 *      (A has been removed)
 *      (B has been removed)
 *      (C does nothing)
 *      (D)   check some other flag
 */

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <mozilla/ModuleUtils.h>

#include <sbIDevice.h>
#include <sbIDeviceEvent.h>
#include <sbIDeviceEventTarget.h>
#include <sbIDeviceManager.h>

#include <stdio.h>

NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceEventTesterRemoval,
                              nsIRunnable)

sbDeviceEventTesterRemoval::sbDeviceEventTesterRemoval()
{
  /* member initializers and constructor code */
}

sbDeviceEventTesterRemoval::~sbDeviceEventTesterRemoval()
{
  /* destructor code */
}

/* void run (); */
NS_IMETHODIMP sbDeviceEventTesterRemoval::Run()
{
  nsresult rv;
  nsCOMPtr<sbIDeviceManager2> manager =
    do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceEventTarget> target = do_QueryInterface(manager, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  /// allocate all the listeners at once
  nsRefPtr<sbDeviceEventTesterRemovalHelper> listenerA =
    new sbDeviceEventTesterRemovalHelper('A');
  NS_ENSURE_TRUE(listenerA, NS_ERROR_OUT_OF_MEMORY);
  mListeners.AppendElement(listenerA);
  rv = target->AddEventListener(listenerA);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbDeviceEventTesterRemovalHelper> listenerB =
    new sbDeviceEventTesterRemovalHelper('B');
  NS_ENSURE_TRUE(listenerB, NS_ERROR_OUT_OF_MEMORY);
  mListeners.AppendElement(listenerB);
  rv = target->AddEventListener(listenerB);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbDeviceEventTesterRemovalHelper> listenerC =
    new sbDeviceEventTesterRemovalHelper('C');
  NS_ENSURE_TRUE(listenerC, NS_ERROR_OUT_OF_MEMORY);
  mListeners.AppendElement(listenerC);
  rv = target->AddEventListener(listenerC);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbDeviceEventTesterRemovalHelper> listenerD =
    new sbDeviceEventTesterRemovalHelper('D');
  NS_ENSURE_TRUE(listenerD, NS_ERROR_OUT_OF_MEMORY);
  mListeners.AppendElement(listenerD);
  // listener D not initially listening

  rv = listenerA->SetFlag(0, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = listenerA->AddAction(sbDeviceEventTesterRemovalHelper::ACTION_CHECK_FLAG,
                            PR_TRUE,
                            0, 1);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = listenerA->AddAction(sbDeviceEventTesterRemovalHelper::ACTION_ADDREMOVE,
                            PR_FALSE,
                            listenerB);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = listenerA->AddAction(sbDeviceEventTesterRemovalHelper::ACTION_ADDREMOVE,
                            PR_TRUE,
                            listenerD);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = listenerA->AddAction(sbDeviceEventTesterRemovalHelper::ACTION_SET_FLAG,
                            PR_FALSE,
                            0, 1);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = listenerB->AddAction(sbDeviceEventTesterRemovalHelper::ACTION_CHECK_FLAG,
                            PR_TRUE,
                            0, 1);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = listenerC->AddAction(sbDeviceEventTesterRemovalHelper::ACTION_CHECK_FLAG,
                            PR_FALSE,
                            0, 3);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = listenerC->AddAction(sbDeviceEventTesterRemovalHelper::ACTION_ADDREMOVE,
                            PR_FALSE,
                            listenerA);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = listenerC->AddAction(sbDeviceEventTesterRemovalHelper::ACTION_SET_FLAG,
                            PR_TRUE,
                            0, 1);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = listenerC->AddAction(sbDeviceEventTesterRemovalHelper::ACTION_DISPATCH,
                            PR_FALSE,
                            0, 1);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = listenerD->AddAction(sbDeviceEventTesterRemovalHelper::ACTION_CHECK_FLAG,
                            PR_FALSE,
                            1, 2);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = listenerD->AddAction(sbDeviceEventTesterRemovalHelper::ACTION_SET_FLAG,
                            PR_TRUE,
                            0, 1);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = listenerD->AddAction(sbDeviceEventTesterRemovalHelper::ACTION_SET_FLAG,
                            PR_TRUE,
                            1, 1);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = listenerD->AddAction(sbDeviceEventTesterRemovalHelper::ACTION_CHECK_FLAG,
                            PR_TRUE,
                            0, 1);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceEvent> event;
  rv = manager->CreateEvent(sbIDeviceEvent::EVENT_CLIENT_DEFINED,
                            nsnull,
                            nsnull,
                            sbIDevice::STATE_IDLE,
                            sbIDevice::STATE_IDLE,
                            getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = target->DispatchEvent(event, PR_FALSE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = target->DispatchEvent(event, PR_FALSE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceEventTesterRemovalHelper,
                              sbIDeviceEventListener)

sbDeviceEventTesterRemovalHelper::sbDeviceEventTesterRemovalHelper(const char aName)
 : mName(aName)
{
  mFlags.Init();
}

nsresult sbDeviceEventTesterRemovalHelper::AddAction(ACTION aAction)
{
  return mActions.AppendElement(aAction) ? NS_OK : NS_ERROR_FAILURE;
}

nsresult sbDeviceEventTesterRemovalHelper::SetFlag(PRUint32 aFlag, bool aSet)
{
  return mFlags.Put(aFlag, aSet) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

/* void onDeviceEvent (in sbIDeviceEvent aEvent); */
NS_IMETHODIMP sbDeviceEventTesterRemovalHelper::OnDeviceEvent(sbIDeviceEvent *aEvent)
{
  if (mActions.Length() < 1) {
    // no actions left
    return NS_OK;
  }

  nsresult rv;
  bool succeeded;

  int length = mActions.Length();

  nsCOMPtr<sbIDeviceEventTarget> target;
  rv = aEvent->GetTarget(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);

  for (int index = 0; index < length; ++index) {
    printf("Removal event: %c#%i\n", mName, index);

    switch (mActions[index].type) {
      case ACTION_ADDREMOVE:
        if (mActions[index].set) {
          rv = target->AddEventListener(mActions[index].listener);
        } else {
          rv = target->RemoveEventListener(mActions[index].listener);
        }
        NS_ENSURE_SUCCESS(rv, rv);
        break;

      case ACTION_SET_FLAG:
        succeeded = mFlags.Put(mActions[index].flag, mActions[index].set);
        NS_ENSURE_TRUE(succeeded, NS_ERROR_FAILURE);
        break;

      case ACTION_CHECK_FLAG:
        if (mActions[index].set) {
          // check flag, fail if false
          bool value = PR_FALSE; // value is not touched for missing keys
          succeeded = mFlags.Get(mActions[index].flag, &value);
          NS_ENSURE_TRUE(value, NS_ERROR_ABORT);
        } else {
          // check flag, if not set, do the next (n) actions
          // if set, do the rest
          bool value = PR_FALSE; // value is not touched for missing keys
          succeeded = mFlags.Get(mActions[index].flag, &value);
          if (value) {
            // skip next (n) actions
            index += mActions[index].count;
          } else {
            // do next (n) actions only
            length = index + 1 + mActions[index].count;
          }
        }
        break;

      case ACTION_DISPATCH:
        nsCOMPtr<sbIDeviceManager2> manager =
          do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv);
        PRUint32 eventType;
        rv = aEvent->GetType(&eventType);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<sbIDeviceEvent> event;
        rv = manager->CreateEvent(eventType,
                                  nsnull,
                                  nsnull,
                                  sbIDevice::STATE_IDLE,
                                  sbIDevice::STATE_IDLE,
                                  getter_AddRefs(event));
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<sbIDeviceEventTarget> target;
        rv = aEvent->GetTarget(getter_AddRefs(target));
        NS_ENSURE_SUCCESS(rv, rv);
        rv = target->DispatchEvent(event, PR_FALSE, nsnull);
        NS_ENSURE_SUCCESS(rv, rv);

        break;
    }
  }

  return NS_OK;
}
