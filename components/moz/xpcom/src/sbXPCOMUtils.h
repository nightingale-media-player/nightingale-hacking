/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#ifndef SB_XPCOM_UTILS_H_
#define SB_XPCOM_UTILS_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird XPCOM utilities defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbXPCOMUtils.h
 * \brief Songbird XPCOM Utilities Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird XPCOM utilities imported services.
//
//------------------------------------------------------------------------------

// Mozilla imports.
#include <nsICategoryManager.h>
#include <nsISupportsPrimitives.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>


//------------------------------------------------------------------------------
//
// Songbird XPCOM utilities services.
//
//------------------------------------------------------------------------------

/**
 * Return in aEntryValueList the list of entry values for the category specified
 * by aCategory.
 *
 * \param aCategory             Category for which to get entry values.
 * \param aEntryValueList       List of category entry values.
 */

static inline nsresult
SB_GetCategoryEntryValues(const char*          aCategory,
                          nsTArray<nsCString>& aEntryValueList)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aCategory);

  // Function variables.
  nsresult rv;

  // Get the category manager.
  nsCOMPtr<nsICategoryManager>
    categoryManager = do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the category entry enumerator.
  nsCOMPtr<nsISimpleEnumerator> enumerator;
  rv = categoryManager->EnumerateCategory(aCategory,
                                          getter_AddRefs(enumerator));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the value of each entry.
  bool hasMore;
  rv = enumerator->HasMoreElements(&hasMore);
  NS_ENSURE_SUCCESS(rv, rv);
  while (hasMore) {
    // Get the next entry name.
    nsCString             entryName;
    nsCOMPtr<nsISupports> entryNameSupports;
    rv = enumerator->GetNext(getter_AddRefs(entryNameSupports));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsISupportsCString>
      entryNameSupportsCString = do_QueryInterface(entryNameSupports, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = entryNameSupportsCString->GetData(entryName);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the entry value.
    nsCString entryValue;
    rv = categoryManager->GetCategoryEntry(aCategory,
                                           entryName.get(),
                                           getter_Copies(entryValue));
    NS_ENSURE_SUCCESS(rv, rv);

    // Add the entry value to the value list.
    nsCString* appendedElement = aEntryValueList.AppendElement(entryValue);
    NS_ENSURE_TRUE(appendedElement, NS_ERROR_OUT_OF_MEMORY);

    // Check for more entries.
    rv = enumerator->HasMoreElements(&hasMore);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


#endif // SB_XPCOM_UTILS_H_

