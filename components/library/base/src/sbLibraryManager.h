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

/** 
 * \file  sbLibraryManager.h
 * \brief Songbird Library Manager Definition.
 */

#ifndef __SB_LIBRARYMANAGER_H__
#define __SB_LIBRARYMANAGER_H__

#include <nsIObserver.h>
#include <nsWeakReference.h>
#include <sbILibrary.h>
#include <sbILibraryManager.h>

#include <nsCategoryCache.h>
#include <nsAutoLock.h>
#include <nsCOMPtr.h>
#include <nsHashKeys.h>
#include <nsClassHashtable.h>
#include <nsTHashtable.h>
#include <prlock.h>
#include <sbILibraryLoader.h>

#define SONGBIRD_LIBRARYMANAGER_DESCRIPTION                \
  "Songbird Library Manager"
#define SONGBIRD_LIBRARYMANAGER_CONTRACTID                 \
  "@songbirdnest.com/Songbird/library/Manager;1"
#define SONGBIRD_LIBRARYMANAGER_CLASSNAME                  \
  "Songbird Library Manager"
#define SONGBIRD_LIBRARYMANAGER_CID                        \
{ /* ff27fd1d-183d-4c6d-89e7-1cd489f18bb9 */               \
  0xff27fd1d,                                              \
  0x183d,                                                  \
  0x4c6d,                                                  \
  { 0x89, 0xe7, 0x1c, 0xd4, 0x89, 0xf1, 0x8b, 0xb9 }       \
}

#define SB_PREFBRANCH_LIBRARY    "songbird.library."

#define SB_PREF_MAIN_LIBRARY     SB_PREFBRANCH_LIBRARY "main"
#define SB_PREF_WEB_LIBRARY      SB_PREFBRANCH_LIBRARY "web"
#define SB_PREF_DOWNLOAD_LIBRARY SB_PREFBRANCH_LIBRARY "download"

class nsIComponentManager;
class nsIFile;
class nsIRDFDataSource;
class sbILibraryFactory;
class sbILibraryLoader;

struct nsModuleComponentInfo;

class sbLibraryManager : public sbILibraryManager,
                         public nsIObserver,
                         public nsSupportsWeakReference
{
  struct sbLibraryInfo {
    sbLibraryInfo(PRBool aLoadAtStartup = PR_FALSE)
    : loadAtStartup(aLoadAtStartup),
      loader(nsnull)
    { }

    nsCOMPtr<sbILibrary> library;

    // Don't need an owning ref here because we already have one in
    // mLoaderCache.
    sbILibraryLoader* loader;

    PRBool loadAtStartup;
  };

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_SBILIBRARYMANAGER

  sbLibraryManager();

  static NS_METHOD RegisterSelf(nsIComponentManager* aCompMgr,
                                nsIFile* aPath,
                                const char* aLoaderStr,
                                const char* aType,
                                const nsModuleComponentInfo *aInfo);

  nsresult Init();

private:
  ~sbLibraryManager();

  static PLDHashOperator PR_CALLBACK
    AddLibrariesToCOMArrayCallback(nsStringHashKey::KeyType aKey,
                                   sbLibraryInfo* aEntry,
                                   void* aUserData);

  static PLDHashOperator PR_CALLBACK
    AddStartupLibrariesToCOMArrayCallback(nsStringHashKey::KeyType aKey,
                                          sbLibraryInfo* aEntry,
                                          void* aUserData);

  static PLDHashOperator PR_CALLBACK
    AssertAllLibrariesCallback(nsStringHashKey::KeyType aKey,
                               sbLibraryInfo* aEntry,
                               void* aUserData);

  static PLDHashOperator PR_CALLBACK
    ShutdownAllLibrariesCallback(nsStringHashKey::KeyType aKey,
                                 sbLibraryInfo* aEntry,
                                 void* aUserData);

  static PLDHashOperator PR_CALLBACK
    NotifyListenersLibraryRegisteredCallback(nsISupportsHashKey* aKey,
                                             void* aUserData);

  static PLDHashOperator PR_CALLBACK
    NotifyListenersLibraryUnregisteredCallback(nsISupportsHashKey* aKey,
                                               void* aUserData);

  static nsresult AssertLibrary(nsIRDFDataSource* aDataSource,
                                sbILibrary* aLibrary);

  static nsresult UnassertLibrary(nsIRDFDataSource* aDataSource,
                                  sbILibrary* aLibrary);

  nsresult GenerateDataSource();

  void NotifyListenersLibraryRegistered(sbILibrary* aLibrary);

  void NotifyListenersLibraryUnregistered(sbILibrary* aLibrary);

  void InvokeLoaders();

  sbILibraryLoader* FindLoaderForLibrary(sbILibrary* aLibrary);

private:
  /**
   * \brief A hashtable that holds all the registered libraries.
   */
  nsClassHashtableMT<nsStringHashKey, sbLibraryInfo> mLibraryTable;

  /**
   * \brief An in-memory datasource that contains information about the
   *        registered libraries.
   */
  nsCOMPtr<nsIRDFDataSource> mDataSource;

  /**
   * \brief A list of listeners.
   */
  nsTHashtable<nsISupportsHashKey> mListeners;

  /**
   * \brief A list of library loaders registered through the Category Manager.
   */
  nsCategoryCache<sbILibraryLoader> mLoaderCache;

  nsCOMPtr<sbILibraryLoader> mCurrentLoader;

  PRLock* mLock;
};

#endif /* __SB_LIBRARYMANAGER_H__ */
