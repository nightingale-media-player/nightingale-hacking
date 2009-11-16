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

#ifndef __SB_IPD_UTILS_H__
#define __SB_IPD_UTILS_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// iPod device utility services defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbIPDUtils.h
 * \brief Songbird iPod Device Utility Definitions.
 */

//------------------------------------------------------------------------------
//
// iPod device utility imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbMemoryUtils.h>

// Songbird interface imports
#include <sbIDevice.h>

// Mozilla imports.
#include <nsComponentManagerUtils.h>
#include <nsIIOService.h>
#include <nsIURI.h>
#include <nsIVariant.h>
#include <nsServiceManagerUtils.h>
#include <nsStringAPI.h>

// Libgpod imports.
#include <itdb.h>

// Std C imports.
#include <stdlib.h>


//------------------------------------------------------------------------------
//
// iPod device utility services classes.
//
//------------------------------------------------------------------------------

//
// Auto-disposal class wrappers.
//
//   sbAutoGMemPtr              Wrapper to auto-dispose memory blocks allocated
//                              with g_malloc.
//   sbAutoITDBTrackPtr         Wrapper to auto-dispose iPod database track data
//                              records.
//

SB_AUTO_CLASS(sbAutoGMemPtr, void*, mValue, g_free(mValue), mValue = NULL);
SB_AUTO_CLASS(sbAutoITDBTrackPtr,
              Itdb_Track*,
              mValue,
              itdb_track_free(mValue),
              mValue = NULL);


/**
 * This class constructs nsIVariant objects from simpler data types.
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
 *   nsCOMPtr<nsIVariant> value = sbIPDVariant("Value").get();
 */

class sbIPDVariant
{
public:

  sbIPDVariant(nsISupports* aValue)
  {
    nsresult rv;
    mVariant = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to create variant.");
    rv = mVariant->SetAsISupports(aValue);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to write variant.");
  }

  sbIPDVariant(REFNSIID aIID, nsISupports* aValue)
  {
    nsresult rv;
    mVariant = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to create variant.");
    rv = mVariant->SetAsInterface(aIID, aValue);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to write variant.");
  }

  sbIPDVariant(const nsAString& aValue)
  {
    nsresult rv;
    mVariant = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to create variant.");
    rv = mVariant->SetAsAString(aValue);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to write variant.");
  }

  sbIPDVariant(const nsACString& aValue,
               PRUint16 aType = nsIDataType::VTYPE_CSTRING)
  {
    nsresult rv;
    mVariant = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to create variant.");
    if (aType == nsIDataType::VTYPE_UTF8STRING)
      rv = mVariant->SetAsAUTF8String(aValue);
    else
      rv = mVariant->SetAsACString(aValue);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to write variant.");
  }

  sbIPDVariant(const char* aValue)
  {
    nsresult rv;
    mVariant = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to create variant.");
    rv = mVariant->SetAsAString(NS_ConvertASCIItoUTF16(aValue));
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to write variant.");
  }

  sbIPDVariant(char aValue)
  {
    nsresult rv;
    mVariant = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to create variant.");
    rv = mVariant->SetAsChar(aValue);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to write variant.");
  }

  sbIPDVariant(PRInt16 aValue)
  {
    nsresult rv;
    mVariant = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to create variant.");
    rv = mVariant->SetAsInt16(aValue);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to write variant.");
  }

  sbIPDVariant(PRInt32 aValue)
  {
    nsresult rv;
    mVariant = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to create variant.");
    rv = mVariant->SetAsInt32(aValue);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to write variant.");
  }

  sbIPDVariant(PRUint32 aValue,
               PRUint16 aType = nsIDataType::VTYPE_UINT32)
  {
    nsresult rv;
    mVariant = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to create variant.");
    if (aType == nsIDataType::VTYPE_BOOL)
      rv = mVariant->SetAsBool(aValue);
    else
      rv = mVariant->SetAsUint32(aValue);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to write variant.");
  }

  sbIPDVariant(PRInt64 aValue)
  {
    nsresult rv;
    mVariant = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to create variant.");
    rv = mVariant->SetAsInt64(aValue);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to write variant.");
  }

  sbIPDVariant(PRUint64 aValue)
  {
    nsresult rv;
    mVariant = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to create variant.");
    rv = mVariant->SetAsUint64(aValue);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to write variant.");
  }

  sbIPDVariant(double aValue)
  {
    nsresult rv;
    mVariant = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to create variant.");
    rv = mVariant->SetAsDouble(aValue);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to write variant.");
  }

  nsIVariant* get() { return (mVariant); }

  static nsresult GetValue(nsIVariant* aVariant, PRInt16* aValue) {
    return aVariant->GetAsInt16(aValue);
  }

private:
  nsCOMPtr<nsIWritableVariant> mVariant;
};


/**
 * This class constructs nsIURI objects from simpler data types.
 *
 * Example:
 *
 *   // Get an nsIURI set with a C-string value.
 *   nsCOMPtr<nsIURI> uri = sbIPDURI("http://www.songbirdnest.com").get();
 */

class sbIPDURI
{
public:

  sbIPDURI(const char* aValue)
  {
    nsresult rv;
    nsCOMPtr<nsIIOService> ioService =
                             do_GetService("@mozilla.org/network/io-service;1",
                                           &rv);
    NS_ENSURE_SUCCESS(rv, /* void */);
    rv = ioService->NewURI(nsCAutoString(aValue),
                           nsnull,
                           nsnull,
                           getter_AddRefs(mURI));
    NS_ENSURE_SUCCESS(rv, /* void */);
  }

  nsIURI* get() { return (mURI); }

private:
  nsCOMPtr<nsIURI> mURI;
};


//------------------------------------------------------------------------------
//
// iPod device utility services prototypes.
//
//------------------------------------------------------------------------------

nsresult CreateAndDispatchDeviceManagerEvent
                             (PRUint32     aType,
                              nsIVariant*  aData = nsnull,
                              nsISupports* aOrigin = nsnull,
                              PRUint32     aDeviceState = sbIDevice::STATE_IDLE,
                              PRBool       aAsync = PR_FALSE);


#endif /* __SB_IPD_UTILS_H__ */

