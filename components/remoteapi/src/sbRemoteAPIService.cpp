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

#include "sbRemoteAPIService.h"

#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIObserverService.h"

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteAPIService:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gRemoteAPIServiceLog = nsnull;
#endif

#undef LOG
#define LOG(args) PR_LOG(gRemoteAPIServiceLog, PR_LOG_WARN, args)

NS_IMPL_ISUPPORTS2(sbRemoteAPIService,
                   sbIRemoteAPIService,
                   nsIObserver)

sbRemoteAPIService::sbRemoteAPIService()
{
#ifdef PR_LOGGING
  if (!gRemoteAPIServiceLog) {
    gRemoteAPIServiceLog = PR_NewLogModule("sbRemoteAPIService");
  }
#endif
  LOG(("sbRemoteAPIService::sbRemoteAPIService()"));
}

sbRemoteAPIService::~sbRemoteAPIService()
{
  LOG(("sbRemoteAPIService::~sbRemoteAPIService()"));
}

// ---------------------------------------------------------------------------
//
//                           sbIRemoteAPIService
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbRemoteAPIService::Init()
{
  const char* remotes[] = {
    "faceplate.volume",
    "playlist.repeat",
    "playlist.shuffle",
    "playback.control.triggered",
    nsnull };
  
  nsresult rv;
  
  nsCOMPtr<nsIObserverService> obs =
    do_GetService( "@mozilla.org/observer-service;1", &rv );
  NS_ENSURE_SUCCESS( rv, rv );
  obs->AddObserver( this, "quit-application", PR_FALSE );
  
  for (int i = 0; remotes[i]; ++i) {
    nsCOMPtr<sbIDataRemote> dr =
      do_CreateInstance( "@songbirdnest.com/Songbird/DataRemote;1", &rv );
    NS_ENSURE_SUCCESS( rv, rv );
    
    rv = dr->Init( NS_ConvertASCIItoUTF16(remotes[i]), EmptyString() );
    NS_ENSURE_SUCCESS( rv, rv );
    
    rv = dr->BindObserver( this, PR_FALSE );
    NS_ENSURE_SUCCESS( rv, rv );
    
    mDataRemotes.AppendObject(dr);
  }
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteAPIService::HasPlaybackControl(nsIURI *aURI, PRBool *_retval)
{
  NS_ENSURE_ARG(aURI);
  NS_ENSURE_ARG(_retval);
  
#ifdef PR_LOGGING
  nsCString spec, controllerSpec;
  nsresult rv = aURI->GetSpec( spec );
  if ( NS_FAILED(rv) ) {
    spec.AssignLiteral("[unknown spec]");
  }

  if (mPlaybackControllerURI) {
    rv = aURI->GetSpec( controllerSpec );
    if ( NS_FAILED(rv) ) {
      controllerSpec.AssignLiteral("[unknown spec]");
    }
  } else {
    controllerSpec.AssignLiteral("[no uri]");
  }

  LOG(("sbRemoteAPIService::HasPlaybackControl() - \n\t%s\n\t%s",
       spec.BeginReading(),
       controllerSpec.BeginReading()));
#endif /* PR_LOGGING */
  if (!mPlaybackControllerURI) {
    // not playing from the web
    *_retval = PR_FALSE;
    return NS_OK;
  }
  return aURI->Equals(mPlaybackControllerURI, _retval);
}

NS_IMETHODIMP
sbRemoteAPIService::TakePlaybackControl(nsIURI *aURI, nsIURI **_retval)
{
  #ifdef PR_LOGGING
  nsCString spec;
  if (aURI) {
    nsresult rv = aURI->GetSpec( spec );
    if ( NS_FAILED(rv) ) {
      spec.AssignLiteral("[no spec]");
    }
    LOG(("sbRemoteAPIService::TakePlaybackControl() - assigned spec %s",
         spec.BeginReading() ));
  } else {
    LOG(("sbRemoteAPIService::TakePlaybackControl() - clearing" ));
  }
  #endif
  if ( NS_LIKELY(_retval) ) {
    NS_IF_ADDREF( *_retval = mPlaybackControllerURI );
  }
  mPlaybackControllerURI = aURI;
  return NS_OK;
}


// ---------------------------------------------------------------------------
//
//                           nsIObserver
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbRemoteAPIService::Observe( nsISupports *aSubject,
                         const char *aTopic,
                         const PRUnichar *aData )
{
  LOG(("sbRemoteAPIService::Observe() - %s", aTopic));
  nsresult rv;
  
  if (!strcmp( "quit-application", aTopic ) ) {
    // clean up
    for (int i = 0; i < mDataRemotes.Count(); ++i) {
      mDataRemotes[i]->Unbind();
    }

    nsCOMPtr<nsIObserverService> obs =
      do_GetService( "@mozilla.org/observer-service;1", &rv );
    NS_ENSURE_SUCCESS( rv, rv );
    rv = obs->RemoveObserver( this, "quit-application" );
    NS_ENSURE_SUCCESS( rv, rv );
  } else {
    // triggered by data remotes
  
    if (!mPlaybackControllerURI) {
      // we have no content page in control
      return NS_OK;
    }
    
    // assume it's one of the data remotes
    // if the change was in fact initiated by the remote player, we'll clear the
    // URI here and then re-set it in the caller, so we don't have to worry about
    // clearing things when we shouldn't
    
    LOG(( "Cleared controller due to observing %s", aTopic ));
    mPlaybackControllerURI = nsnull;
  }
  
  return NS_OK;
}
