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
#include <nsICryptoHash.h>

#include <sbIPropertyManager.h>
#include <sbIPropertyInfo.h>
#include <sbILocalDatabaseResourcePropertyBag.h>
#include <sbILocalDatabaseMediaItem.h>
#include <sbDebugUtils.h>

NS_IMPL_THREADSAFE_ISUPPORTS1(sbIdentityService,
                              sbIIdentityService)

 /* The following consts are used to retrieve metadata properties
  * and concatenate them into a string that will be hashed to form the
  * metadata_hash_identity for each mediaitem.
  *
  * Any changes to any of these consts, including the order in the arrays,
  * will require a migration to ensure stored identities of existing mediaitems
  * are coherent with new identity calculations.
  *
  * The initial migration to this effect is
  * sbMigrate19to110pre0.addMetadataHashIdentity.js
  */

// the separator that will be used between parts of the metadata hash identity
static const char SEPARATOR[] = "|";

/* the properties that will be used as part of the metadata hash identity
 * for audio files */
static const char* const sAudioPropsToHash[] = {
  SB_PROPERTY_CONTENTTYPE,
  SB_PROPERTY_TRACKNAME,
  SB_PROPERTY_ARTISTNAME,
  SB_PROPERTY_ALBUMNAME,
  SB_PROPERTY_GENRE
};

// jhawk for now the properties used for Video and Audio are the same, but that
//       will change in the near future.
/* the properties that will be used as part of the metadata hash identity
 * for video files */
