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
#include <nsStringAPI.h>
#include <nsIClassInfo.h>
#include <list>
#include <map>
#include "sbMediaExportPrefController.h"

typedef std::list<nsString>                 sbStringList;
typedef sbStringList::iterator              sbStringListIter;
typedef std::map<nsString, sbStringList>    sbMediaListItemMap;
typedef sbMediaListItemMap::value_type      sbMediaListItemMapPair;
typedef sbMediaListItemMap::iterator        sbMediaListItemMapIter;


class sbMediaExportService : public nsIClassInfo,
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
  NS_DECL_NSICLASSINFO
  NS_DECL_NSIOBSERVER
  NS_DECL_SBIMEDIALISTLISTENER
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER
  NS_DECL_SBIJOBPROGRESS
  NS_DECL_SBISHUTDOWNJOB

protected:
  nsresult InitInternal();
  nsresult Shutdown();
  nsresult StopListening();
  nsresult ListenToMediaList(sbIMediaList *aMediaList);
  nsresult GetShouldWatchMediaList(sbIMediaList *aMediaList, 
                                   PRBool *aShouldWatch);

  nsresult BeginExportData();

private:
  nsRefPtr<sbMediaExportPrefController>  mPrefController;
  nsCOMPtr<sbIMutablePropertyArray>      mFilteredProperties;
  nsCOMArray<sbIMediaList>               mObservedMediaLists;
  sbMediaListItemMap                     mAddedItemsMap;
  sbStringList                           mAddedMediaList;
  sbStringList                           mRemovedMediaLists;
  PRBool                                 mIsRunning;

  // sbIJobProgress / sbIShutdownJob stuff:
  nsCOMArray<sbIJobProgressListener> mJobListeners;
};

#endif  // sbMediaExportService_h_

