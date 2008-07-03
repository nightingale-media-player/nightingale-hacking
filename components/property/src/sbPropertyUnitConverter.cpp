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

// Base class for property unit converters (sbIPropertyUnitConverter)

#include "sbPropertyUnitConverter.h"
#include <sbLockUtils.h>
#include <nsCOMPtr.h>
#include "nsEnumeratorUtils.h"
#include "nsArrayEnumerator.h"

static const char *gsFmtRadix10 = "%lld";
static const char *gsFmtFloatOut = "%f";
static const char *gsFmtFloatIn = "%Lg";

// ---------------------------------------------------------------------------
// sbPropertyUnit class - describes a unit exposed by sbIPropertyUnitConverter
// ---------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(sbPropertyUnit, sbIPropertyUnit)

// ctor
sbPropertyUnit::sbPropertyUnit()
: mLock(nsnull)
, mInitialized(PR_FALSE)
{
  mLock = PR_NewLock();
  NS_ASSERTION(mLock, "sbPropertyUnit::mLock failed to create lock!");
}

// ctor w/init
sbPropertyUnit::sbPropertyUnit(const nsAString& aName,
                               const nsAString& aShortName,
                               const nsAString& aID)
: mLock(nsnull)
, mInitialized(PR_TRUE)
, mName(aName)
, mShortName(aShortName)
, mID(aID)
{
  mLock = PR_NewLock();
  NS_ASSERTION(mLock, "sbPropertyOperator::mLock failed to create lock!");
}

// dtor
sbPropertyUnit::~sbPropertyUnit()
{
  if(mLock) {
    PR_DestroyLock(mLock);
  }
}

// Init function, to use in conjunction with the default constructor
NS_IMETHODIMP sbPropertyUnit::Init(const nsAString & aName, 
                                   const nsAString & aShortName, 
                                   const nsAString & aID)
{
  sbSimpleAutoLock lock(mLock);
  NS_ENSURE_TRUE(mInitialized == PR_FALSE, NS_ERROR_ALREADY_INITIALIZED);

  mName = aName;
  mShortName = aShortName;
  mID = aID;
  
  mInitialized = PR_TRUE;

  return NS_OK;
}

// Attribute getter for the the full name of the unit, ie. "Kilobyte".
// This should be a partial entity (eg. "&my.partial.entity"), so that
// it may be localized.
NS_IMETHODIMP sbPropertyUnit::GetName(nsAString & aName)
{
  sbSimpleAutoLock lock(mLock);
  if (!mInitialized) return NS_ERROR_NOT_INITIALIZED;
  aName = mName;
  return NS_OK;
}

// Attribute getter for the short name of the unit, ie. "kB". This
// should be a partial entity (eg. "&my.partial.entity"), so that it may
// be localized.
NS_IMETHODIMP sbPropertyUnit::GetShortName(nsAString & aShortName)
{
  sbSimpleAutoLock lock(mLock);
  if (!mInitialized) return NS_ERROR_NOT_INITIALIZED;
  aShortName = mShortName;
  return NS_OK;
}

// Attribute getter for the id of the unit, ie "kb". The id is not a
// localized string, it never changes, and is used in calls to 
// sbIPropertyUnitConverter's convert function, to specify from and
// to which unit to convert.
NS_IMETHODIMP sbPropertyUnit::GetId(nsAString &aID)
{
  sbSimpleAutoLock lock(mLock);
  if (!mInitialized) return NS_ERROR_NOT_INITIALIZED;
  aID = mID;
  return NS_OK;
}

// -----------------------------------------------------------------------------
// sbPropertyUnitConverter class - exposes the units and the conversion function
// -----------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(sbPropertyUnitConverter, sbIPropertyUnitConverter)

// ctor
sbPropertyUnitConverter::sbPropertyUnitConverter()
: mNative(0)
, mLock(nsnull)
{
  mLock = PR_NewLock();
  NS_ASSERTION(mLock, "sbPropertyUnitConverter::mLock failed to create lock!");
}

// dtor
sbPropertyUnitConverter::~sbPropertyUnitConverter() 
{
  if(mLock) {
    PR_DestroyLock(mLock);
  }
}

