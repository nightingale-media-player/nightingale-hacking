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
: mDeviceLibrariesMonitor(nsnull)
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
  Finalize();

  if(mDeviceLibrariesMonitor) {
    nsAutoMonitor::DestroyMonitor(mDeviceLibrariesMonitor);
  }
  TRACE(("DeviceContent[0x%.8x] - Destructed", this));
}

sbDeviceContent * sbDeviceContent::New()
{
  return new sbDeviceContent;
}

NS_IMETHODIMP
sbDeviceContent::Initialize()
{
  mDeviceLibrariesMonitor = nsAutoMonitor::NewMonitor("sbDeviceContent::mDeviceLibrariesMonitor");
  NS_ENSURE_TRUE(mDeviceLibrariesMonitor, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv;
  mDeviceLibraries = 
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceContent::Finalize()
{
  nsresult rv;

  // Finalize and remove all device libraries.
  if (mDeviceLibraries) {
    PRUint32 length;
    rv = mDeviceLibraries->GetLength(&length);
    if (NS_SUCCEEDED(rv)) {
      for (PRUint32 i = 0; i < length; i++) {
        nsCOMPtr<sbIDeviceLibrary> library;
        rv = mDeviceLibraries->QueryElementAt(i,
                                              NS_GET_IID(sbIDeviceLibrary),
                                              getter_AddRefs(library));
        if (NS_SUCCEEDED(rv))
          library->Finalize();
      }
    }
    mDeviceLibraries->Clear();
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceContent::GetLibraries(nsIArray** aLibraries)
{
  NS_ENSURE_ARG_POINTER(aLibraries);

  nsAutoMonitor mon(mDeviceLibrariesMonitor);
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
  nsAutoMonitor mon(mDeviceLibrariesMonitor);
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

  nsAutoMonitor mon(mDeviceLibrariesMonitor);
  rv = mDeviceLibraries->RemoveElementAt(itemIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbDeviceContent::FindLibrary(sbIDeviceLibrary* aLibrary, PRUint32* _retval)
{
  NS_ENSURE_ARG_POINTER(aLibrary);
  NS_ENSURE_ARG_POINTER(_retval);

  nsAutoMonitor mon(mDeviceLibrariesMonitor);
  nsresult rv;
  PRUint32 index;
  rv = mDeviceLibraries->IndexOf(0, aLibrary, &index);
  if (rv == NS_ERROR_FAILURE) {
    // yes, not found throws a generic error. sigh.
    return NS_ERROR_NOT_AVAILABLE;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = (PRUint32)index;
  return NS_OK;
}

