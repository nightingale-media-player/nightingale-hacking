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
#include <nsTArray.h>
#include <nsCOMArray.h>
#include <nsCOMPtr.h>
#include <nsITimer.h>
#include <nsIThread.h>
#include <nsIRunnable.h>
#include <nsIStringBundle.h>
#include <xpcom/nsIObserver.h>
#include <nsIStringBundle.h>
#include <nsInterfaceHashtable.h>

#include "sbILibraryManager.h"
#include "sbIDatabaseQuery.h"
#include "sbIDatabaseResult.h"
#include "sbIDataRemote.h"
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
class sbIMediaItem;
class sbIPropertyArray;
class sbIPropertyManager;
class MetadataJobProcessorThread;

class sbMetadataJob : public sbIMetadataJob
{
  friend class MetadataJobProcessorThread;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMETADATAJOB

  sbMetadataJob();
  virtual ~sbMetadataJob();

  static void MetadataJobTimer(nsITimer *aTimer, void *aClosure)
  {
    ((sbMetadataJob*)aClosure)->RunTimer();
  }

  static inline nsString DATABASE_GUID() { return NS_LITERAL_STRING( "sbMetadataJob" ); }

protected:
  struct jobitem_t
  {
    jobitem_t(
      nsString _library_guid = nsString(),
      nsString _item_guid = nsString(),
      nsString _url = nsString(),
      nsString _worker_thread = nsString(),
      nsString _is_scanned = nsString(),
      sbIMetadataHandler *_handler = nsnull
    ) :
      library_guid( _library_guid ),
      item_guid( _item_guid ),
      url( _url ),
      worker_thread( _worker_thread ),
      is_scanned( _is_scanned ),
      handler( _handler )
    {}

    nsString library_guid;
    nsString item_guid;
    nsString url;
    nsString worker_thread;
    nsString is_scanned;
    nsCOMPtr<sbIMetadataHandler> handler;
  };

  nsresult RunTimer();
  nsresult CancelTimer();
  nsresult RunThread( PRBool * bShutdown );
  nsresult FinishJob();

  static nsresult AddItemToJobTableQuery( sbIDatabaseQuery *aQuery, nsString aTableName, sbIMediaItem *aMediaItem, jobitem_t **_retval );
  static nsresult GetNextItem( sbIDatabaseQuery *aQuery, nsString aTableName, PRBool isWorkerThread, jobitem_t **_retval );
  static nsresult SetItemIsScanned( sbIDatabaseQuery *aQuery, nsString aTableName, jobitem_t *aItem );
  static nsresult SetItemIsWrittenAndDelete( sbIDatabaseQuery *aQuery, nsString aTableName, jobitem_t *aItem, PRBool aExecute = PR_TRUE );
  static nsresult SetItemsAreWrittenAndDelete( sbIDatabaseQuery *aQuery, nsString aTableName, nsTArray< jobitem_t * > &aItemArray );
  static nsresult SetItemIs( const nsAString &aColumnString, sbIDatabaseQuery *aQuery, nsString aTableName, jobitem_t *aItem, PRBool aExecute = PR_TRUE );
  static nsresult ResetUnwritten( sbIDatabaseQuery *aQuery, nsString aTableName );
  static nsresult StartHandlerForItem( jobitem_t *aItem );
  static nsresult AddMetadataToItem( jobitem_t *aItem, PRBool aShouldFlush );
  static nsresult AddDefaultMetadataToItem( jobitem_t *aItem, sbIMediaItem *aMediaItem, PRBool aShouldFlush );
  static nsString CreateDefaultItemName( const nsString &aURLString );
  static nsresult AppendIfValid(sbIPropertyManager* aPropertyManager, sbIMutablePropertyArray* aProperties, const nsAString& aName, const nsAString& aValue);

  void IncrementDataRemote();
  void DecrementDataRemote();

#define MDJ_CRACK( function, string ) \
  static inline nsAutoString function ( sbIDatabaseResult *i ) { nsAutoString str; i->GetRowCellByColumn(0, NS_LITERAL_STRING( string ), str); return str; }
MDJ_CRACK( LG, "library_guid" );
MDJ_CRACK( IG, "item_guid" );
MDJ_CRACK( URL, "url" );
MDJ_CRACK( WT, "worker_thread" );
MDJ_CRACK( IS, "is_scanned" );

  nsCOMArray<sbIMediaItem>      mInitArray;
  PRUint32                      mInitCount;
  nsString                      mStatusDisplayString;
  nsCOMPtr<sbIDataRemote>       mDataStatusDisplay;
  nsCOMPtr<sbIDataRemote>       mDataCurrentMetadataJobs;
  nsString                      mTableName;
  PRUint32                      mSleepMS;
  nsCOMPtr<sbIDatabaseQuery>    mMainThreadQuery;
  nsCOMPtr<nsITimer>            mTimer;
  nsCOMPtr<nsIThread>           mThread;
  nsTArray<jobitem_t *>         mTimerWorkers;
  nsCOMPtr<MetadataJobProcessorThread> mMetatataJobProcessor;
  nsCOMPtr<nsIObserver>         mObserver;
  PRBool                        mCompleted;
  PRBool                        mInitCompleted;
  PRBool                        mInitExecuted;
  PRBool                        mTimerCompleted;
  PRBool                        mThreadCompleted;
};

class MetadataJobProcessorThread : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  MetadataJobProcessorThread(sbMetadataJob* pMetadataJob) {
    NS_ASSERTION(pMetadataJob, "Null pointer!");
    mMetadataJob = pMetadataJob;
    mShutdown = PR_FALSE;
  }

  NS_IMETHOD Run()
  {
    mMetadataJob->mThreadCompleted = PR_FALSE;
    nsresult rv = mMetadataJob->RunThread( &mShutdown );
    mMetadataJob->mThreadCompleted = PR_TRUE;
    return rv;
  }

  sbMetadataJob* mMetadataJob;
  PRBool mShutdown;
};

#endif // __METADATA_JOB_H__
