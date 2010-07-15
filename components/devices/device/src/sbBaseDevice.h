/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
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
 *=END SONGBIRD GPL
 */

#ifndef __SBBASEDEVICE__H__
#define __SBBASEDEVICE__H__

#include <list>
#include <map>

#include "sbIDevice.h"
#include "sbBaseDeviceEventTarget.h"
#include "sbBaseDeviceVolume.h"
#include "sbDeviceLibrary.h"

#include <nsAutoPtr.h>
#include <nsClassHashtable.h>
#include <nsCOMPtr.h>
#include <nsCOMArray.h>
#include <nsDataHashtable.h>
#include <nsIRunnable.h>
#include <nsISupportsImpl.h>
#include <nsIThread.h>
#include <nsITimer.h>
#include <nsIFile.h>
#include <nsIURL.h>
#include <nsTArray.h>
#include <prlock.h>

#include <sbAutoRWLock.h>
#include <sbILibraryChangeset.h>
#include <sbIMediaItem.h>
#include <sbIMediaList.h>
#include <sbITemporaryFileFactory.h>
#include <sbITranscodeManager.h>

#include <sbMemoryUtils.h>
#include <nsComponentManagerUtils.h>

#include "sbDeviceStatistics.h"

class nsIPrefBranch;

class sbAutoIgnoreWatchFolderPath;
class sbBaseDeviceLibraryListener;
class sbDeviceBaseLibraryCopyListener;
class sbDeviceSupportsItemHelper;
class sbDeviceTranscoding;
class sbDeviceImages;
class sbBaseDeviceMediaListListener;
class sbIDeviceInfoRegistrar;
class sbIDeviceLibrary;
class sbITranscodeProfile;
class sbITranscodeAlbumArt;
class sbIMediaFormat;

/* Property used to force a sync diff. */
#define DEVICE_PROPERTY_SYNC_FORCE_DIFF \
          "http://songbirdnest.com/device/1.0#forceDiff"

// Device property XML namespace.
#define SB_DEVICE_PROPERTY_NS "http://songbirdnest.com/device/1.0"

#define SB_SYNC_PARTNER_PREF NS_LITERAL_STRING("SyncPartner")

/**
 * Base class for implementing a device
 *
 * Derived classes should
 *  - QI to sbIDevice, sbIDeviceEventTarget
 *  - call sbBaseDevice::Init() at some early point
 *  - Set the hidden property to false on the library when done mounting.
 *  - Set the hidden property on all medialists in the library when
 *    done mounting.
 */
