/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#ifndef __SBBASEDEVICE__H__
#define __SBBASEDEVICE__H__

#include <list>
#include <map>

#include "sbIDevice.h"
#include "sbBaseDeviceEventTarget.h"
#include "sbDeviceLibrary.h"

#include <nsAutoPtr.h>
#include <nsClassHashtable.h>
#include <nsCOMPtr.h>
#include <nsCOMArray.h>
#include <nsDataHashtable.h>
#include <nsISupportsImpl.h>
#include <nsITimer.h>
#include <prlock.h>

#include <sbILibraryChangeset.h>
#include "sbIMediaItem.h"
#include "sbIMediaList.h"
#include "sbDeviceStatistics.h"

class nsIPrefBranch;

class sbBaseDeviceLibraryListener;
class sbDeviceBaseLibraryCopyListener;
class sbBaseDeviceMediaListListener;
class sbIDeviceCapabilitiesRegistrar;
class sbIDeviceLibrary;
class sbITranscodeProfile;

/* Property used to force a sync diff. */
#define DEVICE_PROPERTY_SYNC_FORCE_DIFF \
          "http://songbirdnest.com/device/1.0#forceDiff"

#define SB_ERROR_REQUEST_ABORTED NS_ERROR_GENERATE_FAILURE( NS_ERROR_MODULE_GENERAL, 2 )

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
      /* delete all files */
      REQUEST_WIPE          = sbIDevice::REQUEST_WIPE,
      /* move an item in one playlist */
      REQUEST_MOVE          = sbIDevice::REQUEST_MOVE,
      REQUEST_UPDATE        = sbIDevice::REQUEST_UPDATE,
      REQUEST_NEW_PLAYLIST  = sbIDevice::REQUEST_NEW_PLAYLIST
   };

    int type;                        /* one of the REQUEST_* constants,
                                          or a custom type */
    nsCOMPtr<sbIMediaItem> item;     /* the item this request pertains to */
    nsCOMPtr<sbIMediaList> list;     /* the list this request is to act on */
    nsCOMPtr<nsISupports>  data;     /* optional data that may or may not be used by a type of request */
    PRUint32 index;                  /* the index in the list for this action */
    PRUint32 otherIndex;             /* any secondary index needed */
    PRUint32 batchCount;             /* the number of items in this batch
                                          (batch = run of requests of the same
                                          type) */
    PRUint32 batchIndex;             /* index of item in the batch to process */

    PRUint32 itemTransferID;         /* id for this item transfer */

    static const PRInt32 PRIORITY_DEFAULT = 0x1000000;
    PRInt32 priority;                /* priority for the request (lower first) */

    PRBool spaceEnsured;             /* true if enough free space is ensured for
                                          request */
    PRInt64 timeStamp;               /* time stamp of when request was
                                          enqueued */
    PRUint32 batchID;                /* ID of request batch */

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
    TransferRequest() : spaceEnsured(PR_FALSE) {}
    ~TransferRequest(){} /* we're refcounted, no manual deleting! */
  };
  
public:
  /* selected methods from sbIDevice */
  NS_IMETHOD GetPreference(const nsAString & aPrefName, nsIVariant **_retval);
  NS_IMETHOD SetPreference(const nsAString & aPrefName, nsIVariant *aPrefValue);
  NS_IMETHOD GetIsBusy(PRBool *aIsBusy);
  NS_IMETHOD GetCanDisconnect(PRBool *aCanDisconnect);
  NS_IMETHOD GetState(PRUint32 *aState);
  NS_IMETHOD SyncLibraries(void);
  NS_IMETHOD Format(void);
  NS_IMETHOD GetSupportsReformat(PRBool *_retval);

