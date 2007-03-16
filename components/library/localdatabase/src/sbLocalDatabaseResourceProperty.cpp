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

#include "sbLocalDatabaseResourceProperty.h"

#include <nsAutoLock.h>
#include <nsMemory.h>

#include <stdio.h>
#include <prprf.h>

NS_IMPL_THREADSAFE_ISUPPORTS2(sbLocalDatabaseResourceProperty,
                   sbILocalDatabaseResourceProperty,
                   sbILibraryResource)

sbLocalDatabaseResourceProperty::sbLocalDatabaseResourceProperty()
: mPropertyCacheLock(nsnull)
, mPropertyBagLock(nsnull)
, mWriteThrough(PR_FALSE)
, mWritePending(PR_FALSE)
{
  mPropertyCacheLock = PR_NewLock();
  NS_ASSERTION(mPropertyCacheLock, "sbLocalDatabaseResourceProperty::mPropertyCacheLock creation failure!");

  mPropertyBagLock = PR_NewLock();
  NS_ASSERTION(mPropertyBagLock, "sbLocalDatabaseResourceProperty::mPropertyBagLock creation failure!");
}

sbLocalDatabaseResourceProperty::sbLocalDatabaseResourceProperty(
  sbILocalDatabaseLibrary *aLibrary,
  const nsAString& aGuid)
: mPropertyCacheLock(nsnull)
, mPropertyBagLock(nsnull)
, mGuidLock(nsnull)
, mWriteThrough(PR_FALSE)
, mWritePending(PR_FALSE)
{
  mPropertyCacheLock = PR_NewLock();
  NS_ASSERTION(mPropertyCacheLock, "sbLocalDatabaseResourceProperty::mPropertyCacheLock creation failure!");

  mPropertyBagLock = PR_NewLock();
  NS_ASSERTION(mPropertyBagLock, "sbLocalDatabaseResourceProperty::mPropertyBagLock creation failure!");

  nsresult rv = aLibrary->GetPropertyCache(getter_AddRefs(mPropertyCache));
  NS_ASSERTION(NS_SUCCEEDED(rv), "sbLocalDatabaseResourceProperty::mPropertyCache fetch failure!");

  mGuid = aGuid;
}

sbLocalDatabaseResourceProperty::~sbLocalDatabaseResourceProperty()
{
  if(mPropertyCacheLock) {
    PR_DestroyLock(mPropertyCacheLock);
  }

  if(mPropertyBagLock) {
    PR_DestroyLock(mPropertyBagLock);
  }
}

NS_IMETHODIMP
sbLocalDatabaseResourceProperty::Init(sbILocalDatabasePropertyCache *aPropertyCache, 
                                      const nsAString& aGuid)
{
  NS_ENSURE_TRUE(mPropertyCacheLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mPropertyBagLock, NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_ARG_POINTER(aPropertyCache);

  PR_Lock(mPropertyCacheLock);
  
  mPropertyCache = aPropertyCache;
  mGuid = aGuid;

  PR_Unlock(mPropertyCacheLock);

  return NS_OK;
}

NS_IMETHODIMP 
sbLocalDatabaseResourceProperty::GetWriteThrough(PRBool *aWriteThrough)
{
  NS_ENSURE_ARG_POINTER(aWriteThrough);
  *aWriteThrough = mWriteThrough;
  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseResourceProperty::SetWriteThrough(PRBool aWriteThrough)
{
  mWriteThrough = aWriteThrough;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseResourceProperty::GetWritePending(PRBool *aWritePending)
{
  NS_ENSURE_ARG_POINTER(aWritePending);
  *aWritePending = mWritePending;
  return NS_OK;
}

NS_IMETHODIMP sbLocalDatabaseResourceProperty::Write()
{
  NS_ENSURE_TRUE(mPropertyCacheLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mPropertyBagLock, NS_ERROR_NOT_INITIALIZED);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseResourceProperty::GetUri(nsIURI * *aUri)
{
  NS_ENSURE_TRUE(mPropertyCacheLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mPropertyBagLock, NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_ARG_POINTER(aUri);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseResourceProperty::GetCreated(PRInt32 *aCreated)
{
  NS_ENSURE_TRUE(mPropertyCacheLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mPropertyBagLock, NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_ARG_POINTER(aCreated);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseResourceProperty::GetUpdated(PRInt32 *aUpdated)
{
  NS_ENSURE_TRUE(mPropertyCacheLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mPropertyBagLock, NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_ARG_POINTER(aUpdated);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseResourceProperty::GetProperty(const nsAString & aName, 
                                             nsAString & _retval)
{
  NS_ENSURE_TRUE(mPropertyCacheLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mPropertyBagLock, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;
  nsAutoLock lock(mPropertyBagLock);

  //Sometimes the property bag isn't ready for us to get during construction
  //because the cache isn't done loading. Check here to see if we need
  //to grab it.
  
  //XXX: If cache is invalidated, refresh now?
  if(!mPropertyBag) {
    PRUint32 count = 0;
    const PRUnichar** guids = new const PRUnichar*[1];
    sbILocalDatabaseResourcePropertyBag** bags;

    guids[0] = PromiseFlatString(mGuid).get();

    nsAutoLock cacheLock(mPropertyCacheLock);
    rv = mPropertyCache->GetProperties(guids, 1, &count, &bags);

    NS_ENSURE_SUCCESS(rv, rv);

    if (count > 0 && bags[0]) {
      mPropertyBag = bags[0];
    }

    nsMemory::Free(bags);
    delete [] guids;
  }

  rv = NS_ERROR_NOT_AVAILABLE;
  if(mPropertyBag)
  {
    rv = mPropertyBag->GetProperty(aName, _retval);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return rv;
}

NS_IMETHODIMP
sbLocalDatabaseResourceProperty::SetProperty(const nsAString & aName, 
                                             const nsAString & aValue)
{
  NS_ENSURE_TRUE(mPropertyCacheLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mPropertyBagLock, NS_ERROR_NOT_INITIALIZED);

  return NS_ERROR_NOT_IMPLEMENTED;
}
