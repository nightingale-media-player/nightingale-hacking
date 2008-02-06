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

#include "sbDeviceCompatibility.h"

#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>

NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceCompatibility, sbIDeviceCompatibility)

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbDeviceCompatibility:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gDeviceCompatibilityLog = nsnull;
#define TRACE(args) PR_LOG(gDeviceCompatibilityLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gDeviceCompatibilityLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */

sbDeviceCompatibility::sbDeviceCompatibility()
: mInitialized(PR_FALSE)
, mCompatibility(0)
, mUserPreference(0)
{
#ifdef PR_LOGGING
  if (!gDeviceCompatibilityLog) {
    gDeviceCompatibilityLog = PR_NewLogModule("sbDeviceCompatibility");
  }
#endif
  TRACE(("DeviceCompatibility[0x%.8x] - Constructed", this));
}

sbDeviceCompatibility::~sbDeviceCompatibility()
{
  TRACE(("DeviceCompatibility[0x%.8x] - Destructed", this));
}

nsresult
sbDeviceCompatibility::Init(PRUint32 aCompatibility, 
                            PRUint32 aUserPreference)
{
  NS_ENSURE_FALSE(mInitialized, NS_ERROR_ALREADY_INITIALIZED);

  mInitialized = PR_TRUE;
  mCompatibility = aCompatibility;
  mUserPreference = aUserPreference;

  return NS_OK;
}

/* readonly attribute unsigned long compatibility; */
NS_IMETHODIMP 
sbDeviceCompatibility::GetCompatibility(PRUint32 *aCompatibility)
{
  NS_ENSURE_ARG_POINTER(aCompatibility);
  *aCompatibility = mCompatibility;
  return NS_OK;
}

/* readonly attribute unsigned long userPreference; */
NS_IMETHODIMP 
sbDeviceCompatibility::GetUserPreference(PRUint32 *aUserPreference)
{
  NS_ENSURE_ARG_POINTER(aUserPreference);
  *aUserPreference = mUserPreference;
  return NS_OK;
}
