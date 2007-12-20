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

#ifndef __SB_PROPERTYARRAY_H__
#define __SB_PROPERTYARRAY_H__

#include <sbIPropertyArray.h>

#include <nsAutoLock.h>
#include <nsCOMArray.h>
#include <nsCOMPtr.h>
#include <nsIArray.h>
#include <nsIClassInfo.h>
#include <nsIMutableArray.h>
#include <nsISerializable.h>

class sbIProperty;
class sbIPropertyManager;

class sbPropertyArray : public sbIMutablePropertyArray,
                        public nsIMutableArray,
                        public nsISerializable,
                        public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIARRAY
  NS_DECL_NSICLASSINFO
  NS_DECL_NSIMUTABLEARRAY
  NS_DECL_NSISERIALIZABLE
  NS_DECL_SBIPROPERTYARRAY
  NS_DECL_SBIMUTABLEPROPERTYARRAY

  sbPropertyArray();
  ~sbPropertyArray();

  nsresult Init();

private:
  nsresult PropertyIsValid(sbIProperty* aProperty,
                           PRBool* _retval);
  nsresult ValueIsValid(const nsAString& aID,
                        const nsAString& aValue,
                        PRBool* _retval);

private:
  nsCOMArray<sbIProperty> mArray;
  nsCOMPtr<sbIPropertyManager> mPropManager;
  PRLock* mArrayLock;
  PRBool mStrict;
};

#endif /* __SB_PROPERTYARRAY_H__ */
