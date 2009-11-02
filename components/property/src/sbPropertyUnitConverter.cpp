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
#include <nsServiceManagerUtils.h>
#include <sbLockUtils.h>
#include <nsCOMPtr.h>
#include "nsEnumeratorUtils.h"
#include "nsArrayEnumerator.h"
#include <prprf.h>

#include <locale.h>

static const char *gsFmtFloatOut = "%f";
static const char *gsFmtFloatIn = "%lf";

static inline 
PRUnichar GetDecimalPoint() {
  PRUnichar decimalPoint = '.';
  lconv *localInfo = localeconv();
  if(localInfo) {
    decimalPoint = localInfo->decimal_point[0];
  }
    
  return decimalPoint;
}

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
, mName(aName)
, mShortName(aShortName)
, mID(aID)
, mInitialized(PR_TRUE)
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
: mNativeInternal(-1)
, mLock(nsnull)
, mDecimalPoint('.')
{
  mLock = PR_NewLock();
  NS_ASSERTION(mLock, "sbPropertyUnitConverter::mLock failed to create lock!");

  mDecimalPoint = GetDecimalPoint();
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
  if (isNative) {
    mNative = aUnitExternalID;
    mNativeInternal = aUnitInternalID;
  }
  sbPropertyUnit *unit = new sbPropertyUnit(aUnitName, 
                                            aUnitShortName, 
                                            aUnitExternalID);
  propertyUnit u = { unit, aUnitInternalID };
  mUnits.push_back(u);
  nsString key(aUnitExternalID);
  mUnitsMap[key] = u;
  mUnitsMapInternal[aUnitInternalID] = u;
}

nsresult
  sbPropertyUnitConverter::SscanfFloat64(const nsAString &aValue, 
                                         PRFloat64 &aOutValue) {
  // parse the string as a double value
  NS_ConvertUTF16toUTF8 narrow(aValue);
  if(PR_sscanf(narrow.get(), gsFmtFloatIn, &aOutValue) != 1) {
    // wrong format, or empty string
    return NS_ERROR_INVALID_ARG;
  }
  return NS_OK;
}

nsresult
  sbPropertyUnitConverter::SprintfFloat64(const PRFloat64 aValue, 
                                          nsAString &aOutValue) {
  // turn the double value into a string
  char out[64] = {0};
  if(PR_snprintf(out, 63, gsFmtFloatOut, aValue) == -1) {
    aOutValue = EmptyString();
    return NS_ERROR_FAILURE;
  }
  NS_ConvertUTF8toUTF16 wide(out);
  aOutValue = wide;
  return NS_OK;
}

