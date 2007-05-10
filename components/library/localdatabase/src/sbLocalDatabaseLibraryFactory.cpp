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

#include <nsIFile.h>
#include <nsIIOService.h>
#include <nsILocalFile.h>
#include <nsIProperties.h>
#include <nsIPropertyBag2.h>
#include <nsIURI.h>
#include <nsIURL.h>
#include <nsIUUIDGenerator.h>
#include <nsIWritablePropertyBag2.h>
#include <nsIXMLHttpRequest.h>
#include <sbIDatabaseQuery.h>
#include <sbIDatabaseResult.h>
#include <sbILibrary.h>
#include <sbILocalDatabasePropertyCache.h>
#include <sbISQLBuilder.h>

#include <DatabaseQuery.h>
#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsNetUtil.h>
#include <nsServiceManagerUtils.h>
#include <nsXPCOMCID.h>
#include "sbLocalDatabaseCID.h"
#include "sbLocalDatabaseLibrary.h"
#include <sbSQLBuilderCID.h>

#define DEFAULT_LIBRARY_NAME NS_LITERAL_STRING("defaultlibrary.db")
#define NS_HASH_PROPERTY_BAG_CONTRACTID "@mozilla.org/hash-property-bag;1"
#define PROPERTY_KEY_DATABASEFILE "databaseFile"
#define SCHEMA_URL "chrome://songbird/content/library/localdatabase/schema.sql"

#define PERMISSIONS_FILE      0644
#define PERMISSIONS_DIRECTORY 0755

static nsresult
CreateDirectory(nsIFile* aDirectory)
{
  PRBool exists;
  nsresult rv = aDirectory->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (exists) {
    return NS_OK;
  }

  rv = aDirectory->Create(nsIFile::DIRECTORY_TYPE, PERMISSIONS_DIRECTORY);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

static PRBool
IsDirectoryWritable(nsIFile* aDirectory)
{
  PRBool isDirectory;
  nsresult rv = aDirectory->IsDirectory(&isDirectory);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  NS_ENSURE_TRUE(isDirectory, PR_FALSE);

  PRBool exists;
  rv = aDirectory->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  NS_ENSURE_TRUE(exists, PR_FALSE);

  nsCOMPtr<nsIFile> testFile;
  rv = aDirectory->Clone(getter_AddRefs(testFile));
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  rv = testFile->Append(NS_LITERAL_STRING("libraryFactory.test"));
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  rv = testFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, PERMISSIONS_FILE);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  rv = testFile->Remove(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  return PR_TRUE;
}

static already_AddRefed<nsILocalFile>
GetDBFolder()
{
  nsresult rv;
  nsCOMPtr<nsIProperties> ds =
    do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, nsnull);

  nsILocalFile* file;
  rv = ds->Get("ProfD", NS_GET_IID(nsILocalFile), (void**)&file);
  NS_ENSURE_SUCCESS(rv, nsnull);

  rv = file->AppendRelativePath(NS_LITERAL_STRING("db"));
  if (NS_FAILED(rv)) {
    NS_WARNING("AppendRelativePath failed!");

    NS_RELEASE(file);
    return nsnull;
  }

  return file;
}

NS_IMPL_ISUPPORTS1(sbLocalDatabaseLibraryFactory, sbILibraryFactory)

