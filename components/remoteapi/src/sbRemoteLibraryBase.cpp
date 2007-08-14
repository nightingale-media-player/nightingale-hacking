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

#include "sbRemoteAPI.h"
#include "sbRemoteAPIUtils.h"
#include "sbRemoteLibraryBase.h"
#include "sbRemoteMediaItem.h"
#include "sbRemoteMediaList.h"
#include "sbRemoteSiteMediaList.h"

#include <sbILibrary.h>
#include <sbILibraryManager.h>
#include <sbIMediaList.h>
#include <sbIMediaListView.h>
#include <sbIMetadataJob.h>
#include <sbIMetadataJobManager.h>
#include <sbIPlaylistReader.h>
#include <sbIWrappedMediaItem.h>
#include <sbIWrappedMediaList.h>

#include <nsComponentManagerUtils.h>
#include <nsEventDispatcher.h>
#include <nsICategoryManager.h>
#include <nsIDocument.h>
#include <nsIDOMDocument.h>
#include <nsIDOMDocumentEvent.h>
#include <nsIDOMEvent.h>
#include <nsIDOMEventTarget.h>
#include <nsIDOMWindow.h>
#include <nsIFile.h>
#include <nsIMutableArray.h>
#include <nsIObserver.h>
#include <nsIPermissionManager.h>
#include <nsIPrefService.h>
#include <nsIPresShell.h>
#include <nsIProgrammingLanguage.h>
#include <nsIScriptNameSpaceManager.h>
#include <nsIScriptSecurityManager.h>
#include <nsISupportsPrimitives.h>
#include <nsIWindowWatcher.h>
#include <nsMemory.h>
#include <nsNetUtil.h>
#include <nsServiceManagerUtils.h>
#include <nsStringEnumerator.h>
#include <nsStringGlue.h>
#include <nsTHashtable.h>
#include <prlog.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteLibraryBase:<5|4|3|2|1>
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gLibraryLog = nsnull;
#endif

#define LOG(args) PR_LOG(gLibraryLog, PR_LOG_WARN, args)

// Observer for the PlaylistReader that launches a metadata job when the
// playlist is loaded.
class sbRemoteMetadataScanLauncher : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS

  NS_IMETHODIMP Observe( nsISupports *aSubject,
                         const char *aTopic,
                         const PRUnichar *aData)
  {
    LOG(( "sbRemoteMetadataScanLauncher::Observe(%s)", aTopic ));
    nsresult rv;
    nsCOMPtr<sbIMetadataJobManager> metaJobManager =
      do_GetService( "@songbirdnest.com/Songbird/MetadataJobManager;1", &rv );
    NS_ENSURE_SUCCESS(rv, rv);

    if(NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIMutableArray> mediaItems =
                                 do_CreateInstance( NS_ARRAY_CONTRACTID, &rv );
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<sbIMediaList> mediaList ( do_QueryInterface( aSubject, &rv ) );
      NS_ENSURE_SUCCESS(rv, rv);

      PRUint32 length;
      rv = mediaList->GetLength( &length );
      NS_ENSURE_SUCCESS(rv, rv);

      for ( PRUint32 index = 0; index < length; index++ ) {

        nsCOMPtr<sbIMediaItem> item;
        rv = mediaList->GetItemByIndex( index, getter_AddRefs ( item ) );
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mediaItems->AppendElement( item, PR_FALSE );
        NS_ENSURE_SUCCESS(rv, rv);
      }

      nsCOMPtr<sbIMetadataJob> job;
      rv = metaJobManager->NewJob( mediaItems, 50, getter_AddRefs(job) );
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return NS_OK;
  }
};
NS_IMPL_ISUPPORTS1( sbRemoteMetadataScanLauncher, nsIObserver )

NS_IMPL_ISUPPORTS8( sbRemoteLibraryBase,
                    nsISecurityCheckedComponent,
                    sbISecurityAggregator,
                    sbIRemoteMediaList,
                    sbIMediaList,
                    sbIWrappedMediaList,
                    sbIWrappedMediaItem,
                    sbIMediaItem,
                    sbIRemoteLibrary )

sbRemoteLibraryBase::sbRemoteLibraryBase() : mShouldScan(PR_TRUE)
{
#ifdef PR_LOGGING
  if (!gLibraryLog) {
    gLibraryLog = PR_NewLogModule("sbRemoteLibraryBase");
  }
  LOG(("sbRemoteLibraryBase::sbRemoteLibraryBase()"));
#endif
}

