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

#include "sbRemotePlayer.h"
#include "sbRemoteLibrary.h"
#include <sbClassInfoUtils.h>
#include <sbILibrary.h>
#include <sbIMediaList.h>
#include <sbIMediaListView.h>
#include <sbIPlaylistCommands.h>
#include <sbITabBrowser.h>
#include <sbIPropertyManager.h>
#include <sbPropertiesCID.h>

#include <nsComponentManagerUtils.h>
#include <nsDOMJSUtils.h>
#include <nsIArray.h>
#include <nsICategoryManager.h>
#include <nsIContent.h>
#include <nsIDocShell.h>
#include <nsIDocShellTreeItem.h>
#include <nsIDocShellTreeOwner.h>
#include <nsIDocument.h>
#include <nsIDOMDocument.h>
#include <nsIDOMDocumentEvent.h>
#include <nsIDOMElement.h>
#include <nsIDOMEvent.h>
#include <nsIDOMEventTarget.h>
#include <nsIDOMWindow.h>
#include <nsPIDOMWindow.h>
#include <nsIDOMWindowInternal.h>
#include <nsIDOMXULDocument.h>
#include <nsIInterfaceRequestorUtils.h>
#include <nsIJSContextStack.h>
#include <nsIPermissionManager.h>
#include <nsIPrefBranch.h>
#include <nsIPresShell.h>
#include <nsIPrivateDOMEvent.h>
#include <nsIProgrammingLanguage.h>
#include <nsIScriptGlobalObject.h>
#include <nsIScriptNameSpaceManager.h>
#include <nsIScriptSecurityManager.h>
#include <nsITreeSelection.h>
#include <nsITreeView.h>
#include <nsIURI.h>
#include <nsIWindowMediator.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>
#include <prlog.h>

// added for download support. Some will stay after bug 3521 is fixed
#include <sbIDeviceBase.h>
#include <sbIDeviceManager.h>
#include <sbILibraryManager.h>
#include <nsIDialogParamBlock.h>
#include <nsISupportsPrimitives.h>
#include <nsIWindowWatcher.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemotePlayer:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gRemotePlayerLog = nsnull;
#define LOG(args)   if (gRemotePlayerLog) PR_LOG(gRemotePlayerLog, PR_LOG_WARN, args)
#else
#define LOG(args)   /* nothing */
#endif

static NS_DEFINE_CID(kRemotePlayerCID, SONGBIRD_REMOTEPLAYER_CID);

const static char* sPublicWProperties[] = {""};

const static char* sPublicRProperties[] =
  { "library:name",
    "library:playlists",
    "library:webPlaylist",
    "metadata:currentArtist",
    "metadata:currentAlbum",
    "metadata:currentTrack",
    "binding:commands",
    "classinfo:classDescription",
    "classinfo:contractID",
    "classinfo:classID",
    "classinfo:implementationLanguage",
    "classinfo:flags" };

const static char* sPublicMethods[] =
  { "controls:play",
    "controls:playMediaList",
    "controls:stop",
    "controls:pause",
    "controls:previous",
    "controls:next",
    "controls:playURL",
    "binding:downloadItem",
    "binding:downloadList",
    "binding:downloadSelected",
    "binding:registerCommands",
    "binding:unregisterCommands",
    "binding:siteLibrary",
    "binding:libraries",
    "binding:removeListener",
    "binding:addListener" };

const static char* sPublicMetadata[] =
  { "metadata.artist",
    "metadata.title",
    "metadata.album",
    "metadata.genre",
    "metadata.url",
    "metadata.position",
    "metadata.length",
    "metadata.position.str",
    "metadata.length.str",
    "playlist.shuffle",
    "playlist.repeat",
    "faceplate.playing",
    "faceplate.paused" };
// needs to be in nsEventDispatcher.cpp
#define RAPI_EVENT_CLASS      NS_LITERAL_STRING("Events")
#define RAPI_EVENT_TYPE       NS_LITERAL_STRING("remoteapi")
#define SB_PREFS_ROOT         NS_LITERAL_STRING("songbird.")
#define SB_EVENT_CMNDS_UP     NS_LITERAL_STRING("playlist-commands-updated")
#define SB_WEB_TABBROWSER_ID  NS_LITERAL_STRING("frame_main_pane")

// callback for destructor to clear out the observer hashtable
PR_STATIC_CALLBACK(PLDHashOperator)
UnbindAndRelease ( const nsAString &aKey,
                   sbRemoteObserver &aRemObs,
                   void *userArg )
{
  LOG(("UnbindAndRelease(%s, %x %x)", PromiseFlatString(aKey).get(), aRemObs.observer.get(), aRemObs.remote.get()));
  NS_ASSERTION(&aRemObs, "GAH! Hashtable contains a null entry");
  NS_ASSERTION(aRemObs.remote, "GAH! Container contains a null remote");
  NS_ASSERTION(aRemObs.observer, "GAH! Container contains a null observer");

  aRemObs.remote->Unbind();
  return PL_DHASH_REMOVE;
}

NS_IMPL_ISUPPORTS6( sbRemotePlayer,
                    nsIClassInfo,
                    nsISecurityCheckedComponent,
                    sbIRemotePlayer,
                    nsIDOMEventListener,
                    nsISupportsWeakReference,
                    sbISecurityAggregator )

NS_IMPL_CI_INTERFACE_GETTER5( sbRemotePlayer,
                              nsISecurityCheckedComponent,
                              sbIRemotePlayer,
                              nsIDOMEventListener,
                              nsISupportsWeakReference,
                              sbISecurityAggregator )

