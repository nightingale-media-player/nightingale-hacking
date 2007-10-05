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

#include "sbRemoteAPIUtils.h"
#include "sbRemoteCommands.h"
#include "sbRemoteLibrary.h"
#include "sbRemoteLibraryBase.h"
#include "sbRemotePlaylistClickEvent.h"
#include "sbRemoteSiteLibrary.h"
#include "sbRemoteWebLibrary.h"
#include "sbRemoteWebPlaylist.h"
#include "sbSecurityMixin.h"
#include "sbURIChecker.h"
#include <sbClassInfoUtils.h>
#include <sbIDataRemote.h>
#include <sbIDownloadDevice.h>
#include <sbILibrary.h>
#include <sbIMediaList.h>
#include <sbIMediaListView.h>
#include <sbIRemoteAPIService.h>
#include <sbIPlaylistClickEvent.h>
#include <sbIPlaylistCommands.h>
#include <sbITabBrowser.h>
#include <sbIPropertyManager.h>
#include <sbPropertiesCID.h>
#include <sbIPropertyBuilder.h>

#include <nsAutoPtr.h>
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
#include <nsIDOMMouseEvent.h>
#include <nsIDOMNodeList.h>
#include <nsIDOMNSEvent.h>
#include <nsIDOMWindow.h>
#include <nsPIDOMWindow.h>
#include <nsIDOMWindowInternal.h>
#include <nsIDOMXULDocument.h>
#include <nsIDOMXULElement.h>
#include <nsIInterfaceRequestorUtils.h>
#include <nsIJSContextStack.h>
#include <nsIPrefBranch.h>
#include <nsIPresShell.h>
#include <nsIPrivateDOMEvent.h>
#include <nsIPromptService.h>
#include <nsIScriptGlobalObject.h>
#include <nsIScriptNameSpaceManager.h>
#include <nsIStringBundle.h>
#include <nsITreeSelection.h>
#include <nsITreeView.h>
#include <nsIURI.h>
#include <nsIWindowMediator.h>
#include <nsNetUtil.h>
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
#endif

#undef LOG
#define LOG(args) PR_LOG(gRemotePlayerLog, PR_LOG_WARN, args)

static NS_DEFINE_CID(kRemotePlayerCID, SONGBIRD_REMOTEPLAYER_CID);

const static char* sPublicWProperties[] =
  { "playback_control:position" };

const static char* sPublicRProperties[] =
  { "site:playing",
    "site:paused",
    "site:repeat",
    "site:shuffle",
    "site:position",
    "site:volume",
    "site:mute",
    "site:name",
    "library_read:playlists",
    "library_read:webLibrary",
    "library_read:mainLibrary",
    "site:siteLibrary",
    "site:webPlaylist",
    "site:downloadMediaList",
    "playback_read:currentArtist",
    "playback_read:currentAlbum",
    "playback_read:currentTrack",
    "site:commands",
    "classinfo:classDescription",
    "classinfo:contractID",
    "classinfo:classID",
    "classinfo:implementationLanguage",
    "classinfo:flags" };

const static char* sPublicMethods[] =
  { "playback_control:play",
    "playback_control:playMediaList",
    "playback_control:stop",
    "playback_control:pause",
    "playback_control:previous",
    "playback_control:next",
    "playback_control:playURL",
    "library_write:downloadItem",
    "library_write:downloadList",
    "library_write:downloadSelected",
    "site:setSiteScope",
    "library_read:libraries",
    "playback_read:removeListener",
    "playback_read:addListener",
    "site:createTextProperty",
    "site:createDateTimeProperty",
    "site:createURIProperty",
    "site:createNumberProperty",
    "site:createImageProperty",
    "site:createRatingsProperty",
    "site:createButtonProperty",
    "site:createDownloadButtonProperty",
    "site:hasAccess" };

// dataremotes keys that can be listened to
// when you change this, please update /documentation/ListenerTopics.txt thanks!
const static char* sPublicMetadata[] =
  { "metadata.artist",
    "metadata.title",
    "metadata.album",
    "metadata.genre",
    "metadata.position",
    "metadata.length",
    "metadata.position.str",
    "metadata.length.str",
    "playlist.shuffle",
    "playlist.repeat",
    "faceplate.volume",
    "faceplate.mute",
    "faceplate.playing",
    "faceplate.paused" };

// This is a lookup table for converting the human readable catagory names
// into our internal names (see sbSecurityMinin.cpp:sScopes).
// The first entry is the javascript text, and the second one is for internal,
// we add a : at the end of the internal one because that is what is compared.
const static char* sPublicCategoryConversions[][2] =
  { { "Control Playback", "playback_control:" },
    { "Read Current", "playback_read:" },
    { "Read Library", "library_read:" },
    { "Add Media", "library_write:" },
    { "Create Medialists", "library_create:" } };

// needs to be in nsEventDispatcher.cpp
#define RAPI_EVENT_CLASS      NS_LITERAL_STRING("Events")
#define RAPI_EVENT_TYPE       NS_LITERAL_STRING("remoteapi")
#define SB_PREFS_ROOT         NS_LITERAL_STRING("songbird.")
#define SB_EVENT_CMNDS_UP     NS_LITERAL_STRING("playlist-commands-updated")
#define SB_WEB_TABBROWSER     NS_LITERAL_STRING("sb-tabbrowser")

#define SB_EVENT_RAPI_PREF_PANEL_OPENED   NS_LITERAL_STRING("RemoteAPIPrefPanelOpened")
#define SB_EVENT_RAPI_PREF_CHANGED        NS_LITERAL_STRING("RemoteAPIPrefChanged")
#define SB_EVENT_RAPI_HAT_CLOSED          NS_LITERAL_STRING("RemoteAPIHatClosed")

#define SB_LIB_NAME_MAIN      "main"
#define SB_LIB_NAME_WEB       "web"

#define SB_DATAREMOTE_FACEPLATE_STATUS NS_LITERAL_STRING("faceplate.status.override.text")

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

// helper class to push a system pricipal so that all calls across xpconnect
// will succeed.  Similar to nsAutoLock, except for principals
class sbAutoPrincipalPusher
{
  public:
    sbAutoPrincipalPusher()
    {
      mStack = do_GetService("@mozilla.org/js/xpc/ContextStack;1");
      if (mStack) {
        nsresult rv = mStack->Push(nsnull);
        if (NS_FAILED(rv)) {
          // failed to push the stack, don't accidentally pop it later
          mStack = nsnull;
        }
      }
    }
    ~sbAutoPrincipalPusher()
    {
      if (mStack) {
        JSContext *cx;
        mStack->Pop(&cx);
      }
    }
    // tell people if we succeeded in pushing the principal
    operator PRBool() const
    {
      return mStack ? PR_TRUE : PR_FALSE ;
    }
  protected:
    nsCOMPtr<nsIJSContextStack> mStack;
};

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
                   kRemotePlayerCID )

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
  NS_ENSURE_TRUE( success, NS_ERROR_FAILURE );

  success = mCachedLibraries.Init(2);
  NS_ENSURE_TRUE( success, NS_ERROR_FAILURE );

  nsRefPtr<sbSecurityMixin> mixin = new sbSecurityMixin();
  NS_ENSURE_TRUE( mixin, NS_ERROR_OUT_OF_MEMORY );

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

  mSecurityMixin = do_QueryInterface(
                          NS_ISUPPORTS_CAST( sbISecurityMixin*, mixin ), &rv );
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
  // Set the content document on our mixin so that it knows where to send
  // notification events
  //
  rv = mixin->SetNotificationDocument( mContentDoc );
  NS_ENSURE_SUCCESS( rv, rv );

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

  LOG(("sbRemotePlayer::Init() -- registering PlaylistCellClick listener"));
  eventTarget->AddEventListener( NS_LITERAL_STRING("PlaylistCellClick"), this , PR_TRUE );

  LOG(("sbRemotePlayer::Init() -- registering RemoteAPIPrefPanelOpened listener"));
  eventTarget->AddEventListener( SB_EVENT_RAPI_PREF_PANEL_OPENED, this , PR_TRUE );

  LOG(("sbRemotePlayer::Init() -- registering RemoteAPIHatClosed listener"));
  eventTarget->AddEventListener( SB_EVENT_RAPI_HAT_CLOSED, this , PR_TRUE );

  LOG(("sbRemotePlayer::Init() -- registering RemoteAPIPrefChanged listener"));
  eventTarget->AddEventListener( SB_EVENT_RAPI_PREF_CHANGED, this , PR_TRUE );

  mNotificationMgr = new sbRemoteNotificationManager();
  NS_ENSURE_TRUE(mNotificationMgr, NS_ERROR_OUT_OF_MEMORY);

  rv = mNotificationMgr->Init();
  NS_ENSURE_SUCCESS(rv, rv);
  
  mScopeDomain.SetIsVoid(PR_TRUE);
  mScopePath.SetIsVoid(PR_TRUE);
  
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
sbRemotePlayer::GetMainLibrary( sbIRemoteLibrary **aMainLibrary )
{
  return Libraries( NS_LITERAL_STRING(SB_LIB_NAME_MAIN), aMainLibrary );
}

