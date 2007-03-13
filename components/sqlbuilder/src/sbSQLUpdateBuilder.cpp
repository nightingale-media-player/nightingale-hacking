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

#include "sbSQLUpdateBuilder.h"
#include "sbSQLWhereBuilder.h"
#include "sbSQLBuilderCriterion.h"

NS_IMPL_ISUPPORTS_INHERITED1(sbSQLUpdateBuilder,
                             sbSQLWhereBuilder,
                             sbISQLUpdateBuilder)

sbSQLUpdateBuilder::sbSQLUpdateBuilder()
{
}

NS_IMETHODIMP
sbSQLUpdateBuilder::GetTableName(nsAString& aTableName)
{
  aTableName = mTableName;
  return NS_OK;
}
NS_IMETHODIMP
sbSQLUpdateBuilder::SetTableName(const nsAString& aTableName)
{
  mTableName = aTableName;
  return NS_OK;
}

NS_IMETHODIMP
sbSQLUpdateBuilder::AddAssignmentString(const nsAString& aColumnName,
                                        const nsAString& aValue)
{
  Assignment* a = mAssignments.AppendElement();
  NS_ENSURE_TRUE(a, NS_ERROR_OUT_OF_MEMORY);

  a->type        = eString;
  a->columnName  = aColumnName;
  a->stringValue = aValue;

  return NS_OK;
}

NS_IMETHODIMP
sbSQLUpdateBuilder::AddAssignmentParameter(const nsAString& aColumnName)
{
  Assignment* a = mAssignments.AppendElement();
  NS_ENSURE_TRUE(a, NS_ERROR_OUT_OF_MEMORY);

  a->type        = eParameter;
  a->columnName  = aColumnName;

  return NS_OK;
}

NS_IMETHODIMP
sbSQLUpdateBuilder::Reset()
{
  sbSQLWhereBuilder::Reset();
  mTableName.Truncate();
  mAssignments.Clear();
  return NS_OK;
}

NS_IMETHODIMP
sbSQLUpdateBuilder::ToString(nsAString& _retval)
{
  nsresult rv;
  nsAutoString buff;

  buff.AssignLiteral("update ");

  buff.Append(mTableName);

  buff.AppendLiteral(" set ");

  PRUint32 len = mAssignments.Length();
  for (PRUint32 i = 0; i < len; i++) {
    const Assignment& a = mAssignments[i];

    buff.Append(a.columnName);
    buff.AppendLiteral(" = ");

    switch(a.type) {
      case eIsNull:
        buff.AppendLiteral("null");
        break;
      case eString:
      {
        nsAutoString escapedValue(a.stringValue);
        SB_EscapeSQL(escapedValue);

        buff.AppendLiteral("'");
        buff.Append(escapedValue);
        buff.AppendLiteral("'");
        break;
      }
      case eInteger32:
        buff.AppendInt(a.int32Value);
        break;
      case eParameter:
        buff.AppendLiteral("?");
        break;
    }

    if (i + 1 < len) {
      buff.AppendLiteral(", ");
    }
  }

  rv = AppendWhere(buff);
  NS_ENSURE_SUCCESS(rv, rv);

  _retval.Assign(buff);
  return NS_OK;
}