SB_IMPL_CLASSINFO( sbRemotePlayer,
                   SONGBIRD_REMOTEPLAYER_CONTRACTID,
                   SONGBIRD_REMOTEPLAYER_CLASSNAME,
                   nsIProgrammingLanguage::CPLUSPLUS,
                   0,
                   kRemotePlayerCID );

SB_IMPL_SECURITYCHECKEDCOMP_WITH_INIT (sbRemotePlayer);

sbRemotePlayer*
sbRemotePlayer::GetInstance()
{
#ifdef PR_LOGGING
  if (!gRemotePlayerLog) {
    gRemotePlayerLog = PR_NewLogModule("sbRemotePlayer");
  }
  LOG(("**** sbRemotePlayer::GetInstance() ****"));
#endif

  sbRemotePlayer *player = new sbRemotePlayer();
  if (!player)
    return nsnull;

  // initialize the player
  if (NS_FAILED(player->Init())) {
    return nsnull;
  }

  // addref it before handing it back
  NS_ADDREF(player);
  return player;
}

sbRemotePlayer::sbRemotePlayer() : mInitialized(PR_FALSE)
{
  LOG(("sbRemotePlayer::sbRemotePlayer()"));
}

sbRemotePlayer::~sbRemotePlayer()
{
  LOG(("sbRemotePlayer::~sbRemotePlayer()"));
  mRemObsHash.Enumerate(UnbindAndRelease, nsnull);
  mRemObsHash.Clear();
}

