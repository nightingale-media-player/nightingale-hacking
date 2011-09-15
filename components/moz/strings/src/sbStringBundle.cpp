/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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
// Songbird string bundle services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbStringBundle.cpp
 * \brief Songbird String Bundle Source.
 */


//------------------------------------------------------------------------------
//
// Songbird string bundle imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbStringBundle.h"

// Songbird imports.
#include <sbIStringBundleService.h>

// Mozilla imports.
#include <nsIStringBundle.h>
#include <nsServiceManagerUtils.h>
#include <prlog.h>

/**
 * logging
 * 
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbStringBundle:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gStringBundleLog = nsnull;
#define TRACE(args) PR_LOG(gStringBundleLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gStringBundleLog, PR_LOG_WARN, args)
#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif /* __GNUC__ */
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */


//------------------------------------------------------------------------------
//
// Songbird string bundle constructors/destructors.
//
//------------------------------------------------------------------------------

/**
 *   Construct a Songbird string bundle with the string bundle URI specified by
 * aURI.  If aURI is not specified, use the default Songbird string bundle.
 *   Upon any failure, ensure that use of the Songbird string bundle is safe.
 *
 * \param aURI                  URI of string bundle.
 */

sbStringBundle::sbStringBundle(const char* aURI)
{
  nsresult rv;
  
#ifdef PR_LOGGING
  if (!gStringBundleLog) {
    gStringBundleLog = PR_NewLogModule("sbStringBundle");
  }
#endif

  TRACE(("%s[%8.x] - %s", __FUNCTION__, this, aURI));

  // Get the Songbird string bundle service.
  mStringBundleService = do_GetService(SB_STRINGBUNDLESERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, /* void */);

  // Load the string bundle.  If no string bundle is specified, load the default
  // one.
  if (aURI) {
    rv = LoadBundle(aURI);
    NS_ENSURE_SUCCESS(rv, /* void */);
  } else {
    nsCOMPtr<nsIStringBundle> defaultBundle;
    rv = mStringBundleService->GetBundle(getter_AddRefs(defaultBundle));
    NS_ENSURE_SUCCESS(rv, /* void */);
    rv = LoadBundle(defaultBundle);
    NS_ENSURE_SUCCESS(rv, /* void */);
  }
}


/**
 *   Construct a Songbird string bundle with the string bundle specified by
 * aBundle.
 *   Upon any failure, ensure that use of the Songbird string bundle is safe.
 *
 * \param aBundle               String bundle object.
 */

sbStringBundle::sbStringBundle(nsIStringBundle* aBundle)
{
  nsresult rv;

#ifdef PR_LOGGING
  if (!gStringBundleLog) {
    gStringBundleLog = PR_NewLogModule("sbStringBundle");
  }
#endif

  TRACE(("%s[%8.x] - (existing bundle %.08x)", __FUNCTION__, this, aBundle));

  // Get the Songbird string bundle service.
  mStringBundleService = do_GetService(SB_STRINGBUNDLESERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, /* void */);

  // Load the string bundle.
  rv = LoadBundle(aBundle);
  NS_ENSURE_SUCCESS(rv, /* void */);
}


//------------------------------------------------------------------------------
//
// Public Songbird string bundle services.
//
//------------------------------------------------------------------------------

/**
 *   Get and return the string with the bundle key specified by aKey.  If the
 * string cannot be found, return the string specified by aDefault; if aDefault
 * is not specified, return aKey.
 *
 * \param aKey                  String bundle key.
 * \param aDefault              Default string value.
 *
 * \return                      Bundle string.
 */

nsString
sbStringBundle::Get(const nsAString& aKey,
                    const nsAString& aDefault)
{
  nsString stringValue;
  nsresult rv;
  
  TRACE(("%s[%8.x] - %s (default %s)",
         __FUNCTION__,
         this,
         NS_ConvertUTF16toUTF8(aKey).get(),
         NS_ConvertUTF16toUTF8(aDefault).get()));

  // Get the default string value.
  if (!aDefault.IsVoid())
    stringValue = aDefault;
  else
    stringValue = aKey;

  // Get the string from the bundle list.
  nsString bundleString;
  PRInt32 bundleCount = mBundleList.Count();
  for (PRInt32 i = 0; i < bundleCount; i++) {
    rv = mBundleList[i]->GetStringFromName(aKey.BeginReading(),
                                           getter_Copies(bundleString));
    if (NS_SUCCEEDED(rv)) {
      stringValue = bundleString;
      break;
    }
  }

  // Apply string substitutions.
  ApplySubstitutions(stringValue);

  return stringValue;
}

