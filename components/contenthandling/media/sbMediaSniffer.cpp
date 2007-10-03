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

#include "sbMediaSniffer.h"

#include <nsICategoryManager.h>
#include <nsIHttpChannel.h>
#include <nsIRequest.h>
#include <nsIServiceManager.h>
#include <nsIURI.h>
#include <sbIPlaylistPlayback.h>

#include <nsComponentManagerUtils.h>
#include <nsCOMPtr.h>
#include <nsNetCID.h>
#include <nsServiceManagerUtils.h>
#include <nsXPCOM.h>
#include <nsXPCOMCID.h>

#define SONGBIRD_PLAYLISTPLAYBACK_CONTRACTID \
  "@songbirdnest.com/Songbird/PlaylistPlayback;1"

/**
 * An implementation of nsIContentSniffer that prevents audio and video files
 * from being downloaded through the Download Manager or displayed with
 * fullscreen plugins.
 *
 * This component works hand-in-hand with the sbMediaContentListener component.
 * When a URI is first resolved this component is called to override the MIME
 * type of files that we recognize as media files. Thereafter no other content
 * listeners will be able to support the MIME type except for our
 * sbMediaContentListener.
 */

NS_IMPL_ISUPPORTS1(sbMediaSniffer, nsIContentSniffer)

NS_IMETHODIMP
sbMediaSniffer::GetMIMETypeFromContent(nsIRequest* aRequest, 
                                       const PRUint8* aData, 
                                       PRUint32 aLength, 
                                       nsACString& aSniffedType)
{
  NS_ENSURE_ARG_POINTER(aRequest);

  nsresult rv;

  // This won't always succeed, so don't warn for this expected failure.
  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(aRequest, &rv);

  if (NS_SUCCEEDED(rv)) {
    nsCAutoString requestMethod;
    rv = httpChannel->GetRequestMethod(requestMethod);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!requestMethod.EqualsLiteral("GET") &&
        !requestMethod.EqualsLiteral("POST")) {
        aSniffedType.Truncate();
        return NS_OK;
    }
  }


  nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // There are apparently problems with 'view-source'... See nsFeedSniffer.
  nsCOMPtr<nsIURI> originalURI;
  rv = channel->GetOriginalURI(getter_AddRefs(originalURI));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString scheme;
  rv = originalURI->GetScheme(scheme);
  NS_ENSURE_SUCCESS(rv, rv);

  if (scheme.EqualsLiteral("view-source")) {
    aSniffedType.Truncate();
    return NS_OK;
  }

  nsCOMPtr<nsIURI> uri;
  rv = channel->GetURI(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString spec;
  rv = uri->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPlaylistPlayback> playlistPlayback =
    do_GetService(SONGBIRD_PLAYLISTPLAYBACK_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isMediaURL;

  // This will fail if there are no media cores registered (like at startup),
  // so don't warn for this expected failure.
  rv = playlistPlayback->IsMediaURL(NS_ConvertUTF8toUTF16(spec), &isMediaURL);

  if (NS_SUCCEEDED(rv) && isMediaURL) {
    aSniffedType.AssignLiteral(TYPE_MAYBE_MEDIA);
  }
  else {
    aSniffedType.Truncate();
  }

  return NS_OK;
}

NS_METHOD
sbMediaSniffer::Register(nsIComponentManager* aCompMgr,
                         nsIFile* aPath, 
                         const char* aRegistryLocation,
                         const char* aComponentType, 
                         const nsModuleComponentInfo* aInfo)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catman =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = catman->AddCategoryEntry(NS_CONTENT_SNIFFER_CATEGORY,
                                SONGBIRD_MEDIASNIFFER_DESCRIPTION, 
                                SONGBIRD_MEDIASNIFFER_CONTRACTID,
                                PR_TRUE, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_METHOD
sbMediaSniffer::Unregister(nsIComponentManager* aCompMgr,
                           nsIFile* aPath, 
                           const char* aRegistryLocation,
                           const nsModuleComponentInfo* aInfo)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catman =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = catman->DeleteCategoryEntry(NS_CONTENT_SNIFFER_CATEGORY,
                                   SONGBIRD_MEDIASNIFFER_DESCRIPTION, 
                                   PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