nsresult
sbRemotePlayer::Init()
{
  LOG(("sbRemotePlayer::Init()"));

  nsresult rv;
  mGPPS = do_GetService("@songbirdnest.com/Songbird/PlaylistPlayback;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool success = mRemObsHash.Init();
  NS_ASSERTION(success, "sbPropertyManager::mRemObsHash failed to initialize!");

  mCurrentTrack = do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1");
  mCurrentArtist = do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1");
  mCurrentAlbum = do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1");

  if (!mCurrentTrack || ! mCurrentArtist || ! mCurrentAlbum) {
    LOG(("sbRemotePlayer::Init() -- ERROR creating dataremotes"));
    return NS_ERROR_OUT_OF_MEMORY; 
  }

  mCurrentTrack->Init( NS_LITERAL_STRING("metadata.title"), SB_PREFS_ROOT );
  mCurrentArtist->Init( NS_LITERAL_STRING("metadata.artist"), SB_PREFS_ROOT );
  mCurrentAlbum->Init( NS_LITERAL_STRING("metadata.album"), SB_PREFS_ROOT );

  nsCOMPtr<sbISecurityMixin> mixin =
        do_CreateInstance("@songbirdnest.com/remoteapi/security-mixin;1", &rv);
  if (NS_FAILED(rv)) {
    LOG(("sbRemotePlayer::Init() -- ERROR creating mixin"));
    return NS_ERROR_OUT_OF_MEMORY; 
  }

  // Get the list of IIDs to pass to the security mixin
  nsIID ** iids;
  PRUint32 iidCount;
  GetInterfaces(&iidCount, &iids);

  // initialize our mixin with approved interfaces, methods, properties
  rv = mixin->Init( (sbISecurityAggregator*)this,
                    (const nsIID**)iids, iidCount,
                    sPublicMethods,NS_ARRAY_LENGTH(sPublicMethods),
                    sPublicRProperties,NS_ARRAY_LENGTH(sPublicRProperties),
                    sPublicWProperties, NS_ARRAY_LENGTH(sPublicWProperties) );
  NS_ENSURE_SUCCESS( rv, rv );

  mSecurityMixin = do_QueryInterface(mixin, &rv);
  NS_ENSURE_SUCCESS( rv, rv );

  //
  // Hook up an event listener to listen to the unload event so we can
  //   remove any commands from the PlaylistCommandsManager
  //

  // pull the dom window from the js stack and context
  nsCOMPtr<nsPIDOMWindow> privWindow = sbRemotePlayer::GetWindowFromJS();
  NS_ENSURE_STATE(privWindow);

  //
  // Get the Content Document
  //
  privWindow->GetDocument( getter_AddRefs(mContentDoc) );
  NS_ENSURE_STATE(mContentDoc);

  //
  // Get the Chrome Document
  //
  nsIDocShell *docShell = privWindow->GetDocShell();
  NS_ENSURE_STATE(docShell);

  // Step up to the chrome layer through the event handler
  nsCOMPtr<nsIDOMEventTarget> chromeEventHandler;
  docShell->GetChromeEventHandler( getter_AddRefs(chromeEventHandler) );
  NS_ENSURE_STATE(chromeEventHandler);
  nsCOMPtr<nsIContent> chromeElement(
                                do_QueryInterface( chromeEventHandler, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );

  // Get the document from the chrome content
  nsIDocument *doc = chromeElement->GetDocument();
  NS_ENSURE_STATE(doc);

  mChromeDoc = do_QueryInterface( doc, &rv );
  NS_ENSURE_SUCCESS( rv, rv );

  LOG(("sbRemotePlayer::Init() -- registering unload listener"));
  // Have our HandleEvent called on page unloads, so we can unhook commands
  nsCOMPtr<nsIDOMEventTarget> eventTarget( do_QueryInterface(mChromeDoc) );
  NS_ENSURE_STATE(eventTarget);
  eventTarget->AddEventListener( NS_LITERAL_STRING("unload"), this , PR_TRUE );

  mInitialized = PR_TRUE;

  return NS_OK;
}

// ---------------------------------------------------------------------------
//
//                        sbIRemotePlayer
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP 
sbRemotePlayer::GetName( nsAString &aName )
{
  LOG(("sbRemotePlayer::GetName()"));
  aName.AssignLiteral("Songbird");
  return NS_OK;
}

NS_IMETHODIMP
sbRemotePlayer::Libraries( const nsAString &aLibraryID,
                           sbIRemoteLibrary **aLibrary )
{
  LOG(( "sbRemotePlayer::Libraries(%s)",
        NS_LossyConvertUTF16toASCII(aLibraryID).get() ));
  nsresult rv;
  nsCOMPtr<sbIRemoteLibrary> library =
       do_CreateInstance( "@songbirdnest.com/remoteapi/remotelibrary;1", &rv );
  NS_ENSURE_SUCCESS( rv, rv );

  // Library is going to hash the ID passed in, if necessary
  rv = library->ConnectToDefaultLibrary( aLibraryID );
  if (NS_SUCCEEDED(rv)) {
    NS_ADDREF( *aLibrary = library );
  } else {
    *aLibrary = nsnull;
  }
  return NS_OK;
}

NS_IMETHODIMP
sbRemotePlayer::SiteLibrary( const nsAString &aDomain,
                             const nsAString &aPath,
                             sbIRemoteLibrary **aSiteLibrary )
{
  LOG(( "sbRemotePlayer::SiteLibrary(%s)",
        NS_LossyConvertUTF16toASCII(aPath).get()));

  nsresult rv;
  if (!mSiteLibrary) {
    mSiteLibrary =
       do_CreateInstance( "@songbirdnest.com/remoteapi/remotelibrary;1", &rv );
    NS_ENSURE_SUCCESS( rv, rv );
  }

  // Library is going to hash the ID passed in
  rv = mSiteLibrary->ConnectToMediaLibrary( aDomain, aPath );
  if (NS_SUCCEEDED(rv)) {
    NS_ADDREF( *aSiteLibrary = mSiteLibrary );
  } else {
    *aSiteLibrary = nsnull;
  }
  return NS_OK;
}

NS_IMETHODIMP
sbRemotePlayer::GetCommands( sbIRemoteCommands **aCommandsObject )
{
  NS_ENSURE_ARG_POINTER(aCommandsObject);

  LOG(("sbRemotePlayer::GetCommands()"));

  nsresult rv;
  if (!mCommandsObject) {
    LOG(("sbRemotePlayer::GetCommands() -- creating it"));
    mCommandsObject =
      do_CreateInstance( "@songbirdnest.com/remoteapi/remotecommands;1", &rv );
    NS_ENSURE_SUCCESS( rv, rv );
    mCommandsObject->SetOwner(this);
    RegisterCommands(PR_TRUE);
  }
  NS_ADDREF( *aCommandsObject = mCommandsObject );
  return NS_OK;
}

nsresult
sbRemotePlayer::RegisterCommands( PRBool aUseDefaultCommands )
{
  NS_ENSURE_STATE(mCommandsObject);
  nsresult rv;

  // store the default command usage
  mUseDefaultCommands = aUseDefaultCommands;

  // Get the PlaylistCommandsManager object and register commands with it.
  nsCOMPtr<sbIPlaylistCommandsManager> mgr( 
         do_GetService( "@songbirdnest.com/Songbird/PlaylistCommandsManager;1", &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );

  nsCOMPtr<sbIPlaylistCommands> commands( do_QueryInterface( mCommandsObject,
                                                             &rv ) );

  // XXXredfive - make this pull the GUID from the web playlist
  //              need sbIRemoteMediaLists online before we can do that.
  // Registration of commands is changing soon, for now type='library' works
  NS_ENSURE_SUCCESS( rv, rv );
  rv = mgr->RegisterPlaylistCommandsMediaItem( NS_LITERAL_STRING("remote-test-guid"),
                                               NS_LITERAL_STRING("library"),
                                               commands );
  NS_ASSERTION( NS_SUCCEEDED(rv),
                "Failed to register commands in playlistcommandsmanager" );
  rv = mgr->RegisterPlaylistCommandsMediaItem( NS_LITERAL_STRING("remote-test-guid"),
                                               NS_LITERAL_STRING("simple"),
                                               commands );
  NS_ASSERTION( NS_SUCCEEDED(rv),
                "Failed to register commands in playlistcommandsmanager" );

  OnCommandsChanged();

  return rv;
}

NS_IMETHODIMP
sbRemotePlayer::OnCommandsChanged()
{
  LOG(("sbRemotePlayer::OnCommandsChanged()"));
  if (!mWebPlaylistWidget) {
    nsresult rv = AcquirePlaylistWidget();
    NS_ENSURE_SUCCESS( rv, rv );
  }

  //
  // This is where we want to add code to register the default commands, when
  // the api for that comes in to being. Key off of mUseDefaultCommands.
  //

  // When the commands system is able to broadcast change notices about
  // registered commands this can go away. In the meantime we need to tell
  // the playlist to rescan so it picks up new/deleted commands.
  // Theoretically we could just fire an event here, but it wasn't getting
  // caught in the binding, need to look in to that more.
  mWebPlaylistWidget->RescanCommands();
  return NS_OK;
}

NS_IMETHODIMP 
sbRemotePlayer::GetWebPlaylist( sbIRemoteMediaList **aWebPlaylist )
{
  LOG(("sbRemotePlayer::GetWebPlaylist()"));
  NS_ENSURE_ARG_POINTER(aWebPlaylist);
  if (!mWebPlaylistWidget) {
    nsresult rv = AcquirePlaylistWidget();
    NS_ENSURE_SUCCESS( rv, rv );
  }

  // get the view from the web playlist widget
  nsresult rv;
  nsCOMPtr<sbIMediaListView> mediaListView;
  rv = mWebPlaylistWidget->GetView( getter_AddRefs(mediaListView) );
  if ( !mediaListView ) {
    *aWebPlaylist = nsnull;
    return rv;  // NS_OK?
  }

  // Set the view on the RemoteMediaList
  nsCOMPtr<sbIRemoteMediaList> remoteWebPlaylist;
  rv = SB_WrapMediaList( mediaListView, getter_AddRefs(remoteWebPlaylist) );
  NS_ENSURE_SUCCESS( rv, rv );
  LOG(("sbRemotePlayer::GetWebPlaylist() -- created wrapped playlist"));

  NS_ADDREF( *aWebPlaylist = remoteWebPlaylist ); 

  return NS_OK;
}

NS_IMETHODIMP 
sbRemotePlayer::SetWebPlaylist( sbIRemoteMediaList *aWebPlaylist )
{
  LOG(("sbRemotePlayer::SetWebPlaylist()"));
  NS_ENSURE_ARG_POINTER(aWebPlaylist);
  if (!mWebPlaylistWidget) {
    nsresult rv = AcquirePlaylistWidget();
    NS_ENSURE_SUCCESS( rv, rv );
  }

  // Get a view from the RemoteMediaList passed in
  nsCOMPtr<sbIMediaList> webMediaList = do_QueryInterface(aWebPlaylist);
  NS_ENSURE_TRUE( webMediaList, NS_ERROR_INVALID_ARG );

  nsresult rv;
  nsCOMPtr<sbIMediaListView> mediaListView;
  rv = aWebPlaylist->GetView( getter_AddRefs(mediaListView) );
  if (!mediaListView) {
    rv = webMediaList->CreateView( getter_AddRefs(mediaListView) );
  }
  NS_ENSURE_TRUE( mediaListView, NS_ERROR_FAILURE );

  // Bind the view to the playlist widget
  rv = mWebPlaylistWidget->Bind( mediaListView,
                                 nsnull,
                                 PR_FALSE,
                                 PR_FALSE );
  NS_ENSURE_SUCCESS( rv, rv );
  LOG(("sbRemotePlayer::SetWebPlaylist() -- successful bind"));

  // Update the commands because the widget has been reset and
  // needs to know the useDefaultCommands setting
  OnCommandsChanged();

  return NS_OK;
}

NS_IMETHODIMP 
sbRemotePlayer::DownloadItem( sbIMediaItem *aItem )
{
  LOG(("sbRemotePlayer::DownloadItem()"));

  NS_ENSURE_ARG_POINTER(aItem);

  nsCOMPtr<sbIMediaList> downloadList;
  nsresult rv = GetDownloadList( getter_AddRefs(downloadList) );
  NS_ENSURE_TRUE( downloadList, rv );

  downloadList->Add(aItem);
 
  return NS_OK;
}

NS_IMETHODIMP 
sbRemotePlayer::DownloadList( sbIRemoteMediaList *aList )
{
  LOG(("sbRemotePlayer::DownloadList()"));

  NS_ENSURE_ARG_POINTER(aList);

  nsCOMPtr<sbIMediaList> downloadList;
  nsresult rv = GetDownloadList( getter_AddRefs(downloadList) );
  NS_ENSURE_TRUE( downloadList, rv );

  nsCOMPtr<sbIMediaList> list (do_QueryInterface(aList, &rv));
  NS_ENSURE_SUCCESS( rv, rv );

  downloadList->AddAll(list);

  return NS_OK;
}

NS_IMETHODIMP 
sbRemotePlayer::DownloadSelected( sbIRemoteMediaList *aMediaList ) 
{
  LOG(("sbRemotePlayer::DownloadSelected()"));

  NS_ENSURE_ARG_POINTER(aMediaList);

  nsCOMPtr<sbIMediaList> downloadList;
  nsresult rv = GetDownloadList( getter_AddRefs(downloadList) );
  NS_ENSURE_TRUE( downloadList, rv );

  nsCOMPtr<nsISimpleEnumerator> selection;
  rv = aMediaList->GetSelection( getter_AddRefs(selection) );
  NS_ENSURE_SUCCESS( rv, rv );

  nsRefPtr<sbUnwrappingSimpleEnumerator> wrapper(
    new sbUnwrappingSimpleEnumerator(selection) );
  NS_ENSURE_TRUE(wrapper, NS_ERROR_OUT_OF_MEMORY);

  downloadList->AddSome(wrapper);

  return NS_OK;
}

// Moving to download device in bug 3521 - will remove in follow-up bug
nsresult
sbRemotePlayer::GetDownloadLocation( nsAString &aLocation, PRBool &aAlways )
{
  // check for a download folder location
  nsresult rv;
  nsCOMPtr<sbIDataRemote> dlFolderRemote =
    do_CreateInstance( "@songbirdnest.com/Songbird/DataRemote;1", &rv );
  NS_ENSURE_SUCCESS( rv, rv );
  rv = dlFolderRemote->Init( NS_LITERAL_STRING("download.folder"),
                             SB_PREFS_ROOT );
  dlFolderRemote->GetStringValue(aLocation);

  // check to see if we always download to the folder
  nsCOMPtr<sbIDataRemote> dlAlwaysRemote =
    do_CreateInstance( "@songbirdnest.com/Songbird/DataRemote;1", &rv );
  NS_ENSURE_SUCCESS( rv, rv );
  rv = dlAlwaysRemote->Init( NS_LITERAL_STRING("download.always"),
                             SB_PREFS_ROOT );
  dlAlwaysRemote->GetBoolValue(&aAlways);

  return rv;
}

// Moving to download device in bug 3521 - will remove in follow-up bug
PRBool
sbRemotePlayer::LaunchDownloadDialog( nsAString &aLocation )
{
  LOG(("sbRemotePlayer::LaunchDownloadDialog()"));

  // get a parameter block to get return value
  nsresult rv;
  nsCOMPtr<nsIDialogParamBlock> ioParamBlock(
    do_CreateInstance( "@mozilla.org/embedcomp/dialogparam;1", &rv ) );
  NS_ENSURE_SUCCESS( rv, PR_FALSE );

  // find out if we need to add the titlebar from the accessibility pref
  nsCOMPtr<sbIDataRemote> accEnabledRemote =
    do_CreateInstance( "@songbirdnest.com/Songbird/DataRemote;1", &rv );
  NS_ENSURE_SUCCESS( rv, PR_FALSE );
  rv = accEnabledRemote->Init( NS_LITERAL_STRING("accessibility.enabled"),
                               SB_PREFS_ROOT );
  PRBool wantTitlebar;
  accEnabledRemote->GetBoolValue(&wantTitlebar);

  // set the string for the chrome features
  nsCAutoString chromeFeatures = NS_LITERAL_CSTRING("centerscreen,chrome,modal");
  if (wantTitlebar)
    chromeFeatures += ",titlebar";

  nsCOMPtr<nsIDOMWindow> window;
  nsCOMPtr<nsIWindowWatcher> windowWatcher(
    do_GetService( NS_WINDOWWATCHER_CONTRACTID, &rv ) );
  NS_ENSURE_SUCCESS( rv, PR_FALSE );

  rv = windowWatcher->OpenWindow( nsnull,
                                  "chrome://songbird/content/xul/download.xul",
                                  nsnull,
                                  chromeFeatures.get(),
                                  ioParamBlock,
                                  getter_AddRefs(window) );
  NS_ENSURE_SUCCESS( rv, PR_FALSE );
  LOG(("sbRemotePlayer::LaunchDownloadDialog() -- returned from dialog "));

  PRInt32 buttonPressed = 0;
  nsString location;
  ioParamBlock->GetInt( 0, &buttonPressed );
  ioParamBlock->GetString( 0, getter_Copies(location) );
  aLocation = location;
  return ( buttonPressed ? PR_TRUE : PR_FALSE ); 
}

// Moving to download device in bug 3521 - will remove in follow-up bug
nsresult
sbRemotePlayer::GetDownloadList( sbIMediaList **aMediaList ) {
  LOG(("sbRemotePlayer::GetDownloadList()"));

  nsAutoString folder;
  PRBool always;
  nsresult rv = GetDownloadLocation( folder, always );

  PRBool dialogRetval = PR_TRUE;
  if ( folder.IsEmpty() || !always ) {
    dialogRetval = LaunchDownloadDialog( folder );
  }

  LOG(( "sbRemotePlayer::GetDownloadList() -- folder: %s",
        NS_LossyConvertUTF16toASCII(folder).get() ));

  // If the user cancelled things are OK, but no list
  if (!dialogRetval) {
    return NS_OK;
  }

  // go to prefs and get the GUID for the download list
  nsCOMPtr<nsIPrefBranch> prefService =
    do_GetService("@mozilla.org/preferences-service;1", &rv);
  NS_ENSURE_SUCCESS( rv, nsnull );

  nsCOMPtr<nsISupportsString> prefValue;
  rv = prefService->GetComplexValue( "songbird.library.download",
                                     NS_GET_IID(nsISupportsString),
                                     getter_AddRefs(prefValue) );
  NS_ENSURE_SUCCESS( rv, nsnull );

  nsString downloadListGUID;
  prefValue->ToString( getter_Copies(downloadListGUID) );
  LOG(( "sbRemotePlayer::GetDownloadList() -- GUID: %s",
        NS_LossyConvertUTF16toASCII(downloadListGUID).get() ));

  // get the library manager
  nsCOMPtr<sbILibraryManager> libraryManager(
    do_GetService( "@songbirdnest.com/Songbird/library/Manager;1", &rv ) );
  NS_ENSURE_SUCCESS( rv, nsnull );

  // downloads currently only go to the main library, get it
  nsCOMPtr<sbILibrary> mainLibrary;
  rv = libraryManager->GetMainLibrary(getter_AddRefs(mainLibrary));
  NS_ENSURE_SUCCESS( rv, nsnull );

  // get the download list
  nsCOMPtr<sbIMediaItem> item;
  rv = mainLibrary->GetMediaItem( downloadListGUID, getter_AddRefs(item) );
  NS_ENSURE_SUCCESS( rv, nsnull );

  // QI to medialist
  nsCOMPtr<sbIMediaList> downloadList (do_QueryInterface(item, &rv) );
  NS_ENSURE_SUCCESS( rv, nsnull );

  NS_IF_ADDREF( *aMediaList = downloadList );
  return NS_OK;
}

NS_IMETHODIMP 
sbRemotePlayer::GetCurrentArtist( nsAString &aCurrentArtist )
{
  LOG(("sbRemotePlayer::GetCurrentArtist()"));
  NS_ENSURE_STATE(mCurrentArtist);
  return mCurrentArtist->GetStringValue(aCurrentArtist);
}

NS_IMETHODIMP 
sbRemotePlayer::GetCurrentAlbum( nsAString &aCurrentAlbum )
{
  LOG(("sbRemotePlayer::GetCurrentAlbum()"));
  NS_ENSURE_STATE(mCurrentAlbum);
  return mCurrentAlbum->GetStringValue(aCurrentAlbum);
}

NS_IMETHODIMP 
sbRemotePlayer::GetCurrentTrack( nsAString &aCurrentTrack )
{
  LOG(("sbRemotePlayer::GetCurrentTrack()"));
  NS_ENSURE_STATE(mCurrentTrack);
  return mCurrentTrack->GetStringValue(aCurrentTrack);
}

NS_IMETHODIMP 
sbRemotePlayer::AddListener( const nsAString &aKey,
                             nsIObserver *aObserver )
{
  LOG(("sbRemotePlayer::AddListener()"));
  NS_ENSURE_ARG_POINTER(aObserver);

  // Make sure the key passed in is a valid one for attaching to
  PRInt32 length = NS_ARRAY_LENGTH(sPublicMetadata);
  for (PRInt32 index = 0 ; index < length; index++ ) {
    // if we find it break out of the loop and keep going
    if ( aKey.EqualsLiteral(sPublicMetadata[index]) )
      break;
    // if we get to this point it isn't accepted, shortcircuit
    if ( index == (length-1) )
      return NS_ERROR_FAILURE;
  }

  nsresult rv;
  nsCOMPtr<sbIDataRemote> dr =
           do_CreateInstance( "@songbirdnest.com/Songbird/DataRemote;1", &rv );
  NS_ENSURE_SUCCESS( rv, rv );
  rv = dr->Init( aKey, SB_PREFS_ROOT );
  NS_ENSURE_SUCCESS( rv, rv );
  rv = dr->BindObserver( aObserver, PR_FALSE );
  NS_ENSURE_SUCCESS( rv, rv );

  sbRemoteObserver remObs;
  remObs.observer = aObserver;
  remObs.remote = dr;
  PRBool success = mRemObsHash.Put( aKey, remObs );
  NS_ENSURE_TRUE( success, NS_ERROR_OUT_OF_MEMORY );

  return NS_OK;
}

NS_IMETHODIMP
sbRemotePlayer::RemoveListener( const nsAString &aKey,
                                nsIObserver *aObserver )
{
  NS_ENSURE_ARG_POINTER(aObserver);
  LOG(("sbRemotePlayer::RemoveListener(%s %x)",
       PromiseFlatString(aKey).get(), aObserver));

  sbRemoteObserver remObs;
  mRemObsHash.Get( aKey, &remObs );

  if ( remObs.observer == aObserver ) {
    LOG(( "sbRemotePlayer::RemoveListener(%s %x) -- found observer",
          NS_LossyConvertUTF16toASCII(aKey).get(), aObserver ));

    remObs.remote->Unbind();
    mRemObsHash.Remove(aKey);
  }
  else {
    LOG(( "sbRemotePlayer::RemoveListener(%s %x) -- did NOT find observer",
          PromiseFlatString(aKey).get(), aObserver ));
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbRemotePlayer::Play()
{
  LOG(("sbRemotePlayer::Play()"));
  NS_ENSURE_STATE(mGPPS);

  if (!mWebPlaylistWidget) {
    nsresult rv = AcquirePlaylistWidget();
    NS_ENSURE_SUCCESS( rv, rv );
  }

  nsCOMPtr<sbIMediaListView> mediaListView;
  nsresult rv = mWebPlaylistWidget->GetView( getter_AddRefs(mediaListView) );
  NS_ENSURE_SUCCESS( rv, rv );

  nsCOMPtr<nsITreeView> treeView;
  rv = mediaListView->GetTreeView( getter_AddRefs(treeView) );
  NS_ENSURE_SUCCESS( rv, rv );

  nsCOMPtr<nsITreeSelection> treeSelection;
  rv = treeView->GetSelection( getter_AddRefs(treeSelection) );
  NS_ENSURE_SUCCESS( rv, rv );

  PRInt32 index;
  treeSelection->GetCurrentIndex(&index);
  if ( index < 0 )
    index = 0;

  PRBool retval;
  mGPPS->PlayView( mediaListView, index, &retval );
  return retval ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
sbRemotePlayer::PlayMediaList( sbIRemoteMediaList *aList, PRInt32 aIndex )
{
  LOG(("sbRemotePlayer::PlayMediaList()"));
  NS_ENSURE_ARG_POINTER(aList);
  NS_ENSURE_STATE(mGPPS);

  nsresult rv;
  nsCOMPtr<sbIMediaList> list( do_QueryInterface( aList, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );
  
  nsCOMPtr<sbIMediaListView> mediaListView;
  rv = list->CreateView( getter_AddRefs(mediaListView) );
  NS_ENSURE_SUCCESS( rv, rv );

  if ( aIndex < 0 )
    aIndex = 0;

  PRBool retval;
  mGPPS->PlayView( mediaListView, aIndex, &retval );
  return retval ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
sbRemotePlayer::PlayURL( const nsAString &aURL )
{
  LOG(("sbRemotePlayer::PlayURL()"));
  NS_ENSURE_STATE(mGPPS);
  PRBool retval;
  mGPPS->PlayURL( aURL, &retval );
  return retval ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
sbRemotePlayer::Stop()
{
  LOG(("sbRemotePlayer::Stop()"));
  NS_ENSURE_STATE(mGPPS);
  PRBool retval;
  mGPPS->Stop(&retval);
  return retval ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
sbRemotePlayer::Pause()
{
  LOG(("sbRemotePlayer::Pause()"));
  NS_ENSURE_STATE(mGPPS);
  PRBool retval;
  mGPPS->Pause(&retval);
  return retval ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
sbRemotePlayer::Next()
{
  LOG(("sbRemotePlayer::Next()"));
  NS_ENSURE_STATE(mGPPS);
  PRInt32 retval;
  mGPPS->Next(&retval);
  return ( retval > -1 ) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
sbRemotePlayer::Previous()
{
  LOG(("sbRemotePlayer::Previous()"));
  NS_ENSURE_STATE(mGPPS);
  PRInt32 retval;
  mGPPS->Previous(&retval);
  return ( retval > -1 ) ? NS_OK : NS_ERROR_FAILURE;
}

nsresult
sbRemotePlayer::FireEventToContent( const nsAString &aClass,
                                    const nsAString &aType )
{
  LOG(( "sbRemotePlayer::FireEventToContent(%s, %s)",
        NS_LossyConvertUTF16toASCII(aClass).get(),
        NS_LossyConvertUTF16toASCII(aType).get() ));

  return sbRemotePlayer::DispatchEvent(mContentDoc, aClass, aType, PR_FALSE);
}

// ---------------------------------------------------------------------------
//
//                           nsIDOMEventListener
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbRemotePlayer::HandleEvent( nsIDOMEvent *aEvent )
{
  LOG(("sbRemotePlayer::HandleEvent()"));
  NS_ENSURE_ARG_POINTER(aEvent);
  nsAutoString type;
  aEvent->GetType(type);
  if ( type.EqualsLiteral("unload") ) {
    nsresult rv;
    nsCOMPtr<nsIDOMEventTarget> eventTarget( do_QueryInterface(mChromeDoc) );
    NS_ENSURE_STATE(eventTarget);
    rv = eventTarget->RemoveEventListener( NS_LITERAL_STRING("unload"),
                                           this ,
                                           PR_TRUE );
    NS_ASSERTION( NS_SUCCEEDED(rv),
                  "Failed to remove unload listener from document" );

    // the page is going away, clean up things that will cause us to
    // not get released.
    UnregisterCommands();
    mCommandsObject = nsnull;
    mContentDoc = nsnull;
    mChromeDoc = nsnull;
  }
  return NS_OK;
}

nsresult
sbRemotePlayer::UnregisterCommands()
{
  LOG(("sbRemotePlayer::UnregisterCommands()"));
  if (!mCommandsObject)
    return NS_OK;

  // Get the PlaylistCommandsManager object to unregister the commands
  nsresult rv;
  nsCOMPtr<sbIPlaylistCommandsManager> mgr( 
         do_GetService( "@songbirdnest.com/Songbird/PlaylistCommandsManager;1", &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );

  nsCOMPtr<sbIPlaylistCommands> commands(
                                   do_QueryInterface( mCommandsObject, &rv ) );
  // Registration of commands is changing soon, for now type='library' is it
  NS_ENSURE_SUCCESS( rv, rv );
  rv = mgr->UnregisterPlaylistCommandsMediaItem( NS_LITERAL_STRING("remote-test-guid"),
                                                 NS_LITERAL_STRING("library"),
                                                 commands );
  NS_ASSERTION( NS_SUCCEEDED(rv),
                "Failed to unregister commands from playlistcommandsmanager" );

  rv = mgr->UnregisterPlaylistCommandsMediaItem( NS_LITERAL_STRING("remote-test-guid"),
                                                 NS_LITERAL_STRING("simple"),
                                                 commands );
  NS_ASSERTION( NS_SUCCEEDED(rv),
                "Failed to unregister commands from playlistcommandsmanager" );
  
  return NS_OK;
}

// ---------------------------------------------------------------------------
//
//                        nsISecurityCheckedComponent
//
// ---------------------------------------------------------------------------

PRBool
sbRemotePlayer::ShouldNotifyUser( PRBool aPassedSecurityCheck )
{
  LOG(("sbRemotePlayer::ShouldNotifyUser()"));
  nsresult rv;
  nsCOMPtr<nsIPrefBranch> prefService =
    do_GetService("@mozilla.org/preferences-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  // build the pref key to check
  nsCAutoString prefKey("songbird.rapi.notify.");

  PRBool notify;
  if ( aPassedSecurityCheck ) {
    // check the pref for 'always notify me'
    prefKey += "always";
  } else {
    // check the pref for 'notify if denied'
    prefKey += "denied";
  }

  // get the pref value
  LOG(( "sbRemotePlayer::ShouldNotifyUser() - asking for pref: %s", prefKey.get() ));
  rv = prefService->GetBoolPref(prefKey.get(), &notify);
  NS_ENSURE_SUCCESS(rv, PR_TRUE);

  return notify;
}


// ---------------------------------------------------------------------------
//
//                             helpers
//
// ---------------------------------------------------------------------------

already_AddRefed<nsPIDOMWindow>
sbRemotePlayer::GetWindowFromJS() {
  LOG(("sbRemotePlayer::GetWindowFromJS()"));
  // Get the js context from the top of the stack. We may need to iterate
  // over these at some point in order to make sure we are sending events
  // to the correct window, but for now this works.
  nsCOMPtr<nsIJSContextStack> stack =
                           do_GetService("@mozilla.org/js/xpc/ContextStack;1");
 
  if (!stack) {
    LOG(("sbRemotePlayer::GetWindowFromJS() -- NO STACK!!!"));
    return nsnull;
  }
 
  JSContext *cx;
  if (NS_FAILED(stack->Peek(&cx)) || !cx) {
    LOG(("sbRemotePlayer::GetWindowFromJS() -- NO CONTEXT!!!"));
    return nsnull;
  }

  // Get the script context from the js context
  nsCOMPtr<nsIScriptContext> scCx = GetScriptContextFromJSContext(cx); 
  NS_ENSURE_TRUE(scCx, nsnull);

  // Get the DOMWindow from the script global via the script context
  nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface( scCx->GetGlobalObject() );
  NS_ENSURE_TRUE( win, nsnull );
  NS_ADDREF( win.get() );
  return win.get();
}

nsresult
sbRemotePlayer::FireRemoteAPIAccessedEvent( nsIDOMDocument *aContentDoc )
{
  LOG(("sbRemotePlayer::FireRemoteAPIAccessedEvent()"));

  return sbRemotePlayer::DispatchEvent( aContentDoc,
                        RAPI_EVENT_CLASS,
                        RAPI_EVENT_TYPE,
                        PR_TRUE );
}

nsresult
sbRemotePlayer::DispatchEvent( nsIDOMDocument *aDoc,
                               const nsAString &aClass,
                               const nsAString &aType,
                               PRBool aIsTrusted )
{
  LOG(( "sbRemotePlayer::DispatchEvent(%s, %s)",
        NS_LossyConvertUTF16toASCII(aClass).get(),
        NS_LossyConvertUTF16toASCII(aType).get() ));

  //change interfaces to create the event
  nsresult rv;
  nsCOMPtr<nsIDOMDocumentEvent> docEvent( do_QueryInterface( aDoc, &rv ) );
  NS_ENSURE_SUCCESS( rv , rv );

  //create the event
  nsCOMPtr<nsIDOMEvent> event;
  docEvent->CreateEvent( aClass, getter_AddRefs(event) );
  NS_ENSURE_STATE(event);
  rv = event->InitEvent( aType, PR_TRUE, PR_TRUE );
  NS_ENSURE_SUCCESS( rv , rv );

  //use the document for a target.
  nsCOMPtr<nsIDOMEventTarget> eventTarget( do_QueryInterface( aDoc, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );

  //make the event trusted
  nsCOMPtr<nsIPrivateDOMEvent> privEvt( do_QueryInterface( event, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );
  privEvt->SetTrusted(aIsTrusted);

  // Fire an event to the chrome system. This even will NOT get to content.
  PRBool dummy;
  return eventTarget->DispatchEvent( event, &dummy );
}

nsresult
sbRemotePlayer::AcquirePlaylistWidget()
{
  LOG(("sbRemotePlayer::AcquirePlaylistWidget()"));

  // These get set in initialization, so if there aren't set, bad news
  if (!mChromeDoc || !mContentDoc)
    return NS_ERROR_FAILURE;

  // Get the tabbrowser, ask it for the tab for our document
  nsCOMPtr<nsIDOMElement> tabBrowserElement;
  mChromeDoc->GetElementById( SB_WEB_TABBROWSER_ID,
                              getter_AddRefs(tabBrowserElement) );
  NS_ENSURE_STATE(tabBrowserElement);

  // Get our interface
  nsresult rv;
  nsCOMPtr<sbITabBrowser> tabbrowser( do_QueryInterface( tabBrowserElement,
                                                         &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );

  // Get the tab for our particular document
  nsCOMPtr<sbITabBrowserTab> browserTab;
  tabbrowser->GetTabForDocument( mContentDoc, getter_AddRefs(browserTab) );
  NS_ENSURE_STATE(browserTab);

  // Get the outer playlist from the tab, it's the web playlist
  nsCOMPtr<nsIDOMElement> playlist;
  browserTab->GetPlaylist( getter_AddRefs(playlist) );
  NS_ENSURE_STATE(playlist);

  // hold on to that for later as an sbIPlaylistWidget
  mWebPlaylistWidget = do_QueryInterface( playlist, &rv);
  NS_ENSURE_SUCCESS( rv, rv );

  return NS_OK;
}

// ---------------------------------------------------------------------------
//
//                             Component stuff
//
// ---------------------------------------------------------------------------

NS_METHOD
sbRemotePlayer::Register( nsIComponentManager* aCompMgr,
                          nsIFile* aPath,
                          const char *aLoaderStr,
                          const char *aType,
                          const nsModuleComponentInfo *aInfo )
{
  nsresult rv;

  nsCOMPtr<nsICategoryManager> catMan(
                                do_GetService(NS_CATEGORYMANAGER_CONTRACTID) );
  if (!catMan)
    return NS_ERROR_FAILURE;

  // allow ourself to be accessed as a js global in webpage
  rv = catMan->AddCategoryEntry( JAVASCRIPT_GLOBAL_PROPERTY_CATEGORY,
                                 "songbird",      /* use this name to access */
                                 SONGBIRD_REMOTEPLAYER_CONTRACTID,
                                 PR_TRUE,         /* persist */
                                 PR_TRUE,         /* replace existing */
                                 nsnull );

  return rv;
}


NS_METHOD
sbRemotePlayer::Unregister( nsIComponentManager* aCompMgr,
                            nsIFile* aPath,
                            const char *aLoaderStr,
                            const nsModuleComponentInfo *aInfo )
{
  nsresult rv;

  nsCOMPtr<nsICategoryManager> catMan(
                                do_GetService(NS_CATEGORYMANAGER_CONTRACTID) );
  if (!catMan)
    return NS_ERROR_FAILURE;

  rv = catMan->DeleteCategoryEntry( JAVASCRIPT_GLOBAL_PROPERTY_CATEGORY,
                                    "songbird",
                                    PR_TRUE );   /* delete persisted data */
  return rv;
}

