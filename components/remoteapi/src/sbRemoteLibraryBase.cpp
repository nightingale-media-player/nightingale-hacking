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

#include "sbRemoteLibraryBase.h"
#include "sbRemotePlayer.h"
#include "sbRemoteWrappingSimpleEnumerator.h"

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
#include <nsIPrefService.h>
#include <nsIPresShell.h>
#include <nsISimpleEnumerator.h>
#include <nsIStringEnumerator.h>
#include <nsISupportsPrimitives.h>
#include <nsIWindowWatcher.h>
#include <sbILibrary.h>
#include <sbILibraryManager.h>
#include <sbIMediaList.h>
#include <sbIMediaListView.h>
#include <sbIMetadataJobManager.h>
#include <sbIPlaylistReader.h>
#include <sbIPropertyArray.h>
#include <sbIWrappedMediaItem.h>
#include <sbIWrappedMediaList.h>

#include <nsArrayEnumerator.h>
#include <nsCOMArray.h>
#include <nsEnumeratorUtils.h>
#include <nsEventDispatcher.h>
#include <nsNetUtil.h>
#include <nsServiceManagerUtils.h>
#include <nsStringEnumerator.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <nsTHashtable.h>
#include <prlog.h>
#include <sbPropertiesCID.h>
#include "sbRemoteMediaItem.h"
#include "sbRemoteMediaList.h"
#include "sbRemoteSiteMediaList.h"
#include "sbRemoteAPIUtils.h"
#include <sbStandardProperties.h>
#include "sbURIChecker.h"

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteLibraryBase:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gLibraryLog = nsnull;
#endif

#undef LOG
#define LOG(args) PR_LOG(gLibraryLog, PR_LOG_WARN, args)

// Observer for the PlaylistReader that notifies the callback and optionally
// launches a metadata job when the playlist is loaded.
class sbPlaylistReaderObserver : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS

  sbPlaylistReaderObserver(sbRemotePlayer* aRemotePlayer,
                           sbICreateMediaListCallback* aCallback,
                           PRBool aShouldScan) :
    mRemotePlayer(aRemotePlayer),
    mCallback(aCallback),
    mShouldScan(aShouldScan)
  {
    NS_ASSERTION(aRemotePlayer, "aRemotePlayer is null");
    NS_ASSERTION(aCallback, "aCallback is null");
  }

  NS_IMETHODIMP Observe( nsISupports *aSubject,
                         const char *aTopic,
                         const PRUnichar *aData)
  {
    NS_ENSURE_ARG_POINTER(aSubject);
    LOG(( "sbPlaylistReaderObserver::Observe(%s)", aTopic ));

    nsresult rv;

    nsCOMPtr<sbIMediaList> mediaList ( do_QueryInterface( aSubject, &rv ) );
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 length;
    rv = mediaList->GetLength( &length );
    NS_ENSURE_SUCCESS(rv, rv);

    if (mShouldScan && length) {
      nsCOMPtr<sbIMetadataJobManager> metaJobManager =
        do_GetService( "@songbirdnest.com/Songbird/MetadataJobManager;1", &rv );
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIMutableArray> mediaItems =
                                 do_CreateInstance( NS_ARRAY_CONTRACTID, &rv );
      NS_ENSURE_SUCCESS(rv, rv);

      for ( PRUint32 index = 0; index < length; index++ ) {

        nsCOMPtr<sbIMediaItem> item;
        rv = mediaList->GetItemByIndex( index, getter_AddRefs ( item ) );
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mediaItems->AppendElement( item, PR_FALSE );
        NS_ENSURE_SUCCESS(rv, rv);
      }

      // We will most likely have to create a new job.
      PRBool createJob = PR_TRUE;

      // Already have a job, check completion status.
      if (mMetadataJob) {
        PRBool completed;

        rv = mMetadataJob->GetCompleted(&completed);
        NS_ENSURE_SUCCESS(rv, rv);

        // Job hasn't completed yet, attempt to append to it.
        if (!completed) {
          rv = mMetadataJob->Append( mediaItems );

          // If the append succeeds, there is no need to create a new job.
          if(NS_SUCCEEDED(rv)) {
            createJob = PR_FALSE;
          }
        }
      }

      // Create a new metadata job if required.
      if (createJob) {
        nsCOMPtr<sbIMetadataJob> job;

        rv = metaJobManager->NewJob( mediaItems, 150, getter_AddRefs(job) );
        NS_ENSURE_SUCCESS(rv, rv);

        mMetadataJob = job;
      }
    }

    if (mCallback) {
      nsCOMPtr<sbIMediaList> wrappedList;
      rv = SB_WrapMediaList(mRemotePlayer,
                            mediaList,
                            getter_AddRefs(wrappedList));
      NS_ENSURE_SUCCESS(rv, rv);
      mCallback->OnCreated(wrappedList);
    }

    return NS_OK;
  }

