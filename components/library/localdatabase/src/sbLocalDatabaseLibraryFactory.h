/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

#ifndef __SBLOCALDATABASELIBRARYFACTORY_H__
#define __SBLOCALDATABASELIBRARYFACTORY_H__

#include <sbIDatabaseQuery.h>
#include <sbILibraryFactory.h>

#include <nsCOMPtr.h>
#include <nsHashKeys.h>
#include <nsInterfaceHashtable.h>
#include <nsIGenericFactory.h>
#include <nsStringGlue.h>

#define SB_LOCALDATABASE_LIBRARYFACTORY_TYPE               \
  SB_LOCALDATABASE_LIBRARYFACTORY_DESCRIPTION

class nsIFile;
class nsILocalFile;
class nsIPropertyBag2;
class nsIWeakReference;
class sbILibrary;

class sbLocalDatabaseLibraryFactory : public sbILibraryFactory
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILIBRARYFACTORY

  static NS_METHOD RegisterSelf(nsIComponentManager* aCompMgr,
                                nsIFile* aPath,
                                const char* aLoaderStr,
                                const char* aType,
                                const nsModuleComponentInfo *aInfo);

  already_AddRefed<nsILocalFile> GetFileForGUID(const nsAString& aGUID);
  void GetGUIDFromFile(nsILocalFile* aFile,
                       nsAString& aGUID);

  nsresult CreateLibraryFromDatabase(nsIFile* aDatabase,
                                     sbILibrary** _retval,
                                     nsIPropertyBag2* aCreationParameters = nsnull,
                                     nsString aResourceGUID = EmptyString());

  nsresult Init();

private:
  nsresult InitalizeLibrary(nsIFile* aDatabaseFile, const nsAString &aResourceGUID);

  nsresult UpdateLibrary(nsIFile* aDatabaseFile);

  nsresult SetQueryDatabaseFile(sbIDatabaseQuery* aQuery,
                                nsIFile*          aDatabaseFile);

private:
  nsInterfaceHashtable<nsHashableHashKey, nsIWeakReference> mCreatedLibraries;
};

#endif /* __SBLOCALDATABASELIBRARYFACTORY_H__ */
