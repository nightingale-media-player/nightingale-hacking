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

#ifndef sbMediaExportService_h_
#define sbMediaExportService_h_

#include "sbMediaExportDefines.h"
#include "sbMediaExportTaskWriter.h"
#include <sbIShutdownJob.h>
#include <sbIJobProgress.h>
#include <sbIPropertyArray.h>
#include <nsIObserver.h>
#include <sbIMediaListListener.h>
#include <nsIComponentManager.h>
#include <nsIGenericFactory.h>
#include <nsIFile.h>
#include <nsIArray.h>
#include <nsCOMPtr.h>
#include <nsAutoPtr.h>
#include <nsCOMArray.h>
#include <nsIClassInfo.h>
#include <sbIMediaExportService.h>
#include <map>
#include "sbMediaExportPrefController.h"

typedef std::map<nsString, sbStringList>    sbMediaListItemMap;
typedef sbMediaListItemMap::value_type      sbMediaListItemMapPair;
typedef sbMediaListItemMap::iterator        sbMediaListItemMapIter;

//
// \brief Enum for keeping track of what specific type of data that the 
//        enumerated property lookup is currently running for.
//
typedef enum {
  eNone                = 0,
  eAllMediaLists       = 1,
  eAddedMediaLists     = 2,
  eRemovedMediaLists   = 3,
  eAddedMediaItems    = 4,
  eMediaListAddedItems = 5,
} EEnumerationLookupState;


class sbMediaExportService : public sbIMediaExportService,
                             public nsIClassInfo,
                             public nsIObserver,
                             public sbIMediaListListener,
                             public sbIMediaListEnumerationListener,
                             public sbIShutdownJob,
                             public sbMediaExportPrefListener
{
public:
  sbMediaExportService();
  virtual ~sbMediaExportService();

  nsresult Init();

  static NS_METHOD RegisterSelf(nsIComponentManager* aCompMgr,
                                nsIFile* aPath,
                                const char* aLoaderStr,
                                const char* aType,
                                const nsModuleComponentInfo *aInfo);

  // sbMediaExportPrefListener
  NS_IMETHOD OnBoolPrefChanged(const nsAString & aPrefName,
                               const PRBool aNewPrefValue);

  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIAEXPORTSERVICE
  NS_DECL_NSICLASSINFO
  NS_DECL_NSIOBSERVER
  NS_DECL_SBIMEDIALISTLISTENER
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER
  NS_DECL_SBIJOBPROGRESS
  NS_DECL_SBISHUTDOWNJOB

protected:
  // Internal service startup handling
  nsresult InitInternal();

  // Internal service shutdown handling
  nsresult Shutdown();

  // Stop listening to all observed media lists and clear changes. 
  nsresult StopListeningMediaLists();

  // Media list listening utility methods
  nsresult ListenToMediaList(sbIMediaList *aMediaList);
  nsresult GetShouldWatchMediaList(sbIMediaList *aMediaList, 
                                   PRBool *aShouldWatch);

  // Background thread method to Write export data to disk.
  void WriteExportData();

  // Background thread method to notify the listeners on the main thread.
  void ProxyNotifyListeners();

  // Start/Stop entry points for the entire export process
  nsresult BeginExportData();
  nsresult FinishExportData();

  // Start/stop entry points for each specific data export types.
  nsresult DetermineNextExportState();
  nsresult StartExportState();
  nsresult FinishExportState();

  // Lookup mediaitems by a guid list.
  nsresult GetMediaListByGuid(const nsAString & aItemGuid,
                              sbIMediaList **aMediaList);
  nsresult EnumerateItemsByGuids(sbStringList & aGuidStringList,
                                 sbIMediaList *aMediaList);

  // Notify job progress listeners.
  nsresult NotifyListeners();

private:
  // Core and changed item stuff:
  nsRefPtr<sbMediaExportPrefController>  mPrefController;
  nsCOMPtr<sbIMutablePropertyArray>      mFilteredProperties;
  nsCOMArray<sbIMediaList>               mObservedMediaLists;
  sbMediaListItemMap                     mAddedItemsMap;
  sbStringList                           mAddedMediaList;
  sbStringList                           mRemovedMediaLists;
  PRBool                                 mIsRunning;
  EEnumerationLookupState                mEnumState;

  // Exporting stuff:
  nsRefPtr<sbMediaExportTaskWriter>  mTaskWriter;
  EEnumerationLookupState            mExportState;
  sbMediaListItemMapIter             mCurExportListIter;
  nsCOMPtr<sbIMediaList>             mCurExportMediaList;
  PRBool                             mFinishedExportState;
  
  // sbIJobProgress / sbIShutdownJob stuff:
  nsCOMArray<sbIJobProgressListener> mJobListeners;
  PRUint32                           mTotal;
  PRUint32                           mProgress;
  PRUint16                           mStatus;
};

#endif  // sbMediaExportService_h_

