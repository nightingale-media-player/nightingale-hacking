/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//=BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://www.songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the GPL).
// 
// Software distributed under the License is distributed
// on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
// 
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
// 
//=END SONGBIRD GPL
*/

#ifndef __SB_IPD_DEVICE_H__
#define __SB_IPD_DEVICE_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// iPod device defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbIPDDevice.h
 * \brief Songbird iPod Device Definitions.
 */

//------------------------------------------------------------------------------
//
// iPod device imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbIPDLibrary.h"
#include "sbIPDProperties.h"
#include "sbIPDStatus.h"

// Songbird imports.
#include <sbBaseDevice.h>
#include <sbDeviceCapabilities.h>
#include <sbIDatabaseQuery.h>
#include <sbIDeviceContent.h>
#include <sbIDeviceLibrary.h>
#include <sbILibraryManager.h>
#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>
#include <sbLibraryListenerHelpers.h>

// Mozilla imports.
#include <nsIClassInfo.h>
#include <nsIFileProtocolHandler.h>
#include <nsIRunnable.h>
#include <nsIStringBundle.h>
#include <nsIThread.h>
#include <nsTArray.h>
#include <prrwlock.h>

/* C++ STL imports. */
#include <map>
#include <vector>

// Libgpod imports.
#include <itdb.h>


//------------------------------------------------------------------------------
//
// Thread support strategy:
//
//   The iPod device services provide support for multi-threaded access using
// a combination of locks and request queues.  Certain iPod device object
// fields may only be accessed under a lock and some may be accessed without
// restriction.
//   The iPod device object fields are divided into different categories
// depending upon their access policy.
//   After the iPod device object is created, the only method that is
// available is the Initialize method.  It must be called immediately after
// object construction (XXXeps, currently, the object constructor calls the
// Initialize method).  After initialization, all other methods may be called;
// however, some may return a not available error (e.g., those that depend
// upon the device being connected).
//   Fields within the "not locked" category are created during initialization
// and may be accessed at any time without acquiring a lock.  These fields are
// assumed to be internally thread safe.  Fields within the "constant"
// category are created and set during initialization and should never be
// altered afterward.
//
//   Fields within the "connect lock" category must only be accessed within
// the connect lock.  All fields except mConnectLock and mConnected are
// only usable when the device is connected.  These fields are created and
// initialized during device connection and are disposed of when the device is
// disconnected.
//   The connect lock is a read/write lock.  Only the Connect and Disconnect
// methods acquire a write lock on the connect lock.  All other methods
// acquire read locks.
//   In order to access the "connect lock" fields, a method must first
// acquire a read lock on the connect lock and check the mConnected field to
// see if the device is connected.  If the device is not connected, the method
// must release the connect lock and not access any other field.  If the
// device is connected, the method may access any field as long as it holds
// the connect lock.
//   Since the connect lock is a read/write lock, acquiring a read lock does
// not lock out other threads from acquiring a read lock.  This allows the
// request thread to maintain a long lived read lock on the connect lock
// without locking out other threads; it only locks out device disconnect.
//   The "connect lock" only ensures that the "connect lock" field values won't
// change.  Thus, the values of "connect lock" pointer fields will always point
// to the same location while the connect lock is held.  However, the "connect
// lock" does not ensure that the data pointed to does not change.  If a
// "connect lock" field is not internally thread safe, it may need to be
// protected within another lock (e.g., mITDB).
//
//   Fields within the "request lock" category may only be accessed within the
// request lock.  Each time the request thread processes a request, it
// acquires the request lock until the request completes.
//
//   Fields within the "pref lock" category may only be accessed within the
// preference lock.  In order to access "pref lock" fields, a method must first
// acquire the preference lock and check the mPrefConnected field to see if the
// preference services are connected.  If not, the method must release the
// preference lock and not access any other field.
//
// Connect lock
//
//   mConnected
//   mPrefLock
//   mDeviceLibrary
//   mDeviceLibraryML
//   mMountPath
//   mITDB
//   mITDBDirty
//   mITDBDevice
//   mMasterPlaylist
//   mReqAddedEvent
//   mReqThread
//
// Request lock
//
//   mITDB
//   mITDBDirty
//   mITDBDevice
//   mMasterPlaylist
//   mIPDStatus
//   mStatsUpdatePeriod
//   mLastStatsUpdate
//
// Pref lock
//
//   mPrefConnected
//   mIPodPrefs
//   mIPodPrefsDirty
//   mSyncPlaylistList
//   mSyncPlaylistListDirty
//
// Not locked
//   mConnectLock
//   mRequestLock
//   mReqStopProcessing
//   mDeviceContent
//   mCreationProperties
//   mCreationProperties2
//   mProperties
//   mCapabilities
//   mSBMainLib
//   mSBMainML
//
// Constant
//   mDeviceID
//   mControllerID
//   mFileProtocolHandler
//   mPropertyManager
//   mLibraryManager
//   mLocale
//   mFirewireGUID
//   mIPodEventHandler
//
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//
// iPod device defs.
//
//------------------------------------------------------------------------------

