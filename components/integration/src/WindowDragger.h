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
* \file  WindowDragger.h
* \brief Songbird Window Dragger Object Definition.
*/

#ifndef __WINDOW_DRAGGER_H__
#define __WINDOW_DRAGGER_H__

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

#ifdef WIN32
#include <windows.h>
#endif

#include "IWindowDragger.h"

#include "nsCOMPtr.h"
class sbIDataRemote;


// DEFINES ====================================================================
#define SONGBIRD_WINDOWDRAGGER_CONTRACTID                 \
  "@songbirdnest.com/Songbird/WindowDragger;1"
#define SONGBIRD_WINDOWDRAGGER_CLASSNAME                  \
  "Songbird Window Dragger Interface"
#define SONGBIRD_WINDOWDRAGGER_CID                        \
{ /* cf132153-958a-4070-a829-5596fd665b5f */              \
  0xcf132153,                                             \
  0x958a,                                                 \
  0x4070,                                                 \
  {0xa8, 0x29, 0x55, 0x96, 0xfd, 0x66, 0x5b, 0x5f}        \
}

// CLASSES ====================================================================
class CWindowDragger : public sbIWindowDragger
{
public:
  CWindowDragger();
  virtual ~CWindowDragger();

  NS_DECL_ISUPPORTS
  NS_DECL_SBIWINDOWDRAGGER
  

#ifdef XP_WIN
  LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  
protected:

  void OnDrag();
  void EndWindowDrag(UINT msg, WPARAM flags);
  void OnCaptureLost();
  void DoDocking(HWND wnd, int *x, int *y, POINT *monitorPoint);

  void InitPause();
  void IncPause();
  void DecPause();
  
  HWND m_draggedWindow;
  HWND m_captureWindow;
  HWND m_oldCapture;
  int m_dockDistance;

  bool m_backscanPaused; // hack hack hack.  at least it's through a data remote.
  nsCOMPtr<sbIDataRemote> m_pauseScan;

  POINT m_relativeClickPos;
#endif // XP_WIN

};


#endif // __WINDOW_DRAGGER_H__