// Unit registration function. This is not part of the interface, it is meant to
// be called by inheritors of this class, so that they may register each of
// their particular units.
void sbPropertyUnitConverter::RegisterUnit(PRUint32 aUnitInternalID, 
                                           const nsAString &aUnitExternalID, 
                                           const nsAString &aUnitName, 
                                           const nsAString &aUnitShortName, 
                                           PRBool isNative)
{
  sbSimpleAutoLock lock(mLock);
  if (isNative) mNative = aUnitExternalID;
  sbPropertyUnit *unit = new sbPropertyUnit(aUnitName, 
                                            aUnitShortName, 
                                            aUnitExternalID);
  propertyUnit u = { unit, aUnitInternalID };
  mUnits.push_back(u);
  nsString key(aUnitExternalID);
  mUnitsMap[key] = u;
}

// Unit conversion function
NS_IMETHODIMP
sbPropertyUnitConverter::Convert(const nsAString & aValue, 
                                 const nsAString & aFromUnitID, 
                                 const nsAString & aToUnitID,
                                 nsAString & _retval) 
{
  sbSimpleAutoLock lock(mLock);
  
  // If converting to and from the same unit, there is nothing to do
  if (aFromUnitID.Equals(aToUnitID)) {
    _retval = aValue;
    return NS_OK;    
  }

  // look up the 'from' unit in the map
  nsString fromKey(aFromUnitID);
  propertyUnitMap::iterator fromUnitIterator = mUnitsMap.find(fromKey);
  // not found ?
  if (fromUnitIterator == mUnitsMap.end()) 
    return NS_ERROR_INVALID_ARG;
    
  // look up the 'to' unit in the map
  nsString toKey(aToUnitID);
  propertyUnitMap::iterator toUnitIterator = mUnitsMap.find(toKey);
  // not found ?
  if (toUnitIterator == mUnitsMap.end()) 
    return NS_ERROR_INVALID_ARG;

  // grab the internal ids
  PRUint32 fromUnit = (*fromUnitIterator).second.mInternalId;
  PRUint32 toUnit = (*toUnitIterator).second.mInternalId;
  
  PRFloat64 converted=0;

  // parse the string as a double value
  NS_ConvertUTF16toUTF8 narrow(aValue);
  if(PR_sscanf(narrow.get(), gsFmtFloatIn, &converted) != 1) {
    // wrong format, or empty string
    return NS_ERROR_INVALID_ARG;
  }
  
  nsresult rv;

  // convert into native unit
  rv = ConvertFromUnitToNative(converted, fromUnit, converted);
  NS_ENSURE_SUCCESS(rv, rv);

  // convert into requested unit
  rv = ConvertFromNativeToUnit(converted, toUnit, converted);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // turn back into a string
  char out[64] = {0};
  if(PR_snprintf(out, 63, gsFmtFloatOut, converted) == -1) {
    rv = NS_ERROR_FAILURE;
    _retval = EmptyString();
  }
  if (rv != NS_ERROR_FAILURE) {
    NS_ConvertUTF8toUTF16 wide(out);
    rv = NS_OK;

    // %f formats 1 as '1.000000', %g would format it as '1' but if the number
    // is big enough, it'd use exponent format, which we don't want either,
    // so use %f and strip trailing zeros if there is a decimal point, then
    // strip the decimal point itself. (depending on the sprintf docs you read,
    // the # flag could do the job, but moz doesn't support that flag because
    // "The ANSI C spec. of the '#' flag is somewhat ambiguous and not ideal")
    PRUint32 decimal = wide.FindChar('.');
    if (decimal != -1) {
      while (wide.CharAt(wide.Length()-1) == '0')
        wide.Cut(wide.Length()-1, 1);
      if (wide.Length() == decimal+1)
        wide.Cut(decimal, 1);
    }

    _retval = wide;
  }

  return rv;
}

// accessor for an nsISimpleEnumerator of all the exposed units for this
// property.
NS_IMETHODIMP sbPropertyUnitConverter::GetUnits(nsISimpleEnumerator **aUnits)
{
  NS_ENSURE_ARG_POINTER(aUnits);
  sbSimpleAutoLock lock(mLock);

  nsCOMArray<sbIPropertyUnit> array;
  propertyUnitList::iterator it = mUnits.begin();
  for(; it != mUnits.end(); it++) {
    propertyUnit u = (*it);
    nsCOMPtr<sbIPropertyUnit> unit = u.mUnit;
    array.AppendObject(unit);
  }

  return NS_NewArrayEnumerator(aUnits, array);
}

// accessor for the id of the native unit used by this property
NS_IMETHODIMP sbPropertyUnitConverter::GetNativeUnitId(nsAString &aNativeUnit)
{
  sbSimpleAutoLock lock(mLock);
  aNativeUnit = mNative;
  return NS_OK;
}
