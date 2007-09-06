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

#ifndef __SBIMMUTABLEPROPERTYINFO_H__
#define __SBIMMUTABLEPROPERTYINFO_H__

#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>

#include <nsIStringBundle.h>
#include <nsCOMPtr.h>
#include <nsStringGlue.h>

class sbImmutablePropertyInfo : public sbIPropertyInfo
{
public:

  NS_DECL_ISUPPORTS
  NS_DECL_SBIPROPERTYINFO

  sbImmutablePropertyInfo();

protected:
  nsresult Init();

  PRUint32 mNullSort;
  nsCOMPtr<sbIPropertyArray> mSortProfile;
  nsString mName;
  nsString mType;
  nsString mDisplayName;
  PRBool mUserViewable;
  PRBool mUserEditable;
  nsString mUnits;
  nsCOMPtr<nsIStringBundle> mBundle;
};

#endif /* __SBIMMUTABLEPROPERTYINFO_H__ */

