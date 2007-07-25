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
#include <nsIProgrammingLanguage.h>
#include <nsIScriptSecurityManager.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>
#include <prlog.h>

#define PERM_TYPE_CONTROLS "rapi.controls"
#define PERM_TYPE_BINDING  "rapi.binding"
#define PERM_TYPE_METADATA "rapi.metadata"
#define PERM_TYPE_LIBRARY "rapi.library"

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbSecurityMixin:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gLibraryLog = nsnull;
#define LOG(args)   if (gLibraryLog) PR_LOG(gLibraryLog, PR_LOG_WARN, args)
#else
#define LOG(args)   /* nothing */
#endif

static NS_DEFINE_CID(kSecurityMixinCID, SONGBIRD_SECURITYMIXIN_CID);

NS_IMPL_ISUPPORTS3(sbSecurityMixin,
                   nsIClassInfo,
                   nsISecurityCheckedComponent,
                   sbISecurityMixin)

NS_IMPL_CI_INTERFACE_GETTER2(sbSecurityMixin,
                             nsISecurityCheckedComponent,
                             sbISecurityMixin)

SB_IMPL_CLASSINFO( sbSecurityMixin,
                   SONGBIRD_SECURITYMIXIN_CONTRACTID,
                   SONGBIRD_SECURITYMIXIN_CLASSNAME,
                   nsIProgrammingLanguage::CPLUSPLUS,
                   0,
                   kSecurityMixinCID );

sbSecurityMixin::sbSecurityMixin() : mInterfacesCount(0)
{
#ifdef PR_LOGGING
  if (!gLibraryLog) {
    gLibraryLog = PR_NewLogModule("sbSecurityMixin");
  }
#endif
}

