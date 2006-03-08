/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 Pioneers of the Inevitable LLC
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
* \file  WindowMinMax.h
* \brief Service for setting min/max limit callbacks to a window position and size - Prototypes.
*/

#pragma once

// INCLUDES ===================================================================

// XXX Remove Me !!!
#ifndef PRUSTRING_DEFINED
#define PRUSTRING_DEFINED
#include <string>
#include "nscore.h"
namespace std
{
  typedef basic_string< PRUnichar > prustring;
};
#endif

#include "IWindowMinMax.h"
#include <list>

class CWindowMinMaxSubclass;

// DEFINES ====================================================================
#define SONGBIRD_WINDOWMINMAX_CONTRACTID  "@songbird.org/Songbird/WindowMinMax;1"
#define SONGBIRD_WINDOWMINMAX_CLASSNAME   "Songbird Window MinMax Interface"

// {5F4ABE1E-76D2-43b5-8D5E-E6E272A49C50}
#define SONGBIRD_WINDOWMINMAX_CID { 0x5f4abe1e, 0x76d2, 0x43b5, { 0x8d, 0x5e, 0xe6, 0xe2, 0x72, 0xa4, 0x9c, 0x50 } }

// CLASSES ====================================================================
class CWindowMinMax : public sbIWindowMinMax
{
public:
  CWindowMinMax();
  virtual ~CWindowMinMax();

  NS_DECL_ISUPPORTS
  NS_DECL_SBIWINDOWMINMAX
  
protected:
  CWindowMinMaxSubclass *findSubclassByWindow(nsISupports *window);
  std::list<CWindowMinMaxSubclass*> m_subclasses;
};

