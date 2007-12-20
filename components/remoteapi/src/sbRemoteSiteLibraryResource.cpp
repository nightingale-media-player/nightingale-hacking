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

#include <sbILibrary.h>
#include <sbIPropertyManager.h>
#include <sbIRemotePlayer.h>

#include "sbRemoteSiteLibraryResource.h"
#include <sbStandardProperties.h>
#include <nsServiceManagerUtils.h>
#include <prlog.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteLibraryResource:5
 *   LOG_RES() defined in sbRemoteLibraryResource.h/.cpp
 */

NS_IMPL_ISUPPORTS1( sbRemoteSiteLibraryResource,
                    sbILibraryResource )

sbRemoteSiteLibraryResource::sbRemoteSiteLibraryResource( sbRemotePlayer *aRemotePlayer,
                                                          sbIMediaItem *aMediaItem ) :
  sbRemoteLibraryResource( aRemotePlayer, aMediaItem )
{
  NS_ASSERTION( aRemotePlayer, "Null remote player!" );
  NS_ASSERTION( aMediaItem, "Null media item!" );

  LOG_RES(("sbRemoteSiteLibraryResource::sbRemoteSiteLibraryResource()"));
}

sbRemoteSiteLibraryResource::~sbRemoteSiteLibraryResource() {
  LOG_RES(("sbRemoteSiteLibraryResource::~sbRemoteSiteLibraryResource()"));
}

// ----------------------------------------------------------------------------
//
//                          sbILibraryResource
//
// ----------------------------------------------------------------------------

NS_IMETHODIMP
sbRemoteSiteLibraryResource::GetProperty( const nsAString &aID,
                                          nsAString &_retval )
{
  LOG_RES(( "sbRemoteSiteLibraryResource::GetProperty(%s)",
            NS_LossyConvertUTF16toASCII(aID).get() ));

  // First call the base class impl and see what it says. contentURL will get
  // denied as it is not remoteReadable by default, we special case for site
  // objects
  nsresult rv = sbRemoteLibraryResource::GetProperty( aID, _retval );
  if ( NS_SUCCEEDED(rv) || !aID.EqualsLiteral(SB_PROPERTY_CONTENTURL) ) {
    return rv;
  }

  // Do some special checking for contentURL properties. Sites should be able
  // to access the contentURLs for tracks that aren't on the hard drive.
  nsString contentURL;
  rv = mMediaItem->GetProperty( aID, contentURL );
  NS_ENSURE_SUCCESS( rv, rv );

  if ( StringBeginsWith( contentURL, NS_LITERAL_STRING("file:") ) ) {
    LOG_RES(( "sbRemoteSiteLibraryResource::GetProperty() - "
              "Disallowing access to 'file:' URI from contentURL property" ));
    return NS_ERROR_FAILURE;
  }
  LOG_RES(( "sbRemoteSiteLibraryResource::GetProperty() - "
            "Allowing access to non-file contentURL to site item/list/lib: %s",
            NS_LossyConvertUTF16toASCII(contentURL).get() ));

  _retval.Assign(contentURL);
  return NS_OK;
}

