/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

/** 
 * \file  sbWindowMoveService.h
 * \brief Native window manager interface implementation
 */

#ifndef __SB_WINDOWMOVE_SERVICE_H__
#define __SB_WINDOWMOVE_SERVICE_H__

#include "sbIWindowMoveService.h"

#include <nsITimer.h>
#include <nsHashKeys.h>
#include <nsInterfaceHashtable.h>

#include <map>

#include <windows.h>

class sbWindowMoveService : public sbIWindowMoveService,
                            public nsITimerCallback
{
public:
  sbWindowMoveService();

protected:
  virtual ~sbWindowMoveService();

public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIWINDOWMOVESERVICE
  NS_DECL_NSITIMERCALLBACK

  typedef std::map<HWND, nsCOMPtr<sbIWindowMoveListener> > listeners_t;
  typedef std::map<HWND, HHOOK> hooks_t;
  typedef std::map<HWND, bool> resizing_t;
  
  typedef std::map<HWND, nsCOMPtr<nsITimer> > timers_t;
  typedef std::map<nsITimer *, HWND> timertohwnd_t;

  nsresult Init();

  static LRESULT CALLBACK CallWndProc(int aCode, WPARAM wParam, LPARAM lParam);
  PRBool IsHooked(HWND aWnd);

protected:
  listeners_t mListeners;
  hooks_t     mHooks;
  resizing_t  mResizing;

  timers_t      mTimers;
  timertohwnd_t mTimersToWnd;
};

#endif /*__SB_WINDOWMOVE_SERVICE_H__*/
