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

#ifndef __SB_VARIANT_UTILS_H__
#define __SB_VARIANT_UTILS_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Variant utility defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbVariantUtils.h
 * \brief Nightingale Variant Utility Definitions.
 */

//------------------------------------------------------------------------------
//
// Variant utility imported services.
//
//------------------------------------------------------------------------------

// Mozilla imports.
#include <nsComponentManagerUtils.h>
#include <nsIVariant.h>
#include <nsStringAPI.h>


//------------------------------------------------------------------------------
//
// Variant utility classes.
//
//------------------------------------------------------------------------------

/**
 * This class constructs thread-safe nsIVariant objects from simpler data types.
 * The constructors create a null variant on error.
 *
 * Most constructors create the proper nsIVariant object based upon the data
 * type of the value argument.  However, some nsIVariant types have the same C
 * data types (e.g., boolean and UTF8 nsIVariants).  For these cases, an extra
 * nsIDataType argument must be provided to the constructor.  The following
 * nsIDataTypes must be explicitly specified in the constructor:
 *
 *   VTYPE_UTF8STRING
 *   VTYPE_BOOL
 *
 * Example:
 *
 *   // Get an nsIVariant set with a C-string value.
 *   nsCOMPtr<nsIVariant> value = sbNewVariant("Value");
 */

class sbNewVariant
{
public:

  //
  // Variant constructors.
  //

  sbNewVariant()
  {
    nsresult rv;
    mVariant = do_CreateInstance("@getnightingale.com/Nightingale/Variant;1", &rv);
    if (NS_SUCCEEDED(rv))
      rv = mVariant->SetAsVoid();
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to create new variant.");
      mVariant = nsnull;
    }
  }

  sbNewVariant(nsISupports* aValue)
  {
    nsresult rv;
    mVariant = do_CreateInstance("@getnightingale.com/Nightingale/Variant;1", &rv);
    if (NS_SUCCEEDED(rv)) {
      if (aValue)
        rv = mVariant->SetAsISupports(aValue);
      else
        rv = mVariant->SetAsEmpty();
    }
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to create new variant.");
      mVariant = nsnull;
    }
  }

  sbNewVariant(REFNSIID aIID, nsISupports* aValue)
  {
    nsresult rv;
    mVariant = do_CreateInstance("@getnightingale.com/Nightingale/Variant;1", &rv);
    if (NS_SUCCEEDED(rv)) {
      if (aValue)
        rv = mVariant->SetAsInterface(aIID, aValue);
      else
        rv = mVariant->SetAsEmpty();
    }
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to create new variant.");
      mVariant = nsnull;
    }
  }

  sbNewVariant(const nsAString& aValue)
  {
    nsresult rv;
    mVariant = do_CreateInstance("@getnightingale.com/Nightingale/Variant;1", &rv);
    if (NS_SUCCEEDED(rv))
      rv = mVariant->SetAsAString(aValue);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to create new variant.");
      mVariant = nsnull;
    }
  }

  sbNewVariant(const nsACString& aValue,
               PRUint16 aType = nsIDataType::VTYPE_CSTRING)
  {
    nsresult rv;
    mVariant = do_CreateInstance("@getnightingale.com/Nightingale/Variant;1", &rv);
    if (NS_SUCCEEDED(rv)) {
      if (aType == nsIDataType::VTYPE_UTF8STRING)
        rv = mVariant->SetAsAUTF8String(aValue);
      else
        rv = mVariant->SetAsACString(aValue);
    }
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to create new variant.");
      mVariant = nsnull;
    }
  }

  sbNewVariant(const char* aValue)
  {
    nsresult rv;
    nsString value;
    if (aValue)
      value.AssignLiteral(aValue);
    mVariant = do_CreateInstance("@getnightingale.com/Nightingale/Variant;1", &rv);
    if (NS_SUCCEEDED(rv)) {
      if (aValue)
        rv = mVariant->SetAsAString(value);
      else
        rv = mVariant->SetAsEmpty();
    }
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to create new variant.");
      mVariant = nsnull;
    }
  }

  sbNewVariant(char aValue)
  {
    nsresult rv;
    mVariant = do_CreateInstance("@getnightingale.com/Nightingale/Variant;1", &rv);
    if (NS_SUCCEEDED(rv))
      rv = mVariant->SetAsChar(aValue);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to create new variant.");
      mVariant = nsnull;
    }
  }

  sbNewVariant(PRInt16 aValue)
  {
    nsresult rv;
    mVariant = do_CreateInstance("@getnightingale.com/Nightingale/Variant;1", &rv);
    if (NS_SUCCEEDED(rv))
      rv = mVariant->SetAsInt16(aValue);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to create new variant.");
      mVariant = nsnull;
    }
  }

  sbNewVariant(PRInt32 aValue)
  {
    nsresult rv;
    mVariant = do_CreateInstance("@getnightingale.com/Nightingale/Variant;1", &rv);
    if (NS_SUCCEEDED(rv))
      rv = mVariant->SetAsInt32(aValue);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to create new variant.");
      mVariant = nsnull;
    }
  }

  sbNewVariant(PRUint32 aValue,
               PRUint16 aType = nsIDataType::VTYPE_UINT32)
  {
    nsresult rv;
    mVariant = do_CreateInstance("@getnightingale.com/Nightingale/Variant;1", &rv);
    if (NS_SUCCEEDED(rv)) {
      if (aType == nsIDataType::VTYPE_BOOL)
        rv = mVariant->SetAsBool(aValue);
      else
        rv = mVariant->SetAsUint32(aValue);
    }
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to create new variant.");
      mVariant = nsnull;
    }
  }

  sbNewVariant(PRInt64 aValue)
  {
    nsresult rv;
    mVariant = do_CreateInstance("@getnightingale.com/Nightingale/Variant;1", &rv);
    if (NS_SUCCEEDED(rv))
      rv = mVariant->SetAsInt64(aValue);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to create new variant.");
      mVariant = nsnull;
    }
  }

  sbNewVariant(PRUint64 aValue)
  {
    nsresult rv;
    mVariant = do_CreateInstance("@getnightingale.com/Nightingale/Variant;1", &rv);
    if (NS_SUCCEEDED(rv))
      rv = mVariant->SetAsUint64(aValue);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to create new variant.");
      mVariant = nsnull;
    }
  }

  sbNewVariant(double aValue)
  {
    nsresult rv;
    mVariant = do_CreateInstance("@getnightingale.com/Nightingale/Variant;1", &rv);
    if (NS_SUCCEEDED(rv))
      rv = mVariant->SetAsDouble(aValue);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to create new variant.");
      mVariant = nsnull;
    }
  }


  /**
   * Return the constructed variant object.
   */

  nsIVariant* get() const { return mVariant; }

  operator nsIVariant*() const { return get(); }


private:
  nsCOMPtr<nsIWritableVariant> mVariant;
};


#endif /* __SB_VARIANT_UTILS_H__ */

