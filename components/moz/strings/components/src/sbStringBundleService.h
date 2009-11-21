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

#ifndef __SB_STRINGBUNDLESERVICE_H__
#define __SB_STRINGBUNDLESERVICE_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird string bundle service.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbStringBundleService.h
 * \brief Songbird String Bundle Service Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird string bundle service imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbIStringBundleService.h>

// Mozilla imports.
#include <nsCOMArray.h>
#include <nsCOMPtr.h>
#include <nsIObserver.h>
#include <nsIStringBundle.h>


//------------------------------------------------------------------------------
//
// Songbird string bundle service defs.
//
//------------------------------------------------------------------------------

//
// Songbird string bundle service component defs.
//

#define SB_STRINGBUNDLESERVICE_CLASSNAME "sbStringBundleService"
#define SB_STRINGBUNDLESERVICE_CID                                             \
  /* {04134658-9CF9-4FEA-90C0-BC159C016037} */                                 \
  {                                                                            \
    0x04134658,                                                                \
    0x9CF9,                                                                    \
    0x4FEA,                                                                    \
    { 0x90, 0xC0, 0xBC, 0x15, 0x9C, 0x01, 0x60, 0x37 }                         \
  }


//------------------------------------------------------------------------------
//
// Songbird string bundle service classes.
//
//------------------------------------------------------------------------------

/**
 * This class implements the Songbird string bundle service component.  This
 * service is provided so that Songbird string bundles can be created.
 */

class sbStringBundleService : public nsIStringBundleService,
                              public nsIObserver,
                              public sbIStringBundleService
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // Inherited interfaces.
  //

  NS_DECL_ISUPPORTS
  NS_DECL_NSISTRINGBUNDLESERVICE
  NS_DECL_NSIOBSERVER
  NS_DECL_SBISTRINGBUNDLESERVICE


  //
  // Public services.
  //

  sbStringBundleService();

  virtual ~sbStringBundleService();

  nsresult Initialize();
  nsresult ReloadBundles();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mBaseStringBundleService   Base string bundle service.
  // mBundle                    Main Songbird string bundle.
  // mBrandBundle               Main Songbird brand string bundle.
  //

  nsCOMPtr<nsIStringBundleService>
                                mBaseStringBundleService;
  nsCOMPtr<nsIStringBundle>     mBundle;
  nsCOMPtr<nsIStringBundle>     mBrandBundle;
};


/**
 * This class implements the Songbird string bundle.
 *
 *   The Songbird string bundle provides an enhanced set of string bundle
 * services.  In particular, this class allows string bundles to include other
 * string bundles and to include bundle strings in other bundle strings.
 *   To include one or more string bundles in a top-level string bundle, define
 * the string "include_bundle_list" in the top-level bundle.  This string should
 * consist of a comma separated list of included string bundle URI's.  When
 * the Songbird string bundle object looks up a string, it will look in the
 * top-level string bundle and all included string bundles.  The included string
 * bundles can include additional string bundles too.
 *   Note that circular string bundle includes will lead to errors or hangs.
 *   To include a bundle string in another bundle string, encapsulate the
 * included bundle string in "&" and ";", much like XML entities.  Use "&amp;"
 * for a literal "&".
 *
 * Example:
 *
 * include_bundle_list=chrome://bundles1.properties,chrome://bundle2.properties
 *
 * string1=World
 * string2=Hello &string1; &amp; Everyone Else
 *
 * string2 evaluates to "Hello World & Everyone Else".
 */

class sbStringBundle: public nsIStringBundle
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // Inherited interfaces.
  //

  NS_DECL_ISUPPORTS
  NS_DECL_NSISTRINGBUNDLE


  //
  // Public services.
  //

  static nsresult New(nsIStringBundle* aBaseStringBundle,
                      sbStringBundle** aStringBundle);

  virtual ~sbStringBundle();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mBaseStringBundleService   Base string bundle service.
  // mBaseStringBundle          Base string bundle.
  // mStringBundleList          List of loaded string bundles.
  //

  nsCOMPtr<nsIStringBundleService>
                                mBaseStringBundleService;
  nsCOMPtr<nsIStringBundle>     mBaseStringBundle;
  nsCOMArray<nsIStringBundle>   mStringBundleList;


  //
  // Private services.
  //

  sbStringBundle(nsIStringBundle* aBaseStringBundle);

  nsresult Initialize();

  nsresult LoadBundle(nsAString& aBundleURLSpec);

  nsresult LoadBundle(nsIStringBundle* aBundle);

  nsresult ApplySubstitutions(nsAString& aString);
};


#endif // __SB_STRINGBUNDLESERVICE_H__

