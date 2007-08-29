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

#ifndef __SBBOOLEANPROPERTYINFO_H__
#define __SBBOOLEANPROPERTYINFO_H__

#include <sbIPropertyManager.h>
#include "sbPropertyInfo.h"

#include <nsCOMPtr.h>
#include <nsStringGlue.h>

class sbBooleanPropertyInfo : public sbPropertyInfo,
                              public sbIBooleanPropertyInfo
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_SBIPROPERTYINFO_NOVALIDATE_NOFORMAT(sbPropertyInfo::);
  NS_DECL_SBIBOOLEANPROPERTYINFO

  sbBooleanPropertyInfo();
  virtual ~sbBooleanPropertyInfo();

  nsresult Init();

  nsresult InitializeOperators();

  NS_IMETHOD Validate(const nsAString & aValue, PRBool *_retval);
  NS_IMETHOD Sanitize(const nsAString & aValue, nsAString & _retval);
  NS_IMETHOD Format(const nsAString & aValue, nsAString & _retval);
  NS_IMETHOD MakeSortable(const nsAString & aValue, nsAString & _retval);
};

#endif /* __SBBOOLEANPROPERTYINFO_H__ */
