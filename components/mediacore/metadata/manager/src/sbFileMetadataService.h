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
 * \file sbFileMetadataService.h
 * \brief Coordinates reading and writing of media file metadata
 */

#ifndef SBFILEMETADATASERVICE_H__
#define SBFILEMETADATASERVICE_H__

// INCLUDES ===================================================================
#include <nscore.h>
#include <prlock.h>
#include <nsStringGlue.h>
#include <nsITimer.h>
#include <nsCOMPtr.h>
#include <nsIObserver.h>
#include <nsIStringBundle.h>
#include <nsTArray.h>
#include <nsAutoPtr.h>

#include <sbIDataRemote.h>
#include <sbIMediacoreManager.h>

#include "sbIFileMetadataService.h"
#include "sbMetadataJob.h"


// DEFINES ====================================================================
#define SONGBIRD_FILEMETADATASERVICE_CONTRACTID \
"@songbirdnest.com/Songbird/FileMetadataService;1"

#define SONGBIRD_FILEMETADATASERVICE_CLASSNAME \
"Songbird File Metadata Service"

#define SONGBIRD_FILEMETADATASERVICE_DESCRIPTION \
"Songbird File Metadata Service - Reads and writes media file metadata"

// {183C63A5-8195-4294-8D41-A21FE16A0D7C}
#define SONGBIRD_FILEMETADATASERVICE_CID \
{0x183C63A5, 0x8195, 0x4294, {0x8D, 0x41, 0xA2, 0x1F, 0xE1, 0x6A, 0x0D, 0x7C}}


// CLASSES ====================================================================
class sbMetadataJob;
class sbMetadataJobItem;
class sbMainThreadMetadataProcessor;
class sbBackgroundThreadMetadataProcessor;
class sbMetadataCrashTracker;

/**
 * \class sbFileMetadataService
 * \brief Manages reading and writing metadata to and from 
 *        sbIMediaItem objects and media files
 *
 * Supports the creation of threaded read/write jobs for local media 
 * (URIs beginning with file://), and timer driven read jobs for remote media.
 *
 *
 * Simplified Architecture:
 *
 *   - sbFileMetadataService creates and owns all active 
 *     sbMetadataJobs, which are representations of user read/write requests.
 *   - sbMetadataJob keeps sbMetadataJobItems in a waiting list, and is
 *     responsible for managing sbIJobProgress requests.
 *   - sbFileMetadataService owns two job processors, 
 *     sbBackgroundThreadMetadataProcessor and sbMainThreadMetadataProcessor,
 *     which pull sbMetadataJobItems from waiting jobs, run the associated
 *     sbIMetadataJobHandlers, and then give the items back. 
 *   - When sbMetadataJobItems are returned to a sbMetadataJob, they are 
 *     handled by reading the found properties (if needed) and tracking 
 *     progress.
 *   - sbFileMetadataService contains a timer that drives sbIJobProgress 
 *     notification.
 *
 * [ Processor ]        [ Service ]
 *      |                    |-Create-------->[ Job ]-Create--->[ Handler ]
 *      |<------------Start--|                   |                   |
 *      |                    |                   |                   |
 *      |--GetQueuedItem---->|-GetQueuedItem---->|                   |
 *      |                    |                   |                   |
 *      |--Read/Write----------------------------------------------->|
 *      |                    |                   |
 *      |--PutProcessedItem->|-PutProcessedItem->|
 *
 *
 * The complexity results from the fact that much of the work can only
 * be done on the main thread (due to limitations of sbIMetadataHandlers 
 * and Mozilla APIs).
 *
 * In theory we could do everything on the main thread, but since
 * some handlers do blocking IO this is very undesirable. Instead 
 * we run some safe sbIMetadataHandlers on a background thread, and 
 * use timers to do all other work on the main thread.
 *
 *
 * Additional Notes:
 *
 * All interactions other than GetQueuedJobItem and PutProcessedJobItem must
 * be performed on the main thread.
 *
 * \sa sbIJobProgress, sbIFileMetadataService
 */
class sbFileMetadataService : public sbIFileMetadataService, public nsIObserver
{
public:
  
  NS_DECL_ISUPPORTS
  NS_DECL_SBIFILEMETADATASERVICE
  NS_DECL_NSIOBSERVER

  sbFileMetadataService();
  virtual ~sbFileMetadataService();
  
