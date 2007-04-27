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

#include <nsComponentManagerUtils.h>
#include <nsICategoryManager.h>
#include <nsIClassInfoImpl.h>
#include <nsIPermissionManager.h>
#include <nsIProgrammingLanguage.h>
#include <nsIScriptNameSpaceManager.h>
#include <nsIScriptSecurityManager.h>
#include <nsIURI.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>
#include <prlog.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemotePlayer:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gLibraryLog = nsnull;
#define LOG(args)   if (gLibraryLog) PR_LOG(gLibraryLog, PR_LOG_WARN, args)
#else
#define LOG(args)   /* nothing */
#endif

static NS_DEFINE_CID(kRemotePlayerCID, SONGBIRD_REMOTEPLAYER_CID);

const static PRUint32 sPublicWPropertiesLength = 0;
const static char* sPublicWProperties[] = {""};

const static PRUint32 sPublicRPropertiesLength = 10;
const static char* sPublicRProperties[] =
  { "metadata:name",
    "library:playlists",
    "library:webPlaylist",
    "metadata:currentArtist",
    "metadata:currentAlbum",
    "metadata:currentTrack",
    "classinfo:classDescription",
    "classinfo:contractID",
    "classinfo:classID",
    "classinfo:implementationLanguage",
    "classinfo:flags" };

const static PRUint32 sPublicMethodsLength = 8;
const static char* sPublicMethods[] =
  { "controls:play",
    "controls:stop",
    "controls:pause",
    "controls:previous",
    "controls:next",
    "controls:playURL",
    "binding:removeListener",
    "binding:addListener" };

const static PRUint32 sPublicMetadataLength = 13;
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

// Manage our singletoness
static sbRemotePlayer *sRemotePlayer = nsnull;

NS_IMPL_ISUPPORTS4(sbRemotePlayer,
                   nsIClassInfo,
                   nsISecurityCheckedComponent,
                   sbISecurityAggregator,
                   sbIRemotePlayer)

sbRemotePlayer*
sbRemotePlayer::GetInstance()
{
#ifdef PR_LOGGING
  if (!gLibraryLog) {
    gLibraryLog = PR_NewLogModule("sbRemotePlayer");
  }
  LOG(("sbRemotePlayer::GetInstance()\n"));
#endif
  if (!sRemotePlayer) {
    sRemotePlayer = new sbRemotePlayer();
    if (!sRemotePlayer)
      return nsnull;
    NS_ADDREF(sRemotePlayer);  // addref the static global
  }
  NS_ADDREF(sRemotePlayer);    // addref the return result
  return sRemotePlayer;
}

sbRemotePlayer::sbRemotePlayer() : mInitialized(0)
{
}

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

  mCurrentTrack->Init( NS_LITERAL_STRING("metadata.title"), NS_LITERAL_STRING("songbird.") );
  mCurrentArtist->Init( NS_LITERAL_STRING("metadata.artist"), NS_LITERAL_STRING("songbird.") );
  mCurrentAlbum->Init( NS_LITERAL_STRING("metadata.album"), NS_LITERAL_STRING("songbird.") );

  nsCOMPtr<sbISecurityMixin> mixin = do_CreateInstance("@songbirdnest.com/remoteapi/security-mixin;1", &rv);
  if (NS_FAILED(rv)) {
    LOG(("sbRemotePlayer::Init() -- ERROR creating mixin"));
    return NS_ERROR_OUT_OF_MEMORY; 
  }

  // Get the list of IIDs to pass to the security mixin
  nsIID ** iids;
  PRUint32 iidCount;
  GetInterfaces(&iidCount, &iids);

  // initialize our mixin with approved interfaces, methods, properties
  mixin->Init( (sbISecurityAggregator*)this,
               (const nsIID**)iids, iidCount,
               sPublicMethods,sPublicMethodsLength,
               sPublicRProperties,sPublicRPropertiesLength,
               sPublicWProperties, sPublicWPropertiesLength );

  mSecurityMixin = do_QueryInterface(mixin, &rv);
  if (NS_FAILED(rv) || !mSecurityMixin) {
    LOG(("sbRemotePlayer::Init() -- ERROR, no mSecurityMixin"));
    return NS_ERROR_FAILURE;
  }

  mInitialized = PR_TRUE;

  return NS_OK;
}

