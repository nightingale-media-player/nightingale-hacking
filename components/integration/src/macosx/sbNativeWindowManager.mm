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
 * \brief Interface to native window managers (on MacOS X).
 */
 

#include "sbNativeWindowManager.h"
#include "../NativeWindowFromNode.h"

NS_IMPL_ISUPPORTS1(sbNativeWindowManager, sbINativeWindowManager)

NS_IMETHODIMP sbNativeWindowManager::BeginResizeDrag(nsISupports *aWindow, nsIDOMMouseEvent* aEvent, PRInt32 aDirection)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP sbNativeWindowManager::GetSupportsResizeDrag(PRBool *aSupportsResizeDrag)
{
  NS_ENSURE_ARG_POINTER(aSupportsResizeDrag);
  
  *aSupportsResizeDrag = PR_FALSE;

  return NS_OK;
}


NS_IMETHODIMP sbNativeWindowManager::SetMinimumWindowSize(nsISupports *aWindow, PRInt32 aMinimumWidth, PRInt32 aMinimumHeight)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP sbNativeWindowManager::GetSupportsMinimumWindowSize(PRBool *aSupportsMinimumWindowSize)
{
  NS_ENSURE_ARG_POINTER(aSupportsMinimumWindowSize);
  *aSupportsMinimumWindowSize = PR_FALSE;
  return NS_OK;
}


NS_IMETHODIMP sbNativeWindowManager::SetOnTop(nsISupports *aWindow, PRBool aOnTop)
{
  nsresult rv;

  NS_ENSURE_ARG_POINTER(aWindow);

  id window = NativeWindowFromNode::get(aWindow);

  [window setLevel:(aOnTop?NSStatusWindowLevel:NSNormalWindowLevel)];

  return NS_OK;
}


NS_IMETHODIMP sbNativeWindowManager::GetSupportsOnTop(PRBool *aSupportsOnTop)
{
  NS_ENSURE_ARG_POINTER(aSupportsOnTop);

  *aSupportsOnTop = PR_TRUE;

  return NS_OK;
}


