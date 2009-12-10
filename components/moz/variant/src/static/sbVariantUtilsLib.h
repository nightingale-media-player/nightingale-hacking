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
#include <nsIVariant.h>


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

