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

#ifndef __SBPROPERTYUNITCONVERTER_H__
#define __SBPROPERTYUNITCONVERTER_H__

#include "sbIPropertyUnitConverter.h"
#include "sbIPropertyManager.h"
#include <nsIStringBundle.h>
#include <nsStringGlue.h>
#include <nsIWeakReference.h>
#include <nsIWeakReferenceUtils.h>
#include <nsCOMArray.h>
#include <nsCOMPtr.h>
#include <map>
#include <list>

// sbPropertyUnit class - describes a unit exposed by sbIPropertyUnitConverter
class sbPropertyUnit : public sbIPropertyUnit {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIPROPERTYUNIT

  sbPropertyUnit();
  sbPropertyUnit(const nsAString& aName, 
                 const nsAString& aShortName, 
                 const nsAString& aID);
  virtual ~sbPropertyUnit();

  NS_IMETHODIMP Init(const nsAString & aName, 
                     const nsAString & aShortName, 
                     const nsAString &aID);

protected:
  
  PRLock *mLock;
  nsString mName;
  nsString mShortName;
  nsString mID;
  PRBool mInitialized;
};

// sbPropertyUnitConverter class - exposes the units and the conversion function
class sbPropertyUnitConverter : public sbIPropertyUnitConverter {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIPROPERTYUNITCONVERTER
  
  sbPropertyUnitConverter();
  virtual ~sbPropertyUnitConverter();

protected:
  void RegisterUnit(PRUint32 aUnitInternalID, 
                    const nsAString &aUnitExternalID, 
                    const nsAString &aUnitName, 
                    const nsAString &aUnitShortName, 
                    PRBool isNative = PR_FALSE);

  void SetStringBundle(const nsAString &aStringBundle);


  NS_IMETHOD ConvertFromNativeToUnit(PRFloat64 aValue, 
                                     PRUint32 aUnitID, 
                                     PRFloat64 &_retVal)=0;
  NS_IMETHOD ConvertFromUnitToNative(PRFloat64 aValue, 
                                     PRUint32 aUnitID, 
                                     PRFloat64 &_retVal)=0;

  virtual PRInt32 GetAutoUnit(PRFloat64 aValue) { return -1; }
  nsresult PerformConversion(PRFloat64 &aValue,
                             PRUint32 aFromUnit,
                             PRUint32 aToUnit);
  
  void RemoveTrailingZeroes(nsAString &aValue);
  void LimitToNDecimals(nsAString &aValue, PRUint32 aDecimals);
  void ForceToNDecimals(nsAString &aValue, PRUint32 aDecimals);
  void ApplyDecimalLimits(nsAString &aValue, 
                          PRInt32 aMinDecimals, 
                          PRInt32 aMaxDecimals);
  
  PRLock* mLock;
  nsString mNative;
  PRUint32 mNativeInternal;
  nsString mStringBundle;
  nsCOMPtr<nsIStringBundleService> mStringBundleService;
  nsCOMPtr<nsIStringBundle> mStringBundleObject;

  typedef struct {
    nsCOMPtr<sbIPropertyUnit> mUnit;
    PRUint32 mInternalId;
  } propertyUnit;

  class propertyUnitMap : public std::map<nsString, propertyUnit> {};
  class propertyUnitMapInternal : public std::map<PRUint32, propertyUnit> {};
  class propertyUnitList : public std::list<propertyUnit> {};
  propertyUnitMap mUnitsMap;
  propertyUnitMapInternal mUnitsMapInternal;
  propertyUnitList mUnits;

  PRUnichar mDecimalPoint;
  
  nsCOMPtr<nsIWeakReference> mPropertyInfo;
  nsresult SscanfFloat64(const nsAString &aValue, PRFloat64 &aOutValue);
  nsresult SprintfFloat64(const PRFloat64 aValue, nsAString &aOutValue);
};

#endif // __SBPROPERTYUNITCONVERTER_H__
