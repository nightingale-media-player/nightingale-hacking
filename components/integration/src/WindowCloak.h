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
* \file  WindowCloak.h
* \brief Songbird Window Cloaker Object Definition.
*/

#ifndef __WINDOW_CLOAK_H__
#define __WINDOW_CLOAK_H__


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

#include "IWindowCloak.h"
#include <list>
#include "NativeWindowFromNode.h"

// DEFINES ====================================================================
#define SONGBIRD_WINDOWCLOAK_CONTRACTID                   \
  "@songbirdnest.com/Songbird/WindowCloak;1"
#define SONGBIRD_WINDOWCLOAK_CLASSNAME                    \
  "Songbird Window Cloak Interface"
#define SONGBIRD_WINDOWCLOAK_CID                          \
{ /* d5267aa4-f3ba-4b7b-b136-c1861c410ee5 */              \
  0xd5267aa4,                                             \
  0xf3ba,                                                 \
  0x4b7b,                                                 \
  {0xb1, 0x36, 0xc1, 0x86, 0x1c, 0x41, 0xe, 0xe5}         \
}
// CLASSES ====================================================================
class WindowCloakEntry 
{
public:
  NATIVEWINDOW m_hwnd;
  short m_oldX;
  short m_oldY;
};

class CWindowCloak : public sbIWindowCloak
{
public:
  CWindowCloak();
  virtual ~CWindowCloak();

  NS_DECL_ISUPPORTS
  NS_DECL_SBIWINDOWCLOAK
  
  WindowCloakEntry *findItem(NATIVEWINDOW wnd);
  
protected:
  static std::list<WindowCloakEntry*> m_items;
};

#endif // __WINDOW_CLOAK_H__

