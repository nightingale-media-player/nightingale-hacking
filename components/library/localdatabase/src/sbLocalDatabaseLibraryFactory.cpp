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
#include <nsIXMLHttpRequest.h>
#include <nsComponentManagerUtils.h>
#include <sbISQLBuilder.h>
#include <sbSQLBuilderCID.h>
#include <sbIDatabaseQuery.h>
#include <DatabaseQuery.h>
#include <sbIDatabaseResult.h>
#include <nsIUUIDGenerator.h>

#define SCHEMA_URL "chrome://songbird/content/library/localdatabase/schema.sql"
#define DEFAULT_LIBRARY_NAME NS_LITERAL_STRING("defaultlibrary.db")

NS_IMPL_ISUPPORTS1(sbLocalDatabaseLibraryFactory,
                   sbILocalDatabaseLibraryFactory)

NS_IMETHODIMP
sbLocalDatabaseLibraryFactory::GetNameKey(nsAString& aNameKey)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseLibraryFactory::CreateLibrary(sbILibrary** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

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
  NS_ENSURE_ARG_POINTER(aDatabase);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  nsAutoString fileName;
  rv = aDatabase->GetLeafName(fileName);
  NS_ENSURE_SUCCESS(rv, rv);

  /*
   * XXX: Trim off the last three characters to get the guid.  This will
   * probably change when we can ask the dbengine to use a path to a db file,
   * not just a guid
   */
  const nsAString& guid = Substring(fileName, 0, fileName.Length() - 3);

  PRBool exists;
  rv = aDatabase->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  // If the database file does not exist, create and initalize it
  if (!exists) {
    rv = InitalizeLibrary(guid);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  sbLocalDatabaseLibrary* library = new sbLocalDatabaseLibrary(guid);
  NS_ENSURE_TRUE(library, NS_ERROR_OUT_OF_MEMORY);

  rv = library->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = library);
  return NS_OK;
}

nsresult
sbLocalDatabaseLibraryFactory::InitalizeLibrary(const nsAString& aDatabaseGuid)
{
  nsresult rv;
  PRInt32 dbOk;

  // Load the schema file, and add each colon-newline delimited line into
  // the query object
  nsCOMPtr<nsIXMLHttpRequest> request = 
    do_CreateInstance(NS_XMLHTTPREQUEST_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = request->OpenRequest(NS_LITERAL_CSTRING("GET"),
                            NS_LITERAL_CSTRING(SCHEMA_URL),
                            PR_FALSE, EmptyString(), EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = request->Send(nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString response;
  rv = request->GetResponseText(response);
  NS_ENSURE_SUCCESS(rv, rv);

  // Run the schema
  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance(SONGBIRD_DATABASEQUERY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetDatabaseGUID(aDatabaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetAsyncQuery(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING(colonNewline, ";\n");
  PRInt32 posStart = 0;
  PRInt32 posEnd = response.Find(colonNewline, posStart);
  while (posEnd >= 0) {
    rv = query->AddQuery(Substring(response, posStart, posEnd - posStart));
    NS_ENSURE_SUCCESS(rv, rv);
    posStart = posEnd + 2;
    posEnd = response.Find(colonNewline, posStart);
  }

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  // Create a resource guid for this database
  nsCOMPtr<nsIUUIDGenerator> uuidGen =
    do_GetService("@mozilla.org/uuid-generator;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsID id;
  rv = uuidGen->GenerateUUIDInPlace(&id);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString guidUtf8(id.ToString());
  nsAutoString fullGuid;
  fullGuid = NS_ConvertUTF8toUTF16(guidUtf8);

  const nsAString& guid = Substring(fullGuid, 1, fullGuid.Length() - 2);

  // Insert the guid into the database
  nsCOMPtr<sbISQLInsertBuilder> insert =
    do_CreateInstance(SB_SQLBUILDER_INSERT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->SetIntoTableName(NS_LITERAL_STRING("library_metadata"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddColumn(NS_LITERAL_STRING("name"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddColumn(NS_LITERAL_STRING("value"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddValueString(NS_LITERAL_STRING("resource-guid"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddValueString(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString sql;
  rv = insert->ToString(sql);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(sql);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  return NS_OK;
}