public:
  sbBaseDevice();
  ~sbBaseDevice();

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

  /* clear the request queue */
  nsresult ClearRequests(const nsAString &aDeviceID);

  /**
   * Internally set preference specified by aPrefName to the value specified by
   * aPrefValue.  If the new preference value is different than that old, return
   * true in aHasChanged unless aHasChanged is null.
   * This method may be used for loading preferences from device storage.  It
   * will not send preference changed events.
   *
   * @param aPrefName Name of the preference to set.
   * @param aPrefValue Value to set preference.
   * @param aHasChanged True if preference value changed.
   */

  nsresult SetPreferenceInternal(const nsAString& aPrefName,
                                 nsIVariant*      aPrefValue,
                                 PRBool*          aHasChanged);

  /* A request has been added, process the request
     (or schedule it to be processed) */
  virtual nsresult ProcessRequest() = 0;

  /**
   * Set the device state
   * @param aState new device state
   */
  nsresult SetState(PRUint32 aState);

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
  nsresult CheckAccess(sbIDeviceLibrary* aDevLib);

  /**
   * Remove the library specified by aDevLib from the device content.
   * @param aDevLib the device library to remove.
   */
  nsresult RemoveLibrary(sbIDeviceLibrary* aDevLib);

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
   * Return our statistics collector
   */
  sbDeviceStatistics & DeviceStatistics()
  {
    return mDeviceStatistics;
  }

  nsresult CreateTransferRequest(PRUint32 aRequest,
                                 nsIPropertyBag2 *aRequestParameters,
                                 TransferRequest **aTransferRequest);

  /**
   * Create an event for the device and dispatch it
   * @param aType type of event
   * @param aData event data
   */
  nsresult CreateAndDispatchEvent(PRUint32 aType,
                                  nsIVariant *aData,
                                  PRBool aAsync = PR_TRUE);

  /**
   * Returns true if the current request should be aborted. This subsequently
   * resets the flags
   */
  PRBool IsRequestAborted()
  {
    NS_ASSERTION(mRequestMonitor, "mRequestMonitor must be initialized");
    nsAutoMonitor mon(mRequestMonitor);
    PRBool const abort = mAbortCurrentRequest;
    mAbortCurrentRequest = PR_FALSE;
    return abort;
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

protected:
  friend class sbBaseDeviceInitHelper;
  /**
   * Base class initialization this will call the InitDevice first then
   * do the intialization needed by the sbDeviceBase
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
  nsresult GetFirstRequest(PRBool aRemove, TransferRequest** _retval);

protected:
  /* to block the background thread from transferring files while the batch
     has not completed; if a delay is active, either the request monitor OR
     the request removal lock needs to be held.  No requests may be removed
     while the removal lock is behind held.  No actions (including pushing
     additional requests) may be performed while the request monitor is held. */
  PRMonitor * mRequestMonitor;
  nsRefPtr<TransferRequest> mRequestBatchStart;
  nsCOMPtr<nsITimer> mRequestBatchTimer;
  nsCOMPtr<nsITimer> mBatchEndTimer;
  PRInt32 mNextBatchID;
  /**
   * Protects the mRequestBatchTimer checking and setting
   */
  PRLock * mRequestBatchTimerLock;

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
  sbDeviceStatistics mDeviceStatistics;
  PRBool mAbortCurrentRequest;
  PRInt32 mIgnoreMediaListCount; // Allows us to know if we're ignoring lists
  PRUint32 mPerTrackOverhead; // estimated bytes of overhead per track

  nsRefPtr<sbBaseDeviceLibraryListener> mLibraryListener;
  nsRefPtr<sbDeviceBaseLibraryCopyListener> mLibraryCopyListener;
  nsDataHashtableMT<nsISupportsHashKey, nsRefPtr<sbBaseDeviceMediaListListener> > mMediaListListeners;
  nsCOMPtr<sbIDeviceCapabilitiesRegistrar> mCapabilitiesRegistrar;
  PRUint32 mCapabilitiesRegistrarType;
  PRLock*  mPreferenceLock;
  PRUint32 mMusicLimitPercent;

  // cache data for media management preferences
  typedef struct OrganizeData_t {
    PRBool    organizeEnabled;
    nsCString dirFormat;
    nsCString fileFormat;
    OrganizeData_t() : organizeEnabled(PR_FALSE) {}
  } OrganizeData;
  nsClassHashtableMT<nsIDHashKey, OrganizeData> mOrganizeLibraryPrefs;

protected:

  /**
   * Default per track storage overhead.  10000 is enough for one 8K block plus
   * extra media database or filesystem overhead.
   */
  static const PRUint32 DEFAULT_PER_TRACK_OVERHEAD = 10000;

  /**
   * Percent free space to reserve as margin when building a sync playlist.
   */
  static const PRUint32 SYNC_PLAYLIST_MARGIN_PCT = 1;

  /**
   * Make sure that there is enough free space to complete the write request
   * specified by aRequest, taking batches into consideration.  If only a
   * partial batch has been enqueued, return true in aWait, indicating that the
   * caller should wait to process requests.  ProcessRequest will be called when
   * the batch is complete.
   * If not enough free space is available, ask the user what to do.
   * Remove requests from the queue to make room.  Set the request spaceEnsured
   * flag for requests that are ensured space on the device.
   *
   * \param aRequest the request to check size for
   * \param aWait if true, caller should wait to process further requests
   */
  nsresult EnsureSpaceForWrite(TransferRequest* aRequest,
                               PRBool*          aWait);

 /**
   * Go through the given queue to make sure that there is enough free space
   * to complete the write requests.  If not, and the user agrees, attempt to
   * transfer a subset.
   *
   * \param aQueue the queue to check size for
   */
  virtual nsresult EnsureSpaceForWrite(TransferRequestQueue& aQueue);

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

  //----------------------------------------------------------------------------
  //
  // Device preference services.
  //
  //----------------------------------------------------------------------------

  /* get a prefbranch for this device */
  nsresult GetPrefBranch(nsIPrefBranch** aPrefBranch);

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

  //----------------------------------------------------------------------------
  //
  // Device sync services.
  //
  //----------------------------------------------------------------------------

  /**
   * Check if device is linked to the local sync partner.  If it is, return true
   * in aIsLinkedLocally; otherwise, return false.  If aRequestPartnerChange is
   * true and the device is not linked locally, make a request to the user to
   * change the device sync partner to the local sync partner.
   *
   * \param aRequestPartnerChange   Request that the sync partner be changed to
   *                                the local sync partner.
   * \param aIsLinkedLocally        Returned true if the device is linked to the
   *                                local sync partner.
   */
  nsresult SyncCheckLinkedPartner(PRBool  aRequestPartnerChange,
                                  PRBool* aIsLinkedLocally);

  /**
   * Make a request to the user to change the device sync partner to the local
   * sync partner.  If the user grants the request, return true in
   * aPartnerChangeGranted; otherwise, return true.
   *
   * \param aPartnerChangeGranted   Returned true if the user granted the
   *                                request.
   */
  nsresult SyncRequestPartnerChange(PRBool* aPartnerChangeGranted);

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
   */
  nsresult SyncCreateAndSyncToList
             (sbILibrary*                                   aSrcLib,
              sbIDeviceLibrary*                             aDstLib,
              nsCOMArray<sbIMediaItem>&                     aSyncItemList,
              nsDataHashtable<nsISupportsHashKey, PRInt64>& aSyncItemSizeMap,
              PRInt64                                       aAvailableSpace);

  /**
   * Shuffle the sync item list specified by aSyncItemList with sizes specified
   * by aSyncItemSizeMap and create a subset of the list that fits in the
   * available space specified by aAvailableSpace.  Return the subset in
   * aShuffleSyncItemList.
   */
  nsresult SyncShuffleSyncItemList
    (nsCOMArray<sbIMediaItem>&                     aSyncItemList,
     nsDataHashtable<nsISupportsHashKey, PRInt64>& aSyncItemSizeMap,
     PRInt64                                       aAvailableSpace,
     nsIArray**                                    aShuffleSyncItemList);

  /**
   * Create a source sync media list in the library specified by aSrcLib from
   * the list of sync items specified by aSyncItemList and return the sync media
   * list in aSyncMediaList.
   *
   * \param aSrcLib             Source library of sync.
   * \param aSyncItemList       List of items to sync.
   * \param aSyncMediaList      Created sync media list.
   */
  nsresult SyncCreateSyncMediaList(sbILibrary*    aSrcLib,
                                   nsIArray*      aSyncItemList,
                                   sbIMediaList** aSyncMediaList);

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
   * Return in aSyncList the list of all sync media lists, as configured for the
   * destination sync library specified by aDstLib.  The sync source library is
   * specified by aSrcLib.
   *
   * \param aSrcLib             Sync source library.
   * \param aDstLib             Sync destination library.
   * \param aSyncList           List of all sync media lists.  nsIArray of
   *                            sbIMediaList.
   */
  nsresult SyncGetSyncList(sbILibrary*       aSrcLib,
                           sbIDeviceLibrary* aDstLib,
                           nsIArray**        aSyncList);

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
   * Returns the primary library for the device
   * 
   * \param aDeviceLibrary out parameter receiving the primary library object
   */
  nsresult GetPrimaryLibrary(sbIDeviceLibrary ** aDeviceLibrary);

  /**
   *   Set the content source for the device write destination media item
   * specified by aWriteDstItem.  The base URI of the content source is
   * specified by aContentSrcBaseURI.  The URI of the source of the write
   * operation may be specified by aWriteSrcURI; if none is specified, the
   * source information is obtained from the write destination item.
   *
   *   Currently, only base URI schemes of "file:" are supported.
   *
   * \param aWriteDstItem       Device write destination media item.
   * \param aContentSrcBaseURI  Base URI of device content source.
   * \param aWriteSrcURI        URI of source of write.  Defaults to null.
   */
  nsresult SetDeviceWriteContentSrc(sbIMediaItem* aWriteDstItem,
                                    nsIURI*       aContentSrcBaseURI,
                                    nsIURI*       aWriteSrcURI = nsnull);

  /**
   * Present the user with a dialog for the initial device setup.
   */

  nsresult SetupDevice();
  /**
   * Calls any registered device capabilities augmenter.
   * \param aCapabilities Capabilities object to augment.
   */
  nsresult RegisterDeviceCapabilities(sbIDeviceCapabilities * aCapabilities);
  
  /**
   * Process the capabilities registrars to find out which ones are interested
   * in us.
   */
  nsresult ProcessCapabilitiesRegistrars();
  
  /**
   * Returns the profile for the given media item
   * \brief aMediaItem The media item to find the profile for
   * \brief aProfile the profile found or may be null if no transcoding
   *                 is needed.
   * \note NS_ERROR_NOT_AVAILABLE is returned if no suitable profile is found
   *       but transcoding is needed
   */
  nsresult FindTranscodeProfile(sbIMediaItem * aMediaItem,
                                sbITranscodeProfile ** aProfile);
};


#endif /* __SBBASEDEVICE__H__ */