//
// iPod device configuration.
//
//   IPOD_LOCALE_BUNDLE_PATH    Path to localized string bundle.
//   IPOD_STATS_UPDATE_PERIOD   Period in milliseconds for updating the iPod
//                              statistics.
//

#define IPOD_LOCALE_BUNDLE_PATH     "chrome://ipod/locale/IPodDevice.properties"
#define IPOD_STATS_UPDATE_PERIOD    500


//------------------------------------------------------------------------------
//
// iPod device globals.
//
//------------------------------------------------------------------------------

//
// sbIPDSupportedMediaList      List of supported media file extensions derived
//                 from "http://docs.info.apple.com/article.html?artnum=304784".
// sbIPDSupportedMediaListLength Length of supported media file extension list.
// sbIPDSupportedAudioMediaList List of supported audio media file extensions
//                              derived from
//                      "http://docs.info.apple.com/article.html?artnum=304784".
// sbIPDSupportedAudioMediaListLength
//                              Length of supported audio media file extension
//                              list.
//

extern const char *sbIPDSupportedMediaList[];
extern PRUint32 sbIPDSupportedMediaListLength;
extern const char *sbIPDSupportedAudioMediaList[];
extern PRUint32 sbIPDSupportedAudioMediaListLength;


//------------------------------------------------------------------------------
//
// iPod device classes.
//
//------------------------------------------------------------------------------

/**
 * This class represents an iPod device and is used to communicate with a single
 * instance of an iPod device.
 */

