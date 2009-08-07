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

/**
 * \file  sbLocalDatabaseDynamicMediaList.cpp
 * \brief Songbird Local Database Dynamic Media List Source.
 */

//------------------------------------------------------------------------------
//
// Songbird dynamic media list imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbLocalDatabaseDynamicMediaList.h"

// Local imports.
#include "sbLocalDatabaseCID.h"

// Songbird imports.
#include <sbIDynamicPlaylistService.h>
#include <sbIMediaListFactory.h>
#include <sbStandardProperties.h>

// Mozilla imports.
#include <nsAutoPtr.h>
#include <nsIClassInfoImpl.h>
#include <nsIProgrammingLanguage.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>


//------------------------------------------------------------------------------
//
// Songbird dynamic media list logging services.
//
//------------------------------------------------------------------------------

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbLocalDatabaseDynamicMediaList:5
 */

#ifdef PR_LOGGING
static PRLogModuleInfo* gLocalDatabaseDynamicMediaListLog = nsnull;
#define TRACE(args) PR_LOG(gLocalDatabaseDynamicMediaListLog,                  \
                           PR_LOG_DEBUG,                                       \
                           args)
#define LOG(args)   PR_LOG(gLocalDatabaseDynamicMediaListLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif


//------------------------------------------------------------------------------
//
// Songbird dynamic media list nsISupports and nsIClassInfo implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS7(sbLocalDatabaseDynamicMediaList,
                              nsIClassInfo,
                              nsISupportsWeakReference,
                              sbILibraryResource,
                              sbIMediaItem,
                              sbIMediaList,
                              sbIDynamicMediaList,
                              sbILocalDatabaseMediaItem)

NS_IMPL_CI_INTERFACE_GETTER6(sbLocalDatabaseDynamicMediaList,
                             nsIClassInfo,
                             nsISupportsWeakReference,
                             sbILibraryResource,
                             sbIMediaItem,
                             sbIMediaList,
                             sbIDynamicMediaList)

// nsIClassInfo implementation.
NS_IMPL_THREADSAFE_CI(sbLocalDatabaseDynamicMediaList)


//------------------------------------------------------------------------------
//
// Songbird dynamic media list sbIDynamicMediaList implementation.
//
//------------------------------------------------------------------------------

/**
 * Force an update of the dynamic media list.
 */

NS_IMETHODIMP
sbLocalDatabaseDynamicMediaList::Update()
{
  // Trace execution.
  TRACE(("sbLocalDatabaseDynamicMediaList[0x%p] - Update()", this));

  // Function variables.
  nsresult rv;

  // Update the list.
  nsCOMPtr<sbIDynamicPlaylistService> dynamicPlaylistService =
    do_GetService("@songbirdnest.com/Songbird/Library/DynamicPlaylistService;1",
                  &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = dynamicPlaylistService->UpdateNow(this);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Songbird dynamic media list sbIMediaList implementation.
//
//------------------------------------------------------------------------------

/**
 * \brief The type of media list (eg "simple")
 */

NS_IMETHODIMP
sbLocalDatabaseDynamicMediaList::GetType(nsAString& aType)
{
  aType.Assign(NS_LITERAL_STRING("dynamic"));
  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Public Songbird dynamic media list services.
//
//------------------------------------------------------------------------------

/**
 * Create and initialize a dynamic media list for the inner media item specified
 * by aInner.  Return the created dynamic media list in aMediaList.
 *
 * \param aInner                Media list inner media item.
 * \param aMediaList            Returned created dynamic media list.
 */

/* static */ nsresult
sbLocalDatabaseDynamicMediaList::New(sbIMediaItem*  aInner,
                                     sbIMediaList** aMediaList)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aInner);
  NS_ENSURE_ARG_POINTER(aMediaList);

  // Function variables.
  nsresult rv;

  // Create and initialize a dynamic media list.
  nsRefPtr<sbLocalDatabaseDynamicMediaList>
    dynamicMediaList = new sbLocalDatabaseDynamicMediaList();
  NS_ENSURE_TRUE(dynamicMediaList, NS_ERROR_OUT_OF_MEMORY);
  rv = dynamicMediaList->Initialize(aInner);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  rv = CallQueryInterface(dynamicMediaList.get(), aMediaList);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Destroy a dynamic media list object.
 */

sbLocalDatabaseDynamicMediaList::~sbLocalDatabaseDynamicMediaList()
{
}


//------------------------------------------------------------------------------
//
// Private Songbird dynamic media list services.
//
//------------------------------------------------------------------------------

/**
 * Create a dynamic media list object.
 */

sbLocalDatabaseDynamicMediaList::sbLocalDatabaseDynamicMediaList()
{
  // Initialize the logging services.
#ifdef PR_LOGGING
  if (!gLocalDatabaseDynamicMediaListLog) {
    gLocalDatabaseDynamicMediaListLog =
      PR_NewLogModule("sbLocalDatabaseDynamicMediaList");
  }
#endif
}


/**
 * Initialize the dynamic media list using the inner media item specified by
 * aInner.
 *
 * \param aInner                Media list inner media item.
 */

nsresult
sbLocalDatabaseDynamicMediaList::Initialize(sbIMediaItem* aInner)
{
  // Trace execution.
  TRACE(("sbLocalDatabaseDynamicMediaList[0x%p] - Initialize(0x%p)",
         this,
         aInner));

  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aInner);

  // Function variables.
  nsresult rv;

  // Create a base simple media list.
  nsCOMPtr<sbIMediaListFactory> simpleMediaListFactory =
    do_GetService(SB_LOCALDATABASE_SIMPLEMEDIALISTFACTORY_CONTRACTID,
                  &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = simpleMediaListFactory->CreateMediaList(aInner,
                                               getter_AddRefs(mBaseMediaList));
  NS_ENSURE_SUCCESS(rv, rv);
  mBaseLocalDatabaseMediaList = do_QueryInterface(mBaseMediaList, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set some dynamic media list properties.
  nsAutoString customType;
  mBaseMediaList->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CUSTOMTYPE),
                              customType);
  if (customType.IsEmpty()) {
    rv = mBaseMediaList->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_CUSTOMTYPE),
                                     NS_LITERAL_STRING("dynamic"));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  rv = mBaseMediaList->SetProperty
                         (NS_LITERAL_STRING(SB_PROPERTY_ISSUBSCRIPTION),
                          NS_LITERAL_STRING("1"));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

