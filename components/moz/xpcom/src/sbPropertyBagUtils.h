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

#ifndef SB_PROPERTY_BAG_UTILS_H__
#define SB_PROPERTY_BAG_UTILS_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Property bag utilities defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbPropertyBagUtilsLib.h
 * \brief Songbird Property Bag Utilities Definitions.
 */

//------------------------------------------------------------------------------
//
// Property bag utilities imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbVariantUtils.h>

// Mozilla imports.
#include <nsIWritablePropertyBag.h>
#include <nsIWritablePropertyBag2.h>


//------------------------------------------------------------------------------
//
// Property bag helper classes.
//
//------------------------------------------------------------------------------

/**
 * Helper class for individual properties.  When a property is requested,
 * sbPropertyBagHelper returns an sbPropertyHelper object that can convert the
 * property value to the requested type.
 */

// Forward declarations.
class sbPropertyBagHelper;

class sbPropertyHelper
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  /**
   * Construct a property helper object for the property with the key specified
   * by aKey from the property bag specified by aPropertyBag.  If aRV is
   * specified, return all error results from all methods in aRV.
   *
   * \param aPropertyBag        Property bag containing property.
   * \param aKey                Property key.
   * \param aRV                 Optional.  Return value to use for all methods.
   */
  sbPropertyHelper(nsIPropertyBag*  aPropertyBag,
                   const nsAString& aKey,
                   nsresult*        aRV = nsnull) :
    mPropertyBag(aPropertyBag),
    mKey(aKey),
    mRV(aRV),
    mInternalRV(NS_OK)
  {
    // Use internal return value if none specified.
    if (!mRV)
      mRV = &mInternalRV;
  }


  /**
   * Construct an empty property helper object.  If aRV is specified, return all
   * error results from all methods in aRV.
   *
   * \param aRV                 Optional.  Return value to use for all methods.
   */
  explicit sbPropertyHelper(nsresult* aRV = nsnull) :
    mRV(aRV),
    mInternalRV(NS_OK)
  {
    // Use internal return value if none specified.
    if (!mRV)
      mRV = &mInternalRV;
  }


  /**
   * Return the property value.
   *
   * \return                    Property value.
   */
  template<class T>
  operator T() const
  {
    // Validate state.
    NS_ENSURE_TRUE(mPropertyBag, T());

    // Get the value as a variant.
    nsCOMPtr<nsIVariant> value;
    *mRV = mPropertyBag->GetProperty(mKey, getter_AddRefs(value));
    NS_ENSURE_SUCCESS(*mRV, T());

    // Return the value.
    return sbVariantHelper(value, mRV);
  }


  /**
   * Set the property to the value specified by aValue.
   *
   * \param aValue              Value to which to set property.
   *
   * \return                    Value.
   */
  template <class T>
  sbPropertyHelper& operator=(T aValue)
  {
    // Validate state.
    NS_ENSURE_TRUE(mPropertyBag, *this);

    // Get a writable property bag.
    nsCOMPtr<nsIWritablePropertyBag>
      propertyBag = do_QueryInterface(mPropertyBag, mRV);
    NS_ENSURE_SUCCESS(*mRV, *this);

    // Set the property value.
    *mRV = propertyBag->SetProperty(mKey, sbNewVariant(aValue));
    NS_ENSURE_SUCCESS(*mRV, *this);

    return *this;
  }


  /**
   * Set the property value to the variant specified by aValue.
   *
   * \param aValue              Variant value to which to set property.
   *
   * \return                    Variant value.
   */
  sbPropertyHelper& operator=(nsIVariant* aValue)
  {
    // Validate state.
    NS_ENSURE_TRUE(mPropertyBag, *this);

    // Get a writable property bag.
    nsCOMPtr<nsIWritablePropertyBag>
      propertyBag = do_QueryInterface(mPropertyBag, mRV);
    NS_ENSURE_SUCCESS(*mRV, *this);

    // Set the property value.
    *mRV = propertyBag->SetProperty(mKey, aValue);
    NS_ENSURE_SUCCESS(*mRV, *this);

    return *this;
  }


  /**
   * Assume that the property value is a property bag.  Return the value of the
   * property with the key specified by aKey from the property bag.
   *
   * \param aKey                Property key.
   *
   * \return                    Property value.
   */
  sbPropertyHelper operator[](const char* aKey)
  {
    // Validate state.
    NS_ENSURE_TRUE(mPropertyBag, sbPropertyHelper(mRV));

    // Get the property bag.
    nsCOMPtr<nsIPropertyBag2>
      propertyBag = do_QueryInterface(mPropertyBag, mRV);
    NS_ENSURE_SUCCESS(*mRV, sbPropertyHelper(mRV));

    // Get the value property bag.
    nsCOMPtr<nsIPropertyBag> valuePropertyBag;
    *mRV = propertyBag->GetPropertyAsInterface(mKey,
                                               NS_GET_IID(nsIPropertyBag),
                                               getter_AddRefs(valuePropertyBag));
    NS_ENSURE_SUCCESS(*mRV, sbPropertyHelper(mRV));

    // Return the value.
    return sbPropertyHelper(valuePropertyBag, sbAutoString(aKey), mRV);
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
  // Protected interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mPropertyBag               Bag containing property.
  // mKey                       Property key.
  // mRV                        Pointer to return value.
  // mInternalRV                Internal return value storage.
  //

  nsCOMPtr<nsIPropertyBag>      mPropertyBag;
  nsString                      mKey;
  nsresult*                     mRV;
  nsresult                      mInternalRV;
};