class NS_HIDDEN sbBaseDevice : public sbIDevice,
                               public sbBaseDeviceEventTarget
{
public:
  // Friend declarations for classes used to divide up device work
  friend class sbDeviceTranscoding;
  friend class sbDeviceImages;
  friend class sbBatchCleanup;
  friend class sbBaseDeviceVolume;

  struct TransferRequest : public nsISupports {
    /* types of requests. not all types necessarily apply to all types of
     * devices, and devices may have custom types.
     * Request types that should result in delaying shutdown (e.g. writing to
     * the device) should have the write flag set.
     * The specific device may internally define more request types.
     */

    static const int REQUEST_FLAG_USER  = sbIDevice::REQUEST_FLAG_USER;
    static const int REQUEST_FLAG_WRITE = sbIDevice::REQUEST_FLAG_WRITE;

    /* note that type 0 is reserved */
    enum {
      /* read requests */
      REQUEST_MOUNT         = sbIDevice::REQUEST_MOUNT,
      REQUEST_READ          = sbIDevice::REQUEST_READ,
      REQUEST_EJECT         = sbIDevice::REQUEST_EJECT,
      REQUEST_SUSPEND       = sbIDevice::REQUEST_SUSPEND,

      /* not in sbIDevice, internal use */
      REQUEST_BATCH_BEGIN,
      REQUEST_BATCH_END,

      /* write requests */
      REQUEST_WRITE         = sbIDevice::REQUEST_WRITE,
      REQUEST_DELETE        = sbIDevice::REQUEST_DELETE,
      REQUEST_SYNC          = sbIDevice::REQUEST_SYNC,
      REQUEST_IMAGESYNC     = sbIDevice::REQUEST_IMAGESYNC,
      /* delete all files */
      REQUEST_WIPE          = sbIDevice::REQUEST_WIPE,
      /* move an item in one playlist */
      REQUEST_MOVE          = sbIDevice::REQUEST_MOVE,
      REQUEST_UPDATE        = sbIDevice::REQUEST_UPDATE,
      REQUEST_NEW_PLAYLIST  = sbIDevice::REQUEST_NEW_PLAYLIST,
      REQUEST_FORMAT        = sbIDevice::REQUEST_FORMAT
    };

    enum {
      REQUESTBATCH_UNKNOWN       = 0,
      REQUESTBATCH_AUDIO         = 1,
      REQUESTBATCH_VIDEO         = 2,
      REQUESTBATCH_IMAGE         = 4
    };

    int type;                        /* one of the REQUEST_* constants,
                                          or a custom type */
    nsCOMPtr<sbIMediaItem> item;     /* the item this request pertains to */
    nsCOMPtr<sbIMediaList> list;     /* the list this request is to act on */
    nsCOMPtr<nsISupports>  data;     /* optional data that may or may not be used by a type of request */
    sbITranscodeProfile * transcodeProfile; /* Transcode profile to use if available
                                               this is manually ref counted so
                                               we don't have all of our consumers
                                               have to include the transcode profile
                                               directory */
    PRUint32 index;                  /* the index in the list for this action */
    PRUint32 otherIndex;             /* any secondary index needed */
    PRUint32 batchCount;             /* the number of items in this batch
                                          (batch = run of requests of the same
                                          type) */
    PRUint32 batchIndex;             /* index of item in the batch to process */

    PRUint32 itemTransferID;         /* id for this item transfer */

    static const PRInt32 PRIORITY_HIGHEST   = 0x0000000;
    static const PRInt32 PRIORITY_HIGH      = 0x1000000;
    static const PRInt32 PRIORITY_DEFAULT   = 0x2000000;
    static const PRInt32 PRIORITY_LOW       = 0x3000000;
    static const PRInt32 PRIORITY_LOWEST    = 0x4000000;
    PRInt32 priority;                /* priority for the request (lower first) */

    PRInt64 timeStamp;               /* time stamp of when request was
                                          enqueued */
    PRUint32 batchID;                /* ID of request batch */
    PRUint32 itemType;               /* Item type: Audio, Video... */

    /* Write request fields. */
    PRBool contentSrcSet;            /* if true, the content source URI for the
                                        destination item has been set */
    PRBool destinationMediaPresent;  /* if true, the destination media is
                                        present on the device (e.g., it's been
                                        copied or transcoded to the device) */
    /* This determines how much this item will be transferred onto the device */
    typedef enum {
      COMPAT_UNSUPPORTED,            /* this item cannot be transferred */
      COMPAT_SUPPORTED,              /* this item is directly supported on the device */
      COMPAT_NEEDS_TRANSCODING       /* this item needs to be transcoded */
    } CompatibilityType;
    CompatibilityType destinationCompatibility;
    PRBool transcoded;               /* if true, write item media was
                                        transcoded */
    nsCOMPtr<sbITranscodeAlbumArt> albumArt; /* Album art transcoding object,
                                                or null if no album art
                                                transcoding should be done */
    /* Factory for creating temporary files.  All temporary files are removed
       when factory is destroyed. */
    nsCOMPtr<sbITemporaryFileFactory> temporaryFileFactory;

    NS_DECL_ISUPPORTS
    /**
     * Returns PR_TRUE if the request is for a playlist and PR_FALSE otherwise
     */
    PRBool IsPlaylist() const;
    /**
     * Returns PR_TRUE if the request should be counted as part of he batch
     * otherwise returns PR_FALSE
     */
    PRBool IsCountable() const;

    static TransferRequest * New();

    /* Don't allow manual construction/destruction, but allow sub-classing. */
  protected:
    TransferRequest();
    ~TransferRequest(); /* we're refcounted, no manual deleting! */
  };

  typedef std::list<nsRefPtr<TransferRequest> > Batch;

public:
  /* selected methods from sbIDevice */
  NS_IMETHOD Connect();
  NS_IMETHOD Disconnect();
  NS_IMETHOD GetPreference(const nsAString & aPrefName, nsIVariant **_retval);
  NS_IMETHOD SetPreference(const nsAString & aPrefName, nsIVariant *aPrefValue);
  NS_IMETHOD GetIsDirectTranscoding(PRBool *aIsDirect);
  NS_IMETHOD GetIsBusy(PRBool *aIsBusy);
  NS_IMETHOD GetCanDisconnect(PRBool *aCanDisconnect);
  NS_IMETHOD GetState(PRUint32 *aState);
  NS_IMETHOD GetPreviousState(PRUint32 *aState);
  NS_IMETHOD SyncLibraries(void);
  /**
   * return NS_ERROR_ABORT if user aborts the ejection while playback is on
   * device item.
   */
  NS_IMETHOD Eject(void);
  NS_IMETHOD Format(void);
  NS_IMETHOD GetSupportsReformat(PRBool *_retval);
  NS_IMETHOD GetCacheSyncRequests(PRBool *_retval);
  NS_IMETHOD SetCacheSyncRequests(PRBool aChacheSyncRequests);
  NS_IMETHOD SupportsMediaItem(sbIMediaItem*                  aMediaItem,
                               sbIDeviceSupportsItemCallback* aCallback);
  NS_IMETHOD GetDefaultLibrary(sbIDeviceLibrary** aDefaultLibrary);
  NS_IMETHOD SetDefaultLibrary(sbIDeviceLibrary* aDefaultLibrary);

public:
  /**
   * Starting value for batch indexes
   */
  static PRUint32 const BATCH_INDEX_START = 1;
  sbBaseDevice();
  virtual ~sbBaseDevice();

  /* add a transfer/action request to the request queue */
  nsresult  PushRequest(const int aType,
                        sbIMediaItem* aItem = nsnull,
                        sbIMediaList* aList = nsnull,
                        PRUint32 aIndex = PR_UINT32_MAX,
                        PRUint32 aOtherIndex = PR_UINT32_MAX,
                        PRInt32 aPriority = TransferRequest::PRIORITY_DEFAULT);

  virtual nsresult PushRequest(TransferRequest *aRequest);

  /* remove the next request to be processed; note that _retval will be null
     while returning NS_OK if there are no requests left */
  nsresult PopRequest(TransferRequest** _retval);

  /**
   * Returns the next request batch in aBatch and marks it as the current
   * request.  The batch may be a single request if the next request is not part
   * of a batch.
   * \param aBatch A collection that receives the batch items found.
   */
  nsresult StartNextRequest(Batch & aBatch);

  /**
   * Complete the current request (single or batch) and clear the current
   * request.
   */
  nsresult CompleteCurrentRequest();

  /* get a reference to the next request without removing it; note that _retval
   * will be null while returning NS_OK if there are no requests left
   * once a request has been peeked, it will be returned for further PeekRequest
   * and PopRequest calls until after it has been popped.
   */
  nsresult PeekRequest(TransferRequest** _retval);

  /* remove a given request from the transfer queue
     note: returns NS_OK on sucessful removal,
           and NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA if no match was found.
     if the request is not unique, removes the first matching one
     (note that it is not possible to filter by priority) */
  nsresult RemoveRequest(const int aType,
                         sbIMediaItem* aItem = nsnull,
                         sbIMediaList* aList = nsnull);

  /**
   * clear the request queue
   * \param aSetCancel Determines whether or not the current request is "canceled"
   */
  nsresult ClearRequests(bool aSetCancel = true);

  /**
   * Return in aRequestType the request type of the request batch specified by
   * aBatch.
   *
   * \param aBatch              Request batch.
   * \param aRequestType        Batch request type.
   */
  nsresult BatchGetRequestType(sbBaseDevice::Batch& aBatch,
                               int*                 aRequestType);

  /**
   * Internally set preference specified by aPrefName to the value specified by
   * aPrefValue.  If the new preference value is different than that old, return
   * true in aHasChanged unless aHasChanged is null.
   * This method may be used for loading preferences from device storage.  It
   * will not send preference changed events.
   *
   * @param aPrefBranch The branch to set preferences on.
   * @param aPrefName Name of the preference to set.
   * @param aPrefValue Value to set preference.
   * @param aHasChanged True if preference value changed.
   */

  nsresult SetPreferenceInternal(nsIPrefBranch*   aPrefBranch,
                                 const nsAString& aPrefName,
                                 nsIVariant*      aPrefValue,
                                 PRBool*          aHasChanged);

  /**
   * Return true in aHasPreference if the device has the preference specified by
   * aHasPreference.
   * @param aPrefName The preference to check.
   * @aHasPreference Returned true if device has preference.
   */
  nsresult HasPreference(nsAString& aPrefName,
                         PRBool*    aHasPreference);

  /* A request has been added, process the request
     (or schedule it to be processed) */
  virtual nsresult ProcessRequest() = 0;

  /**
   * Set the device state
   * @param aState new device state
   */
  nsresult SetState(PRUint32 aState);

  /**
   * Set the device's previous state
   * @param aState new device state
   */
  nsresult SetPreviousState(PRUint32 aState);

  /**
   * Create a local database library for the device.  The library must be
   * finalized by calling FinalizeDeviceLibrary.
   * @param aId the ID for the library
   * @param aLibraryLocation the file to name the library, or null to use some default
   * @return the library created (or re-used if it exists)
   */
  nsresult CreateDeviceLibrary(const nsAString& aId,
                               nsIURI* aLibraryLocation,
                               sbIDeviceLibrary** _retval);

  /**
   * Initialize a library for the device.  The library must be finalized by
   * calling FinalizeDeviceLibrary.
   * @param aDevLib the device library to initialize.
   * @param aId the ID for the library
   * @param aLibraryLocation the file to name the library, or null to use some default
   */
  nsresult InitializeDeviceLibrary(sbDeviceLibrary* aDevLib,
                                   const nsAString& aId,
                                   nsIURI*          aLibraryLocation);

  /**
   * Initialize the preferences for the device library specified by aDevLib.
   * @param aDevLib the device library for which to initialize preferences.
   */
  nsresult InitializeDeviceLibraryPreferences(sbDeviceLibrary* aDevLib);

  /**
   * Finalize a library for the device
   * @param aDevLib the device library to finalize.
   */
  void FinalizeDeviceLibrary(sbIDeviceLibrary* aDevLib);

  /**
   * Add the library specified by aDevLib to the device content.
   * @param aDevLib the device library to add.
   */
  nsresult AddLibrary(sbIDeviceLibrary* aDevLib);

  /**
   * Check the library access (read-only vs. read-write) of the device library
   * specified by aDevLib and prompt the user if needed.
   * @param aDevLib the device library to check.
   */
  virtual nsresult CheckAccess(sbIDeviceLibrary* aDevLib);

  /**
   * Remove the library specified by aDevLib from the device content.
   * @param aDevLib the device library to remove.
   */
  nsresult RemoveLibrary(sbIDeviceLibrary* aDevLib);

  /**
   * Update the property with the ID specified by aPropertyID to the value
   * specified by aPropertyValue in the library specified by aLibrary if
   * aPropertyValue is different than the current property value.
   *
   * \param aLibrary            Library to update.
   * \param aPropertyID         ID of property to update.
   * \param aPropertyValue      Property value to which to update.
   */
  nsresult UpdateLibraryProperty(sbILibrary*      aLibrary,
                                 const nsAString& aPropertyID,
                                 const nsAString& aPropertyValue);

  /**
   * Update the default library to the library specified by aDevLib.
   * @param aDevLib the device library to which to update the default library.
   */
  nsresult UpdateDefaultLibrary(sbIDeviceLibrary* aDevLib);

  /**
   * Handle a change to the default library.
   */
  virtual nsresult OnDefaultLibraryChanged();

  /**
   * Called when a media list has been added to the device library
   * @param aList the media list to listen for modifications
   */
  nsresult ListenToList(sbIMediaList* aList);

  typedef struct {
    sbBaseDevice*        device;
    nsCOMPtr<sbILibrary> library;
  } EnumerateFinalizeMediaListListenersInfo;

  static NS_HIDDEN_(PLDHashOperator)
           EnumerateFinalizeMediaListListeners
            (nsISupports* aKey,
             nsRefPtr<sbBaseDeviceMediaListListener>& aData,
             void* aClosure);

  static NS_HIDDEN_(PLDHashOperator) EnumerateIgnoreMediaListListeners(nsISupports* aKey,
                                                                       nsRefPtr<sbBaseDeviceMediaListListener> aData,
                                                                       void* aClosure);

  /**
  * Set the ignore flag on all media list listeners registered for
  * the library for this device.
  * \param aIgnoreListener Ignore flag value.
  */
  nsresult SetIgnoreMediaListListeners(PRBool aIgnoreListener);

  /**
   * Set the ignore flag on all library listeners registered for the
   * library for this device
   * \param aIgnoreListener Ignore flag value.
   */
  nsresult SetIgnoreLibraryListener(PRBool aIgnoreListener);

  /**
   * Set all media lists in the library hidden. This is useful
   * for hiding the lists during mounting operations.
   * \param aLibrary The library containing the media lists you wish to hide.
   * \param aHidden True to hide, false to show.
   */
  nsresult SetMediaListsHidden(sbIMediaList *aLibrary, PRBool aHidden);

  /**
   * Ignores events for the given media item
   */
  nsresult IgnoreMediaItem(sbIMediaItem * aItem);

  /**
   * Restores listening to events for the given item
   */
  nsresult UnignoreMediaItem(sbIMediaItem * aItem);

  /**
   * Delete an item from the library and suppress notifications during
   * delete. This is to avoid having the listeners watching the library
   * attempt to double delete an item.
   */
  nsresult DeleteItem(sbIMediaList *aLibrary, sbIMediaItem *aItem);

  /**
   * Return in aContentType the type of content contained in the media item
   * specified by aMediaItem.  The content type values are as defined in
   * sbIDeviceCapabilities.
   *
   * Sub-classes may override this method to provide custom content type
   * determination.
   *
   * \param aMediaItem          Media item for which to get content type.
   * \param aContentType        Returned content type.
   */
  virtual nsresult GetItemContentType(sbIMediaItem* aMediaItem,
                                      PRUint32*     aContentType);

  nsresult CreateTransferRequest(PRUint32 aRequest,
                                 nsIPropertyBag2 *aRequestParameters,
                                 TransferRequest **aTransferRequest);

  /**
   * Create an event for the device and dispatch it
   * @param aType type of event
   * @param aData event data
   * @param aAsync if true, dispatch asynchronously
   * @param aTarget event target
   */
  nsresult CreateAndDispatchEvent(PRUint32 aType,
                                  nsIVariant *aData,
                                  PRBool aAsync = PR_TRUE,
                                  sbIDeviceEventTarget* aTarget = nsnull);

  /**
   * Create and dispatch an event through the device manager of the type
   * specified by aType with the data specified by aData, originating from the
   * device.  If aAsync is true, dispatch and return immediately; otherwise,
   * wait for the event handling to complete.
   *
   * \param aType                 Type of event.
   * \param aData                 Event data.
   * \param aAsync                If true, dispatch asynchronously.
   */
  nsresult CreateAndDispatchDeviceManagerEvent(PRUint32 aType,
                                               nsIVariant *aData,
                                               PRBool aAsync = PR_TRUE);

  /**
   * Regenerate the Media URL when the media management service is enabled.
   * @note This method will regenerate the content URL for the Media Item.
   */
  nsresult RegenerateMediaURL(sbIMediaItem *aItem, nsIURI **_retval);

  /**
   * Used to generate a filename for an item. This is virtual so different
   * devices can choose to name the file differently. Eventually this logic
   * should be moved to a more centralized place.
   * \param aItem The item we're generating a filename for
   * \param aFilename The filename that was generated. This is can be an escaped
   *                  filename so you may need to unescape or use SetFilename
   *                  on a URL object.
   */
  virtual nsresult GenerateFilename(sbIMediaItem * aItem,
                                    nsACString & aFilename);

  /**
   *   Create a unique media file, starting with the file specified by aFileURI.
   * aFileURI must QI to nsIFileURL.  If the specified file already exists,
   * modify the file name and try again.
   *   If aUniqueFile is not null, return the file object in aUniqueFile.
   *   If aUniqueFileURI is not null, return the file URI in aUniqueFileURI.
   *
   * \param aFileURI First file URI to try creating.
   * \param aUniqueFile Returned unique file object.
   * \param aUniqueFileURI Returned URI of unique file.
   */
  nsresult CreateUniqueMediaFile(nsIURI  *aFileURI,
                                 nsIFile **aUniqueFile,
                                 nsIURI  **aUniqueFileURI);

  /**
   * Generates a URI for the item
   * \param aItem The item we're generating a URI for
   * \param aFilename The URI that was generated
   */
  nsresult
  RegenerateFromDownloadFolder(sbIMediaItem * aItem,
                               nsIURI ** aURI);

  /**
   * Initialize device requests.
   */
  nsresult InitializeRequestThread();

  /**
   * Finalize device requests.
   */
  void FinalizeRequestThread();

  /**
   * Stop processing device requests.
   */
  void ShutdownRequestThread();

  /**
   * Start processing device requests.
   */
  nsresult ReqProcessingStart();

  /**
   * Stop processing device requests.
   */
  nsresult ReqProcessingStop();

  /**
   * Return true if the active request should abort; otherwise, return false.
   *
   * \return PR_TRUE              Active request should abort.
   *         PR_FALSE             Active request should not abort.
   */
  PRBool ReqAbortActive();

  /**
   * Returns true if the current request should be aborted.
   */
  PRBool IsRequestAborted()
  {
    NS_ASSERTION(mRequestMonitor, "mRequestMonitor must be initialized");
    nsAutoMonitor mon(mRequestMonitor);
    return mAbortCurrentRequest;
  }

  /**
   * Returns true if the space has been checked.
   */
  bool GetEnsureSpaceChecked() const {
    return mEnsureSpaceChecked;
  }

  /**
   * Set the checked flag.
   */
  void SetEnsureSpaceChecked(bool aChecked) {
    mEnsureSpaceChecked = aChecked;
  }

  NS_SCRIPTABLE NS_IMETHOD SetWarningDialogEnabled(const nsAString & aWarning, PRBool aEnabled);
  NS_SCRIPTABLE NS_IMETHOD GetWarningDialogEnabled(const nsAString & aWarning, PRBool *_retval);
  NS_SCRIPTABLE NS_IMETHOD ResetWarningDialogs(void);

  /**
   * Returns PR_TRUE if the request has been aborted or the device is
   * disconnected
   */
  PRBool IsRequestAbortedOrDeviceDisconnected() {
    PRUint32 deviceState;
    return (IsRequestAborted() ||
        NS_FAILED(GetState(&deviceState)) ||
        deviceState == sbIDevice::STATE_DISCONNECTED) ? PR_TRUE : PR_FALSE;
  }

  sbDeviceTranscoding * GetDeviceTranscoding() const {
    return mDeviceTranscoding;
  }
  /**
   * This should be used with extreme caution. It is exposed because the
   * MTP property implementation needs to lock the state for formatting
   * so that properties do not block the main thread when formatting
   */
  PRLock * StateLock() {
    return mStateLock;
  }
  /**
   * Return the state without locking. This is for use in conjunction with
   * StateLock
   */
  PRUint32 GetDeviceState() {
    return mState;
  }

  sbDeviceImages* GetDeviceImages() const {
    return mDeviceImages;
  }

  /**
   * Reset the Batch Depth when abort.
   */
  void ResetBatchDepth() {
    PR_AtomicSet(&mBatchDepth, 0);
  }

protected:
  friend class sbBaseDeviceInitHelper;
  friend class sbDeviceEnsureSpaceForWrite;
  friend class sbDeviceStatistics;
  friend class sbDeviceReqAddedEvent;
  /**
   * Base class initialization this will call the InitDevice first then
   * do the initialization needed by the sbDeviceBase
   */
  nsresult Init();
  /**
   * This allows the derived classes to be initialized if needed
   */
  virtual nsresult InitDevice() { return NS_OK; }
private:
  /**
   * Helper for PopRequest / PeekRequest
   */
  nsresult GetFirstRequest(bool aRemove, TransferRequest** _retval);
protected:
  /* to block the background thread from transferring files while the batch
     has not completed; if a delay is active, either the request monitor OR
     the request removal lock needs to be held.  No requests may be removed
     while the removal lock is behind held.  No actions (including pushing
     additional requests) may be performed while the request monitor is held. */
  PRMonitor * mRequestMonitor;
  nsCOMPtr<nsITimer> mBatchEndTimer;
  PRInt32 mNextBatchID;
  /**
   * Tracks the REQUEST_BATCH_BEGIN/REQUEST_BATCH_END depth
   */
  PRInt32 mBatchDepth;
public:
  /**
   * The request queue type
   */
  typedef std::list<nsRefPtr<TransferRequest> > TransferRequestQueue;
  /**
   * The type of collection used to hold request queues based on priority
   */
  typedef std::map<PRInt32, TransferRequestQueue> TransferRequestQueueMap;
protected:

  TransferRequestQueueMap mRequests;
  PRUint32 mLastTransferID;
  PRInt32 mLastRequestPriority; // to make sure peek returns the same
  PRLock *mStateLock;
  PRUint32 mState;
  PRLock *mPreviousStateLock;
  PRUint32 mPreviousState;
  PRBool mHaveCurrentRequest;
  PRBool mAbortCurrentRequest;
  PRInt32 mIgnoreMediaListCount; // Allows us to know if we're ignoring lists
  PRUint32 mPerTrackOverhead; // estimated bytes of overhead per track

  enum {
    SYNC_STATE_NORMAL         = 0,
    SYNC_STATE_CACHE          = 1,
    SYNC_STATE_PENDING        = 2
  };
  PRUint32 mSyncState;            // State of how to handle sync requsts

  nsCOMPtr<sbIDeviceLibrary> mDefaultLibrary;
  nsRefPtr<sbBaseDeviceLibraryListener> mLibraryListener;
  nsRefPtr<sbDeviceBaseLibraryCopyListener> mLibraryCopyListener;
  nsDataHashtableMT<nsISupportsHashKey, nsRefPtr<sbBaseDeviceMediaListListener> > mMediaListListeners;
  nsCOMPtr<sbIDeviceInfoRegistrar> mInfoRegistrar;
  PRUint32 mInfoRegistrarType;
  nsCOMPtr<sbIDeviceCapabilities> mCapabilities;
  PRLock*  mPreferenceLock;
  PRUint32 mMusicLimitPercent;
  sbDeviceTranscoding * mDeviceTranscoding;
  sbDeviceImages *mDeviceImages;

  enum {
    CAN_TRANSCODE_UNKNOWN = 0,
    CAN_TRANSCODE_YES = 1,
    CAN_TRANSCODE_NO = 2
  };
  PRUint32 mCanTranscodeAudio;
  PRUint32 mCanTranscodeVideo;

  bool mVideoInserted; // Flag on whether video is inserted
  PRUint32 mSyncType; // syncing type to pass to the UI
  // Iterator points to the first video request
  TransferRequestQueue::iterator mFirstVideoIterator;

  bool mEnsureSpaceChecked;

  //
  //   mConnected               True if device is connected.
  //   mConnectLock             Connect lock.
  //   mReqAddedEvent           Request added event object.
  //   mReqThread               Request processing thread.
  //   mReqWaitMonitor          Monitor used by the request thread to wait for
  //                            various events.
  //   mReqStopProcessing       Non-zero if request processing should stop.
  //   mIsHandlingRequests      True if requests are being handled.
  //   mDeferredSetupDeviceTimer Timer used to defer presentation of the device
  //                             setup dialog.
  //

  PRRWLock* mConnectLock;
  PRBool mConnected;
  nsCOMPtr<nsIRunnable> mReqAddedEvent;
  nsCOMPtr<nsIThread> mReqThread;
  PRMonitor* mReqWaitMonitor;
  PRInt32 mReqStopProcessing;
  PRInt32 mIsHandlingRequests;
  nsCOMPtr<nsITimer> mDeferredSetupDeviceTimer;

  // cache data for media management preferences
  struct OrganizeData {
    PRBool    organizeEnabled;
    nsCString dirFormat;
    nsCString fileFormat;
    OrganizeData() : organizeEnabled(PR_FALSE) {}
  };
  nsClassHashtableMT<nsIDHashKey, OrganizeData> mOrganizeLibraryPrefs;

  nsClassHashtable<nsUint32HashKey, nsString> mMediaFolderURLTable;

  /**
   * Default per track storage overhead.  10000 is enough for one 8K block plus
   * extra media database or filesystem overhead.
   */
  static const PRUint32 DEFAULT_PER_TRACK_OVERHEAD = 10000;

  /**
   * Percent available space for syncing when building a sync playlist.
   */
  static const PRUint32 SYNC_PLAYLIST_AVAILABLE_PCT = 95;

  /**
   * Amount of time in milliseconds to delay presentation of the device setup
   * dialog.
   */
  static const PRUint32 DEFER_DEVICE_SETUP_DELAY = 2000;

  /**
   * Make sure that there is enough free space for the batch. If there is not
   * enough space for all the items in the batch and the user does not abort
   * the operation, items in the batch will be removed.
   *
   * What items get removed depends on the current sync mode. If we're in
   * manual sync then the last items not fitting will be removed. If we're are
   * in an automatic sync mode then a random subset of items will be removed
   * from the batch.
   *
   * \param aBatch The batch to ensure space for. This collection may be
   *               modified on return.
   */
  nsresult EnsureSpaceForWrite(Batch & aBatch);

  /**
   * Wait for the end of a request batch to be enqueued.
   */
  nsresult WaitForBatchEnd();

  /**
   * Static callback for batch end wait timer.
   */
  static void WaitForBatchEndCallback(nsITimer* aTimer,
                                      void* aClosure);

  /**
   * Object instance callback for batch end wait timer.
   */
  void WaitForBatchEndCallback();

  /**
   * Return in aFreeMusicSpace the amount of available music space that is free
   * for use in the library specified by aLibrary.  This value may be limited to
   * a value less than the total free space.
   *
   * \param aLibrary            Library for which to get space.
   * \param aFreeMusicSpace     Returned free music space.
   */
  nsresult GetMusicFreeSpace(sbILibrary* aLibrary,
                             PRInt64*    aFreeMusicSpace);

  /**
   * Return in aMusicAvailableSpace the amount of all available music space in
   * the library specified by aLibrary.  This value may be limited to a value
   * less than the total capacity of the library.
   */
  nsresult GetMusicAvailableSpace(sbILibrary* aLibrary,
                                  PRInt64*    aMusicAvailableSpace);

  /**
   * Internal version of SupportsMediaItem.
   *
   * @param aCallback [optional] A callback to use to report the result.  If the
   *                  callback is used, this method will raise
   *                  NS_ERROR_IN_PROGRESS.  Note that it may not be used even
   *                  if available; in that case, _retval will be used as in the
   *                  synchronous case.
   *
   * This is expected to be called from the main thread when asynchronous
   * behaviour is desired.
   *
   * @throw NS_ERROR_IN_PROGRESS if the result will be asynchronously supplied
   *                             via the callback.
   */
  nsresult SupportsMediaItem(sbIMediaItem*                  aMediaItem,
                             sbDeviceSupportsItemHelper*    aCallback,
                             PRBool                         aReportErrors,
                             PRBool*                        _retval);
  friend class sbDeviceSupportsItemHelper;

  /**
   * Return true in _retval if DRM is supported for the media item specified by
   * aMediaItem; return false otherwise.  If aReportErrors is true and DRM is
   * not supported, report a device error.
   *
   * \param aMediaItem    Media item to check.
   * \param aReportErrors If true, report device errors.
   * \param _retval       Returned true if DRM is supported.
   */
  virtual nsresult SupportsMediaItemDRM(sbIMediaItem* aMediaItem,
                                        PRBool        aReportErrors,
                                        PRBool*       _retval);

  //----------------------------------------------------------------------------
  //
  // Device volume services.
  //
  //----------------------------------------------------------------------------

  //
  //   mVolumeLock              Volume lock.
  //   mVolumeList              List of volumes.
  //   mVolumeGUIDTable         Table mapping volume GUIDs to volumes.
  //   mVolumeLibraryGUIDTable  Table mapping library GUIDs to volumes.
  //   mPrimaryVolume           Primary volume (usually internal storage).
  //   mDefaultVolume           Default volume when none specified.
  //

  PRLock*                                    mVolumeLock;
  nsTArray< nsRefPtr<sbBaseDeviceVolume> >   mVolumeList;
  nsInterfaceHashtableMT<nsStringHashKey,
                         sbBaseDeviceVolume> mVolumeGUIDTable;
  nsInterfaceHashtableMT<nsStringHashKey,
                         sbBaseDeviceVolume> mVolumeLibraryGUIDTable;
  nsRefPtr<sbBaseDeviceVolume>               mPrimaryVolume;
  nsRefPtr<sbBaseDeviceVolume>               mDefaultVolume;

  /**
   * Add the media volume specified by aVolume to the set of device media
   * volumes.
   *
   * \param aVolume               Volume to add.
   */
  nsresult AddVolume(sbBaseDeviceVolume* aVolume);

  /**
   * Remove the media volume specified by aVolume from the set of device media
   * volumes.
   *
   * \param aVolume               Volume to remove.
   */
  nsresult RemoveVolume(sbBaseDeviceVolume* aVolume);

  /**
   * Return in aVolume the media volume containing the media item specified by
   * aItem.  If the media item does not belong to a media volume, return
   * NS_ERROR_NOT_AVAILABLE.
   *
   * \param aItem                 Media item for which to get volume.
   * \param aVolume               Returned media volume.
   *
   * \returns NS_ERROR_NOT_AVAILABLE
   *                              Media item does not belong to a volume.
   */
  nsresult GetVolumeForItem(sbIMediaItem*        aItem,
                            sbBaseDeviceVolume** aVolume);

  //----------------------------------------------------------------------------
  //
  // Device settings services.
  //
  //----------------------------------------------------------------------------

  /**
   * Return in aDeviceSettingsDocument a DOM document object representing the
   * Songbird settings stored on the device.  Return null in
   * aDeviceSettingsDocument if no Songbird settings are stored on the device.
   *
   * \param aDeviceSettingsDocument Returned device settings document object.
   */
  virtual nsresult GetDeviceSettingsDocument
                     (class nsIDOMDocument** aDeviceSettingsDocument);

  /**
   * Return in aDeviceSettingsDocument the device settings stored in the file
   * specified by aDeviceSettingsFile.  Return null in aDeviceSettingsDocument
   * if the device settings file does not exist.
   *
   * \param aDeviceSettingsFile     File containing device settings.
   * \param aDeviceSettingsDocument Returned device settings document object.
   */
  nsresult GetDeviceSettingsDocument
             (nsIFile*               aDeviceSettingsFile,
              class nsIDOMDocument** aDeviceSettingsDocument);

  /**
   * Return in aDeviceSettingsDocument the device settings contained in
   * aDeviceSettingsContent.
   *
   * \param aDeviceSettingsContent  Device settings content.
   * \param aDeviceSettingsDocument Returned device settings document object.
   */
  nsresult GetDeviceSettingsDocument
             (nsTArray<PRUint8>&     aDeviceSettingsContent,
              class nsIDOMDocument** aDeviceSettingsDocument);
  /**
   * Apply the contents of the device settings document to the device settings.
   */
  virtual nsresult ApplyDeviceSettingsDocument();

  /**
   * Apply the contents of the device settings document specified by
   * aDeviceSettingsDocument to the device settings.
   *
   * \param aDeviceSettingsDocument   Device settings document.
   */
  virtual nsresult ApplyDeviceSettings
                     (class nsIDOMDocument* aDeviceSettingsDocument);

  /**
   * Apply the contents of the device settings document specified by
   * aDeviceSettingsDocument to the device property specified by
   * aPropertyName.  The property will be set to the same value as the device
   * setting element with the same tag name as the device property suffix.  The
   * suffix is the unique suffix of the device property.
   *
   * \param aDeviceSettingsDocument Device settings document.
   * \param aPropertyName           Name of device property to which to apply.
   */
  nsresult ApplyDeviceSettingsToProperty
             (class nsIDOMDocument*  aDeviceSettingsDocument,
              const nsAString&       aPropertyName);

  /**
   * Apply the device settings property value specified by aPropertyValue to the
   * device property specified by aPropertyName.
   *
   * \param aPropertyName           Name of device property to which to apply.
   * \param aPropertyValue          Device settings property value.
   */
  virtual nsresult ApplyDeviceSettingsToProperty
                     (const nsAString& aPropertyName,
                      nsIVariant*      aPropertyValue);

  /**
   * Apply the device info content of the device settings document specified by
   * aDeviceSettingsDocument.
   *
   * \param aDeviceSettingsDocument Device settings document.
   */
  nsresult ApplyDeviceSettingsDeviceInfo
             (class nsIDOMDocument* aDeviceSettingsDocument);

  /**
   * Apply the contents of the device settings document specified by
   * aDeviceSettingsDocument to the device capabilities.
   *
   * \param aDeviceSettingsDocument   Device settings document.
   */
  nsresult ApplyDeviceSettingsToCapabilities
            (class nsIDOMDocument* aDeviceSettingsDocument);


  //----------------------------------------------------------------------------
  //
  // Device properties services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the device properties.
   */
  virtual nsresult InitializeProperties();

  /**
   * Update the device properties.
   */
  virtual nsresult UpdateProperties();

  /**
   * Update the device property specified by aName.
   *
   * \param aName                 Name of property to update.
   */
  virtual nsresult UpdateProperty(const nsAString& aName);

  /**
   * Update the device statistics properties.
   */
  virtual nsresult UpdateStatisticsProperties();

  /**
   * Update the names of all device volumes.
   */
  nsresult UpdateVolumeNames();

  /**
   * Update the name of the device volume specified by aVolume.
   *
   * \param aVolume               Device volume for which to update name.
   */
  virtual nsresult UpdateVolumeName(sbBaseDeviceVolume* aVolume);


  //----------------------------------------------------------------------------
  //
  // Device preference services.
  //
  //----------------------------------------------------------------------------

  /* get the prefbranch root for this device */
  nsresult GetPrefBranchRoot(nsACString& aRoot);

  /* get a prefbranch for this device */
  nsresult GetPrefBranch(nsIPrefBranch** aPrefBranch);

  /* get a prefbranch for a device library */
  nsresult GetPrefBranch(sbIDeviceLibrary* aLibrary,
                         nsIPrefBranch**   aPrefBranch);

  /* Helper function to get a particular pref branch */
  nsresult GetPrefBranch(const char *aPrefBranchName,
                         nsIPrefBranch** aPrefBranch);

  /* Get a preference from a specific pref branch */
  nsresult GetPreferenceInternal(nsIPrefBranch *aPrefBranch,
                                 const nsAString & aPrefName,
                                 nsIVariant **_retval);

  /* Set a preference on a specific pref branch */
  nsresult SetPreferenceInternal(nsIPrefBranch *aPrefBranch,
                                 const nsAString & aPrefName,
                                 nsIVariant *aPrefValue);

  /* Set a preference, not sending an event if the preference changed */
  nsresult SetPreferenceInternalNoNotify(const nsAString & aPrefName,
                                         nsIVariant *aPrefValue,
                                         PRBool *aPrefChanged);


  /**
   * Apply the preference specified by aPrefName with the value specified by
   * aPrefValue.  Update device state and take any actions appropriate for the
   * preference value.
   *
   * \param aPrefName           Name of preference to apply.
   * \param aPrefValue          Value of preference.
   */
  virtual nsresult ApplyPreference(const nsAString& aPrefName,
                                   nsIVariant*      aPrefValue);

  /**
   * Return true if the preference specified by aPrefName is a library
   * preference.
   *
   * \param aPrefName           Name of preference.
   * \return                    True if preference is a library preference.
   */
  PRBool GetIsLibraryPreference(const nsAString& aPrefName);

  /**
   * Return in aLibrary, if not null, the library corresponding to the library
   * preference specified by aPrefName.  Return the library pref base in
   * aLibraryPrefBase.
   *
   * \param aPrefName           Name of library preference.
   * \param aLibrary            Optionally returned preference library.
   * \param aLibraryPrefBase    Library preference base.
   * \return NS_ERROR_NOT_AVAILABLE
   *                            Preference is not a library preference or
   *                            library is not available.
   */
  nsresult GetPreferenceLibrary(const nsAString&   aPrefName,
                                sbIDeviceLibrary** aLibrary,
                                nsAString&         aLibraryPrefBase);

  /**
   * Return in aPrefValue the value of the library preference specified by the
   * library preference name aLibraryPrefName for the library specified by
   * aLibrary.  The library preference name is relative ot the library
   * preference base.
   *
   * \param aLibrary            Library for which to get preference.
   * \param aLibraryPrefName    Library preference name to get.
   * \param aPrefValue          Returned preference value.
   */
  nsresult GetLibraryPreference(sbIDeviceLibrary* aLibrary,
                                const nsAString&  aLibraryPrefName,
                                nsIVariant**      aPrefValue);

  /**
   * Return in aPrefValue the value of the library preference specified by the
   * library preference base aLibraryPrefBase and library preference name
   * aLibraryPrefName.
   *
   * \param aLibraryPrefBase    Library preference base.
   * \param aLibraryPrefName    Library preference name.
   * \param aPrefValue          Returned preference value.
   */
  nsresult GetLibraryPreference(const nsAString& aLibraryPrefBase,
                                const nsAString& aLibraryPrefName,
                                nsIVariant**     aPrefValue);

  /**
   * Apply the preference specified by the library preference name
   * aLibraryPrefName with the value specified by aPrefValue for the library
   * specified by aLibrary.  If aPrefValue is null, read the current preference
   * value and apply it.  If aLibraryPrefName is empty, read and apply all
   * library preferences.
   *
   * \param aLibrary            Library for which to apply preference.
   * \param aLibraryPrefName    Library preference name.
   * \param aPrefValue          Library preference value.
   */
  virtual nsresult ApplyLibraryPreference(sbIDeviceLibrary* aLibrary,
                                          const nsAString&  aLibraryPrefName,
                                          nsIVariant*       aPrefValue);

  /**
   * Apply the preference for library organization specified by the library
   * preference name aLibraryPrefName with the value specified by aPrefValue
   * for the library specified by aLibrary.  If aPrefValue is null, read the
   * current preference value and apply it.  If aLibraryPrefName is empty,
   * read and apply all library organization preferences.
   *
   * \see ApplyLibraryPreference
   * \param aLibrary            Library for which to apply preference.
   * \param aLibraryPrefName    Library preference name.
   * \param aLibraryPrefBase    [optional] Library preference base.
   * \param aPrefValue          Library preference value.
   */
  virtual nsresult ApplyLibraryOrganizePreference(sbIDeviceLibrary* aLibrary,
                                                  const nsAString&  aLibraryPrefName,
                                                  const nsAString&  aLibraryPrefBase,
                                                  nsIVariant*       aPrefValue);

  /**
   * Return in aLibraryPrefName the library preference name for the preference
   * with the device preference name aPrefName.
   *
   * \param aPrefName           Device preference name.
   * \param aLibraryPrefName    Returned library preference name.
   */
  nsresult GetLibraryPreferenceName(const nsAString& aPrefName,
                                    nsAString&       aLibraryPrefName);

  /**
   * Return in aLibraryPrefName the library preference name for the preference
   * with the device preference name aPrefName.  The library preference base is
   * specified by aLibraryPrefBase.
   *
   * \param aPrefName           Device preference name.
   * \param aLibraryPrefBase    Library preference base.
   * \param aLibraryPrefName    Returned library preference name.
   */
  nsresult GetLibraryPreferenceName(const nsAString&  aPrefName,
                                    const nsAString&  aLibraryPrefBase,
                                    nsAString&        aLibraryPrefName);

  /**
   * Return in aPrefBase the library preference base for the library specified
   * by aLibrary.
   *
   * \param aLibrary            Library for which to get preference base.
   * \param aPrefBase           Returned library preference base.
   */
  nsresult GetLibraryPreferenceBase(sbIDeviceLibrary* aLibrary,
                                    nsAString&        aPrefBase);

  /**
   * Return in aCapabilities the device capabilities preference.  Return a void
   * variant if no device capabilities preference has been set.
   *
   * \param aCapabilities       Returned device capabilities preference.
   */
  nsresult GetCapabilitiesPreference(nsIVariant** aCapabilities);

  /**
   * Return in aLocalDeviceDir the local directory in which to store device
   * related files (e.g., device settings file backups).
   *
   * \param aLocalDeviceDir       Retured local device file directory.
   */
  nsresult GetLocalDeviceDir(nsIFile** aLocalDeviceDir);


  //----------------------------------------------------------------------------
  //
  // Device sync services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle the sync request specified by aRequest.
   *
   * \param aRequest              Request data record.
   */
  nsresult HandleSyncRequest(TransferRequest* aRequest);

  /**
   * Ensure enough space is available for the sync request specified by
   * aRequest.  If not, ask the user if the sync should be changed to a random
   * subset of the items to sync that fits in the available space.  If the user
   * chooses to do so, create the list and configure to sync to it.  If the user
   * chooses to abort, return true in aAbort.
   *
   * \param aRequest            Sync request.
   * \param aAbort              Abort sync if true.
   */
  nsresult EnsureSpaceForSync(TransferRequest* aRequest,
                              PRBool*          aAbort);

  /**
   * Create a random subset of sync items from the list specified by
   * aSyncItemList with sizes specified by aSyncItemSizeMap that fits in the
   * available space specified by aAvailableSpace.  Create a sync media list
   * from this subset within the source library specified by aSrcLib and
   * configure the destination library specified by aDstLib to sync to it.
   *
   * \param aSrcLib             Source library from which to sync.
   * \param aDstLib             Destination library to which to sync.
   * \param aSyncItemList       Full list of sync items.
   * \param aSyncItemSizeMap    Size of sync items.
   * \param aAvailableSpace     Space available for sync.
   * \return NS_OK if successful.
   *         NS_ERROR_ABORT if user aborts syncing.
   *         else some other NS_ERROR value.
   */
  nsresult SyncCreateAndSyncToList
             (sbILibrary*                                   aSrcLib,
              sbIDeviceLibrary*                             aDstLib,
              nsCOMArray<sbIMediaItem>&                     aSyncItemList,
              nsDataHashtable<nsISupportsHashKey, PRInt64>& aSyncItemSizeMap,
              PRInt64                                       aAvailableSpace);

  /**
   * Create a source sync media list in the library specified by aSrcLib from
   * the list of sync items specified by aSyncItemList and return the sync media
   * list in aSyncMediaList.
   *
   * \param aSrcLib             Source library of sync.
   * \param aDstLib             Destination library to which to sync.
   * \param aAvailableSpace     Space available for sync.
   * \param aSyncMediaList      Created sync media list.
   * \return NS_OK if successful.
   *         NS_ERROR_ABORT if user aborts syncing.
   *         else some other NS_ERROR value.
   */
  nsresult SyncCreateSyncMediaList(sbILibrary*       aSrcLib,
                                   sbIDeviceLibrary* aDstLib,
                                   PRInt64           aAvailableSpace,
                                   sbIMediaList**    aSyncMediaList);

  /**
   * Configure the destination library specified by aDstLib to sync to the media
   * list specified by aMediaList.
   *
   * \param aDstLib             Destination sync library.
   * \param aMediaList          Media list to sync.
   */
  nsresult SyncToMediaList(sbIDeviceLibrary* aDstLib,
                           sbIMediaList*     aMediaList);

  /**
   * Return in aSyncItemList and aSyncItemSizeMap the set of items to sync and
   * their sizes, as configured for the destination sync library specified by
   * aDstLib.  The sync source library is specified by aSrcLib.  The total size
   * of all sync items is specified by aTotalSyncSize.
   *
   * \param aSrcLib             Sync source library.
   * \param aDstLib             Sync destination library.
   * \param aSyncItemList       List of items from which to sync.
   * \param aSyncItemSizeMap    Size of sync items.
   * \param aTotalSyncSize      Total size of all sync items.
   */
  nsresult SyncGetSyncItemSizes
             (sbILibrary*                                   aSrcLib,
              sbIDeviceLibrary*                             aDstLib,
              nsCOMArray<sbIMediaItem>&                     aSyncItemList,
              nsDataHashtable<nsISupportsHashKey, PRInt64>& aSyncItemSizeMap,
              PRInt64*                                      aTotalSyncSize);

  /**
   * Add to aSyncItemList and aSyncItemSizeMap the set of items and their sizes
   * from the sync media list specified by aSyncML.  Add the total size of the
   * items to aTotalSyncSize.
   *
   * \param aSyncML             Sync media list.
   * \param aSyncItemList       List of items from which to sync.
   * \param aSyncItemSizeMap    Size of sync items.
   * \param aTotalSyncSize      Total size of all sync items.
   */
  nsresult SyncGetSyncItemSizes
             (sbIMediaList*                                 aSyncML,
              nsCOMArray<sbIMediaItem>&                     aSyncItemList,
              nsDataHashtable<nsISupportsHashKey, PRInt64>& aSyncItemSizeMap,
              PRInt64*                                      aTotalSyncSize);

  /**
   * Add to aSyncItemList and aSyncItemSizeMap the item and the size specified
   * by aSyncMI. Add the size of the item to aTotalSyncSize.
   *
   * \param aSyncMI             Sync media item.
   * \param aSyncItemList       List of items from which to sync.
   * \param aSyncItemSizeMap    Size of sync items.
   * \param aTotalSyncSize      Total size of all sync items.
   */
   nsresult SyncGetSyncItemSizes
             (sbIMediaItem*                                 aSyncMI,
              nsCOMArray<sbIMediaItem>&                     aSyncItemList,
              nsDataHashtable<nsISupportsHashKey, PRInt64>& aSyncItemSizeMap,
              PRInt64*                                      aTotalSyncSize);

  /**
   * Return in aAvailableSpace the space available for syncing to the library
   * specified by aLibrary.  The space available includes all free space
   * available plus the space currently taken for items being synced.
   *
   * \param aLibrary            Sync destination library.
   * \param aAvailableSpace     Space available for sync.
   */
  nsresult SyncGetSyncAvailableSpace(sbILibrary* aLibrary,
                                     PRInt64*    aAvailableSpace);

  /**
   * Produce the sync change set for the sync request specified by aRequest and
   * return the change set in aChangeset.
   *
   * \param aRequest            Sync request data record.
   * \param aChangeset          Sync request change set.
   */
  nsresult SyncProduceChangeset(TransferRequest*      aRequest,
                                sbILibraryChangeset** aChangeset);

  /**
   * Apply the sync change set specified by aChangeset to the device library
   * specified by aDstLibrary.
   *
   * \param aDstLibrary         Device library to which to apply sync change
   *                            set.
   * \param aChangeset          Set of sync changes.
   */
  nsresult SyncApplyChanges(sbIDeviceLibrary*    aDstLibrary,
                            sbILibraryChangeset* aChangeset);

  /**
   * Add the media list specified by aMediaList to the device library specified
   * by aDstLibrary.
   *
   * \param aDstLibrary         Device library to which to add media list.
   * \param aMediaList          Media list to add.
   */
  nsresult SyncAddMediaList(sbIDeviceLibrary* aDstLibrary,
                            sbIMediaList*     aMediaList);

  /**
   * Synchronize all the media lists in the list of media list changes specified
   * by aMediaListChangeList.
   *
   * \param aMediaListChangeList  List of media list changes.
   */
  nsresult SyncMediaLists(nsCOMArray<sbILibraryChange>& aMediaListChangeList);

  /**
   * Update the properties of a media item as specified by the sync change
   * specified by aChange.
   *
   * \param aChange               Changes to make to media item.
   */
  nsresult SyncUpdateProperties(sbILibraryChange* aChange);

  /**
   * Merges the property value based on what property we're dealing with
   */
  nsresult SyncMergeProperty(sbIMediaItem * aItem,
                             nsAString const & aPropertyId,
                             nsAString const & aNewValue,
                             nsAString const & aOldValue);

  /**
   * Returns the latest of the date/time. The dates are in milliseconds since
   * the JS Data's epoch date.
   */
  nsresult SyncMergeSetToLatest(nsAString const & aNewValue,
                                nsAString const & aOldValue,
                                nsAString & aMergedValue);

  /**
   * Set up all media lists within the media list specified by aMediaList to
   * trigger a difference when syncing.  The media list aMediaList is not set up
   * to force a difference.
   *
   * This function triggers a difference by setting a property that should not
   * be set on any source media lists.
   *
   * \param aMediaList          Media list containing media lists to force
   *                            differences.
   */
  nsresult SyncForceDiffMediaLists(sbIMediaList* aMediaList);

  /**
   * Determine whether the media list specified by aMediaList should be synced
   * to the device.  Return true in aShouldSync if media list should be synced.
   * The download media list and hidden media lists are examples of media lists
   * that should not be synced.
   *
   * \param aMediaList          Media list to check.
   * \param aShouldSync         Returned true if media list should be synced.
   */
  nsresult ShouldSyncMediaList(sbIMediaList* aMediaList,
                               PRBool*       aShouldSync);


  /**
   * Show a dialog box to ask the user if they would like the device ejected
   * even though Songbird is currently playing from the device.
   *
   * \param aEject [out, retval]  Should the device be ejected?
   */
  nsresult PromptForEjectDuringPlayback(PRBool* aEject);

  /**
   * Update locations of folders containing media.
   */
  virtual nsresult UpdateMediaFolders();

  /**
   *   Determine the URI for the device write destination media item specified
   * by aWriteDstItem.  The base URI is specified by aContentSrcBaseURI.  The
   * URI of the source of the write operation may be specified by aWriteSrcURI;
   * if none is specified, the source information is obtained from the write
   * destination item.
   *
   *   Currently, only base URI schemes of "file:" are supported.
   *
   * @note The URI is _not_ suitable for inserting into contentSrc directly;
   * it needs to go through sbILibraryUtils::getContentURI
   */
  nsresult GetDeviceWriteDestURI(sbIMediaItem* aWriteDstItem,
                                 nsIURI*       aContentSrcBaseURI,
                                 nsIURI*       aWriteSrcURI,
                                 nsIURI **     aDestinationURI);

  /**
   * Present the user with a dialog for the initial device setup.
   */
  nsresult SetupDevice();

  /**
   * Callback functions for deferring the presentation of the initial device
   * setup dialog.
   */
  static void DeferredSetupDevice(nsITimer* aTimer,
                                  void*     aClosure);

  nsresult DeferredSetupDevice();

  /**
   * Calls the device info registrars to register the device info.
   */
  nsresult RegisterDeviceInfo();
  /**
   * Calls any registered device capabilities augmenter.
   * \param aCapabilities Capabilities object to augment.
   */
  nsresult RegisterDeviceCapabilities(sbIDeviceCapabilities * aCapabilities);

  /**
   * Process the info registrars to find out which ones are interested in us.
   */
  nsresult ProcessInfoRegistrars();

  /**
    * Returns a list of transcode profiles that the device supports
    * \param aType the type of transcode profiles to retrieve.
    * \param aSupportedProfiles the list of profiles that were found
    * \return NS_OK if successful else some NS_ERROR value
    */
   virtual nsresult GetSupportedTranscodeProfiles(PRUint32 aType,
                                                  nsIArray **aSupportedProfiles);
   /**
   * Dispatch a transcode error event for the media item specified by aMediaItem
   * with the error message specified by aErrorMessage.
   *
   * \param aMediaItem The media item for which to dispatch an error event.
   * \param aErrorMessage Error event message.
   */
  nsresult DispatchTranscodeErrorEvent(sbIMediaItem*    aMediaItem,
                                       const nsAString& aErrorMessage);

  /* Return an array of sbIImageFormatType describing all the supported
   * album art formats for the device.
   *
   * The array may be empty; this should be interpreted as "unknown" rather than
   * "album art is not supported"
   */
  nsresult GetSupportedAlbumArtFormats(nsIArray * *aFormats);

  /**
   * Gets the first request. This does not lock request queue
   * Iterator
   */
  nsresult FindFirstRequest(
    TransferRequestQueueMap::iterator & aMapIter,
    TransferRequestQueue::iterator & aQueueIter,
    bool aRemove);

  /**
   * Returns if the device pref to enable music space limiting is turned on.
   * \param aPrefBase The device preference root.
   * \param aOutShouldLimitSpace The outparam for setting if the device is
   *                             currently set for music space limiting.
   * WARNING: This method expects to be under the |mPreferenceLock| when called.
   */
  nsresult GetShouldLimitMusicSpace(const nsAString & aPrefBase,
                                    PRBool *aOutShouldLimitSpace);

  /**
   * Return the current limited music space as a percentage that the device's
   * pref is currently set to.
   * \param aPrefBase The device preference root.
   * \param aOutLimitPercentage The outparam for setting the music limit
   *                            percentage value.
   * WARNING: This method expects to be under the |mPreferenceLock| when called.
   */
  nsresult GetMusicLimitSpacePercent(const nsAString & aPrefBase,
                                     PRUint32 *aOutLimitPercentage);

  /**
   * Hashtable enumerator function that removes the items from the list
   */
  static PLDHashOperator RemoveLibraryEnumerator(
                                             nsISupports * aList,
                                             nsCOMPtr<nsIMutableArray> & aItems,
                                             void * aUserArg);

  /**
   * Auto class to unignore when leaving scope
   */
  class AutoListenerIgnore
  {
  public:
    AutoListenerIgnore(sbBaseDevice * aDevice);
    ~AutoListenerIgnore();
  private:
    sbBaseDevice * mDevice;
  };

  /**
   * This iterates over the transfer requests and removes the Songbird library
   * items that were created for the requests. REQUEST_WRITE entries come from
   * items being added to the device the library. Those items are hidden when
   * created, so they need to be removed if they weren't copied successfully.
   * REQUEST_READ items are create when items are copy from the device to
   * another library. Those items are created hidden and need to be deleted
   * if they aren't successfully copied.
   * \param iter Starting point of the requests
   * \param end Typical iteration end point (1 past the last item)
   */
  template <class T>
  nsresult RemoveLibraryItems(T iter, T end)
  {
    nsresult rv;
    nsInterfaceHashtable<nsISupportsHashKey, nsIMutableArray> groupedItems;
    groupedItems.Init();

    while (iter != end) {
      nsRefPtr<sbBaseDevice::TransferRequest> request = *iter;
      // If this is a request that adds an item to the device we need to remove
      // it from the device since it never was copied
      switch (request->type) {
        case sbBaseDevice::TransferRequest::REQUEST_WRITE:
        case sbBaseDevice::TransferRequest::REQUEST_READ: {
          if (request->list && request->item) {
            nsCOMPtr<nsIMutableArray> items;
            groupedItems.Get(request->list, getter_AddRefs(items));
            if (!items) {
              items = do_CreateInstance(
                               "@songbirdnest.com/moz/xpcom/threadsafe-array;1",
                               &rv);
              NS_ENSURE_TRUE(groupedItems.Put(request->list, items),
                             NS_ERROR_OUT_OF_MEMORY);
            }
            rv = items->AppendElement(request->item, PR_FALSE);
            NS_ENSURE_SUCCESS(rv, rv);
          }
        }
        break;
      }
      ++iter;
    }
    AutoListenerIgnore ignore(this);
    groupedItems.Enumerate(RemoveLibraryEnumerator, this);
    return NS_OK;
  }

  /**
   * Base implementation for the device name.
   * \param aName the returned name
   */
  nsresult GetNameBase(nsAString& aName);
  /**
   * Base implementation for the product name
   * \param aDefaultModelNumberString The localized string name for the default
   *                                  model number
   * \param aProductString the localized sttring name for the product name
   * \param aProductName The returned product name
   */
  nsresult GetProductNameBase(char const * aDefaultModelNumberString,
                              nsAString& aProductName);
  /**
   * For use in the request handler thread event mechanism. This is called
   * to process pending events. The default implementation does nothing, as
   * not all implementations use this, and much of the implementation is
   * device specific
   */
  virtual nsresult ReqHandleRequestAdded()
  {
    return NS_OK;
  }
  /**
   * Creates an ignore auto object that will cause the watch folder to ignore
   * the path while it's being created. If this is not a file URI this will
   * return nothing and not fail.
   */
  nsresult
  IgnoreWatchFolderPath(nsIURI * aURI,
                        sbAutoIgnoreWatchFolderPath ** aIgnorePath);
  /**
   * Log the list of device folders.
   */
  void LogDeviceFolders();
  /**
   * Enumerator function for logging the list of device folders.
   */
  static PLDHashOperator LogDeviceFoldersEnum(const unsigned int& aKey,
                                              nsString*           aData,
                                              void*               aUserArg);
  /**
   * Determines if a request already exists that is similar to one being
   * submitted
   * \param aQueue is the queue to check for a duplicate
   * \param aRequest is the request we're checking duplicates of
   */
  nsresult IsRequestDuplicate(TransferRequestQueue & aQueue,
                              TransferRequest * aRequest,
                              bool & aIsDuplicate);
  nsresult  GetExcludedFolders(nsTArray<nsString> & aExcludedFolders);
};

