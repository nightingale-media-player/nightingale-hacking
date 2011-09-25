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
 * \file sbINativeWindowManager
 * \brief Interface to native window managers (on MacOS X).
 */

#include "sbNativeWindowManager.h"
#include "../NativeWindowFromNode.h"


NS_IMPL_ISUPPORTS1(sbNativeWindowManager, sbINativeWindowManager)

sbNativeWindowManager::sbNativeWindowManager()
{
}


sbNativeWindowManager::~sbNativeWindowManager()
{
}


NS_IMETHODIMP 
sbNativeWindowManager::BeginResizeDrag(nsISupports *aWindow, 
                                       nsIDOMMouseEvent* aEvent, 
                                       PRInt32 aDirection)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP 
sbNativeWindowManager::GetSupportsResizeDrag(PRBool *aSupportsResizeDrag)
{
  NS_ENSURE_ARG_POINTER(aSupportsResizeDrag);
  *aSupportsResizeDrag = PR_FALSE;
  return NS_OK;
}


NS_IMETHODIMP 
sbNativeWindowManager::SetMinimumWindowSize(nsISupports *aWindow, 
                                            PRInt32 aMinimumWidth, 
                                            PRInt32 aMinimumHeight)
{
  nsresult rv = NS_OK;
  id window = NativeWindowFromNode::get(aWindow);
  if (window)
    [window setMinSize:NSMakeSize(aMinimumWidth, aMinimumHeight)];
  else
    rv = NS_ERROR_UNEXPECTED;

  return rv;
}


NS_IMETHODIMP 
sbNativeWindowManager::SetMaximumWindowSize(nsISupports *aWindow, 
                                            PRInt32 aMaximumWidth, 
                                            PRInt32 aMaximumHeight)
{
  nsresult rv = NS_OK;
  id window = NativeWindowFromNode::get(aWindow);
  if (window)
    [window setMaxSize:NSMakeSize(aMaximumWidth, aMaximumHeight)];
  else
    rv = NS_ERROR_UNEXPECTED;
  
  return rv;
}


NS_IMETHODIMP 
sbNativeWindowManager::GetSupportsMinimumWindowSize(PRBool *aSupportsMinimumWindowSize)
{
  NS_ENSURE_ARG_POINTER(aSupportsMinimumWindowSize);
  *aSupportsMinimumWindowSize = PR_TRUE;
  return NS_OK;
}


NS_IMETHODIMP 
sbNativeWindowManager::GetSupportsMaximumWindowSize(PRBool *aSupportsMaximumWindowSize)
{
  NS_ENSURE_ARG_POINTER(aSupportsMaximumWindowSize);
  *aSupportsMaximumWindowSize = PR_TRUE;
  return NS_OK;
}


NS_IMETHODIMP 
sbNativeWindowManager::SetOnTop(nsISupports *aWindow, PRBool aOnTop)
{
  NS_ENSURE_ARG_POINTER(aWindow);

  nsresult rv = NS_OK;
  id window = NativeWindowFromNode::get(aWindow);
  if (window)
    [window setLevel:(aOnTop?NSStatusWindowLevel:NSNormalWindowLevel)];
  else
    rv = NS_ERROR_UNEXPECTED;

  return rv;
}

NS_IMETHODIMP 
sbNativeWindowManager::SetShadowing(nsISupports *aWindow, PRBool aShadowing)
{
  // Not required on Mac, this is automatic thanks to a XULRunner patch.
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbNativeWindowManager::GetSupportsOnTop(PRBool *aSupportsOnTop)
{
  NS_ENSURE_ARG_POINTER(aSupportsOnTop);
  *aSupportsOnTop = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP 
sbNativeWindowManager::GetSupportsShadowing(PRBool *aSupportsShadowing)
{
  NS_ENSURE_ARG_POINTER(aSupportsShadowing);

  // Not required on Mac, this is automatic thanks to a XULRunner patch.
  *aSupportsShadowing = PR_FALSE;

  return NS_OK;
}
