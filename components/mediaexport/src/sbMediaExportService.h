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
#include <sbILocalDatabaseSmartMediaList.h>
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

typedef std::map<nsString, nsString>        sbSmartMediaListMap;
typedef sbSmartMediaListMap::value_type     sbSmartMediaListMapPair;
typedef sbSmartMediaListMap::iterator       sbSmartMediaListMapIter;


class sbMediaExportService : public sbIMediaExportService,
                             public nsIClassInfo,
                             public nsIObserver,
                             public sbIMediaListListener,
                             public sbILocalDatabaseSmartMediaListListener,
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
  NS_DECL_SBILOCALDATABASESMARTMEDIALISTLISTENER
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

  // Methods for writing the task file
  nsresult WriteChangesToTaskFile();
  nsresult WriteAddedMediaLists();
  nsresult WriteRemovedMediaLists();
  nsresult WriteAddedMediaItems();
  nsresult WriteUpdatedMediaItems();
  nsresult WriteUpdatedSmartPlaylists();
  nsresult WriteMediaItemsArray(nsIArray *aItemsArray);

  // Lookup mediaitems by a guid list.
  nsresult GetMediaListByGuid(const nsAString & aItemGuid,
                              sbIMediaList **aMediaList);

  // Notify job progress listeners.
  nsresult NotifyListeners();

  // Returns true if there is some observed changes that have not been
  // exported yet.
  PRBool GetHasRecordedChanges();

private:
  // Core and changed item stuff:
  nsRefPtr<sbMediaExportPrefController>  mPrefController;
  nsCOMPtr<sbIMutablePropertyArray>      mFilteredProperties;
  nsCOMArray<sbIMediaList>               mObservedMediaLists;
  nsCOMArray<sbILocalDatabaseSmartMediaList> mObservedSmartMediaLists;

  // a map of <media list guid> to array of <media item>s added to that list
  sbMediaListItemMap                     mAddedItemsMap;
  // a set of <media item guid>s of items changed in the library
  sbStringSet                            mUpdatedItems;
  // a list of <media list guid>s that were added to the library
  sbStringList                           mAddedMediaList;
  // a list of <media list guid>s that were removed from the library
  sbStringList                           mRemovedMediaLists;
  // a list of <media list guid>s of updated <smart media list>s
  sbStringList                           mUpdatedSmartMediaLists;
  PRBool                                 mIsRunning;

  // Exporting stuff:
  nsRefPtr<sbMediaExportTaskWriter>  mTaskWriter;
  
  // sbIJobProgress / sbIShutdownJob stuff:
  nsCOMArray<sbIJobProgressListener> mJobListeners;
  PRUint32                           mTotal;
  PRUint32                           mProgress;
  PRUint16                           mStatus;
};

#endif  // sbMediaExportService_h_

