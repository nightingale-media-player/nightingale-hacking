/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

#ifndef __SB_STRINGBUNDLE_H__
#define __SB_STRINGBUNDLE_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Nightingale string bundle services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/** 
 * \file  sbStringBundle.h
 * \brief Nightingale String Bundle Definitions.
 */

// Nightingale imports.
#include <sbIStringBundleService.h>
#include <sbStringUtils.h>

// Mozilla imports.
#include <nsCOMArray.h>
#include <nsCOMPtr.h>
#include <nsIStringBundle.h>


/**
 *   This class implements the Nightingale string bundle object.
 *   This class provides support for including string bundles within string
 * bundles.  A string bundle may specify a list of included string bundles by
 * setting the string "include_bundle_list" to a comma separated list of string
 * bundle URI specs.  When looking up a string, this class will search the
 * string bundle as well as all of its included string bundles and their
 * included string bundles.
 *   This class also provides support for string substitutions.  A string may be
 * substituted within another string by specifying its key as "&<key>;".  A
 * literal "&" may be specified as "&amp;".
 *
 * Included string bundle example:
 *
 *   include_bundle_list=chrome://branding/locale/brand.properties,chrome://nightingale/locale/other.properties
 *
 * String substitution example:
 *
 *   brandShortName=Acme
 *   rocket_skates.eula=&brandShortName; is not responsible for any damage &amp; injury caused by our Rocket Skates.
 */

class sbStringBundle
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // Constructors/destructors.
  //

  /**
   *   Construct a Nightingale string bundle with the string bundle URI specified
   * by aURI.  If aURI is not specified, use the default Nightingale string bundle.
   *   Upon any failure, ensure that use of the Nightingale string bundle is safe.
   *
   * \param aURI                  URI of string bundle.
   */

  sbStringBundle(const char* aURI = nsnull);


  /**
   *   Construct a Nightingale string bundle with the string bundle specified by
   * aBundle.
   */

  sbStringBundle(nsIStringBundle* aBundle);


  //
  // Public services.
  //

  /**
   *   Get and return the string with the bundle key specified by aKey.  If the
   * string cannot be found, return the string specified by aDefault; if
   * aDefault is not specified, return aKey.
   *   Upon any failure, ensure that use of the Nightingale string bundle is safe.
   *
   * \param aKey                  String bundle key.
   * \param aDefault              Default string value.
   *
   * \return                      Bundle string.
   */

  nsString Get(const nsAString& aKey,
               const nsAString& aDefault = SBVoidString());

  nsString Get(const char* aKey,
               const char* aDefault = nsnull);


  /**
   *   Get and return the formatted string with the bundle key specified by aKey
   * and the parameters specified by aParams.  If the string cannot be found,
   * return the string specified by aDefault; if aDefault is not specified,
   * return aKey.
   *
   * \param aKey                  String bundle key.
   * \param aParams               Bundle string format parameters.
   * \param aDefault              Default string value.
   *
   * \return                      Bundle string.
   */

  nsString Format(const nsAString&    aKey,
                  nsTArray<nsString>& aParams,
                  const nsAString&    aDefault = SBVoidString());

  nsString Format(const char*         aKey,
                  nsTArray<nsString>& aParams,
                  const char*         aDefault = nsnull);

  nsString Format(const nsAString&    aKey,
                  const nsAString&    aParam,
                  const nsAString&    aDefault = SBVoidString());

  nsString Format(const char*         aKey,
                  const nsAString&    aParam,
                  const char*         aDefault = nsnull);


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mStringBundleService       Nightingale string bundle service.
  // mBundleList                List of string bundles.
  //

  nsCOMPtr<sbIStringBundleService>
                                mStringBundleService;
  nsCOMArray<nsIStringBundle>   mBundleList;


  //
  // Internal services.
  //

  nsresult LoadBundle(const char* aURI);

  nsresult LoadBundle(nsIStringBundle* aBundle);

  nsresult ApplySubstitutions(nsAString& aString);
};


#endif // __SB_STRINGBUNDLE_H__

