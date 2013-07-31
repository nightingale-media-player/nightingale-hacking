/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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

/** 
 * \file  sbLibraryManager.h
 * \brief Songbird Library Manager Definition.
 */

#ifndef __SB_LIBRARYMANAGER_H__
#define __SB_LIBRARYMANAGER_H__

#include <nsIObserver.h>
#include <nsIThreadManager.h>
#include <sbILibrary.h>
#include <sbILibraryManager.h>
#include <sbILibraryUtils.h>

#include <nsCategoryCache.h>
#include <mozilla/Mutex.h>
#include <nsCOMPtr.h>
#include <nsHashKeys.h>
#include <nsClassHashtable.h>
#include <nsInterfaceHashtable.h>
#include <prlock.h>
#include <sbILibraryLoader.h>

#include <sbWeakReference.h>

#define SB_PREFBRANCH_LIBRARY    "songbird.library."

#define SB_PREF_MAIN_LIBRARY      SB_PREFBRANCH_LIBRARY "main"
#define SB_PREF_WEB_LIBRARY       SB_PREFBRANCH_LIBRARY "web"
#define SB_PREF_PLAYQUEUE_LIBRARY SB_PREFBRANCH_LIBRARY "playqueue"

class nsIComponentManager;
class nsIFile;
class nsIRDFDataSource;
class sbILibraryFactory;
class sbILibraryLoader;
class sbILibraryManagerListener;

struct nsModuleComponentInfo;

class sbLibraryManager : public sbILibraryManager,
                         public sbILibraryUtils,
                         public nsIObserver,
                         public sbSupportsWeakReference
{
  struct sbLibraryInfo {
    sbLibraryInfo(PRBool aLoadAtStartup = PR_FALSE)
    : loader(nsnull),
       loadAtStartup(aLoadAtStartup)
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
  NS_DECL_SBILIBRARYUTILS

  sbLibraryManager();

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
    AddListenersToCOMArrayCallback(nsISupportsHashKey::KeyType aKey,
                                   sbILibraryManagerListener* aEntry,
                                   void* aUserData);

  static PLDHashOperator PR_CALLBACK
    AssertAllLibrariesCallback(nsStringHashKey::KeyType aKey,
                               sbLibraryInfo* aEntry,
                               void* aUserData);

  static nsresult AssertLibrary(nsIRDFDataSource* aDataSource,
                                sbILibrary* aLibrary);

  static nsresult UnassertLibrary(nsIRDFDataSource* aDataSource,
                                  sbILibrary* aLibrary);

  nsresult GenerateDataSource();

  void NotifyListenersLibraryRegistered(sbILibrary* aLibrary);

  void NotifyListenersLibraryUnregistered(sbILibrary* aLibrary);

  void InvokeLoaders();

  nsresult SetLibraryLoadsAtStartupInternal(sbILibrary* aLibrary,
                                            PRBool aLoadAtStartup,
                                            sbLibraryInfo** aInfo);

private:
  /**
   * \brief A hashtable that holds all the registered libraries.
   */
  nsClassHashtable<nsStringHashKey, sbLibraryInfo> mLibraryTable;

  /**
   * \brief An in-memory datasource that contains information about the
   *        registered libraries.
   */
  nsCOMPtr<nsIRDFDataSource> mDataSource;

  /**
   * \brief A list of listeners.
   */
  nsInterfaceHashtable<nsISupportsHashKey, sbILibraryManagerListener> mListeners;

  /**
   * \brief A list of library loaders registered through the Category Manager.
   */
  nsCategoryCache<sbILibraryLoader> mLoaderCache;

  nsCOMPtr<sbILibraryLoader> mCurrentLoader;

  mozilla::Mutex mLock;

  nsCOMPtr<nsIThreadManager> mThreadManager;
};

#endif /* __SB_LIBRARYMANAGER_H__ */
