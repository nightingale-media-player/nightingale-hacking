/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2011 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

/**
* \file  sbIdentityService.cpp
* \brief Songbird Identity Service Implementation.
*/

#include "sbIdentityService.h"

#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include <nsICategoryManager.h>

#include <nsStringAPI.h>
#include <sbStringUtils.h>
#include <sbStandardProperties.h>

#include <sbDebugUtils.h>
#include <nsICryptoHash.h>


NS_IMPL_THREADSAFE_ISUPPORTS1(sbIdentityService,
                              sbIIdentityService)

// the separator that will be used between parts of the metadata hash identity
const char SEPARATOR = '|';

// the properties that will be used as part of the metadata hash identity
nsString mPropsToHash[] = {
  NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME),
  NS_LITERAL_STRING(SB_PROPERTY_ARTISTNAME),
  NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME),
  NS_LITERAL_STRING(SB_PROPERTY_GENRE)
};

//-----------------------------------------------------------------------------
sbIdentityService::sbIdentityService()
{
  SB_PRLOG_SETUP(sbIdentityService);
}

//-----------------------------------------------------------------------------
/* virtual */ sbIdentityService::~sbIdentityService()
{
}

//-----------------------------------------------------------------------------
nsresult
sbIdentityService::HashString(const nsAString  &aString,
                              nsAString        &_retval)
{
  NS_ENSURE_TRUE(!aString.IsEmpty(), NS_ERROR_INVALID_ARG);

  TRACE_FUNCTION("Hashing the string \'%s\'",
                 NS_ConvertUTF16toUTF8(aString).get());

  nsresult rv;

  // Create and initialize a hash object.
  nsCOMPtr<nsICryptoHash>
    cryptoHash = do_CreateInstance("@mozilla.org/security/hash;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  /* hash with md5 algorithm for very low chance of hash collision between
   * differing strings */
  rv = cryptoHash->Init(nsICryptoHash::MD5);
  NS_ENSURE_SUCCESS(rv, rv);

  // handle the input string as a bytestring that cryptoHash can handle
  nsCString toHash;
  toHash = NS_ConvertUTF16toUTF8(aString);

  // generate the hash
  rv = cryptoHash->Update
                   (reinterpret_cast<PRUint8 const *>(toHash.BeginReading()),
                    toHash.Length());
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString hashValue;
  rv = cryptoHash->Finish(PR_TRUE, hashValue);
  NS_ENSURE_SUCCESS(rv, rv);

  _retval.AssignLiteral(hashValue.get());

  return NS_OK;
}

//-----------------------------------------------------------------------------
/*  sbIdentityService.idl, getIdentity */
NS_IMETHODIMP
sbIdentityService::GetIdentity(sbIMediaItem *aMediaItem,
                               nsAString    &_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  nsresult rv;

  nsString trackName;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME),
                               trackName);
  if (NS_FAILED(rv) || trackName.IsEmpty()) {
    trackName.AssignLiteral("No Track Name");
  }
  TRACE_FUNCTION("Generating an identity for \'%s\' ",
                 NS_ConvertUTF16toUTF8(trackName).get());

  // concatenate the properties that we are interested in together
  nsAutoString propString;
  PRUint32 propCount = NS_ARRAY_LENGTH(mPropsToHash);
  for (PRUint32 i = 0; i < propCount; i++) {
    // mPropsToHash contains the ids for the properties we are interested in
    nsString propVal;
    rv = aMediaItem->GetProperty(mPropsToHash[i],
                                 propVal);

    if (NS_FAILED(rv) || propVal.IsEmpty())
    {
      propVal = NS_LITERAL_STRING("");
    }

    // append this property to the concatenated string
    if (i == 0)
    {
      propString.Assign(propVal);
    }
    else {
      propString.AppendLiteral(&SEPARATOR);
      propString.Append(propVal);
    }
  }

  // hash the concatenated string and return it
  rv = HashString(propString, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//-----------------------------------------------------------------------------
/*  sbIdentityService.idl, saveIdentity */
NS_IMETHODIMP
sbIdentityService::SaveIdentity(sbIMediaItem    *aMediaItem,
                                const nsAString &aIdentity)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_TRUE(!aIdentity.IsEmpty(), NS_ERROR_INVALID_ARG);

  TRACE_FUNCTION("Saving an identity of \'%s\' ",
                 NS_ConvertUTF16toUTF8(aIdentity).get());

  // save aIdentity to the content_hash column in the mediaitems db
  nsresult rv = aMediaItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_HASH),
                                        aIdentity);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
