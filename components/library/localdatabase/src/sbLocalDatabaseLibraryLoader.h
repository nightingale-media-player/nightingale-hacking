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

#include <sbILibraryLoader.h>

#include <nsClassHashtable.h>
#include <nsHashKeys.h>
#include <nsIPrefBranch.h>
#include <nsStringGlue.h>

class nsIComponentManager;
class nsIFile;
class nsILocalFile;
class nsIPrefService;
class sbILibraryManager;
class sbLibraryLoaderInfo;
class sbLocalDatabaseLibraryFactory;

struct nsModuleComponentInfo;

class sbLocalDatabaseLibraryLoader : public sbILibraryLoader
{
  struct sbLoaderInfo
  {
    sbLoaderInfo(sbILibraryManager* aLibraryManager,
                 sbLocalDatabaseLibraryFactory* aLibraryFactory)
    : libraryManager(aLibraryManager),
      libraryFactory(aLibraryFactory)
    { }

    sbILibraryManager*             libraryManager;
    sbLocalDatabaseLibraryFactory* libraryFactory;
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
  NS_DECL_SBILIBRARYLOADER

  sbLocalDatabaseLibraryLoader();

  nsresult Init();

private:
  ~sbLocalDatabaseLibraryLoader();

  nsresult LoadLibraries();

  nsresult EnsureDefaultLibraries();

  nsresult EnsureDefaultLibrary(const nsACString& aLibraryGUIDPref,
                                const nsAString& aDefaultGUID);

  sbLibraryLoaderInfo* CreateDefaultLibraryInfo(const nsACString& aPrefKey,
                                                const nsAString& aGUID);

  PRUint32 GetNextLibraryIndex();

  static void RemovePrefBranch(const nsACString& aPrefBranch);

  static PLDHashOperator PR_CALLBACK
    LoadLibrariesCallback(nsUint32HashKey::KeyType aKey,
                          sbLibraryLoaderInfo* aEntry,
                          void* aUserData);

  static PLDHashOperator PR_CALLBACK
    LibraryExistsCallback(nsUint32HashKey::KeyType aKey,
                          sbLibraryLoaderInfo* aEntry,
                          void* aUserData);

  static PLDHashOperator PR_CALLBACK
    VerifyEntriesCallback(nsUint32HashKey::KeyType aKey,
                          nsAutoPtr<sbLibraryLoaderInfo>& aEntry,
                          void* aUserData);

private:
  nsClassHashtable<nsUint32HashKey, sbLibraryLoaderInfo> mLibraryInfoTable;
  nsCOMPtr<nsIPrefBranch> mRootBranch;
};

class sbLibraryLoaderInfo
{
public:
  sbLibraryLoaderInfo() { }

  nsresult Init(const nsACString& aPrefKey);

  nsresult SetDatabaseGUID(const nsAString& aGUID);
  void GetDatabaseGUID(nsAString& _retval);

  nsresult SetDatabaseLocation(nsILocalFile* aLocation);
  already_AddRefed<nsILocalFile> GetDatabaseLocation();

  nsresult SetLoadAtStartup(PRBool aLoadAtStartup);
  PRBool GetLoadAtStartup();

  void GetPrefBranch(nsACString& _retval);

private:
  nsCOMPtr<nsIPrefBranch> mPrefBranch;
  nsCString mGUIDKey;
  nsCString mLocationKey;
  nsCString mStartupKey;
};

#endif /* __SB_LOCALDATABASELIBRARYLOADER_H__ */
