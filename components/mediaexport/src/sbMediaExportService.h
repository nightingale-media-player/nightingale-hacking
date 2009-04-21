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

#include <sbILibrary.h>
#include <nsIObserver.h>
#include <sbIMediaListListener.h>
#include <nsIComponentManager.h>
#include <nsIGenericFactory.h>
#include <nsIFile.h>
#include <nsCOMPtr.h>
#include <nsAutoPtr.h>
#include <nsStringAPI.h>
#include <set>
#include "sbMediaExportPrefController.h"

typedef std::set<nsString>    sbStringSet;
typedef sbStringSet::iterator sbStringSetIter;


class sbMediaExportService : public nsIObserver,
                               public sbIMediaListListener
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

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_SBIMEDIALISTLISTENER

protected:
  nsresult InitInternal();
  nsresult Shutdown();

private:
  nsRefPtr<sbMediaExportPrefController> mPrefController;
  nsCOMPtr<sbILibrary>                  mMainLibrary;

  PRBool mIsRunning;

  sbStringSet      mAddedItemGuids;
  sbStringSet      mRemovedItemGuids;
};

#endif  // sbMediaExportService_h_

