/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2011 POTI, Inc.
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

#include <algorithm>
#include <list>
#include <map>
#include <vector>

#include "sbIDevice.h"
#include "sbBaseDeviceEventTarget.h"
#include "sbBaseDeviceVolume.h"
#include "sbDeviceLibrary.h"

#include <nsArrayUtils.h>
#include <nsAutoPtr.h>
#include <nsClassHashtable.h>
#include <nsCOMPtr.h>
#include <nsCOMArray.h>
#include <nsDataHashtable.h>
#include <nsIRunnable.h>
#include <nsISupportsImpl.h>
#include <nsITimer.h>
#include <nsIFile.h>
#include <nsIURL.h>
#include <nsTArray.h>

#include <sbAutoRWLock.h>
#include <sbILibraryChangeset.h>
#include <sbIMediaItem.h>
#include <sbIMediaList.h>
#include "sbRequestItem.h"
#include "sbDeviceRequestThreadQueue.h"
#include <sbITemporaryFileFactory.h>
#include <sbITranscodeManager.h>

#include <sbMemoryUtils.h>
#include <nsComponentManagerUtils.h>

#include "sbDeviceStatistics.h"

class nsIArray;
class nsIMutableArray;
class nsIPrefBranch;

class sbAutoIgnoreWatchFolderPath;
class sbBaseDeviceLibraryListener;
class sbDeviceBaseLibraryCopyListener;
class sbDeviceStatusHelper;
class sbDeviceSupportsItemHelper;
class sbDeviceTranscoding;
class sbDeviceImages;
class sbBaseDeviceMediaListListener;
class sbIDeviceInfoRegistrar;
class sbIDeviceLibrary;
class sbITranscodeProfile;
class sbITranscodeAlbumArt;
class sbIMediaFormat;

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
  friend class sbBaseDeviceVolume;
  friend class sbDeviceRequestThreadQueue;
  friend class sbDevieEnsureSpaceForWrite;

  typedef sbRequestThreadQueue::Batch Batch;

  struct TransferRequest : public sbRequestItem {
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

      /* write requests */
      REQUEST_WRITE         = sbIDevice::REQUEST_WRITE,
      REQUEST_DELETE        = sbIDevice::REQUEST_DELETE,
      REQUEST_SYNC          = sbIDevice::REQUEST_SYNC,
      REQUEST_IMAGESYNC     = sbIDevice::REQUEST_IMAGESYNC,
      REQUEST_WRITE_FILE    = sbIDevice::REQUEST_WRITE_FILE,
      REQUEST_DELETE_FILE   = sbIDevice::REQUEST_DELETE_FILE,
      /* delete all files */
      REQUEST_WIPE          = sbIDevice::REQUEST_WIPE,
      /* move an item in one playlist */
      REQUEST_MOVE          = sbIDevice::REQUEST_MOVE,
      REQUEST_UPDATE        = sbIDevice::REQUEST_UPDATE,
      REQUEST_NEW_PLAYLIST  = sbIDevice::REQUEST_NEW_PLAYLIST,
      REQUEST_FORMAT        = sbIDevice::REQUEST_FORMAT,
      REQUEST_SYNC_COMPLETE = sbIDevice::REQUEST_SYNC_COMPLETE
    };

    enum {
      REQUESTBATCH_UNKNOWN       = 0,
      REQUESTBATCH_AUDIO         = 1,
      REQUESTBATCH_VIDEO         = 2,
      REQUESTBATCH_IMAGE         = 4
    };

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

    nsCOMPtr<nsIFile> downloadedFile; /* used if request item content source had
                                         to be downloaded */

    /**
     * Returns PR_TRUE if the request is for a playlist and PR_FALSE otherwise
     * The playlist test is determined by checking "list". If list is not set
     * then this is not a playlist request. If list is set AND list is not a
     * sbILibrary then this is a playlist request.
     */
    PRBool IsPlaylist() const;

    /**
     * Returns PR_TRUE if the request should be counted as part of the batch,
     * otherwise returns PR_FALSE
     */
    PRBool IsCountable() const;

    static TransferRequest * New(PRUint32 aType,
                                 sbIMediaItem * aItem,
                                 sbIMediaList * aList,
                                 PRUint32 aIndex,
                                 PRUint32 aOtherIndex,
                                 nsISupports * aData);

    /* Don't allow manual construction/destruction, but allow sub-classing. */
  protected:
    TransferRequest();

    ~TransferRequest(); /* we're refcounted, no manual deleting! */
  };

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
  NS_IMETHOD SetState(PRUint32 aState);
  NS_IMETHOD GetPreviousState(PRUint32 *aState);
  NS_IMETHOD SyncLibraries(void);
  /**
   * return NS_ERROR_ABORT if user aborts the ejection while playback is on
   * device item.
   */
  NS_IMETHOD Eject(void);
  NS_IMETHOD Format(void);
  NS_IMETHOD GetSupportsReformat(PRBool *_retval);
  NS_IMETHOD SupportsMediaItem(sbIMediaItem*                  aMediaItem,
                               sbIDeviceSupportsItemCallback* aCallback);
  NS_IMETHOD GetDefaultLibrary(sbIDeviceLibrary** aDefaultLibrary);
  NS_IMETHOD SetDefaultLibrary(sbIDeviceLibrary* aDefaultLibrary);
  NS_IMETHOD GetPrimaryLibrary(sbIDeviceLibrary** aPrimaryLibrary);
  NS_IMETHOD SubmitRequest(PRUint32 aRequest,
                           nsIPropertyBag2 *aRequestParameters);
  NS_IMETHOD CancelRequests();

  NS_IMETHOD ImportFromDevice(sbILibrary * aImportToLibrary,
                              sbILibraryChangeset * aImportChangeset);

  NS_IMETHOD ExportToDevice(sbIDeviceLibrary*    aDevLibrary,
                            sbILibraryChangeset* aChangeset);

