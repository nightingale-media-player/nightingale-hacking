// vim: set sw=2 : */
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

#ifndef __SBPROPERTYINFO_H__
#define __SBPROPERTYINFO_H__

#include <sbIPropertyInfo.h>
#include <sbIPropertyArray.h>
#include "sbPropertyUnitConverter.h"

#include <nsCOMPtr.h>
#include <nsIURI.h>
#include <nsStringGlue.h>
#include <nsCOMArray.h>
#include <nsWeakReference.h>
#include "sbPropertyOperator.h"

#define NS_FORWARD_SBIPROPERTYINFO_NOSPECIFICS(_to) \
NS_IMETHOD GetOPERATOR_EQUALS(nsAString & aOPERATOR_EQUALS) { return _to GetOPERATOR_EQUALS(aOPERATOR_EQUALS); } \
NS_IMETHOD GetOPERATOR_NOTEQUALS(nsAString & aOPERATOR_NOTEQUALS) { return _to GetOPERATOR_NOTEQUALS(aOPERATOR_NOTEQUALS); } \
NS_IMETHOD GetOPERATOR_GREATER(nsAString & aOPERATOR_GREATER) { return _to GetOPERATOR_GREATER(aOPERATOR_GREATER); } \
NS_IMETHOD GetOPERATOR_GREATEREQUAL(nsAString & aOPERATOR_GREATEREQUAL) { return _to GetOPERATOR_GREATEREQUAL(aOPERATOR_GREATEREQUAL); } \
NS_IMETHOD GetOPERATOR_LESS(nsAString & aOPERATOR_LESS) { return _to GetOPERATOR_LESS(aOPERATOR_LESS); } \
NS_IMETHOD GetOPERATOR_LESSEQUAL(nsAString & aOPERATOR_LESSEQUAL) { return _to GetOPERATOR_LESSEQUAL(aOPERATOR_LESSEQUAL); } \
NS_IMETHOD GetOPERATOR_CONTAINS(nsAString & aOPERATOR_CONTAINS) { return _to GetOPERATOR_CONTAINS(aOPERATOR_CONTAINS); } \
NS_IMETHOD GetOPERATOR_NOTCONTAINS(nsAString & aOPERATOR_NOTCONTAINS) { return _to GetOPERATOR_NOTCONTAINS(aOPERATOR_NOTCONTAINS); } \
NS_IMETHOD GetOPERATOR_BEGINSWITH(nsAString & aOPERATOR_BEGINSWITH) { return _to GetOPERATOR_BEGINSWITH(aOPERATOR_BEGINSWITH); } \
NS_IMETHOD GetOPERATOR_NOTBEGINSWITH(nsAString & aOPERATOR_NOTBEGINSWITH) { return _to GetOPERATOR_NOTBEGINSWITH(aOPERATOR_NOTBEGINSWITH); } \
NS_IMETHOD GetOPERATOR_ENDSWITH(nsAString & aOPERATOR_ENDSWITH) { return _to GetOPERATOR_ENDSWITH(aOPERATOR_ENDSWITH); } \
NS_IMETHOD GetOPERATOR_NOTENDSWITH(nsAString & aOPERATOR_NOTENDSWITH) { return _to GetOPERATOR_NOTENDSWITH(aOPERATOR_NOTENDSWITH); } \
NS_IMETHOD GetOPERATOR_BETWEEN(nsAString & aOPERATOR_BETWEEN) { return _to GetOPERATOR_BETWEEN(aOPERATOR_BETWEEN); } \
NS_IMETHOD GetOPERATOR_ISSET(nsAString & aOPERATOR_ISSET) { return _to GetOPERATOR_ISSET(aOPERATOR_ISSET); } \
NS_IMETHOD GetOPERATOR_ISNOTSET(nsAString & aOPERATOR_ISNOTSET) { return _to GetOPERATOR_ISNOTSET(aOPERATOR_ISNOTSET); } \
NS_IMETHOD GetNullSort(PRUint32 *aNullSort) { return _to GetNullSort(aNullSort); } \
NS_IMETHOD SetNullSort(PRUint32 aNullSort) { return _to SetNullSort(aNullSort); } \
NS_IMETHOD GetSecondarySort(sbIPropertyArray * *aSecondarySort) { return _to GetSecondarySort(aSecondarySort); } \
NS_IMETHOD SetSecondarySort(sbIPropertyArray * aSecondarySort) { return _to SetSecondarySort(aSecondarySort); } \
NS_IMETHOD GetId(nsAString & aID) { return _to GetId(aID); } \
NS_IMETHOD SetId(const nsAString & aID) { return _to SetId(aID); } \
NS_IMETHOD GetType(nsAString & aType) { return _to GetType(aType); } \
NS_IMETHOD SetType(const nsAString & aType) { return _to SetType(aType); } \
NS_IMETHOD GetDisplayName(nsAString & aDisplayName) { return _to GetDisplayName(aDisplayName); } \
NS_IMETHOD SetDisplayName(const nsAString & aDisplayName) { return _to SetDisplayName(aDisplayName); } \
NS_IMETHOD GetLocalizationKey(nsAString & aLocalizationKey) { return _to GetLocalizationKey(aLocalizationKey); } \
NS_IMETHOD SetLocalizationKey(const nsAString & aLocalizationKey) { return _to SetLocalizationKey(aLocalizationKey); } \
NS_IMETHOD GetUserViewable(PRBool *aUserViewable) { return _to GetUserViewable(aUserViewable); } \
NS_IMETHOD SetUserViewable(PRBool aUserViewable) { return _to SetUserViewable(aUserViewable); } \
NS_IMETHOD GetUserEditable(PRBool *aUserEditable) { return _to GetUserEditable(aUserEditable); } \
NS_IMETHOD SetUserEditable(PRBool aUserEditable) { return _to SetUserEditable(aUserEditable); } \
NS_IMETHOD GetRemoteReadable(PRBool *aRemoteReadable) { return _to GetRemoteReadable(aRemoteReadable); } \
NS_IMETHOD SetRemoteReadable(PRBool aRemoteReadable) { return _to SetRemoteReadable(aRemoteReadable); } \
NS_IMETHOD GetRemoteWritable(PRBool *aRemoteWritable) { return _to GetRemoteWritable(aRemoteWritable); } \
NS_IMETHOD SetRemoteWritable(PRBool aRemoteWritable) { return _to SetRemoteWritable(aRemoteWritable); } \
NS_IMETHOD GetOperators(nsISimpleEnumerator * *aOperators) { return _to GetOperators(aOperators); } \
NS_IMETHOD SetOperators(nsISimpleEnumerator * aOperators) { return _to SetOperators(aOperators); } \
NS_IMETHOD GetOperator(const nsAString & aOperator, sbIPropertyOperator * *_retval) { return _to GetOperator(aOperator, _retval); } \
NS_IMETHOD SetUnitConverter(sbIPropertyUnitConverter *aUnitConverter) { return _to SetUnitConverter(aUnitConverter); } \
NS_IMETHOD GetUnitConverter(sbIPropertyUnitConverter **retVal) { return _to GetUnitConverter(retVal); }