nsString
sbStringBundle::Get(const char* aKey,
                    const char* aDefault)
{
  TRACE(("%s[%8.x] - %s (default %s)", __FUNCTION__, this, aKey, aDefault));

  // Convert the key.
  nsAutoString key;
  if (aKey)
    key = NS_ConvertUTF8toUTF16(aKey);
  else
    key = SBVoidString();

  // Convert the default string.
  nsAutoString defaultString;
  if (aDefault)
    defaultString = NS_ConvertUTF8toUTF16(aDefault);
  else
    defaultString = SBVoidString();

  return Get(key, defaultString);
}


/**
 *   Get and return the formatted string with the bundle key specified by aKey
 * and the parameters specified by aParams.  If the string cannot be found,
 * return the string specified by aDefault; if aDefault is not specified, return
 * aKey.
 *
 * \param aKey                  String bundle key.
 * \param aParams               Bundle string format parameters.
 * \param aDefault              Default string value.
 *
 * \return                      Bundle string.
 */

nsString
sbStringBundle::Format(const nsAString&    aKey,
                       nsTArray<nsString>& aParams,
                       const nsAString&    aDefault)
{
  nsString stringValue;
  nsresult rv;

  TRACE(("%s[%8.x] - %s (has params) (default %s)",
         __FUNCTION__,
         this,
         NS_ConvertUTF16toUTF8(aKey).get(),
         NS_ConvertUTF16toUTF8(aDefault).get()));

  // Get the default string value.
  if (!aDefault.IsVoid())
    stringValue = aDefault;
  else
    stringValue = aKey;

  // Convert format parameters.
  nsTArray<const PRUnichar*> params;
  PRUint32 paramCount = aParams.Length();
  for (PRUint32 i = 0; i < paramCount; i++) {
    params.AppendElement(aParams[i].get());
  }

  // Get the string from the bundle list.
  nsString bundleString;
  PRInt32 bundleCount = mBundleList.Count();
  for (PRInt32 i = 0; i < bundleCount; i++) {
    rv = mBundleList[i]->FormatStringFromName(aKey.BeginReading(),
                                              params.Elements(),
                                              paramCount,
                                              getter_Copies(bundleString));
    if (NS_SUCCEEDED(rv)) {
      stringValue = bundleString;
      break;
    }
  }

  // Apply string substitutions.
  ApplySubstitutions(stringValue);

  return stringValue;
}

nsString
sbStringBundle::Format(const char*         aKey,
                       nsTArray<nsString>& aParams,
                       const char*         aDefault)
{
  TRACE(("%s[%8.x] - %s (has params) (default %s)",
         __FUNCTION__,
         this,
         aKey,
         aDefault));

  // Convert the key.
  nsAutoString key;
  if (aKey)
    key = NS_ConvertUTF8toUTF16(aKey);
  else
    key = SBVoidString();

  // Convert the default string.
  nsAutoString defaultString;
  if (aDefault)
    defaultString = NS_ConvertUTF8toUTF16(aDefault);
  else
    defaultString = SBVoidString();

  return Format(key, aParams, defaultString);
}

nsString
sbStringBundle::Format(const nsAString&    aKey,
                       const nsAString&    aParam,
                       const nsAString&    aDefault)
{
  TRACE(("%s[%8.x] - %s (param %s) (default %s)",
         __FUNCTION__,
         this,
         NS_ConvertUTF16toUTF8(aKey).get(),
         NS_ConvertUTF16toUTF8(aParam).get(),
         NS_ConvertUTF16toUTF8(aDefault).get()));

  nsTArray<nsString> params(1);
  params.AppendElement(aParam);
  return Format(aKey, params, aDefault);
}

