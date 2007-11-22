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
#include "sbRemoteSiteLibrary.h"

#include <nsINetUtil.h>
#include <nsIPrefService.h>
#include <nsIProgrammingLanguage.h>
#include <nsIProperties.h>
#include <nsISupportsPrimitives.h>
#include <nsIURI.h>
#include <nsIURL.h>
#include <nsIWritablePropertyBag2.h>
#include <sbILibraryFactory.h>
#include <sbILibraryManager.h>

#include <nsComponentManagerUtils.h>
#include <nsHashKeys.h>
#include <nsMemory.h>
#include <nsNetCID.h>
#include <nsNetUtil.h>
#include <nsServiceManagerUtils.h>
#include <nsXPCOMCIDInternal.h>
#include <prlog.h>
#include <prnetdb.h>
#include <sbClassInfoUtils.h>
#include <sbLocalDatabaseCID.h>
#include <sbStandardProperties.h>
#include "sbURIChecker.h"

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteSiteLibrary:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gSiteLibraryLog = nsnull;
#endif

#undef LOG
#define LOG(args) PR_LOG(gSiteLibraryLog, PR_LOG_WARN, args)

const static char* sPublicWProperties[] =
  { // sbIMediaList
    "site:name",

    // sbIRemoteLibrary
    "site:scanMediaOnCreation"
  };

const static char* sPublicRProperties[] =
  { // sbIMediaList
    "site:name",
    "site:type",
    "site:length",

    // sbIRemoteLibrary
    "site:scanMediaOnCreation",
    "site:playlists",

    // sbIScriptableFilterResult
    "site:artists",
    "site:albums",
    "site:genres",
    "site:years",

    // sbIRemoteSiteLibrary
#ifdef DEBUG
    "site:filename",
#endif

    // nsIClassInfo
    "classinfo:classDescription",
    "classinfo:contractID",
    "classinfo:classID",
    "classinfo:implementationLanguage",
    "classinfo:flags"
  };

const static char* sPublicMethods[] =
  { // sbIRemoteLibrary
    "site:createSimpleMediaList",
    "site:createMediaListFromURL",
    "site:createMediaItem",
    "site:getMediaListBySiteID",
    "site:getPlaylists",

    // sbIScriptableFilterResult
    "site:getArtists",
    "site:getAlbums",
    "site:getYears",
    "site:getGenres",

    // sbIMediaList
    "site:getItemByGuid",
    "site:getItemByIndex",
    "site:enumerateAllItems",
    "site:enumerateItemsByProperty",
    "site:indexOf",
    "site:lastIndexOf",
    "site:contains",
    "site:add",
    "site:addAll",
    "site:addSome",
    "site:clear",
    "site:remove",
    "site:removeByIndex",
    "site:getDistinctValuesForProperty",

    // sbILibraryResource
    "site:getProperty",
    "site:setProperty",
    "site:equals"
  };

NS_IMPL_ISUPPORTS_INHERITED2( sbRemoteSiteLibrary,
                              sbRemoteLibraryBase,
                              sbIRemoteSiteLibrary,
                              nsIClassInfo )

NS_IMPL_CI_INTERFACE_GETTER8( sbRemoteSiteLibrary,
                              sbIScriptableFilterResult,
                              sbISecurityAggregator,
                              sbIRemoteLibrary,
                              sbIRemoteSiteLibrary,
                              sbIRemoteMediaList,
                              sbIMediaList,
                              sbIMediaItem,
                              nsISecurityCheckedComponent )

SB_IMPL_CLASSINFO_INTERFACES_ONLY(sbRemoteSiteLibrary)

SB_IMPL_SECURITYCHECKEDCOMP_INIT(sbRemoteSiteLibrary)

#define kNotFound -1

sbRemoteSiteLibrary::sbRemoteSiteLibrary(sbRemotePlayer* aRemotePlayer) :
  sbRemoteLibraryBase(aRemotePlayer)
{
#ifdef PR_LOGGING
  if (!gSiteLibraryLog) {
    gSiteLibraryLog = PR_NewLogModule("sbRemoteSiteLibrary");
  }
  LOG(("sbRemoteSiteLibrary::sbRemoteSiteLibrary()"));
#endif
}

sbRemoteSiteLibrary::~sbRemoteSiteLibrary()
{
  LOG(("sbRemoteSiteLibrary::~sbRemoteSiteLibrary()"));
}


