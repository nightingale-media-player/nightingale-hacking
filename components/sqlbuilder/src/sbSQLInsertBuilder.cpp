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

#include "sbSQLInsertBuilder.h"

NS_IMPL_ISUPPORTS_INHERITED1(sbSQLInsertBuilder,
                           sbSQLBuilderBase,
                           sbISQLInsertBuilder)

NS_IMETHODIMP
sbSQLInsertBuilder::AddColumn(const nsAString& aColumnName)
{
  sbColumnInfo* ci = mOutputColumns.AppendElement();
  NS_ENSURE_TRUE(ci, NS_ERROR_OUT_OF_MEMORY);

  ci->tableName  = EmptyString();
  ci->columnName = aColumnName;

  return NS_OK;
}

NS_IMETHODIMP
sbSQLInsertBuilder::AddValueString(const nsAString& aValue)
{
  sbValueItem* vi = mValueList.AppendElement();
  NS_ENSURE_TRUE(vi, NS_ERROR_OUT_OF_MEMORY);

  vi->type        = eString;
  vi->stringValue = aValue;

  return NS_OK;
}

NS_IMETHODIMP
sbSQLInsertBuilder::AddValueLong(PRInt32 aValue)
{
  sbValueItem* vi = mValueList.AppendElement();
  NS_ENSURE_TRUE(vi, NS_ERROR_OUT_OF_MEMORY);

  vi->type       = eInteger32;
  vi->int32Value = aValue;

  return NS_OK;
}

NS_IMETHODIMP
sbSQLInsertBuilder::AddValueNull()
{
  sbValueItem* vi = mValueList.AppendElement();
  NS_ENSURE_TRUE(vi, NS_ERROR_OUT_OF_MEMORY);

  vi->type = eIsNull;

  return NS_OK;
}

NS_IMETHODIMP
sbSQLInsertBuilder::AddValueParameter()
{
  sbValueItem* vi = mValueList.AppendElement();
  NS_ENSURE_TRUE(vi, NS_ERROR_OUT_OF_MEMORY);

  vi->type = eIsParameter;

  return NS_OK;
}

NS_IMETHODIMP
sbSQLInsertBuilder::GetIntoTableName(nsAString& aIntoTableName)
{
  aIntoTableName = mIntoTableName;
  return NS_OK;
}
NS_IMETHODIMP
sbSQLInsertBuilder::SetIntoTableName(const nsAString& aIntoTableName)
{
  mIntoTableName = aIntoTableName;
  return NS_OK;
}

NS_IMETHODIMP
sbSQLInsertBuilder::GetSelect(sbISQLSelectBuilder** aSelect)
{
  NS_ADDREF(*aSelect = mSelect);
  return NS_OK;
}
NS_IMETHODIMP
sbSQLInsertBuilder::SetSelect(sbISQLSelectBuilder* aSelect)
{
  mSelect = aSelect;
  return NS_OK;
}

NS_IMETHODIMP
sbSQLInsertBuilder::ResetInternal()
{
  mSelect = nsnull;
  mValueList.Clear();
  mOutputColumns.Clear();
  return NS_OK;
}

NS_IMETHODIMP
sbSQLInsertBuilder::ToStringInternal(nsAString& _retval)
{
  nsresult rv;
  nsAutoString buff;

  buff.AssignLiteral("insert into ");

  buff.Append(mIntoTableName);

  PRUint32 len = mOutputColumns.Length();
  if(len > 0) {
    buff.AppendLiteral(" (");
    for (PRUint32 i = 0; i < len; i++) {
      const sbColumnInfo& ci = mOutputColumns[i];
      buff.Append(ci.columnName);
      if (i + 1 < len) {
        buff.AppendLiteral(", ");
      }
    }
    buff.AppendLiteral(")");
  }

  if (mSelect) {
    nsAutoString sql;
    rv = mSelect->ToString(sql);
    NS_ENSURE_SUCCESS(rv, rv);
    buff.AppendLiteral(" ");
    buff.Append(sql);
  }
  else {
    buff.AppendLiteral(" values (");
    len = mValueList.Length();
    for (PRUint32 i = 0; i < len; i++) {
      const sbValueItem& vi = mValueList[i];

      switch(vi.type) {
        case eIsNull:
          buff.AppendLiteral("null");
          break;
        case eIsParameter:
          buff.AppendLiteral("?");
          break;
        case eString:
        {
          nsAutoString escapedValue(vi.stringValue);
          SB_EscapeSQL(escapedValue);

          buff.AppendLiteral("'");
          buff.Append(escapedValue);
          buff.AppendLiteral("'");
          break;
        }
        case eInteger32:
          buff.AppendInt(vi.int32Value);
          break;
      }

      if (i + 1 < len) {
        buff.AppendLiteral(", ");
      }
    }

    buff.AppendLiteral(")");
  }

  _retval.Assign(buff);
  return NS_OK;
}

