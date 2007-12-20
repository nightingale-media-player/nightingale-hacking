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

#include "sbRemoteSiteMediaItem.h"
#include "sbRemoteSiteLibraryResource.h"
#include "sbRemotePlayer.h"

#include <prlog.h>
#include <sbClassInfoUtils.h>
#include <sbStandardProperties.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteSiteMediaItem:5
 *   LOG_ITEM defined in sbRemoteMediaItem.h/.cpp
 */
#undef LOG
#define LOG(args) LOG_ITEM(args)

const static char* sPublicWProperties[] = {""};

const static char* sPublicRProperties[] =
{
  // sbILibraryResource
  "site:guid",
  "site:created",
  "site:updated",

  // sbIMediaItem
  // omitting library since we don't want the user to get back
  // to the original library
  "site:isMutable",
  "site:mediaCreated",
  "site:mediaUpdated",
  "site:contentSrc",
  "site:contentLength",
  "site:contentType",

  // nsIClassInfo
  "classinfo:classDescription",
  "classinfo:contractID",
  "classinfo:classID",
  "classinfo:implementationLanguage",
  "classinfo:flags"
};

const static char* sPublicMethods[] =
{
  // sbILibraryResource
  "site:getProperty",
  "site:setProperty",
  "site:equals"

  // sbIMediaItem
  // none applicable
};

NS_IMPL_ISUPPORTS_INHERITED0( sbRemoteSiteMediaItemSecurityMixin,
                              sbSecurityMixin )

NS_IMETHODIMP
sbRemoteSiteMediaItemSecurityMixin::CanGetProperty( const nsIID* aIID,
                                                    const PRUnichar* aPropertyID,
                                                    char** _retval )
{
  LOG_ITEM(("sbRemoteSiteMediaItemSecurityMixin::CanGetProperty()"));
  nsresult rv = sbSecurityMixin::CanGetProperty( aIID, aPropertyID, _retval );
  NS_ENSURE_SUCCESS( rv, rv );

  if (mPrivileged) {
    return NS_OK;
  }

  nsDependentString propID(aPropertyID);
  if (propID.EqualsLiteral("contentSrc")) {
    nsCOMPtr<nsIURI> uri;
    rv = mMediaItem->GetContentSrc(getter_AddRefs(uri));
    NS_ENSURE_SUCCESS( rv, rv );

    PRBool isFileURI;
    rv = uri->SchemeIs( "file" , &isFileURI );
    NS_ENSURE_SUCCESS( rv, rv );

    if (isFileURI) {
      NS_WARNING("Disallowing access to 'file:' URI from contentSrc attribute");
      return NS_ERROR_FAILURE;
    }
  }

  return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED1( sbRemoteSiteMediaItem,
                              sbRemoteMediaItem,
                              nsIClassInfo )

NS_IMPL_CI_INTERFACE_GETTER5(sbRemoteSiteMediaItem,
                             nsISupports,
                             sbISecurityAggregator,
                             sbIMediaItem,
                             sbILibraryResource,
                             nsISecurityCheckedComponent)

SB_IMPL_CLASSINFO_INTERFACES_ONLY(sbRemoteSiteMediaItem)

SB_IMPL_SECURITYCHECKEDCOMP_INIT_MIXIN_LIBRES(sbRemoteSiteMediaItem,
                                              sbRemoteSiteMediaItemSecurityMixin,
                                              (mMediaItem),
                                              sbRemoteSiteLibraryResource,
                                              (mRemotePlayer, mMediaItem) )


sbRemoteSiteMediaItem::sbRemoteSiteMediaItem(sbRemotePlayer* aRemotePlayer,
                                             sbIMediaItem* aMediaItem) :
  sbRemoteMediaItem(aRemotePlayer, aMediaItem)
{
  LOG_ITEM(("sbRemoteSiteMediaItem::sbRemoteSiteMediaItem()"));
  NS_ASSERTION( aMediaItem, "Null media item!" );
}

sbRemoteSiteMediaItem::~sbRemoteSiteMediaItem()
{
  LOG_ITEM(("sbRemoteSiteMediaItem::~sbRemoteSiteMediaItem()"));
}

