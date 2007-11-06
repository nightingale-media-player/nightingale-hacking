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

#include "sbSecurityMixin.h"
#include <sbClassInfoUtils.h>
#include <sbTArrayStringEnumerator.h>

#include <nsIConsoleService.h>
#include <nsIPermissionManager.h>
#include <nsIPrefBranch.h>
#include <nsIScriptSecurityManager.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>
#include <prlog.h>
#include <plstr.h>

#include "sbRemotePlayer.h"

#define PERM_TYPE_PLAYBACK_CONTROL "rapi.playback_control"
#define PERM_TYPE_PLAYBACK_READ    "rapi.playback_read"
#define PERM_TYPE_LIBRARY_READ     "rapi.library_read"
#define PERM_TYPE_LIBRARY_WRITE    "rapi.library_write"

#define PREF_PLAYBACK_CONTROL "playback_control_disable"
#define PREF_PLAYBACK_READ    "playback_read_disable"
#define PREF_LIBRARY_READ     "library_read_disable"
#define PREF_LIBRARY_WRITE    "library_write_disable"
/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbSecurityMixin:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gLibraryLog = nsnull;
#endif

#undef LOG
#define LOG(args) PR_LOG(gLibraryLog, PR_LOG_WARN, args)

static const char* sNotificationNone = "none";
static const char* sNotificationHat = "hat";
static const char* sNotificationAlert = "alert";
static const char* sNotificationStatus = "status";


struct Scope {
  const char* name;
  const char* blocked_notification;
  const char* allowed_notification;
};

// Note: If you change the catagory names here then please change them in
// sbRemotePlayer.cpp in the sPublicCatagoryConversions array as well
static const Scope sScopes[] = {
  { "playback_control", sNotificationHat, sNotificationNone, },
  { "playback_read", sNotificationHat, sNotificationNone, },
  { "library_read", sNotificationHat, sNotificationNone, },
  { "library_write", sNotificationAlert, sNotificationStatus, },
};

static NS_DEFINE_CID( kSecurityMixinCID, SONGBIRD_SECURITYMIXIN_CID );

NS_IMPL_ISUPPORTS3( sbSecurityMixin,
                    nsIClassInfo,
                    nsISecurityCheckedComponent,
                    sbISecurityMixin )

NS_IMPL_CI_INTERFACE_GETTER2( sbSecurityMixin,
                              nsISecurityCheckedComponent,
                              sbISecurityMixin )

SB_IMPL_CLASSINFO( sbSecurityMixin,
                   SONGBIRD_SECURITYMIXIN_CONTRACTID,
                   SONGBIRD_SECURITYMIXIN_CLASSNAME,
                   nsIProgrammingLanguage::CPLUSPLUS,
                   0,
                   kSecurityMixinCID );

sbSecurityMixin::sbSecurityMixin() 
: mInterfacesCount(0),
  mPrivileged(PR_FALSE)
{
#ifdef PR_LOGGING
  if (!gLibraryLog) {
    gLibraryLog = PR_NewLogModule( "sbSecurityMixin" );
  }
#endif
}

sbSecurityMixin::~sbSecurityMixin()
{
  if (mInterfacesCount) {
    for (PRUint32 index = 0; index < mInterfacesCount; ++index)
      nsMemory::Free( mInterfaces[index] );
    nsMemory::Free(mInterfaces);
  }
}

// ---------------------------------------------------------------------------
//
//                          sbISecurityMixin
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbSecurityMixin::Init(sbISecurityAggregator *aOuter,
                      const nsIID **aInterfacesArray, PRUint32 aInterfacesArrayLength,
                      const char **aMethodsArray, PRUint32 aMethodsArrayLength,
                      const char **aRPropsArray, PRUint32 aRPropsArrayLength,
                      const char **aWPropsArray, PRUint32 aWPropsArrayLength,
                      PRBool aPrivileged)
{
  NS_ENSURE_ARG_POINTER(aOuter);

  mOuter = aOuter; // no refcount

  // do interfaces last so we know when to free the allocation there
  if ( NS_FAILED( CopyStrArray( aMethodsArrayLength, aMethodsArray, &mMethods ) ) ||
       NS_FAILED( CopyStrArray( aRPropsArrayLength, aRPropsArray, &mRProperties ) ) ||
       NS_FAILED( CopyStrArray( aWPropsArrayLength, aWPropsArray, &mWProperties ) ) ||
       NS_FAILED( CopyIIDArray( aInterfacesArrayLength,
                                aInterfacesArray,
                                &mInterfaces)) )
    return NS_ERROR_OUT_OF_MEMORY;

  // set this only if we've succeeded
  mInterfacesCount = aInterfacesArrayLength;
  mPrivileged = aPrivileged;

  return NS_OK;
}