NS_IMETHODIMP
sbRemotePlayer::GetWebLibrary( sbIRemoteLibrary **aWebLibrary )
{
  return Libraries( NS_LITERAL_STRING(SB_LIB_NAME_WEB), aWebLibrary );
}

NS_IMETHODIMP
sbRemotePlayer::Libraries( const nsAString &aLibraryID,
                           sbIRemoteLibrary **aLibrary )
{
  NS_ENSURE_ARG_POINTER(aLibrary);
  LOG(( "sbRemotePlayer::Libraries(%s)",
        NS_LossyConvertUTF16toASCII(aLibraryID).get() ));
  nsresult rv;

  // Go ahead and return the cached copy if we have it. This Get call will take
  // care of AddRef'ing for us.
  if ( mCachedLibraries.Get( aLibraryID, aLibrary ) ) {
    return NS_OK;
  }

  // for a newly created library
  nsRefPtr<sbRemoteLibrary> library;

  // NOTE: if you add special library names here, document them in sbIRemotePlayer.idl
  if ( aLibraryID.EqualsLiteral(SB_LIB_NAME_MAIN) ) {
    // No cached main library create a new one.
    LOG(("sbRemotePlayer::Libraries() - creating main library"));
    library = new sbRemoteLibrary(this);
    NS_ENSURE_TRUE( library, NS_ERROR_OUT_OF_MEMORY );
  }
  else if ( aLibraryID.EqualsLiteral(SB_LIB_NAME_WEB) ) {
    // No cached web library create a new one.
    LOG(("sbRemotePlayer::Libraries() - creating web library"));
    library = new sbRemoteWebLibrary(this);
    NS_ENSURE_TRUE( library, NS_ERROR_OUT_OF_MEMORY );
  }
  else {
    // we don't handle anything but "main" and "web" yet.
    return NS_ERROR_INVALID_ARG;
  }

  rv = library->Init();
  NS_ENSURE_SUCCESS( rv, rv );

  rv = library->ConnectToDefaultLibrary( aLibraryID );
  NS_ENSURE_SUCCESS( rv, rv );

  nsCOMPtr<sbIRemoteLibrary> remoteLibrary =
    do_QueryInterface( NS_ISUPPORTS_CAST( sbIRemoteLibrary*, library ), &rv );
  NS_ENSURE_SUCCESS( rv, rv );

  // Now that initialization and library connection has succeeded cache
  // the created library in the hashtable.
  if ( !mCachedLibraries.Put( aLibraryID, remoteLibrary ) ) {
    NS_WARNING("Failed to cache remoteLibrary!");
  }

  NS_ADDREF( *aLibrary = remoteLibrary );
  return NS_OK;
}

NS_IMETHODIMP
sbRemotePlayer::SetSiteScope(const nsACString & aDomain, const nsACString & aPath)
{
  nsresult rv;
  
#if SB_DEBUG_RAPI
  NS_WARNING("sbRemotePlayer::SetSiteScope() - skipping multiple site library checks");
#else
  NS_ENSURE_TRUE( mScopeDomain.IsVoid(), NS_ERROR_ALREADY_INITIALIZED );
  NS_ENSURE_TRUE( mScopePath.IsVoid(), NS_ERROR_ALREADY_INITIALIZED );
#endif /* SB_DEBUG_RAPI */
  
  nsCString domain(aDomain);
  nsCString path(aPath);
  nsCOMPtr<nsIURI> codebaseURI;
  nsCOMPtr<sbISecurityMixin> mixin = do_QueryInterface( mSecurityMixin, &rv );
  NS_ENSURE_SUCCESS( rv, rv );
  rv = mixin->GetCodebase( getter_AddRefs(codebaseURI) );
  NS_ENSURE_SUCCESS( rv, rv );
  
  rv = sbURIChecker::CheckURI(domain, path, codebaseURI);
  NS_ENSURE_SUCCESS( rv, rv );
  
  mScopeDomain = domain;
  mScopePath = path;

  return NS_OK;
}

