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

#ifndef __SBSQLBUILDER_H__
#define __SBSQLBUILDER_H__

#include <sbISQLBuilder.h>

#include <nsStringGlue.h>
#include <nsTArray.h>
#include <nsCOMArray.h>
#include <nsCOMPtr.h>

#define QUOTE_CHAR '\''

class sbSQLBuilderBase : public sbISQLBuilder
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBISQLBUILDER

  sbSQLBuilderBase();
protected:

  struct sbColumnInfo 
  {
    nsString tableName;
    nsString columnName;
  };

  struct sbJoinInfo
  {
    PRUint32 type;
    nsString joinedTableName;
    nsString joinedTableAlias;
    nsString joinedColumnName;
    nsString joinToTableName;
    nsString joinToColumnName;
    nsCOMPtr<sbISQLBuilderCriterion> criterion;
  };

  struct sbOrderInfo
  {
    nsString tableName;
    nsString columnName;
    PRBool ascending;
  };

  struct sbSubqueryInfo
  {
    nsCOMPtr<sbISQLSelectBuilder> subquery;
    nsString alias;
  };

  NS_IMETHOD ToStringInternal(nsAString& _retval) = 0;
  NS_IMETHOD ResetInternal() = 0;

  nsString mBaseTableName;
  nsString mBaseTableAlias;
  PRBool mIsDistinct;
  PRInt32 mLimit;
  PRBool mLimitIsParameter;
  PRInt32 mOffset;
  PRBool mOffsetIsParameter;
  nsTArray<sbColumnInfo> mOutputColumns;
  nsTArray<sbJoinInfo> mJoins;
  nsTArray<sbSubqueryInfo> mSubqueries;
  nsCOMArray<sbISQLBuilderCriterion> mCritera;

};

static nsresult
SB_EscapeSQL(nsAString& str)
{
  nsAutoString dest;

  PRInt32 pos = str.FindChar(QUOTE_CHAR, 0);
  PRInt32 lastPos = 0;
  PRBool hasQuote = PR_FALSE;
  while(pos >= 0) {
    dest.Append(Substring(str, lastPos, pos - lastPos + 1));
    dest.Append(QUOTE_CHAR);
    lastPos = pos + 1;
    pos = str.FindChar(QUOTE_CHAR, lastPos);
    hasQuote = PR_TRUE;
  }

  if (hasQuote) {
    dest.Append(Substring(str, lastPos, str.Length() - lastPos));
    str.Assign(dest);
  }

  return NS_OK;
}

#endif /* __SBSQLBUILDER_H__ */

