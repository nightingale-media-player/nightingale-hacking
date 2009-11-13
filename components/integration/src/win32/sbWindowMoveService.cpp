/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#include "sbWindowMoveService.h"
#include "../NativeWindowFromNode.h"

#include <nsThreadUtils.h>

#define PROP_WMS_INST L"WindowMoveServiceInstance"

static const PRUint32 DEFAULT_HASHTABLE_SIZE = 1;

NS_IMPL_ISUPPORTS1(sbWindowMoveService, sbIWindowMoveService)

sbWindowMoveService::sbWindowMoveService() 
{
}

sbWindowMoveService::~sbWindowMoveService()
{
}

nsresult
sbWindowMoveService::Init()
{
  return NS_OK;
}

inline HHOOK
GetHookForWindow(HWND aWnd, const sbWindowMoveService::hooks_t &aHooks) 
{
  sbWindowMoveService::hooks_t::const_iterator cit = aHooks.find(aWnd);
  if(cit != aHooks.end()) {
    return cit->second;
  }

  return NULL;
}

inline void
CallListenerMoveStarted(HWND aWnd, 
                        const sbWindowMoveService::listeners_t &aListeners)
{
  sbWindowMoveService::listeners_t::const_iterator cit = aListeners.find(aWnd);
  if(cit != aListeners.end()) {
    cit->second->OnMoveStarted();
  }

  return;
}

inline void
CallListenerMoveStopped(HWND aWnd, 
                        const sbWindowMoveService::listeners_t &aListeners)
{
  sbWindowMoveService::listeners_t::const_iterator cit = aListeners.find(aWnd);
  if(cit != aListeners.end()) {
    cit->second->OnMoveStopped();
  }

  return;
}

/*static*/ LRESULT CALLBACK 
sbWindowMoveService::CallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
  PCWPSTRUCT msg = reinterpret_cast<PCWPSTRUCT>(lParam);
  
  sbWindowMoveService *self = 
    reinterpret_cast<sbWindowMoveService *>(::GetPropW(msg->hwnd, PROP_WMS_INST));

  HHOOK hookHandle = GetHookForWindow(msg->hwnd, self->mHooks);

  // This isn't a message we shouldn't process. See SetWindowHookEx docs
  // on MSDN for more information about how to process window hook
  // messages.
  if(nCode < 0) {
    return CallNextHookEx(hookHandle, nCode, wParam, lParam);
  }

  if(msg->message == WM_MOVING ||
     msg->message == WM_WINDOWPOSCHANGING) {
    sbWindowMoveService::resizing_t::iterator it = 
     self->mResizing.find(msg->hwnd);

    if(it == self->mResizing.end()) {
      self->mResizing.insert(std::make_pair<HWND, bool>(msg->hwnd, true));
      CallListenerMoveStarted(msg->hwnd, self->mListeners);
    }
    else if(!it->second) {
      it->second = true;
      CallListenerMoveStarted(msg->hwnd, self->mListeners);
    }
  }
  else if(msg->message == WM_MOVE ||
          msg->message == WM_WINDOWPOSCHANGED) {
    sbWindowMoveService::resizing_t::iterator it = 
      self->mResizing.find(msg->hwnd);

    if(it != self->mResizing.end() &&
       it->second == true) {
      self->mResizing.erase(it);
      CallListenerMoveStopped(msg->hwnd, self->mListeners);
    }
  }

  return CallNextHookEx(hookHandle, nCode, wParam, lParam);
}

PRBool 
sbWindowMoveService::IsHooked(HWND aWnd)
{
  NS_ENSURE_ARG_POINTER(aWnd);

  if(mHooks.find(aWnd) != mHooks.end()) {
    return PR_TRUE;
  }

  return PR_FALSE;
}

NS_IMETHODIMP
sbWindowMoveService::StartWatchingWindow(nsISupports *aWindow,
                                         sbIWindowMoveListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aWindow);
  NS_ENSURE_ARG_POINTER(aListener);
  
  NS_WARN_IF_FALSE(NS_IsMainThread(), "This service is MAIN THREAD ONLY!");

  HWND windowHandle = NULL;
  
  windowHandle = NativeWindowFromNode::get(aWindow);
  NS_ENSURE_TRUE(windowHandle, NS_ERROR_INVALID_ARG);

  // Already hooked. Can only hook once.
  if(IsHooked(windowHandle)) {
    NS_WARNING("Window already hooked. Can only hook a window once.");
    return NS_OK;
  }

  BOOL success = ::SetPropW(windowHandle, PROP_WMS_INST, (HANDLE) this);
  NS_ENSURE_TRUE(success != 0, NS_ERROR_UNEXPECTED);

  HHOOK hookHandle = ::SetWindowsHookEx(WH_CALLWNDPROC, 
                                        sbWindowMoveService::CallWndProc,
                                        NULL,
                                        ::GetCurrentThreadId());
  NS_ENSURE_TRUE(hookHandle, NS_ERROR_FAILURE);

  nsCOMPtr<sbIWindowMoveListener> listener(aListener);
  mListeners.insert(
    std::make_pair<HWND, nsCOMPtr<sbIWindowMoveListener> >(windowHandle, 
                                                           listener));

  mHooks.insert(std::make_pair<HWND, HHOOK>(windowHandle, hookHandle));

  return NS_OK;
}

NS_IMETHODIMP
sbWindowMoveService::StopWatchingWindow(nsISupports *aWindow, 
                                        sbIWindowMoveListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aWindow);
  NS_ENSURE_ARG_POINTER(aListener);

  NS_WARN_IF_FALSE(NS_IsMainThread(), "This service is MAIN THREAD ONLY!");

  HWND windowHandle = NativeWindowFromNode::get(aWindow);
  NS_ENSURE_TRUE(windowHandle, NS_ERROR_INVALID_ARG);

  // Not hooked, nothing to do.
  if(!IsHooked(windowHandle)) {
    NS_WARNING("Attempting to unhook a window that was never hooked.");
    return NS_OK;
  }

  HANDLE propHandle = ::RemovePropW(windowHandle, PROP_WMS_INST);
  NS_WARN_IF_FALSE(propHandle == (HANDLE) this, 
    "Removed property that didn't match what we should've set!");

  HHOOK hookHandle = GetHookForWindow(windowHandle, mHooks);
  NS_ENSURE_TRUE(hookHandle, NS_ERROR_FAILURE);

  BOOL success = ::UnhookWindowsHookEx(hookHandle);
  NS_ENSURE_TRUE(success != 0, NS_ERROR_FAILURE);

  mHooks.erase(windowHandle);
  mListeners.erase(windowHandle);

  return NS_OK;
}
