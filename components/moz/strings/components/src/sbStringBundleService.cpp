/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird string bundle service.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbStringBundleService.cpp
 * \brief Songbird String Bundle Service Source.
 */

//------------------------------------------------------------------------------
//
// Songbird string bundle service imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbStringBundleService.h"

// Mozilla interfaces
#include <nsIObserverService.h>

// Local imports.
#include "sbStringUtils.h"

// Mozilla imports.
#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>

//------------------------------------------------------------------------------
//
// nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS3(sbStringBundleService,
                              nsIStringBundleService,
                              nsIObserver,
                              sbIStringBundleService)


//------------------------------------------------------------------------------
//
// sbIStringBundleService implementation.
//
//------------------------------------------------------------------------------

//
// Getters/setters.
//

/**
 * \brief Main Songbird string bundle.  Thread-safe.
 */

NS_IMETHODIMP
sbStringBundleService::GetBundle(nsIStringBundle** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ADDREF(*_retval = mBundle);
  return NS_OK;
}


/**
 * \brief Main Songbird brand string bundle.  Thread-safe.
 */

NS_IMETHODIMP
sbStringBundleService::GetBrandBundle(nsIStringBundle** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ADDREF(*_retval = mBrandBundle);
  return NS_OK;
}


//------------------------------------------------------------------------------
//
// nsIStringBundleService implementation.
//
//------------------------------------------------------------------------------

/**
 * \brief Create a thread-safe string bundle for the bundle with the URL spec
 *        specified by aURLSpec.
 *
 * \param aURLSpec            URL spec of string bundle.
 *
 * \return                    Thread-safe string bundle.
 */