sbSecurityMixin::~sbSecurityMixin()
{
  if (mInterfacesCount) {
    for (PRUint32 index = 0; index < mInterfacesCount; ++index)
      nsMemory::Free(mInterfaces[index]);
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
                      const char **aWPropsArray, PRUint32 aWPropsArrayLength)
{
  NS_ENSURE_ARG_POINTER(aOuter);

  mOuter = aOuter; // no refcount

  // do interfaces last so we know when to free the allocation there
  if ( NS_FAILED(CopyStrArray(aMethodsArrayLength, aMethodsArray, &mMethods)) ||
       NS_FAILED(CopyStrArray(aRPropsArrayLength, aRPropsArray, &mRProperties)) ||
       NS_FAILED(CopyStrArray(aWPropsArrayLength, aWPropsArray, &mWProperties)) ||
       NS_FAILED(CopyIIDArray(aInterfacesArrayLength,
                              aInterfacesArray,
                              &mInterfaces)) )
    return NS_ERROR_OUT_OF_MEMORY;

  // set this only if we've succeeded
  mInterfacesCount = aInterfacesArrayLength;

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
    if (aIID->Equals(*ifaceIID)) {
      canCreate = PR_TRUE;
      break;
    }
  }
  if (!canCreate) { 
    LOG(( "sbSecurityMixin::CanCreateWrapper() - DENIED, bad interface" ));
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
  }

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
  if ( GetPermission(codebase, PERM_TYPE_CONTROLS, "disable_controls") ||
       GetPermission(codebase, PERM_TYPE_BINDING, "disable_binding") ||
       GetPermission(codebase, PERM_TYPE_METADATA, "disable_metadata") ) {
    LOG(("sbSecurityMixin::CanCreateWrapper - Permission GRANTED!!!"));
    *_retval = SB_CloneAllAccess();
  } else {
    LOG(("sbSecurityMixin::CanCreateWrapper - Permission DENIED (looser)!!!"));
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
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

  GetScopedName(mMethods, inMethodName, method); 
  if (method.IsEmpty()) {
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

  GetScopedName(mRProperties, inPropertyName, prop); 
  if (prop.IsEmpty()) {
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

  GetScopedName(mWProperties, inPropertyName, prop); 
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
  nsCOMPtr<nsIScriptSecurityManager> secman(do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIPrincipal> principal;
  secman->GetSubjectPrincipal(getter_AddRefs(principal));

  if (!principal) {
    LOG(("sbSecurityMixin::GetCodebase -- Error: No Subject Principal."));
    *aCodebase = nsnull;
    return NS_OK;
  }
  LOG(("SecurityMixin::GetCodebase -- Have Subject Principal."));

#ifdef PR_LOGGING
  nsCOMPtr<nsIPrincipal> systemPrincipal;
  secman->GetSystemPrincipal(getter_AddRefs(systemPrincipal));

  if (principal == systemPrincipal) {
    LOG(("sbSecurityMixin::GetCodebase -- System Principal."));
  } else {
    LOG(("sbSecurityMixin::GetCodebase -- Not System Principal."));
  }
#endif

  nsCOMPtr<nsIURI> codebase;
  principal->GetDomain(getter_AddRefs(codebase));

  if (!codebase) {
    LOG(("sbSecurityMixin::GetCodebase -- no codebase from domain, getting it from URI."));
    principal->GetURI(getter_AddRefs(codebase));
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
  NS_ENSURE_TRUE(methods, PR_FALSE);

  while ( NS_SUCCEEDED(methods->GetNext(method)) ) {
    LOG(("    -- checking method: %s", NS_ConvertUTF16toUTF8(method).get() ));
    if ( StringEndsWith(method, aMethodName) ) {
      aScopedName = method;
      approved = PR_TRUE;
      break;
    }
  }
  return approved;
}

PRBool
sbSecurityMixin::GetPermissionForScopedName(const nsAString &aScopedName)
{
  LOG(( "sbSecurityMixin::GetPermissionForScopedName()"));
  PRBool allowed = PR_FALSE;

  nsCOMPtr<nsIURI> codebase;
  GetCodebase(getter_AddRefs(codebase));

  if (StringBeginsWith(aScopedName, NS_LITERAL_STRING("internal:"))) {
    if (!codebase) {
      // internal stuff always allowed, should never be called from insecure code.
      allowed = PR_TRUE;
    } else {
      // I think this is always a bad thing. It should represent calling of 
      // methods flagged as internal only being called by insecure code (not
      // using system principal)
      LOG(( "sbSecurityMixin::GetPermissionForScopedName() -- AHHH!!! Asked for internal with codebase"));
      return PR_FALSE;
    }
  }

  // without a codebase we're either calling internal stuff or wrong.
  if (!codebase)
    return allowed;

  if (StringBeginsWith(aScopedName, NS_LITERAL_STRING("controls:"))) {
    allowed = GetPermission(codebase, PERM_TYPE_CONTROLS, "disable_controls");
  }
  else if (StringBeginsWith(aScopedName, NS_LITERAL_STRING("binding:"))) {
    allowed = GetPermission(codebase, PERM_TYPE_BINDING, "disable_binding");
  }
  else if (StringBeginsWith(aScopedName, NS_LITERAL_STRING("metadata:"))) {
    allowed = GetPermission(codebase, PERM_TYPE_METADATA, "disable_metadata");
  }
  else if (StringBeginsWith(aScopedName, NS_LITERAL_STRING("library:"))) {
    allowed = GetPermission(codebase, PERM_TYPE_LIBRARY, "disable_library");
  }
  else if (StringBeginsWith(aScopedName, NS_LITERAL_STRING("site:"))) {
    // site library methods are always cleared
    allowed = PR_TRUE;
  }
  else if (StringBeginsWith(aScopedName, NS_LITERAL_STRING("helper:"))) {
    // helper classes always allowed
    allowed = PR_TRUE;
  }
  else if (StringBeginsWith(aScopedName, NS_LITERAL_STRING("classinfo:"))) {
    // class info stuff always allowed - this might need to move above the codebase check.
    allowed = PR_TRUE;
  }
  return allowed;
}

PRBool
sbSecurityMixin::GetPermission(nsIURI *aURI, const char *aType, const char *aRAPIPref )
{
  NS_ENSURE_TRUE(aURI, PR_FALSE);
  NS_ENSURE_TRUE(aType, PR_FALSE);
  NS_ENSURE_TRUE(aRAPIPref, PR_FALSE);

#ifdef PR_LOGGING
  if (aURI) {
    nsCAutoString spec;
    aURI->GetSpec(spec);
    LOG(( "sbSecurityMixin::GetPermission( %s, %s, %s)", spec.get(), aType, aRAPIPref ));
  }
#endif

  nsresult rv;
  nsCOMPtr<nsIPrefBranch> prefService =
    do_GetService("@mozilla.org/preferences-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  // build the pref key to check
  PRBool prefBlocked = PR_TRUE;
  nsCAutoString prefKey("songbird.rapi.");
  prefKey += aRAPIPref;

  // get the pref value
  LOG(( "sbSecurityMixin::GetPermission() - asking for pref: %s", prefKey.get() ));
  rv = prefService->GetBoolPref(prefKey.get(), &prefBlocked);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  // get the permission for the domain
  nsCOMPtr<nsIPermissionManager> permMgr(do_GetService(NS_PERMISSIONMANAGER_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  PRUint32 perms = nsIPermissionManager::UNKNOWN_ACTION;
  rv = permMgr->TestPermission(aURI, aType, &perms);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

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

  nsIID **iids = static_cast<nsIID**>(nsMemory::Alloc(aCount * sizeof(nsIID*)));
  if (!iids) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  for (PRUint32 index = 0; index < aCount; ++index) {
    iids[index] = static_cast<nsIID*>(nsMemory::Clone(aSourceArray[index], sizeof(nsIID)));

    if (!iids[index]) {
      for (PRUint32 alloc_index = 0; alloc_index < index; ++alloc_index)
        nsMemory::Free(iids[alloc_index]);
      nsMemory::Free(iids);
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  *aDestArray = iids;
  return NS_OK;
}

inline char* SB_CloneAllAccess()
{
  return ToNewCString(NS_LITERAL_CSTRING("AllAccess"));
}

