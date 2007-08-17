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

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteSiteLibrary:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gSiteLibraryLog = nsnull;
#endif

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
    "site:createMediaList",
    "site:createMediaListFromURL",
    "site:createMediaItem",
    "site:getMediaListByName",

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

NS_IMPL_CI_INTERFACE_GETTER7( sbRemoteSiteLibrary,
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

sbRemoteSiteLibrary::sbRemoteSiteLibrary()
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
  nsresult rv = mediaList->CreateView( getter_AddRefs(mediaListView) );
  NS_ENSURE_SUCCESS( rv, rv );

  mRemSiteMediaList = new sbRemoteSiteMediaList( mediaList, mediaListView );
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
  LOG(( "sbRemoteSiteLibrary::GetSiteLibraryFile(domain:%s path:%s)",
         aDomain.BeginReading(),
         aPath.BeginReading() ));

  nsCOMPtr<nsIURI> siteURI = GetURI();
  if (!siteURI) {
    LOG(("sbRemoteSiteLibrary::GetSiteLibraryFile() -- FAILED to get URI"));
    return nsnull;
  }

  nsCString domain(aDomain);
  nsresult rv = CheckDomain( domain, siteURI );
  if ( NS_FAILED(rv) ) {
    LOG(("sbRemoteSiteLibrary::GetSiteLibraryFile() -- FAILED domain Check"));
    return nsnull;
  }

  nsCString path(aPath);
  rv = CheckPath( path, siteURI );
  if ( NS_FAILED(rv) ) {
    LOG(("sbRemoteSiteLibrary::GetSiteLibraryFile() -- FAILED path Check"));
    return nsnull;
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

nsresult
sbRemoteSiteLibrary::CheckDomain( nsACString &aDomain, nsIURI *aSiteURI )
{
  NS_ENSURE_ARG_POINTER(aSiteURI);
  LOG(( "sbRemoteSiteLibrary::CheckDomain(%s)", aDomain.BeginReading() ));

  // get host from URI, remove leading/trailing dots, lowercase it
  nsCAutoString host;
  nsresult rv = aSiteURI->GetHost(host);
  NS_ENSURE_SUCCESS( rv, rv );

  host.Trim(".");
  ToLowerCase(host);

  if ( !aDomain.IsEmpty() ) {
    LOG(("sbRemoteSiteLibrary::CheckDomain() -- Have a domain from the user"));
    // remove trailing dots, lowercase it
    aDomain.Trim(".");
    ToLowerCase(aDomain);

    // Deal first with numerical ip addresses
    PRNetAddr addr;
    if ( PR_StringToNetAddr( host.get(), &addr ) == PR_SUCCESS ) {
      // numerical ip address
      LOG(("sbRemoteSiteLibrary::CheckDomain() -- Numerical Address "));
      if ( !aDomain.Equals(host) ) {
        LOG(("sbRemoteSiteLibrary::CheckDomain() -- FAILED ip address check"));
        return NS_ERROR_FAILURE;
      }
    } else {
      // domain based host, check it against host from URI
      LOG(("sbRemoteSiteLibrary::CheckDomain() -- Domain based host "));

      // make sure the domain wasn't '.com' - it should have a dot in it
      PRInt32 dot = aDomain.FindChar('.');
      if ( dot == kNotFound ) {
        LOG(("sbRemoteSiteLibrary::CheckDomain() -- FAILED dot test "));
        return NS_ERROR_FAILURE;
      }

      // prepend a dot so bar.com doesn't match foobar.com but does foo.bar.com
      aDomain.Insert( NS_LITERAL_CSTRING("."), 0 );

      PRInt32 domainLength = aDomain.Length();
      PRInt32 lengthDiff = host.Length() - domainLength;
      if ( lengthDiff == -1 ) {
        LOG(("sbRemoteSiteLibrary::CheckDomain() -- long domain check"));
        // special case:  from user: .bar.com   vs. from URI:  bar.com
        // XXXredfive - I actually think we'll see this most often because
        //             of the prepending of the dot to the user supplied domain
        if ( !Substring( aDomain, 1, domainLength - 1 ).Equals( host ) ) {
          LOG(("sbRemoteSiteLibrary::CheckDomain() -- FAILED long domain check"));
          return NS_ERROR_FAILURE;
        }
      } else if ( lengthDiff == 0 ) {
        LOG(("sbRemoteSiteLibrary::CheckDomain() -- same length check"));
        // same length better be the same strings
        if ( !aDomain.Equals(host) ) {
          LOG(("sbRemoteSiteLibrary::CheckDomain() -- FAILED same length check"));
          return NS_ERROR_FAILURE;
        }
      } else if ( lengthDiff > 0 ) {
        LOG(("sbRemoteSiteLibrary::CheckDomain() -- parent domain check"));
        // normal case URI host is longer that host from user
        //   from user: .bar.com   from URI:  foo.bar.com
        if ( !aDomain.Equals( Substring( host, lengthDiff, domainLength ) ) ) {
          LOG(("sbRemoteSiteLibrary::CheckDomain() -- FAILED parent domain check"));
          return NS_ERROR_FAILURE;
        }
      } else {
        // domains are WAY off, the user domain is more than 1 char longer than
        // the URI domain. ie: user: jgaunt.com URI: ""
        LOG(("sbRemoteSiteLibrary::CheckDomain() -- FAILED, user domain is superset"));
        return NS_ERROR_FAILURE;
      }

      // remove the leading dot we added
      aDomain.Cut( 0, 1 );
    }
  } else {
    LOG(( "sbRemoteSiteLibrary::CheckDomain() -- NO domain from the user"));
    // If the user didn't specify a host
    // if the URI host is empty, make sure we're file://
    if ( host.IsEmpty() ) {
      PRBool isFileURI;
      rv = aSiteURI->SchemeIs( "file", &isFileURI );
      NS_ENSURE_SUCCESS( rv, rv );

      if (!isFileURI) {
        // non-file URI without a host!!!
        LOG(("sbRemoteSiteLibrary::CheckDomain() -- FAILED file scheme check"));
        return NS_ERROR_FAILURE;
      }
    } else {
      // no domain from the user but there is a domain from the URI
      aDomain.Assign(host);
    }
  }

  LOG(("sbRemoteSiteLibrary::CheckDomain() -- PASSED match test"));
  return NS_OK;
}

nsresult
sbRemoteSiteLibrary::CheckPath( nsACString &aPath, nsIURI *aSiteURI )
{
  NS_ENSURE_ARG_POINTER(aSiteURI);
  LOG(( "sbRemoteSiteLibrary::CheckPath(%s)", aPath.BeginReading() ));

  nsresult rv;
  NS_NAMED_LITERAL_CSTRING( slashString, "/" );

  // Build a URI object to use for easy directory checking. This avoids manual
  // string parsing and works when aPath is empty and also when it is not.
  nsCOMPtr<nsIURI> uri;

  if ( aPath.IsEmpty() ) {
    // If the path was empty we use aSiteURI that was retrieved from the system.
    uri = aSiteURI;
  }
  else {
    // Construct a dummy URL that incorporates the path that the user passed in.
    nsCString dummyURL("http://www.dummy.com");

    // Make sure that aPath begins with a slash. Otherwise we could end up with
    // something like "foo.combar" rather than "foo.com/bar".
    if ( !StringBeginsWith( aPath, slashString ) ) {
      dummyURL.Append(slashString);
    }

    dummyURL.Append(aPath);

    rv = NS_NewURI( getter_AddRefs(uri), dummyURL );
    NS_ENSURE_SUCCESS( rv, rv );
  }

  // At this point uri contains the location we will use for the path. It will
  // be compared to aSiteURI later for validation if it was constructed from
  // aPath.

  nsCOMPtr<nsIURL> url( do_QueryInterface( uri, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );

  rv = url->GetDirectory(aPath);
  NS_ENSURE_SUCCESS( rv, rv );

  NS_ASSERTION( StringEndsWith( aPath, slashString ),
                "GetDirectory returned a string with no trailing slash!" );

  // According to nsIURL the directory will always have a trailing slash (???).
  // However, if the spec from aSiteURI did *not* have a trailing slash then the
  // fileName attribute of url may be set. If it is then we assume it is
  // actually a directory as long as the fileExtension is empty. Thus the
  // following transformations will result:
  //
  //    Original spec from aSiteURI -----> aPath
  //
  //    http://www.foo.com/bar/baz/ -----> /bar/baz/
  //    http://www.foo.com/bar/baz ------> /bar/baz
  //    http://www.foo.com/bar/baz.ext --> /bar/

  nsCString fileName;
  rv = url->GetFileName(fileName);
  NS_ENSURE_SUCCESS( rv, rv );

  if (!fileName.IsEmpty()) {
    nsCString fileExt;
    rv = url->GetFileExtension(fileExt);
    NS_ENSURE_SUCCESS( rv, rv );

    if (fileExt.IsEmpty()) {
      // If there is no file extension then assume it is actually a directory
      // and append a trailing slash.
      aPath.Append(fileName);
      aPath.Append(slashString);
    }
  }

  // The rest of this function will ensure that the path is actually a subpath
  // of aSiteURI. If nothing was passed in aPath then we already know that this
  // is true because we constructed aPath from aSiteURI. Therefore we're done
  // and can go ahead and return. We waited until this point to return early so
  // that we are sure that aPath both begins and ends with a slash (unless it is
  // only a single slash).
  if (uri == aSiteURI) {
    return NS_OK;
  }

  // Verify that this path is indeed part of the site URI.
  nsCString sitePath;
  rv = aSiteURI->GetPath(sitePath);
  NS_ENSURE_SUCCESS( rv, rv );

  // We know that aPath has a trailing slash but sitePath may not have one. Make
  // sure that we ignore the trailing slash in the following comparison.
  nsDependentCSubstring headPath( aPath, 0, aPath.Length() - 1 );

  if ( !StringBeginsWith( sitePath, headPath ) ) {
    LOG(("sbRemoteSiteLibrary::CheckPath() -- FAILED Path Prefix Check"));
    return NS_ERROR_FAILURE;
  }

  LOG(("sbRemoteSiteLibrary::CheckPath() -- PASSED Path Prefix Check"));
  return NS_OK;
}
