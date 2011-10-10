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

#include "sbTestDummyMediacoreManager.h"
#include <sbBaseMediacoreEventTarget.h>

NS_IMPL_THREADSAFE_ISUPPORTS1(sbTestDummyMediacoreManager,
                              sbIMediacoreEventTarget);

sbTestDummyMediacoreManager::sbTestDummyMediacoreManager()
{
  mBaseEventTarget = new sbBaseMediacoreEventTarget(this);
  /* member initializers and constructor code */
}

sbTestDummyMediacoreManager::~sbTestDummyMediacoreManager()
{
  /* destructor code */
}

NS_IMETHODIMP
sbTestDummyMediacoreManager::DispatchEvent(sbIMediacoreEvent *aEvent,
                                    PRBool aAsync,
                                    PRBool* _retval)
{
  return mBaseEventTarget ? mBaseEventTarget->DispatchEvent(aEvent, aAsync, _retval) : NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
sbTestDummyMediacoreManager::AddListener(sbIMediacoreEventListener *aListener)
{
  return mBaseEventTarget ? mBaseEventTarget->AddListener(aListener) : NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
sbTestDummyMediacoreManager::RemoveListener(sbIMediacoreEventListener *aListener)
{
  return mBaseEventTarget ? mBaseEventTarget->RemoveListener(aListener) : NS_ERROR_NULL_POINTER;

}
