/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

#ifndef SBWINDOWCHROMESERVICE_H_
#define SBWINDOWCHROMESERVICE_H_

#include "sbIWindowChromeService.h"

#include <set>
#include <windows.h>

class sbWindowChromeService : public sbIWindowChromeService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIWINDOWCHROMESERVICE

  sbWindowChromeService();

  // The subclassed WndProc
  static LRESULT WINAPI WndProc(HWND hWnd,
                         UINT uMsg,
                         WPARAM wParam,
                         LPARAM lParam,
                         UINT_PTR uIdSubclass,
                         DWORD_PTR dwRefData);

private:
  ~sbWindowChromeService();

protected:
  // helper to check if DWM composition is enabled
  static bool IsCompositionEnabled(const sbWindowChromeService* self);

  // declarations to memoize DwmIsCompositionEnabled
  typedef HRESULT (WINAPI *t_DwmIsCompositionEnabled)(BOOL *);
  HMODULE mhDWMAPI;
  t_DwmIsCompositionEnabled mDwmIsCompositionEnabled;
};

#define NIGHTINGALE_WINDOW_CHROME_SERVICE_CONTRACTID         \
  "@getnightingale.com/Nightingale/WindowChromeService;1"
#define NIGHTINGALE_WINDOW_CHROME_SERVICE_CLASSNAME          \
  "Nightingale Window Chrome Service"
#define NIGHTINGALE_WINDOW_CHROME_SERVICE_CID                \
{ /* {2DA9047B-2256-4ab0-87D7-EA0AD684027A} */            \
  0x2da9047b,                                             \
  0x2256,                                                 \
  0x4ab0,                                                 \
  {0x87, 0xd7, 0xea, 0xa, 0xd6, 0x84, 0x2, 0x7a}          \
}

#endif /* SBWINDOWCHROMESERVICE_H_ */
