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

#ifndef __SBTIMINGSERVICE_H__
#define __SBTIMINGSERVICE_H__

#include <sbITimingService.h>

#include <nsIFile.h>
#include <nsIObserver.h>
#include <nsIGenericFactory.h>

#include <nsAutoLock.h>
#include <nsCOMPtr.h>
#include <nsInterfaceHashtable.h>
#include <nsStringGlue.h>
#include <nsTHashtable.h>

class sbTimingService;
class sbTimingServiceTimer;

class sbTimingServiceTimer : public sbITimingServiceTimer 
{
  friend class sbTimingService;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBITIMINGSERVICETIMER;

  sbTimingServiceTimer();
  ~sbTimingServiceTimer();

  nsresult Init(const nsAString &aTimerName);

protected:
  PRLock * mTimerLock;

  nsString mTimerName;

  PRTime   mTimerStartTime;
  PRTime   mTimerStopTime;
  PRTime   mTimerTotalTime;
};

class sbTimingService : public sbITimingService,
                        public nsIObserver
{
public:

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_SBITIMINGSERVICE

  sbTimingService();
  ~sbTimingService();

  static NS_METHOD RegisterSelf(nsIComponentManager* aCompMgr,
                                nsIFile* aPath,
                                const char* aLoaderStr,
                                const char* aType,
                                const nsModuleComponentInfo *aInfo);

  NS_METHOD Init();

  nsresult FormatResultsToString(nsACString &aOutput);

private:
  PRLock *          mLoggingLock;
  PRBool            mLoggingEnabled;
  nsCOMPtr<nsIFile> mLogFile;

  // Supplementary lock is required to ensure that a timer of the same name
  // as another does not get inserted while the first time is in process of
  // insertion.
  PRLock *mTimersLock;

  nsInterfaceHashtableMT<nsStringHashKey, sbITimingServiceTimer> mTimers;
  
  // Supplementary lock is required to ensure that the count doesn't change
  // between the time we look it up and the time we insert an entry.
  PRLock *mResultsLock;

  nsInterfaceHashtableMT<nsUint32HashKey, sbITimingServiceTimer> mResults;
};

#endif /* __SBTIMINGSERVICE_H__ */