nsString
sbStringBundle::Format(const char*         aKey,
                       const nsAString&    aParam,
                       const char*         aDefault)
{
  TRACE(("%s[%8.x] - %s (param %s) (default %s)",
         __FUNCTION__,
         this,
         aKey,
         NS_ConvertUTF16toUTF8(aParam).BeginReading(),
         aDefault));

  nsTArray<nsString> params(1);
  params.AppendElement(aParam);
  return Format(aKey, params, aDefault);
}

//------------------------------------------------------------------------------
//
// Internal Songbird string bundle services.
//
//------------------------------------------------------------------------------

/**
 *   Load the string bundle with the URI specified by aURI as well as any string
 * bundles included by the specified string bundle.
 *
 * \param aURI                  URI spec of string bundle to load.
 */

nsresult
sbStringBundle::LoadBundle(const char* aURI)
{
  nsresult rv;
  
  TRACE(("%s[%8.x] - loading bundle %s", __FUNCTION__, this, aURI));

  // Create the string bundle.
  nsCOMPtr<nsIStringBundle> bundle;
  nsCOMPtr<nsIStringBundleService>
    stringBundleService = do_QueryInterface(mStringBundleService, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stringBundleService->CreateBundle(aURI, getter_AddRefs(bundle));
  NS_ENSURE_SUCCESS(rv, rv);

  // Load the string bundle.
  rv = LoadBundle(bundle);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 *   Load the string bundle specified by aBundle as well as any string bundles
 * included by the specified string bundle.
 *
 * \param aBundle               String bundle to load.
 */

nsresult
sbStringBundle::LoadBundle(nsIStringBundle* aBundle)
{
  nsresult rv;

  TRACE(("%s[%8.x] - existing bundle [%8.x]", __FUNCTION__, this, aBundle));

  // Add the string bundle to the list of string bundles.
  mBundleList.AppendObject(aBundle);

  // Get the list of included string bundles.
  nsString includeBundleListString;
  rv = aBundle->GetStringFromName
                  (NS_LITERAL_STRING("include_bundle_list").get(),
                   getter_Copies(includeBundleListString));
  if (NS_FAILED(rv))
    return NS_OK;

  // Load each of the included string bundles.
  nsTArray<nsString> includeBundleList;
  nsString_Split(includeBundleListString,
                 NS_LITERAL_STRING(","),
                 includeBundleList);
  PRUint32 includeBundleCount = includeBundleList.Length();
  for (PRUint32 i = 0; i < includeBundleCount; i++) {
    rv = LoadBundle(NS_ConvertUTF16toUTF8(includeBundleList[i]).get());
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


/*
 * Apply and string bundle substitutions to the string specified by aString.
 *
 * \param aString               String to which to apply substitutions.
 */

nsresult
sbStringBundle::ApplySubstitutions(nsAString& aString)
{
  TRACE(("%s[%8.x] - applying %s",
         __FUNCTION__,
         this,
         NS_ConvertUTF16toUTF8(aString).get()));
  // Apply all string substitutions.
  PRInt32 currentOffset = 0;
  do {
    // Get the starting and ending index of the next string substitution.
    PRInt32 subStartIndex = aString.Find(NS_LITERAL_STRING("&"), currentOffset);
    if (subStartIndex < 0)
      break;
    PRInt32 subEndIndex = aString.Find(NS_LITERAL_STRING(";"),
                                       subStartIndex + 1);
    if (subEndIndex < 0)
      break;

    // Get the substitution string key.
    PRInt32 subKeyStartIndex = subStartIndex + 1;
    PRInt32 subKeyLength = subEndIndex - subKeyStartIndex;
    nsAutoString subKey;
    subKey.Assign(nsDependentSubstring(aString,
                                       subKeyStartIndex,
                                       subKeyLength));

    // Get the substitution string, special-casing "&amp;".
    nsAutoString subString;
    if (subKey.EqualsLiteral("amp")) {
      subString = NS_LITERAL_STRING("&");
    } else {
      subString = Get(subKey, NS_LITERAL_STRING(""));
    }

    // Apply the substitution.
    PRInt32 subLength = subEndIndex - subStartIndex + 1;
    aString.Replace(subStartIndex, subLength, subString);

    // Advance past the substitution.
    currentOffset = subStartIndex + subString.Length();
  } while (1);

  return NS_OK;
}

