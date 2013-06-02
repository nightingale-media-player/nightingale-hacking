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
 * \file sbMetadataJob.h
 * \brief A container of sbMetadataJobItems 
 */

#ifndef SBMETADATAJOB_H_
#define SBMETADATAJOB_H_

// INCLUDES ===================================================================

#include <nscore.h>
#include <prlock.h>
#include <mozilla/Mutex.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <nsCOMArray.h>
#include <nsCOMPtr.h>
#include <nsAutoPtr.h>
#include <nsIClassInfo.h>

#include <sbIAlbumArtListener.h>
#include <sbIMediaItem.h>
#include <sbIJobProgress.h>
#include <sbIJobProgressUI.h>
#include <sbIJobCancelable.h>
#include <sbIPropertyManager.h>

#include <sbMediaListBatchCallback.h>

#include <set>


#define SB_METADATAJOB_CID                                                    \
{ /* b65fe178-f728-4ce0-8d41-157f84225635 */                                  \
	0xB65FE178,                                                               \
	0xF728,                                                                   \
	0x4CE0,                                                                   \
	{ 0x8D, 0x41, 0x15, 0x7F, 0x84, 0x22, 0x56, 0x35 }                        \
}


// CLASSES ====================================================================
typedef std::set<nsString> sbStringSet;

class sbMetadataJobItem;
class sbIMutablePropertyArray;
class sbIAlbumArtFetcherSet;

/**
 * \class sbMetadataJob
 *
 * A container of sbMetadataJobItems representing a request to
 * sbFileMetadataService. Provides feedback to the caller via sbIJobProgress,
 * and allows the request to be cancelled via sbIJobCancelable.
 *
 * Expects job items to be taken from the container, processed, and
 * then returned in a completed state. Responsible for preparing
 * media item properties when writing, and filtering and setting
 * propertes when reading.
 *
 * All interactions other than GetQueuedItem and PutProcessedItem must
 * be performed on the main thread.
 */
