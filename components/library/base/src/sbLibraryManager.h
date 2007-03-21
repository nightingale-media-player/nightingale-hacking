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

#include <sbILibraryManager.h>

#include <nsCOMPtr.h>
#include <nsHashKeys.h>
#include <nsIGenericFactory.h>
#include <nsInterfaceHashtable.h>
#include <nsIObserver.h>

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

class nsIRDFDataSource;
class sbILibrary;
class sbILibraryFactory;

class sbLibraryManager : public sbILibraryManager,
                         public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_SBILIBRARYMANAGER

  sbLibraryManager();

  /**
   * See sbLibraryManager.cpp
   */
  NS_METHOD Init();

private:
  ~sbLibraryManager();

  /**
   * See sbLibraryManager.cpp
   */
  static PLDHashOperator PR_CALLBACK
    AddLibrariesToCOMArrayCallback(nsStringHashKey::KeyType aKey,
                                   sbILibrary* aEntry,
                                   void* aUserData);

  /**
   * See sbLibraryManager.cpp
   */
  static PLDHashOperator PR_CALLBACK
    AddFactoriesToCOMArrayCallback(nsStringHashKey::KeyType aKey,
                                   sbILibraryFactory* aEntry,
                                   void* aUserData);

  /**
   * See sbLibraryManager.cpp
   */
  static PLDHashOperator PR_CALLBACK
    AssertAllLibrariesCallback(nsStringHashKey::KeyType aKey,
                               sbILibrary* aEntry,
                               void* aUserData);

  /**
   * See sbLibraryManager.cpp
   */
  static PLDHashOperator PR_CALLBACK
    ShutdownAllLibrariesCallback(nsStringHashKey::KeyType aKey,
                                 sbILibrary* aEntry,
                                 void* aUserData);

  /**
   * See sbLibraryManager.cpp
   */
  static NS_METHOD AssertLibrary(nsIRDFDataSource* aDataSource,
                                 sbILibrary* aLibrary);

  /**
   * See sbLibraryManager.cpp
   */
  static NS_METHOD UnassertLibrary(nsIRDFDataSource* aDataSource,
                                   sbILibrary* aLibrary);

  /**
   * See sbLibraryManager.cpp
   */
  NS_METHOD GenerateDataSource();

private:
  /**
   * \brief A hashtable that holds all the registered libraries.
   */
  nsInterfaceHashtable<nsStringHashKey, sbILibrary> mLibraryTable;

  /**
   * \brief A hashtable that holds all the registered library factories.
   */
  nsInterfaceHashtable<nsStringHashKey, sbILibraryFactory> mFactoryTable;

  /**
   * \brief An in-memory datasource that contains information about the
   *        registered libraries.
   */
  nsCOMPtr<nsIRDFDataSource> mDataSource;
};

#endif /* __SB_LIBRARYMANAGER_H__ */
