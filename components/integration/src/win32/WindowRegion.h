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
* \file  WindowRegion.h
* \brief Songbird Window Region Object Definition.
*/

#ifndef __WINDOW_REGION_H__
#define __WINDOW_REGION_H__

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

#include "IWindowRegion.h"
#include "NativeWindowFromNode.h"

// DEFINES ====================================================================
#define SONGBIRD_WINDOWREGION_CONTRACTID                  \
  "@songbirdnest.com/Songbird/WindowRegion;1"
#define SONGBIRD_WINDOWREGION_CLASSNAME                   \
  "Songbird Window Region Interface"
#define SONGBIRD_WINDOWREGION_CID                         \
{ /* 3aec3cbb-cba8-4a51-a028-4b28a4719bff */              \
  0x3aec3cbb,                                             \
  0xcba8,                                                 \
  0x4a51,                                                 \
  {0xa0, 0x28, 0x4b, 0x28, 0xa4, 0x71, 0x9b, 0xff}        \
}
// CLASSES ====================================================================
class CWindowRegion : public sbIWindowRegion
{
public:
  CWindowRegion();
  virtual ~CWindowRegion();

  NS_DECL_ISUPPORTS
  NS_DECL_SBIWINDOWREGION
};

#endif // __WINDOW_REGION_H__

