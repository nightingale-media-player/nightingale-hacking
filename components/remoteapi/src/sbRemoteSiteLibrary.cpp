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
#include <nsServiceManagerUtils.h>
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

#define LOG(args) PR_LOG(gSiteLibraryLog, 1, args)

static NS_DEFINE_CID(kRemoteSiteLibraryCID, SONGBIRD_REMOTESITELIBRARY_CID);

const static char* sPublicWProperties[] =
  {
    "site:name",
    "site:scanMediaOnCreation"
  };

const static char* sPublicRProperties[] =
  { //
    "site:artists",
    "site:albums",
    "site:name",
    "site:type",
    "site:length",

    // sbIRemoteLibrary
    "site:selection",
    "site:scanMediaOnCreation",
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
    "site:connectToMediaLibrary",
    "site:connectToDefaultLibrary",
    "site:createMediaList",
    "site:createMediaListFromURL",
    "site:createMediaItem",
    "site:addMediaListByURL",
    "site:getMediaList",

    // sbIRemoteMediaList
    "site:addItemByURL",

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

SB_IMPL_CLASSINFO( sbRemoteSiteLibrary,
                   SONGBIRD_REMOTESITELIBRARY_CONTRACTID,
                   SONGBIRD_REMOTESITELIBRARY_CLASSNAME,
                   nsIProgrammingLanguage::CPLUSPLUS,
                   0,
                   kRemoteSiteLibraryCID )

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
sbRemoteSiteLibrary::ConnectToSiteLibrary( const nsAString &aDomain,
                                           const nsAString &aPath )
{
  LOG(( "sbRemoteSiteLibrary::ConnectToSiteLibrary(domain:%s path:%s)",
        NS_LossyConvertUTF16toASCII(aDomain).get(),
        NS_LossyConvertUTF16toASCII(aPath).get() ));

  nsresult rv;
  nsCOMPtr<nsIFile> siteDBFile;
  rv = GetSiteLibraryFile( aDomain, aPath, getter_AddRefs(siteDBFile) );
  if ( NS_FAILED(rv) ) {
    LOG(("sbRemoteSiteLibrary::ConnectToSiteLibrary() - no site db file "));
    return rv;
  }

  // Get the library factory to create a new instance from the file
  nsCOMPtr<sbILibraryFactory> libFactory(
        do_CreateInstance( SB_LOCALDATABASE_LIBRARYFACTORY_CONTRACTID, &rv ) );
  NS_ENSURE_SUCCESS(rv, rv);

  // Get a propertybag to pass in the filename
  nsCOMPtr<nsIWritablePropertyBag2> propBag(
                do_CreateInstance( "@mozilla.org/hash-property-bag;1", &rv ) );
  NS_ENSURE_SUCCESS(rv, rv);
  propBag->SetPropertyAsInterface( NS_LITERAL_STRING("databaseFile"),
                                   siteDBFile );

  // Create the library
  libFactory->CreateLibrary( propBag, getter_AddRefs(mLibrary) );
  NS_ENSURE_STATE(mLibrary);

  rv = InitInternalMediaList();
  NS_ENSURE_SUCCESS( rv, rv );

  // Library has been created, store a pref key to point to the guid
  nsAutoString guid;
  rv = GetGuid(guid);
  NS_ENSURE_SUCCESS( rv, rv );

  // put the guid in a supports string for the pref system
  nsCOMPtr<nsISupportsString> supportsString =
                       do_CreateInstance( NS_SUPPORTS_STRING_CONTRACTID, &rv );
  NS_ENSURE_SUCCESS( rv, rv );
  rv = supportsString->SetData(guid);
  NS_ENSURE_SUCCESS( rv, rv );

  nsCOMPtr<nsIPrefBranch> prefService =
                               do_GetService( NS_PREFSERVICE_CONTRACTID, &rv );
  NS_ENSURE_SUCCESS( rv, rv );

  // the key includes the filename so we can get the guid back reliably
  nsAutoString key;
  key.AssignLiteral("songbird.library.site.");
  key.Append(mFilename);

  rv = prefService->SetComplexValue( NS_LossyConvertUTF16toASCII(key).get(),
                                     NS_GET_IID(nsISupportsString),
                                     supportsString );

  // Register the library with the library manager so it shows up in servicepane
  nsCOMPtr<sbILibraryManager> libManager(
        do_GetService( "@songbirdnest.com/Songbird/library/Manager;1", &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );

  rv = libManager->RegisterLibrary( mLibrary, false );
  NS_ASSERTION( NS_SUCCEEDED(rv), "Failed to register library" );

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

nsresult
sbRemoteSiteLibrary::GetSiteLibraryFile( const nsAString &aDomain,
                                         const nsAString &aPath,
                                         nsIFile **aSiteDBFile )
{
  LOG(( "sbRemoteSiteLibrary::GetSiteLibraryFile(domain:%s path:%s)",
         NS_LossyConvertUTF16toASCII(aDomain).get(),
         NS_LossyConvertUTF16toASCII(aPath).get() ));
  NS_ENSURE_ARG_POINTER(aSiteDBFile);

  nsCOMPtr<nsIURI> siteURI = GetURI();
  if (!siteURI) {
    LOG(("sbRemoteSiteLibrary::GetSiteLibraryFile() -- FAILED to get URI"));
    return NS_ERROR_FAILURE;
  }

  nsString domain(aDomain);
  nsresult rv = CheckDomain( domain, siteURI );
  if ( NS_FAILED(rv) ) {
    LOG(("sbRemoteSiteLibrary::GetSiteLibraryFile() -- FAILED domain Check"));
    return rv;
  }

  nsString path(aPath);
  rv = CheckPath( path, siteURI );
  if ( NS_FAILED(rv) ) {
    LOG(("sbRemoteSiteLibrary::GetSiteLibraryFile() -- FAILED path Check"));
    return rv;
  }

  // Do some character escaping
  nsCOMPtr<nsINetUtil> netUtil = do_GetService( NS_NETUTIL_CONTRACTID, &rv );
  NS_ENSURE_SUCCESS( rv, rv );

  nsCString escapedDomain;
  rv = netUtil->EscapeString( NS_ConvertUTF16toUTF8(domain),
                              nsINetUtil::ESCAPE_XALPHAS,
                              escapedDomain );
  NS_ENSURE_SUCCESS( rv, rv );

  nsCString escapedPath;
  rv = netUtil->EscapeString( NS_ConvertUTF16toUTF8(path),
                              nsINetUtil::ESCAPE_XALPHAS,
                              escapedPath );
  NS_ENSURE_SUCCESS( rv, rv );

  nsString filename = NS_ConvertUTF8toUTF16(escapedDomain);
  filename.Append( NS_ConvertUTF8toUTF16(escapedPath) );
  filename.AppendLiteral(".db");

  // get the directory service
  nsCOMPtr<nsIProperties> directoryService(
                       do_GetService( NS_DIRECTORY_SERVICE_CONTRACTID, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );

  // get the users profile directory
  nsCOMPtr<nsIFile> siteDBFile;
  rv = directoryService->Get( "ProfD",
                              NS_GET_IID(nsIFile),
                              getter_AddRefs(siteDBFile) );
  NS_ENSURE_SUCCESS( rv, rv );

  // extend the path and add the file
  siteDBFile->Append( NS_LITERAL_STRING("db") );
  siteDBFile->Append(filename);

  // Save the filename locally
  mFilename = filename;

  siteDBFile.swap(*aSiteDBFile);
  return NS_OK;
}

already_AddRefed<nsIURI>
sbRemoteSiteLibrary::GetURI()
{
  LOG(("sbRemoteSiteLibrary::GetURI()"));

  nsresult rv;
  nsCOMPtr<sbISecurityMixin> mixin( do_QueryInterface( mSecurityMixin, &rv ) );
  NS_ENSURE_SUCCESS( rv, nsnull );

  nsIURI* siteURI;
  rv = mixin->GetCodebase( &siteURI );
  NS_ENSURE_SUCCESS( rv, nsnull );

  return siteURI;
}

nsresult
sbRemoteSiteLibrary::CheckDomain( nsAString &aDomain, nsIURI *aSiteURI )
{
  NS_ENSURE_ARG_POINTER(aSiteURI);
  LOG(( "sbRemoteSiteLibrary::CheckDomain(%s)",
         NS_LossyConvertUTF16toASCII(aDomain).get() ));

  // get host from URI, remove leading/trailing dots, lowercase it
  nsCAutoString uriCHost;
  aSiteURI->GetHost(uriCHost);
  uriCHost.Trim(".");
  ToLowerCase(uriCHost);

  if ( !aDomain.IsEmpty() ) {
    LOG(("sbRemoteSiteLibrary::CheckDomain() -- Have a domain from the user"));
    // remove trailing dots - there is no ToLowerCase for AStrings
    aDomain.Trim(".");

    // Deal first with numerical ip addresses
    PRNetAddr addr;
    if ( PR_StringToNetAddr( uriCHost.get(), &addr ) == PR_SUCCESS ) {
      // numerical ip address
      LOG(("sbRemoteSiteLibrary::CheckDomain() -- Numerical Address "));
      if ( !aDomain.EqualsLiteral( uriCHost.get() ) ) {
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
      aDomain.Insert( NS_LITERAL_STRING("."), 0 );

      PRInt32 domainLength = aDomain.Length();
      PRInt32 lengthDiff = uriCHost.Length() - domainLength;
      if ( lengthDiff == 0 ) {
        LOG(("sbRemoteSiteLibrary::CheckDomain() -- same length check"));
        // same length better be the same strings
        if ( !aDomain.LowerCaseEqualsLiteral( uriCHost.get() ) ) {
          LOG(("sbRemoteSiteLibrary::CheckDomain() -- FAILED same length check"));
          return NS_ERROR_FAILURE;
        }
      } else if ( lengthDiff > 0 ) {
        LOG(("sbRemoteSiteLibrary::CheckDomain() -- parent domain check"));
        // normal case URI host is longer that host from user
        //   from user: .bar.com   from URI:  foo.bar.com
        nsCAutoString subHost( Substring( uriCHost, lengthDiff, domainLength ) );
        if ( !aDomain.EqualsLiteral( subHost.get() ) ) {
          LOG(("sbRemoteSiteLibrary::CheckDomain() -- FAILED parent domain check"));
          return NS_ERROR_FAILURE;
        }
      } else if ( lengthDiff == -1 ) {
        LOG(("sbRemoteSiteLibrary::CheckDomain() -- long domain check"));
        // special case:  from user: .bar.com   vs. from URI:  bar.com
        // XXXredfive - I actually think we'll see this most often because
        //             of the prepending of the dot to the user supplied domain
        if ( !Substring( aDomain, 1, domainLength - 1 )
                .LowerCaseEqualsLiteral( uriCHost.get() ) ) {
          LOG(("sbRemoteSiteLibrary::CheckDomain() -- FAILED long domain check"));
          return NS_ERROR_FAILURE;
        }
      } else {
        // domains are WAY off, the user domain is more than 1 char longer than
        // the URI domain. ie: user: jgaunt.com URI: ""
        LOG(("sbRemoteSiteLibrary::CheckDomain() -- FAILED, user domain is superset"));
        return NS_ERROR_FAILURE;
      }

      // remove the leading dot we added
      aDomain.Trim( ".", PR_TRUE, PR_FALSE );
    }
  } else {
    LOG(( "sbRemoteSiteLibrary::CheckDomain() -- NO domain from the user"));
    // If the user didn't specify a host
    // if the URI host is empty, make sure we're file://
    if ( uriCHost.IsEmpty() ) {
      PRBool isFileURI = PR_FALSE;
      aSiteURI->SchemeIs( "file", &isFileURI );
      if (!isFileURI) {
        // non-file URI without a host!!!
        LOG(("sbRemoteSiteLibrary::CheckDomain() -- FAILED file scheme check"));
        return NS_ERROR_FAILURE;
      }
    } else {
      // no domain from the user but there is a domain from the URI
      aDomain.Truncate();
      aDomain.AppendLiteral( uriCHost.get() );
    }
  }

  LOG(("sbRemoteSiteLibrary::CheckDomain() -- PASSED match test"));
  return NS_OK;
}

nsresult
sbRemoteSiteLibrary::CheckPath( nsAString &aPath, nsIURI *aSiteURI )
{
  NS_ENSURE_ARG_POINTER(aSiteURI);
  LOG(( "sbRemoteSiteLibrary::CheckPath(%s)",
         NS_LossyConvertUTF16toASCII(aPath).get() ));

  nsCAutoString uriCPath;

  // if the user didn't specify a path, use the full path, minus the filename
  if ( aPath.IsEmpty() ) {
    nsCOMPtr<nsIURL> siteURL = do_QueryInterface(aSiteURI);
    if (siteURL) {
      LOG(("sbRemoteSiteLibrary::CheckPath() -- IS URL"));
      siteURL->GetDirectory(uriCPath);
    } else {
      LOG(("sbRemoteSiteLibrary::CheckPath() -- NOT URL"));
      aSiteURI->GetPath(uriCPath);
      PRInt32 slash = uriCPath.RFindChar('/');
      if ( slash != kNotFound ) {
        // cut the filename from the path
        uriCPath.Cut( slash+1, uriCPath.Length()-slash );
      }
    }

    // path matches URI for sure, set it and return ok
    aPath = NS_ConvertUTF8toUTF16(uriCPath);
    return NS_OK;
  }

  // Get the path from the URI
  aSiteURI->GetPath(uriCPath);
  nsAutoString uriPath = NS_ConvertUTF8toUTF16(uriCPath);

  // Path passed in must be a prefix of the URI's path
  LOG(("sbRemoteSiteLibrary::CheckPath() -- Path Prefix Check"));
  if ( !StringBeginsWith( uriPath, aPath ) ) {
    LOG(("sbRemoteSiteLibrary::CheckPath() -- FAILED Path Prefix Check"));
    return NS_ERROR_FAILURE;
  }

  LOG(( "sbRemoteSiteLibrary::CheckPath( user: %s URI: %s ) PASSED Path Check",
         NS_LossyConvertUTF16toASCII(aPath).get(),
         NS_LossyConvertUTF16toASCII(uriPath).get() ));

  return NS_OK;
}
