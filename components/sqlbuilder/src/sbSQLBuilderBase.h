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

#ifndef __SBSQLBUILDER_H__
#define __SBSQLBUILDER_H__

#include <sbISQLBuilder.h>

#include <nsStringGlue.h>
#include <nsTArray.h>
#include <nsCOMArray.h>
#include <nsCOMPtr.h>

#define QUOTE_CHAR '\''

#define NS_FORWARD_SBISQLBUILDER_WITHOUT_TOSTRING_RESET(_to) \
  NS_IMETHOD GetLimit(PRInt32 *aLimit) { return _to GetLimit(aLimit); } \
  NS_IMETHOD SetLimit(PRInt32 aLimit) { return _to SetLimit(aLimit); } \
  NS_IMETHOD GetLimitIsParameter(PRBool *aLimitIsParameter) { return _to GetLimitIsParameter(aLimitIsParameter); } \
  NS_IMETHOD SetLimitIsParameter(PRBool aLimitIsParameter) { return _to SetLimitIsParameter(aLimitIsParameter); } \
  NS_IMETHOD GetOffset(PRInt32 *aOffset) { return _to GetOffset(aOffset); } \
  NS_IMETHOD SetOffset(PRInt32 aOffset) { return _to SetOffset(aOffset); } \
  NS_IMETHOD GetOffsetIsParameter(PRBool *aOffsetIsParameter) { return _to GetOffsetIsParameter(aOffsetIsParameter); } \
  NS_IMETHOD SetOffsetIsParameter(PRBool aOffsetIsParameter) { return _to SetOffsetIsParameter(aOffsetIsParameter); } \
  NS_IMETHOD AddJoin(PRUint32 aJoinType, const nsAString & aJoinedTableName, const nsAString & aJoinedTableAlias, const nsAString & aJoinedColumnName, const nsAString & aJoinToTableName, const nsAString & aJoinToColumnName) { return _to AddJoin(aJoinType, aJoinedTableName, aJoinedTableAlias, aJoinedColumnName, aJoinToTableName, aJoinToColumnName); } \
  NS_IMETHOD AddSubqueryJoin(PRUint32 aJoinType, sbISQLSelectBuilder *aJoinedSubquery, const nsAString & aJoinedTableAlias, const nsAString & aJoinedColumnName, const nsAString & aJoinToTableName, const nsAString & aJoinToColumnName) { return _to AddSubqueryJoin(aJoinType, aJoinedSubquery, aJoinedTableAlias, aJoinedColumnName, aJoinToTableName, aJoinToColumnName); } \
  NS_IMETHOD AddJoinWithCriterion(PRUint32 aJoinType, const nsAString & aJoinedTableName, const nsAString & aJoinedTableAlias, sbISQLBuilderCriterion *aCriterion) { return _to AddJoinWithCriterion(aJoinType, aJoinedTableName, aJoinedTableAlias, aCriterion); } \
  NS_IMETHOD AddSubquery(sbISQLSelectBuilder *aSubquery, const nsAString & aAlias) { return _to AddSubquery(aSubquery, aAlias); } \

class sbSQLBuilderBase : public sbISQLBuilder
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBISQLBUILDER

  sbSQLBuilderBase();
  virtual ~sbSQLBuilderBase();
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
    nsCOMPtr<sbISQLSelectBuilder> subquery;
  };

  struct sbSubqueryInfo
  {
    nsCOMPtr<sbISQLSelectBuilder> subquery;
    nsString alias;
  };

  PRInt32 mLimit;
  PRBool mLimitIsParameter;
  PRInt32 mOffset;
  PRBool mOffsetIsParameter;
  nsTArray<sbJoinInfo> mJoins;
  nsTArray<sbSubqueryInfo> mSubqueries;

};

inline nsresult
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