public:

  sbBaseDevice();
  virtual ~sbBaseDevice();

  /**
   *  add a transfer/action request to the request queue
   */
  nsresult  PushRequest(const PRUint32 aType,
                        sbIMediaItem* aItem = nsnull,
                        sbIMediaList* aList = nsnull,
                        PRUint32 aIndex = PR_UINT32_MAX,
                        PRUint32 aOtherIndex = PR_UINT32_MAX,
                        nsISupports * aData = nsnull);

  /**
   * Starts a batch of requests
   */
  nsresult BatchBegin();

  /**
   * Marks the end of a batch of requests
   */
  nsresult BatchEnd();
  /**
   * Clears requests from the request queue
   */
  nsresult ClearRequests();

  /**
   * Return in aTemporaryFileFactory a temporary file factory for the request
   * specified by aRequest, creating one if necessary.  When the request
   * completes and is destroyed, and any external references to the temporary
   * file factory are released, the temporary file factory will be destroyed,
   * and all of its temporary files will be deleted.
   *
   * \param aRequest            Request for which to get temporary file factory.
   * \param aTemporaryFileFactory
   *                            Returned temporary file factory for request.
   */
  nsresult GetRequestTemporaryFileFactory
             (TransferRequest*          aRequest,
              sbITemporaryFileFactory** aTemporaryFileFactory);

  /**
   * Download the item for the request specified by aRequest and update the
   * request downloadedFile field.  Use the device status helper speicifed by
   * aDeviceStatusHelper to update the device status.
   *
   * \param aRequest              Request for which to download item.
   * \param aDeviceStatusHelper   Helper to use to update device status.
   */
  nsresult DownloadRequestItem(TransferRequest*       aRequest,
                               PRUint32               aBatchCount,
                               sbDeviceStatusHelper*  aDeviceStatusHelper);

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

  /**
   * Process this batch of requests
   */
  virtual nsresult ProcessBatch(Batch & aBatch) = 0;

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

  /**
   * Set the content source for the media item in the request specified by
   * aRequest to the URI specified by aURI.  If the content source has not
   * already been set for the request, copy the media item content source to the
   * origin URL property before updating the content source to aURI.
   *
   * \param aRequest            Request for which to update media item.
   * \param aURI                New content source URI.
   */
  nsresult UpdateOriginAndContentSrc(TransferRequest* aRequest,
                                     nsIURI*          aURI);

  nsresult CreateTransferRequest(PRUint32 aRequestType,
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
   * Start processing device requests.
   */
  nsresult ReqProcessingStart();

  /**
   * Stop processing device requests.
   */
  nsresult ReqProcessingStop(nsIRunnable * aShutdownAction);

  /**
   * Returns true if the current request should be aborted.
   * It also resets the abort flag.
   */
  bool CheckAndResetRequestAbort();
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
  NS_SCRIPTABLE NS_IMETHOD OpenInputStream(nsIURI*          aURI,
                                           nsIInputStream** retval);

  /**
   * Returns true if the request has been aborted or the device is
   * disconnected. This calls CheckAndResetAbort and so once this is called
   * the caller should not call this again for the current request if it returns
   * true
   */
  virtual PRBool IsRequestAborted();

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

  /**
   * Changes the state of the underlying deviceStatusHelper.  This means that
   * the devices status and substate are set as well as a call to SetState
   * for setting to certain states.
   *
   * The relation and individual uses of ChangeState and SetState are not
   * very well defined.  Please take great care before using this method.
   * If you can use SetState you probably should, but there may be some
   * instances when you have to use ChangeState.
   *
   */
  virtual nsresult ChangeState(PRUint32 aState);

private:
  /**
   * Derived devices must implement this to perform any disconnect logic
   * they need. Making it a pure virtual so it's not forgotten.
   */
  virtual nsresult DeviceSpecificDisconnect() = 0;
  /**
   * Helper for PopRequest / PeekRequest
   */
  nsresult GetFirstRequest(bool aRemove, TransferRequest** _retval);
protected:


  PRLock *mStateLock;
  PRUint32 mState;
  PRLock *mPreviousStateLock;
  PRUint32 mPreviousState;
  PRInt32 mIgnoreMediaListCount; // Allows us to know if we're ignoring lists
  PRUint32 mPerTrackOverhead; // estimated bytes of overhead per track
  nsAutoPtr<sbDeviceStatusHelper> mStatus;

  nsCOMPtr<sbIDeviceLibrary> mDefaultLibrary;
  nsCOMPtr<sbILibrary> mMainLibrary;
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

  // This is a hack so that the UI has the information of what kind of media
  // the set of sync changes contains. TODO: XXX we should look at replacing
  // this at some point.
  PRUint32 mSyncType;

  bool mEnsureSpaceChecked;

  //
  //   mConnectLock             Connect lock.
  //   mConnected               True if device is connected.
  //   mDeferredSetupDeviceTimer Timer used to defer presentation of the device
  //                             setup dialog.
  //   mRequestThreadQueue      This contains the logic to process requests
  //                            for the device thread

  PRRWLock* mConnectLock;
  PRBool mConnected;
  nsCOMPtr<nsITimer> mDeferredSetupDeviceTimer;
  nsRefPtr<sbDeviceRequestThreadQueue> mRequestThreadQueue;

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
   * Make sure that there is enough free space for the changeset. If there is
   * not enough space for all the changes and the user does not abort
   * the operation, items in the changeset will be removed.
   *
   * We preference different types of change operations differently.
   * Most commonly the changeset will be composed of ADD and MODIFIED changes,
   * and we preference non-ADD changes over ADDs so that we will perform
   * all modifications before considering ADDs.
   *
   * \param aChangeset The changest to ensure space for. The changes within
   *                   the changeset may be modified on return.
   * \param aDevLibrary The device library representing the device for whom the
   *                    changeset was generated.
   */
  nsresult EnsureSpaceForWrite(sbILibraryChangeset* aChangeset,
                               sbIDeviceLibrary* aDevLibrary);

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

  nsDataHashtableMT<nsStringHashKey, PRBool> mTrackSourceTable;

  /**
   * Update the destination compatibility for the streaming items in the batch.
   *
   * \param aBatch        The batch to be updated.
   */

  nsresult UpdateStreamingItemSupported(Batch & aBatch);

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
   * Send a sync-completed request to be handled by the device.
   */
  nsresult SendSyncCompleteRequest();

  /**
   * Handle the sync request specified by aRequest.
   *
   * \param aRequest              Request data record.
   */
  nsresult HandleSyncRequest(TransferRequest* aRequest);

  /**
   * Handle the sync-completed request, updating the last-sync timestamp.
   *
   * \param aRequest              Request data record.
   */
  nsresult HandleSyncCompletedRequest(TransferRequest* aRequest);

  /**
   * Ensure enough space is available for the sync request specified by
   * aRequest.  If not, ask the user if the sync should be changed to a random
   * subset of the items to sync that fits in the available space.  If the user
   * chooses to do so, create the list and configure to sync to it.  If the user
   * chooses to abort, return true in aAbort.
   *
   * \param aChangeset          The change set to check for space
   * \param aCapacityExceeded   Returns whether the capacity was exceeded
   * \param aAbort              Abort sync if true.
   */
  nsresult EnsureSpaceForSync(sbILibraryChangeset* aChangeset,
                              PRBool*              aCapacityExceeded,
                              PRBool*              aAbort);

  /**
   * Create a random subset of sync items from the list specified by
   * aSyncItemList with sizes specified by aSyncItemSizeMap that fits in the
   * available space specified by aAvailableSpace.  Create a sync media list
   * from this subset within the source library specified by aSrcLib and
   * configure the destination library specified by aDstLib to sync to it.
   *
   * \param aSrcLib             Source library from which to sync.
   * \param aDstLib             Destination library to which to sync.
   * \param aAvailableSpace     Space available for sync.
   * \return NS_OK if successful.
   *         NS_ERROR_ABORT if user aborts syncing.
   *         else some other NS_ERROR value.
   */
  nsresult SyncCreateAndSyncToList
             (sbILibrary*                                   aSrcLib,
              sbIDeviceLibrary*                             aDstLib,
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
   * Returns the size for a given change. If it's a modification it will return
   * the difference between the original and the new version
   *
   * \param aDestLibrary The destination library for the change
   * \param aChange The change item to use
   */
  PRInt64 GetChangeSize(sbIDeviceLibrary * aDestLibrary,
                        sbILibraryChange * aChange);
  /**
   * Return in aSyncItemList and aSyncItemSizeMap the set of items to sync and
   * their sizes, as configured for the destination sync library specified by
   * aDstLib.  The sync source library is specified by aSrcLib.  The total size
   * of all sync items is specified by aTotalSyncSize.
   *
   * \param aDstLib             Sync destination library.
   * \param aChangeset          List of items from which to sync.
   * \param aAvailableSpace     The available space on the device in bytes
   * \param aLastChangeThatFit  Index of the last item that fits in available
   *                            space
   * \param aTotalSyncSize      Total size of all sync items.
   */
  nsresult SyncGetSyncItemSizes(sbIDeviceLibrary * aDestLibrary,
                                sbILibraryChangeset* aChangeset,
                                PRInt64 aAvailableSpace,
                                PRUint32& aLastChangeThatFit,
                                PRInt64& aTotalSyncSize);

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
   * \param aSyncChangeset      Sync request change set.
   * \param aImportChangeset    Changes for the import operation
   */
  nsresult SyncProduceChangeset(TransferRequest*      aRequest,
                                sbILibraryChangeset** aExportChangeset,
                                sbILibraryChangeset** aImportChangeset);

  /**
   * Check whether a media item points to a currently existing main library
   * item as its origin, and update SB_PROPERTY_ORIGIN_IS_IN_MAIN_LIBRARY
   * accordingly.  This function is intended for items in the device's
   * library.
   *
   * \param aMediaItem          Media item to check.
   */
  nsresult SyncMainLibraryFlag(sbIMediaItem * aMediaItem);

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
   * Get the value of the transcoding property with the name specified by
   * aPropertyName and transcoding type specified by aTranscodeType.  Return the
   * value in aPropertyValue.
   *
   * \param aTranscodeType      Type of transcode. E.g.,
   *                            sbITranscodeProfile::TRANSCODE_TYPE_AUDIO.
   * \param aPropertyName       Name of property.
   * \param aPropertyValue      Returned property value.
   */
  nsresult GetDeviceTranscodingProperty(PRUint32         aTranscodeType,
                                        const nsAString& aPropertyName,
                                        nsIVariant**     aPropertyValue);

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
      sbBaseDevice::TransferRequest * request = *iter;
      PRUint32 type = request->GetType();

      // If this is a request that adds an item to the device we need to remove
      // it from the device since it never was copied
      switch (type) {
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
  nsresult  GetExcludedFolders(nsTArray<nsString> & aExcludedFolders);

  /**
   * This updates a collection of media lists.
   * \param aMediaLists The list of media lists to be updated
   */
  nsresult UpdateMediaLists(nsIArray * aMediaLists);

  /**
   * This adds teh media lists to the given library
   * \param aLibrary The library the media lists are to be added
   * \param aMediaLists The list of media lists to be added
   */
  nsresult AddMediaLists(sbILibrary * aLibrary,
                         nsIArray * aMediaLists);

  /**
   * Returns boolean flags for whether we should import audio and/or video
   * \param aSettings A sync settings object
   * \param aImportAudio Returns whether audio should be imported
   * \param aImportVideo Returns whether video should be imported
   */
  static nsresult GetImportSettings(sbIDeviceLibrary * aDevLibrary,
                                    PRBool * aImportAudio,
                                    PRBool * aImportVideo);

  /**
   * Copies media items to a media list for a media list change.
   * \param aChange the change to get the items from
   * \param aMediaList the list to copy to
   */
  nsresult CopyChangedMediaItemsToMediaList(sbILibraryChange * aChange,
                                            sbIMediaList * aMediaList);
};


void SBWriteRequestSplitBatches(const sbBaseDevice::Batch & aInput,
                                sbBaseDevice::Batch & aNonTranscodeItems,
                                sbBaseDevice::Batch & aTrancodeItems,
                                sbBaseDevice::Batch & aPlaylistItems);
//
// Auto-disposal class wrappers.
//
//   sbAutoResetEnsureSpaceChecked
//                              Wrapper to automatically reset the space
//                              checked flag.

SB_AUTO_NULL_CLASS(sbAutoResetEnsureSpaceChecked,
                   sbBaseDevice*,
                   mValue->SetEnsureSpaceChecked(false));

#endif /* __SBBASEDEVICE__H__ */
