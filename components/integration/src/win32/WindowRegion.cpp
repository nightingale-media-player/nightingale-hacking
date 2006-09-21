/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2006 POTI, Inc.
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
 * \file WindowRegion.cpp
 * \brief Songbird Window Region Object Implementation.
 */
 
#include "WindowRegion.h"
#include "nsIScriptableRegion.h"
#include "nsIRegion.h"

// CLASSES ====================================================================
//=============================================================================
// CWindowRegionClass
//=============================================================================

//-----------------------------------------------------------------------------
/* Implementation file */
NS_IMPL_ISUPPORTS1(CWindowRegion, sbIWindowRegion)

//-----------------------------------------------------------------------------
CWindowRegion::CWindowRegion()
{
} // ctor

//-----------------------------------------------------------------------------
CWindowRegion::~CWindowRegion()
{
} // dtor

NS_IMETHODIMP CWindowRegion::SetWindowRegion(nsISupports *window, nsISupports *region)
{
  if (!region) return NS_ERROR_FAILURE;
  if (!window) return NS_ERROR_FAILURE;
  
  NATIVEWINDOW wnd = NativeWindowFromNode::get(window);

  nsIScriptableRegion *srgn = NULL;
  region->QueryInterface(NS_GET_IID(nsIScriptableRegion), (void **)&srgn);
  if (!srgn) return NS_ERROR_FAILURE;
  
  nsIRegion *rgn = NULL;
  srgn->GetRegion(&rgn);
  
  if (rgn)
  {
#ifdef XP_WIN
    void *hrgn;
    rgn->GetNativeRegion(hrgn);
    ::SetWindowRgn(wnd, (HRGN)hrgn, TRUE);
#endif
  }
  
  NS_RELEASE(srgn);  

  return NS_OK;
}

