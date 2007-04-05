/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

#include <sbIPropertyManager.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>

#include <nsIURI.h>

#define NS_FORWARD_SBIPROPERTYINFO_NOVALIDATE_NOFORMAT(_to) \
NS_IMETHOD GetNullSort(PRUint32 *aNullSort) { return _to GetNullSort(aNullSort); } \
NS_IMETHOD SetNullSort(PRUint32 aNullSort) { return _to SetNullSort(aNullSort); } \
NS_IMETHOD GetName(nsAString & aName) { return _to GetName(aName); } \
NS_IMETHOD SetName(const nsAString & aName) { return _to SetName(aName); } \
NS_IMETHOD GetType(nsAString & aType) { return _to GetType(aType); } \
NS_IMETHOD SetType(const nsAString & aType) { return _to SetType(aType); } \
NS_IMETHOD GetDisplayKey(nsAString & aDisplayKey) { return _to GetDisplayKey(aDisplayKey); } \
NS_IMETHOD SetDisplayKey(const nsAString & aDisplayKey) { return _to SetDisplayKey(aDisplayKey); } \
NS_IMETHOD GetDisplayUsingSimpleType(nsAString & aDisplayUsingSimpleType) { return _to GetDisplayUsingSimpleType(aDisplayUsingSimpleType); } \
NS_IMETHOD SetDisplayUsingSimpleType(const nsAString & aDisplayUsingSimpleType) { return _to SetDisplayUsingSimpleType(aDisplayUsingSimpleType); } \
NS_IMETHOD GetDisplayUsingXBLWidget(nsIURI * *aDisplayUsingXBLWidget) { return _to GetDisplayUsingXBLWidget(aDisplayUsingXBLWidget); } \
NS_IMETHOD SetDisplayUsingXBLWidget(nsIURI * aDisplayUsingXBLWidget) { return _to SetDisplayUsingXBLWidget(aDisplayUsingXBLWidget); } \
NS_IMETHOD GetUnits(nsAString & aUnits) { return _to GetUnits(aUnits); } \
NS_IMETHOD SetUnits(const nsAString & aUnits) { return _to SetUnits(aUnits); }

#define SB_IPROPERTYINFO_CAST(__unambiguousBase, __expr) \
  NS_STATIC_CAST(sbIPropertyInfo*, NS_STATIC_CAST(__unambiguousBase, __expr))

class sbPropertyInfo : public sbIPropertyInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIPROPERTYINFO

  sbPropertyInfo();
  virtual ~sbPropertyInfo();

protected:
  PRUint32  mNullSort;

  PRLock*   mNameLock;
  nsString  mName;

  PRLock*   mTypeLock;
  nsString  mType;

  PRLock*   mDisplayKeyLock;
  nsString  mDisplayKey;

  PRLock*   mDisplayUsingSimpleTypeLock;
  nsString  mDisplayUsingSimpleType;
  
  PRLock*          mDisplayUsingXBLWidgetLock;
  nsCOMPtr<nsIURI> mDisplayUsingXBLWidget;

  PRLock*   mUnitsLock;
  nsString  mUnits;

};

#endif /* __SBPROPERTYINFO_H__ */
