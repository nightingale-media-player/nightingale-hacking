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

#ifndef __SBSQLINSERTBUILDER_H__
#define __SBSQLINSERTBUILDER_H__

#include <sbISQLBuilder.h>
#include "sbSQLBuilderBase.h"

#include <nsStringGlue.h>
#include <nsCOMPtr.h>

class sbSQLInsertBuilder : public sbSQLBuilderBase,
                           public sbISQLInsertBuilder
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  // defined in sbSQLBuilderBase.h
  NS_FORWARD_SBISQLBUILDER_WITHOUT_TOSTRING_RESET(sbSQLBuilderBase::)
  NS_DECL_SBISQLINSERTBUILDER

  // override sbISQLBuilder::ToString and sbISQLBuilder::Reset
  NS_IMETHOD ToString(nsAString& _retval);
  NS_IMETHOD Reset();

private:
  enum ParameterType {
    eIsNull,
    eString,
    eInteger32,
    eIsParameter
  };

  struct sbValueItem
  {
    ParameterType type;
    nsString stringValue;
    PRInt32 int32Value;
  };

  nsTArray<sbColumnInfo> mOutputColumns;
  nsTArray<sbValueItem> mValueList;

  nsCOMPtr<sbISQLSelectBuilder> mSelect;

  nsString mIntoTableName;
};

#endif /* __SBSQLINSERTBUILDER_H__ */

