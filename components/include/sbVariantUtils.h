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
 * \brief Songbird Variant Utility Definitions.
 */

//------------------------------------------------------------------------------
//
// Variant utility imported services.
//
//------------------------------------------------------------------------------

// Mozilla imports.
#include <nsComponentManagerUtils.h>
#include <nsIVariant.h>
#include <sbStringUtils.h>


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
    mVariant = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
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
    mVariant = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
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
    mVariant = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
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
    mVariant = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
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
    mVariant = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
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
    mVariant = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
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
    mVariant = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
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
    mVariant = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
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
    mVariant = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
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
    mVariant = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
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
    mVariant = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
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
    mVariant = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
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
    mVariant = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
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


/**
 * Helper class for variants to return the value of the variant.  This class is
 * mainly useful for template functions that use variants.
 */

class sbVariantHelper
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  /**
   * Construct a variant helper for the variant specified by aVariant.  If aRV
   * is specified, return all error results from all methods in aRV.
   *
   * \param aVariant            Variant.
   * \param aRV                 Optional.  Return value to use for all methods.
   */
  sbVariantHelper(nsIVariant* aVariant,
                  nsresult*   aRV = nsnull) :
    mVariant(aVariant),
    mRV(aRV),
    mInternalRV(NS_OK)
  {
    NS_ASSERTION(mVariant, "Null variant");
    if (!mRV)
      mRV = &mInternalRV;
  }


  //
  // Operator methods for returning the variant value.
  //

  operator PRUint8() const
  {
    // Ensure variant is present.
    if (!mVariant) {
      NS_WARNING("Null variant");
      *mRV = NS_ERROR_NULL_POINTER;
      return 0;
    }

    // Get the variant value.
    PRUint8 value;
    *mRV = mVariant->GetAsUint8(&value);
    NS_ENSURE_SUCCESS(*mRV, 0);

    return value;
  }

  // XXXeps nsIVaraint.getAsInt8 returns PRUint8

  operator PRUint16() const
  {
    // Ensure variant is present.
    if (!mVariant) {
      NS_WARNING("Null variant");
      *mRV = NS_ERROR_NULL_POINTER;
      return 0;
    }

    // Get the variant value.
    PRUint16 value;
    *mRV = mVariant->GetAsUint16(&value);
    NS_ENSURE_SUCCESS(*mRV, 0);

    return value;
  }

  operator PRInt16() const
  {
    // Ensure variant is present.
    if (!mVariant) {
      NS_WARNING("Null variant");
      *mRV = NS_ERROR_NULL_POINTER;
      return 0;
    }

    // Get the variant value.
    PRInt16 value;
    *mRV = mVariant->GetAsInt16(&value);
    NS_ENSURE_SUCCESS(*mRV, 0);

    return value;
  }

  operator PRUint32() const
  {
    // Ensure variant is present.
    if (!mVariant) {
      NS_WARNING("Null variant");
      *mRV = NS_ERROR_NULL_POINTER;
      return 0;
    }

    // Get the variant value.
    PRUint32 value;
    *mRV = mVariant->GetAsUint32(&value);
    NS_ENSURE_SUCCESS(*mRV, 0);

    return value;
  }

  operator PRInt32() const
  {
    // Ensure variant is present.
    if (!mVariant) {
      NS_WARNING("Null variant");
      *mRV = NS_ERROR_NULL_POINTER;
      return 0;
    }

    // Get the variant value.
    PRInt32 value;
    *mRV = mVariant->GetAsInt32(&value);
    NS_ENSURE_SUCCESS(*mRV, 0);

    return value;
  }

  operator PRUint64() const
  {
    // Ensure variant is present.
    if (!mVariant) {
      NS_WARNING("Null variant");
      *mRV = NS_ERROR_NULL_POINTER;
      return 0;
    }

    // Get the variant value.
    PRUint64 value;
    *mRV = mVariant->GetAsUint64(&value);
    NS_ENSURE_SUCCESS(*mRV, 0);

    return value;
  }

  operator PRInt64() const
  {
    // Ensure variant is present.
    if (!mVariant) {
      NS_WARNING("Null variant");
      *mRV = NS_ERROR_NULL_POINTER;
      return 0;
    }

    // Get the variant value.
    PRInt64 value;
    *mRV = mVariant->GetAsInt64(&value);
    NS_ENSURE_SUCCESS(*mRV, 0);

    return value;
  }

  operator nsString() const
  {
    // Ensure variant is present.
    if (!mVariant) {
      NS_WARNING("Null variant");
      *mRV = NS_ERROR_NULL_POINTER;
      return SBVoidString();
    }

    // Get the variant value.
    nsAutoString value;
    *mRV = mVariant->GetAsAString(value);
    NS_ENSURE_SUCCESS(*mRV, SBVoidString());

    return value;
  }


  /**
   * Return the result of the last method called.
   *
   * \return                    Reult of last method called.
   */
  nsresult rv()
  {
    return *mRV;
  }


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mVariant                   Base variant.
  // mRV                        Pointer to return value.
  // mInternalRV                Internal return value storage.
  //

  nsCOMPtr<nsIVariant>          mVariant;
  nsresult*                     mRV;
  nsresult                      mInternalRV;
};


#endif /* __SB_VARIANT_UTILS_H__ */

