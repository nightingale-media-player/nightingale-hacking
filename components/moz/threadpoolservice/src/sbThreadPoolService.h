/*
//
// BEGIN NIGHTINGALE GPL
// 
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
 */

/** 
* \file  sbThreadPoolService.h
* \brief Nightingale Thread Pool Service.
*/

#ifndef __SB_THREADPOOLSERVICE_H__
#define __SB_THREADPOOLSERVICE_H__

#include <nsIObserver.h>
#include <nsIRunnable.h>
#include <nsIThreadPool.h>
#include <nsCOMPtr.h>

#define SB_THREADPOOLSERVICE_CONTRACTID                   \
  "@getnightingale.com/Nightingale/ThreadPoolService;1"
#define SB_THREADPOOLSERVICE_CLASSNAME                    \
  "Nightingale ThreadPool Service"
#define SB_THREADPOOLSERVICE_CID                          \
{ /* f3980a5a-39da-4e1b-bb63-c3e26e85d3c6 */              \
  0xf3980a5a,                                             \
  0x39da,                                                 \
  0x4e1b,                                                 \
  { 0xbb, 0x63, 0xc3, 0xe2, 0x6e, 0x85, 0xd3, 0xc6 }      \
}

class sbThreadPoolService : public nsIThreadPool,
                            public nsIObserver
{
public:
  sbThreadPoolService();
  virtual ~sbThreadPoolService();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_FORWARD_SAFE_NSITHREADPOOL(mThreadPool)
  NS_FORWARD_SAFE_NSIEVENTTARGET(mThreadPool)

  nsresult Init();

private:
  nsCOMPtr<nsIThreadPool> mThreadPool;
};

#endif // __SB_THREADPOOLSERVICE_H__