// ---------------------------------------------------------------------------
//
//                      nsISecurityCheckedComponent (nsISCC)
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbSecurityMixin::CanCreateWrapper(const nsIID *aIID, char **_retval)
{
  LOG(("sbSecurityMixin::CanCreateWrapper()"));
  NS_ENSURE_ARG_POINTER(aIID);
  NS_ENSURE_ARG_POINTER(_retval);

  if (!mOuter) {
    LOG(("sbSecurityMixin::CanCreateWrapper() - ERROR, no outer"));
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
  }

  // Make sure the interface requested is one of the approved interfaces
  PRBool canCreate = PR_FALSE;
  for ( PRUint32 index = 0; index < mInterfacesCount; index++ ) {
    const nsIID* ifaceIID = mInterfaces[index];
    if ( aIID->Equals(*ifaceIID) ) {
      canCreate = PR_TRUE;
      break;
    }
  }
  if (!canCreate) { 
    LOG(( "sbSecurityMixin::CanCreateWrapper() - DENIED, bad interface" ));
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
  }

  // XXXredfive - if the interface is approved, create the wrapper; any actions
  //  on the object will be passed through security for further checking
  *_retval = SB_CloneAllAccess();
  return NS_OK;

#if 0
  // Get the codebase of the current page
  nsCOMPtr<nsIURI> codebase;
  GetCodebase( getter_AddRefs(codebase) );

  // bz mentioned in an email that if we don't have a codebase we can infer
  // that the code didn't come from a http: load. He also mentions that if
  // they move to a lattice of principals this may not hold true.
  // for his full message see
  //  http://groups.google.com/group/mozilla.dev.security/msg/fc48e8d0c795a638
  if (!codebase) {
    // When the commands object gets called by the sb-commands bindings we
    // wind up here, no codebase because the call is coming from internal code.
    *_retval = SB_CloneAllAccess();
    return NS_OK;
  }

  // Do this directly opposed to how it is done in CanCallMethod et al. because
  //   if ANY of these are allowed we need to let the object get created since
  //   we don't know at this point what the request on the object is actually
  //   going to be.
  if ( GetPermission( codebase, PERM_TYPE_PLAYBACK_CONTROL, PREF_PLAYBACK_CONTROL ) ||
       GetPermission( codebase, PERM_TYPE_PLAYBACK_READ, PREF_PLAYBACK_READ ) ||
       GetPermission( codebase, PERM_TYPE_LIBRARY_READ, PREF_LIBRARY_READ ) ) {
    LOG(("sbSecurityMixin::CanCreateWrapper - Permission GRANTED!!!"));
    *_retval = SB_CloneAllAccess();
  } else {
    LOG(("sbSecurityMixin::CanCreateWrapper - Permission DENIED (looser)!!!"));
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
#endif
}

NS_IMETHODIMP
sbSecurityMixin::CanCallMethod(const nsIID *aIID, const PRUnichar *aMethodName, char **_retval)
{
  // For some reasone we always get the IID for canCallMethod,
  //   so this works (see CanGetProperty)
  NS_ENSURE_ARG_POINTER(aIID);
  NS_ENSURE_ARG_POINTER(aMethodName);
  NS_ENSURE_ARG_POINTER(_retval);

  LOG(( "sbSecurityMixin::CanCallMethod(%s)",
        NS_LossyConvertUTF16toASCII(aMethodName).get() ));

  nsAutoString method; 
  nsDependentString inMethodName(aMethodName);

  GetScopedName( mMethods, inMethodName, method ); 
  if ( method.IsEmpty() ) {
    LOG(( "sbSecurityMixin::CanCallMethod(%s) - DENIED, unapproved method",
          NS_LossyConvertUTF16toASCII(aMethodName).get() ));
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
  }

  if ( GetPermissionForScopedName(method) ) {
    LOG(("sbSecurityMixin::CanCallMethod - Permission GRANTED!!!"));
    *_retval = SB_CloneAllAccess();
  } else {
    LOG(("sbSecurityMixin::CanCallMethod - Permission DENIED (looser)!!!"));
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
sbSecurityMixin::CanGetProperty(const nsIID *aIID, const PRUnichar *aPropertyName, char **_retval)
{
  // Because of the peculiarities of XPConnect, XPCWrappedNatives,
  //   XPCNativeWrappers (they're different), XPCWrappedJS and the nsISCC
  //   patterns we get 2 calls for each getproperty and one has an IID and one
  //   doesn't. So we can't verify IID here. Good thing we don't actually need
  //   it.  XXXredfive - file a bug!!!
  //NS_ENSURE_ARG_POINTER(aIID);
  NS_ENSURE_ARG_POINTER(aPropertyName);
  NS_ENSURE_ARG_POINTER(_retval);

  LOG(( "sbSecurityMixin::CanGetProperty(%s)",
        NS_LossyConvertUTF16toASCII(aPropertyName).get() ));

  nsAutoString prop; 
  nsDependentString inPropertyName(aPropertyName);

  GetScopedName( mRProperties, inPropertyName, prop ); 
  if ( prop.IsEmpty() ) {
    LOG(( "sbSecurityMixin::CanGetProperty(%s) - DENIED, unapproved property",
          NS_LossyConvertUTF16toASCII(aPropertyName).get() ));
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
  }

  if ( GetPermissionForScopedName(prop) ) {
    LOG(("sbSecurityMixin::CanGetProperty - Permission GRANTED!!!"));
    *_retval = SB_CloneAllAccess();
  } else {
    LOG(("sbSecurityMixin::CanGetProperty - Permission DENIED (looser)!!!"));
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
sbSecurityMixin::CanSetProperty(const nsIID *aIID, const PRUnichar *aPropertyName, char **_retval)
{
  // Because of the peculiarities of XPConnect, XPCWrappedNatives,
  //   XPCNativeWrappers (they're different), XPCWrappedJS and the nsISCC
  //   patterns we get 2 calls for each SetProperty and one has an IID and one
  //   doesn't. So we can't verify IID here. Good thing we don't actually need
  //   it.  XXXredfive - file a bug!!!
  //NS_ENSURE_ARG_POINTER(aIID);
  NS_ENSURE_ARG_POINTER(aPropertyName);
  NS_ENSURE_ARG_POINTER(_retval);

  LOG(( "sbSecurityMixin::CanSetProperty(%s)",
        NS_LossyConvertUTF16toASCII(aPropertyName).get() ));

  nsAutoString prop; 
  nsDependentString inPropertyName(aPropertyName);

  GetScopedName( mWProperties, inPropertyName, prop ); 
  if (prop.IsEmpty()) {
    LOG(( "sbSecurityMixin::CanSetProperty(%s) - DENIED, unapproved property",
          NS_LossyConvertUTF16toASCII(aPropertyName).get() ));
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
  }

  if ( GetPermissionForScopedName(prop) ) {
    LOG(("sbSecurityMixin::CanSetProperty - Permission GRANTED!!!"));
    *_retval = SB_CloneAllAccess();
  } else {
    LOG(("sbSecurityMixin::CanSetProperty - Permission DENIED (looser)!!!"));
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

// ---------------------------------------------------------------------------
//
//                         nsISCC helper functions
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbSecurityMixin::GetCodebase(nsIURI **aCodebase) {
  NS_ENSURE_ARG_POINTER(aCodebase);

  // Get the current domain.
  nsresult rv;
  nsCOMPtr<nsIScriptSecurityManager> secman( do_GetService( NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );
  nsCOMPtr<nsIPrincipal> principal;
  secman->GetSubjectPrincipal( getter_AddRefs(principal) );

  if (!principal) {
    LOG(("sbSecurityMixin::GetCodebase -- Error: No Subject Principal."));
    *aCodebase = nsnull;
    return NS_OK;
  }
  LOG(("sbSecurityMixin::GetCodebase -- Have Subject Principal."));

#ifdef PR_LOGGING
  nsCOMPtr<nsIPrincipal> systemPrincipal;
  secman->GetSystemPrincipal( getter_AddRefs(systemPrincipal) );

  if (principal == systemPrincipal) {
    LOG(("sbSecurityMixin::GetCodebase -- System Principal."));
  } else {
    LOG(("sbSecurityMixin::GetCodebase -- Not System Principal."));
  }
#endif

  nsCOMPtr<nsIURI> codebase;
  principal->GetDomain( getter_AddRefs(codebase) );

  if (!codebase) {
    LOG(("sbSecurityMixin::GetCodebase -- no codebase from domain, getting it from URI."));
    principal->GetURI( getter_AddRefs(codebase) );
  }

  *aCodebase = codebase;
  NS_IF_ADDREF(*aCodebase);
  return NS_OK;
} 

PRBool
sbSecurityMixin::GetScopedName(nsTArray<nsCString> &aStringArray,
                               const nsAString &aMethodName,
                               nsAString &aScopedName)
{
  LOG(( "sbSecurityMixin::GetScopedName()"));
  PRBool approved = PR_FALSE;
  nsAutoString method; 

  nsCOMPtr<nsIStringEnumerator> methods = new sbTArrayStringEnumerator(&aStringArray);
  NS_ENSURE_TRUE( methods, PR_FALSE );

  while ( NS_SUCCEEDED( methods->GetNext(method) ) ) {
    LOG(( "    -- checking method: %s", NS_ConvertUTF16toUTF8(method).get() ));
    if ( StringEndsWith( method, aMethodName ) ) {
      aScopedName = method;
      approved = PR_TRUE;
      break;
    }
  }
  return approved;
}

PRBool
sbSecurityMixin::GetPermissionForScopedName(const nsAString &aScopedName,
                                            PRBool disableNotificationCheck)
{
  LOG(( "sbSecurityMixin::GetPermissionForScopedName()"));

  // If this instance was set to be privileged, skip the check
  if (mPrivileged) {
    return PR_TRUE;
  }

  PRBool allowed = PR_FALSE;

  nsCOMPtr<nsIURI> codebase;
  GetCodebase( getter_AddRefs(codebase) );

  if ( StringBeginsWith( aScopedName, NS_LITERAL_STRING("internal:") ) ) {
    if (!codebase) {
      // internal stuff always allowed, should never be called from insecure code.
      allowed = PR_TRUE;
    } else {
      // I think this is always a bad thing. It should represent calling of 
      // methods flagged as internal only being called by insecure code (not
      // using system principal)
      NS_WARNING("sbSecurityMixin::GetPermissionForScopedName() -- AHHH!!! Asked for internal with codebase");
      return PR_FALSE;
    }
  }

  // without a codebase we're either calling internal stuff or wrong.
  if (!codebase)
    return allowed;
  
  // look up information about the scope
  const struct Scope* scope = GetScopeForScopedName( aScopedName );
  
  if (scope) {
    // if the current scope is in the table then use the values in the table
    // to get permission
    allowed = GetPermission( codebase, scope );
  }
  else if ( StringBeginsWith( aScopedName, NS_LITERAL_STRING("site:") ) ) {
    // site library methods are always cleared
    allowed = PR_TRUE;
  }
  else if ( StringBeginsWith( aScopedName, NS_LITERAL_STRING("helper:") ) ) {
    // helper classes always allowed
    allowed = PR_TRUE;
  }
  else if ( StringBeginsWith( aScopedName, NS_LITERAL_STRING("classinfo:") ) ) {
    // class info stuff always allowed - this might need to move above the codebase check.
    allowed = PR_TRUE;
  }
  
  // if we found additional info about the scope, work out if there's a
  // notification to fire
  if ( scope && !disableNotificationCheck ) {
    const char* notification =
        allowed ? scope->allowed_notification : scope->blocked_notification;
    LOG(( "sbSecurityMixin::GetPermissionsForScopedName() notification=%s",
          notification ));

    if ( strcmp(notification, sNotificationAlert) == 0 ) {
      LOG(( "sbSecurityMixin::GetPermissionsForScopedName() - ALERT" ));
      // Launch a prompt asking the user if they want to do this action.

      allowed =
        sbRemotePlayer::GetUserApprovalForHost( codebase,
                            NS_LITERAL_STRING("rapi.media_add.request.title"),
                            NS_LITERAL_STRING("rapi.media_add.request.message"),
                            scope->name );

    }
    else if ( strcmp(notification, sNotificationStatus) == 0 ) {
      LOG(( "sbSecurityMixin::GetPermissionsForScopedName() - STATUS" ));
    }
    else if ( strcmp(notification, sNotificationHat) == 0 ) {
      LOG(( "sbSecurityMixin::GetPermissionsForScopedName() - HAT" ));
      // notification is not "none"
      
      // we want to fire a notification, but does the user want a notification?
      
      // get the prefs service
      nsresult rv;
      nsCOMPtr<nsIPrefBranch> prefService =
        do_GetService( "@mozilla.org/preferences-service;1", &rv );
      NS_ENSURE_SUCCESS( rv, allowed );
      
      PRBool notify;
      // look up the pref
      nsCString prefKey("songbird.rapi.");
      prefKey.Append(scope->name);
      prefKey.AppendLiteral("_notify");
      rv = prefService->GetBoolPref( prefKey.get(), &notify );
      NS_ENSURE_SUCCESS( rv, allowed );
      
      LOG(( "sbSecurityMixin::GetPermissionsForScopedName() pref value: %s",
           notify?"true":"false" ));
      
      // if the pref is true we should notify
      if (notify) {
        DispatchNotificationEvent(notification, scope, allowed);
      }
    }
  }
  
  return allowed;
}

const struct Scope*
sbSecurityMixin::GetScopeForScopedName(const nsAString &aScopedName) {
  for (unsigned i=0; i<NS_ARRAY_LENGTH(sScopes); i++) {
    NS_ConvertUTF8toUTF16 prefix( sScopes[i].name );
    prefix.AppendLiteral(":");
    if( StringBeginsWith( aScopedName, prefix ) ) {
      return &sScopes[i];
    }
  }
  return NULL;
}

PRBool
sbSecurityMixin::GetPermission(nsIURI *aURI, const struct Scope *aScope )
{
  NS_ENSURE_TRUE( aURI, PR_FALSE );
  NS_ENSURE_TRUE( aScope, PR_FALSE );
  NS_ENSURE_TRUE( aScope->name, PR_FALSE );

#ifdef PR_LOGGING
  if (aURI) {
    nsCAutoString spec;
    aURI->GetSpec(spec);
    LOG(( "sbSecurityMixin::GetPermission( %s, %s)", spec.get(), aScope->name ));
  }
#endif

  nsresult rv;
  nsCOMPtr<nsIPrefBranch> prefService =
    do_GetService( "@mozilla.org/preferences-service;1", &rv );
  NS_ENSURE_SUCCESS( rv, PR_FALSE );

  // build the pref key to check
  PRBool prefBlocked = PR_TRUE;
  nsCString prefKey("songbird.rapi.");
  prefKey.Append(aScope->name);
  prefKey.AppendLiteral("_disable");

  // get the pref value
  LOG(( "sbSecurityMixin::GetPermission() - asking for pref: %s", prefKey.get() ));
  rv = prefService->GetBoolPref( prefKey.get(), &prefBlocked );
  NS_ENSURE_SUCCESS( rv, PR_FALSE );

  // get the permission for the domain
  nsCString permission_name("rapi.");
  permission_name.Append(aScope->name);
  nsCOMPtr<nsIPermissionManager> permMgr( do_GetService( NS_PERMISSIONMANAGER_CONTRACTID, &rv ) );
  NS_ENSURE_SUCCESS( rv, PR_FALSE );
  PRUint32 perms = nsIPermissionManager::UNKNOWN_ACTION;
  rv = permMgr->TestPermission( aURI, permission_name.get(), &perms );
  NS_ENSURE_SUCCESS( rv, PR_FALSE );

  // check to see if the action is allowed for the domain
  if (prefBlocked) {
    LOG(( "sbSecurityMixin::GetPermission() - action is blocked" ));
    // action is blocked, see if domain is cleared explicitly
    if ( perms == nsIPermissionManager::ALLOW_ACTION ) {
      LOG(("sbSecurityMixin::GetPermission - Permission GRANTED!!!"));
      return PR_TRUE;
    }
  } else {
    LOG(( "sbSecurityMixin::GetPermission() - action not blocked" ));
    // action not blocked, make sure domain isn't blocked explicitly
    if ( perms != nsIPermissionManager::DENY_ACTION ) {
      LOG(("sbSecurityMixin::GetPermission - Permission GRANTED!!!"));
      return PR_TRUE;
    }
  }

  // Negative Ghostrider, pattern is full.
  LOG(("sbSecurityMixin::GetPermission - Permission DENIED (looooooser)!!!"));
  return PR_FALSE;
}

nsresult
sbSecurityMixin::SetPermission(nsIURI *aURI, const nsACString &aScopedName)
{
  NS_ENSURE_ARG(aURI);
  NS_ENSURE_TRUE( !aScopedName.IsEmpty(), NS_ERROR_INVALID_ARG );
  
  nsresult rv;
  nsCString permission_name("rapi.");
  permission_name.Append(aScopedName);
  nsCOMPtr<nsIPermissionManager> permMgr =
    do_GetService( NS_PERMISSIONMANAGER_CONTRACTID, &rv );
  NS_ENSURE_SUCCESS( rv, rv );
  
  rv = permMgr->Add( aURI,
                     permission_name.BeginReading(),
                     nsIPermissionManager::ALLOW_ACTION );
  NS_ENSURE_SUCCESS( rv, rv );
  
  return NS_OK;
}


nsresult
sbSecurityMixin::DispatchNotificationEvent(const char* aNotificationType,
                                           const Scope* aScope,
                                           PRBool aHasAccess)
{
  // NOTE: This method only /tries/ to dispatch the notification event.
  // If there's no notification document then it fails mostly silently.
  // This is intentional since there might be cases where this mixin is
  // used when no notification document is available.
  
  NS_ENSURE_ARG_POINTER(aNotificationType);
  NS_ENSURE_ARG_POINTER(aScope);
  
  // TODO: we need to add the notification type to the event eventually
  // get it? event eventually...
  
  LOG(( "sbSecurityMixin::DispatchNotificationEvent(%s)", aNotificationType ));
  
  // see if we've got a document to dispatch events to
  if ( mNotificationDocument ) {
    LOG(( "sbSecurityMixin::DispatchNotificationEvent - dispatching event" ));

    nsCOMPtr<sbIRemotePlayer> remotePlayer;
    nsresult rv = mOuter->GetRemotePlayer(getter_AddRefs(remotePlayer));
    
    // objects that do not return a remote player do not want to trigger notifications because
    // they are owned by other objects that supercede them in the security hierarchy.
    if(NS_SUCCEEDED(rv)) {

      return sbRemotePlayer::DispatchSecurityEvent( mNotificationDocument,
        remotePlayer,
        RAPI_EVENT_CLASS,
        RAPI_EVENT_TYPE,
        NS_ConvertASCIItoUTF16(aScope->name),
        aHasAccess,
        PR_TRUE );

    }

  } else {
    LOG(( "sbSecurityMixin::DispatchNotificationEvent - not dispatching event" ));
    NS_WARNING( "sbSecurityMixin::DispatchNotificationEvent didn't have a notification document to dispatch to" );
  }
  
  return NS_OK;
}

NS_IMETHODIMP
sbSecurityMixin::GetNotificationDocument(nsIDOMDocument **aNotificationDocument)
{
  NS_ENSURE_ARG_POINTER(aNotificationDocument);

  NS_IF_ADDREF(*aNotificationDocument = mNotificationDocument);

  return NS_OK;
}
NS_IMETHODIMP sbSecurityMixin::SetNotificationDocument(nsIDOMDocument * aNotificationDocument)
{
  NS_ENSURE_ARG_POINTER(aNotificationDocument);

  mNotificationDocument = aNotificationDocument;
  return NS_OK;
}


// ---------------------------------------------------------------------------
//
//                             Helper Functions
//
// ---------------------------------------------------------------------------

nsresult
sbSecurityMixin::CopyStrArray(PRUint32 aCount, const char **aSourceArray, nsTArray<nsCString> *aDestArray  )
{
  NS_ENSURE_ARG_POINTER(aSourceArray);
  NS_ENSURE_ARG_POINTER(aDestArray);

  for ( PRUint32 index = 0; index < aCount; index++ ) {
    if ( !aDestArray->AppendElement(aSourceArray[index]))
      return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}

nsresult
sbSecurityMixin::CopyIIDArray(PRUint32 aCount, const nsIID **aSourceArray, nsIID ***aDestArray)
{
  NS_ENSURE_ARG_POINTER(aSourceArray);
  NS_ENSURE_ARG_POINTER(aDestArray);

  *aDestArray = 0;

  nsIID **iids = static_cast<nsIID**>( nsMemory::Alloc( aCount * sizeof(nsIID*) ) );
  if (!iids) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  for (PRUint32 index = 0; index < aCount; ++index) {
    iids[index] = static_cast<nsIID*>( nsMemory::Clone( aSourceArray[index], sizeof(nsIID) ) );

    if (!iids[index]) {
      for (PRUint32 alloc_index = 0; alloc_index < index; ++alloc_index)
        nsMemory::Free( iids[alloc_index] );
      nsMemory::Free(iids);
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  *aDestArray = iids;
  return NS_OK;
}

inline char* SB_CloneAllAccess()
{
  return ToNewCString( NS_LITERAL_CSTRING("AllAccess") );
}

NS_IMETHODIMP
sbSecurityMixin::GetPermissionForScopedNameWrapper( const nsAString& aRemotePermCategory,
                                                    PRBool *_retval )
{
  // We need to prevent the notification from appearing when calling
  *_retval = GetPermissionForScopedName( aRemotePermCategory, PR_TRUE );
  return NS_OK;
}
