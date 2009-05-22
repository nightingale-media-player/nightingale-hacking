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
#ifndef SBITUNESSIGNATURE_H_
#define SBITUNESSIGNATURE_H_

#include <nsStringAPI.h>

#include "sbiTunesImporterCommon.h"

class nsIScriptableUnicodeConverter;
class nsICryptoHash;
class sbIDatabaseQuery;
class sbIDatabasePreparedStatement;

/**
 * This calculates the signature of "things" such as the iTunes database
 * or a playlist. This only runs on the import thread, so we just
 * proxy the converter and db query objects
 */
class sbiTunesSignature
{
public:
  sbiTunesSignature();
  ~sbiTunesSignature();
  /**
   * Initializes the converter and database objects
   */
  nsresult Initialize();
  /**
   * Updates the signature with new data
   * \param aStringData the data to update the signature with
   */
  nsresult Update(nsAString const & aStringData);
  /**
   * Returns the signature
   */
  nsresult GetSignature(nsAString & aSignature);
  /**
   * Stores the signature in the database
   * \param aSignature contains the signature being returned
   */
  nsresult StoreSignature(nsAString const & aID,
                          nsAString const & aSignature);
  /**
   * Retrieves the signature from the database
   */
  nsresult RetrieveSignature(nsAString const & aID,
                             nsAString & aSignature);  
private:
  typedef nsCOMPtr<nsIScriptableUnicodeConverter> nsIScriptableUnicodeConverterPtr;
  typedef nsCOMPtr<nsICryptoHash> nsICryptoHashPtr;
  typedef nsCOMPtr<sbIDatabasePreparedStatement> PreparedStatementPtr;
  
  /**
   * Hash component
   */
  nsICryptoHashPtr mHashProc;
  
  /**
   * Our DB query object
   */
  sbIDatabaseQueryPtr mDBQuery;

  /**
   * Used as a cache for GetSignature
   */
  nsString mSignature;
  
  /**
   * Store signature prepared statement
   */
  PreparedStatementPtr mInsertSig;
  
  /**
   * Retrieve signature prepared statement
   */
  PreparedStatementPtr mRetrieveSig;
  
};

#endif /* SBITUNESSIGNATURE_H_ */
