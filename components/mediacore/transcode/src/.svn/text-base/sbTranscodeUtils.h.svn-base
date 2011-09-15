/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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

#include <nsIURI.h>

#include <sbITranscodeError.h>
#include <sbITranscodeManager.h>

#include <nsStringGlue.h>

class nsIURI;
class sbIMediaItem;

/**
 * Helper function for C++ callers to create a new transcode error
 */

inline nsresult
SB_NewTranscodeError(const nsAString& aMessageWithItem,
                     const nsAString& aMessageWithoutItem,
                     const nsAString& aDetails,
                     const nsAString& aUri,
                     sbIMediaItem* aMediaItem,
                     sbITranscodeError** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  nsresult rv;
  nsCOMPtr<sbITranscodeError> error =
    do_CreateInstance(SONGBIRD_TRANSCODEERROR_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = error->Init(aMessageWithItem, aMessageWithoutItem, aDetails);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = error->SetSourceUri(aUri);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = error->SetSourceItem(aMediaItem);
  NS_ENSURE_SUCCESS(rv, rv);
  error.forget(_retval);
  return NS_OK;
}
inline nsresult
SB_NewTranscodeError(const nsAString& aMessageWithItem,
                     const nsAString& aMessageWithoutItem,
                     const nsAString& aDetails,
                     nsIURI* aUri,
                     sbIMediaItem* aMediaItem,
                     sbITranscodeError** _retval)
{
  NS_ENSURE_ARG_POINTER(aUri);
  nsCString uri;
  nsresult rv = aUri->GetSpec(uri);
  NS_ENSURE_SUCCESS(rv, rv);
  return SB_NewTranscodeError(aMessageWithItem, aMessageWithoutItem, aDetails,
                              NS_ConvertUTF8toUTF16(uri), aMediaItem, _retval);
}
inline nsresult
SB_NewTranscodeError(const char* aMessageWithItem,
                     const char* aMessageWithoutItem,
                     const char* aDetails,
                     nsIURI* aUri,
                     sbIMediaItem* aMediaItem,
                     sbITranscodeError** _retval)
{
  return SB_NewTranscodeError(NS_ConvertASCIItoUTF16(aMessageWithItem),
                              NS_ConvertASCIItoUTF16(aMessageWithoutItem),
                              NS_ConvertASCIItoUTF16(aDetails),
                              aUri,
                              aMediaItem,
                              _retval);
}
inline nsresult
SB_NewTranscodeError(const char* aMessageWithItem,
                     const char* aMessageWithoutItem,
                     const char* aDetails,
                     const char* aUri,
                     sbIMediaItem* aMediaItem,
                     sbITranscodeError** _retval)
{
  return SB_NewTranscodeError(NS_ConvertASCIItoUTF16(aMessageWithItem),
                              NS_ConvertASCIItoUTF16(aMessageWithoutItem),
                              NS_ConvertASCIItoUTF16(aDetails),
                              NS_ConvertASCIItoUTF16(aUri),
                              aMediaItem,
                              _retval);
}