nsresult sbPropertyUnitConverter::PerformConversion(PRFloat64 &aValue,
                                                    PRUint32 aFromUnit,
                                                    PRUint32 aToUnit) {
  // convert into native unit
  nsresult rv = ConvertFromUnitToNative(aValue, aFromUnit, aValue);
  NS_ENSURE_SUCCESS(rv, rv);

  // convert into requested unit
  rv = ConvertFromNativeToUnit(aValue, aToUnit, aValue);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

// Unit conversion function
NS_IMETHODIMP
sbPropertyUnitConverter::Convert(const nsAString & aValue, 
                                 const nsAString & aFromUnitID, 
                                 const nsAString & aToUnitID,
                                 PRInt32 aMinDecimals,
                                 PRInt32 aMaxDecimals,
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
  
  PRFloat64 floatValue;
  nsresult rv = SscanfFloat64(aValue, floatValue);
  NS_ENSURE_SUCCESS(rv, rv);

  PerformConversion(floatValue, fromUnit, toUnit);

  nsAutoString out;
  rv = SprintfFloat64(floatValue, out);
  NS_ENSURE_SUCCESS(rv, rv);
  
  ApplyDecimalLimits(out, aMinDecimals, aMaxDecimals);

  _retval = out;

  return rv;
}

// iteratively remove all trailing zeroes, and the period if necessary
void sbPropertyUnitConverter::RemoveTrailingZeroes(nsAString &aValue) {
  PRUint32 decimal = aValue.FindChar(mDecimalPoint);
  if (decimal != -1) {
    while (aValue.CharAt(aValue.Length()-1) == '0')
      aValue.Cut(aValue.Length()-1, 1);
    if (aValue.Length() == decimal+1)
      aValue.Cut(decimal, 1);
  }
}

// limit precision of a value to N decimals
void sbPropertyUnitConverter::LimitToNDecimals(nsAString &aValue,
                                               PRUint32 aDecimals) {
  PRUint32 decimal = aValue.FindChar(mDecimalPoint);
  if (decimal != -1) {
    PRUint32 p = decimal + aDecimals;
    if (aValue.Length() > p+1) {
      aValue.Cut(p+1, aValue.Length()-1-p);
    }
  }
}

// force at least N decimals
void sbPropertyUnitConverter::ForceToNDecimals(nsAString &aValue,
                                               PRUint32 aDecimals) {
  PRUint32 decimal = aValue.FindChar(mDecimalPoint);
  if (decimal == -1) {
    aValue += mDecimalPoint;
    decimal = aValue.Length()-1;
  }
  PRUint32 n = aValue.Length() - decimal - 1;
  for (;n<aDecimals;n++) {
    aValue += NS_LITERAL_STRING("0");
  }
}

// depending on the sprintf docs you read, the # flag could do the job of this
// function, but moz doesn't support that flag because "The ANSI C spec. of the
// '#' flag is somewhat ambiguous and not ideal".
void sbPropertyUnitConverter::ApplyDecimalLimits(nsAString &aValue,
                                                 PRInt32 aMinDecimals,
                                                 PRInt32 aMaxDecimals) {
  // strip to at most N decimals
  if (aMaxDecimals != -1) {
    LimitToNDecimals(aValue, aMaxDecimals);
  }
  
  // remove trailing zeroes
  RemoveTrailingZeroes(aValue);
  
  // ensure there are at least N decimals
  if (aMinDecimals != -1) {
    ForceToNDecimals(aValue, aMinDecimals);
  }
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

// url to the string bundle used to localize partial entities
NS_IMETHODIMP sbPropertyUnitConverter::GetStringBundle(nsAString &aStringBundle)
{
  sbSimpleAutoLock lock(mLock);
  aStringBundle = mStringBundle;
  return NS_OK;
}

// sets the url to the string bundle -- not part of the interface, called by inheritors
void sbPropertyUnitConverter::SetStringBundle(const nsAString &aStringBundle) {
  mStringBundle = aStringBundle;
}

// auto formats a value using the most suitable unit. this is the default
// implementation, which simply calls the property info's format function
NS_IMETHODIMP 
sbPropertyUnitConverter::AutoFormat(const nsAString &aValue,
                                    PRInt32 aMinDecimals,
                                    PRInt32 aMaxDecimals,
                                    nsAString &_retval) {
  if (!mPropertyInfo) 
    return NS_ERROR_NOT_INITIALIZED;

  sbSimpleAutoLock lock(mLock);

  // parse as number
  PRFloat64 v;
  nsresult rv = SscanfFloat64(aValue, v);
  
  // It is okay to fail parsing, just default to propertyinfo.Format
  if (rv != NS_OK) {
    nsCOMPtr<sbIPropertyInfo> propInfo = do_QueryReferent(mPropertyInfo, &rv);
    if (NS_FAILED(rv) || !propInfo)
      return NS_ERROR_FAILURE;
    return propInfo->Format(aValue, _retval);
  }
  
  // request the most suited unit for this number, implemented by inheritor
  PRInt32 autoUnit = GetAutoUnit(v);
  
  // if not implemented by inheritor, default implementation returns -1, 
  // for 'not supported'.
  if (autoUnit < 0) {
    // in which case we just format using the property info native unit
    nsCOMPtr<sbIPropertyInfo> propInfo = do_QueryReferent(mPropertyInfo, &rv);
    if (NS_FAILED(rv) || !propInfo)
      return NS_ERROR_FAILURE;
    
    nsresult rv = propInfo->Format(aValue, _retval);
    NS_ENSURE_SUCCESS(rv, rv);
    
    return NS_OK;
  }
  
  // otherwise, format using that unit
  PerformConversion(v, mNativeInternal, autoUnit);
  
  nsAutoString out;
  SprintfFloat64(v, out);
  
  ApplyDecimalLimits(out, aMinDecimals, aMaxDecimals);

  // if we formatted to a particular unit, append the unit locale
  if (autoUnit >= 0) {
    out += NS_LITERAL_STRING(" ");
    
    // get the string for this unit
    propertyUnitMapInternal::iterator 
      toUnitIterator = mUnitsMapInternal.find(autoUnit);

    if (toUnitIterator == mUnitsMapInternal.end()) 
      return NS_ERROR_FAILURE;

    propertyUnit u = (*toUnitIterator).second;
    nsCOMPtr<sbIPropertyUnit> unit = u.mUnit;
    
    nsString shortName;
    nsresult rv = unit->GetShortName(shortName);
    NS_ENSURE_SUCCESS(rv, rv);
    
    // if this is a partial entity, translate it
    if (shortName.First() == '&' &&
        shortName.CharAt(shortName.Length()-1) != ';') {
       shortName.Cut(0, 1);

      // get bundle if we haven't done so yet
      if (!mStringBundleObject) {
        nsCOMPtr<nsIStringBundleService> stringBundleService = 
          do_GetService("@mozilla.org/intl/stringbundle;1", &rv);

        NS_ConvertUTF16toUTF8 url(mStringBundle);
        rv = stringBundleService->CreateBundle(url.get(),
                                               getter_AddRefs(mStringBundleObject));
        NS_ENSURE_SUCCESS(rv, rv);
      }

      // localize string
      nsString str;
      rv = mStringBundleObject->GetStringFromName(shortName.get(),
                                                  getter_Copies(str));
      NS_ENSURE_SUCCESS(rv, rv);

      // and append it
      out += str;
    } else {
      // no localization needed, just append the shortName value
      out += shortName;
    }
  }

  _retval = out;
  
  return NS_OK;
}

// not exposed to the interface, this function allows the propertyinfo that
// instantiates this converter to register itself into it
NS_IMETHODIMP
sbPropertyUnitConverter::SetPropertyInfo(sbIPropertyInfo *aPropInfo) {
  sbSimpleAutoLock lock(mLock);

  if (mPropertyInfo) 
    return NS_ERROR_ALREADY_INITIALIZED;

  nsresult rv;
  mPropertyInfo = do_GetWeakReference(aPropInfo, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

// returns the propertyinfo object that owns this converter
NS_IMETHODIMP 
sbPropertyUnitConverter::GetPropertyInfo(sbIPropertyInfo **_retval) {
  NS_ENSURE_ARG_POINTER(_retval);

  sbSimpleAutoLock lock(mLock);
  
  if (!mPropertyInfo) 
    return NS_ERROR_NOT_INITIALIZED;
  
  nsresult rv;
  nsCOMPtr<sbIPropertyInfo> propInfo = do_QueryReferent(mPropertyInfo, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = propInfo;
  
  return NS_OK;
}