class sbIPDDevice : public sbBaseDevice,
                    public nsIClassInfo
{
  //----------------------------------------------------------------------------
  //
  // Class friends.
  //
  //----------------------------------------------------------------------------

  friend class sbIPDAutoDBFlush;
  friend class sbIPDAutoIdle;
  friend class sbIPDAutoTrack;
  friend class sbIPDLibrary;
  friend class sbIPDReqAddedEvent;
  friend class sbIPDProperties;


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
  NS_DECL_SBIDEVICE
  NS_DECL_NSICLASSINFO


  //
  // Constructors/destructors.
  //

  sbIPDDevice(const nsID&     aControllerID,
              nsIPropertyBag* aProperties);

  virtual ~sbIPDDevice();


  //----------------------------------------------------------------------------
  //
  // Protected interface.
  //
  //----------------------------------------------------------------------------

protected:

  //
  // Internal iPod device services.
  //

  nsresult Initialize();

  void Finalize();

  //
  // Internal iPod device properties services.
  //

  virtual nsresult InitializeProperties();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //----------------------------------------------------------------------------
  //
  // iPod device sbIDevice services.
  //
  //----------------------------------------------------------------------------

  nsresult ConnectInternal();


  //----------------------------------------------------------------------------
  //
  // iPod device mapping services.
  //
  //----------------------------------------------------------------------------

  //
  // iPod device mapping services defs.
  //
  //   TypeTrack                Item is a track.
  //   TypePlaylist             Item is a playlist.
  //
  //   SBIDDelimiter            Delimiter between fields in mapped Songbird
  //                            ID's.
  //

  static const int TypeTrack = 1;
  static const int TypePlaylist = 2;

  static const char SBIDDelimiter = ':';


  //
  // iPod device mapping services.
  //

  nsresult MapInitialize();

  void MapFinalize();


  //
  // iPod device ID mapping services.
  //

  nsresult IDMapAdd(nsAString& aSBID,
                    guint64    aIPodID);

  nsresult IDMapRemove(guint64 aIPodID);

  nsresult IDMapGet(nsAString&            aSBID,
                    nsTArray<guint64>&    aIPodIDList);

  nsresult IDMapGet(nsAString& aSBID,
                    guint64*   aIPodID);

  nsresult IDMapGet(guint64    aIPodID,
                    nsAString& aSBID);

  nsresult IDMapGet(guint64    aIPodID,
                    nsAString& aLibraryGUID,
                    nsAString& aResourceGUID);

  nsresult IDMapGet(guint64        aIPodID,
                    sbIMediaItem** aMediaItem);

  nsresult IDMapCreateDBQuery(sbIDatabaseQuery** aQuery);

  nsresult GetSBID(sbIMediaItem* aMediaItem,
                   nsAString&    aSBID);


  //
  // Internal iPod device mapping services.
  //

  nsresult IPodItemGetID(void*    aIPodItem,
                         int      aType,
                         guint64* aIPodID);

  nsresult DecodeSBID(const nsAString& aSBID,
                      nsAString&       aLibraryGUID,
                      nsAString&       aResourceGUID);

  nsresult EncodeSBID(nsAString&       aSBID,
                      const nsAString& aLibraryGUID,
                      const nsAString& aResourceGUID);

  nsresult ExecuteQuery(sbIDatabaseQuery*   aDBQuery,
                        const char*         aQueryStr,
                        sbIDatabaseResult** aDBResult);


  //----------------------------------------------------------------------------
  //
  // iPod device request services.
  //
  //----------------------------------------------------------------------------

  //
  // Request services defs.
  //
  //   REQUEST_WRITE_PREFS      Use to write the device preferences.
  //   REQUEST_SET_PROPERTY     Use to set device properties.
  //   REQUEST_SET_PREF         Use to set device preferences.
  //

  static const PRUint32 REQUEST_WRITE_PREFS =
                          REQUEST_FLAG_USER + REQUEST_FLAG_WRITE + 1;
  static const PRUint32 REQUEST_SET_PROPERTY =
                          REQUEST_FLAG_USER + REQUEST_FLAG_WRITE + 2;
  static const PRUint32 REQUEST_SET_PREF =
                          REQUEST_FLAG_USER + REQUEST_FLAG_WRITE + 3;


  //
  // Request services set named value class.
  //
  //   name                     Name of value to set.
  //   value                    Value to set.
  //

  class SetNamedValueRequest : public TransferRequest
  {
  public:
    nsString                    name;
    nsCOMPtr<nsIVariant>        value;
  };


  //
  // Request services fields.
  //
  //   mReqAddedEvent           Request added event object.
  //   mReqThread               Request processing thread.
  //   mReqStopProcessing       Non-zero if request processing should stop.
  //   mIsHandlingRequests      True if requests are being handled.
  //

  nsCOMPtr<nsIRunnable>         mReqAddedEvent;
  nsCOMPtr<nsIThread>           mReqThread;
  PRInt32                       mReqStopProcessing;
  PRBool                        mIsHandlingRequests;


  //
  // iPod device request sbBaseDevice services.
  //

  virtual nsresult ProcessRequest();


  //
  // iPod device request handler services.
  //

  nsresult ReqHandleRequestAdded();

  void ReqHandleMount(TransferRequest* aRequest);

  void ReqHandleWrite(TransferRequest* aRequest);

  void ReqHandleWriteTrack(TransferRequest* aRequest);

  void ReqHandleWritePlaylistTrack(TransferRequest* aRequest);

  void ReqHandleDelete(TransferRequest* aRequest);

  void ReqHandleDeleteTrack(TransferRequest* aRequest);

  void ReqHandleDeletePlaylistTrack(TransferRequest* aRequest);

  void ReqHandleDeletePlaylist(TransferRequest* aRequest);
  
  void ReqHandleWipe(TransferRequest* aRequest);

  void ReqHandleNewPlaylist(TransferRequest* aRequest);

  void ReqHandleUpdate(TransferRequest* aRequest);

  void ReqHandleMovePlaylistTrack(TransferRequest* aRequest);

  void ReqHandleFactoryReset(TransferRequest* aRequest);

  void ReqHandleWritePrefs(TransferRequest* aRequest);

  void ReqHandleSetProperty(TransferRequest* aRequest);

  void ReqHandleSetPref(TransferRequest* aRequest);

  //
  // iPod device request services.
  //

  nsresult ReqInitialize();

  void ReqFinalize();

  nsresult ReqConnect();

  void ReqDisconnect();

  nsresult ReqProcessingStart();

  nsresult ReqProcessingStop();

  PRBool ReqAbortActive();

  nsresult ReqPushSetNamedValue(int              aType,
                                const nsAString& aName,
                                nsIVariant*      aValue);


  //----------------------------------------------------------------------------
  //
  // iPod device track services.
  //
  //----------------------------------------------------------------------------

  //
  // iPod device track services.
  //

  nsresult ImportTracks();

  nsresult UploadTrack(sbIMediaItem* aMediaItem);

  nsresult AddTrack(sbIMediaItem* aMediaItem,
                    Itdb_Track**  aTrack);

  nsresult DeleteTrack(sbIMediaItem* aMediaItem);

  nsresult DeleteTrack(Itdb_Track* aTrack);

  nsresult WipeDevice();

  nsresult GetTrack(sbIMediaItem* aMediaItem,
                    Itdb_Track**  aTrack);


  //
  // iPod device track property services.
  //

  void SetTrackProperties(Itdb_Track*   aTrack,
                          sbIMediaItem* aMediaItem);

  void SetTrackProperties(sbIMediaItem* aMediaItem,
                          Itdb_Track*   aTrack);

  nsresult GetTrackProperties(Itdb_Track*        aTrack,
                              sbIPropertyArray** aPropertyArray);

  nsresult GetTrackProperties(Itdb_Track*      aTrack,
                              nsIMutableArray* aPropertiesArrayArray);

  nsresult TrackUpdateProperties(sbIMediaItem* aMediaItem,
                                 Itdb_Track*   aTrack = NULL);

  nsresult GetTrackProp(sbIMediaItem* aMediaItem,
                        const char*   aPropName,
                        gchar**       aProp);

  nsresult SetTrackProp(sbIMediaItem* aMediaItem,
                        const char*   aPropName,
                        gchar*        aProp);

  nsresult SetTrackProp(sbIMutablePropertyArray* aPropertyArray,
                        const char*              aPropName,
                        const nsAString&         aProp);

  nsresult SetTrackProp(sbIMutablePropertyArray* aPropertyArray,
                        const char*              aPropName,
                        gchar*                   aProp);

  nsresult GetTrackProp(sbIMediaItem* aMediaItem,
                        const char*   aPropName,
                        gint*         aProp);

  nsresult SetTrackProp(sbIMediaItem* aMediaItem,
                        const char*   aPropName,
                        gint          aProp);

  nsresult SetTrackProp(sbIMutablePropertyArray* aPropertyArray,
                        const char*              aPropName,
                        gint                     aProp);

  nsresult GetTrackProp(sbIMediaItem* aMediaItem,
                        const char*   aPropName,
                        guint32*      aProp);

  nsresult SetTrackProp(sbIMediaItem* aMediaItem,
                        const char*   aPropName,
                        guint32       aProp);

  nsresult SetTrackProp(sbIMutablePropertyArray* aPropertyArray,
                        const char*              aPropName,
                        guint32                  aProp);

  nsresult GetTrackProp(sbIMediaItem* aMediaItem,
                        const char*   aPropName,
                        guint64*      aProp);

  nsresult SetTrackProp(sbIMediaItem* aMediaItem,
                        const char*   aPropName,
                        guint64       aProp);

  nsresult SetTrackProp(sbIMutablePropertyArray* aPropertyArray,
                        const char*              aPropName,
                        guint64                  aProp);

  nsresult GetTrackPropDur(sbIMediaItem* aMediaItem,
                           const char*   aPropName,
                           gint*         aProp);

  nsresult SetTrackPropDur(sbIMediaItem* aMediaItem,
                           const char*   aPropName,
                           gint          aProp);

  nsresult SetTrackPropDur(sbIMutablePropertyArray* aPropertyArray,
                           const char*              aPropName,
                           gint                     aProp);

  nsresult GetTrackPropRating(sbIMediaItem* aMediaItem,
                              const char*   aPropName,
                              guint32*      aProp);

  nsresult SetTrackPropRating(sbIMediaItem* aMediaItem,
                              const char*   aPropName,
                              guint32       aProp);

  nsresult SetTrackPropRating(sbIMutablePropertyArray* aPropertyArray,
                              const char*              aPropName,
                              guint32                  aProp);

  nsresult GetTrackPropFileType(sbIMediaItem* aMediaItem,
                                gchar**       aProp);


  //
  // iPod device track info services.
  //

  nsresult GetTrackURI(Itdb_Track* aTrack,
                       nsIURI**    aTrackURI);

  nsresult TrackGetFile(Itdb_Track* aTrack,
                        nsIFile**   aTrackFile);


  //
  // Internal iPod device track services.
  //

  nsresult ImportTrackBatch(Itdb_Track** aTrackBatch,
                            PRUint32     aBatchCount);

  static nsresult ImportTrackBatch1(nsISupports* aUserData);

  nsresult ImportTrackBatch2(Itdb_Track** aTrackBatch,
                             PRUint32     aBatchCount);

  void RemoveTrackFromAllPlaylists(Itdb_Track* aTrack);

  PRBool IsMediaSupported(sbIMediaItem* aMediaItem);

  void AddUnsupportedMediaItem(sbIMediaItem* aMediaItem);


  //----------------------------------------------------------------------------
  //
  // iPod device playlist services.
  //
  //----------------------------------------------------------------------------

  //
  // iPod device playlist services configuration.
  //
  //   TrackBatchSize         Number of tracks to process in a batch.
  //

  static const PRUint32 TrackBatchSize = 100;


  //
  // iPod device playlist import services.
  //

  nsresult ImportPlaylists();

  nsresult ImportPlaylist(Itdb_Playlist* aPlaylist);

  nsresult ImportPlaylist(sbILibrary*    aLibrary,
                          Itdb_Playlist* aPlaylist,
                          sbIMediaList** aMediaList = nsnull);

  nsresult ImportPlaylistTracks(Itdb_Playlist* aPlaylist,
                                sbIMediaList*  aMediaList);

  nsresult ImportPlaylistTrackBatch(sbIMediaList* aMediaList,
                                    Itdb_Track**  aTrackBatch,
                                    PRUint32      aBatchCount);


  //
  // iPod device playlist manipulation services.
  //

  nsresult PlaylistAdd(sbIMediaList*   aMediaList,
                       Itdb_Playlist** aPlaylist);

  nsresult PlaylistDelete(sbIMediaList* aMediaList);

  nsresult PlaylistUpdateProperties(sbIMediaList*  aMediaList,
                                    Itdb_Playlist* aPlaylist = NULL);

  nsresult PlaylistAddTrack(sbIMediaList* aMediaList,
                            sbIMediaItem* aMediaItem,
                            PRUint32      aIndex);

  nsresult PlaylistRemoveTrack(sbIMediaList* aMediaList,
                               sbIMediaItem* aMediaItem,
                               PRUint32      aIndex);

  nsresult PlaylistMoveTrack(sbIMediaList* aMediaList,
                             PRUint32      aIndexFrom,
                             PRUint32      aIndexTo);

  nsresult PlaylistWipe(sbIMediaList * aMediaList);


  //
  // iPod device on-the-go playlist services.
  //

  nsresult ProcessOTGPlaylists();

  nsresult SetOTGPlaylistName(Itdb_Playlist* aPlaylist,
                              PRUint32       aPlaylistIndex);


  //
  // iPod device playlist services.
  //

  nsresult GetPlaylist(sbIMediaItem*    aMediaItem,
                       Itdb_Playlist**  aPlaylist);


  //----------------------------------------------------------------------------
  //
  // iPod device FairPlay services.
  //
  //----------------------------------------------------------------------------

  //
  // FairPlay services fields.
  //
  //   mFPUserIDList            List of FairPlay user IDs for which the device
  //                            is authorized.
  //

  nsTArray<PRUint32>            mFPUserIDList;


  //
  // FairPlay services.
  //

  nsresult FPConnect();

  void FPDisconnect();

  nsresult FPCheckTrackAuth(Itdb_Track* aTrack);

  nsresult FPSetupTrackInfo(nsIFile*    aTrackFile,
                            Itdb_Track* aTrack);


  //
  // Internal FairPlay services.
  //

  nsresult FPGetKeyInfo();

  nsresult FPGetTrackAuthInfo(Itdb_Track*      aTrack,
                              PRUint32*        aUserID,
                              nsAString&       aAccountName,
                              nsAString&       aUserName);


  //----------------------------------------------------------------------------
  //
  // iPod device connection services.
  //
  //----------------------------------------------------------------------------

  nsresult DBConnect();

  void DBDisconnect();

  nsresult LibraryConnect();

  void LibraryDisconnect();

  nsresult InitSysInfo();


  //----------------------------------------------------------------------------
  //
  // iPod device preference services.
  //
  //----------------------------------------------------------------------------

  //
  // Preference services fields.
  //
  //   mPrefLock                Preference lock.
  //   mPrefConnected           True if preference services are connected.
  //   mIPodPrefs               iPod preferences record.
  //   mIPodPrefsDirty          True if the iPod preferences are dirty and need
  //                            to be written.
  //   mSyncPlaylistList        List of playlists from which to synchronize.
  //   mSyncPlaylistListDirty   True if the sync playlist list is dirty and
  //                            needs to be written.
  //

  PRLock*                       mPrefLock;
  PRBool                        mPrefConnected;
  Itdb_Prefs*                   mIPodPrefs;
  PRBool                        mIPodPrefsDirty;
  nsTArray<guint64>             mSyncPlaylistList;
  PRBool                        mSyncPlaylistListDirty;


  //
  // Preference services.
  //

  nsresult PrefInitialize();

  void PrefFinalize();

  nsresult PrefConnect();

  void PrefDisconnect();


  //
  // Preference sbIDeviceLibrary services.
  //

  nsresult GetMgmtType(PRUint32* aMgmtType);

  nsresult SetMgmtType(PRUint32 aMgmtType);

  nsresult GetIsSetUp(PRBool* aIsSetUp);

  nsresult SetIsSetUp(PRBool aIsSetUp);

  nsresult GetSyncPlaylistList(nsIArray** aPlaylistList);

  nsresult SetSyncPlaylistList(nsIArray* aPlaylistList);

  nsresult AddToSyncPlaylistList(sbIMediaList* aPlaylist);

  nsresult GetSyncPlaylist(sbIMediaItem* aPlaylist,
                           guint64*      aIPodID);


  //
  // Preference iPod preferences services.
  //

  nsresult PrefConnectIPodPrefs();

  void PrefDisconnectIPodPrefs();

  nsresult PrefInitializeSyncPartner();


  //----------------------------------------------------------------------------
  //
  // iPod device sync services.
  //
  //----------------------------------------------------------------------------

  //
  // iPod device sync track services.
  //

  nsresult SyncFromIPodTracks();

  nsresult SyncFromIPodTrack(Itdb_Track* aTrack);


  //
  // iPod device sync services.
  //

  nsresult SyncFromDevice();

  nsresult SyncFromOTGPlaylists();


  //----------------------------------------------------------------------------
  //
  // iPod device statistics services.
  //
  //----------------------------------------------------------------------------

  //
  // Statistics services fields.
  //
  //   mStatsUpdatePeriod       Statistics update time period.
  //   mLastStatsUpdate         Time of last statistics update.
  //

  PRIntervalTime                mStatsUpdatePeriod;
  PRIntervalTime                mLastStatsUpdate;


  //
  // Statistics services.
  //

  nsresult StatsInitialize();

  void StatsFinalize();

  void StatsUpdate(PRBool aForceUpdate);


  //----------------------------------------------------------------------------
  //
  // iPod device event services.
  //
  //----------------------------------------------------------------------------

  //
  // mIPodEventHandler          Handler for iPod specific events.
  //

  nsCOMPtr<nsISupports>         mIPodEventHandler;


  //
  // Device event services.
  //

  nsresult CreateAndDispatchFairPlayEvent(PRUint32      aType,
                                          PRUint32      aUserID,
                                          nsAString&    aAccountName,
                                          nsAString&    aUserName,
                                          sbIMediaItem* aMediaItem = nsnull,
                                          PRBool        aAsync = PR_TRUE);


  //----------------------------------------------------------------------------
  //
  // Internal iPod device services.
  //
  //----------------------------------------------------------------------------

  //
  // Internal services fields.
  //
  //   mConnectLock             Connect lock.
  //   mRequestLock             Request lock.
  //
  //   mDeviceID                ID of device.
  //   mControllerID            ID of device controller.
  //   mDeviceContent           Device content object.
  //   mDeviceLibrary           Device library object.
  //   mDeviceLibraryML         sbIMediaList interface for device library.
  //   mCreationProperties      Set of device creation properties.
  //   mCreationProperties2     mCreationProperties as an nsIPropertyBag2.
  //   mProperties              Set of device properties.
  //   mCapabilities            Set of device capabilities.
  //
  //   mITDB                    Libgpod iPod database data record.
  //   mITDBDirty               True if the database is dirty and needs to be
  //                            written.
  //   mITDBDevice              Libgpod iPod device data record.
  //   mMasterPlaylist          iPod database master playlist.
  //
  //   mFileProtocolHandler     File protocol handler object.
  //   mPropertyManager         Media property manager object.
  //   mLibraryManager          Songbird library manager object.
  //   mLocale                  Localized string bundle.
  //
  //   mConnected               True if device is connected.
  //   mFirewireGUID            Device Firewire GUID.
  //   mIPDStatus               iPod device status object.
  //   mMountPath               iPod device file system mount path.
  //   mSBMainLib               Songbird main library.
  //   mSBMainML                Songbird main library media list.
  //

  PRRWLock*                     mConnectLock;
  PRMonitor*                    mRequestLock;

  nsID                          mDeviceID;
  nsID                          mControllerID;
  nsCOMPtr<sbIDeviceContent>    mDeviceContent;
  nsRefPtr<sbIPDLibrary>        mDeviceLibrary;
  nsCOMPtr<sbIMediaList>        mDeviceLibraryML;
  nsCOMPtr<nsIPropertyBag>      mCreationProperties;
  nsCOMPtr<nsIPropertyBag2>     mCreationProperties2;
  nsRefPtr<sbIPDProperties>     mProperties;
  nsCOMPtr<sbIDeviceCapabilities>
                                mCapabilities;

  Itdb_iTunesDB*                mITDB;
  PRBool                        mITDBDirty;
  Itdb_Device*                  mITDBDevice;
  Itdb_Playlist*                mMasterPlaylist;

  nsCOMPtr<nsIFileProtocolHandler>
                                mFileProtocolHandler;
  nsCOMPtr<sbIPropertyManager>  mPropertyManager;
  nsCOMPtr<sbILibraryManager>   mLibraryManager;
  nsCOMPtr<nsIStringBundle>     mLocale;

  PRBool                        mConnected;
  nsString                      mFirewireGUID;
  sbIPDStatus*                  mIPDStatus;
  nsString                      mMountPath;
  nsCOMPtr<sbILibrary>          mSBMainLib;
  nsCOMPtr<sbIMediaList>        mSBMainML;


  //
  // Internal iPod device services.
  //

  nsresult ImportDatabase();

  nsresult DBFlush();

  nsresult CapabilitiesConnect();

  nsresult GetIPodID(sbIMediaItem* aMediaItem,
                     guint64*      aIPodID);

  nsresult CreateDeviceID(nsID* aDeviceID);

  nsresult AddNSIDHash(unsigned char*   aBuffer,
                       PRInt32&         aOffset,
                       nsIPropertyBag2* aPropBag,
                       const nsAString& aProp);

  PRBool IsFileSystemSupported();

  nsresult SetUpIfNeeded();
};


