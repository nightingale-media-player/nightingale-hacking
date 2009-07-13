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
// iPod device utility services macros.
//
//------------------------------------------------------------------------------

/**
 * Define a class that wraps a data type and auto-disposes it when going out of
 * scope.  The class name is specified by aName, and the data type is specified
 * by aType.  An additional class field type may be specified by aType2.  A code
 * snippet that checks for valid data is specified by aIsValid.  A code snippet
 * that disposes of the data is specified by aDispose.  A code snippet that
 * invalidates the data is specified by aInvalidate.  The code snippets may use
 * mValue to refer to the wrapped data and mValue2 to refer to the additional
 * class field.
 *
 * \param aName                 Name of class.
 * \param aType                 Class data type.
 * \param aType2                Additional class field data type.
 * \param aIsValid              Code snippet to test for data validity.
 * \param aDispose              Code snippet to dispose of the data.
 * \param aInvalidate           Code snippet to invalidate the data.
 *
 * Example:
 *   SB_AUTO_CLASS(sbAutoMemPtr,
 *                 void*,
 *                 mValue,
 *                 NS_Free(mValue),
 *                 mValue = nsnull)
 *   sbAutoMemPtr autoMem(memPtr);
 *
 *   SB_AUTO_CLASS2(sbAutoClassData,
 *                  void*,
 *                  classType*,
 *                  mValue,
 *                  mValue2->Delete(mValue),
 *                  mValue = nsnull)
 *   sbAutoClassData(data, this);
 */

#define SB_AUTO_CLASS2(aName, aType, aType2, aIsValid, aDispose, aInvalidate)  \
class aName                                                                    \
{                                                                              \
public:                                                                        \
  aName() { Invalidate(); }                                                    \
                                                                               \
  aName(aType aValue) : mValue(aValue) {}                                      \
                                                                               \
  aName(aType aValue, aType2 aValue2) : mValue(aValue), mValue2(aValue2) {}    \
                                                                               \
  virtual ~aName()                                                             \
  {                                                                            \
    if (aIsValid) {                                                            \
      aDispose;                                                                \
    }                                                                          \
  }                                                                            \
                                                                               \
  void Set(aType aValue) { mValue = aValue; }                                  \
                                                                               \
  void Set(aType aValue, aType2 aValue2)                                       \
         { mValue = aValue; mValue2 = aValue2; }                               \
                                                                               \
  aType forget()                                                               \
  {                                                                            \
    aType value = mValue;                                                      \
    Invalidate();                                                              \
    return value;                                                              \
  }                                                                            \
                                                                               \
private:                                                                       \
  aType mValue;                                                                \
  aType2 mValue2;                                                              \
                                                                               \
  void Invalidate() { aInvalidate; }                                           \
}

#define SB_AUTO_CLASS(aName, aType, aIsValid, aDispose, aInvalidate)           \
          SB_AUTO_CLASS2(aName, aType, char, aIsValid, aDispose, aInvalidate)


//------------------------------------------------------------------------------
//
// iPod device utility services classes.
//
//------------------------------------------------------------------------------

//
// Auto-disposal class wrappers.
//
//   sbAutoMemPtr               Wrapper to auto-dipose memory blocks allocated
//                              with malloc.
//   sbAutoNSMemPtr             Wrapper to auto-dispose memory blocks allocated
//                              with NS_Alloc.
//   sbAutoGMemPtr              Wrapper to auto-dispose memory blocks allocated
//                              with g_malloc.
//   sbAutoITDBTrackPtr         Wrapper to auto-dispose iPod database track data
//                              records.
//

SB_AUTO_CLASS(sbAutoMemPtr, void*, mValue, free(mValue), mValue = NULL);
SB_AUTO_CLASS(sbAutoNSMemPtr, void*, mValue, NS_Free(mValue), mValue = nsnull);
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

nsresult CreateAndDispatchDeviceManagerEvent(PRUint32     aType,
                                             nsIVariant*  aData = nsnull,
                                             nsISupports* aOrigin = nsnull,
                                             PRBool       aAsync = PR_FALSE);


#endif /* __SB_IPD_UTILS_H__ */