// ----------------------------------------------------------------------------
//
//                              sbIRemoteSiteLibrary
//
// ----------------------------------------------------------------------------

NS_IMETHODIMP
sbRemoteSiteLibrary::GetFilename( nsAString &aFilename )
{
  LOG(("sbRemoteSiteLibrary::GetFilename()"));
#ifdef DEBUG
  aFilename.Assign(mFilename);
#endif
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteSiteLibrary::ConnectToSiteLibrary( const nsACString &aDomain,
                                           const nsACString &aPath )
{
  // aDomain and aPath may be empty
  LOG(( "sbRemoteSiteLibrary::ConnectToSiteLibrary(domain:%s path:%s)",
        aDomain.BeginReading(), aPath.BeginReading() ));

  nsCOMPtr<nsIFile> siteDBFile = GetSiteLibraryFile( aDomain, aPath );
  if (!siteDBFile) {
    LOG(("sbRemoteSiteLibrary::ConnectToSiteLibrary() - no site db file "));
    return NS_ERROR_FAILURE;
  }

  nsresult rv;

#ifdef DEBUG
  rv = siteDBFile->GetLeafName(mFilename);
  NS_ENSURE_SUCCESS( rv, rv );
#endif

  // Get the library factory to create a new instance from the file
  nsCOMPtr<sbILibraryFactory> libFactory(
    do_CreateInstance( SB_LOCALDATABASE_LIBRARYFACTORY_CONTRACTID, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );

  // Get a propertybag to pass in the filename
  nsCOMPtr<nsIWritablePropertyBag2> propBag(
    do_CreateInstance( NS_HASH_PROPERTY_BAG_CONTRACTID, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );

  rv = propBag->SetPropertyAsInterface( NS_LITERAL_STRING("databaseFile"),
                                        siteDBFile );
  NS_ENSURE_SUCCESS( rv, rv );

  // Create the library
  rv = libFactory->CreateLibrary( propBag, getter_AddRefs(mLibrary) );
  NS_ENSURE_SUCCESS( rv, rv );

  rv = mLibrary->SetProperty( NS_LITERAL_STRING( SB_PROPERTY_HIDDEN ),
                              NS_LITERAL_STRING( "1" ) );
  NS_ENSURE_SUCCESS( rv, rv );

  // Register the library
  nsCOMPtr<sbILibraryManager> libraryManager =
    do_GetService( "@songbirdnest.com/Songbird/library/Manager;1", &rv );
  NS_ENSURE_SUCCESS( rv, rv );

  PRBool hasLibrary;
  rv = libraryManager->HasLibrary( mLibrary, &hasLibrary );
  NS_ENSURE_SUCCESS( rv, rv );

  if (!hasLibrary) {
    rv = libraryManager->RegisterLibrary( mLibrary, PR_FALSE );
    NS_ENSURE_SUCCESS( rv, rv );
  }

  rv = InitInternalMediaList();
  NS_ENSURE_SUCCESS( rv, rv );

  return NS_OK;
}

// ----------------------------------------------------------------------------
//
//                              Helper Methods
//
// ----------------------------------------------------------------------------

nsresult
sbRemoteSiteLibrary::InitInternalMediaList()
{
  LOG(("sbRemoteSiteLibrary::InitInternalMediaList()"));
  NS_ENSURE_STATE(mLibrary);

  nsCOMPtr<sbIMediaList> mediaList = do_QueryInterface(mLibrary);
  NS_ENSURE_TRUE( mediaList, NS_ERROR_FAILURE );

  nsCOMPtr<sbIMediaListView> mediaListView;
  nsresult rv = mediaList->CreateView( nsnull, getter_AddRefs(mediaListView) );
  NS_ENSURE_SUCCESS( rv, rv );

  mRemSiteMediaList = new sbRemoteSiteMediaList( mRemotePlayer,
                                                 mediaList,
                                                 mediaListView );
  NS_ENSURE_TRUE( mRemSiteMediaList, NS_ERROR_OUT_OF_MEMORY );

  rv = mRemSiteMediaList->Init();
  NS_ENSURE_SUCCESS( rv, rv );

  // base class uses the base list to forward interfaces
  mRemMediaList = mRemSiteMediaList;

  return rv;
}

already_AddRefed<nsIFile>
sbRemoteSiteLibrary::GetSiteLibraryFile( const nsACString &aDomain,
                                         const nsACString &aPath )
{
  // aPath and aDomain may be empty
  LOG(( "sbRemoteSiteLibrary::GetSiteLibraryFile(domain:%s path:%s)",
         aDomain.BeginReading(),
         aPath.BeginReading() ));

  nsCOMPtr<nsIURI> siteURI = GetURI();
  if (!siteURI) {
    LOG(("sbRemoteSiteLibrary::GetSiteLibraryFile() -- FAILED to get URI"));
    return nsnull;
  }

  nsCString domain(aDomain);
  nsCString path(aPath);
  nsresult rv = sbURIChecker::CheckURI(domain, path, siteURI);
  if ( NS_FAILED(rv) ) {
    // this should not be possible, sbRemotePlayer::SetSiteScope() should
    // have done the exact same check already
    LOG(("sbRemoteSiteLibrary::GetSiteLibraryFile() -- FAILED URI Check"));
    return nsnull;
  }

  nsString filename;
  rv = GetFilenameForSiteLibraryInternal( domain, path, PR_FALSE, filename );
  NS_ENSURE_SUCCESS( rv, nsnull );

  // get the directory service
  nsCOMPtr<nsIProperties> directoryService(
    do_GetService( NS_DIRECTORY_SERVICE_CONTRACTID, &rv ) );
  NS_ENSURE_SUCCESS( rv, nsnull );

  // get the users profile directory
  nsCOMPtr<nsIFile> siteDBFile;
  rv = directoryService->Get( "ProfD", NS_GET_IID(nsIFile),
                              getter_AddRefs(siteDBFile) );
  NS_ENSURE_SUCCESS( rv, nsnull );

  // extend the path and add the file
  siteDBFile->Append( NS_LITERAL_STRING("db") );
  siteDBFile->Append(filename);

  nsIFile* outFile = nsnull;
  siteDBFile.swap(outFile);

  return outFile;
}

already_AddRefed<nsIURI>
sbRemoteSiteLibrary::GetURI()
{
  LOG(("sbRemoteSiteLibrary::GetURI()"));

  nsresult rv;
  nsCOMPtr<sbISecurityMixin> mixin( do_QueryInterface( mSecurityMixin, &rv ) );
  NS_ENSURE_SUCCESS( rv, nsnull );

  // GetCodebase AddRef's for us here.
  nsIURI* siteURI;
  rv = mixin->GetCodebase( &siteURI );
  NS_ENSURE_SUCCESS( rv, nsnull );

  return siteURI;
}

/* static */
nsresult
sbRemoteSiteLibrary::GetFilenameForSiteLibraryInternal( const nsACString& aDomain,
                                                        const nsACString& aPath,
                                                        PRBool aDoFixup,
                                                        nsAString& _retval )
{
  // aDomain and aPath may be empty

  nsresult rv;

  nsCString domain, path;
  if (aDoFixup) {
    // External callers will need to have these arguments validated.
    rv = sbURIChecker::FixupDomain( aDomain, domain );
    NS_ENSURE_SUCCESS( rv, rv );

    rv = sbURIChecker::FixupPath( aPath, path );
    NS_ENSURE_SUCCESS( rv, rv );
  }
  else {
    // Internal callers should have already performed a more rigorous validation
    // of the arguments so we don't need to do anything special here.
    domain.Assign(aDomain);
    path.Assign(aPath);
  }

  // Do some character escaping
  nsCOMPtr<nsINetUtil> netUtil = do_GetService( NS_NETUTIL_CONTRACTID, &rv );
  NS_ENSURE_SUCCESS( rv, nsnull );

  nsCString escapedDomain;
  rv = netUtil->EscapeString( domain, nsINetUtil::ESCAPE_XALPHAS,
                              escapedDomain );
  NS_ENSURE_SUCCESS( rv, nsnull );

  nsCString escapedPath;
  rv = netUtil->EscapeString( path, nsINetUtil::ESCAPE_XALPHAS,
                              escapedPath );
  NS_ENSURE_SUCCESS( rv, nsnull );

  nsString filename = NS_ConvertUTF8toUTF16(escapedDomain);
  filename.Append( NS_ConvertUTF8toUTF16(escapedPath) );
  filename.AppendLiteral(".db");

  _retval.Assign(filename);
  return NS_OK;
}
