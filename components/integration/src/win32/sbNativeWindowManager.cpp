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
 * \file sbINativeWindowManager
 * \brief Interface to native window managers (on Windows).
 */
 

#include "sbNativeWindowManager.h"
#include "../NativeWindowFromNode.h"

#include <windows.h>
#include <dwmapi.h>

NS_IMPL_ISUPPORTS1(sbNativeWindowManager, sbINativeWindowManager)

typedef HRESULT (STDAPICALLTYPE* DwmSetWindowAttributeProc_t)(HWND,
                                                              DWORD,
                                                              LPCVOID,
                                                              DWORD);

typedef HRESULT (STDAPICALLTYPE* DwmExtendFrameIntoClientArea_t)(HWND,
                                                                 const MARGINS *);

NS_IMETHODIMP sbNativeWindowManager::BeginResizeDrag(nsISupports *aWindow, nsIDOMMouseEvent* aEvent, PRInt32 aDirection)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP sbNativeWindowManager::GetSupportsResizeDrag(bool *aSupportsResizeDrag)
{
  NS_ENSURE_ARG_POINTER(aSupportsResizeDrag);
  *aSupportsResizeDrag = PR_FALSE;
  return NS_OK;
}


NS_IMETHODIMP sbNativeWindowManager::SetMinimumWindowSize(nsISupports *aWindow, PRInt32 aMinimumWidth, PRInt32 aMinimumHeight)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP sbNativeWindowManager::SetMaximumWindowSize(nsISupports *aWindow, PRInt32 aMaximumWidth, PRInt32 aMaximumHeight)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP sbNativeWindowManager::GetSupportsMinimumWindowSize(bool *aSupportsMinimumWindowSize)
{
  NS_ENSURE_ARG_POINTER(aSupportsMinimumWindowSize);
  *aSupportsMinimumWindowSize = PR_FALSE;
  return NS_OK;
}


NS_IMETHODIMP sbNativeWindowManager::GetSupportsMaximumWindowSize(bool *aSupportsMaximumWindowSize)
{
  NS_ENSURE_ARG_POINTER(aSupportsMaximumWindowSize);
  *aSupportsMaximumWindowSize = PR_FALSE;
  return NS_OK;
}


NS_IMETHODIMP sbNativeWindowManager::SetOnTop(nsISupports *aWindow, bool aOnTop)
{
  NS_ENSURE_ARG_POINTER(aWindow);

  HWND window = NativeWindowFromNode::get(aWindow);
  SetWindowPos(window, aOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, 
    SWP_NOSIZE | SWP_NOMOVE);

  return NS_OK;
}

NS_IMETHODIMP sbNativeWindowManager::SetShadowing(nsISupports *aWindow, bool aShadowing)
{
  NS_ENSURE_ARG_POINTER(aWindow);

  HWND window = NativeWindowFromNode::get(aWindow);
  HMODULE hDll = LoadLibraryW(L"dwmapi");

  // failure is not an error.
  if(!hDll) {
    return NS_OK;
  }

  DwmSetWindowAttributeProc_t setWindowAttribute = 
    (DwmSetWindowAttributeProc_t) GetProcAddress(hDll, "DwmSetWindowAttribute");

  DwmExtendFrameIntoClientArea_t extendFrameIntoClientArea = 
    (DwmExtendFrameIntoClientArea_t) GetProcAddress(hDll, "DwmExtendFrameIntoClientArea");

  if(!setWindowAttribute || !extendFrameIntoClientArea) {
    return NS_ERROR_UNEXPECTED;
  }

  DWMNCRENDERINGPOLICY ncrp = aShadowing ? DWMNCRP_ENABLED : DWMNCRP_DISABLED;

  // enable non-client area rendering on window
  HRESULT hr = setWindowAttribute(window, DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_UNEXPECTED);

  //set margins
  MARGINS margins = {1, 0, 0, 0};

  //extend frame 
  hr = extendFrameIntoClientArea(window, &margins);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_UNEXPECTED);

  FreeLibrary(hDll);

  return NS_OK;
}

NS_IMETHODIMP sbNativeWindowManager::GetSupportsOnTop(bool *aSupportsOnTop)
{
  NS_ENSURE_ARG_POINTER(aSupportsOnTop);

  *aSupportsOnTop = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP sbNativeWindowManager::GetSupportsShadowing(bool *aSupportsShadowing)
{
  NS_ENSURE_ARG_POINTER(aSupportsShadowing);

  *aSupportsShadowing = PR_TRUE;

  return NS_OK;
}