NS_IMETHODIMP
sbLocalDatabaseLibraryFactory::GetType(nsAString& aType)
{
  aType.AssignLiteral(SB_LOCALDATABASE_LIBRARYFACTORY_TYPE);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseLibraryFactory::GetContractID(nsACString& aContractID)
{
  aContractID.AssignLiteral(SB_LOCALDATABASE_LIBRARYFACTORY_CONTRACTID);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseLibraryFactory::CreateLibrary(nsIPropertyBag2* aCreationParameters,
                                             sbILibrary** _retval)
{
  NS_ENSURE_ARG_POINTER(aCreationParameters);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<nsILocalFile> file;
  nsresult rv =
    aCreationParameters->GetPropertyAsInterface(NS_LITERAL_STRING(PROPERTY_KEY_DATABASEFILE),
                                                NS_GET_IID(nsILocalFile),
                                                getter_AddRefs(file));
  if (NS_FAILED(rv)) {
    NS_WARNING("You passed in a property bag with the wrong data!");

    file = GetDBFolder();
    NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);

    rv = file->AppendRelativePath(DEFAULT_LIBRARY_NAME);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = CreateLibraryFromDatabase(file, _retval, aCreationParameters);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseLibraryFactory::CreateLibraryFromDatabase(nsIFile* aDatabase,
                                                         sbILibrary** _retval,
                                                         nsIPropertyBag2* aCreationParameters)
{
  NS_ENSURE_ARG_POINTER(aDatabase);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  // If the database file does not exist, create and initalize it
  PRBool exists;
  rv = aDatabase->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!exists) {
    rv = InitalizeLibrary(aDatabase);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIIOService> ioService = do_GetIOService(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> databaseURI;
  rv = NS_NewFileURI(getter_AddRefs(databaseURI), aDatabase, ioService);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURL> databaseURL = do_QueryInterface(databaseURI, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString utf8GUID;
  rv = databaseURL->GetFileBaseName(utf8GUID);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> databaseParent;
  rv = aDatabase->GetParent(getter_AddRefs(databaseParent));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> databaseLocation;
  rv = NS_NewFileURI(getter_AddRefs(databaseLocation), databaseParent,
                     ioService);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoPtr<sbLocalDatabaseLibrary> library(new sbLocalDatabaseLibrary());
  NS_ENSURE_TRUE(library, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIPropertyBag2> creationParams = aCreationParameters;
  if (!creationParams) {
    nsCOMPtr<nsIWritablePropertyBag2> bag =
      do_CreateInstance(NS_HASH_PROPERTY_BAG_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = bag->SetPropertyAsInterface(NS_LITERAL_STRING(PROPERTY_KEY_DATABASEFILE),
                                     aDatabase);
    NS_ENSURE_SUCCESS(rv, rv);

    creationParams = do_QueryInterface(bag, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = library->Init(NS_ConvertUTF8toUTF16(utf8GUID), creationParams, this,
                     databaseLocation);
  NS_ENSURE_SUCCESS(rv, rv);


  NS_ADDREF(*_retval = library.forget());
  return NS_OK;
}

nsresult
sbLocalDatabaseLibraryFactory::InitalizeLibrary(nsIFile* aDatabaseFile)
{
  nsresult rv;
  PRInt32 dbOk;

  nsCOMPtr<nsIFile> parentDirectory;
  rv = aDatabaseFile->GetParent(getter_AddRefs(parentDirectory));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool parentExists;
  rv = parentDirectory->Exists(&parentExists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!parentExists) {
    rv = CreateDirectory(parentDirectory);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRBool parentIsWritable = IsDirectoryWritable(parentDirectory);
  NS_ENSURE_TRUE(parentIsWritable, NS_ERROR_FILE_ACCESS_DENIED);

  // Now that we know we have appropriate permissions make a new query.
  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance(SONGBIRD_DATABASEQUERY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetAsyncQuery(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIIOService> ioService = do_GetIOService(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Figure out a GUID based on the filename. All our database files use the
  // '.db' extension, so if someone requests another extension then that's just
  // too bad.
  nsCOMPtr<nsIURI> fileURI;
  rv = NS_NewFileURI(getter_AddRefs(fileURI), aDatabaseFile, ioService);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURL> fileURL = do_QueryInterface(fileURI, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString fileBaseName;
  rv = fileURL->GetFileBaseName(fileBaseName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetDatabaseGUID(NS_ConvertUTF8toUTF16(fileBaseName));
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef DEBUG
  // Warn anyone if they passed in an extension other than '.db'.
  nsCAutoString fileExtension;
  rv = fileURL->GetFileExtension(fileExtension);
  if (NS_SUCCEEDED(rv)) {
    if (!fileExtension.IsEmpty() &&
        !fileExtension.Equals("db", CaseInsensitiveCompare)) {
      NS_WARNING("All database files are forced to use the '.db' extension.");
    }
  }
#endif
  // Set the parent directory as the location for the new database file.
  nsCOMPtr<nsIURI> parentURI;
  rv = NS_NewFileURI(getter_AddRefs(parentURI), parentDirectory, ioService);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetDatabaseLocation(parentURI);
  NS_ENSURE_SUCCESS(rv, rv);

  // Load the schema file, and add each colon-newline delimited line into
  // the query object.
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

  // Create a resource guid for this database.
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

  // Insert the guid into the database.
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

already_AddRefed<nsILocalFile>
sbLocalDatabaseLibraryFactory::GetFileForGUID(const nsAString& aGUID)
{
  nsCOMPtr<nsILocalFile> file = GetDBFolder();
  NS_ENSURE_TRUE(file, nsnull);

  nsAutoString filename(aGUID);
  filename.AppendLiteral(".db");

  nsresult rv = file->AppendRelativePath(filename);
  NS_ENSURE_SUCCESS(rv, nsnull);

  nsILocalFile* _retval;
  NS_ADDREF(_retval = file);

  return _retval;
}

void
sbLocalDatabaseLibraryFactory::GetGUIDFromFile(nsILocalFile* aFile,
                                               nsAString& aGUID)
{
  nsAutoString filename;
  nsresult rv = aFile->GetLeafName(filename);
  NS_ENSURE_SUCCESS(rv,);

  aGUID.Assign(StringHead(filename, filename.Length() - 3));
}
