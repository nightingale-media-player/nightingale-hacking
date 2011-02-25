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

// the separator that will be used between parts of the metadata hash identity
const char SEPARATOR = '|';

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
nsresult
sbIdentityService::GetPropertyStringForAudio
                   (sbILocalDatabaseResourcePropertyBag *aPropertyBag,
                               nsAString    &_retval)
{
  NS_ENSURE_ARG_POINTER(aPropertyBag);
  nsresult rv;

  nsAutoString propString;
  PRUint32 propCount = NS_ARRAY_LENGTH(sAudioPropsToHash);
  for (PRUint32 i = 0; i < propCount; i++) {

    // sAudioPropsToHash contains ids for the properties we are interested in
    nsString propVal;
    rv = aPropertyBag->GetProperty(NS_ConvertUTF8toUTF16(sAudioPropsToHash[i]),
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

  _retval.Assign(propString);
  return NS_OK;
}

//-----------------------------------------------------------------------------
nsresult
sbIdentityService::GetPropertyStringForVideo
                   (sbILocalDatabaseResourcePropertyBag *aPropertyBag,
                    nsAString &_retval)
{
  NS_ENSURE_ARG_POINTER(aPropertyBag);
  nsresult rv;

  nsAutoString propString;
  PRUint32 propCount = NS_ARRAY_LENGTH(sVideoPropsToHash);
  for (PRUint32 i = 0; i < propCount; i++) {

    // sVideoPropsToHash contains ids for the properties we are interested in
    nsString propVal;
    rv = aPropertyBag->GetProperty(NS_ConvertUTF8toUTF16(sVideoPropsToHash[i]),
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

  _retval.Assign(propString);
  return NS_OK;
}

//-----------------------------------------------------------------------------
/*  sbIdentityService.idl, getIdentityForMediaItem */
NS_IMETHODIMP
sbIdentityService::GetIdentityForMediaItem
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

  rv = GetIdentityForBag(propertyBag, _retval);
  return rv;
}

//-----------------------------------------------------------------------------
/*  sbIdentityService.idl, getIdentityForBag */
NS_IMETHODIMP
sbIdentityService::GetIdentityForBag
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
   * audio, so detect the type and get a string of the relvant property values
   * concatenated together */
  nsString propString;
  if (contentType.EqualsLiteral("video")) {
    GetPropertyStringForVideo(aPropertyBag, propString);
  }
  else if (contentType.EqualsLiteral("audio")) {
    GetPropertyStringForAudio(aPropertyBag, propString);
  }
  else {
    NS_ERROR("Cannot get identity for unrecognized contenttype");
  }

  // hash the concatenated string and return it
  rv = HashString(propString, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

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
  rv = aMediaItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_HASH), aIdentity);
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
  rv = aPropertyBag->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_HASH),
                                        aIdentity);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
