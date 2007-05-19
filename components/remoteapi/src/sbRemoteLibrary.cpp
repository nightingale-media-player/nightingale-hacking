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

#include "sbRemoteLibrary.h"
#include <sbILibrary.h>
#include <sbILibraryFactory.h>
#include <sbILibraryManager.h>
#include <sbLocalDatabaseCID.h>

#include <nsComponentManagerUtils.h>
#include <nsEventDispatcher.h>
#include <nsHashKeys.h>
#include <nsICategoryManager.h>
#include <nsIClassInfoImpl.h>
#include <nsIDocument.h>
#include <nsIDOMDocument.h>
#include <nsIDOMDocumentEvent.h>
#include <nsIDOMEvent.h>
#include <nsIDOMEventTarget.h>
#include <nsIDOMWindow.h>
#include <nsIFile.h>
#include <nsIPermissionManager.h>
#include <nsIPrefService.h>
#include <nsIPresShell.h>
#include <nsIProgrammingLanguage.h>
#include <nsIProperties.h>
#include <nsIScriptNameSpaceManager.h>
#include <nsIScriptSecurityManager.h>
#include <nsISupportsPrimitives.h>
#include <nsIURI.h>
#include <nsIWindowWatcher.h>
#include <nsIWritablePropertyBag2.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>
#include <nsWeakPtr.h>
#include <prlog.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteLibrary:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gLibraryLog = nsnull;
#define LOG(args)   if (gLibraryLog) PR_LOG(gLibraryLog, PR_LOG_WARN, args)
#else
#define LOG(args)   /* nothing */
#endif

static NS_DEFINE_CID(kRemoteLibraryCID, SONGBIRD_REMOTELIBRARY_CID);

const static char* sPublicWProperties[] = {""};

const static char* sPublicRProperties[] =
  { "metadata:artists",
    "metadata:albums",
    "metadata:name",
    "metadata:type",
    "metadata:length",
    "classinfo:classDescription",
    "classinfo:contractID",
    "classinfo:classID",
    "classinfo:implementationLanguage",
    "classinfo:flags" };

const static char* sPublicMethods[] =
  { "binding:connectToMediaLibrary",
    "binding:addMediaItem",
    "binding:addMediaListByURL" };

NS_IMPL_ISUPPORTS4( sbRemoteLibrary,
                    nsIClassInfo,
                    nsISecurityCheckedComponent,
                    sbISecurityAggregator,
                    sbIRemoteLibrary )

sbRemoteLibrary::sbRemoteLibrary() 
{
#ifdef PR_LOGGING
  if (!gLibraryLog) {
    gLibraryLog = PR_NewLogModule("sbRemoteLibrary");
  }
  LOG(("sbRemoteLibrary::sbRemoteLibrary()"));
#endif
}

sbRemoteLibrary::~sbRemoteLibrary()
{
  LOG(("sbRemoteLibrary::~sbRemoteLibrary()"));
}

