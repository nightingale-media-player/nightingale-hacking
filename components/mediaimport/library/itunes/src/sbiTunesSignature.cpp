/*
 //
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
 */

#include "sbiTunesSignature.h"

#include <nsComponentManagerUtils.h>
#include <nsICryptoHash.h>
#include <nsMemory.h>
#include <nsIScriptableUConv.h>

#include <sbIDatabaseQuery.h>
#include <sbIDatabaseResult.h>
#include <sbIDatabasePreparedStatement.h>

#include <sbProxiedComponentManager.h>


char const SB_ITUNES_DB_GUID[] = "nightingale";

sbiTunesSignature::sbiTunesSignature() {}
sbiTunesSignature::~sbiTunesSignature() {}

nsresult sbiTunesSignature::Initialize() {
  nsresult rv;

  /* Create a hashing processor. */
  mHashProc = do_CreateInstance("@mozilla.org/security/hash;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  mHashProc->Init(nsICryptoHash::MD5);
  
  /* Create a signature database query component. */
  mDBQuery = do_CreateInstance("@getnightingale.com/Nightingale/DatabaseQuery;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  mDBQuery->SetAsyncQuery(PR_FALSE);
  mDBQuery->SetDatabaseGUID(NS_LITERAL_STRING("nightingale"));
  
  /* Ensure the signature database table exists. */
  nsString sql(NS_LITERAL_STRING("CREATE TABLE IF NOT EXISTS itunes_signatures (id TEXT UNIQUE NOT NULL, signature TEXT NOT NULL)"));
  
  rv = mDBQuery->AddQuery(sql);
  NS_ENSURE_SUCCESS(rv, rv);
  
  PRInt32 result;
  rv = mDBQuery->Execute(&result);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(result == 0, NS_ERROR_FAILURE);
  
  rv = mDBQuery->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING(INSERT_SQL,
                          "INSERT OR REPLACE INTO itunes_signatures (id, signature) VALUES (?, ?)");
  rv = mDBQuery->PrepareQuery(INSERT_SQL,
                              getter_AddRefs(mInsertSig));
  NS_ENSURE_SUCCESS(rv, rv);
  
  NS_NAMED_LITERAL_STRING(SELECT_SQL,
                          "SELECT signature FROM itunes_signatures WHERE id = ?");
  rv = mDBQuery->PrepareQuery(SELECT_SQL,
                              getter_AddRefs(mRetrieveSig));
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

nsresult sbiTunesSignature::Update(nsAString const & aStringData) {
  nsresult rv;
  
  nsCString const & byteString = ::NS_ConvertUTF16toUTF8(aStringData);
  rv = mHashProc->Update(reinterpret_cast<PRUint8 const *>(byteString.BeginReading()), 
                         byteString.Length());
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult 
sbiTunesSignature::GetSignature(nsAString & aSignature) {
  if (mSignature.IsEmpty()) {
    nsCString hashData;
    nsresult rv = mHashProc->Finish(PR_TRUE, hashData);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCString hashString;
    unsigned char const ZERO = '0';
    for (unsigned char const * c = 
             reinterpret_cast<unsigned char const *>(hashData.BeginReading());
         *c; 
         ++c) {
      unsigned char nibble = (*c >> 4) + ZERO;
      hashString.Append(nibble);
      nibble = (*c & 0xF) + ZERO;
      hashString.Append(nibble);
    }
    mSignature = NS_ConvertASCIItoUTF16(hashString);
  }
  aSignature = mSignature;
  return NS_OK;
}

nsresult 
sbiTunesSignature::StoreSignature(nsAString const & aID,
                                  nsAString const & aSignature) {
  
  nsresult rv = mDBQuery->AddPreparedStatement(mInsertSig);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = mDBQuery->BindStringParameter(0, aID);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = mDBQuery->BindStringParameter(1, aSignature);
  NS_ENSURE_SUCCESS(rv, rv);
  
  PRInt32 result;
  rv = mDBQuery->Execute(&result);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(result == 0, NS_ERROR_FAILURE);
  
  rv = mDBQuery->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult 
sbiTunesSignature::RetrieveSignature(nsAString const & aID,
                                     nsAString & aSignature) {
  
  nsresult rv = mDBQuery->AddPreparedStatement(mRetrieveSig);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = mDBQuery->BindStringParameter(0, aID);
  NS_ENSURE_SUCCESS(rv, rv);
  
  PRInt32 dbResult;
  rv = mDBQuery->Execute(&dbResult);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbResult == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = mDBQuery->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);
  
  rv = result->GetRowCell(0, 0, aSignature);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = mDBQuery->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}