/**
 *   This class provides an nsIRunnable interface that may be used to dispatch
 * and process device request added events.
 */

class sbIPDReqAddedEvent : public nsIRunnable
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

  static nsresult New(sbIPDDevice*  aDevice,
                      nsIRunnable** aEvent);


  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // Private fields.
  //
  //   mDevice                  Device associated with event.
  //

  nsRefPtr<sbIPDDevice>         mDevice;


  // Make default constructor private to force use of factory methods.
  sbIPDReqAddedEvent() {};
};


/**
 * Auto-disposal class wrappers.
 *
 *   sbIPDAutoDBFlush           Wrapper to auto-flush the iPod database.
 *   sbIPDAutoIdle              Wrapper to auto idle the iPod device.
 *   sbIPDAutoTrack             Wrapper to auto delete track from the iPod
 *                              device.  First constructor parameter is track
 *                              and the second is the iPod device object.
 *   sbIPDAutoStopIgnoreLibrary Wrapper to auto stop ignoring library changes.
 *   sbIPDAutoStopIgnoreMediaLists
 *                              Wrapper to auto stop ignoring media list
 *                              changes.
 *   sbIPDAutoFalse             Wrapper to automatically set a boolean to false.
 */

SB_AUTO_CLASS(sbIPDAutoDBFlush,
              sbIPDDevice*,
              mValue,
              mValue->DBFlush(),
              mValue = nsnull);

