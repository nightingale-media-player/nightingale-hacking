/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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
* \file  NativeWindowFromNode.h
* \brief Finds the Native window handle associated with a DOM node - Prototypes.
*/

#ifndef __NATIVE_WINDOW_FROM_NODE_H__
#define __NATIVE_WINDOW_FROM_NODE_H__

#ifdef XP_WIN
#include <windows.h>
#include <dbt.h>
#define NATIVEWINDOW HWND
#define NATIVEDEVICENOTIFY HDEVNOTIFY 
#elif defined(XP_MACOSX)
#include <Carbon/Carbon.h>
#define NATIVEWINDOW WindowPtr
#elif defined(XP_UNIX)
#include <gdk/gdk.h>
#define NATIVEWINDOW GdkWindow*
#else
#define NATIVEWINDOW void*
#define NATIVEDEVICENOTIFY void* 
#endif



// INCLUDES ===================================================================

class nsISupports;
class nsIWidget;

// CLASSES ====================================================================
class NativeWindowFromNode 
{
public:
  static NATIVEWINDOW get(nsISupports *window);  
  static nsIWidget *getWidget(nsISupports *window);  

protected:
};

#endif // __NATIVE_WINDOW_FROM_NODE_H__

