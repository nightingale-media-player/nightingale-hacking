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
#include <sbTArrayStringEnumerator.h>

#include <nsComponentManagerUtils.h>
#include <nsIClassInfoImpl.h>
#include <nsIConsoleService.h>
#include <nsIGenericFactory.h>
#include <nsIPermissionManager.h>
#include <nsIProgrammingLanguage.h>
#include <nsIScriptSecurityManager.h>
#include <nsIServiceManager.h>
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

static NS_DEFINE_CID(kSecurityMixinCID, SONGBIRD_SECURITYMIXIN_CID);

NS_DECL_CI_INTERFACE_GETTER(sbSecurityMixin)
NS_IMPL_ISUPPORTS3(sbSecurityMixin,
                   nsISecurityCheckedComponent,
                   nsIClassInfo,
                   sbISecurityMixin)

sbSecurityMixin::sbSecurityMixin() : mInterfacesCount(0)
{
#ifdef PR_LOGGING
  if (!gLibraryLog) {
    gLibraryLog = PR_NewLogModule("sbRemotePlayer");
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
                      const nsIID** aInterfacesArray, PRUint32 aInterfacesLength,
                      const PRUnichar **aMethodsArray, PRUint32 aMethodsLength,
                      const PRUnichar **aRPropsArray, PRUint32 aRPropsLength,
                      const PRUnichar **aWPropsArray, PRUint32 aWPropsLength)
{
  NS_ENSURE_ARG(aOuter);

  mOuter = aOuter; // no refcount

  // do interfaces last so we know when to free the allocation there
  if ( NS_FAILED(CopyStrArray(aMethodsLength, aMethodsArray, &mMethods)) ||
       NS_FAILED(CopyStrArray(aRPropsLength, aRPropsArray, &mRProperties)) ||
       NS_FAILED(CopyStrArray(aWPropsLength, aWPropsArray, &mWProperties)) ||
       NS_FAILED(CopyIIDArray( aInterfacesLength, aInterfacesArray, &mInterfaces)))
    return NS_ERROR_OUT_OF_MEMORY;

  // set this only if we've succeeded
  mInterfacesCount = aInterfacesLength;

  return NS_OK;
}

// ---------------------------------------------------------------------------
//
//                        nsISecurityCheckedComponent
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbSecurityMixin::CanCreateWrapper(const nsIID *aIID, char **_retval)
{
  LOG(("sbSecurityMixin::CanCreateWrapper()"));
  NS_ENSURE_ARG(aIID);
  NS_ENSURE_ARG(_retval);

  if (!mOuter) {
    LOG(("sbSecurityMixin::CanCreateWrapper() - ERROR, no outer"));
    *_retval = nsnull;
    return NS_OK;
  }

  PRBool canCreate = PR_FALSE;
  for ( PRUint32 index = 0; index < mInterfacesCount; index++ ) {
    const nsIID* ifaceIID = mInterfaces[index];
    if (aIID->Equals(*ifaceIID)) {
      canCreate = PR_TRUE;
      break;
    }
  }
 
  if (canCreate) { 
    LOG(( "sbSecurityMixin::CanCreateWrapper() - GRANTED" ));
    *_retval = xpc_CloneAllAccess();
  } else {
    LOG(( "sbSecurityMixin::CanCreateWrapper() - DENIED" ));
    *_retval = nsnull;
  }

  return NS_OK;

/*  XXXredfive - this code will get turned on when preferences lands

  // This is a block of code that will check the preferences system to make
  //   sure the property getting access is cleared for this domain.

  // Get the current domain.
  nsCOMPtr<nsIScriptSecurityManager> secman(do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID));
  nsCOMPtr<nsIPrincipal> principal;
  secman->GetSubjectPrincipal(getter_AddRefs(principal));
  nsCOMPtr<nsIURI> codebase;
  principal->GetURI(getter_AddRefs(codebase));

  // Check the permission manager for permission for the domain
  nsCOMPtr<nsIPermissionManager> permMgr (do_GetService(NS_PERMISSIONMANAGER_CONTRACTID));
  PRUint32 permissions = nsIPermissionManager::UNKNOWN_ACTION;

  permMgr->TestPermission(codebase, "create-wrapper", &permissions);

  if (permissions == nsIPermissionManager::ALLOW_ACTION) {
    LOG(("sbSecurityMixin::CanCreateWrapper - Permission GRANTED!!!"));
    *_retval = xpc_CloneAllAccess();
  }
  else {
    LOG(("sbSecurityMixin::CanCreateWrapper - Permission DENIED (looser)!!!"));
    //*_retval = xpc_CloneAllAccess();
    *_retval = nsnull;
  }
  
  // Get the property for creation
  // Check the preferences to see if the user has enabled this domain to
  //   this capability.
  // Get the prefs service
  // Get the current domain
  // look for a key for the current domain and songbird.remote.create.class.domain set to true
  //LOG(("sbSecurityMixin::CanCreateWrapper() - leaving"));
  return NS_OK;
*/

} // CanCreateWrapper()

NS_IMETHODIMP
sbSecurityMixin::CanCallMethod(const nsIID *aIID, const PRUnichar *aMethodName, char **_retval)
{
  NS_ENSURE_ARG(aIID);
  NS_ENSURE_ARG(aMethodName);
  NS_ENSURE_ARG(_retval);

  LOG(( "sbSecurityMixin::CanCallMethod(%s)",
        NS_ConvertUTF16toUTF8(aMethodName).get() ));

  nsresult rv;
  PRBool canCall = PR_FALSE;
  PRBool hasMore = PR_FALSE;
  nsAutoString method; 

  nsCOMPtr<nsIStringEnumerator> methods = new sbTArrayStringEnumerator(&mMethods);
  NS_ENSURE_TRUE(methods, NS_ERROR_OUT_OF_MEMORY);

  while ( NS_SUCCEEDED(methods->HasMore(&hasMore)) && hasMore) {
    rv = methods->GetNext(method);
    NS_ENSURE_SUCCESS(rv, rv);
    LOG(("    -- checking method: %s", NS_ConvertUTF16toUTF8(method).get() ));
    if ( method == aMethodName ) {
      canCall = PR_TRUE;
      break;
    }
  }

  if (canCall) {
    LOG(( "sbSecurityMixin::CanCallProperty(%s) - GRANTED",
          NS_ConvertUTF16toUTF8(aMethodName).get() ));
    *_retval = xpc_CloneAllAccess();
  } else {
    LOG(( "sbSecurityMixin::CanCallProperty(%s) - DENIED",
          NS_ConvertUTF16toUTF8(aMethodName).get() ));
    *_retval = nsnull;
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
  //NS_ENSURE_ARG(aIID);
  NS_ENSURE_ARG(aPropertyName);
  NS_ENSURE_ARG(_retval);

  LOG(( "sbSecurityMixin::CanGetProperty(%s)",
        NS_ConvertUTF16toUTF8(aPropertyName).get() ));

  nsresult rv;
  PRBool canGet = PR_FALSE;
  PRBool hasMore = PR_FALSE;
  nsAutoString prop; 

  nsCOMPtr<nsIStringEnumerator> props = new sbTArrayStringEnumerator(&mRProperties);
  NS_ENSURE_TRUE(props, NS_ERROR_OUT_OF_MEMORY);

  while ( NS_SUCCEEDED(props->HasMore(&hasMore)) && hasMore) {
    rv = props->GetNext(prop);
    NS_ENSURE_SUCCESS(rv, rv);
    LOG(("    -- checking property: %s", NS_ConvertUTF16toUTF8(prop).get() ));
    if ( prop == aPropertyName ) {
      canGet = PR_TRUE;
      break;
    }
  }

  if (canGet) {
    LOG(( "sbSecurityMixin::CanGetProperty(%s) - GRANTED",
          NS_ConvertUTF16toUTF8(aPropertyName).get() ));
    *_retval = xpc_CloneAllAccess();
  } else {
    LOG(( "sbSecurityMixin::CanGetProperty(%s) - DENIED",
          NS_ConvertUTF16toUTF8(aPropertyName).get() ));
    *_retval = nsnull;
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
  //NS_ENSURE_ARG(aIID);
  NS_ENSURE_ARG(aPropertyName);
  NS_ENSURE_ARG(_retval);

  LOG(( "sbSecurityMixin::CanSetProperty(%s)",
        NS_ConvertUTF16toUTF8(aPropertyName).get() ));

  nsresult rv;
  PRBool canSet = PR_FALSE;
  PRBool hasMore = PR_FALSE;
  nsAutoString prop; 

  nsCOMPtr<nsIStringEnumerator> props = new sbTArrayStringEnumerator(&mWProperties);
  NS_ENSURE_TRUE(props, NS_ERROR_OUT_OF_MEMORY);

  while ( NS_SUCCEEDED(props->HasMore(&hasMore)) && hasMore) {
    rv = props->GetNext(prop);
    NS_ENSURE_SUCCESS(rv, rv);
    LOG(("    -- checking property: %s", NS_ConvertUTF16toUTF8(prop).get() ));
    if ( prop == aPropertyName ) {
      canSet = PR_TRUE;
      break;
    }
  }

  if (canSet) {
    LOG(( "sbSecurityMixin::CanSetProperty(%s) - GRANTED",
          NS_ConvertUTF16toUTF8(aPropertyName).get() ));
    *_retval = xpc_CloneAllAccess();
  } else {
    LOG(( "sbSecurityMixin::CanSetProperty(%s) - DENIED",
          NS_ConvertUTF16toUTF8(aPropertyName).get() ));
    *_retval = nsnull;
  }
  return NS_OK;
}

// ---------------------------------------------------------------------------
//
//                            nsIClassInfo
//
// ---------------------------------------------------------------------------

NS_IMPL_CI_INTERFACE_GETTER2(sbSecurityMixin,
                             sbISecurityMixin,
                             nsISecurityCheckedComponent)

NS_IMETHODIMP 
sbSecurityMixin::GetInterfaces(PRUint32 *aCount, nsIID ***aArray)
{ 
  return NS_CI_INTERFACE_GETTER_NAME(sbSecurityMixin)(aCount, aArray);
}

NS_IMETHODIMP 
sbSecurityMixin::GetHelperForLanguage(PRUint32 language, nsISupports **_retval)
{
    *_retval = nsnull;
    return NS_OK;
}

NS_IMETHODIMP 
sbSecurityMixin::GetContractID(char **aContractID)
{
    *aContractID = ToNewCString(NS_LITERAL_CSTRING(SONGBIRD_SECURITYMIXIN_CONTRACTID));
    return *aContractID ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP 
sbSecurityMixin::GetClassDescription(char **aClassDescription)
{
    *aClassDescription = ToNewCString(NS_LITERAL_CSTRING("sbSecurityMixin"));
    return *aClassDescription ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP 
sbSecurityMixin::GetClassID(nsCID **aClassID)
{
    *aClassID = (nsCID*) nsMemory::Alloc(sizeof(nsCID));
    return *aClassID ? GetClassIDNoAlloc(*aClassID) : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP 
sbSecurityMixin::GetImplementationLanguage(PRUint32 *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}

NS_IMETHODIMP 
sbSecurityMixin::GetFlags(PRUint32 *aFlags)
{
    *aFlags = nsIClassInfo::DOM_OBJECT;
    return NS_OK;
}

NS_IMETHODIMP 
sbSecurityMixin::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
    *aClassIDNoAlloc = kSecurityMixinCID;
    return NS_OK;
}

// ---------------------------------------------------------------------------
//
//                             Helper Functions
//
// ---------------------------------------------------------------------------

nsresult
sbSecurityMixin::CopyStrArray(PRUint32 aCount, const PRUnichar **aSourceArray, nsTArray<nsString> *aDestArray  )
{
  NS_ENSURE_ARG_POINTER(aSourceArray);
  NS_ENSURE_ARG_POINTER(aDestArray);

  for ( PRUint32 index = 0; index < aCount; index++ ) {
    if ( !aDestArray->AppendElement(nsString(aSourceArray[index])))
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

  nsIID **iids = NS_STATIC_CAST(nsIID**, nsMemory::Alloc(aCount * sizeof(nsIID*)));
  if (!iids) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  for (PRUint32 index = 0; index < aCount; ++index) {
    iids[index] = NS_STATIC_CAST(nsIID*, nsMemory::Clone(aSourceArray[index], sizeof(nsIID)));

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

char* xpc_CloneAllAccess()
{
    static const char allAccess[] = "AllAccess";
    return (char*)nsMemory::Clone(allAccess, sizeof(allAccess));
}

// ---------------------------------------------------------------------------
//
//                             Component stuff
//
// ---------------------------------------------------------------------------

// creates a generic static constructor function
NS_GENERIC_FACTORY_CONSTRUCTOR(sbSecurityMixin)

// fill out data struct to register with component system
static const nsModuleComponentInfo components[] =
{
  {
    SONGBIRD_SECURITYMIXIN_CLASSNAME,
    SONGBIRD_SECURITYMIXIN_CID,
    SONGBIRD_SECURITYMIXIN_CONTRACTID,
    sbSecurityMixinConstructor,
    NULL,
    NULL,
    NULL,
    NS_CI_INTERFACE_GETTER_NAME(sbSecurityMixin)
  }
};

// create the module info struct that is used to regsiter
NS_IMPL_NSGETMODULE(SONGBIRD_SECURITYMIXIN_CLASSNAME, components)