#define NS_FORWARD_SBIPROPERTYINFO_MAKESORTABLE(_to) \
NS_IMETHOD MakeSortable(const nsAString &aValue, nsAString &retVal) { return _to MakeSortable(aValue, retVal); }

#define NS_FORWARD_SBIPROPERTYINFO_STDPROP(_to) \
NS_FORWARD_SBIPROPERTYINFO_NOSPECIFICS(_to) \
NS_FORWARD_SBIPROPERTYINFO_MAKESORTABLE(_to)

#define SB_IPROPERTYINFO_CAST(__unambiguousBase, __expr) \
  static_cast<sbIPropertyInfo*>(static_cast<__unambiguousBase>(__expr))

class sbPropertyInfo : public sbIPropertyInfo, public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIPROPERTYINFO

  sbPropertyInfo();
  virtual ~sbPropertyInfo();

  NS_IMETHOD SetUnitConverter(sbIPropertyUnitConverter *aUnitConverter);

  nsresult Init();

protected:

  PRUint32  mNullSort;

  PRLock*   mSecondarySortLock;
  nsCOMPtr<sbIPropertyArray> mSecondarySort;

  PRLock*   mIDLock;
  nsString  mID;

  PRLock*   mTypeLock;
  nsString  mType;

  PRLock*   mDisplayNameLock;
  nsString  mDisplayName;

  PRLock*   mLocalizationKeyLock;
  nsString  mLocalizationKey;

  PRLock*   mUserViewableLock;
  PRBool    mUserViewable;

  PRLock*   mUserEditableLock;
  PRBool    mUserEditable;

  PRLock*   mOperatorsLock;
  nsCOMArray<sbIPropertyOperator> mOperators;
  
  PRLock*   mRemoteReadableLock;
  PRBool    mRemoteReadable;
  
  PRLock*   mRemoteWritableLock;
  PRBool    mRemoteWritable;
  
  PRLock*   mUnitConverterLock;
  nsCOMPtr<sbIPropertyUnitConverter> mUnitConverter;
};

#endif /* __SBPROPERTYINFO_H__ */
