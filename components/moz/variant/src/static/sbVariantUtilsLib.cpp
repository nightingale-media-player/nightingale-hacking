/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Nightingale variant utilities.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbVariantUtilsLib.cpp
 * \brief Nightingale Variant Utilities Source.
 */

//------------------------------------------------------------------------------
//
// Nightingale variant utilities imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbVariantUtilsLib.h"

// Mozilla imports.
#include <nsCOMPtr.h>
#include <nsStringGlue.h>

// C++ STL imports.
#include <iomanip>
#include <sstream>


//------------------------------------------------------------------------------
//
// Internal Nightingale variant utilities.
//
//------------------------------------------------------------------------------

/**
 * Convert the non-string variant specified by aVariant to an integer and return
 * the result in aInt.
 *
 * \param aVariant              Variant to convert.
 * \param aInt                  Returned integer value.
 */

static nsresult
sbNonStringVariantToInt(nsIVariant* aVariant,
                        PRInt64*    aInt)
{
  NS_ENSURE_ARG_POINTER(aVariant);
  NS_ENSURE_ARG_POINTER(aInt);
  nsresult rv = aVariant->GetAsInt64(aInt);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

static nsresult
sbNonStringVariantToInt(nsIVariant* aVariant,
                        PRUint64*   aInt)
{
  NS_ENSURE_ARG_POINTER(aVariant);
  NS_ENSURE_ARG_POINTER(aInt);
  nsresult rv = aVariant->GetAsUint64(aInt);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}


/**
 * Convert the variant specified by aVariant to an integer and return the result
 * in aInt.  If aVariant is a string and of the form "0xde10", it will be
 * converted as a hexadecimal.
 *
 * \param aVariant              Variant to convert.
 * \param aInt                  Returned integer value.
 */

template <class T>
static nsresult
sbVariantToInt(nsIVariant* aVariant,
               T*          aInt)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aVariant);
  NS_ENSURE_ARG_POINTER(aInt);

  // Function variables.
  nsresult rv;

  // Convert the variant.
  PRUint16 dataType;
  rv = aVariant->GetDataType(&dataType);
  NS_ENSURE_SUCCESS(rv, rv);
  switch (dataType) {
    case nsIDataType::VTYPE_DOMSTRING :
    case nsIDataType::VTYPE_CHAR_STR :
    case nsIDataType::VTYPE_WCHAR_STR :
    case nsIDataType::VTYPE_UTF8STRING :
    case nsIDataType::VTYPE_CSTRING :
    case nsIDataType::VTYPE_ASTRING :
    case nsIDataType::VTYPE_STRING_SIZE_IS :
    case nsIDataType::VTYPE_WSTRING_SIZE_IS :
    {
      // Convert from string with support for hexadecimal strings.
      nsCAutoString variantString;
      rv = aVariant->GetAsACString(variantString);
      NS_ENSURE_SUCCESS(rv, rv);
      std::string s(variantString.get());
      std::istringstream iss(s);
      iss >> std::setbase(0) >> *aInt;
      break;
    }

    default :
      rv = sbNonStringVariantToInt(aVariant, aInt);
      NS_ENSURE_SUCCESS(rv, rv);
      break;
  }

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Nightingale variant utilities.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// sbVariantsEqual
//

