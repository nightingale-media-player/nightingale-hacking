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

#include "sbLocalDatabaseLibraryFactory.h"
#include "sbLocalDatabaseLibrary.h"

#include <sbILibrary.h>
#include <nsXPCOMCID.h>
#include <nsIProperties.h>
#include <nsILocalFile.h>
#include <nsServiceManagerUtils.h>

NS_IMPL_ISUPPORTS1(sbLocalDatabaseLibraryFactory,
                   sbILocalDatabaseLibraryFactory)

#define DEFAULT_LIBRARY_NAME NS_LITERAL_STRING("defaultlibrary.db")

sbLocalDatabaseLibraryFactory::sbLocalDatabaseLibraryFactory()
{
}

sbLocalDatabaseLibraryFactory::~sbLocalDatabaseLibraryFactory()
{
}

NS_IMETHODIMP
sbLocalDatabaseLibraryFactory::GetNameKey(nsAString& aNameKey)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseLibraryFactory::CreateLibrary(sbILibrary** _retval)
{
  nsresult rv;

  nsCOMPtr<nsIProperties> ds =
    do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILocalFile> file;
  rv = ds->Get("ProfD", NS_GET_IID(nsILocalFile), getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = file->AppendRelativePath(NS_LITERAL_STRING("db"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = file->AppendRelativePath(DEFAULT_LIBRARY_NAME);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibrary> library;
  rv = CreateLibraryFromDatabase(file, getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = library);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseLibraryFactory::CreateLibraryFromDatabase(nsIFile* aDatabase,
                                                         sbILibrary** _retval)
{
  nsresult rv;

  PRBool exists;
  rv = aDatabase->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!exists) {
    // TODO: Create the new database
    // Creaat db, run the schema and static data
    // Note: a database needs to get its own guid so it can also be a resource
  }

  nsAutoString fileName;
  rv = aDatabase->GetLeafName(fileName);
  NS_ENSURE_SUCCESS(rv, rv);

  /*
   * XXX: Trim off the last three characters to get the guid.  This will
   * probably change when we can ask the dbengine to use a path to a db file,
   * not just a guid
   */
  const nsAString& guid = Substring(fileName, 0, fileName.Length() - 3);

  sbLocalDatabaseLibrary* library = new sbLocalDatabaseLibrary(guid);
  NS_ENSURE_TRUE(library, NS_ERROR_OUT_OF_MEMORY);

  rv = library->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = library);
  return NS_OK;
}

