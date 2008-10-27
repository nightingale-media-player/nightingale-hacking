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

#include "DatabasePreparedStatement.h"

// INCLUDES ===================================================================
#include "DatabaseQuery.h"
#include "DatabaseEngine.h"

#include <nsCOMPtr.h>
#include <nsStringGlue.h>

#include <prmem.h>
#include <prtypes.h>

// The maximum characters to output in a single PR_LOG call
#define MAX_PRLOG 400

NS_IMPL_THREADSAFE_ISUPPORTS1(CDatabasePreparedStatement, sbIDatabasePreparedStatement)

CDatabasePreparedStatement::CDatabasePreparedStatement(const nsAString &sql) 
  : mStatement(nsnull), mDB(nsnull), mSql(sql)
{
}

CDatabasePreparedStatement::~CDatabasePreparedStatement() 
{
  if (mStatement) {
    // this should always be safe if mStatement is defined.
    // if it does, it means we have a bad mStatement pointer. 
    // error codes returned here are okay, since they either reiterate
    // errors caused by bad statements being compiled, or indicate
    // that the statement was aborted during execution.
    // see: http://sqlite.org/c3ref/finalize.html
    sqlite3_finalize(mStatement);
    mStatement = nsnull;
  }
}

NS_IMETHODIMP CDatabasePreparedStatement::GetQueryString(nsAString &_retval)
{
  if (mSql.Length() > 0) {
    _retval = mSql;
  }
  else if (mStatement) {
    const char* sql = sqlite3_sql(mStatement);
    _retval = NS_ConvertUTF8toUTF16(sql);
  }
  else {
    _retval = EmptyString();
  }
  return NS_OK;
}

sqlite3_stmt* CDatabasePreparedStatement::GetStatement(sqlite3 *db) 
{
  if (!db) {
    NS_WARNING("GetStatement called without a database pointer.");
    return nsnull;
  }
  
  // either reset and return the existing statement, or compile it first and return that. 
  if (mStatement) {
    if (db != mDB) {
      NS_WARNING("GetStatement() called with a different DB than the one originally used compile it!.");
      return nsnull;
    }
    //Always reset the statement before sending it out for reuse.
    int retDB = 0;
    retDB = sqlite3_reset(mStatement);
    retDB = sqlite3_clear_bindings(mStatement);
  }
  else {
    mDB = db;
    
    if (mSql.Length() == 0) {
      NS_WARNING("GetStatement() called on a PreparedStatement with no SQL.");
      return nsnull;
    }

    const char *pzTail = nsnull;
    nsCString sqlStr = NS_ConvertUTF16toUTF8(mSql);
    int retDB = sqlite3_prepare_v2(db, sqlStr.get(), sqlStr.Length(),
                                   &mStatement, &pzTail);
    if (retDB != SQLITE_OK) {
      NS_WARNING(NS_ConvertUTF16toUTF8(mSql).get());
      const char *szErr = sqlite3_errmsg(db);
      nsCAutoString log;
      log.Append(szErr);
      log.AppendLiteral("\n");
      NS_WARNING(log.get());
      return nsnull;
    }
    // Henceforth, the sqlite_stmt will be responsible for holding onto the sql, not this object.
    mSql = EmptyString();
  }
  return mStatement;
}