nsresult
sbVariantsEqual(nsIVariant* aVariant1,
                nsIVariant* aVariant2,
                PRBool*     aEqual)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aEqual);

  // Function variables.
  nsresult rv;

  // If either variant is null, they're only equal if they're both null.
  if (!aVariant1 || !aVariant2) {
    *aEqual = (!aVariant1 && !aVariant2);
    return NS_OK;
  }

  // Get the variant data types.
  PRUint16 dataType1;
  PRUint16 dataType2;
  rv = aVariant1->GetDataType(&dataType1);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aVariant2->GetDataType(&dataType2);
  NS_ENSURE_SUCCESS(rv, rv);

  // Compare the variants.
  switch (dataType1) {
    case nsIDataType::VTYPE_INT8 :
    case nsIDataType::VTYPE_INT16 :
    case nsIDataType::VTYPE_INT32 :
    case nsIDataType::VTYPE_INT64 :
    case nsIDataType::VTYPE_UINT8 :
    case nsIDataType::VTYPE_UINT16 :
    case nsIDataType::VTYPE_UINT32 :
    {
      PRInt64 value1;
      PRInt64 value2;
      rv = aVariant1->GetAsInt64(&value1);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = sbVariantToInt(aVariant2, &value2);
      NS_ENSURE_SUCCESS(rv, rv);
      *aEqual = (value1 == value2);
      break;
    }

    case nsIDataType::VTYPE_UINT64 :
    {
      PRUint64 value1;
      PRUint64 value2;
      rv = aVariant1->GetAsUint64(&value1);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = sbVariantToInt(aVariant2, &value2);
      NS_ENSURE_SUCCESS(rv, rv);
      *aEqual = (value1 == value2);
      break;
    }

    case nsIDataType::VTYPE_FLOAT :
    case nsIDataType::VTYPE_DOUBLE :
    {
      double value1;
      double value2;
      rv = aVariant1->GetAsDouble(&value1);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aVariant2->GetAsDouble(&value2);
      NS_ENSURE_SUCCESS(rv, rv);
      *aEqual = (value1 == value2);
      break;
    }

    case nsIDataType::VTYPE_BOOL :
    {
      PRBool value1;
      PRBool value2;
      rv = aVariant1->GetAsBool(&value1);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aVariant2->GetAsBool(&value2);
      NS_ENSURE_SUCCESS(rv, rv);
      *aEqual = (value1 == value2);
      break;
    }

    case nsIDataType::VTYPE_CHAR :
    {
      char value1;
      char value2;
      rv = aVariant1->GetAsChar(&value1);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aVariant2->GetAsChar(&value2);
      NS_ENSURE_SUCCESS(rv, rv);
      *aEqual = (value1 == value2);
      break;
    }

    case nsIDataType::VTYPE_WCHAR :
    {
      PRUnichar value1;
      PRUnichar value2;
      rv = aVariant1->GetAsWChar(&value1);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aVariant2->GetAsWChar(&value2);
      NS_ENSURE_SUCCESS(rv, rv);
      *aEqual = (value1 == value2);
      break;
    }

    case nsIDataType::VTYPE_VOID :
    case nsIDataType::VTYPE_EMPTY :
    {
      *aEqual = (dataType1 == dataType2);
      break;
    }

    case nsIDataType::VTYPE_ID :
    {
      nsID value1;
      nsID value2;
      rv = aVariant1->GetAsID(&value1);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aVariant2->GetAsID(&value2);
      NS_ENSURE_SUCCESS(rv, rv);
      *aEqual = value1.Equals(value2);
      break;
    }

    case nsIDataType::VTYPE_DOMSTRING :
    case nsIDataType::VTYPE_CHAR_STR :
    case nsIDataType::VTYPE_WCHAR_STR :
    case nsIDataType::VTYPE_UTF8STRING :
    case nsIDataType::VTYPE_CSTRING :
    case nsIDataType::VTYPE_ASTRING :
    case nsIDataType::VTYPE_STRING_SIZE_IS :
    case nsIDataType::VTYPE_WSTRING_SIZE_IS :
    {
      nsAutoString value1;
      nsAutoString value2;
      rv = aVariant1->GetAsAString(value1);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aVariant2->GetAsAString(value2);
      NS_ENSURE_SUCCESS(rv, rv);
      *aEqual = value1.Equals(value2);
      break;
    }

    case nsIDataType::VTYPE_INTERFACE :
    case nsIDataType::VTYPE_INTERFACE_IS :
    {
      nsCOMPtr<nsISupports> value1;
      nsCOMPtr<nsISupports> value2;
      rv = aVariant1->GetAsISupports(getter_AddRefs(value1));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aVariant2->GetAsISupports(getter_AddRefs(value2));
      NS_ENSURE_SUCCESS(rv, rv);
      *aEqual = (value1 == value2);
      break;
    }

    case nsIDataType::VTYPE_ARRAY :
    case nsIDataType::VTYPE_EMPTY_ARRAY :
    default :
      return NS_ERROR_NOT_IMPLEMENTED;
  }

  return NS_OK;
}