private:

  nsRefPtr<sbRemotePlayer> mRemotePlayer;
  nsCOMPtr<sbICreateMediaListCallback> mCallback;
  PRBool mShouldScan;

  nsCOMPtr<sbIMetadataJob> mMetadataJob;
};
NS_IMPL_ISUPPORTS1( sbPlaylistReaderObserver, nsIObserver )

class sbRemoteLibraryEnumCallback : public sbIMediaListEnumerationListener
{
  public:
    NS_DECL_ISUPPORTS

    sbRemoteLibraryEnumCallback( nsCOMArray<sbIMediaItem>& aArray ) :
      mArray(aArray) { }

    NS_IMETHODIMP OnEnumerationBegin( sbIMediaList*, PRBool* _retval )
    {
      NS_ENSURE_ARG(_retval);
      *_retval = PR_TRUE;
      return NS_OK;
    }
    NS_IMETHODIMP OnEnumerationEnd( sbIMediaList*, nsresult )
    {
      return NS_OK;
    }
    NS_IMETHODIMP OnEnumeratedItem( sbIMediaList*, sbIMediaItem* aItem, PRBool* _retval )
    {
      NS_ENSURE_ARG(_retval);
      *_retval = PR_TRUE;

      mArray.AppendObject( aItem );

      return NS_OK;
    }
  private:
    nsCOMArray<sbIMediaItem>& mArray;
};
NS_IMPL_ISUPPORTS1( sbRemoteLibraryEnumCallback, sbIMediaListEnumerationListener )

/**
 * Simple struct to allow sorting of scopeURLs
 */
struct sbRemoteLibraryScopeURLSet {
  sbRemoteLibraryScopeURLSet( const nsACString& path,
                              sbIMediaItem* item )
  : scopePath(path),
    item(item),
    length(path.Length())
  {
    NS_ASSERTION( item, "Null pointer!");
  }

  PRBool operator ==(const sbRemoteLibraryScopeURLSet& rhs) const
  {
    return (length == rhs.length) && (scopePath.Equals(rhs.scopePath));
  }

  PRBool operator <(const sbRemoteLibraryScopeURLSet& rhs) const
  {
    return length < rhs.length;
  }

  const nsCString scopePath;
  const nsCOMPtr<sbIMediaItem> item;
  const PRUint32 length;
};


NS_IMPL_ISUPPORTS9( sbRemoteLibraryBase,
                    nsISecurityCheckedComponent,
                    sbISecurityAggregator,
                    sbIRemoteMediaList,
                    sbIMediaList,
                    sbIWrappedMediaList,
                    sbIWrappedMediaItem,
                    sbIMediaItem,
                    sbILibraryResource,
                    sbIRemoteLibrary )