static const char* const sVideoPropsToHash[] = {
  SB_PROPERTY_CONTENTTYPE,
  SB_PROPERTY_TRACKNAME,
  SB_PROPERTY_ARTISTNAME,
  SB_PROPERTY_ALBUMNAME,
  SB_PROPERTY_GENRE
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
/*  sbIdentityService.idl, hashString */
NS_IMETHODIMP
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
nsresult
sbIdentityService::GetPropertyStringFor
                   (sbILocalDatabaseResourcePropertyBag *aPropertyBag,
                    const char * const * aPropsToHash,
                    PRUint32 aPropsToHashLength,
                    nsAString &_retval)
{
  NS_ENSURE_ARG_POINTER(aPropertyBag);
  NS_ENSURE_ARG_POINTER(aPropsToHash);
  nsresult rv;

  // Tracks whether we got a property value or not
  bool propertyFound = false;
  nsAutoString propString;
  for (PRUint32 i = 0; i < aPropsToHashLength; i++) {

    // sAudioPropsToHash contains ids for the properties we are interested in
    nsString propVal;
    rv = aPropertyBag->GetProperty(NS_ConvertUTF8toUTF16(aPropsToHash[i]),
                                   propVal);

    if (NS_FAILED(rv) || propVal.IsEmpty()) {
      propVal.Truncate();
    }
    // Content type doesn't count for determining if a property is found
    else if (strcmp(aPropsToHash[i], SB_PROPERTY_CONTENTTYPE) != 0) {

      propertyFound = true;
    }
    // append this property to the concatenated string
    if (i == 0) {
      propString.Assign(propVal);
    }
    else {
      propString.AppendLiteral(SEPARATOR);
      propString.Append(propVal);
    }
  }

  // If we found property values return the concatenated string else return void
  if (propertyFound) {
    _retval.Assign(propString);
  }
  else {
    _retval.SetIsVoid(PR_TRUE);
  }
  return NS_OK;
}

//-----------------------------------------------------------------------------
nsresult
sbIdentityService::GetPropertyStringForAudio
                   (sbILocalDatabaseResourcePropertyBag *aPropertyBag,
                    nsAString &_retval)
{
  NS_ENSURE_ARG_POINTER(aPropertyBag);

  return GetPropertyStringFor(aPropertyBag,
                              sAudioPropsToHash,
                              NS_ARRAY_LENGTH(sAudioPropsToHash),
                              _retval);
}

//-----------------------------------------------------------------------------
nsresult
sbIdentityService::GetPropertyStringForVideo
                   (sbILocalDatabaseResourcePropertyBag *aPropertyBag,
                    nsAString &_retval)
{
  NS_ENSURE_ARG_POINTER(aPropertyBag);

  return GetPropertyStringFor(aPropertyBag,
                              sVideoPropsToHash,
                              NS_ARRAY_LENGTH(sVideoPropsToHash),
                              _retval);
}

//-----------------------------------------------------------------------------
/*  sbIdentityService.idl, calculateIdentityForMediaItem */
NS_IMETHODIMP
sbIdentityService::CalculateIdentityForMediaItem
                   (sbIMediaItem *aMediaItem,
                    nsAString    &_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  nsresult rv;

  nsCOMPtr<sbILocalDatabaseMediaItem> localItem =
    do_QueryInterface(aMediaItem, &rv);
  if (NS_FAILED(rv)) {
    // we were passed a mediaitem that we won't be able to get a propertybag for
    return NS_OK;
  }

  // get the propertybag underlying aMediaItem and make an identity from that
  nsCOMPtr<sbILocalDatabaseResourcePropertyBag> propertyBag;
  rv = localItem->GetPropertyBag(getter_AddRefs(propertyBag));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CalculateIdentityForBag(propertyBag, _retval);
  return rv;
}

//-----------------------------------------------------------------------------
/*  sbIdentityService.idl, calculateIdentityForBag */
NS_IMETHODIMP
sbIdentityService::CalculateIdentityForBag
                   (sbILocalDatabaseResourcePropertyBag *aPropertyBag,
                    nsAString &_retval)
{
  NS_ENSURE_ARG_POINTER(aPropertyBag);
  nsresult rv;

  #ifdef DEBUG
  nsString trackName;
  rv = aPropertyBag->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME),
                                trackName);
  if (NS_FAILED(rv) || trackName.IsEmpty()) {
    trackName.AssignLiteral("No Track Name");
  }
  TRACE_FUNCTION("Generating an identity for \'%s\' ",
                 NS_ConvertUTF16toUTF8(trackName).get());
  #endif

  // concatenate the properties that we are interested in together
  nsString contentType;
  rv = aPropertyBag->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTTYPE),
                                 contentType);
  NS_ENSURE_SUCCESS(rv, rv);

  /* we use different properties in the identity calculations for video and
   * audio, so detect the type and get a string of the relevant property values
   * concatenated together
   * If the content property is not set then we don't need the hash, throw
   * NS_ERROR_NOT_AVAILABLE.
   */
  nsString propString;
  if (contentType.EqualsLiteral("video")) {
    rv = GetPropertyStringForVideo(aPropertyBag, propString);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (contentType.EqualsLiteral("audio")) {
    rv = GetPropertyStringForAudio(aPropertyBag, propString);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // If we didn't recognize the content type or the hash properties
  // were not found return a void string.
  if (propString.IsEmpty()) {
    _retval.SetIsVoid(PR_TRUE);
  }
  else {
    // hash the concatenated string and return it
    rv = HashString(propString, _retval);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

//-----------------------------------------------------------------------------
/*  sbIdentityService.idl, saveIdentityForMediaItem */
NS_IMETHODIMP
sbIdentityService::SaveIdentityToMediaItem
                   (sbIMediaItem    *aMediaItem,
                                const nsAString &aIdentity)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  nsresult rv;

  #ifdef DEBUG
  nsString trackName;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME),
                               trackName);
  if (NS_FAILED(rv) || trackName.IsEmpty()) {
    trackName.AssignLiteral("No Track Name");
  }

  // present the debug string to NSPR log and console
  TRACE_FUNCTION("Saving an identity of \'%s\' for track \'%s\'",
                 NS_ConvertUTF16toUTF8(aIdentity).get(),
                 NS_ConvertUTF16toUTF8(trackName).get());
  #endif

  // save aIdentity to the propertybag for the param aMediaitem
  rv = aMediaItem->SetProperty
                  (NS_LITERAL_STRING(SB_PROPERTY_METADATA_HASH_IDENTITY),
                   aIdentity);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//-----------------------------------------------------------------------------
/*  sbIdentityService.idl, saveIdentityForBag */
NS_IMETHODIMP
sbIdentityService::SaveIdentityToBag
                   (sbILocalDatabaseResourcePropertyBag *aPropertyBag,
                    const nsAString &aIdentity)
{
  NS_ENSURE_ARG_POINTER(aPropertyBag);
  nsresult rv;

  #ifdef DEBUG
  nsString trackName;
  rv = aPropertyBag->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME),
                                 trackName);
  if (NS_FAILED(rv) || trackName.IsEmpty()) {
    trackName.AssignLiteral("No Track Name");
  }

  // present the debug string to NSPR log and console
  TRACE_FUNCTION("Saving an identity of \'%s\' for track \'%s\'",
                 NS_ConvertUTF16toUTF8(aIdentity).get(),
                 NS_ConvertUTF16toUTF8(trackName).get());
  #endif

  // save the identity in the propertybag
  rv = aPropertyBag->SetProperty
                     (NS_LITERAL_STRING(SB_PROPERTY_METADATA_HASH_IDENTITY),
                                        aIdentity);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