void SBUpdateBatchCounts(sbBaseDevice::Batch& aBatch);

void SBUpdateBatchCountsIgnorePlaylist(sbBaseDevice::Batch& aBatch);

void SBUpdateBatchIndex(sbBaseDevice::Batch& aBatch);

void SBCreateSubBatchIndex(sbBaseDevice::Batch& aBatch);

bool SBWriteRequestBatchSortComparator
       (nsRefPtr<sbBaseDevice::TransferRequest> const &p1,
        nsRefPtr<sbBaseDevice::TransferRequest> const &p2);

/**
 * This class provides an nsIRunnable interface that may be used to dispatch
 * and process device request added events.
 */

class sbDeviceReqAddedEvent : public nsIRunnable
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // Inherited interfaces.
  //

  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE


  //
  // Public methods.
  //

  static nsresult New(sbBaseDevice * aDevice,
                      nsIRunnable** aEvent);


  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

private:

  /**
   * Owning reference (using nsRefPtr caused ambiguities)
   */
  sbBaseDevice* mDevice;


  // Make default constructor private to force use of factory methods.
  sbDeviceReqAddedEvent() : mDevice(nsnull) {};
  ~sbDeviceReqAddedEvent()
  {
    nsISupports * device = NS_ISUPPORTS_CAST(sbIDevice *,mDevice);
    mDevice = nsnull;
    NS_IF_RELEASE(device);
  }
};


