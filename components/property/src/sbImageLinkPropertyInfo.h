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

#ifndef __SBIMAGELINKPROPERTYINFO_H__
#define __SBIMAGELINKPROPERTYINFO_H__

#include "sbImmutablePropertyInfo.h"
#include "sbIImageLinkPropertyInfo.h"

#include <sbIPropertyManager.h>
#include <sbITreeViewPropertyInfo.h>
#include <sbIClickablePropertyInfo.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>

class sbImageLinkPropertyInfo : public sbImmutablePropertyInfo,
                                public sbIImageLinkPropertyInfo,
                                public sbIClickablePropertyInfo,
                                public sbITreeViewPropertyInfo
{
public:

  NS_DECL_ISUPPORTS
  NS_DECL_SBIIMAGELINKPROPERTYINFO
  NS_DECL_SBICLICKABLEPROPERTYINFO
  NS_DECL_SBITREEVIEWPROPERTYINFO

  sbImageLinkPropertyInfo(const nsAString& aPropertyID,
                          const nsAString& aDisplayName,
                          const nsAString& aLocalizationKey,
                          const bool aRemoteReadable,
                          const bool aRemoteWritable,
                          const bool aUserViewable,
                          const bool aUserEditable,
                          const nsAString& aUrlPropertyID);

  NS_IMETHOD Format(const nsAString& aValue, nsAString& _retval);

  nsresult Init();
  virtual ~sbImageLinkPropertyInfo() {}
  
protected:
  nsString mUrlPropertyID;
  bool mSuppressSelect;
};

#endif /* __SBIMAGELINKPROPERTYINFO_H__ */