/**
 * Helper class for property bags.
 *
 * TODO: Add getter_AddRefs equivalent
 */

class sbPropertyBagHelper
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  // Forward nsIPropertyBag methods.
  NS_FORWARD_SAFE_NSIPROPERTYBAG(mPropertyBag)
  NS_FORWARD_SAFE_NSIPROPERTYBAG2(mPropertyBag2)
  NS_FORWARD_SAFE_NSIWRITABLEPROPERTYBAG(mWritablePropertyBag)
  NS_FORWARD_SAFE_NSIWRITABLEPROPERTYBAG2(mWritablePropertyBag2)


  /**
   * Construct a property bag helper object with its own property bag.  If aRV
   * is specified, return all error results from all methods in aRV.
   *
   * \param aRV                 Optional.  Return value to use for all methods.
   */
  sbPropertyBagHelper(nsresult* aRV = nsnull) :
    mRV(aRV),
    mInternalRV(NS_OK)
  {
    // Use internal return value if none specified.
    if (!mRV)
      mRV = &mInternalRV;

    // Create property bag.
    mPropertyBag =
      do_CreateInstance("@songbirdnest.com/moz/xpcom/sbpropertybag;1", mRV);
    NS_ENSURE_SUCCESS(*mRV, /* void */);
    mPropertyBag2 = do_QueryInterface(mPropertyBag);
    mWritablePropertyBag = do_QueryInterface(mPropertyBag);
    mWritablePropertyBag2 = do_QueryInterface(mPropertyBag);
  }


  /**
   * Construct a property bag helper object using the property bag specified by
   * aPropertyBag.  If aRV is specified, return all error results from all
   * methods in aRV.
   *
   * \param aPropertyBag        Base property bag.
   * \param aRV                 Optional.  Return value to use for all methods.
   */
  sbPropertyBagHelper(nsISupports* aPropertyBag,
                      nsresult*    aRV = nsnull) :
    mRV(aRV),
    mInternalRV(NS_OK)
  {
    // Use internal return value if none specified.
    if (!mRV)
      mRV = &mInternalRV;

    // Get the property bag.
    mPropertyBag = do_QueryInterface(aPropertyBag, mRV);
    NS_ENSURE_SUCCESS(*mRV, /* void */);
    mPropertyBag2 = do_QueryInterface(aPropertyBag);
    mWritablePropertyBag = do_QueryInterface(mPropertyBag);
    mWritablePropertyBag2 = do_QueryInterface(mPropertyBag);
  }


  /**
   * Return the base property bag.
   */
  nsIPropertyBag* GetBag()
  {
    return mPropertyBag;
  }

  operator nsIPropertyBag*() const
  {
    return mPropertyBag;
  }

  operator nsIPropertyBag2*() const
  {
    return mPropertyBag2;
  }

  operator nsIWritablePropertyBag*() const
  {
    return mWritablePropertyBag;
  }

  operator nsIWritablePropertyBag2*() const
  {
    return mWritablePropertyBag2;
  }


  /**
   * Return the value of the property with the key specified by aKey.
   *
   * \param aKey                Property key.
   *
   * \return                    Property value.
   */
  sbPropertyHelper Get(const char* aKey) const
  {
    return sbPropertyHelper(mPropertyBag, sbAutoString(aKey), mRV);
  }


  /**
   * Return the value of the property with the key specified by aKey.
   *
   * \param aKey                Property key.
   *
   * \return                    Property value.
   */
  sbPropertyHelper operator[](const char* aKey) const
  {
    return Get(aKey);
  }

  sbPropertyHelper operator[](const nsACString & aKey) const
  {
    return Get(aKey.BeginReading());
  }
  /**
   * Allow sbPropertyBagHelper to be used as an nsIPropertyBag*,
   * nsIPropertyBag2*, nsIWritablePropertyBag*, or nsIWritablePropertyBag2*.
   *
   *   sbPropertyBagHelper bag(aPropertyBag);
   *   bag->SetPropertyAsUint32(NS_LITERAL_STRING("answer"), 42);
   */
  sbPropertyBagHelper* operator->()
  {
    return this;
  }

  sbPropertyBagHelper& operator*()
  {
    return *this;
  }


  /**
   * Set the value of the property specified by aKey to the value specified by
   * aValue.
   *
   * \param aKey                Property key.
   * \param aValue              Key value.
   */
  template <class T>
  nsresult Set(char* aKey,
               T     aValue)
  {
    // Validate state and arguments.
    NS_ENSURE_STATE(mWritablePropertyBag);
    NS_ENSURE_ARG_POINTER(aKey);

    // Set the property.
    *mRV = mWritablePropertyBag->SetProperty(sbAutoString(aKey),
                                             sbNewVariant(aValue));
    NS_ENSURE_SUCCESS(*mRV, *mRV);

    return NS_OK;
  }


  /**
   * Return true if the property bag has the key specified by aKey.
   *
   * \param aKey                Key to check.
   *
   * \return true               Property bag has key.
   */
  PRBool HasKey(const char* aKey)
  {
    // Validate state.
    NS_ENSURE_TRUE(mPropertyBag2, PR_FALSE);

    // Check for key.
    PRBool hasKey;
    *mRV = mPropertyBag2->HasKey(sbAutoString(aKey), &hasKey);
    NS_ENSURE_SUCCESS(*mRV, PR_FALSE);

    return hasKey;
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
  // mPropertyBag               Bag containing property.
  // mPropertyBag2
  // mWritablePropertyBag
  // mWritablePropertyBag2
  // mRV                        Pointer to return value.
  // mInternalRV                Internal return value storage.
  //

  nsCOMPtr<nsIPropertyBag>      mPropertyBag;
  nsCOMPtr<nsIPropertyBag2>     mPropertyBag2;
  nsCOMPtr<nsIWritablePropertyBag>
                                mWritablePropertyBag;
  nsCOMPtr<nsIWritablePropertyBag2>
                                mWritablePropertyBag2;
  nsresult*                     mRV;
  nsresult                      mInternalRV;
};


#endif /* SB_PROPERTY_BAG_UTILS_H__ */

