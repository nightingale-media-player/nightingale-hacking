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
* \file MetadataJob.h
* \brief 
*/

#ifndef __METADATA_JOB_H__
#define __METADATA_JOB_H__

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
#include "sbIMetadataJob.h"
#include "sbIMetadataValues.h"

#include <set>

// DEFINES ====================================================================
#define SONGBIRD_METADATAJOB_CONTRACTID \
"@songbirdnest.com/Songbird/MetadataJob;1"

#define SONGBIRD_METADATAJOB_CLASSNAME \
"Songbird Metadata Job Interface"

// {C38FD6BD-3335-4392-A3DE-1855ECEDA4F8}
#define SONGBIRD_METADATAJOB_CID \
{ 0xC38FD6BD, 0x3335, 0x4392, { 0xA3, 0xDE, 0x18, 0x55, 0xEC, 0xED, 0xA4, 0xF8 } }

// FUNCTIONS ==================================================================

// CLASSES ====================================================================
class sbMetadataJob : public sbIMetadataJob
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMETADATAJOB

  sbMetadataJob();
  virtual ~sbMetadataJob();

  static nsresult PR_CALLBACK MetadataJobProcessor(sbMetadataJob* pMetadataJob)
  {
    pMetadataJob->ThreadStart();
  }

  static void MetadataJobTimerInterval(nsITimer *aTimer, void *aClosure);
  static void MetadataJobTimerWorker(nsITimer *aTimer, void *aClosure);

protected:
  void ThreadStart();

  nsString                      mTableName;
  nsCOMPtr<sbIDatabaseQuery>    mQuery;
  PRBool                        mShouldShutdown;
  nsCOMPtr<nsIThread>           mThread;
  nsCOMPtr<sbIMetadataManager>  mMetadataManager;
};

class MetadataJobProcessorThread : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

    MetadataJobProcessorThread(sbMetadataJob* pMetadataJob) {
      NS_ASSERTION(pMetadataJob, "Null pointer!");
      m_pMetadataJob = pMetadataJob;
    }

    NS_IMETHOD Run()
    {
      sbMetadataJob::MetadataJobProcessor(m_pMetadataJob);
      return NS_OK;
    }

protected:
  sbMetadataJob* m_pMetadataJob;
};

#endif // __METADATA_JOB_H__