sbRemoteLibraryBase::sbRemoteLibraryBase(sbRemotePlayer* aRemotePlayer) :
  mShouldScan(PR_TRUE),
  mEnumerationResult(NS_ERROR_NOT_INITIALIZED),
  mRemotePlayer(aRemotePlayer)
{
  NS_ASSERTION(aRemotePlayer, "aRemotePlayer is null");
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

  // We use PR_TRUE here so that 2 sites can create a media item to the same
  // URI and get 2 different objects in the database and don't overwrite
  // each other.
  nsCOMPtr<sbIMediaItem> mediaItem;
  rv = mLibrary->CreateMediaItem(uri,
                                 nsnull,
                                 PR_TRUE,
                                 getter_AddRefs(mediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the OriginPage property so we know where it came from
  rv = mRemotePlayer->SetOriginScope( mediaItem, aURL );
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

      // We will most likely have to create a new job.
      PRBool createJob = PR_TRUE;

      // Already have a job, check completion status.
      if (mMetadataJob) {
        PRBool completed;

        rv = mMetadataJob->GetCompleted(&completed);
        NS_ENSURE_SUCCESS(rv, rv);

        // Job hasn't completed yet, attempt to append to it.
        if (!completed) {
          rv = mMetadataJob->Append( mediaItems );

          // If the append succeeds, there is no need to create a new job.
          if(NS_SUCCEEDED(rv)) {
            createJob = PR_FALSE;
          }
        }
      }

      // Create a new metadata job if required.
      if (createJob) {
        nsCOMPtr<sbIMetadataJob> job;

        rv = metaJobManager->NewJob( mediaItems, 150, getter_AddRefs(job) );
        NS_ENSURE_SUCCESS(rv, rv);

        mMetadataJob = job;
      }
    }
  }

  // This will wrap in the appropriate site/regular RemoteMediaItem
  rv = SB_WrapMediaItem(mRemotePlayer, mediaItem, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  mRemotePlayer->GetNotificationManager()
    ->Action(sbRemoteNotificationManager::eUpdatedWithItems, mLibrary);

  return NS_OK;
}

NS_IMETHODIMP
sbRemoteLibraryBase::CreateSimpleMediaList( const nsAString& aName,
                                            const nsAString& aSiteID,
                                            sbIRemoteMediaList** _retval )
{
  NS_ENSURE_ARG(!aName.IsEmpty());
  NS_ENSURE_ARG_POINTER(_retval);

  nsString siteID;
  if (aSiteID.IsEmpty()) {
    siteID.Assign(aName);
  }
  else {
    siteID.Assign(aSiteID);
  }

  nsresult rv;
  nsCOMPtr<sbIMediaList> mediaList;
  nsCOMPtr<sbIRemoteMediaList> remMediaList = GetMediaListBySiteID(siteID);
  if (remMediaList) {
    nsCOMPtr<sbIWrappedMediaList> wrappedList =
      do_QueryInterface( remMediaList, &rv );
    NS_ENSURE_SUCCESS( rv, rv );

    mediaList = wrappedList->GetMediaList();
    NS_ENSURE_TRUE( mediaList, NS_ERROR_FAILURE );
  }
  else {
    // Now we create one.
    rv = mLibrary->CreateMediaList( NS_LITERAL_STRING("simple"), nsnull,
                                    getter_AddRefs(mediaList) );
    NS_ENSURE_SUCCESS( rv, rv );

    // Set the OriginPage property so we know where it came from
    nsCOMPtr<sbIMediaItem> listAsItem( do_QueryInterface( mediaList, &rv ) );
    NS_ENSURE_SUCCESS( rv, rv) ;

    rv = mRemotePlayer->SetOriginScope( listAsItem, siteID );
    NS_ENSURE_SUCCESS( rv, rv );

    rv = SB_WrapMediaList( mRemotePlayer, mediaList,
                           getter_AddRefs(remMediaList) );
    NS_ENSURE_SUCCESS( rv, rv );
  }

  rv = mediaList->SetProperty( NS_LITERAL_STRING(SB_PROPERTY_MEDIALISTNAME),
                               aName );
  NS_ENSURE_SUCCESS( rv, rv );

  mRemotePlayer->GetNotificationManager()->
    Action( sbRemoteNotificationManager::eUpdatedWithPlaylists, mLibrary );

  NS_ADDREF(*_retval = remMediaList);
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteLibraryBase::CreateMediaListFromURL( const nsAString& aName,
                                             const nsAString& aURL,
                                             sbICreateMediaListCallback* aCallback,
                                             const nsAString& aSiteID )
{
  NS_ENSURE_ARG(!aName.IsEmpty());
  NS_ENSURE_ARG(!aURL.IsEmpty());
  NS_ENSURE_STATE(mLibrary);

  LOG(("sbRemoteLibraryBase::CreateMediaListFromURL()"));

  nsString siteID;
  if (aSiteID.IsEmpty()) {
    siteID.Assign(aName);
  }
  else {
    siteID.Assign(aSiteID);
  }

  nsresult rv;
  nsCOMPtr<sbIMediaList> mediaList;
  nsCOMPtr<sbIRemoteMediaList> remMediaList = GetMediaListBySiteID(siteID);
  if (remMediaList) {
    nsCOMPtr<sbIWrappedMediaList> wrappedList =
      do_QueryInterface( remMediaList, &rv );
    NS_ENSURE_SUCCESS( rv, rv );

    mediaList = wrappedList->GetMediaList();
    NS_ENSURE_TRUE( mediaList, NS_ERROR_FAILURE );
  }
  else {
    rv = mLibrary->CreateMediaList( NS_LITERAL_STRING("simple"), nsnull,
                                    getter_AddRefs(mediaList) );
    NS_ENSURE_SUCCESS(rv, rv);

    // Set the OriginPage property so we know where it came from
    nsCOMPtr<sbIMediaItem> mediaItem(do_QueryInterface(mediaList));
    NS_ENSURE_TRUE(mediaItem, NS_ERROR_FAILURE);

    rv = mRemotePlayer->SetOriginScope( mediaItem, siteID );
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mediaList->SetProperty( NS_LITERAL_STRING(SB_PROPERTY_MEDIALISTNAME),
                               aName );
  NS_ENSURE_SUCCESS( rv, rv );

  mRemotePlayer->GetNotificationManager()
      ->Action(sbRemoteNotificationManager::eUpdatedWithPlaylists, mLibrary);

  nsCOMPtr<sbIPlaylistReaderManager> manager =
       do_GetService( "@songbirdnest.com/Songbird/PlaylistReaderManager;1",
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

  nsRefPtr<sbPlaylistReaderObserver> readerObserver =
    new sbPlaylistReaderObserver(mRemotePlayer, aCallback, mShouldScan);
  NS_ENSURE_TRUE( readerObserver, NS_ERROR_OUT_OF_MEMORY );

  nsCOMPtr<nsIObserver> observer( do_QueryInterface( readerObserver, &rv ) );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = lstnr->SetObserver( observer );
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dummy;
  rv = manager->LoadPlaylist( uri, mediaList, EmptyString(), true, lstnr, &dummy );
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbRemoteLibraryBase::GetMediaListBySiteID( const nsAString &aSiteID,
                                           sbIRemoteMediaList **_retval )
{
  NS_ENSURE_ARG(!aSiteID.IsEmpty());
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIRemoteMediaList> list = GetMediaListBySiteID(aSiteID);
  NS_IF_ADDREF(*_retval = list);
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteLibraryBase::GetArtists( nsIStringEnumerator** _retval )
{
  LOG(("sbRemoteLibraryBase::GetArtists()"));
  return GetDistinctValuesForProperty( NS_LITERAL_STRING(SB_PROPERTY_ARTISTNAME),
                                       _retval );
}

NS_IMETHODIMP
sbRemoteLibraryBase::GetAlbums( nsIStringEnumerator** _retval )
{
  LOG(("sbRemoteLibraryBase::GetAlbums()"));

  return GetDistinctValuesForProperty( NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME),
                                       _retval );
}

NS_IMETHODIMP
sbRemoteLibraryBase::GetGenres( nsIStringEnumerator** _retval )
{
  LOG(("sbRemoteLibraryBase::GetGenres()"));

  return GetDistinctValuesForProperty( NS_LITERAL_STRING(SB_PROPERTY_GENRE),
                                       _retval );
}

NS_IMETHODIMP
sbRemoteLibraryBase::GetYears( nsIStringEnumerator** _retval )
{
  LOG(("sbRemoteLibraryBase::GetYears()"));

  return GetDistinctValuesForProperty( NS_LITERAL_STRING(SB_PROPERTY_YEAR),
                                       _retval );
}

NS_IMETHODIMP
sbRemoteLibraryBase::GetPlaylists( nsISimpleEnumerator** _retval )
{
  LOG(("sbRemoteLibraryBase::GetPlaylists()"));

  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_STATE(mLibrary);

  nsresult rv;
  nsCOMPtr<sbIMediaList> mediaList = do_QueryInterface( mLibrary, &rv );
  NS_ENSURE_SUCCESS( rv, rv );

  rv = mediaList->EnumerateItemsByProperty( NS_LITERAL_STRING(SB_PROPERTY_ISLIST),
                                            NS_LITERAL_STRING("1"),
                                            this,
                                            sbIMediaList::ENUMERATIONTYPE_SNAPSHOT );
  NS_ENSURE_SUCCESS( rv, rv );

  nsCOMPtr<nsISimpleEnumerator> playlistEnum;
  if ( NS_SUCCEEDED(mEnumerationResult) ) {
    // Make an enumerator for the contents of mEnumerationArray.
    if ( mEnumerationArray.Count() ) {
      rv = NS_NewArrayEnumerator( getter_AddRefs(playlistEnum),
                                  mEnumerationArray );
    }
    else {
      rv = NS_NewEmptyEnumerator( getter_AddRefs(playlistEnum) );
    }

    if ( NS_FAILED(rv) ) {
      NS_WARNING("Failed to make array enumerator");
    }
  }
  else {
    NS_WARNING("Item enumeration failed!");
    rv = mEnumerationResult;
  }

  if (!playlistEnum) {
    *_retval = nsnull;
    return NS_OK;
  }

  nsRefPtr<sbRemoteWrappingSimpleEnumerator> wrapped(
           new sbRemoteWrappingSimpleEnumerator(mRemotePlayer, playlistEnum) );
  NS_ENSURE_TRUE( wrapped, NS_ERROR_OUT_OF_MEMORY );

  rv = wrapped->Init();
  NS_ENSURE_SUCCESS( rv, rv );

  NS_ADDREF( *_retval = wrapped );

  // Reset the array and result codes for next time.
  mEnumerationArray.Clear();
  mEnumerationResult = NS_ERROR_NOT_INITIALIZED;

  return rv;
}

// ---------------------------------------------------------------------------
//
//                            sbIWrappedMediaList
//
// ---------------------------------------------------------------------------

already_AddRefed<sbIMediaItem>
sbRemoteLibraryBase::GetMediaItem()
{
  return mRemMediaList->GetMediaItem();
}

already_AddRefed<sbIMediaList>
sbRemoteLibraryBase::GetMediaList()
{
  return mRemMediaList->GetMediaList();
}

// ---------------------------------------------------------------------------
//
//                     sbIMediaListEnumerationListener
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbRemoteLibraryBase::OnEnumerationBegin( sbIMediaList *aMediaList,
                                         PRBool *_retval )
{
  NS_ENSURE_ARG_POINTER(_retval);

  NS_ASSERTION( mEnumerationArray.Count() == 0,
                "Someone forgot to clear mEnumerationArray!" );
  NS_ASSERTION( mEnumerationResult == NS_ERROR_NOT_INITIALIZED,
                "Someone forgot to reset mEnumerationResult!" );

  *_retval = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteLibraryBase::OnEnumeratedItem( sbIMediaList *aMediaList,
                                       sbIMediaItem *aMediaItem,
                                       PRBool *_retval )
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = mEnumerationArray.AppendObject(aMediaItem);
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteLibraryBase::OnEnumerationEnd( sbIMediaList *aMediaList,
                                       nsresult aStatusCode )
{
  mEnumerationResult = aStatusCode;
  return NS_OK;
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

already_AddRefed<sbIRemoteMediaList>
sbRemoteLibraryBase::GetMediaListBySiteID(const nsAString& aSiteID)
{
  NS_ASSERTION(!aSiteID.IsEmpty(), "Don't give me an empty ID!");

  nsresult rv;
  nsCOMPtr<sbIMutablePropertyArray> mutableArray =
    do_CreateInstance( SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv );
  NS_ENSURE_SUCCESS( rv, nsnull );

  rv = mutableArray->AppendProperty( NS_LITERAL_STRING(SB_PROPERTY_RAPISITEID),
                                     aSiteID );
  NS_ENSURE_SUCCESS( rv, nsnull );

  rv = mutableArray->AppendProperty( NS_LITERAL_STRING(SB_PROPERTY_ISLIST),
                                     NS_LITERAL_STRING("1") );
  NS_ENSURE_SUCCESS( rv, nsnull );

  nsCOMArray<sbIMediaItem> items;
  nsRefPtr<sbRemoteLibraryEnumCallback> listener =
    new sbRemoteLibraryEnumCallback(items);
  NS_ENSURE_TRUE( listener, nsnull );

  nsCOMPtr<sbIMediaList> libList = do_QueryInterface( mLibrary, &rv );
  NS_ENSURE_SUCCESS( rv, nsnull );

  rv = libList->EnumerateItemsByProperties( mutableArray,
                                            listener,
                                            sbIMediaList::ENUMERATIONTYPE_SNAPSHOT );
  NS_ENSURE_SUCCESS( rv, nsnull );

  if (items.Count() > 0) {
    nsCOMPtr<sbIMediaItem> foundItem = FindMediaItemWithMatchingScope(items);
    if (foundItem) {
      nsCOMPtr<sbIMediaList> list = do_QueryInterface( foundItem, &rv );
      NS_ASSERTION( NS_SUCCEEDED(rv), "Failed to QI to sbIMediaList!" );

      nsCOMPtr<sbIRemoteMediaList> retval;
      rv = SB_WrapMediaList( mRemotePlayer, list, getter_AddRefs(retval) );
      NS_ENSURE_SUCCESS( rv, nsnull );

      return retval.forget();
    }
  }

  // if we reach this point, we did not find the media list, return nsnull.
  return nsnull;
}

already_AddRefed<sbIMediaItem>
sbRemoteLibraryBase::FindMediaItemWithMatchingScope( const nsCOMArray<sbIMediaItem>& aMediaItems )
{
  nsCOMPtr<nsIURI> siteScopeURI = mRemotePlayer->GetSiteScopeURI();
  NS_ENSURE_TRUE( siteScopeURI, nsnull );

  nsCString siteHost;
  nsresult rv = siteScopeURI->GetHost(siteHost);
  NS_ENSURE_SUCCESS( rv, nsnull );

  PRUint32 itemCount = (PRUint32)aMediaItems.Count();
  NS_ASSERTION(itemCount > 0, "Empty items list!");

  // Build an array of site scope URLs
  nsTArray<sbRemoteLibraryScopeURLSet> scopeURLSet(itemCount);

  for (PRUint32 itemIndex = 0; itemIndex < itemCount; itemIndex++) {
    const nsCOMPtr<sbIMediaItem>& item = aMediaItems.ObjectAt(itemIndex);

    nsString scopeURL;
    rv = item->GetProperty( NS_LITERAL_STRING(SB_PROPERTY_RAPISCOPEURL),
                            scopeURL );
    NS_ENSURE_SUCCESS( rv, nsnull );

    NS_ASSERTION( !scopeURL.IsEmpty(), "Empty scope URL" );

    nsCOMPtr<nsIURI> scopeURI;
    rv = NS_NewURI( getter_AddRefs(scopeURI), scopeURL );
    NS_ENSURE_SUCCESS( rv, nsnull );

    nsCString host;
    rv = scopeURI->GetHost(host);
    NS_ENSURE_SUCCESS( rv, nsnull );

    rv = sbURIChecker::CheckDomain( host, siteScopeURI );
    if (NS_FAILED(rv)) {
      continue;
    }

    nsCString path;
    rv = scopeURI->GetPath(path);
    NS_ENSURE_SUCCESS( rv, nsnull );

    sbRemoteLibraryScopeURLSet* newSet =
      scopeURLSet.AppendElement( sbRemoteLibraryScopeURLSet( path, item ) );
    NS_ENSURE_TRUE( newSet, nsnull );
  }

  // Yay QuickSort!
  scopeURLSet.Sort();

  itemCount = scopeURLSet.Length();
  for (PRUint32 setIndex = itemCount - 1; setIndex >= 0; setIndex--) {
    const sbRemoteLibraryScopeURLSet& set = scopeURLSet.ElementAt(setIndex);

    nsCString path(set.scopePath);
    rv = sbURIChecker::CheckPath( path, siteScopeURI );
    if (NS_SUCCEEDED(rv)) {
      sbIMediaItem* retval = set.item;
      NS_ADDREF(retval);
      return retval;
    }
  }

  return nsnull;
}