  /** 
   * Called automatically on first GetService()
   */
  nsresult Init();
  
  
  /**
   * Take a waiting job item for processing from the next
   * active job.
   *
   * Rotates through available jobs so that they are
   * processed in parallel.
   *
   * May be called off of the main thread.
   *
   * \param aMainThreadOnly If true, will return a job item that needs
   *        to be run on the main thread due to implementation details.
   * \param aJobItem The job item to be processed
   * \return NS_ERROR_NOT_AVAILABLE if there are no more items
   */
  nsresult GetQueuedJobItem(PRBool aMainThreadOnly, sbMetadataJobItem** aJobItem);
  
  /**
   * Give back a job item after processing has been attempted.
   *
   * May be called off of the main thread.
   *
   * \param aJobItem The job item that has been processed
   */
  nsresult PutProcessedJobItem(sbMetadataJobItem* aJobItem);

  /**
   * If completion of the job item specified by aJobItem is blocked (e.g.,
   * writing to a playing file), return true in aJobItemIsBlocked; otherwise,
   * return false.
   *
   * \param aJobItem            Job item to check.
   * \param aJobItemIsBlocked   If true, job item is blocked.
   */
  nsresult GetJobItemIsBlocked(sbMetadataJobItem* aJobItem,
                               PRBool*            aJobItemIsBlocked);


protected:
  
  /**
   * \brief Proxied version of RestartProcessors present on the
   *        sbIFileMetadataService interface.
   */
  nsresult ProxiedRestartProcessors(PRUint16 aProcessorsToRestart);
  /**
   * Calls StartJob on the main thread, proxying if necessary.
   *
   * \param aMediaItemsArray Array of sbIMediaItems
   * \param aRequiredProperties String Enumerator of the properties to be
   *        written, null for read.
   * \param aJobType Operation to perform (TYPE_READ or TYPE_WRITE)
   * \returns An sbIJobProgress interface used to track the work.
   */
  nsresult ProxiedStartJob(nsIArray* aMediaItemsArray,
                           nsIStringEnumerator* aRequiredProperties,
                           sbMetadataJob::JobType aJobType,
                           sbIJobProgress** _retval);
  
  /**
   * Create and begin a new job to process the given media items
   *
   * Notes:
   *   - *** MAIN THREAD ONLY ***
   *   - All media items must be from the same library
   *
   * \param aMediaItemsArray Array of sbIMediaItems
   * \param aRequiredProperties String Enumerator of the properties to be
   *        written, null for read.
   * \param aJobType Operation to perform (TYPE_READ or TYPE_WRITE)
   * \returns An sbIJobProgress interface used to track the work.
   */
  nsresult StartJob(nsIArray* aMediaItemsArray,
                    nsIStringEnumerator* aRequiredProperties,
                    sbMetadataJob::JobType aJobType,
                    sbIJobProgress** _retval);
  
  /** 
   * Called automatically on library manager shutdown
   */
  nsresult Shutdown();
    
  /**
   * Return NS_OK if write jobs are permitted, or 
   * NS_ERROR_NOT_AVAILABLE if not.
   *
   * May prompt the user to permit write jobs.
   */
  nsresult EnsureWritePermitted();
  
  /**
   * Update the legacy dataremote flags based on the job count.
   * Note that job count is passed as a param because UpdateDataRemotes
   * is to be called while mJobArray is locked.
   */
  nsresult UpdateDataRemotes(PRInt64 aJobCount);
  
  // Legacy dataremote used to indicate metadata status
  nsCOMPtr<sbIDataRemote>                  mDataCurrentMetadataJobs;
  
  // Job processors
  nsRefPtr<sbMainThreadMetadataProcessor>  mMainThreadProcessor;
  nsRefPtr<sbBackgroundThreadMetadataProcessor>  
                                           mBackgroundThreadProcessor;

  PRBool                                   mInitialized;
  PRBool                                   mRunning;
  
  // Timer used to call Job.OnJobProgress()
  nsCOMPtr<nsITimer>                       mNotificationTimer;
  
  // Lock to protect mJobArray and mNextJobIndex
  PRLock*                                  mJobLock;
  
  // List of all active jobs
  nsTArray<nsRefPtr<sbMetadataJob> >       mJobArray;
  
  // Index into mJobArray to use with the next call to GetQueuedJobItem.
  // Used to spread processing between all active jobs
  PRUint32                                 mNextJobIndex;
  
  // Used to keep track of what we're processing, and 
  // blacklist files that seem to destroy the app
  nsRefPtr<sbMetadataCrashTracker>         mCrashTracker;

  // Mediacore manager;
  nsCOMPtr<sbIMediacoreManager>            mMediacoreManager;
};

#endif // SBFILEMETADATASERVICE_H__