class sbMetadataJob : public nsIClassInfo,
                      public sbIJobProgressUI,
                      public sbIJobCancelable,
                      public sbIAlbumArtListener
{
  friend class sbMediaListBatchCallback;
  
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBIJOBPROGRESS
  NS_DECL_SBIJOBPROGRESSUI
  NS_DECL_SBIJOBCANCELABLE
  NS_DECL_SBIALBUMARTLISTENER
  
  enum JobType 
  {
    TYPE_READ  = 0,
    TYPE_WRITE = 1
  };

  sbMetadataJob();
  virtual ~sbMetadataJob();

  /**
   * Initialize the job with the given media items. Called
   * by sbFileMetadataService when a request is received.
   *
   * Notes:
   *   - *** MAIN THREAD ONLY ***
   *   - All media items must be from the same library
   *
   * \param aMediaItemsArray Array of sbIMediaItems
   * \param aRequiredProperties Array of nsStrings for properties to use.
   * \param aJobType Operation to perform (TYPE_READ or TYPE_WRITE)
   */
  nsresult Init(nsIArray *aMediaItemsArray,
                nsIStringEnumerator* aRequiredProperties,
                JobType aJobType);

  /**
   * Add additional media items to the job.
   *
   * Notes:
   *   - *** MAIN THREAD ONLY ***
   *   - All media items must be from the same library
   *
   * \param aMediaItemsArray Array of sbIMediaItems
   */
  nsresult AppendMediaItems(nsIArray* aMediaItemsArray);

  /**
   * Append an existing job item to the queue.
   * \param aJobItem The job item to append to the queue.
   */
  nsresult AppendJobItem(sbMetadataJobItem* aJobItem);

  /**
   * Notify listeners of job progress.  
   * To be called periodically by the main thread timer in
   * sbFileMetadataService.
   */
  nsresult OnJobProgress();
  
  /**
   * Take a waiting job item for processing.  Will prepare
   * job item properties and handler appropriately
   *
   * May be called off of the main thread.
   *
   * \param aMainThreadOnly If true, will return a job item that needs
   *        to be run on the main thread due to implementation details.
   * \param aJobItem The job item to be processed
   * \return NS_ERROR_NOT_AVAILABLE if there are no more items in the queue
   */
  nsresult GetQueuedItem(PRBool aMainThreadOnly, sbMetadataJobItem** aJobItem);
  
  /**
   * Give back a job item after processing has been attempted.
   * Records progress and may perform additional processing on the item.
   *
   * May be called off of the main thread.
   *
   * \param aJobItem The job item that has been processed
   */
  nsresult PutProcessedItem(sbMetadataJobItem* aJobItem);
  
  /**
   * TYPE_READ or TYPE_WRITE
   */
  nsresult GetType(JobType* aJobType);

  /**
   * Set the job blocked status to the value specified by aBlocked.
   *
   * \param aBlocked Job blocked status.
   */
  nsresult SetBlocked(PRBool aBlocked);
  
  
private:
  
  /**
   * Set up an sbIMetadataHandler for the given job item.
   *
   * *** MAIN THREAD ONLY ***
   *
   * Must be called on the main thread, as setting up the
   * handler requires non-threadsafe Mozilla components.
   */
  nsresult SetUpHandlerForJobItem(sbMetadataJobItem* aJobItem);

  /**
   * Take the properties from a media item, filter them, 
   * and set them on the associated sbIMetadataHandler. Called on
   * GetQueuedItem during write jobs.
   */
  nsresult PrepareWriteItem(sbMetadataJobItem* aJobItem);

  /**
   * Complete the given job item by copying 
   * read properties or logging any errors.
   * *** MAIN THREAD ONLY ***
   */
  nsresult HandleProcessedItem(sbMetadataJobItem *aJobItem);

  /**
   * Handle things needed after a write job has completed. 
   * *** MAIN THREAD ONLY ***
   */
  nsresult HandleWrittenItem(sbMetadataJobItem *aJobItem);
  
  /**
   * Saves the job item into mProcessedBackgroundThreadItems
   * for later main thread completion by BatchCompleteItems().
   * To be called from background threads
   */
  nsresult DeferProcessedItemHandling(sbMetadataJobItem *aJobItem);

  /**
   * Take properties from a job item and set them on the
   * associated media item.  Called on PutProcessedItem
   * during read jobs.
   * *** MAIN THREAD ONLY ***
   */
  nsresult CopyPropertiesToMediaItem(sbMetadataJobItem* aJobItem,
                                     PRBool* aWillRetry);
  
  /**
   * Trigger an album art scan for the given job item.
   * Assumes that the sbIMetadataHandler for the item
   * is still open
   */
  nsresult ReadAlbumArtwork(sbMetadataJobItem* aJobItem);

  /**
   * Add the given job item to mErrorMessages.
   * Must be called on the main thread, since it uses 
   * URIs and the IOService
   * \param aJobItem The job.
   * \param aShouldRetry Attempt to read metadata with another handler.
   * \param aWillRetry Indicates if another attempt will be made. This is
   *                   always false if aShouldRetry is false. This must
   *                   never be null if aShouldRetry is true!
   */
  nsresult HandleFailedItem(sbMetadataJobItem* aJobItem, 
                            PRBool aShouldRetry = PR_FALSE, 
                            PRBool *aWillRetry = nsnull);

  /**
   * Empty mProcessedBackgroundThreadItems in a library batch.  
   * *** MAIN THREAD ONLY ***
   */
  nsresult BatchCompleteItems();

  /**
   * sbMediaListBatchCallbackFunc used to perform
   * batch sbIMediaItem setProperties calls.
   * Calls this->BatchCompleteItemsCallback()
   */
  static nsresult RunLibraryBatch(nsISupports* aUserData);
  
  /**
   * Finish emptying mProcessedBackgroundThreadItems.
   */
  nsresult BatchCompleteItemsCallback();

  /**
   * Let mLibrary's listeners know that we're going
   * to be doing a lot of work.
   * NOTE: MUST remember to call EndLibraryBatch, 
   * or can leave listeners with unbalanced 
   * batch counts.
   */
  nsresult BeginLibraryBatch();
  nsresult EndLibraryBatch();


  /**
   * Get the unescaped file name for the given media item
   */
  nsresult CreateDefaultItemName(sbIMediaItem* aMediaItem,
                                 nsAString& retval);

  /**
   * If the given property key-value pair is valid, append it to 
   * a property array.
   */
  nsresult AppendToPropertiesIfValid(sbIPropertyManager* aPropertyManager,
                                     sbIMutablePropertyArray* aProperties, 
                                     const nsAString& aID, 
                                     const nsAString& aValue);
  /**
   * Find the size of the file associated with the given media item.
   * *** MAIN THREAD ONLY ***
   */
  nsresult GetFileSize(sbIMediaItem* aMediaItem, PRInt64* aFileSize);
  
  /**
   * Helper to get a string from the Songbird string bundle
   */
  nsresult LocalizeString(const nsAString& aName, nsAString& aValue);
  
  
  // sbIJobProgress variables
  PRUint16                                 mStatus;
  PRBool                                   mBlocked;
  PRUint32                                 mCompletedItemCount;
  PRUint32                                 mTotalItemCount;
  nsTArray<nsString>                       mErrorMessages;
  nsString                                 mTitleText;
  nsString                                 mStatusText;
  nsCOMArray<sbIJobProgressListener>       mListeners;
  
  // TYPE_READ or TYPE_WRITE
  JobType                                  mJobType;
  
  // The library to which all items in this job belong 
  // (since batch operations are per-library) 
  nsCOMPtr<sbILibrary>                     mLibrary;
  
  // List of properties we require for this job
  nsTArray<nsString>                       mRequiredProperties;

  // List of absolute paths that the watch folder service is ignoring.
  sbStringSet                              mIgnoredContentPaths;
  
  // List of job items that MUST be processed on the 
  // main thread due to sbIMetadataHandler limitations
  nsTArray<nsRefPtr<sbMetadataJobItem> >   mMainThreadJobItems;
  PRUint32                                 mNextMainThreadIndex;
  
  // List of job items that may be processed off of the main thread
  // TODO consider using nsDeque
  nsTArray<nsRefPtr<sbMetadataJobItem> >   mBackgroundThreadJobItems;
  PRUint32                                 mNextBackgroundThreadIndex;
  mozilla::Mutex                           mBackgroundItemsLock;
  
  // Pointer to a list of items that have been returned from processing, but have 
  // not yet had their properties set.
  // Used to perform sbIMediaItem.setProperties() batching.
  nsAutoPtr<nsTArray<nsRefPtr<sbMetadataJobItem> > >  
                                           mProcessedBackgroundThreadItems;
  mozilla::Mutex                           mProcessedBackgroundItemsLock;
  
  // Indicates that we've started a library batch, and need
  // to close it before we complete
  PRBool                                   mInLibraryBatch;

  // Cached art fetcher instance
  nsCOMPtr<sbIAlbumArtFetcherSet>          mArtFetcher;
  
  nsCOMPtr<nsIStringBundle>                mStringBundle;
};

#endif // SBMETADATAJOB_H_
