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
* \file  WindowLayer.h
* \brief Service for setting the opacity of a window - Prototypes.
*/

#ifndef __WINDOW_LAYER_H__
#define __WINDOW_LAYER_H__

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

#include "IWindowLayer.h"
#include <list>

#include "../NativeWindowFromNode.h"

// DEFINES ====================================================================
#define SONGBIRD_WINDOWLAYER_CONTRACTID                  \
  "@songbirdnest.com/Songbird/WindowLayer;1"
#define SONGBIRD_WINDOWLAYER_CLASSNAME                   \
  "Songbird Window Layer Interface"
#define SONGBIRD_WINDOWLAYER_CID                         \
{ /* 7b566532-7422-11db-9694-00e08161165f */             \
  0x7b566532,                                            \
  0x7422,                                                \
  0x11db,                                                \
  {0x96, 0x94, 0x0, 0xe0, 0x81, 0x61, 0x16, 0x5f}        \
}
// CLASSES ====================================================================
class CWindowLayer : public sbIWindowLayer
{
public:
  CWindowLayer();
  virtual ~CWindowLayer();

  NS_DECL_ISUPPORTS
  NS_DECL_SBIWINDOWLAYER
  
protected:
#ifdef XP_WIN
  HMODULE hUser32;
#endif

};

#endif // __WINDOW_LAYER_H__

