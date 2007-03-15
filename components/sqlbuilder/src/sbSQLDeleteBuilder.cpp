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

#include "sbSQLDeleteBuilder.h"
#include "sbSQLWhereBuilder.h"

NS_IMPL_ISUPPORTS_INHERITED1(sbSQLDeleteBuilder,
                             sbSQLWhereBuilder,
                             sbISQLDeleteBuilder)

sbSQLDeleteBuilder::sbSQLDeleteBuilder()
{
}

NS_IMETHODIMP
sbSQLDeleteBuilder::GetTableName(nsAString& aTableName)
{
  aTableName = mTableName;
  return NS_OK;
}
NS_IMETHODIMP
sbSQLDeleteBuilder::SetTableName(const nsAString& aTableName)
{
  mTableName = aTableName;
  return NS_OK;
}

NS_IMETHODIMP
sbSQLDeleteBuilder::Reset()
{
  sbSQLWhereBuilder::Reset();
  mTableName.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbSQLDeleteBuilder::ToString(nsAString& _retval)
{
  nsresult rv;
  nsAutoString buff;

  buff.AssignLiteral("delete from ");

  buff.Append(mTableName);

  rv = AppendWhere(buff);
  NS_ENSURE_SUCCESS(rv, rv);

  _retval.Assign(buff);
  return NS_OK;
}

