/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

#include <sbILibrary.h>
#include <sbIPropertyManager.h>

#include "sbRemoteLibraryResource.h"
#include <sbStandardProperties.h>
#include <prlog.h>
#include <nsServiceManagerUtils.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteLibraryResource:5
 */
#ifdef PR_LOGGING
PRLogModuleInfo* gRemoteLibResLog = nsnull;
#endif

NS_IMPL_ISUPPORTS1( sbRemoteLibraryResource,
                    sbILibraryResource )

sbRemoteLibraryResource::sbRemoteLibraryResource( sbRemotePlayer *aRemotePlayer,
                                                  sbIMediaItem *aMediaItem ) :
  mRemotePlayer(aRemotePlayer),
  mMediaItem(aMediaItem)
{
  NS_ASSERTION( aRemotePlayer, "Null remote player!" );
  NS_ASSERTION( aMediaItem, "Null media item!" );

#ifdef PR_LOGGING
  if (!gRemoteLibResLog) {
    gRemoteLibResLog = PR_NewLogModule("sbRemoteLibraryResource");
  }
  LOG_RES(("sbRemoteLibraryResource::sbRemoteLibraryResource()"));
#endif
}

sbRemoteLibraryResource::~sbRemoteLibraryResource() {
  LOG_RES(("sbRemoteLibraryResource::~sbRemoteLibraryResource()"));
}

// ----------------------------------------------------------------------------
//
//                          sbILibraryResource
//
// ----------------------------------------------------------------------------
NS_IMETHODIMP
sbRemoteLibraryResource::GetProperty( const nsAString &aID,
                                      nsAString &_retval )
{
  NS_ENSURE_TRUE( mMediaItem, NS_ERROR_NULL_POINTER );
  LOG_RES(( "sbRemoteLibraryResource::GetProperty(%s)",
             NS_LossyConvertUTF16toASCII(aID).get() ));

  nsresult rv;

  // get the property manager service
  nsCOMPtr<sbIPropertyManager> propertyManager =
      do_GetService( "@getnightingale.com/Nightingale/Properties/PropertyManager;1",
                     &rv );
  NS_ENSURE_SUCCESS( rv, rv );

  // get the property info for the property being requested
  nsCOMPtr<sbIPropertyInfo> propertyInfo;
  rv = propertyManager->GetPropertyInfo( aID, getter_AddRefs(propertyInfo) );
  NS_ENSURE_SUCCESS( rv, rv );

  // ask if this property is remotely readable
  PRBool readable;
  rv = propertyInfo->GetRemoteReadable(&readable);
  NS_ENSURE_SUCCESS( rv, rv );

  // well, is it readable?
  if (!readable) {
    LOG_RES(( "Attempting to get a property's (%s) value that is not allowed "
              "to be read from the remote API!",
              NS_LossyConvertUTF16toASCII(aID).get() ));
    // if not return an error
    return NS_ERROR_FAILURE;
  }

  // readable property
  nsString propVal;
  rv = mMediaItem->GetProperty( aID, propVal );
  NS_ENSURE_SUCCESS( rv, rv );

  // Protect against exposing file:// uris to the world.
  if ( !aID.EqualsLiteral(SB_PROPERTY_ORIGINURL) ||
       !aID.EqualsLiteral(SB_PROPERTY_COPYRIGHTURL) ||
       !aID.EqualsLiteral(SB_PROPERTY_PRIMARYIMAGEURL) ) {

    if ( StringBeginsWith( propVal, NS_LITERAL_STRING("file:") ) ) {
      LOG_RES(( "sbRemoteLibraryResource::GetProperty() - "
                "Disallowing access to 'file:' URI." ));
      // Assign a dummy signal value
      propVal.AssignLiteral("__BLOCKED__");
    } else {
      LOG_RES(( "sbRemoteLibraryResource::GetProperty() - "
                "Allowing access to non-file value: %s",
                NS_LossyConvertUTF16toASCII(propVal).get() ));
    }
  }

  _retval.Assign(propVal);
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteLibraryResource::SetProperty( const nsAString &aID,
                                      const nsAString &aValue )
{
  LOG_RES(( "sbRemoteLibraryResource::SetProperty( %s, %s )",
            NS_LossyConvertUTF16toASCII(aID).get(),
            NS_LossyConvertUTF16toASCII(aValue).get() ));

  NS_ASSERTION( mMediaItem, "SetProperty called before Initialization" ); 

  nsresult rv = NS_OK;

  // get the property manager service
  nsCOMPtr<sbIPropertyManager> propertyManager =
      do_GetService( "@getnightingale.com/Nightingale/Properties/PropertyManager;1",
                     &rv );
  NS_ENSURE_SUCCESS( rv, rv );

  // Check to see if we have the property first, if not we must create it with
  // the right settings so websites can modify it.
  PRBool hasProp;
  rv = propertyManager->HasProperty( aID, &hasProp );

  // get the property info for the property being requested - this will create
  // it if it didn't already exist.
  nsCOMPtr<sbIPropertyInfo> propertyInfo;
  rv = propertyManager->GetPropertyInfo( aID, getter_AddRefs(propertyInfo) );
  NS_ENSURE_SUCCESS( rv, rv );

  // check to see if the prop already existed or if we created it
  if (hasProp) {
    // ask if this property is remotely writable
    PRBool writable = PR_FALSE;
    rv = propertyInfo->GetRemoteWritable(&writable);
    NS_ENSURE_SUCCESS( rv, rv );

    // well, is it writeable?
    if (!writable) {
      // if not return an error
      LOG_RES(( "Attempting to set a property's (%s) value that is not allowed "
                "to be set from the remote API!",
                NS_LossyConvertUTF16toASCII(aID).get() ));
      return NS_ERROR_FAILURE;
    }
  }
  else {
    // we created a new property in the system so enable remote write/read
    rv = propertyInfo->SetRemoteWritable(PR_TRUE);
    NS_ENSURE_SUCCESS( rv, rv );

    rv = propertyInfo->SetRemoteReadable(PR_TRUE);
    NS_ENSURE_SUCCESS( rv, rv );
  }

  // it all looks ok, pass this request on to the real media item
  rv = mMediaItem->SetProperty( aID, aValue );
  NS_ENSURE_SUCCESS( rv, rv );

  nsCOMPtr<sbILibrary> library;
  rv = mMediaItem->GetLibrary( getter_AddRefs(library) );
  NS_ENSURE_SUCCESS( rv, rv );

  mRemotePlayer->GetNotificationManager()->Action(
    sbRemoteNotificationManager::eEditedItems, library );

  return NS_OK;
}

NS_IMETHODIMP
sbRemoteLibraryResource::SetProperties( sbIPropertyArray *aProperties )
{
  LOG_RES(("sbRemoteLibraryResource::SetProperties()"));
  NS_ENSURE_ARG_POINTER( aProperties );
  NS_ASSERTION( mMediaItem, "SetProperties called before Initialization" ); 

  nsresult rv = mMediaItem->SetProperties(aProperties);
  NS_ENSURE_SUCCESS( rv, rv );

  nsCOMPtr<sbILibrary> library;
  rv = mMediaItem->GetLibrary( getter_AddRefs(library) );
  NS_ENSURE_SUCCESS( rv, rv );

  mRemotePlayer->GetNotificationManager()->Action(
    sbRemoteNotificationManager::eEditedItems, library );

  return NS_OK;
}

