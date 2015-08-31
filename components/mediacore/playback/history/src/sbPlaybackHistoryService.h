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

#ifndef __SB_PLAYBACKHISTORYSERVICE_H__
#define __SB_PLAYBACKHISTORYSERVICE_H__

#include <sbIPlaybackHistoryService.h>

#include <nsIArray.h>
#include <nsIClassInfo.h>
#include <nsIComponentManager.h>
#include <nsIFile.h>
#include <nsIGenericFactory.h>
#include <nsIObserver.h>
#include <nsIWeakReference.h>

#include <nsCOMPtr.h>
#include <nsDataHashtable.h>
#include <nsHashKeys.h>
#include <nsInterfaceHashtable.h>
#include <nsStringGlue.h>

#include <prmon.h>
#include <prtime.h>

#include <sbIDatabaseQuery.h>
#include <sbILibrary.h>
#include <sbIMediacoreEventListener.h>
#include <sbIMediaItem.h>
#include <sbIMediaListView.h>
#ifdef METRICS_ENABLED
#include <sbIMetrics.h>
#endif
#include <sbIPlaybackHistoryListener.h>

class sbPlaybackHistoryService : public sbIPlaybackHistoryService,
                                 public sbIMediacoreEventListener,
                                 public nsIObserver
{
public:
  sbPlaybackHistoryService();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_SBIMEDIACOREEVENTLISTENER
  NS_DECL_SBIPLAYBACKHISTORYSERVICE

  static NS_METHOD RegisterSelf(nsIComponentManager* aCompMgr,
                                nsIFile* aPath,
                                const char* aLoaderStr,
                                const char* aType,
                                const nsModuleComponentInfo *aInfo);

  static PLDHashOperator PR_CALLBACK AddListenersToCOMArrayCallback(
                                          nsISupportsHashKey::KeyType aKey,
                                          sbIPlaybackHistoryListener* aEntry,
                                          void* aUserData);

  NS_METHOD Init();
  
  nsresult CreateQueries();
  nsresult CreateDefaultQuery(sbIDatabaseQuery **aQuery);

  nsresult CreateAnnotationsFromEntryId(PRInt64 aEntryId, 
                                        sbIPropertyArray **aAnnotations);

  nsresult CreateEntryFromResultSet(sbIDatabaseResult *aResult,
                                    PRUint32 aRow,
                                    sbIPlaybackHistoryEntry **aEntry);

  nsresult CreateEntriesFromResultSet(sbIDatabaseResult *aResult,
                                      nsIArray **aEntries);

  nsresult EnsureHistoryDatabaseAvailable();

  nsresult FillAddQueryParameters(sbIDatabaseQuery *aQuery,
                                  sbIPlaybackHistoryEntry *aEntry);
  nsresult FillAddAnnotationsQueryParameters(sbIDatabaseQuery *aQuery,
                                             sbIPlaybackHistoryEntry *aEntry);
  nsresult FillRemoveEntryQueryParameters(sbIDatabaseQuery *aQuery,
                                          sbIPlaybackHistoryEntry *aEntry);

  nsresult GetItem(const nsAString &aLibraryGuid,
                   const nsAString &aItemGuid,
                   sbIMediaItem **aItem);

  nsresult GetPropertyDBID(const nsAString &aPropertyID,
                           PRUint32 *aPropertyDBID);
  nsresult InsertPropertyID(const nsAString &aPropertyID, 
                            PRUint32 *aPropertyDBID);
  nsresult LoadPropertyIDs();

  nsresult DoEntryAddedCallback(sbIPlaybackHistoryEntry *aEntry);
  nsresult DoEntriesAddedCallback(nsIArray *aEntries);

  nsresult DoEntryUpdatedCallback(sbIPlaybackHistoryEntry *aEntry);
  nsresult DoEntriesUpdatedCallback(nsIArray *aEntry);
  
  nsresult DoEntryRemovedCallback(sbIPlaybackHistoryEntry *aEntry);
  nsresult DoEntriesRemovedCallback(nsIArray *aEntry);
  
  nsresult DoEntriesClearedCallback();

  // playcount, last played time management.
  nsresult UpdateTrackingDataFromEvent(sbIMediacoreEvent *aEvent);
  nsresult UpdateCurrentViewFromEvent(sbIMediacoreEvent *aEvent);
  nsresult VerifyDataAndCreateNewEntry();
  nsresult ResetTrackingData();

#ifdef METRICS_ENABLED
  // metrics
  nsresult UpdateMetrics();
#endif

protected:
  ~sbPlaybackHistoryService();

private:
  nsString mAddEntryQuery;
  
  nsString mAddAnnotationQuery;
  nsString mInsertAnnotationQuery;
  nsString mUpdateAnnotationQuery;
  nsString mRemoveAnnotationQuery;
  nsString mIsAnnotationPresentQuery;

  nsString mGetEntryCountQuery;

  nsString mGetEntryIDQuery;
  
  nsString mInsertPropertyIDQuery;

  nsString mGetAllEntriesQuery;
  nsString mGetEntriesByIndexQuery;
  nsString mGetEntriesByIndexQueryAscending;

  nsString mGetEntriesByTimestampQuery;
  nsString mGetEntriesByTimestampQueryAscending;

  nsString mGetAnnotationsForEntryQuery;

  nsString mRemoveEntriesQuery;
  nsString mRemoveAnnotationsQuery;

  nsString mRemoveAllEntriesQuery;
  nsString mRemoveAllAnnotationsQuery;

  nsInterfaceHashtableMT<nsStringHashKey, sbILibrary> mLibraries;

  nsInterfaceHashtableMT<nsISupportsHashKey, 
                         sbIPlaybackHistoryListener> mListeners;

  nsDataHashtableMT<nsUint32HashKey, nsString> mPropertyDBIDToID;
  nsDataHashtableMT<nsStringHashKey, PRUint32> mPropertyIDToDBID;

  nsCOMPtr<nsIWeakReference> mMediacoreManager;

  PRMonitor*   mMonitor;

  PRPackedBool mCurrentlyTracking;

  PRTime mCurrentStartTime;
  PRTime mCurrentPauseTime;
  PRTime mCurrentDelta;

  nsCOMPtr<sbIMediaItem> mCurrentItem;
  nsCOMPtr<sbIMediaListView> mCurrentView;

#ifdef METRICS_ENABLED
  nsCOMPtr<sbIMetrics> mMetrics;
#endif
};

#endif /* __SB_PLAYBACKHISTORYSERVICE_H__ */
