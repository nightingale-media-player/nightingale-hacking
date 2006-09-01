/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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
 * \file WindowMinMax.cpp
 * \brief Service for setting min/max limit callbacks to a window position and size - Implementation.
 */
 
  // Yet another shameful hack from lone
#include "WindowMinMax.h"
#include "WindowMinMaxSubclass.h"
#include "../NativeWindowFromNode.h"

// CLASSES ====================================================================
//=============================================================================
// CWindowMinMax Class
//=============================================================================

// This implements a callback into js so that script code may dynamically limit 
// the position and size of a window

//-----------------------------------------------------------------------------
/* Implementation file */
NS_IMPL_ISUPPORTS1(CWindowMinMax, sbIWindowMinMax)

//-----------------------------------------------------------------------------
CWindowMinMax::CWindowMinMax()
{
} // ctor

//-----------------------------------------------------------------------------
CWindowMinMax::~CWindowMinMax()
{
} // dtor

//-----------------------------------------------------------------------------
NS_IMETHODIMP CWindowMinMax::SetCallback(nsISupports *window, sbIWindowMinMaxCallback *cb)
{
  if (NativeWindowFromNode::get(window) == NULL) return NS_ERROR_FAILURE;
  m_subclasses.push_back(new CWindowMinMaxSubclass(window, cb));
  return NS_OK;
} // SetCallback

//-----------------------------------------------------------------------------
NS_IMETHODIMP CWindowMinMax::ResetCallback(nsISupports *window )
{
  CWindowMinMaxSubclass *subclass = findSubclassByWindow(window);
  if (!subclass) return NS_ERROR_FAILURE;
  m_subclasses.remove(subclass);
  delete subclass;
  return NS_OK;
} // ResetCallback

//-----------------------------------------------------------------------------
CWindowMinMaxSubclass *CWindowMinMax::findSubclassByWindow(nsISupports *window)
{
  std::list<CWindowMinMaxSubclass*>::iterator iter;
  for (iter = m_subclasses.begin(); iter != m_subclasses.end(); iter++)
  {
    CWindowMinMaxSubclass *subclass = *iter;
    if (subclass->getWindow()) return subclass;
  }
  return NULL;
} // findSubclassByWindow

