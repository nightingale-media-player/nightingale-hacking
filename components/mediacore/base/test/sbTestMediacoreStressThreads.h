/* vim: set sw=2 :miv */
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

#ifndef sbTestMediacoreStressThreads_h
#define sbTestMediacoreStressThreads_h

#include <nsIRunnable.h>
#include <sbIMediacore.h>
#include <prmon.h>
#include <nsCOMArray.h>
#include <nsCOMPtr.h>
#include <nsIThread.h>

#include <sbBaseMediacoreEventTarget.h>
#include <sbIMediacoreEventListener.h>
#include <sbIMediacoreEventTarget.h>

/*
 * event listener stress testing class this is the listener, target
 * and thread runnable class that pretty much does everything
 */

class sbTestMediacoreStressThreads: public nsIRunnable,
                                    public sbIMediacore,
                                    public sbIMediacoreEventListener,
                                    public sbIMediacoreEventTarget

{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE
  NS_DECL_SBIMEDIACOREEVENTLISTENER
  NS_DECL_SBIMEDIACORE
  NS_DECL_SBIMEDIACOREEVENTTARGET

  sbTestMediacoreStressThreads();

private:
  ~sbTestMediacoreStressThreads();
  void OnEvent();

protected:
  nsAutoPtr<sbBaseMediacoreEventTarget> mBaseEventTarget;
  PRInt32                               mCounter;
  PRMonitor*                            mMonitor;
  nsCOMArray<nsIThread>                 mThreads;
};

#define SB_TEST_MEDIACORE_STRESS_THREADS_DESCRIPTION              \
  "Songbird Test Mediacore Stress Threads"
#define SB_TEST_MEDIACORE_STRESS_THREADS_CONTRACTID               \
  "@songbirdnest.com/mediacore/sbTestMediacoreStressThreads;1"
#define SB_TEST_MEDIACORE_STRESS_THREADS_CLASSNAME                \
  "sbTestMediacoreEventCreator"

#define SB_TEST_MEDIACORE_STRESS_THREADS_CID                \
{ /* 77CF73E7-FC6B-458b-969A-97945F4160B7 */               \
  0xa2c2010f,                                              \
  0xd87d,                                                  \
  0x440d,                                                  \
  { 0xbf, 0x2e, 0x74, 0x54, 0x2b, 0x9b, 0xeb, 0xea }       \
}

#endif
