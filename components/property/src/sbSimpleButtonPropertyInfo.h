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

#ifndef __SBSIMPLEBUTTONPROPERTYINFO_H__
#define __SBSIMPLEBUTTONPROPERTYINFO_H__

#include "sbImmutablePropertyInfo.h"

#include <sbIPropertyManager.h>
#include <sbITreeViewPropertyInfo.h>
#include <sbIClickablePropertyInfo.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>

#include <stdio.h>

class sbSimpleButtonPropertyInfo : public sbImmutablePropertyInfo,
                                   public sbIClickablePropertyInfo,
                                   public sbITreeViewPropertyInfo
{
public:

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_SBICLICKABLEPROPERTYINFO
  NS_DECL_SBITREEVIEWPROPERTYINFO

  sbSimpleButtonPropertyInfo(const nsAString& aPropertyID,
                             const nsAString& aDisplayName,
                             PRBool aHasLabel,
                             const nsAString& aLabel,
                             const PRBool aRemoteReadable,
                             const PRBool aRemoteWritable,
                             const PRBool aUserViewable,
                             const PRBool aUserEditable);

  NS_IMETHOD Format(const nsAString& aValue, nsAString& _retval);

  nsresult Init();

private:

  PRBool mHasLabel;
  nsString mLabel;
};

#endif /* __SBSIMPLEBUTTONPROPERTYINFO_H__ */

