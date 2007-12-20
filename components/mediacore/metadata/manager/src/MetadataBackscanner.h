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
* \file MetadataBackscanner.h
* \brief 
*/

#ifndef __METADATA_BACKSCANNER_H__
#define __METADATA_BACKSCANNER_H__

// INCLUDES ===================================================================
#include <nscore.h>
#include <prlock.h>
#include <prmon.h>

#include <nsStringGlue.h>
#include <nsITimer.h>
#include <nsIThread.h>
#include <nsIRunnable.h>
#include <nsCOMPtr.h>
#include <xpcom/nsIObserver.h>
#include <nsIStringBundle.h>

#include "sbIDatabaseQuery.h"
#include "sbIDatabaseResult.h"

#include "sbIMetadataManager.h"
#include "sbIMetadataHandler.h"
#include "sbIMetadataBackscanner.h"
#include "sbIMetadataValues.h"

#include <set>

// DEFINES ====================================================================
#define SONGBIRD_METADATABACKSCANNER_CONTRACTID \
"@songbirdnest.com/Songbird/MetadataBackscanner;1"

#define SONGBIRD_METADATABACKSCANNER_CLASSNAME \
"Songbird Metadata Backscanner Interface"

// {59FEB66E-288E-4237-B93C-1E69CAEC9485}
#define SONGBIRD_METADATABACKSCANNER_CID \
{ 0x59feb66e, 0x288e, 0x4237, { 0xb9, 0x3c, 0x1e, 0x69, 0xca, 0xec, 0x94, 0x85 } }

// FUNCTIONS ==================================================================
void PrepareStringForQuery(nsAString &str);
void FormatLengthToString(nsAString &str);

// CLASSES ====================================================================
class sbMetadataBackscanner : public sbIMetadataBackscanner,
                              public nsIObserver
{
  friend class BackscannerProcessorThread;

  NS_DECL_ISUPPORTS
  NS_DECL_SBIMETADATABACKSCANNER
  NS_DECL_NSIOBSERVER

  sbMetadataBackscanner();
  virtual ~sbMetadataBackscanner();

  static sbMetadataBackscanner *GetSingleton();
  static nsresult PR_CALLBACK BackscannerProcessor(sbMetadataBackscanner* pBackscanner);

  static void BackscannerTimerInterval(nsITimer *aTimer, void *aClosure);
  static void BackscannerTimerWorker(nsITimer *aTimer, void *aClosure);

protected:
  typedef  std::set<nsString> metacolscache_t;
  
  metacolscache_t m_columnCache;
  nsCOMPtr<nsITimer> m_pIntervalTimer;
  nsCOMPtr<nsITimer> m_pWorkerTimer;

  nsCOMPtr<sbIDatabaseQuery> m_pIntervalQuery;
  nsCOMPtr<sbIDatabaseResult> m_pIntervalResult;
  nsCOMPtr<sbIDatabaseQuery> m_pWorkerQuery;
  nsCOMPtr<sbIMetadataHandler> m_pWorkerHandler;

  PRInt32 m_activeCount;

  PRBool   m_workerHasResultSet;
  PRInt32 m_workerCurrentRow;

  PRBool   m_backscanShouldShutdown;
  PRBool   m_backscanShouldRun;
  PRUint32 m_scanInterval;
  PRUint32 m_updateQueueSize;

  PRMonitor* m_pBackscannerProcessorMonitor;
  nsCOMPtr<nsIThread> m_pThread;

  PRLock *m_pCurrentFileLock;

  nsCOMPtr<nsIStringBundle> m_StringBundle;
  nsCOMPtr<sbIDatabaseQuery> m_pQuery;
  nsCOMPtr<sbIDatabaseQuery> m_pQueryToScan;
  nsCOMPtr<sbIDatabaseResult> m_pResultToScan;
  nsCOMPtr<sbIMetadataManager> m_pManager;
};

class BackscannerProcessorThread : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

    BackscannerProcessorThread(sbMetadataBackscanner* pBackscanner) {
      NS_ASSERTION(pBackscanner, "Null pointer!");
      m_pBackscanner = pBackscanner;
    }

    NS_IMETHOD Run()
    {
      sbMetadataBackscanner::BackscannerProcessor(m_pBackscanner);
      return NS_OK;
    }

protected:
  sbMetadataBackscanner* m_pBackscanner;
};

extern sbMetadataBackscanner *gBackscanner;

#endif // __METADATA_BACKSCANNER_H__
