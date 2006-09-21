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
* \file  WindowMinMaxSubclass.h
* \brief Window subclasser object for WindowMinMax service - Prototypes.
*/

#ifndef __WINDOW_MIN_MAX_SUBCLASS_H__
#define __WINDOW_MIN_MAX_SUBCLASS_H__

// INCLUDES ===================================================================

/*
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
*/

#include "../NativeWindowFromNode.h"

class nsISupports;
class sbIWindowMinMaxCallback;

// CLASSES ====================================================================
class CWindowMinMaxSubclass 
{
public:
  CWindowMinMaxSubclass(nsISupports *window, sbIWindowMinMaxCallback *cb);
  virtual ~CWindowMinMaxSubclass();

#ifdef XP_WIN
  LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
  
  nsISupports *getWindow();
  sbIWindowMinMaxCallback *getCallback();
  NATIVEWINDOW getNativeWindow();

protected:
  void SubclassWindow();
  void UnsubclassWindow();
  
  WNDPROC m_prevWndProc;
  NATIVEWINDOW m_hwnd;
  NATIVEDEVICENOTIFY m_hDevNotify;
  nsISupports *m_window;
  sbIWindowMinMaxCallback *m_callback;
  
  int m_dx, m_ix, m_dy, m_iy;
};

#endif // __WINDOW_MIN_MAX_SUBCLASS_H__

