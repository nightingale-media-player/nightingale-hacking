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

#ifndef __SBPROGRESSPROPERTYINFO_H__
#define __SBPROGRESSPROPERTYINFO_H__

#include "sbNumberPropertyInfo.h"
#include <sbIPropertyManager.h>

#include <nsStringGlue.h>
#include <prlock.h>

class sbProgressPropertyInfo : public sbNumberPropertyInfo,
                               public sbIProgressPropertyInfo
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_SBIPROPERTYINFO(sbNumberPropertyInfo::);
  NS_FORWARD_SBINUMBERPROPERTYINFO(sbNumberPropertyInfo::);
  NS_DECL_SBIPROGRESSPROPERTYINFO

  sbProgressPropertyInfo();

  ~sbProgressPropertyInfo();

private:
  PRLock* mLock;

  nsString mModePropertyName;
};

#endif /* __SBPROGRESSPROPERTYINFO_H__ */
