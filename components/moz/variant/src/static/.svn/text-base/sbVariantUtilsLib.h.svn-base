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

#ifndef __SB_VARIANT_UTILS_LIB_H__
#define __SB_VARIANT_UTILS_LIB_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Variant utility defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbVariantUtilsLib.h
 * \brief Songbird Variant Utility Definitions.
 */

//------------------------------------------------------------------------------
//
// Variant utility imported services.
//
//------------------------------------------------------------------------------

// Mozilla imports.
#include <nsCOMPtr.h>
#include <nsIVariant.h>


//------------------------------------------------------------------------------
//
// Variant utility inline functions.
//
//------------------------------------------------------------------------------

/**
 * Return a QI'd object for the contents of the variant specified by aVariant.
 * This function implements do_QueryInterface for the contents of the variant.
 *
 * \param aVariant              Variant for which to get QI'd object.
 *
 * \return                      QI'd variant content.
 */

inline nsQueryInterface
do_VariantQueryInterface(nsIVariant* aVariant)
{
  nsresult rv;

  nsCOMPtr<nsISupports> variantSupports;
  rv = aVariant->GetAsISupports(getter_AddRefs(variantSupports));
  if (NS_FAILED(rv))
    return nsQueryInterface(nsnull);

  return nsQueryInterface(variantSupports);
}


/**
 * Return a QI'd object for the contents of the variant specified by aVariant
 * and return the error result in aError.  This function implements
 * do_QueryInterface for the contents of the variant.
 *
 * \param aVariant              Variant for which to get QI'd object.
 * \param aError                Returned error result.
 *
 * \return                      QI'd variant content.
 */

inline nsQueryInterfaceWithError
do_VariantQueryInterface(nsIVariant* aVariant, nsresult* aError)
{
  nsresult rv;

  nsCOMPtr<nsISupports> variantSupports;
  rv = aVariant->GetAsISupports(getter_AddRefs(variantSupports));
  if (NS_FAILED(rv)) {
    *aError = rv;
    return nsQueryInterfaceWithError(nsnull, nsnull);
  }

  return nsQueryInterfaceWithError(variantSupports, aError);
}


//------------------------------------------------------------------------------
//
// Variant utility prototypes.
//
//------------------------------------------------------------------------------

/**
 *   Return true in aEqual if the variant specified by aVariant2 is equal to the
 * variant specified by aVariant1; otherwise, return false.  For the comparison,
 * aVariant2 is converted to match the data type of aVariant1.
 *   If aVariant1 is an integer, and aVariant2 is a string of the form "0xde10",
 * aVariant2 will be converted as a hexadecimal.
 *
 * \param aVariant1             Variants to compare.
 * \param aVariant2
 * \param aEqual                Returned true if variants are equal.
 */

nsresult sbVariantsEqual(nsIVariant* aVariant1,
                         nsIVariant* aVariant2,
                         PRBool*     aEqual);

#endif /* __SB_VARIANT_UTILS_LIB_H__ */