// ---------------------------------------------------------------------------
//
//                        sbIRemotePlayer
//
// ---------------------------------------------------------------------------
NS_IMETHODIMP 
sbRemotePlayer::GetName(nsAString &aName)
{
  LOG(("sbRemotePlayer::GetName()"));
  aName.AssignLiteral("Songbird");
  return NS_OK;
}

// XXXredfive
NS_IMETHODIMP 
sbRemotePlayer::GetPlaylists(nsIArray **aPlaylists)
{
  LOG(("sbRemotePlayer::GetPlaylists()"));
  return NS_ERROR_NOT_IMPLEMENTED;
}

// XXXredfive
NS_IMETHODIMP 
sbRemotePlayer::GetWebPlaylist(sbIMediaList **aWebplaylist)
{
  LOG(("sbRemotePlayer::GetWebPlaylist()"));
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbRemotePlayer::GetCurrentArtist(nsAString &aCurrentArtist)
{
  LOG(("sbRemotePlayer::GetCurrentArtist()"));
  mCurrentArtist->GetStringValue(aCurrentArtist);
  return NS_OK;
}

NS_IMETHODIMP 
sbRemotePlayer::GetCurrentAlbum(nsAString & aCurrentAlbum)
{
  LOG(("sbRemotePlayer::GetCurrentAlbum()"));
  mCurrentAlbum->GetStringValue(aCurrentAlbum);
  return NS_OK;
}

NS_IMETHODIMP 
sbRemotePlayer::GetCurrentTrack(nsAString & aCurrentTrack)
{
  LOG(("sbRemotePlayer::GetCurrentTrack()"));
  mCurrentTrack->GetStringValue(aCurrentTrack);
  return NS_OK;
}

NS_IMETHODIMP 
sbRemotePlayer::AddListener(const nsAString &aKey,
                            nsIObserver *aObserver)
{
  LOG(("sbRemotePlayer::AddListener()"));
  NS_ENSURE_ARG_POINTER(aObserver);

  // Make sure the key passed in is a valid one for attaching to
  for (PRUint32 index = 0 ; index < sPublicMetadataLength; index++ ) {
    // if we find it break out of the loop and keep going
    if (aKey == NS_ConvertASCIItoUTF16(sPublicMetadata[index]))
      break;
    // if we get to this point it isn't accepted, shortcircuit
    if (index == (sPublicMetadataLength-1) )
      return NS_ERROR_FAILURE;
  }

  nsresult rv;
  nsCOMPtr<sbIDataRemote> dr = do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  dr->Init( aKey, NS_LITERAL_STRING("songbird.") );
  dr->BindObserver(aObserver, PR_FALSE);

  sbRemoteObserver remObs;
  remObs.observer = aObserver;
  remObs.remote = dr;
  PRBool success = mRemObsHash.Put(aKey, remObs);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP
sbRemotePlayer::RemoveListener( const nsAString &aKey,
                                nsIObserver *aObserver )
{
  LOG(("sbRemotePlayer::RemoveListener(%s %x)",
       PromiseFlatString(aKey).get(), aObserver));

  sbRemoteObserver remObs;
  mRemObsHash.Get(aKey, &remObs);

  if (remObs.observer == aObserver) {
    LOG(("sbRemotePlayer::RemoveListener(%s %x) -- found observer",
         NS_LossyConvertUTF16toASCII(aKey).get(), aObserver));

    remObs.remote->Unbind();
    mRemObsHash.Remove(aKey);
  }
  else {
    LOG(("sbRemotePlayer::RemoveListener(%s %x) -- did NOT find observer",
         PromiseFlatString(aKey).get(), aObserver));
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbRemotePlayer::Play()
{
  LOG(("sbRemotePlayer::Play()"));
  PRBool retval;
  mGPPS->Play(&retval);
  return retval ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
sbRemotePlayer::PlayURL(const nsAString &aURL)
{
  LOG(("sbRemotePlayer::PlayURL()"));
  PRBool retval;
  mGPPS->PlayURL(aURL, &retval);
  return retval ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
sbRemotePlayer::Stop()
{
  LOG(("sbRemotePlayer::Stop()"));
  PRBool retval;
  mGPPS->Stop(&retval);
  return retval ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
sbRemotePlayer::Pause()
{
  LOG(("sbRemotePlayer::Pause()"));
  PRBool retval;
  mGPPS->Pause(&retval);
  return retval ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
sbRemotePlayer::Next()
{
  LOG(("sbRemotePlayer::Next()"));
  PRInt32 retval;
  mGPPS->Next(&retval);
  return (retval > -1) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
sbRemotePlayer::Previous()
{
  LOG(("sbRemotePlayer::Previous()"));
  PRInt32 retval;
  mGPPS->Previous(&retval);
  return (retval > -1) ? NS_OK : NS_ERROR_FAILURE;
}

// ---------------------------------------------------------------------------
//
//                        nsISecurityCheckedComponent
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbRemotePlayer::CanCreateWrapper(const nsIID *aIID, char **_retval)
{
  NS_ENSURE_ARG_POINTER(aIID);
  NS_ENSURE_ARG_POINTER(_retval);
  LOG(("sbRemotePlayer::CanCreateWrapper()"));

  if (!mInitialized)
    Init();

  return mSecurityMixin->CanCreateWrapper(aIID, _retval);
} 

NS_IMETHODIMP
sbRemotePlayer::CanCallMethod(const nsIID *aIID, const PRUnichar *aMethodName, char **_retval)
{
  NS_ENSURE_ARG_POINTER(aIID);
  NS_ENSURE_ARG_POINTER(aMethodName);
  NS_ENSURE_ARG_POINTER(_retval);

  LOG(( "sbRemotePlayer::CanCallMethod(%s)", NS_LossyConvertUTF16toASCII(aMethodName).get() ));

  if (!mInitialized)
    Init();

  return mSecurityMixin->CanCallMethod(aIID, aMethodName, _retval);
}

NS_IMETHODIMP
sbRemotePlayer::CanGetProperty(const nsIID *aIID, const PRUnichar *aPropertyName, char **_retval)
{
  NS_ENSURE_ARG_POINTER(aIID);
  NS_ENSURE_ARG_POINTER(aPropertyName);
  NS_ENSURE_ARG_POINTER(_retval);

  LOG(( "sbRemotePlayer::CanGetProperty(%s)", NS_LossyConvertUTF16toASCII(aPropertyName).get() ));

  if (!mInitialized)
    Init();

  return mSecurityMixin->CanGetProperty(aIID, aPropertyName, _retval);
}

NS_IMETHODIMP
sbRemotePlayer::CanSetProperty(const nsIID *aIID, const PRUnichar *aPropertyName, char **_retval)
{
  NS_ENSURE_ARG_POINTER(aIID);
  NS_ENSURE_ARG_POINTER(aPropertyName);
  NS_ENSURE_ARG_POINTER(_retval);

  LOG(( "sbRemotePlayer::CanSetProperty(%s)", NS_LossyConvertUTF16toASCII(aPropertyName).get() ));

  if (!mInitialized)
    Init();

  return mSecurityMixin->CanSetProperty(aIID, aPropertyName, _retval);
}

// ---------------------------------------------------------------------------
//
//                            nsIClassInfo
//
// ---------------------------------------------------------------------------

NS_IMPL_CI_INTERFACE_GETTER3( sbRemotePlayer,
                              sbISecurityAggregator,
                              sbIRemotePlayer,
                              nsISecurityCheckedComponent )

NS_IMETHODIMP 
sbRemotePlayer::GetInterfaces(PRUint32 *aCount, nsIID ***aArray)
{ 
  NS_ENSURE_ARG_POINTER(aCount);
  NS_ENSURE_ARG_POINTER(aArray);
  LOG(("sbRemotePlayer::GetInterfaces()"));
  return NS_CI_INTERFACE_GETTER_NAME(sbRemotePlayer)(aCount, aArray);
}

NS_IMETHODIMP 
sbRemotePlayer::GetHelperForLanguage(PRUint32 language, nsISupports **_retval)
{
  LOG(("sbRemotePlayer::GetHelperForLanguage()"));
  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP 
sbRemotePlayer::GetContractID(char **aContractID)
{
  LOG(("sbRemotePlayer::GetContractID()"));
  *aContractID = ToNewCString(NS_LITERAL_CSTRING(SONGBIRD_REMOTEPLAYER_CONTRACTID));
  return *aContractID ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP 
sbRemotePlayer::GetClassDescription(char **aClassDescription)
{
  LOG(("sbRemotePlayer::GetClassDescription()"));
  *aClassDescription = ToNewCString(NS_LITERAL_CSTRING("sbRemotePlayer"));
  return *aClassDescription ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP 
sbRemotePlayer::GetClassID(nsCID **aClassID)
{
  LOG(("sbRemotePlayer::GetClassID()"));
  *aClassID = (nsCID*) nsMemory::Alloc(sizeof(nsCID));
  return *aClassID ? GetClassIDNoAlloc(*aClassID) : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP 
sbRemotePlayer::GetImplementationLanguage(PRUint32 *aImplementationLanguage)
{
  LOG(("sbRemotePlayer::GetImplementationLanguage()"));
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

NS_IMETHODIMP 
sbRemotePlayer::GetFlags(PRUint32 *aFlags)
{
  LOG(("sbRemotePlayer::GetFlags()"));
  *aFlags = 0;
  return NS_OK;
}

NS_IMETHODIMP 
sbRemotePlayer::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
  LOG(("sbRemotePlayer::GetClassIDNoAlloc()"));
  *aClassIDNoAlloc = kRemotePlayerCID;
  return NS_OK;
}

// ---------------------------------------------------------------------------
//
//                             Component stuff
//
// ---------------------------------------------------------------------------

NS_METHOD
sbRemotePlayer::Register(nsIComponentManager* aCompMgr,
                         nsIFile* aPath, const char *aLoaderStr,
                         const char *aType,
                         const nsModuleComponentInfo *aInfo)
{
  nsresult rv;

  nsCOMPtr<nsICategoryManager> catMan
    (do_GetService(NS_CATEGORYMANAGER_CONTRACTID));
  if (!catMan)
    return NS_ERROR_FAILURE;

  // allow ourself to be accessed as a js global in webpage
  rv = catMan->AddCategoryEntry( JAVASCRIPT_GLOBAL_PROPERTY_CATEGORY,
                                 "songbird",      /* use this name to access */
                                 SONGBIRD_REMOTEPLAYER_CONTRACTID,
                                 PR_TRUE,         /* persist */
                                 PR_TRUE,         /* replace existing */
                                 nsnull);

  // start at the very beginning, it's a very good place to start
  rv = catMan->AddCategoryEntry( "app-startup",
                                 "songbird",      /* use this name to access */
                                 "service," SONGBIRD_REMOTEPLAYER_CONTRACTID,
                                 PR_TRUE,         /* persist */
                                 PR_TRUE,         /* replace existing */
                                 nsnull);
  return rv;
}


NS_METHOD
sbRemotePlayer::Unregister(nsIComponentManager* aCompMgr,
                           nsIFile* aPath, const char *aLoaderStr,
                           const nsModuleComponentInfo *aInfo)
{
  nsresult rv;

  nsCOMPtr<nsICategoryManager> catMan
    (do_GetService(NS_CATEGORYMANAGER_CONTRACTID));
  if (!catMan)
    return NS_ERROR_FAILURE;

  rv = catMan->DeleteCategoryEntry( JAVASCRIPT_GLOBAL_PROPERTY_CATEGORY,
                                    "songbird",
                                    PR_TRUE );   /* delete persisted data */
  rv = catMan->DeleteCategoryEntry( "app-startup",
                                    "songbird",
                                    PR_TRUE );   /* delete persisted data */
  return rv;
}