SB_AUTO_CLASS(sbIPDAutoIdle,
              sbIPDDevice*,
              mValue,
              mValue->mIPDStatus->Idle(),
              mValue = nsnull);

SB_AUTO_CLASS2(sbIPDAutoTrack,
               Itdb_Track*,
               sbIPDDevice*,
               mValue,
               mValue2->DeleteTrack(mValue),
               mValue = NULL);

SB_AUTO_CLASS(sbIPDAutoStopIgnoreLibrary,
              sbBaseDeviceLibraryListener*,
              mValue,
              mValue->SetIgnoreListener(PR_FALSE),
              mValue = nsnull);

SB_AUTO_CLASS(sbIPDAutoStopIgnoreMediaLists,
              sbIPDDevice*,
              mValue,
              mValue->SetIgnoreMediaListListeners(PR_FALSE),
              mValue = nsnull);

SB_AUTO_CLASS(sbIPDAutoFalse,
              PRBool*,
              mValue,
              *mValue = PR_FALSE,
              mValue = nsnull);


//------------------------------------------------------------------------------
//
// iPod device macros.
//
//------------------------------------------------------------------------------

/**
 * Define a class with the name specified by aName to automatically invoke the
 * sbIPDDevice method specified by aMethod upon scope exit.
 *
 * \param aName                 Name of class.
 * \param aMethod               Method invocation.
 *
 * Example:
 *   SB_IPD_DEVICE_AUTO_INVOKE(AutoDisconnect, PrefDisconnect())
 *     autoDisconnect(this);
 *
 *   This example will call PrefDisconnect() when exiting the scope.  This may
 * be cancelled by calling autoDisconnect.forget().
 */

#define SB_IPD_DEVICE_AUTO_INVOKE(aName, aMethod)                              \
  SB_AUTO_CLASS(aName, sbIPDDevice*, mValue, mValue->aMethod, mValue = nsnull)


#endif // __SB_IPD_DEVICE_H__