//
// Auto-disposal class wrappers.
//
//   sbAutoResetEnsureSpaceChecked
//                              Wrapper to automatically reset the space
//                              checked flag.
//   sbAutoDeviceCompleteCurrentRequest
//                              Wrapper to automatically complete the current
//                              request.
//

SB_AUTO_NULL_CLASS(sbAutoResetEnsureSpaceChecked,
                   sbBaseDevice*,
                   mValue->SetEnsureSpaceChecked(false));
SB_AUTO_NULL_CLASS(sbAutoDeviceCompleteCurrentRequest,
                   sbBaseDevice*,
                   mValue->CompleteCurrentRequest());

/**
 * Auto class to reset isRequestHandling flag
 */
class sbDeviceAutoRequestHandling
{
public:
  sbDeviceAutoRequestHandling(PRInt32 & aIsHandlingRequests) :
    mIsHandlingRequests(aIsHandlingRequests) {
    PR_AtomicSet(&mIsHandlingRequests, 1);
  }
  ~sbDeviceAutoRequestHandling() {
    NS_ASSERTION(mIsHandlingRequests,
                 "mIsHandlingRequest reset outside "
                 "of sbMSCAutoRequestHandling");
    PR_AtomicSet(&mIsHandlingRequests, 0);
  }
private:
  PRInt32 & mIsHandlingRequests;
};

#endif /* __SBBASEDEVICE__H__ */