NS_IMETHODIMP
sbStringBundleService::CreateBundle(const char*       aURLSpec,
                                    nsIStringBundle** _retval)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aURLSpec);
  NS_ENSURE_ARG_POINTER(_retval);

  // Function variables.
  nsresult rv;

  // Get the base string bundle.
  nsCOMPtr<nsIStringBundle> baseStringBundle;
  rv = mBaseStringBundleService->CreateBundle(aURLSpec,
                                              getter_AddRefs(baseStringBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the Songbird string bundle.
  nsRefPtr<sbStringBundle> stringBundle;
  rv = sbStringBundle::New(baseStringBundle, getter_AddRefs(stringBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  NS_ADDREF(*_retval = stringBundle);

  return NS_OK;
}


/**
 * Create an extensible bundle from the registry key specified by aRegistryKey.
 *
 * \param aRegistryKey          Registry key.
 *
 * \return                      Created bundle.
 */

NS_IMETHODIMP
sbStringBundleService::CreateExtensibleBundle(const char      *aRegistryKey,
                                              nsIStringBundle **_retval)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aRegistryKey);
  NS_ENSURE_ARG_POINTER(_retval);

  // Function variables.
  nsresult rv;

  // Get the base string bundle.
  nsCOMPtr<nsIStringBundle> baseStringBundle;
  rv = mBaseStringBundleService->CreateExtensibleBundle
                                   (aRegistryKey,
                                    getter_AddRefs(baseStringBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the Songbird string bundle.
  nsRefPtr<sbStringBundle> stringBundle;
  rv = sbStringBundle::New(baseStringBundle, getter_AddRefs(stringBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  NS_ADDREF(*_retval = stringBundle);

  return NS_OK;
}


/**
 * Formats a message string from a status code and status arguments.
 * @param aStatus - The status code. This is mapped into a string ID and
 *            and used in the string lookup process (see nsIErrorService).
 * @param aStatusArg - The status message argument(s). Multiple arguments
 *            can be separated by newline ('\n') characters.
 * @return the formatted message
 */

NS_IMETHODIMP
sbStringBundleService::FormatStatusMessage(nsresult        aStatus,
                                           const PRUnichar *aStatusArg,
                                           PRUnichar       **_retval)
{
  return mBaseStringBundleService->FormatStatusMessage(aStatus,
                                                       aStatusArg,
                                                       _retval);
}


/**
 * flushes the string bundle cache - useful when the locale changes or
 * when we need to get some extra memory back
 *
 * at some point, we might want to make this flush all the bundles,
 * because any bundles that are floating around when the locale changes
 * will suddenly contain bad data
 *
 */

NS_IMETHODIMP
sbStringBundleService::FlushBundles()
{
  return mBaseStringBundleService->FlushBundles();
}


//------------------------------------------------------------------------------
//
// nsIObserver implementation.
//
//------------------------------------------------------------------------------

NS_IMETHODIMP
sbStringBundleService::Observe(nsISupports *aSubject,
                               const char *aTopic,
                               const PRUnichar *someData)
{
  nsresult rv;
  if (!strcmp("chrome-flush-caches", aTopic)) {
    // the selected locale may have changed, reload the bundles
    rv = ReloadBundles();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (!strcmp("profile-change-teardown", aTopic)) {
    nsCOMPtr<nsIObserverService> obssvc =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = obssvc->RemoveObserver(this, "chrome-flush-caches");
    NS_ENSURE_SUCCESS(rv, rv);
    rv = obssvc->RemoveObserver(this, aTopic);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Public services.
//
//------------------------------------------------------------------------------

/**
 * Construct a Songbird string bundle service instance.
 */

sbStringBundleService::sbStringBundleService()
{
}


/**
 * Destroy a Songbird string bundle service instance.
 */

sbStringBundleService::~sbStringBundleService()
{
}


/**
 * Initialize the Songbird string bundle service.
 */
nsresult
sbStringBundleService::Initialize()
{
  nsresult rv;

  // Hook up the observer to be notified when the locale needs to change
  nsCOMPtr<nsIObserverService> obssvc =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = obssvc->AddObserver(this, "chrome-flush-caches", PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = obssvc->AddObserver(this, "profile-change-teardown", PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ReloadBundles();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Reload the bundles
 */
nsresult
sbStringBundleService::ReloadBundles()
{
  nsresult rv;

  // Get the base string bundle service.
  mBaseStringBundleService = do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the main Songbird string bundle.
  rv = CreateBundle("chrome://songbird/locale/songbird.properties",
                    getter_AddRefs(mBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the main Songbird string bundle.
  rv = CreateBundle("chrome://branding/locale/brand.properties",
                    getter_AddRefs(mBrandBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird string bundle class implementation.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Songbird string bundle nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(sbStringBundle,
                              nsIStringBundle)


//------------------------------------------------------------------------------
//
// Songbird string bundle nsIStringBundle implementation.
//
//------------------------------------------------------------------------------

//
// wstring GetStringFromID (in long aID);
//

NS_IMETHODIMP
sbStringBundle::GetStringFromID(PRInt32   aID,
                                PRUnichar **_retval)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(_retval);

  // Function variables.
  nsresult rv;

  // Get the string from the bundle list.
  nsAutoString bundleString;
  bool       bundleAvailable = PR_FALSE;
  PRUint32     bundleCount = mStringBundleList.Count();
  for (PRUint32 i = 0; i < bundleCount; i++) {
    nsCOMPtr<nsIStringBundle> stringBundle = mStringBundleList[i];
    rv = stringBundle->GetStringFromID(aID, getter_Copies(bundleString));
    if (NS_SUCCEEDED(rv)) {
      bundleAvailable = PR_TRUE;
      break;
    }
  }
  if (!bundleAvailable)
    return NS_ERROR_NOT_AVAILABLE;

  // Apply string substitutions.
  rv = ApplySubstitutions(bundleString);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  PRUnichar* prBundleString = ToNewUnicode(bundleString);
  NS_ENSURE_TRUE(prBundleString, NS_ERROR_OUT_OF_MEMORY);
  *_retval = prBundleString;

  return NS_OK;
}


//
// wstring GetStringFromName (in wstring aName);
//

NS_IMETHODIMP
sbStringBundle::GetStringFromName(const PRUnichar *aName,
                                  PRUnichar       **_retval)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aName);
  NS_ENSURE_ARG_POINTER(_retval);

  // Function variables.
  nsresult rv;

  // Get the string from the bundle list.
  nsAutoString bundleString;
  bool       bundleAvailable = PR_FALSE;
  PRUint32     bundleCount = mStringBundleList.Count();
  for (PRUint32 i = 0; i < bundleCount; i++) {
    nsCOMPtr<nsIStringBundle> stringBundle = mStringBundleList[i];
    rv = stringBundle->GetStringFromName(aName, getter_Copies(bundleString));
    if (NS_SUCCEEDED(rv)) {
      bundleAvailable = PR_TRUE;
      break;
    }
  }
  if (!bundleAvailable)
    return NS_ERROR_NOT_AVAILABLE;

  // Apply string substitutions.
  rv = ApplySubstitutions(bundleString);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  PRUnichar* prBundleString = ToNewUnicode(bundleString);
  NS_ENSURE_TRUE(prBundleString, NS_ERROR_OUT_OF_MEMORY);
  *_retval = prBundleString;

  return NS_OK;
}


//
// wstring formatStringFromID (in long aID,
//                             [array, size_is (length)] in wstring params,
//                             in unsigned long length);
//

NS_IMETHODIMP
sbStringBundle::FormatStringFromID(PRInt32         aID,
                                   const PRUnichar **params,
                                   PRUint32        length,
                                   PRUnichar       **_retval)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(_retval);

  // Function variables.
  nsresult rv;

  // Get the string from the bundle list.
  nsAutoString bundleString;
  bool       bundleAvailable = PR_FALSE;
  PRUint32     bundleCount = mStringBundleList.Count();
  for (PRUint32 i = 0; i < bundleCount; i++) {
    nsCOMPtr<nsIStringBundle> stringBundle = mStringBundleList[i];
    rv = stringBundle->FormatStringFromID(aID,
                                          params,
                                          length,
                                          getter_Copies(bundleString));
    if (NS_SUCCEEDED(rv)) {
      bundleAvailable = PR_TRUE;
      break;
    }
  }
  if (!bundleAvailable)
    return NS_ERROR_NOT_AVAILABLE;

  // Apply string substitutions.
  rv = ApplySubstitutions(bundleString);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  PRUnichar* prBundleString = ToNewUnicode(bundleString);
  NS_ENSURE_TRUE(prBundleString, NS_ERROR_OUT_OF_MEMORY);
  *_retval = prBundleString;

  return NS_OK;
}


//
// wstring formatStringFromName (in wstring aName,
//                               [array, size_is (length)] in wstring params,
//                               in unsigned long length);
//

NS_IMETHODIMP
sbStringBundle::FormatStringFromName(const PRUnichar *aName,
                                     const PRUnichar **params,
                                     PRUint32        length,
                                     PRUnichar       **_retval)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aName);
  NS_ENSURE_ARG_POINTER(_retval);

  // Function variables.
  nsresult rv;

  // Get the string from the bundle list.
  nsAutoString bundleString;
  bool       bundleAvailable = PR_FALSE;
  PRUint32     bundleCount = mStringBundleList.Count();
  for (PRUint32 i = 0; i < bundleCount; i++) {
    nsCOMPtr<nsIStringBundle> stringBundle = mStringBundleList[i];
    rv = stringBundle->FormatStringFromName(aName,
                                            params,
                                            length,
                                            getter_Copies(bundleString));
    if (NS_SUCCEEDED(rv)) {
      bundleAvailable = PR_TRUE;
      break;
    }
  }
  if (!bundleAvailable)
    return NS_ERROR_NOT_AVAILABLE;

  // Apply string substitutions.
  rv = ApplySubstitutions(bundleString);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  PRUnichar* prBundleString = ToNewUnicode(bundleString);
  NS_ENSURE_TRUE(prBundleString, NS_ERROR_OUT_OF_MEMORY);
  *_retval = prBundleString;

  return NS_OK;
}


//
// nsISimpleEnumerator getSimpleEnumeration ();
//

NS_IMETHODIMP
sbStringBundle::GetSimpleEnumeration(nsISimpleEnumerator **_retval)
{
  return mBaseStringBundle->GetSimpleEnumeration(_retval);
}


//------------------------------------------------------------------------------
//
// Public Songbird string bundle services.
//
//------------------------------------------------------------------------------

/**
 * Create a new instance of a Songbird string bundle using the base string
 * bundle specified by aBaseStringBundle.  Return the Songbird string bundle in
 * aStringBundle.
 *
 * \param aBaseStringBundle     Base string bundle.
 * \param aStringBundle         Returned created Songbird string bundle.
 */

/* static */ nsresult
sbStringBundle::New(nsIStringBundle* aBaseStringBundle,
                    sbStringBundle** aStringBundle)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aBaseStringBundle);
  NS_ENSURE_ARG_POINTER(aStringBundle);

  // Function variables.
  nsresult rv;

  // Create the instance.
  nsRefPtr<sbStringBundle> stringBundle = new sbStringBundle(aBaseStringBundle);
  NS_ENSURE_TRUE(stringBundle, NS_ERROR_OUT_OF_MEMORY);

  // Initialize the instance.
  rv = stringBundle->Initialize();
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  stringBundle.forget(aStringBundle);

  return NS_OK;
}


/**
 * Destroy a Songbird string bundle instance.
 */

sbStringBundle::~sbStringBundle()
{
}


//------------------------------------------------------------------------------
//
// Private Songbird string bundle services.
//
//------------------------------------------------------------------------------

/**
 * Construct a new instance of a Songbird string bundle using the base string
 * bundle specified by aBaseStringBundle.
 *
 * \param aBaseStringBundle     Base string bundle.
 */

sbStringBundle::sbStringBundle(nsIStringBundle* aBaseStringBundle) :
  mBaseStringBundle(aBaseStringBundle)
{
}


/**
 * Initialize the Songbird string bundle.
 */

nsresult
sbStringBundle::Initialize()
{
  nsresult rv;

  // Get the base string bundle service.
  mBaseStringBundleService = do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Load the string bundle.
  rv = LoadBundle(mBaseStringBundle);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Load the string bundle with the URL spec specified by aBundleURLSpec and all
 * of its included bundles.
 *
 * \param aBundleURLSpec        URL spec of bundle to load.
 */

nsresult
sbStringBundle::LoadBundle(nsAString& aBundleURLSpec)
{
  nsresult rv;

  // Create a new bundle.
  nsCOMPtr<nsIStringBundle> bundle;
  rv = mBaseStringBundleService->CreateBundle
         (NS_ConvertUTF16toUTF8(aBundleURLSpec).BeginReading(),
          getter_AddRefs(bundle));
  NS_ENSURE_SUCCESS(rv, rv);

  // Load the bundle.
  rv = LoadBundle(bundle);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Load the string bundle specified by aBundle and all of its included bundles.
 *
 * \param aBundle               Bundle to load.
 */

nsresult
sbStringBundle::LoadBundle(nsIStringBundle* aBundle)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aBundle);

  // Function variables.
  nsresult rv;

  // Add the string bundle to the list of string bundles.
  mStringBundleList.AppendObject(aBundle);

  // Get the list of included string bundles.
  nsTArray<nsString> includeBundleList;
  nsAutoString includeBundleListString;
  rv = aBundle->GetStringFromName
                  (NS_LITERAL_STRING("include_bundle_list").get(),
                   getter_Copies(includeBundleListString));
  if (NS_SUCCEEDED(rv)) {
    nsString_Split(includeBundleListString,
                   NS_LITERAL_STRING(","),
                   includeBundleList);
  }

  // Load each of the included string bundles.
  PRUint32 bundleCount = includeBundleList.Length();
  for (PRUint32 i = 0; i < bundleCount; i++) {
    rv = LoadBundle(includeBundleList[i]);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


/**
 * Apply any string bundle substitutions to the string specified by aString.
 *
 * \param aString               String to which to apply substitutions.
 */

nsresult
sbStringBundle::ApplySubstitutions(nsAString& aString)
{
  nsresult rv;

  // Apply embedded bundle strings.
  nsAutoString processedString(aString);
  PRInt32 subStartIndex = 0;
  PRInt32 subEndIndex = -1;
  while (1) {
    // Find the next embedded bundle string.
    PRInt32 subLength;
    subStartIndex = processedString.Find("&",
                                         static_cast<PRUint32>(subStartIndex));
    if (subStartIndex < 0)
      break;
    subEndIndex = processedString.Find(";",
                                       static_cast<PRUint32>(subStartIndex));
    if (subEndIndex < 0)
      break;
    subLength = subEndIndex + 1 - subStartIndex;

    // Get the bundle string name.
    nsAutoString subName(Substring(processedString,
                                   subStartIndex + 1,
                                   subLength - 2));

    // Get the bundle string, special casing "&amp;".
    nsAutoString subString;
    if (subName.Equals(NS_LITERAL_STRING("amp"))) {
      subString.Assign(NS_LITERAL_STRING("&"));
    } else {
      rv = GetStringFromName(subName.get(), getter_Copies(subString));
      if (NS_FAILED(rv))
        subString.Truncate();
    }

    // Apply the embedded bundle string.
    processedString.Replace(subStartIndex, subLength, subString);

    // Continue processing after the bundle string.
    subStartIndex += subString.Length();
  }

  // Return results.
  aString.Assign(processedString);

  return NS_OK;
}

