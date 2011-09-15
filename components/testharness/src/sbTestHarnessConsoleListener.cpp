/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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
 * \file sbTestHarnessConsoleListener.cpp
 * \brief Simple nsIConsoleListener implementation to dump console messages to
 * stdout
 */

#include "sbTestHarnessConsoleListener.h"

#include "nsStringGlue.h"
#include <stdio.h>
#if XP_WIN
  #include <windows.h>
  #undef GetMessage
  #include <stdlib.h>

  void sb_purecall_handler() {
    ::DebugBreak();
  }
#endif

NS_IMPL_ISUPPORTS1(sbTestHarnessConsoleListener, nsIConsoleListener)

sbTestHarnessConsoleListener::sbTestHarnessConsoleListener()
{
  // XXX Mook temp hack
  #if XP_WIN
    _set_purecall_handler(sb_purecall_handler);
  #endif
  MOZ_COUNT_CTOR(sbTestHarnessConsoleListener);
}

sbTestHarnessConsoleListener::~sbTestHarnessConsoleListener()
{
  MOZ_COUNT_DTOR(sbTestHarnessConsoleListener);
}

NS_IMETHODIMP sbTestHarnessConsoleListener::Observe(nsIConsoleMessage *aMessage)
{
  nsAutoString message;
  aMessage->GetMessageMoz(getter_Copies(message));
  printf("<unit-test> %s\n", NS_ConvertUTF16toUTF8(message).get());
  fflush( stdout );
  return NS_OK;
}

