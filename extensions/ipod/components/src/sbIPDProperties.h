/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//=BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://www.songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the GPL).
// 
// Software distributed under the License is distributed
// on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
// 
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
// 
//=END SONGBIRD GPL
*/

#ifndef __SB_IPD_PROPERTIES_H__
#define __SB_IPD_PROPERTIES_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// iPod device properties defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/** 
 * \file  sbIPDProperties.h
 * \brief Songbird iPod Device Properties Definitions.
 */

//------------------------------------------------------------------------------
//
// iPod device properties imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbIPDUtils.h"

// Songbird imports.
#include <sbIDeviceProperties.h>

// Mozilla imports.
#include <nsAutoPtr.h>
#include <nsIWritablePropertyBag.h>
#include <nsIWritablePropertyBag2.h>


//------------------------------------------------------------------------------
//
// iPod device properties classes.
//
//------------------------------------------------------------------------------

/**
 * Forward the nsIWritablePropertyBag2 set method specified by aMethod with the
 * property value type specified by aType to the object specified by aObj.
 *
 * \param aObj                  Object to which to forward.
 * \param aMethod               Set method to forward.
 * \param aType                 Type of property value.
 */

#define SBIPD_FORWARD_SAFE_NSIWRITABLEPROPERTYBAG2_SET(aObj, aMethod, aType) \
NS_IMETHOD aMethod(const nsAString& aName, aType aValue) \
{ \
  return !aObj ? NS_ERROR_NULL_POINTER : \
                 aObj->SetProperty(aName, sbIPDVariant(aValue).get()); \
}


/**
 * Forward the nsIWritablePropertyBag2 set method specified by aMethod with the
 * property value type specified by aType and aVType to the object specified by
 * aObj.
 *
 * \param aObj                  Object to which to forward.
 * \param aMethod               Set method to forward.
 * \param aType                 C type of property value.
 * \param aVType                nsIDataType type of property value.
 */

#define SBIPD_FORWARD_SAFE_NSIWRITABLEPROPERTYBAG2_SET_VTYPE(aObj, \
                                                             aMethod, \
                                                             aType, \
                                                             aVType) \
NS_IMETHOD aMethod(const nsAString& aName, aType aValue) \
{ \
  return !aObj ? NS_ERROR_NULL_POINTER : \
                 aObj->SetProperty(aName, sbIPDVariant(aValue, aVType).get()); \
}


/**
 * Forward nsIWritablePropertyBag2 methods to the object specified by aObj.
 *
 * \param aObj                  Object to which to forward.
 */

#define SBIPD_FORWARD_SAFE_NSIWRITABLEPROPERTYBAG2(aObj) \
  SBIPD_FORWARD_SAFE_NSIWRITABLEPROPERTYBAG2_SET(aObj, \
                                                 SetPropertyAsInt32, \
                                                 PRInt32) \
  SBIPD_FORWARD_SAFE_NSIWRITABLEPROPERTYBAG2_SET(aObj, \
                                                 SetPropertyAsUint32, \
                                                 PRUint32) \
  SBIPD_FORWARD_SAFE_NSIWRITABLEPROPERTYBAG2_SET(aObj, \
                                                 SetPropertyAsInt64, \
                                                 PRInt64) \
  SBIPD_FORWARD_SAFE_NSIWRITABLEPROPERTYBAG2_SET(aObj, \
                                                 SetPropertyAsUint64, \
                                                 PRUint64) \
  SBIPD_FORWARD_SAFE_NSIWRITABLEPROPERTYBAG2_SET(aObj, \
                                                 SetPropertyAsDouble, \
                                                 double) \
  SBIPD_FORWARD_SAFE_NSIWRITABLEPROPERTYBAG2_SET(aObj, \
                                                 SetPropertyAsAString, \
                                                 const nsAString&) \
  SBIPD_FORWARD_SAFE_NSIWRITABLEPROPERTYBAG2_SET(aObj, \
                                                 SetPropertyAsACString, \
                                                 const nsACString&) \
  SBIPD_FORWARD_SAFE_NSIWRITABLEPROPERTYBAG2_SET_VTYPE \
    (aObj, \
     SetPropertyAsAUTF8String, \
     const nsACString&, \
     nsIDataType::VTYPE_UTF8STRING) \
  SBIPD_FORWARD_SAFE_NSIWRITABLEPROPERTYBAG2_SET_VTYPE \
    (aObj, \
     SetPropertyAsBool, \
     bool, \
     nsIDataType::VTYPE_BOOL) \
  SBIPD_FORWARD_SAFE_NSIWRITABLEPROPERTYBAG2_SET(aObj, \
                                                 SetPropertyAsInterface, \
                                                 nsISupports*)


/**
 * This class represents a set of iPod device properties.
 */

class sbIPDDevice;

class sbIPDProperties : public sbIDeviceProperties,
                        public nsIWritablePropertyBag,
                        public nsIWritablePropertyBag2
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public :

  //
  // Inherited interfaces.
  //

  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICEPROPERTIES
  NS_DECL_NSIWRITABLEPROPERTYBAG


  //
  // Forwarded interfaces.
  //

  NS_FORWARD_SAFE_NSIPROPERTYBAG(mProperties)
  NS_FORWARD_SAFE_NSIPROPERTYBAG2(mProperties2)
  SBIPD_FORWARD_SAFE_NSIWRITABLEPROPERTYBAG2(this)


  //
  // iPod device properties services.
  //

  sbIPDProperties(sbIPDDevice* aDevice);

  virtual ~sbIPDProperties();

  nsresult Initialize();

  void Finalize();

  nsresult SetPropertyInternal(const nsAString& aName,
                               nsIVariant*      aValue);


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mDevice                    Device to which this property set belongs.
  //                            Don't refcount to avoid a cycle.
  // mProperties                Storage for device properties.
  // mProperties2               nsIWritablePropertyBag2 interface to device
  //                            properties.
  //

  sbIPDDevice*                  mDevice;
  nsCOMPtr<nsIWritablePropertyBag>
                                mProperties;
  nsCOMPtr<nsIWritablePropertyBag2>
                                mProperties2;
};


#endif // __SB_IPD_PROPERTIES_H__