sbRemoteLibraryBase::~sbRemoteLibraryBase()
{
  LOG(("sbRemoteLibraryBase::~sbRemoteLibraryBase()"));
}

// ---------------------------------------------------------------------------
//
//                        sbIRemoteLibrary
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbRemoteLibraryBase::GetScanMediaOnCreation( PRBool *aShouldScan )
{
  NS_ENSURE_ARG_POINTER(aShouldScan);
  *aShouldScan = mShouldScan;
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteLibraryBase::SetScanMediaOnCreation( PRBool aShouldScan )
{
  mShouldScan = aShouldScan;
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteLibraryBase::ConnectToDefaultLibrary( const nsAString &aLibName )
{
  LOG(( "sbRemoteLibraryBase::ConnectToDefaultLibrary(guid:%s)",
        NS_LossyConvertUTF16toASCII(aLibName).get() ));

  nsAutoString guid;
  nsresult rv = GetLibraryGUID(aLibName, guid);
  if ( NS_SUCCEEDED(rv) ) {
    LOG(( "sbRemoteLibraryBase::ConnectToDefaultLibrary(%s) -- IS a default library",
          NS_LossyConvertUTF16toASCII(guid).get() ));

    // See if the library manager has it lying around.
    nsCOMPtr<sbILibraryManager> libManager(
        do_GetService( "@songbirdnest.com/Songbird/library/Manager;1", &rv ) );
    NS_ENSURE_SUCCESS( rv, rv );

    rv = libManager->GetLibrary( guid, getter_AddRefs(mLibrary) );
    NS_ENSURE_SUCCESS( rv, rv );

    rv = InitInternalMediaList();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return rv;
}

NS_IMETHODIMP
sbRemoteLibraryBase::CreateMediaItem( const nsAString& aURL,
                                      sbIMediaItem** _retval )
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_STATE(mLibrary);

  LOG(("sbRemoteLibraryBase::CreateMediaItem()"));

  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aURL);
  NS_ENSURE_SUCCESS(rv, rv);

  // Only allow the creation of media items with http(s) schemes
  PRBool validScheme;
  uri->SchemeIs("http", &validScheme);
  if (!validScheme) {
    uri->SchemeIs("https", &validScheme);
    if (!validScheme) {
      return NS_ERROR_INVALID_ARG;
    }
  }

  nsCOMPtr<sbIMediaItem> mediaItem;
  rv = mLibrary->CreateMediaItem(uri, nsnull, getter_AddRefs(mediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  if (mShouldScan) {
    nsCOMPtr<sbIMetadataJobManager> metaJobManager =
      do_GetService("@songbirdnest.com/Songbird/MetadataJobManager;1", &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to get MetadataJobManager!");

    if(NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIMutableArray> mediaItems = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mediaItems->AppendElement(mediaItem, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<sbIMetadataJob> job;
      rv = metaJobManager->NewJob(mediaItems, 50, getter_AddRefs(job));
      NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to create metadata job!");
    }
  }

  // This will wrap in the appropriate site/regular RemoteMediaItem
  return SB_WrapMediaItem(mediaItem, _retval);
}

NS_IMETHODIMP
sbRemoteLibraryBase::CreateMediaList( const nsAString& aType,
                                      sbIRemoteMediaList** _retval )
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_STATE(mLibrary);

  LOG(("sbRemoteLibraryBase::CreateMediaList()"));

  nsCOMPtr<sbIMediaList> mediaList;
  nsresult rv = mLibrary->CreateMediaList(aType,
                                          nsnull,
                                          getter_AddRefs(mediaList));
  NS_ENSURE_SUCCESS(rv, rv);

  // This will wrap in the appropriate site/regular RemoteMediaList
  return SB_WrapMediaList(mediaList, _retval);
}

NS_IMETHODIMP
sbRemoteLibraryBase::CreateMediaListFromURL( const nsAString& aURL,
                                              sbIRemoteMediaList** _retval )
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_STATE(mLibrary);

  LOG(("sbRemoteLibraryBase::CreateMediaListFromURL()"));

  nsCOMPtr<sbIMediaList> mediaList;
  nsresult rv = mLibrary->CreateMediaList( NS_LITERAL_STRING("simple"),
                                           nsnull,
                                           getter_AddRefs(mediaList) );
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPlaylistReaderManager> manager =
       do_CreateInstance( "@songbirdnest.com/Songbird/PlaylistReaderManager;1",
                          &rv );
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), aURL);
  NS_ENSURE_SUCCESS(rv, rv);
 
  // Only allow the creation of media lists with http(s) schemes
  PRBool validScheme;
  uri->SchemeIs("http", &validScheme);
  if (!validScheme) {
    uri->SchemeIs("https", &validScheme);
    if (!validScheme) {
      return NS_ERROR_INVALID_ARG;
    }
  }

  nsCOMPtr<sbIPlaylistReaderListener> lstnr =
      do_CreateInstance( "@songbirdnest.com/Songbird/PlaylistReaderListener;1",
                         &rv );
  NS_ENSURE_SUCCESS(rv, rv);

  if (mShouldScan) {
    nsRefPtr<sbRemoteMetadataScanLauncher> launcher =
                                            new sbRemoteMetadataScanLauncher();
    NS_ENSURE_TRUE( launcher, NS_ERROR_OUT_OF_MEMORY );

    nsCOMPtr<nsIObserver> observer( do_QueryInterface( launcher, &rv ) );
    NS_ENSURE_SUCCESS(rv, rv);

    rv = lstnr->SetObserver( observer );
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRInt32 dummy;
  rv = manager->LoadPlaylist( uri, mediaList, EmptyString(), true, lstnr, &dummy );
  NS_ENSURE_SUCCESS(rv, rv);

  // This will wrap in the appropriate site/regular RemoteMediaList
  return SB_WrapMediaList(mediaList, _retval);
}

// ---------------------------------------------------------------------------
//
//                            sbIWrappedMediaList
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP_(already_AddRefed<sbIMediaItem>)
sbRemoteLibraryBase::GetMediaItem()
{
  return mRemMediaList->GetMediaItem();
}

NS_IMETHODIMP_(already_AddRefed<sbIMediaList>)
sbRemoteLibraryBase::GetMediaList()
{
  return mRemMediaList->GetMediaList();
}

// ---------------------------------------------------------------------------
//
//                            Helper Methods
//
// ---------------------------------------------------------------------------

// If the libraryID is one of the default libraries we set the out param
// to the internal GUID of library as understood by the library manager
// and return NS_OK. If it is not one of the default libraries then
// return NS_ERROR_FAILURE. Also returns an error if the pref system is not
// available.
nsresult
sbRemoteLibraryBase::GetLibraryGUID( const nsAString &aLibraryID,
                                     nsAString &aLibraryGUID )
{
#ifdef PR_LOGGING
  // This method is static, so the log might not be initialized
  if (!gLibraryLog) {
    gLibraryLog = PR_NewLogModule("sbRemoteLibraryBase");
  }
  LOG(( "sbRemoteLibraryBase::GetLibraryGUID(%s)",
        NS_LossyConvertUTF16toASCII(aLibraryID).get() ));
#endif

  nsCAutoString prefKey;

  // match the 'magic' strings to the keys for the prefs
  if ( aLibraryID.EqualsLiteral("main") ) {
    prefKey.AssignLiteral("songbird.library.main");
  } else if ( aLibraryID.EqualsLiteral("web") ) {
    prefKey.AssignLiteral("songbird.library.web");
  }

  // right now just bail if it isn't a default
  if ( prefKey.IsEmpty() ) {
    LOG(("sbRemoteLibraryBase::GetLibraryGUID() -- not a default library"));
    // ultimately we need to be able to get the GUID for non-default libraries
    //   if we are going to allow the library manager to manage them.
    // We might want to do the string hashing here and add keys for 
    //   songbird.library.site.hashkey 
    return NS_ERROR_FAILURE;
  }

  // The value of the pref should be a library resourceGUID.
  nsresult rv;
  nsCOMPtr<nsIPrefBranch> prefService =
                                 do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupportsString> supportsString;
  rv = prefService->GetComplexValue( prefKey.get(),
                                     NS_GET_IID(nsISupportsString),
                                     getter_AddRefs(supportsString) );
  //  Get the GUID for this library out of the supports string
  if (NS_SUCCEEDED(rv)) {
    // Use the value stored in the prefs.
    rv = supportsString->GetData(aLibraryGUID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

