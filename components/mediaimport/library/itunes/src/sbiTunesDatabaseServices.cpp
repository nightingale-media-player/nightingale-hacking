/*
 //
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#include "sbiTunesDatabaseServices.h"

// C/C++ includes
#include <time.h>

// Mozilla includes
#include <nsComponentManagerUtils.h>
#include <nsThreadUtils.h>

// Songbird includes
#include <sbIDatabaseQuery.h>
#include <sbIDatabaseResult.h>
#include <sbIDatabasePreparedStatement.h>

#define SB_DBQUERY_CONTRACTID "@songbirdnest.com/Songbird/DatabaseQuery;1"

sbiTunesDatabaseServices::sbiTunesDatabaseServices() : mResetPending(PR_FALSE) {
}

sbiTunesDatabaseServices::~sbiTunesDatabaseServices() {
  // nothing to do, move along
}

nsresult
sbiTunesDatabaseServices::Initialize() {
  nsresult rv;
  mDBQuery = do_CreateInstance(SB_DBQUERY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = mDBQuery->SetAsyncQuery(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = mDBQuery->SetDatabaseGUID(NS_LITERAL_STRING("songbird"));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsString sql;
  sql.AppendLiteral("CREATE TABLE IF NOT EXISTS itunes_id_map "
                    "(itunes_id TEXT UNIQUE NOT NULL, "
                    "songbird_id TEXT UNIQUE NOT NULL)"); 
  rv = mDBQuery->AddQuery(sql);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool dbOK;
  rv = mDBQuery->Execute(&dbOK);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOK == 0, NS_ERROR_FAILURE);

  NS_NAMED_LITERAL_STRING(INSERT_SQL, 
                          "INSERT OR REPLACE INTO itunes_id_map (itunes_id, songbird_id) VALUES (?, ?)");
  rv = mDBQuery->PrepareQuery(INSERT_SQL, getter_AddRefs(mInsertMapID));
  NS_ENSURE_SUCCESS(rv, rv);
  
  NS_NAMED_LITERAL_STRING(SELECT_SQL, 
                          "SELECT songbird_id FROM itunes_id_map WHERE itunes_id = ?");
  rv = mDBQuery->PrepareQuery(SELECT_SQL, getter_AddRefs(mSelectMapID));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING(DELETE_SQL, 
                          "DELETE FROM itunes_id_map WHERE songbird_id = ?");

  rv = mDBQuery->PrepareQuery(SELECT_SQL, getter_AddRefs(mDeleteMapID));
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

nsresult 
sbiTunesDatabaseServices::MapID(nsAString const & aiTunesLibID,
                                nsAString const & aiTunesID,
                                nsAString const & aSongbirdID) {
  
  nsresult rv = mDBQuery->AddPreparedStatement(mInsertMapID);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsString compositeID(aiTunesLibID);
  compositeID.Append(aiTunesID);
  rv = mDBQuery->BindStringParameter(0, compositeID);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = mDBQuery->BindStringParameter(1, aSongbirdID);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool dbOK;
  rv = mDBQuery->Execute(&dbOK);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOK == 0, NS_ERROR_FAILURE);

  return NS_OK;
}

nsresult
sbiTunesDatabaseServices::GetSBIDFromITID(nsAString const & aiTunesLibID,
                                          nsAString const & aiTunesID,
                                          nsAString & aSongbirdID) {

  nsresult rv = mDBQuery->AddPreparedStatement(mSelectMapID);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString iTunesID(aiTunesLibID);
  iTunesID.Append(aiTunesID);
  
  rv = mDBQuery->BindStringParameter(0, iTunesID);
  NS_ENSURE_SUCCESS(rv, rv);
   
  PRInt32 dbOK;
  rv = mDBQuery->Execute(&dbOK);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOK == 0, NS_ERROR_FAILURE);
  
  nsCOMPtr<sbIDatabaseResult> result;
  rv = mDBQuery->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);
  
  rv = result->GetRowCell(0, 0, aSongbirdID);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

nsresult
sbiTunesDatabaseServices::RemoveSBIDEntry(nsAString const & aSongbirdID) {

  nsresult rv = mDBQuery->AddPreparedStatement(mDeleteMapID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBQuery->BindStringParameter(0, aSongbirdID);
  NS_ENSURE_SUCCESS(rv, rv);
   
  PRBool dbOK;
  rv = mDBQuery->Execute(&dbOK);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOK == 0, NS_ERROR_FAILURE);

  return NS_OK;
}
