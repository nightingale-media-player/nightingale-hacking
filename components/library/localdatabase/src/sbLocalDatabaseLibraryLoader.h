/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

#ifndef __SB_LOCALDATABASELIBRARYLOADER_H__
#define __SB_LOCALDATABASELIBRARYLOADER_H__

#include <nsIObserver.h>

#include <nsClassHashtable.h>
#include <nsHashKeys.h>
#include <nsStringGlue.h>

class nsIComponentManager;
class nsIFile;
class nsIPrefBranch;
class nsIPrefService;
class sbILibraryManager;
class sbLibraryInfo;
class sbLocalDatabaseLibraryFactory;

struct nsModuleComponentInfo;

class sbLocalDatabaseLibraryLoader : public nsIObserver
{
  struct sbLoaderInfo
  {
    sbLoaderInfo(sbILibraryManager* aLibraryManager,
                 sbLocalDatabaseLibraryFactory* aLibraryFactory)
    : libraryManager(aLibraryManager),
      libraryFactory(aLibraryFactory),
      registeredLibraryCount(0)
    { }

    sbILibraryManager*             libraryManager;
    sbLocalDatabaseLibraryFactory* libraryFactory;
    PRUint32                       registeredLibraryCount;
  };

  struct sbLibraryExistsInfo
  {
    sbLibraryExistsInfo(const nsAString& aDatabaseGUID)
    : databaseGUID(aDatabaseGUID),
      index(-1)
    { }

    nsString databaseGUID;
    PRInt32  index;
  };

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  sbLocalDatabaseLibraryLoader();

  static NS_METHOD RegisterSelf(nsIComponentManager* aCompMgr,
                                nsIFile* aPath,
                                const char* aLoaderStr,
                                const char* aType,
                                const nsModuleComponentInfo *aInfo);

  nsresult Init();

private:
  ~sbLocalDatabaseLibraryLoader();

  nsresult RealInit();

  nsresult LoadLibraries();

  nsresult EnsureDefaultLibraries();

  nsresult EnsureDefaultLibrary(const nsACString& aLibraryGUIDPref,
                                const nsAString& aDefaultGUID);

  sbLibraryInfo* CreateDefaultLibraryInfo(const nsACString& aPrefKey,
                                          const nsAString& aGUID);

  static void RemovePrefBranch(const nsACString& aPrefBranch);

  static PLDHashOperator PR_CALLBACK
    LoadLibrariesCallback(nsUint32HashKey::KeyType aKey,
                          sbLibraryInfo* aEntry,
                          void* aUserData);

  static PLDHashOperator PR_CALLBACK
    LibraryExistsCallback(nsUint32HashKey::KeyType aKey,
                          sbLibraryInfo* aEntry,
                          void* aUserData);

  static PLDHashOperator PR_CALLBACK
    VerifyEntriesCallback(nsUint32HashKey::KeyType aKey,
                          nsAutoPtr<sbLibraryInfo>& aEntry,
                          void* aUserData);

private:
  nsClassHashtable<nsUint32HashKey, sbLibraryInfo> mLibraryInfoTable;
  PRUint32 mNextLibraryIndex;
  nsCOMPtr<nsIPrefBranch> mRootBranch;
};

#endif /* __SB_LOCALDATABASELIBRARYLOADER_H__ */
