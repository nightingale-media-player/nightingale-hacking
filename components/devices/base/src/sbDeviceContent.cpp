/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

#include "sbDeviceContent.h"

#include <sbIDeviceLibrary.h>

#include <nsArrayEnumerator.h>
#include <nsComponentManagerUtils.h>
#include <nsISimpleEnumerator.h>
#include <nsServiceManagerUtils.h>

NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceContent, sbIDeviceContent)

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbDeviceContent:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gDeviceContentLog = nsnull;
#define TRACE(args) PR_LOG(gDeviceContentLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gDeviceContentLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */

sbDeviceContent::sbDeviceContent()
: mDeviceLibrariesLock(nsnull)
{
#ifdef PR_LOGGING
  if (!gDeviceContentLog) {
    gDeviceContentLog = PR_NewLogModule("sbDeviceContent");
  }
#endif
  TRACE(("DeviceContent[0x%.8x] - Constructed", this));
}

sbDeviceContent::~sbDeviceContent()
{
  if(mDeviceLibrariesLock) {
    nsAutoLock::DestroyLock(mDeviceLibrariesLock);
  }
  TRACE(("DeviceContent[0x%.8x] - Destructed", this));
}

sbDeviceContent * sbDeviceContent::New()
{
  return new sbDeviceContent;
}

nsresult
sbDeviceContent::Init()
{
  mDeviceLibrariesLock = nsAutoLock::NewLock("sbDeviceContent::mDeviceLibrariesLock");
  NS_ENSURE_TRUE(mDeviceLibrariesLock, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv;
  mDeviceLibraries = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceContent::GetLibraries(nsIArray** aLibraries)
{
  NS_ENSURE_ARG_POINTER(aLibraries);

  nsAutoLock lock(mDeviceLibrariesLock);
  *aLibraries = mDeviceLibraries;
  NS_ADDREF(*aLibraries);
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceContent::GetLibraryFactory(sbILibraryFactory** aLibraryFactory)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbDeviceContent::AddLibrary(sbIDeviceLibrary* aLibrary)
{
  NS_ENSURE_ARG_POINTER(aLibrary);

  nsresult rv;
  nsAutoLock lock(mDeviceLibrariesLock);
  PRUint32 existingIndex;
  rv = FindLibrary(aLibrary, &existingIndex);
  if (NS_FAILED(rv)) {
    rv = mDeviceLibraries->AppendElement(aLibrary, false);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceContent::RemoveLibrary(sbIDeviceLibrary* aLibrary)
{
  NS_ENSURE_ARG_POINTER(aLibrary);
  PRUint32 itemIndex;

  nsresult rv;
  rv = FindLibrary(aLibrary, &itemIndex);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsAutoLock lock(mDeviceLibrariesLock);
  rv = mDeviceLibraries->RemoveElementAt(itemIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbDeviceContent::FindLibrary(sbIDeviceLibrary* aLibrary, PRUint32* _retval)
{
  NS_ENSURE_ARG_POINTER(aLibrary);
  NS_ENSURE_ARG_POINTER(_retval);

  nsAutoLock lock(mDeviceLibrariesLock);
  nsresult rv;
  PRUint32 index;
  rv = mDeviceLibraries->IndexOf(0, aLibrary, &index);
  NS_ENSURE_SUCCESS(rv, rv);

  if (index < 0) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  *_retval = (PRUint32)index;
  return NS_OK;
}