NS_IMETHODIMP
sbRemotePlayer::GetSiteLibrary(sbIRemoteLibrary * *aSiteLibrary)
{
  LOG(( "sbRemotePlayer::GetSiteLibrary(%s)", mScopePath.BeginReading() ));

  nsresult rv;
  
  // check if the site scope has beens set before; if not, set it implicitly
  if ( mScopeDomain.IsVoid() || mScopePath.IsVoid() ) {
    SetSiteScope( mScopeDomain, mScopePath );
  }

  nsString siteLibraryFilename;
  rv = sbRemoteSiteLibrary::GetFilenameForSiteLibrary( mScopeDomain,
                                                       mScopePath,
                                                       siteLibraryFilename );
  NS_ENSURE_SUCCESS( rv, rv );

  if ( mCachedLibraries.Get( siteLibraryFilename, aSiteLibrary ) ) {
    return NS_OK;
  }

  nsRefPtr<sbRemoteSiteLibrary> library;
  library = new sbRemoteSiteLibrary(this);
  NS_ENSURE_TRUE( library, NS_ERROR_OUT_OF_MEMORY );

  rv = library->Init();
  NS_ENSURE_SUCCESS( rv, rv );

  rv = library->ConnectToSiteLibrary( mScopeDomain, mScopePath );
  NS_ENSURE_SUCCESS( rv, rv );

  nsCOMPtr<sbIRemoteLibrary> remoteLibrary(
    do_QueryInterface( NS_ISUPPORTS_CAST( sbIRemoteSiteLibrary*, library ),
                       &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );

  PRBool success = mCachedLibraries.Put( siteLibraryFilename, remoteLibrary );
  NS_ENSURE_TRUE( success, NS_ERROR_FAILURE );

  NS_ADDREF( *aSiteLibrary = remoteLibrary );
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
    mCommandsObject = new sbRemoteCommands();
    NS_ENSURE_TRUE( mCommandsObject, NS_ERROR_OUT_OF_MEMORY );

    rv = mCommandsObject->Init();
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

  nsCOMPtr<sbIPlaylistCommands> commands = (sbIPlaylistCommands*) mCommandsObject;
  NS_ENSURE_TRUE( commands, NS_ERROR_UNEXPECTED );

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
sbRemotePlayer::GetWebPlaylist( sbIRemoteWebPlaylist **aWebPlaylist )
{
  LOG(("sbRemotePlayer::GetWebPlaylist()"));
  NS_ENSURE_ARG_POINTER(aWebPlaylist);
  nsresult rv;

  if (!mWebPlaylistWidget) {
    rv = AcquirePlaylistWidget();
    NS_ENSURE_SUCCESS( rv, rv );
  }

  nsCOMPtr<sbIRemoteWebPlaylist> remotePlaylist(
                                do_QueryInterface( mWebPlaylistWidget, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );

  NS_ADDREF( *aWebPlaylist = remotePlaylist );
  return NS_OK;
}

NS_IMETHODIMP
sbRemotePlayer::GetDownloadMediaList( sbIRemoteMediaList **aDownloadMediaList )
{
  LOG(("sbRemotePlayer::GetDownloadMediaList()"));
  NS_ENSURE_ARG_POINTER(aDownloadMediaList);

  nsresult rv;
  nsCOMPtr<sbIMediaList> downloadMediaList(
                                    do_QueryReferent(mWeakDownloadMediaList) );
  if (!downloadMediaList) {
    // get the main library
    nsCOMPtr<sbILibraryManager> libManager(
      do_GetService( "@songbirdnest.com/Songbird/library/Manager;1", &rv ) );
    NS_ENSURE_SUCCESS( rv, rv );

    nsCOMPtr<sbILibrary> mainLibrary;
    rv = libManager->GetMainLibrary( getter_AddRefs(mainLibrary) );
    NS_ENSURE_SUCCESS( rv, rv );

    // go to prefs and get the GUID for the download list
    nsCOMPtr<nsIPrefBranch> prefService =
      do_GetService("@mozilla.org/preferences-service;1", &rv);
    NS_ENSURE_SUCCESS( rv, nsnull );

    // I question the value of storing magic GUIDs in prefs,
    //    accessed by magic strings
    nsCOMPtr<nsISupportsString> prefValue;
    rv = prefService->GetComplexValue( "songbird.library.download",
                                       NS_GET_IID(nsISupportsString),
                                       getter_AddRefs(prefValue) );
    NS_ENSURE_SUCCESS( rv, nsnull );

    // convert the GUID into a string we can use
    nsString downloadListGUID;
    prefValue->ToString( getter_Copies(downloadListGUID) );
    LOG(( "sbRemotePlayer::GetDownloadMediaList() -- GUID: %s",
      NS_LossyConvertUTF16toASCII(downloadListGUID).get() ));

    // get the download list from the main library
    nsCOMPtr<sbIMediaItem> item;
    rv = mainLibrary->GetMediaItem( downloadListGUID,
                                    getter_AddRefs(item) );
    NS_ENSURE_SUCCESS( rv, rv ); 

    // put it in the comptr to be wrapped
    downloadMediaList = do_QueryInterface( item, &rv );
    NS_ENSURE_SUCCESS( rv, rv ); 

    // store it as a weak ref
    mWeakDownloadMediaList = do_GetWeakReference( downloadMediaList, &rv );
    NS_ENSURE_SUCCESS( rv, rv ); 
  }

  rv = SB_WrapMediaList( this, downloadMediaList, aDownloadMediaList );
  NS_ENSURE_SUCCESS( rv, rv ); 

  return NS_OK;
}

NS_IMETHODIMP
sbRemotePlayer::DownloadItem( sbIMediaItem *aItem )
{
  LOG(("sbRemotePlayer::DownloadItem()"));

  NS_ENSURE_ARG_POINTER(aItem);

  // Make sure the item is JUST an item, no lists or libraries allowed
  nsCOMPtr<sbIMediaList> listcheck (do_QueryInterface(aItem) );
  NS_ENSURE_FALSE( listcheck, NS_ERROR_INVALID_ARG );

  nsresult rv;
  nsCOMPtr<sbIDownloadDeviceHelper> dh =
    do_GetService("@songbirdnest.com/Songbird/DownloadDeviceHelper;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dh->DownloadItem(aItem);
  NS_ENSURE_SUCCESS(rv, rv);

  mNotificationMgr->Action(sbRemoteNotificationManager::eDownload, nsnull);

  return NS_OK;
}

NS_IMETHODIMP
sbRemotePlayer::DownloadList( sbIRemoteMediaList *aList )
{
  LOG(("sbRemotePlayer::DownloadList()"));

  NS_ENSURE_ARG_POINTER(aList);

  nsresult rv;
  nsCOMPtr<sbIMediaList> list (do_QueryInterface(aList, &rv));
  NS_ENSURE_SUCCESS( rv, rv );

  nsCOMPtr<sbIDownloadDeviceHelper> dh =
    do_GetService("@songbirdnest.com/Songbird/DownloadDeviceHelper;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dh->DownloadAll(list);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbRemotePlayer::DownloadSelected( sbIRemoteWebPlaylist *aWebPlaylist )
{
  LOG(("sbRemotePlayer::DownloadSelected()"));

  NS_ENSURE_ARG_POINTER(aWebPlaylist);

  nsCOMPtr<nsISimpleEnumerator> selection;
  nsresult rv = aWebPlaylist->GetSelection( getter_AddRefs(selection) );
  NS_ENSURE_SUCCESS( rv, rv );

  nsRefPtr<sbUnwrappingSimpleEnumerator> wrapper(
                                 new sbUnwrappingSimpleEnumerator(selection) );
  NS_ENSURE_TRUE( wrapper, NS_ERROR_OUT_OF_MEMORY );

  nsCOMPtr<sbIDownloadDeviceHelper> dh =
    do_GetService("@songbirdnest.com/Songbird/DownloadDeviceHelper;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dh->DownloadSome(wrapper);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbRemotePlayer::GetCurrentArtist( nsAString &aCurrentArtist )
{
  LOG(("sbRemotePlayer::GetCurrentArtist()"));
  if (!mdrCurrentArtist) {
    nsresult rv;
    mdrCurrentArtist =
           do_CreateInstance( "@songbirdnest.com/Songbird/DataRemote;1", &rv );
    NS_ENSURE_SUCCESS( rv, rv );
    rv = mdrCurrentArtist->Init( NS_LITERAL_STRING("metadata.artist"),
                                 SB_PREFS_ROOT );
    NS_ENSURE_SUCCESS( rv, rv );
  }
  return mdrCurrentArtist->GetStringValue(aCurrentArtist);
}

NS_IMETHODIMP
sbRemotePlayer::GetCurrentAlbum( nsAString &aCurrentAlbum )
{
  LOG(("sbRemotePlayer::GetCurrentAlbum()"));
  if (!mdrCurrentAlbum) {
    nsresult rv;
    mdrCurrentAlbum =
           do_CreateInstance( "@songbirdnest.com/Songbird/DataRemote;1", &rv );
    NS_ENSURE_SUCCESS( rv, rv );
    rv = mdrCurrentAlbum->Init( NS_LITERAL_STRING("metadata.album"),
                                SB_PREFS_ROOT );
    NS_ENSURE_SUCCESS( rv, rv );
  }
  return mdrCurrentAlbum->GetStringValue(aCurrentAlbum);
}

NS_IMETHODIMP
sbRemotePlayer::GetCurrentTrack( nsAString &aCurrentTrack )
{
  LOG(("sbRemotePlayer::GetCurrentTrack()"));
  if (!mdrCurrentTrack) {
    nsresult rv;
    mdrCurrentTrack =
           do_CreateInstance( "@songbirdnest.com/Songbird/DataRemote;1", &rv );
    NS_ENSURE_SUCCESS( rv, rv );
    rv = mdrCurrentTrack->Init( NS_LITERAL_STRING("metadata.title"),
                                SB_PREFS_ROOT );
    NS_ENSURE_SUCCESS( rv, rv );
  }
  return mdrCurrentTrack->GetStringValue(aCurrentTrack);
}

NS_IMETHODIMP
sbRemotePlayer::GetPlaying( PRBool *aPlaying )
{
  LOG(("sbRemotePlayer::GetPlaying()"));
  NS_ENSURE_ARG_POINTER(aPlaying);
  if (!mdrPlaying) {
    nsresult rv;
    mdrPlaying = do_CreateInstance( "@songbirdnest.com/Songbird/DataRemote;1",
                                    &rv );
    NS_ENSURE_SUCCESS( rv, rv );
    rv = mdrPlaying->Init( NS_LITERAL_STRING("faceplate.playing"),
                           SB_PREFS_ROOT );
    NS_ENSURE_SUCCESS( rv, rv );
  }
  return mdrPlaying->GetBoolValue(aPlaying);
}

NS_IMETHODIMP
sbRemotePlayer::GetPaused( PRBool *aPaused )
{
  LOG(("sbRemotePlayer::GetPaused()"));
  NS_ENSURE_ARG_POINTER(aPaused);
  if (!mdrPaused) {
    nsresult rv;
    mdrPaused = do_CreateInstance( "@songbirdnest.com/Songbird/DataRemote;1",
                                    &rv );
    NS_ENSURE_SUCCESS( rv, rv );
    rv = mdrPaused->Init( NS_LITERAL_STRING("faceplate.paused"),
                          SB_PREFS_ROOT );
    NS_ENSURE_SUCCESS( rv, rv );
  }
  return mdrPaused->GetBoolValue(aPaused);
}

NS_IMETHODIMP
sbRemotePlayer::GetRepeat( PRInt64 *aRepeat )
{
  LOG(("sbRemotePlayer::GetRepeat()"));
  NS_ENSURE_ARG_POINTER(aRepeat);
  if (!mdrRepeat) {
    nsresult rv;
    mdrRepeat = do_CreateInstance( "@songbirdnest.com/Songbird/DataRemote;1",
                                   &rv );
    NS_ENSURE_SUCCESS( rv, rv );
    rv = mdrRepeat->Init( NS_LITERAL_STRING("playlist.repeat"),
                          SB_PREFS_ROOT );
    NS_ENSURE_SUCCESS( rv, rv );
  }
  return mdrRepeat->GetIntValue(aRepeat);
}

NS_IMETHODIMP
sbRemotePlayer::GetShuffle( PRBool *aShuffle )
{
  LOG(("sbRemotePlayer::GetShuffle()"));
  NS_ENSURE_ARG_POINTER(aShuffle);
  if (!mdrShuffle) {
    nsresult rv;
    mdrShuffle = do_CreateInstance( "@songbirdnest.com/Songbird/DataRemote;1",
                                    &rv );
    NS_ENSURE_SUCCESS( rv, rv );
    rv = mdrShuffle->Init( NS_LITERAL_STRING("playlist.shuffle"),
                           SB_PREFS_ROOT );
    NS_ENSURE_SUCCESS( rv, rv );
  }
  return mdrShuffle->GetBoolValue(aShuffle);
}

NS_IMETHODIMP
sbRemotePlayer::GetPosition( PRInt64 *aPosition )
{
  LOG(("sbRemotePlayer::GetPosition()"));
  NS_ENSURE_ARG_POINTER(aPosition);
  if (!mdrPosition) {
    nsresult rv;
    mdrPosition = do_CreateInstance( "@songbirdnest.com/Songbird/DataRemote;1",
                                     &rv );
    NS_ENSURE_SUCCESS( rv, rv );
    rv = mdrPosition->Init( NS_LITERAL_STRING("metadata.position"),
                            SB_PREFS_ROOT );
    NS_ENSURE_SUCCESS( rv, rv );
  }
  return mdrPosition->GetIntValue(aPosition);
}

NS_IMETHODIMP
sbRemotePlayer::SetPosition( PRInt64 aPosition )
{
  LOG(("sbRemotePlayer::SetPosition()"));
  NS_ENSURE_ARG_POINTER(aPosition);

  nsresult rv = ConfirmPlaybackControl();
  NS_ENSURE_SUCCESS( rv, rv );

  if (!mdrPosition) {
    nsresult rv;
    mdrPosition = do_CreateInstance( "@songbirdnest.com/Songbird/DataRemote;1",
                                     &rv );
    NS_ENSURE_SUCCESS( rv, rv );
    rv = mdrPosition->Init( NS_LITERAL_STRING("metadata.position"),
                            SB_PREFS_ROOT );
    NS_ENSURE_SUCCESS( rv, rv );
  }
  rv = mdrPosition->SetIntValue(aPosition);
  NS_ENSURE_SUCCESS( rv, rv );
  
  return TakePlaybackControl( nsnull );
}

NS_IMETHODIMP
sbRemotePlayer::GetVolume( PRInt64 *aVolume )
{
  LOG(("sbRemotePlayer::GetVolume()"));
  NS_ENSURE_ARG_POINTER(aVolume);
  if (!mdrVolume) {
    nsresult rv;
    mdrVolume = do_CreateInstance( "@songbirdnest.com/Songbird/DataRemote;1",
                                   &rv );
    NS_ENSURE_SUCCESS( rv, rv );
    rv = mdrVolume->Init( NS_LITERAL_STRING("faceplate.volume"),
                          SB_PREFS_ROOT );
    NS_ENSURE_SUCCESS( rv, rv );
  }
  return mdrVolume->GetIntValue(aVolume);
}

NS_IMETHODIMP
sbRemotePlayer::GetMute( PRBool *aMute )
{
  LOG(("sbRemotePlayer::GetMute()"));
  NS_ENSURE_ARG_POINTER(aMute);
  if (!mdrMute) {
    nsresult rv;
    mdrMute = do_CreateInstance( "@songbirdnest.com/Songbird/DataRemote;1",
                                 &rv );
    NS_ENSURE_SUCCESS( rv, rv );
    rv = mdrMute->Init( NS_LITERAL_STRING("faceplate.mute"), SB_PREFS_ROOT );
    NS_ENSURE_SUCCESS( rv, rv );
  }
  return mdrMute->GetBoolValue(aMute);
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

  nsresult rv = ConfirmPlaybackControl();
  NS_ENSURE_SUCCESS( rv, rv );
  
  if (!mWebPlaylistWidget) {
    rv = AcquirePlaylistWidget();
    NS_ENSURE_SUCCESS( rv, rv );
  }

  nsCOMPtr<sbIMediaListView> mediaListView;
  rv = mWebPlaylistWidget->GetListView( getter_AddRefs(mediaListView) );
  NS_ENSURE_SUCCESS( rv, rv );

  // If the page does not have a web playlist, fall back
  // to the standard play button functionality.
  if (!mediaListView) {
    PRBool retval;
    mGPPS->Play( &retval );
    return retval ? NS_OK : NS_ERROR_FAILURE;
  }

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
  rv = mGPPS->PlayView( mediaListView, index, &retval );
  NS_ENSURE_SUCCESS( rv, rv );
  rv = TakePlaybackControl( nsnull );
  NS_ENSURE_SUCCESS( rv, rv );
  return retval ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
sbRemotePlayer::PlayMediaList( sbIRemoteMediaList *aList, PRInt32 aIndex )
{
  LOG(("sbRemotePlayer::PlayMediaList()"));
  NS_ENSURE_ARG_POINTER(aList);
  NS_ENSURE_STATE(mGPPS);

  nsresult rv = ConfirmPlaybackControl();
  NS_ENSURE_SUCCESS( rv, rv );

  // Get the existing view if there is one
  nsCOMPtr<sbIMediaListView> mediaListView;
  rv = aList->GetView( getter_AddRefs(mediaListView) );

  // There isn't a view so create one
  if (!mediaListView) {
    nsCOMPtr<sbIMediaList> list( do_QueryInterface( aList, &rv ) );
    NS_ENSURE_SUCCESS( rv, rv );

    rv = list->CreateView( nsnull, getter_AddRefs(mediaListView) );
    NS_ENSURE_SUCCESS( rv, rv );
  }

  if ( aIndex < 0 )
    aIndex = 0;

  PRBool retval;
  rv = mGPPS->PlayView( mediaListView, aIndex, &retval );
  NS_ENSURE_SUCCESS( rv, rv );
  rv = TakePlaybackControl( nsnull );
  NS_ENSURE_SUCCESS( rv, rv );
  return retval ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
sbRemotePlayer::PlayURL( const nsAString &aURL )
{
  LOG(("sbRemotePlayer::PlayURL()"));
  NS_ENSURE_STATE(mGPPS);

  nsresult rv = ConfirmPlaybackControl();
  NS_ENSURE_SUCCESS( rv, rv );

  PRBool retval;
  rv = mGPPS->PlayURL( aURL, &retval );
  NS_ENSURE_SUCCESS( rv, rv );
  rv = TakePlaybackControl( nsnull );
  NS_ENSURE_SUCCESS( rv, rv );
  return retval ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
sbRemotePlayer::Stop()
{
  LOG(("sbRemotePlayer::Stop()"));
  NS_ENSURE_STATE(mGPPS);

  nsresult rv = ConfirmPlaybackControl();
  NS_ENSURE_SUCCESS( rv, rv );

  PRBool retval;
  rv = mGPPS->Stop(&retval);
  NS_ENSURE_SUCCESS( rv, rv );
  rv = TakePlaybackControl( nsnull );
  NS_ENSURE_SUCCESS( rv, rv );
  return retval ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
sbRemotePlayer::Pause()
{
  LOG(("sbRemotePlayer::Pause()"));
  NS_ENSURE_STATE(mGPPS);

  nsresult rv = ConfirmPlaybackControl();
  NS_ENSURE_SUCCESS( rv, rv );

  PRBool retval;
  rv = mGPPS->Pause(&retval);
  NS_ENSURE_SUCCESS( rv, rv );

  rv = TakePlaybackControl( nsnull );
  NS_ENSURE_SUCCESS( rv, rv );

  return retval ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
sbRemotePlayer::Next()
{
  LOG(("sbRemotePlayer::Next()"));
  NS_ENSURE_STATE(mGPPS);

  nsresult rv = ConfirmPlaybackControl();
  NS_ENSURE_SUCCESS( rv, rv );

  PRInt32 retval;
  rv = mGPPS->Next(&retval);
  NS_ENSURE_SUCCESS( rv, rv );
  rv = TakePlaybackControl( nsnull );
  NS_ENSURE_SUCCESS( rv, rv );
  return ( retval > -1 ) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
sbRemotePlayer::Previous()
{
  LOG(("sbRemotePlayer::Previous()"));
  NS_ENSURE_STATE(mGPPS);

  nsresult rv = ConfirmPlaybackControl();
  NS_ENSURE_SUCCESS( rv, rv );

  PRInt32 retval;
  rv = mGPPS->Previous(&retval);
  NS_ENSURE_SUCCESS( rv, rv );
  rv = TakePlaybackControl( nsnull );
  NS_ENSURE_SUCCESS( rv, rv );
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
  LOG(( "sbRemotePlayer::HandleEvent() this: %08x", this ));
  NS_ENSURE_ARG_POINTER(aEvent);
  nsAutoString type;
  aEvent->GetType(type);
  if ( type.EqualsLiteral("unload") ) {
    LOG(("sbRemotePlayer::HandleEvent() - unload event"));
    nsresult rv;

    // check if this is the right document being unloaded (yay tabs)
    nsCOMPtr<nsIDOMNSEvent> nsEvent( do_QueryInterface( aEvent, &rv ) );
    if ( NS_FAILED(rv) )
      return NS_OK;

    nsCOMPtr<nsIDOMEventTarget> originalEventTarget;
    rv = nsEvent->GetOriginalTarget( getter_AddRefs(originalEventTarget) );
    NS_ENSURE_SUCCESS( rv, rv );

    if ( !SameCOMIdentity( originalEventTarget, mContentDoc ) ) {
      // not the content doc we're interested in
      return NS_OK;
    }

    nsCOMPtr<nsIDOMEventTarget> eventTarget( do_QueryInterface(mChromeDoc) );
    NS_ENSURE_STATE(eventTarget);
    rv = eventTarget->RemoveEventListener( NS_LITERAL_STRING("unload"),
                                           this ,
                                           PR_TRUE );
    NS_ASSERTION( NS_SUCCEEDED(rv),
                  "Failed to remove unload listener from document" );

    rv = eventTarget->RemoveEventListener( NS_LITERAL_STRING("PlaylistCellClick"),
                                           this ,
                                           PR_TRUE );
    NS_ASSERTION( NS_SUCCEEDED(rv),
                  "Failed to remove PlaylistCellClick listener from document" );
                
    rv = eventTarget->RemoveEventListener( SB_EVENT_RAPI_PREF_PANEL_OPENED,
                                           this ,
                                           PR_TRUE );
    NS_ASSERTION( NS_SUCCEEDED(rv),
                  "Failed to remove RemoteAPIPrefPanelOpened listener from document" );
                              
    rv = eventTarget->RemoveEventListener( SB_EVENT_RAPI_HAT_CLOSED,
                                           this ,
                                           PR_TRUE );
    NS_ASSERTION( NS_SUCCEEDED(rv),
                  "Failed to remove RemoteAPIHatClosed listener from document" );
                             
    rv = eventTarget->RemoveEventListener( SB_EVENT_RAPI_PREF_CHANGED,
                                           this ,
                                           PR_TRUE );
    NS_ASSERTION( NS_SUCCEEDED(rv),
                  "Failed to remove RemoteAPIPrefChanged listener from document" );                              
    // the page is going away, clean up things that will cause us to
    // not get released.
    UnregisterCommands();
    mCommandsObject = nsnull;
    mContentDoc = nsnull;
    mChromeDoc = nsnull;

    // Explicitly clear the cached libraries so we can break the reference
    // cycle between the library classes and this class
    mCachedLibraries.Clear();
  } else if ( type.EqualsLiteral("PlaylistCellClick") ) {
    LOG(("sbRemotePlayer::HandleEvent() - PlaylistCellClick event"));
    nsresult rv;

    // get the event information from the playlist
    nsCOMPtr<nsIDOMNSEvent> srcEvent( do_QueryInterface(aEvent, &rv) );
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMEventTarget> srcEventTarget;
    rv = srcEvent->GetOriginalTarget( getter_AddRefs(srcEventTarget) );
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIPlaylistWidget> playlist( do_QueryInterface(srcEventTarget, &rv) );
    // this recurses because we see the event targeted at the content document
    // bubble up, so just gracefully ignore it if the event wasn't from a playlist
    if (NS_FAILED(rv))
      return NS_OK;

    nsCOMPtr<sbIPlaylistClickEvent> playlistClickEvent;
    rv = playlist->GetLastClickEvent( getter_AddRefs(playlistClickEvent) );
    NS_ENSURE_SUCCESS(rv, rv);

    // make a mouse event
    nsCOMPtr<nsIDOMDocumentEvent> docEvent( do_QueryInterface(mContentDoc, &rv) );
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMEvent> newEvent;
    rv = docEvent->CreateEvent( NS_LITERAL_STRING("mouseevent"), getter_AddRefs(newEvent) );
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool ctrlKey = PR_FALSE, altKey = PR_FALSE,
           shiftKey = PR_FALSE, metaKey = PR_FALSE;
    PRUint16 button = 0;
    nsCOMPtr<nsIDOMMouseEvent> srcMouseEvent( do_QueryInterface(playlistClickEvent, &rv) );
    if (NS_SUCCEEDED(rv)) {
      srcMouseEvent->GetCtrlKey(&ctrlKey);
      srcMouseEvent->GetAltKey(&altKey);
      srcMouseEvent->GetShiftKey(&shiftKey);
      srcMouseEvent->GetMetaKey(&metaKey);
      srcMouseEvent->GetButton(&button);
    } else {
      LOG(("sbRemotePlayer::HandleEvent() - no mouse event for PlaylistCellClick"));
    }

    nsCOMPtr<nsIDOMMouseEvent> newMouseEvent( do_QueryInterface(newEvent, &rv) );
    NS_ENSURE_SUCCESS(rv, rv);
    rv = newMouseEvent->InitMouseEvent( NS_LITERAL_STRING("PlaylistCellClick"),
                                        PR_TRUE, /* can bubble */
                                        PR_TRUE, /* can cancel */
                                        nsnull,
                                        0, /* detail */
                                        0, 0, /* screen x & y */
                                        0, 0, /* client x & y */
                                        ctrlKey, /* ctrl key */
                                        altKey, /* alt key */
                                        shiftKey, /* shift key */
                                        metaKey, /* meta key */
                                        button, /* button */
                                        nsnull /* related target */
                                      );
    NS_ENSURE_SUCCESS(rv, rv);

    //make the event trusted
    nsCOMPtr<nsIPrivateDOMEvent> privEvt( do_QueryInterface( newEvent, &rv ) );
    NS_ENSURE_SUCCESS( rv, rv );
    privEvt->SetTrusted( PR_TRUE );

    // make our custom wrapper event
    nsRefPtr<sbRemotePlaylistClickEvent> remoteEvent( new sbRemotePlaylistClickEvent(this) );
    NS_ENSURE_TRUE(remoteEvent, NS_ERROR_OUT_OF_MEMORY);

    rv = remoteEvent->Init();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = remoteEvent->InitEvent( playlistClickEvent, newMouseEvent );
    NS_ENSURE_SUCCESS(rv, rv);

    // dispatch the event the the content document
    PRBool dummy;
    nsCOMPtr<nsIDOMEventTarget> destEventTarget = do_QueryInterface( mContentDoc, &rv );
    NS_ENSURE_SUCCESS(rv, rv);
    rv = destEventTarget->DispatchEvent( remoteEvent, &dummy );
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    // The 2 cases we want to pass the event on.
    //   1) The target was the chrome document, happens when the event comes
    //      from the preferences panel
    //   2) The target was a XUL Element, happens when the event comes from
    //      the notification box.

    // Get the original target
    nsresult rv;
    nsCOMPtr<nsIDOMNSEvent> nsEvent( do_QueryInterface( aEvent, &rv ) );
    if ( NS_FAILED(rv) )
      return NS_OK;
    
    nsCOMPtr<nsIDOMEventTarget> originalEventTarget;
    rv = nsEvent->GetOriginalTarget( getter_AddRefs(originalEventTarget) );
    NS_ENSURE_SUCCESS( rv, rv );

    PRBool allow = PR_FALSE;
    if ( SameCOMIdentity( originalEventTarget, mChromeDoc ) ) {
      LOG(( "sbRemotePlayer::HandleEvent() - target IS this chromeDoc - ALLOWING " ));
      allow = PR_TRUE;
    }

    nsCOMPtr<nsIDOMXULElement> domXULElement = do_QueryInterface(originalEventTarget, &rv);
    if (domXULElement) {
      // XXXredfive - when the rapi service comes online and we can map between
      //              a tab and the songbird object we will be able to look up
      //              to see if this notification box was ours. For now always
      //              fire an event.
      LOG(( "sbRemotePlayer::HandleEvent() - is a DOMXULElement - ALLOWING" ));
      allow = PR_TRUE;
    }

    if ( allow ) {
      if ( type.Equals(SB_EVENT_RAPI_PREF_PANEL_OPENED) ||
           type.Equals(SB_EVENT_RAPI_PREF_CHANGED) ||
           type.Equals(SB_EVENT_RAPI_HAT_CLOSED) ) {
        LOG(( "sbRemotePlayer::HandleEvent() - %s",
              NS_LossyConvertUTF16toASCII(type).get() ));
        return FireEventToContent( RAPI_EVENT_CLASS, type );
      } else {
        LOG(("sbRemotePlayer::HandleEvent() - Some other event"));
      }
    }
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

  nsCOMPtr<sbIPlaylistCommands> commands = (sbIPlaylistCommands*) mCommandsObject;
  NS_ENSURE_TRUE( commands, NS_ERROR_UNEXPECTED );
  // Registration of commands is changing soon, for now type='library' is it
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

nsresult
sbRemotePlayer::ConfirmPlaybackControl() {
  // the web site is trying to control the user's playback.
  // try to guess if this is likely to annoy the user.

  LOG(("sbRemotePlayer::ConfirmPlaybackControl()"));
  
  PRBool isPlaying;
  nsresult rv;
  
  // we are safe to run from untrusted script.  Since we may end up across
  // XPConnect and land in JS again, we need to push a principal so that when
  // XPConnect looks it sees that it's a chrome caller.
  
  // before we do that, however, we need to get any information we need from
  // the content document (since that will be unavailable when we push)
  nsCOMPtr<sbISecurityMixin> securityMixin =
    do_QueryInterface( mSecurityMixin, &rv );
  NS_ENSURE_SUCCESS( rv, rv );
  nsCOMPtr<nsIURI> codebaseURI;
  rv = securityMixin->GetCodebase( getter_AddRefs(codebaseURI) );
  NS_ENSURE_SUCCESS( rv, rv );

  // scope the principal push
  {
    // we can now push the new principal
    sbAutoPrincipalPusher principal;
    if (!principal) {
      return NS_ERROR_FAILURE;
    }

    rv = mGPPS->GetPlaying( &isPlaying );
    NS_ENSURE_SUCCESS( rv, rv );
  
    if (!isPlaying) {
      // The player is not playing, allow it
      LOG(("sbRemotePlayer::ConfirmPlaybackControl() -- not playing, allow"));
      return NS_OK;
    }
  
    // check to see if this page has control
    nsCOMPtr<sbIRemoteAPIService> remoteAPIService =
      do_GetService( "@songbirdnest.com/remoteapi/remoteapiservice;1", &rv );
    NS_ENSURE_SUCCESS( rv, rv );
    PRBool hasPlaybackControl;
    rv = remoteAPIService->HasPlaybackControl( codebaseURI, &hasPlaybackControl );
    NS_ENSURE_SUCCESS( rv, rv );
    if ( hasPlaybackControl ) {
      LOG(("sbRemotePlayer::ConfirmPlaybackControl() -- URI has control, allow"));
      return NS_OK;
    }
  } // pop the principal

  PRBool allowed =
    GetUserApprovalForHost( codebaseURI,
                            NS_LITERAL_STRING("rapi.playback_control.blocked.title"),
                            NS_LITERAL_STRING("rapi.playback_control.blocked.message") );

  return allowed ? NS_OK : NS_ERROR_ABORT ;
}

PRBool
sbRemotePlayer::GetUserApprovalForHost( nsIURI *aURI,
                                        const nsAString &aTitleKey,
                                        const nsAString &aMessageKey )
{
  LOG(("sbRemotePlayer::GetUserApprovalForHost(URI)"));
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ASSERTION(aTitleKey.IsEmpty(), "Title Key empty!!!!");
  NS_ASSERTION(aMessageKey.IsEmpty(), "Message key empty!!!!");

  nsCString hostUTF8;
  nsresult rv = aURI->GetHost(hostUTF8);
  NS_ENSURE_SUCCESS( rv, rv );

  if ( hostUTF8.IsEmpty() ) {
    // no host (e.g. file:///); fall back to full URL
    rv = aURI->GetSpec(hostUTF8);
    NS_ENSURE_SUCCESS( rv, rv );
  }
  NS_ConvertUTF8toUTF16 host(hostUTF8);

  nsCOMPtr<nsIStringBundleService> sbs =
    do_GetService( NS_STRINGBUNDLE_CONTRACTID, &rv );
  NS_ENSURE_SUCCESS( rv, PR_FALSE );
  
  // get the branding...
  nsCOMPtr<nsIStringBundle> bundle;
  rv = sbs->CreateBundle( "chrome://branding/locale/brand.properties",
                          getter_AddRefs(bundle) );
  NS_ENSURE_SUCCESS( rv, PR_FALSE );
  
  nsString branding;
  rv = bundle->GetStringFromName( NS_LITERAL_STRING("brandShortName").get(),
                                  getter_Copies(branding) );
  NS_ENSURE_SUCCESS( rv, PR_FALSE );
  
  // and the prompt title and message
  rv = sbs->CreateBundle( "chrome://songbird/locale/songbird.properties",
                          getter_AddRefs(bundle) );
  NS_ENSURE_SUCCESS( rv, PR_FALSE );
  
  const PRUnichar *formatParams[2];
  formatParams[0] = branding.BeginReading();
  formatParams[1] = host.BeginReading();

  nsString message;
  rv = bundle->FormatStringFromName( aMessageKey.BeginReading(),
                                     formatParams,
                                     NS_ARRAY_LENGTH(formatParams),
                                     getter_Copies(message) );
  NS_ENSURE_SUCCESS( rv, PR_FALSE );
  
  nsString title;
  rv = bundle->GetStringFromName( aTitleKey.BeginReading(),
                                  getter_Copies(title) );
  NS_ENSURE_SUCCESS( rv, PR_FALSE );
  
  // now we can actually go for the prompt
  nsCOMPtr<nsIPromptService> promptService =
    do_GetService( "@mozilla.org/embedcomp/prompt-service;1", &rv );
  NS_ENSURE_SUCCESS( rv, PR_FALSE );

  // this is launched by the child window, tell Gecko about it.
  nsCOMPtr<nsPIDOMWindow> jsWindow = GetWindowFromJS();
  nsCOMPtr<nsIDOMWindow> window = do_QueryInterface( jsWindow, &rv );
  NS_ENSURE_SUCCESS( rv, PR_FALSE );

  PRBool allowed;
  rv = promptService->Confirm( window,
                               title.BeginReading(),
                               message.BeginReading(),
                               &allowed );
  NS_ENSURE_SUCCESS( rv, PR_FALSE );

  return allowed;
}

nsresult
sbRemotePlayer::GetBrowser( nsIDOMElement** aElement )
{
  nsresult rv;
  // Find the sb-tabbrowser ByTagName()[0] (not ById()!)
  NS_ENSURE_STATE(mChromeDoc);
  nsCOMPtr<nsIDOMNodeList> tabBrowserElementList;
  mChromeDoc->GetElementsByTagName( SB_WEB_TABBROWSER,
                              getter_AddRefs(tabBrowserElementList) );
  NS_ENSURE_STATE(tabBrowserElementList);
  nsCOMPtr<nsIDOMNode> tabBrowserElement;
  rv = tabBrowserElementList->Item( 0, getter_AddRefs(tabBrowserElement) );
  NS_ENSURE_STATE(tabBrowserElement);
  NS_ENSURE_SUCCESS( rv, rv );

  // Get our interface
  nsCOMPtr<sbITabBrowser> tabbrowser( do_QueryInterface( tabBrowserElement,
                                                         &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );
  
  rv = tabbrowser->GetBrowserForDocument( mContentDoc, aElement );
  NS_ENSURE_SUCCESS( rv, rv );

  return NS_OK;
}

nsresult
sbRemotePlayer::TakePlaybackControl( nsIURI* aURI )
{
  nsresult rv;
  nsCOMPtr<nsIURI> uri = aURI;
  
  // try to guess the URI if we are not given one
  if ( NS_UNLIKELY(!uri) ) {
    nsCOMPtr<sbISecurityMixin> securityMixin =
      do_QueryInterface( mSecurityMixin, &rv );
    NS_ENSURE_SUCCESS( rv, rv );
    rv = securityMixin->GetCodebase( getter_AddRefs(uri) );
    NS_ENSURE_SUCCESS( rv, rv );
  }
  
  nsCOMPtr<sbIRemoteAPIService> remoteAPIService =
    do_GetService( "@songbirdnest.com/remoteapi/remoteapiservice;1", &rv );
  NS_ENSURE_SUCCESS( rv, rv );
  
  rv = remoteAPIService->TakePlaybackControl( uri, nsnull );
  NS_ENSURE_SUCCESS( rv, rv );
  return NS_OK;
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

  // Fire an event to the chrome system. This event will NOT get to content.
  PRBool dummy;
  return eventTarget->DispatchEvent( event, &dummy );
}

nsresult
sbRemotePlayer::AcquirePlaylistWidget()
{
  nsresult rv;
  LOG(("sbRemotePlayer::AcquirePlaylistWidget()"));

  // These get set in initialization, so if they aren't set, bad news
  if (!mChromeDoc || !mContentDoc)
    return NS_ERROR_FAILURE;

  // Find the sb-tabbrowser ByTagName()[0] (not ById()!)
  nsCOMPtr<nsIDOMNodeList> tabBrowserElementList;
  mChromeDoc->GetElementsByTagName( SB_WEB_TABBROWSER,
                              getter_AddRefs(tabBrowserElementList) );
  NS_ENSURE_STATE(tabBrowserElementList);
  nsCOMPtr<nsIDOMNode> tabBrowserElement;
  rv = tabBrowserElementList->Item( 0, getter_AddRefs(tabBrowserElement) );
  NS_ENSURE_STATE(tabBrowserElement);
  NS_ENSURE_SUCCESS( rv, rv );

  // Get our interface
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

  nsCOMPtr<sbIPlaylistWidget> playlistWidget ( do_QueryInterface( playlist,
                                                                  &rv) );
  NS_ENSURE_SUCCESS( rv, rv );

  // Create the RemotePlaylistWidget
  nsRefPtr<sbRemoteWebPlaylist> pWebPlaylist =
                         new sbRemoteWebPlaylist( this, playlistWidget, browserTab );
  NS_ENSURE_TRUE( pWebPlaylist, NS_ERROR_FAILURE );

  rv = pWebPlaylist->Init();
  NS_ENSURE_SUCCESS( rv, rv );

  mWebPlaylistWidget = pWebPlaylist;
  NS_ENSURE_TRUE( mWebPlaylistWidget, NS_ERROR_FAILURE );

  return NS_OK;
}

sbRemoteNotificationManager*
sbRemotePlayer::GetNotificationManager()
{
  return mNotificationMgr;
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

nsresult
sbRemotePlayer::CreateProperty( const nsAString& aPropertyType,
                                const nsAString& aPropertyID,
                                const nsAString& aDisplayName,
                                const nsAString& aButtonLabel,
                                PRBool aUserViewable,
                                PRBool aUserEditable,
                                PRUint32 aNullSort )
{
  LOG(( "sbRemotePlayer::CreateProperty(%s, %s, %s, %s)",
        NS_LossyConvertUTF16toASCII(aPropertyType).get(),
        NS_LossyConvertUTF16toASCII(aPropertyID).get(),
        NS_LossyConvertUTF16toASCII(aDisplayName).get(),
        NS_LossyConvertUTF16toASCII(aButtonLabel).get() ));

  nsresult rv;
  nsCOMPtr<sbIPropertyManager> propMngr( 
                         do_GetService( SB_PROPERTYMANAGER_CONTRACTID, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );

  PRBool hasProp;
  nsCOMPtr<sbIPropertyInfo> info;
  propMngr->HasProperty( aPropertyType, &hasProp );

  if (!hasProp) {
    // property doesn't exist, add it.

    // these types get created directly
    if (aPropertyType.EqualsLiteral("text") ||
        aPropertyType.EqualsLiteral("datetime") ||
        aPropertyType.EqualsLiteral("uri") ||
        aPropertyType.EqualsLiteral("number")) {
      if ( aPropertyType.EqualsLiteral("text") ){
        info = do_CreateInstance( SB_TEXTPROPERTYINFO_CONTRACTID, &rv );
      } else if ( aPropertyType.EqualsLiteral("datetime") ) {
        info = do_CreateInstance( SB_DATETIMEPROPERTYINFO_CONTRACTID, &rv );
      } else if ( aPropertyType.EqualsLiteral("uri") ) {
        info = do_CreateInstance( SB_URIPROPERTYINFO_CONTRACTID, &rv );
      } else if ( aPropertyType.EqualsLiteral("number") ) {
        info = do_CreateInstance( SB_NUMBERPROPERTYINFO_CONTRACTID, &rv );
      }
      NS_ENSURE_SUCCESS( rv, rv );

      if (info) {
        // set the data on the propinfo
        rv = info->SetName(aPropertyType);
        NS_ENSURE_SUCCESS( rv, rv );
        rv = info->SetDisplayName(aDisplayName);
        NS_ENSURE_SUCCESS( rv, rv );
        rv = info->SetUserViewable(aUserViewable);
        NS_ENSURE_SUCCESS( rv, rv );
        rv = info->SetUserEditable(aUserEditable);
        NS_ENSURE_SUCCESS( rv, rv );
        rv = info->SetNullSort(aNullSort);
        NS_ENSURE_SUCCESS( rv, rv );
      }
    }
    // these types get created through builders
    else {
      nsCOMPtr<sbIPropertyBuilder> builder;
      if (aPropertyType.EqualsLiteral("button")) {
        LOG(( "sbRemotePlayer::CreateProperty() - using a button builder" ));
        builder =
          do_CreateInstance(SB_SIMPLEBUTTONPROPERTYBUILDER_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<sbISimpleButtonPropertyBuilder> buttonBuilder (
          do_QueryInterface(builder));
        NS_ENSURE_STATE(buttonBuilder);

        rv = buttonBuilder->SetLabel(aButtonLabel);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else if (aPropertyType.EqualsLiteral("image")) {
        LOG(( "sbRemotePlayer::CreateProperty() - using a image builder" ));
        builder =
          do_CreateInstance(SB_IMAGEPROPERTYBUILDER_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else if (aPropertyType.EqualsLiteral("downloadbutton")) {
        LOG(( "sbRemotePlayer::CreateProperty() - using a downloadbutton builder" ));
        builder =
          do_CreateInstance(SB_DOWNLOADBUTTONPROPERTYBUILDER_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<sbIDownloadButtonPropertyBuilder> dlBuilder (
          do_QueryInterface(builder));
        NS_ENSURE_STATE(dlBuilder);

        rv = dlBuilder->SetLabel(aButtonLabel);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else if (aPropertyType.EqualsLiteral("rating")) {
        LOG(( "sbRemotePlayer::CreateProperty() - using a rating builder" ));
        builder =
          do_CreateInstance(SB_RATINGPROPERTYBUILDER_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        // bad type, return an error - Should we just create text by default?
        return NS_ERROR_FAILURE;
      }

      // set all the common properties together
      if (builder) {
        rv = builder->SetPropertyName(aPropertyType);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = builder->SetDisplayName(aDisplayName);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = builder->SetRemoteReadable(PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = builder->SetRemoteWritable(PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = builder->Get(getter_AddRefs(info));
        NS_ENSURE_SUCCESS(rv, rv);
      }

    }

    // we created a new property in the system so enable remote write/read
    if (NS_SUCCEEDED(rv) ) {
      LOG(( "sbRemotePlayer::CreateProperty() - we have a propertyInfo item" ));
      rv = info->SetRemoteWritable(PR_TRUE);
      if ( NS_FAILED(rv) && rv != NS_ERROR_NOT_IMPLEMENTED ) {
        return rv;
      }

      rv = info->SetRemoteReadable(PR_TRUE);
      if ( NS_FAILED(rv) && rv != NS_ERROR_NOT_IMPLEMENTED ) {
        return rv;
      }
    } else {
      LOG(( "sbRemotePlayer::CreateProperty() - we DONT have a propertyInfo item" ));
    }

    // add to the property manager
    rv = propMngr->AddPropertyInfo(info);
    NS_ENSURE_SUCCESS( rv, rv );

  }

  return NS_OK;
}

NS_IMETHODIMP
sbRemotePlayer::CreateTextProperty( const nsAString& aPropertyID,
                                    const nsAString& aDisplayName,
                                    PRBool aUserViewable,
                                    PRBool aUserEditable,
                                    PRUint32 aNullSort )
{
  return CreateProperty( NS_LITERAL_STRING("text"), aPropertyID, aDisplayName,
                         NS_LITERAL_STRING(""), aUserViewable,
                         aUserEditable, aNullSort );
}

NS_IMETHODIMP
sbRemotePlayer::CreateDateTimeProperty( const nsAString& aPropertyID,
                                        const nsAString& aDisplayName,
                                        PRBool aUserViewable,
                                        PRBool aUserEditable,
                                        PRUint32 aNullSort )
{
  return CreateProperty( NS_LITERAL_STRING("datetime"), aPropertyID, aDisplayName,
                         NS_LITERAL_STRING(""), aUserViewable,
                         aUserEditable, aNullSort );
}

NS_IMETHODIMP
sbRemotePlayer::CreateURIProperty( const nsAString& aPropertyID,
                                   const nsAString& aDisplayName,
                                   PRBool aUserViewable,
                                   PRBool aUserEditable,
                                   PRUint32 aNullSort)
{
  return CreateProperty( NS_LITERAL_STRING("uri"), aPropertyID, aDisplayName,
                         NS_LITERAL_STRING(""), aUserViewable,
                         aUserEditable, aNullSort );
}

NS_IMETHODIMP
sbRemotePlayer::CreateNumberProperty( const nsAString& aPropertyID,
                                      const nsAString& aDisplayName,
                                      PRBool aUserViewable,
                                      PRBool aUserEditable,
                                      PRUint32 aNullSort )
{
  return CreateProperty( NS_LITERAL_STRING("number"), aPropertyID, aDisplayName,
                         NS_LITERAL_STRING(""), aUserViewable,
                         aUserEditable, aNullSort );
}

NS_IMETHODIMP
sbRemotePlayer::CreateImageProperty( const nsAString& aPropertyID,
                                     const nsAString& aDisplayName,
                                     PRBool aUserViewable,
                                     PRBool aUserEditable,
                                     PRUint32 aNullSort )
{
  return CreateProperty( NS_LITERAL_STRING("image"), aPropertyID, aDisplayName,
                         NS_LITERAL_STRING(""), aUserViewable,
                         aUserEditable, aNullSort );
}

NS_IMETHODIMP
sbRemotePlayer::CreateRatingsProperty( const nsAString& aPropertyID,
                                       const nsAString& aDisplayName,
                                       PRBool aUserViewable,
                                       PRBool aUserEditable,
                                       PRUint32 aNullSort )
{
  return CreateProperty( NS_LITERAL_STRING("rating"), aPropertyID, aDisplayName,
                         NS_LITERAL_STRING(""), aUserViewable,
                         aUserEditable, aNullSort );
}

NS_IMETHODIMP
sbRemotePlayer::CreateButtonProperty( const nsAString& aPropertyID,
                                      const nsAString& aDisplayName,
                                      const nsAString& aButtonLabel,
                                      PRBool aUserViewable,
                                      PRBool aUserEditable,
                                      PRUint32 aNullSort )
{
  return CreateProperty( NS_LITERAL_STRING("button"), aPropertyID, aDisplayName,
                         aButtonLabel, aUserViewable,
                         aUserEditable, aNullSort );
}

NS_IMETHODIMP
sbRemotePlayer::CreateDownloadButtonProperty( const nsAString& aPropertyID,
                                              const nsAString& aDisplayName,
                                              const nsAString& aButtonLabel,
                                              PRBool aUserViewable,
                                              PRBool aUserEditable,
                                              PRUint32 aNullSort )
{
  return CreateProperty( NS_LITERAL_STRING("downloadbutton"), aPropertyID,
                         aDisplayName, aButtonLabel, aUserViewable,
                         aUserEditable, aNullSort );
}

NS_IMETHODIMP
sbRemotePlayer::HasAccess( const nsAString& aRemotePermCategory,
                           PRBool *_retval )
{
  nsCOMPtr<sbISecurityMixin> mixin;
  mixin = do_QueryInterface(mSecurityMixin);

  // Here we are going to look up the aRemotePermCategory in our
  // sPublicCategoryConversions table to convert the more readable version
  // into our internal version.
  PRInt32 iIndex = -1;
  for ( PRUint32 i=0; iIndex < 0 && i < NS_ARRAY_LENGTH(sPublicCategoryConversions); i++ ) {
    if ( StringBeginsWith( aRemotePermCategory, NS_ConvertASCIItoUTF16( sPublicCategoryConversions[i][0] ) ) ) {
      // Set iIndex which will drop us out of the loop
      iIndex = i;
    }
  }

  // Check that we found a matching catagory, and return false if not.
  if (iIndex == -1)
  {
    *_retval = false;
    return NS_OK;
  }
  
  // Now grab the internal category name and get the permissions
  nsString mCategory;
  mCategory.AssignLiteral(sPublicCategoryConversions[iIndex][1]);
  return mixin->GetPermissionForScopedNameWrapper( mCategory, _retval );
}
