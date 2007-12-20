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

#include "sbAbstractPropertyBuilder.h"

#include <nsAutoPtr.h>

#include <nsServiceManagerUtils.h>

#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>
#include <nsIStringBundle.h>

#ifdef DEBUG
#include <prprf.h>
#endif

NS_IMPL_ISUPPORTS1(sbAbstractPropertyBuilder,
                   sbIPropertyBuilder)

sbAbstractPropertyBuilder::sbAbstractPropertyBuilder() :
  mUserViewable(PR_TRUE),
  mUserEditable(PR_TRUE),
  mRemoteReadable(PR_FALSE),
  mRemoteWritable(PR_FALSE)
{
}

nsresult
sbAbstractPropertyBuilder::Init()
{
  nsresult rv;
  rv = CreateBundle("chrome://songbird/locale/songbird.properties",
                    getter_AddRefs(mBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbAbstractPropertyBuilder::GetFinalDisplayName(nsAString& aDisplayName)
{
  if (!mDisplayNameKey.IsEmpty()) {
    nsresult rv = GetStringFromName(mBundle, mDisplayNameKey, aDisplayName);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    aDisplayName = mDisplayName;
  }

  return NS_OK;
}

/* static */ nsresult
sbAbstractPropertyBuilder::GetStringFromName(nsIStringBundle* aBundle,
                                             const nsAString& aName,
                                             nsAString& _retval)
{
  NS_ENSURE_ARG_POINTER(aBundle);

  nsString value;
  nsresult rv = aBundle->GetStringFromName(aName.BeginReading(),
                                           getter_Copies(value));
  if (NS_SUCCEEDED(rv)) {
    _retval = value;
  }
  else {
#ifdef DEBUG
    char* message = PR_smprintf("sbPropertyManager: '%s' not found in bundle",
                                NS_LossyConvertUTF16toASCII(aName).get());
    NS_WARNING(message);
    PR_smprintf_free(message);
#endif
    _retval = aName;
  }

  return NS_OK;
}

/* static */ nsresult
sbAbstractPropertyBuilder::CreateBundle(const char* aURLSpec, 
                                        nsIStringBundle** _retval)
{
  NS_ENSURE_ARG_POINTER(aURLSpec);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;
  nsCOMPtr<nsIStringBundleService> stringBundleService =
    do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIStringBundle> stringBundle; 
  rv = stringBundleService->CreateBundle(aURLSpec, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbAbstractPropertyBuilder::Get(sbIPropertyInfo** _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbAbstractPropertyBuilder::GetPropertyID(nsAString& aPropertyID)
{
  aPropertyID = mPropertyID;
  return NS_OK;
}
NS_IMETHODIMP
sbAbstractPropertyBuilder::SetPropertyID(const nsAString& aPropertyID)
{
  mPropertyID = aPropertyID;
  return NS_OK;
}

NS_IMETHODIMP
sbAbstractPropertyBuilder::GetDisplayName(nsAString& aDisplayName)
{
  aDisplayName = mDisplayName;
  return NS_OK;
}
NS_IMETHODIMP
sbAbstractPropertyBuilder::SetDisplayName(const nsAString& aDisplayName)
{
  mDisplayName = aDisplayName;
  return NS_OK;
}

NS_IMETHODIMP
sbAbstractPropertyBuilder::GetDisplayNameKey(nsAString& aDisplayNameKey)
{
  aDisplayNameKey = mDisplayNameKey;
  return NS_OK;
}
NS_IMETHODIMP
sbAbstractPropertyBuilder::SetDisplayNameKey(const nsAString& aDisplayNameKey)
{
  mDisplayNameKey = aDisplayNameKey;
  return NS_OK;
}

NS_IMETHODIMP
sbAbstractPropertyBuilder::GetUserViewable(PRBool* aUserViewable)
{
  NS_ENSURE_ARG_POINTER(aUserViewable);
  *aUserViewable = mUserViewable;
  return NS_OK;
}
NS_IMETHODIMP
sbAbstractPropertyBuilder::SetUserViewable(PRBool aUserViewable)
{
  mUserViewable = aUserViewable;
  return NS_OK;
}

NS_IMETHODIMP
sbAbstractPropertyBuilder::GetUserEditable(PRBool* aUserEditable)
{
  NS_ENSURE_ARG_POINTER(aUserEditable);
  *aUserEditable = mUserEditable;
  return NS_OK;
}
NS_IMETHODIMP
sbAbstractPropertyBuilder::SetUserEditable(PRBool aUserEditable)
{
  mUserEditable = aUserEditable;
  return NS_OK;
}

NS_IMETHODIMP
sbAbstractPropertyBuilder::GetRemoteReadable(PRBool* aRemoteReadable)
{
  NS_ENSURE_ARG_POINTER(aRemoteReadable);
  *aRemoteReadable = mRemoteReadable;
  return NS_OK;
}
NS_IMETHODIMP
sbAbstractPropertyBuilder::SetRemoteReadable(PRBool aRemoteReadable)
{
  mRemoteReadable = aRemoteReadable;
  return NS_OK;
}

NS_IMETHODIMP
sbAbstractPropertyBuilder::GetRemoteWritable(PRBool* aRemoteWritable)
{
  NS_ENSURE_ARG_POINTER(aRemoteWritable);
  *aRemoteWritable = mRemoteWritable;
  return NS_OK;
}
NS_IMETHODIMP
sbAbstractPropertyBuilder::SetRemoteWritable(PRBool aRemoteWritable)
{
  mRemoteWritable = aRemoteWritable;
  return NS_OK;
}