nsresult
sbRemoteLibrary::Init()
{
  LOG(("sbRemoteLibrary::Init()"));

  nsresult rv;
  nsCOMPtr<sbISecurityMixin> mixin =
      do_CreateInstance( "@songbirdnest.com/remoteapi/security-mixin;1", &rv );
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the list of IIDs to pass to the security mixin
  nsIID **iids;
  PRUint32 iidCount;
  GetInterfaces( &iidCount, &iids );

  // initialize our mixin with approved interfaces, methods, properties
  rv = mixin->Init( (sbISecurityAggregator*)this,
                    ( const nsIID** )iids, iidCount,
                    sPublicMethods, NS_ARRAY_LENGTH(sPublicMethods),
                    sPublicRProperties, NS_ARRAY_LENGTH(sPublicRProperties),
                    sPublicWProperties, NS_ARRAY_LENGTH(sPublicWProperties) );
  NS_ENSURE_SUCCESS(rv, rv);

  mSecurityMixin = do_QueryInterface( mixin, &rv );
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// ---------------------------------------------------------------------------
//
//                        sbIRemoteLibrary
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbRemoteLibrary::ConnectToMediaLibrary( const nsAString &aLibraryID )
{
  LOG(( "sbRemoteLibrary::ConnectToMediaLibrary(%s)",
        NS_LossyConvertUTF16toASCII(aLibraryID).get() ));

  // convert the id to a guid understandable to the librarymananger
  nsAutoString guid;
  nsresult rv = GetLibraryGUID(aLibraryID, guid);
  if ( NS_SUCCEEDED(rv) ) {
    LOG(( "sbRemoteLibrary::ConnectToMediaLibrary(%s) -- IS a default library",
          NS_LossyConvertUTF16toASCII(guid).get() ));

    // See if the library manager has it lying around.
    nsCOMPtr<sbILibraryManager> libManager(
        do_GetService( "@songbirdnest.com/Songbird/library/Manager;1", &rv ) );
    NS_ENSURE_SUCCESS( rv, rv );
    LOG(("sbRemoteLibrary::ConnectToMediaLibrary() -- have library manager"));

    rv = libManager->GetLibrary( guid, getter_AddRefs(mLibrary) );
    NS_ENSURE_SUCCESS( rv, rv );
    return NS_OK;
  }

  //
  // If we're here, the ID passed in wasn't a default, it should be a path
  //
  LOG(( "sbRemoteLibrary::ConnectToMediaLibrary(%d) - non default library",
        HashString(aLibraryID) ));

  //
  // Add Security Check here to verify website matches the path.
  //   see nsCookieService.cpp CheckPath() for details.
  // might take place in GetLibraryGUID as an alternative
  //

  // This should be moved to the else case in the GetGUID because we want to 
  // try and get the library from the manager first and only if that fails
  // should be go to the factory. But for now this is fine.

  // get the directory service
  nsCOMPtr<nsIProperties> directoryService(
                       do_GetService( NS_DIRECTORY_SERVICE_CONTRACTID, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );
  LOG(("sbRemoteLibrary::ConnectToMediaLibrary() -- have directory service"));

  // get the users profile directory
  nsCOMPtr<nsIFile> currProfileDir;
  rv = directoryService->Get( "ProfD",
                              NS_GET_IID(nsIFile),
                              getter_AddRefs(currProfileDir) );
  NS_ENSURE_SUCCESS( rv, rv );
  LOG(("sbRemoteLibrary::ConnectToMediaLibrary() -- have profileD"));

  // build the filename
  // XXX need the domain as a prefix here
  nsAutoString filename;
  filename.AppendInt(HashString(aLibraryID));
  filename.AppendLiteral(".db");
  LOG(( "sbRemoteLibrary::ConnectToMediaLibrary() -- filename: %s",
        NS_LossyConvertUTF16toASCII(filename).get() ));

  // extend the path and add the file
  currProfileDir->Append( NS_LITERAL_STRING("db") );
  currProfileDir->Append(filename);

  // Get the library factory to create a new instance from the file
  nsCOMPtr<sbILibraryFactory> libFactory(
        do_CreateInstance( SB_LOCALDATABASE_LIBRARYFACTORY_CONTRACTID, &rv ) );
  NS_ENSURE_SUCCESS(rv, rv);
  LOG(("sbRemoteLibrary::ConnectToMediaLibrary() -- have library factory"));

  // Get a propertybag to pass in the filename
  nsCOMPtr<nsIWritablePropertyBag2> propBag(
                do_CreateInstance( "@mozilla.org/hash-property-bag;1", &rv ) );
  NS_ENSURE_SUCCESS(rv, rv);
  propBag->SetPropertyAsInterface( NS_LITERAL_STRING("databaseFile"),
                                   currProfileDir );

  // Create the library
  libFactory->CreateLibrary( propBag, getter_AddRefs(mLibrary) );
  NS_ENSURE_STATE(mLibrary);

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
sbRemoteLibrary::GetLibraryGUID( const nsAString &aLibraryID,
                                 nsAString &aLibraryGUID )
{
  LOG(( "sbRemoteLibrary::GetLibraryGUID(%s)",
        NS_LossyConvertUTF16toASCII(aLibraryID).get() ));

  nsCAutoString prefKey;

  // match the 'magic' strings to the keys for the prefs
  if ( aLibraryID == NS_LITERAL_STRING("main") ) {
    prefKey.AssignLiteral("songbird.library.main");
  } else if ( aLibraryID == NS_LITERAL_STRING("web") ) {
    prefKey.AssignLiteral("songbird.library.web");
  } else if ( aLibraryID == NS_LITERAL_STRING("download") ) {
    prefKey.AssignLiteral("songbird.library.download");
  }

  // right now just bail if it isn't a default
  if ( prefKey == EmptyCString() ) {
    LOG(( "sbRemoteLibrary::GetLibraryGUID() -- not a default library"));
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

// ---------------------------------------------------------------------------
//
//                        nsISecurityCheckedComponent
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbRemoteLibrary::CanCreateWrapper( const nsIID *aIID, char **_retval )
{
  NS_ENSURE_ARG_POINTER(aIID);
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_STATE(mSecurityMixin);

  LOG(("sbRemoteLibrary::CanCreateWrapper()"));

  return mSecurityMixin->CanCreateWrapper( aIID, _retval );
} 

NS_IMETHODIMP
sbRemoteLibrary::CanCallMethod( const nsIID *aIID,
                                const PRUnichar *aMethodName,
                                char **_retval )
{
  NS_ENSURE_ARG_POINTER(aIID);
  NS_ENSURE_ARG_POINTER(aMethodName);
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_STATE(mSecurityMixin);

  LOG(( "sbRemoteLibrary::CanCallMethod(%s)",
        NS_LossyConvertUTF16toASCII(aMethodName).get() ));

  return mSecurityMixin->CanCallMethod( aIID, aMethodName, _retval );
}

NS_IMETHODIMP
sbRemoteLibrary::CanGetProperty( const nsIID *aIID,
                                 const PRUnichar *aPropertyName,
                                 char **_retval )
{
  NS_ENSURE_ARG_POINTER(aIID);
  NS_ENSURE_ARG_POINTER(aPropertyName);
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_STATE(mSecurityMixin);

  LOG(( "sbRemoteLibrary::CanGetProperty(%s)",
        NS_LossyConvertUTF16toASCII(aPropertyName).get() ));

  return mSecurityMixin->CanGetProperty( aIID, aPropertyName, _retval );
}

NS_IMETHODIMP
sbRemoteLibrary::CanSetProperty( const nsIID *aIID,
                                 const PRUnichar *aPropertyName,
                                 char **_retval )
{
  NS_ENSURE_ARG_POINTER(aIID);
  NS_ENSURE_ARG_POINTER(aPropertyName);
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_STATE(mSecurityMixin);

  LOG(( "sbRemoteLibrary::CanSetProperty(%s)",
        NS_LossyConvertUTF16toASCII(aPropertyName).get() ));

  return mSecurityMixin->CanSetProperty( aIID, aPropertyName, _retval );
}

// ---------------------------------------------------------------------------
//
//                            nsIClassInfo
//
// ---------------------------------------------------------------------------

NS_IMPL_CI_INTERFACE_GETTER3( sbRemoteLibrary,
                              sbISecurityAggregator,
                              sbIRemoteLibrary,
                              nsISecurityCheckedComponent )

NS_IMETHODIMP 
sbRemoteLibrary::GetInterfaces( PRUint32 *aCount, nsIID ***aArray )
{ 
  NS_ENSURE_ARG_POINTER(aCount);
  NS_ENSURE_ARG_POINTER(aArray);
  LOG(("sbRemoteLibrary::GetInterfaces()"));
  return NS_CI_INTERFACE_GETTER_NAME(sbRemoteLibrary)( aCount, aArray );
}

NS_IMETHODIMP 
sbRemoteLibrary::GetHelperForLanguage( PRUint32 language,
                                       nsISupports **_retval )
{
  LOG(("sbRemoteLibrary::GetHelperForLanguage()"));
  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP 
sbRemoteLibrary::GetContractID( char **aContractID )
{
  LOG(("sbRemoteLibrary::GetContractID()"));
  *aContractID = ToNewCString( NS_LITERAL_CSTRING(
                                          SONGBIRD_REMOTELIBRARY_CONTRACTID) );
  return *aContractID ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP 
sbRemoteLibrary::GetClassDescription( char **aClassDescription )
{
  LOG(("sbRemoteLibrary::GetClassDescription()"));
  *aClassDescription = ToNewCString( NS_LITERAL_CSTRING(
                                           SONGBIRD_REMOTELIBRARY_CLASSNAME) );
  return *aClassDescription ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP 
sbRemoteLibrary::GetClassID( nsCID **aClassID )
{
  LOG(("sbRemoteLibrary::GetClassID()"));
  *aClassID = (nsCID*) nsMemory::Alloc( sizeof(nsCID) );
  return *aClassID ? GetClassIDNoAlloc(*aClassID) : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP 
sbRemoteLibrary::GetImplementationLanguage( PRUint32 *aImplementationLanguage )
{
  LOG(("sbRemoteLibrary::GetImplementationLanguage()"));
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

NS_IMETHODIMP 
sbRemoteLibrary::GetFlags( PRUint32 *aFlags )
{
  LOG(("sbRemoteLibrary::GetFlags()"));
  *aFlags = 0;
  return NS_OK;
}

NS_IMETHODIMP 
sbRemoteLibrary::GetClassIDNoAlloc( nsCID *aClassIDNoAlloc )
{
  LOG(("sbRemoteLibrary::GetClassIDNoAlloc()"));
  *aClassIDNoAlloc = kRemoteLibraryCID;
  return NS_OK;
}

