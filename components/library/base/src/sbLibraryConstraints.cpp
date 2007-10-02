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

#include "sbLibraryConstraints.h"

#include <nsIClassInfoImpl.h>
#include <nsIObjectInputStream.h>
#include <nsIObjectOutputStream.h>
#include <nsIProgrammingLanguage.h>
#include <nsMemory.h>

static NS_DEFINE_CID(kLibraryFilterCID,
                     SONGBIRD_LIBRARYFILTER_CID);

NS_IMPL_ISUPPORTS3(sbLibraryFilter,
                   sbILibraryFilter,
                   nsISerializable,
                   nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER3(sbLibraryFilter,
                             sbILibraryFilter,
                             nsISerializable,
                             nsIClassInfo)

sbLibraryFilter::sbLibraryFilter() :
  mInitialized(PR_FALSE)
{
}

NS_IMETHODIMP
sbLibraryFilter::Init(const nsAString& aProperty,
                      PRUint32 aCount,
                      const PRUnichar** aValues)
{
  NS_ENSURE_ARG_POINTER(aValues);
  NS_ENSURE_STATE(!mInitialized);

  mProperty = aProperty;

  for (PRUint32 i = 0; i < aCount; i++) {
    nsString value(aValues[i]);
    nsString* added = mValues.AppendElement(value);
    NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);
  }

  mInitialized = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryFilter::InitNative(const nsAString& aProperty,
                            nsTArray<nsString>* aValues)
{
  NS_ENSURE_ARG_POINTER(aValues);
  NS_ENSURE_STATE(!mInitialized);

  mProperty = aProperty;

  for (PRUint32 i = 0; i < aValues->Length(); i++) {
    nsString* added = mValues.AppendElement(aValues->ElementAt(i));
    NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);
  }

  mInitialized = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryFilter::GetProperty(nsAString& aProperty)
{
  NS_ENSURE_STATE(mInitialized);
  aProperty = mProperty;
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryFilter::GetValues(PRUint32* aCount, PRUnichar*** aValues)
{
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(aCount);
  NS_ENSURE_ARG_POINTER(aValues);

  *aCount = mValues.Length();
  *aValues = (PRUnichar **) nsMemory::Alloc(sizeof(PRUnichar*) * (*aCount));
  NS_ENSURE_TRUE(*aValues, NS_ERROR_OUT_OF_MEMORY);

  for (PRUint32 i = 0; i < *aCount; i++) {
    (*aValues)[i] = ToNewUnicode(mValues[i]);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLibraryFilter::GetValuesNative(nsTArray<nsString>** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = &mValues;
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryFilter::ToString(nsAString& aString)
{
  NS_ENSURE_STATE(mInitialized);

  nsString buff;
  buff.AssignLiteral("filter: property = '");
  buff.Append(mProperty);
  buff.AppendLiteral("' values = [");
  for (PRUint32 i = 0; i < mValues.Length(); i++) {
    buff.AppendLiteral("'");
    buff.Append(mValues[i]);
    buff.AppendLiteral("'");
    if (i + 1 < mValues.Length()) {
      buff.AppendLiteral(", ");
    }
  }
  buff.AppendLiteral("]");

  aString = buff;
  return NS_OK;
}

// nsISerializable
NS_IMETHODIMP
sbLibraryFilter::Read(nsIObjectInputStream* aStream)
{
  NS_ENSURE_ARG_POINTER(aStream);

  nsresult rv;

  rv = aStream->ReadString(mProperty);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length;
  rv = aStream->Read32(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < length; i++) {
    nsString value;
    rv = aStream->ReadString(value);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString* added = mValues.AppendElement(value);
    NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);
  }

  mInitialized = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
sbLibraryFilter::Write(nsIObjectOutputStream* aStream)
{
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(aStream);

  nsresult rv;

  rv = aStream->WriteWStringZ(mProperty.BeginReading());
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length = mValues.Length();
  rv = aStream->Write32(length);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < length; i++) {
    rv = aStream->WriteWStringZ(mValues[i].BeginReading());
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

// nsIClassInfo
NS_IMETHODIMP
sbLibraryFilter::GetInterfaces(PRUint32* count, nsIID*** array)
{
  return NS_CI_INTERFACE_GETTER_NAME(sbLibraryFilter)(count, array);
}

NS_IMETHODIMP
sbLibraryFilter::GetHelperForLanguage(PRUint32 language,
                                      nsISupports** _retval)
{
  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryFilter::GetContractID(char** aContractID)
{
  *aContractID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryFilter::GetClassDescription(char** aClassDescription)
{
  *aClassDescription = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryFilter::GetClassID(nsCID** aClassID)
{
  *aClassID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryFilter::GetImplementationLanguage(PRUint32* aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryFilter::GetFlags(PRUint32* aFlags)
{
  *aFlags = 0;
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryFilter::GetClassIDNoAlloc(nsCID* aClassIDNoAlloc)
{
  NS_ENSURE_ARG_POINTER(aClassIDNoAlloc);
  *aClassIDNoAlloc = kLibraryFilterCID;
  return NS_OK;
}

static NS_DEFINE_CID(kLibrarySearchCID,
                     SONGBIRD_LIBRARYSEARCH_CID);

NS_IMPL_ISUPPORTS3(sbLibrarySearch,
                   sbILibrarySearch,
                   nsISerializable,
                   nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER3(sbLibrarySearch,
                             sbILibrarySearch,
                             nsISerializable,
                             nsIClassInfo)

sbLibrarySearch::sbLibrarySearch() :
  mInitialized(PR_FALSE),
  mIsAll(PR_FALSE)
{
}

NS_IMETHODIMP
sbLibrarySearch::Init(const nsAString& aProperty,
                      PRBool aIsAll,
                      PRUint32 aCount,
                      const PRUnichar** aValues)
{
  NS_ENSURE_ARG_POINTER(aValues);
  NS_ENSURE_STATE(!mInitialized);

  mProperty = aProperty;
  mIsAll = aIsAll;

  for (PRUint32 i = 0; i < aCount; i++) {
    nsString value(aValues[i]);
    nsString* added = mValues.AppendElement(value);
    NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);
  }

  mInitialized = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySearch::InitNative(const nsAString& aProperty,
                            PRBool aIsAll,
                            nsTArray<nsString>* aValues)
{
  NS_ENSURE_ARG_POINTER(aValues);
  NS_ENSURE_STATE(!mInitialized);

  mProperty = aProperty;
  mIsAll = aIsAll;

  for (PRUint32 i = 0; i < aValues->Length(); i++) {
    nsString* added = mValues.AppendElement(aValues->ElementAt(i));
    NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);
  }

  mInitialized = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySearch::GetProperty(nsAString& aProperty)
{
  NS_ENSURE_STATE(mInitialized);
  aProperty = mProperty;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySearch::GetIsAll(PRBool* aIsAll)
{
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(aIsAll);

  *aIsAll = mIsAll;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySearch::GetValues(PRUint32* aCount, PRUnichar*** aValues)
{
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(aCount);
  NS_ENSURE_ARG_POINTER(aValues);

  *aCount = mValues.Length();
  *aValues = (PRUnichar **) nsMemory::Alloc(sizeof(PRUnichar*) * (*aCount));
  NS_ENSURE_TRUE(*aValues, NS_ERROR_OUT_OF_MEMORY);

  for (PRUint32 i = 0; i < *aCount; i++) {
    (*aValues)[i] = ToNewUnicode(mValues[i]);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySearch::GetValuesNative(nsTArray<nsString>** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = &mValues;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySearch::ToString(nsAString& aString)
{
  NS_ENSURE_STATE(mInitialized);

  nsString buff;
  buff.AssignLiteral("search: property = '");
  buff.Append(mProperty);
  buff.AppendLiteral("' is all = ");
  buff.AppendLiteral(mIsAll ? "yes" : "no");
  buff.AppendLiteral(" values = [");
  for (PRUint32 i = 0; i < mValues.Length(); i++) {
    buff.AppendLiteral("'");
    buff.Append(mValues[i]);
    buff.AppendLiteral("'");
    if (i + 1 < mValues.Length()) {
      buff.AppendLiteral(", ");
    }
  }
  buff.AppendLiteral("]");

  aString = buff;
  return NS_OK;
}

// nsISerializable
NS_IMETHODIMP
sbLibrarySearch::Read(nsIObjectInputStream* aStream)
{
  NS_ENSURE_ARG_POINTER(aStream);

  nsresult rv;

  rv = aStream->ReadString(mProperty);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aStream->ReadBoolean(&mIsAll);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length;
  rv = aStream->Read32(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < length; i++) {
    nsString value;
    rv = aStream->ReadString(value);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString* added = mValues.AppendElement(value);
    NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);
  }

  mInitialized = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySearch::Write(nsIObjectOutputStream* aStream)
{
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(aStream);

  nsresult rv;

  rv = aStream->WriteWStringZ(mProperty.BeginReading());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aStream->WriteBoolean(mIsAll);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length = mValues.Length();
  rv = aStream->Write32(length);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < length; i++) {
    rv = aStream->WriteWStringZ(mValues[i].BeginReading());
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

// nsIClassInfo
NS_IMETHODIMP
sbLibrarySearch::GetInterfaces(PRUint32* count, nsIID*** array)
{
  return NS_CI_INTERFACE_GETTER_NAME(sbLibrarySearch)(count, array);
}

NS_IMETHODIMP
sbLibrarySearch::GetHelperForLanguage(PRUint32 language,
                                      nsISupports** _retval)
{
  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySearch::GetContractID(char** aContractID)
{
  *aContractID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySearch::GetClassDescription(char** aClassDescription)
{
  *aClassDescription = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySearch::GetClassID(nsCID** aClassID)
{
  *aClassID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySearch::GetImplementationLanguage(PRUint32* aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySearch::GetFlags(PRUint32* aFlags)
{
  *aFlags = 0;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySearch::GetClassIDNoAlloc(nsCID* aClassIDNoAlloc)
{
  NS_ENSURE_ARG_POINTER(aClassIDNoAlloc);
  *aClassIDNoAlloc = kLibrarySearchCID;
  return NS_OK;
}

static NS_DEFINE_CID(kLibrarySortCID,
                     SONGBIRD_LIBRARYSORT_CID);

NS_IMPL_ISUPPORTS3(sbLibrarySort,
                   sbILibrarySort,
                   nsISerializable,
                   nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER3(sbLibrarySort,
                             sbILibrarySort,
                             nsISerializable,
                             nsIClassInfo)

sbLibrarySort::sbLibrarySort() :
  mInitialized(PR_FALSE),
  mIsAscending(PR_FALSE)
{
}

NS_IMETHODIMP
sbLibrarySort::Init(const nsAString& aProperty, PRBool aIsAscending)
{
  NS_ENSURE_STATE(!mInitialized);
  mProperty = aProperty;
  mIsAscending = aIsAscending;
  mInitialized = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySort::GetProperty(nsAString& aProperty)
{
  NS_ENSURE_STATE(mInitialized);
  aProperty = mProperty;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySort::GetIsAscending(PRBool* aIsAscending)
{
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(aIsAscending);

  *aIsAscending = mIsAscending;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySort::ToString(nsAString& aString)
{
  NS_ENSURE_STATE(mInitialized);

  nsString buff;
  buff.AssignLiteral("sort: property = '");
  buff.Append(mProperty);
  buff.AppendLiteral("' is ascending = ");
  buff.AppendLiteral(mIsAscending ? "yes" : "no");

  aString = buff;
  return NS_OK;
}

// nsISerializable
NS_IMETHODIMP
sbLibrarySort::Read(nsIObjectInputStream* aStream)
{
  NS_ENSURE_ARG_POINTER(aStream);

  nsresult rv;

  rv = aStream->ReadString(mProperty);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aStream->ReadBoolean(&mIsAscending);
  NS_ENSURE_SUCCESS(rv, rv);

  mInitialized = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySort::Write(nsIObjectOutputStream* aStream)
{
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(aStream);

  nsresult rv;

  rv = aStream->WriteWStringZ(mProperty.BeginReading());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aStream->WriteBoolean(mIsAscending);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// nsIClassInfo
NS_IMETHODIMP
sbLibrarySort::GetInterfaces(PRUint32* count, nsIID*** array)
{
  return NS_CI_INTERFACE_GETTER_NAME(sbLibrarySort)(count, array);
}

NS_IMETHODIMP
sbLibrarySort::GetHelperForLanguage(PRUint32 language,
                                    nsISupports** _retval)
{
  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySort::GetContractID(char** aContractID)
{
  *aContractID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySort::GetClassDescription(char** aClassDescription)
{
  *aClassDescription = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySort::GetClassID(nsCID** aClassID)
{
  *aClassID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySort::GetImplementationLanguage(PRUint32* aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySort::GetFlags(PRUint32* aFlags)
{
  *aFlags = 0;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySort::GetClassIDNoAlloc(nsCID* aClassIDNoAlloc)
{
  NS_ENSURE_ARG_POINTER(aClassIDNoAlloc);
  *aClassIDNoAlloc = kLibrarySortCID;
  return NS_OK;
}

