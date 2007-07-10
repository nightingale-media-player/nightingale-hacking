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
#include "sbRemoteMediaItem.h"
#include "sbRemoteMediaList.h"

#include <sbClassInfoUtils.h>
#include <sbILibrary.h>
#include <sbILibraryFactory.h>
#include <sbILibraryManager.h>
#include <sbIMediaList.h>
#include <sbIMediaListView.h>
#include <sbIMetadataJob.h>
#include <sbIMetadataJobManager.h>
#include <sbIPlaylistReader.h>
#include <sbLocalDatabaseCID.h>

#include <nsComponentManagerUtils.h>
#include <nsEventDispatcher.h>
#include <nsHashKeys.h>
#include <nsICategoryManager.h>
#include <nsIDocument.h>
#include <nsIDOMDocument.h>
#include <nsIDOMDocumentEvent.h>
#include <nsIDOMEvent.h>
#include <nsIDOMEventTarget.h>
#include <nsIDOMWindow.h>
#include <nsIFile.h>
#include <nsIMutableArray.h>
#include <nsIObserver.h>
#include <nsIPermissionManager.h>
#include <nsIPrefService.h>
#include <nsIPresShell.h>
#include <nsIProgrammingLanguage.h>
#include <nsIProperties.h>
#include <nsIScriptNameSpaceManager.h>
#include <nsIScriptSecurityManager.h>
#include <nsISupportsPrimitives.h>
#include <nsIURI.h>
#include <nsIURL.h>
#include <nsIWindowWatcher.h>
#include <nsIWritablePropertyBag2.h>
#include <nsMemory.h>
#include <nsNetUtil.h>
#include <nsServiceManagerUtils.h>
#include <nsStringEnumerator.h>
#include <nsStringGlue.h>
#include <nsTHashtable.h>
#include <prlog.h>
#include <prnetdb.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteLibrary:<5|4|3|2|1>
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gLibraryLog = nsnull;
#define LOG5(args) if (gLibraryLog) PR_LOG(gLibraryLog, 5, args)
#define LOG4(args) if (gLibraryLog) PR_LOG(gLibraryLog, 4, args)
#define LOG3(args) if (gLibraryLog) PR_LOG(gLibraryLog, 3, args)
#define LOG2(args) if (gLibraryLog) PR_LOG(gLibraryLog, 2, args)
#define LOG1(args) if (gLibraryLog) PR_LOG(gLibraryLog, 1, args)
#else
#define LOG5(args) /* nothing */
#define LOG4(args) /* nothing */
#define LOG3(args) /* nothing */
#define LOG2(args) /* nothing */
#define LOG1(args) /* nothing */
#endif

#define kNotFound -1
static NS_DEFINE_CID(kRemoteLibraryCID, SONGBIRD_REMOTELIBRARY_CID);

// Observer for the PlaylistReader that launches a metadata job when the
// playlist is loaded.
class sbRemoteMetadataScanLauncher : public nsIObserver
{
public:
  sbRemoteMetadataScanLauncher() {}
  virtual ~sbRemoteMetadataScanLauncher() {}

  NS_DECL_ISUPPORTS
  NS_IMETHODIMP Observe( nsISupports *aSubject,
                         const char *aTopic,
                         const PRUnichar *aData)
  {
    LOG1(( "sbRemoteMetadataScanLauncher::Observe(%s)", aTopic ));
    nsresult rv;
    nsCOMPtr<sbIMetadataJobManager> metaJobManager =
      do_GetService( "@songbirdnest.com/Songbird/MetadataJobManager;1", &rv );
    NS_ENSURE_SUCCESS(rv, rv);

    if(NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIMutableArray> mediaItems =
                                 do_CreateInstance( NS_ARRAY_CONTRACTID, &rv );
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<sbIMediaList> mediaList ( do_QueryInterface( aSubject, &rv ) );
      NS_ENSURE_SUCCESS(rv, rv);

      PRUint32 length;
      rv = mediaList->GetLength( &length );
      NS_ENSURE_SUCCESS(rv, rv);

      for ( PRUint32 index = 0; index < length; index++ ) {

        nsCOMPtr<sbIMediaItem> item;
        rv = mediaList->GetItemByIndex( index, getter_AddRefs ( item ) );
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mediaItems->AppendElement( item, PR_FALSE );
        NS_ENSURE_SUCCESS(rv, rv);
      }

      nsCOMPtr<sbIMetadataJob> job;
      rv = metaJobManager->NewJob( mediaItems, 50, getter_AddRefs(job) );
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return NS_OK;
  }
};
NS_IMPL_ISUPPORTS1( sbRemoteMetadataScanLauncher, nsIObserver )


const static char* sPublicWProperties[] =
  {
    "metadata:name",
    "binding:scanMediaOnCreation"
  };

const static char* sPublicRProperties[] =
  { //
    "metadata:artists",
    "metadata:albums",
    "metadata:name",
    "metadata:type",
    "metadata:length",

    // sbIRemoteLibrary
    "binding:selection",
    "binding:scanMediaOnCreation",
#ifdef DEBUG
    "library:filename",
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
    "library:connectToMediaLibrary",
    "library:connectToDefaultLibrary",
    "binding:createMediaList",
    "binding:createMediaListFromFile",
    "binding:createMediaItem",
    "binding:addMediaListByURL",

    // sbIRemoteMediaList
    "binding:addItemByURL",

    // sbIMediaList
    "binding:getItemByGuid",
    "binding:getItemByIndex",
    "binding:enumerateAllItems",
    "binding:enumerateItemsByProperty",
    "binding:indexOf",
    "binding:lastIndexOf",
    "binding:contains",
    "binding:add",
    "binding:addAll",
    "binding:addSome",
    "binding:remove",
    "binding:removeByIndex",
    "binding:getDistinctValuesForProperty",

    // sbILibraryResource
    "library:getProperty",
    "library:setProperty",
    "library:equals"
  };

NS_IMPL_ISUPPORTS9( sbRemoteLibrary,
                    nsIClassInfo,
                    nsISecurityCheckedComponent,
                    sbISecurityAggregator,
                    sbIRemoteMediaList,
                    sbIMediaList,
                    sbIWrappedMediaList,
                    sbIWrappedMediaItem,
                    sbIMediaItem,
                    sbIRemoteLibrary )

NS_IMPL_CI_INTERFACE_GETTER6( sbRemoteLibrary,
                              sbISecurityAggregator,
                              sbIRemoteLibrary,
                              sbIRemoteMediaList,
                              sbIMediaList,
                              sbIMediaItem,
                              nsISecurityCheckedComponent )

SB_IMPL_CLASSINFO( sbRemoteLibrary,
                   SONGBIRD_REMOTELIBRARY_CONTRACTID,
                   SONGBIRD_REMOTELIBRARY_CLASSNAME,
                   nsIProgrammingLanguage::CPLUSPLUS,
                   0,
                   kRemoteLibraryCID )

SB_IMPL_SECURITYCHECKEDCOMP_INIT(sbRemoteLibrary)

sbRemoteLibrary::sbRemoteLibrary() : mShouldScan(PR_TRUE)
{
#ifdef PR_LOGGING
  if (!gLibraryLog) {
    gLibraryLog = PR_NewLogModule("sbRemoteLibrary");
  }
  LOG1(("sbRemoteLibrary::sbRemoteLibrary()"));
#endif
}

sbRemoteLibrary::~sbRemoteLibrary()
{
  LOG1(("sbRemoteLibrary::~sbRemoteLibrary()"));
}

// ---------------------------------------------------------------------------
//
//                        sbIRemoteLibrary
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbRemoteLibrary::GetScanMediaOnCreation( PRBool *aShouldScan )
{
  NS_ENSURE_ARG_POINTER(aShouldScan);
  *aShouldScan = mShouldScan;
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteLibrary::SetScanMediaOnCreation( PRBool aShouldScan )
{
  mShouldScan = aShouldScan;
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteLibrary::GetFilename( nsAString &aFilename )
{
  LOG1(("sbRemoteLibrary::GetFilename()"));
#ifdef DEBUG
  aFilename.Assign(mFilename);
#endif
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteLibrary::ConnectToDefaultLibrary( const nsAString &aLibName )
{
  LOG1(( "sbRemoteLibrary::ConnectToDefaultLibrary(name:%s)",
         NS_LossyConvertUTF16toASCII(aLibName).get() ));

  nsAutoString guid;
  nsresult rv = GetLibraryGUID(aLibName, guid);
  if ( NS_SUCCEEDED(rv) ) {
    LOG4(( "sbRemoteLibrary::ConnectToDefaultLibrary(%s) -- IS a default library",
           NS_LossyConvertUTF16toASCII(guid).get() ));

    // See if the library manager has it lying around.
    nsCOMPtr<sbILibraryManager> libManager(
        do_GetService( "@songbirdnest.com/Songbird/library/Manager;1", &rv ) );
    NS_ENSURE_SUCCESS( rv, rv );

    rv = libManager->GetLibrary( guid, getter_AddRefs(mLibrary) );
    NS_ENSURE_SUCCESS( rv, rv );

    rv = InitInternalMediaList();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return rv;
}

NS_IMETHODIMP
sbRemoteLibrary::ConnectToMediaLibrary( const nsAString &aDomain, const nsAString &aPath )
{
  LOG1(( "sbRemoteLibrary::ConnectToMediaLibrary(domain:%s path:%s)",
         NS_LossyConvertUTF16toASCII(aDomain).get(),
         NS_LossyConvertUTF16toASCII(aPath).get() ));

  nsresult rv;
  nsCOMPtr<nsIFile> siteDBFile;
  rv = GetSiteLibraryFile( aDomain, aPath, getter_AddRefs(siteDBFile) );
  if ( NS_FAILED(rv) ) {
    LOG3(("sbRemoteLibrary::ConnectToMediaLibrary() - Failed to get site db file "));
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
  NS_ENSURE_SUCCESS(rv, rv);

  // Library has been created, store a pref key to point to the guid
  nsAutoString guid;
  rv = GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  // put the guid in a supports string for the pref system 
  nsCOMPtr<nsISupportsString> supportsString =
                         do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = supportsString->SetData(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrefBranch> prefService =
                                 do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

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

NS_IMETHODIMP
sbRemoteLibrary::CreateMediaItem(const nsAString& aURL,
                                 sbIMediaItem** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_STATE(mLibrary);

  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aURL);
  NS_ENSURE_SUCCESS(rv, rv);

  // Only allow the creation of media items with http(s) schemes
  PRBool validScheme;
  uri->SchemeIs("http", &validScheme);
  if (!validScheme) {
    uri->SchemeIs("https", &validScheme);
    if (!validScheme) {
      return NS_ERROR_INVALID_ARG;
    }
  }

  nsCOMPtr<sbIMediaItem> mediaItem;
  rv = mLibrary->CreateMediaItem(uri, nsnull, getter_AddRefs(mediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  if (mShouldScan) {
    nsCOMPtr<sbIMetadataJobManager> metaJobManager =
      do_GetService("@songbirdnest.com/Songbird/MetadataJobManager;1", &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to get MetadataJobManager!");

    if(NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIMutableArray> mediaItems = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mediaItems->AppendElement(mediaItem, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<sbIMetadataJob> job;
      rv = metaJobManager->NewJob(mediaItems, 50, getter_AddRefs(job));
      NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to create metadata job!");
    }
  }

  return SB_WrapMediaItem(mediaItem, _retval);
}

NS_IMETHODIMP
sbRemoteLibrary::CreateMediaList(const nsAString& aType,
                                 sbIRemoteMediaList** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_STATE(mLibrary);

  nsCOMPtr<sbIMediaList> mediaList;
  nsresult rv = mLibrary->CreateMediaList(aType,
                                          nsnull,
                                          getter_AddRefs(mediaList));
  NS_ENSURE_SUCCESS(rv, rv);

  return SB_WrapMediaList(mediaList, _retval);
}

NS_IMETHODIMP
sbRemoteLibrary::CreateMediaListFromFile( const nsAString& aURL,
                                          sbIRemoteMediaList** _retval )
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_STATE(mLibrary);

  nsCOMPtr<sbIMediaList> mediaList;
  nsresult rv = mLibrary->CreateMediaList( NS_LITERAL_STRING("simple"),
                                           nsnull,
                                           getter_AddRefs(mediaList) );
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPlaylistReaderManager> manager =
       do_CreateInstance( "@songbirdnest.com/Songbird/PlaylistReaderManager;1",
                          &rv );
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), aURL);
  NS_ENSURE_SUCCESS(rv, rv);
 
  // Only allow the creation of media lists with http(s) schemes
  PRBool validScheme;
  uri->SchemeIs("http", &validScheme);
  if (!validScheme) {
    uri->SchemeIs("https", &validScheme);
    if (!validScheme) {
      return NS_ERROR_INVALID_ARG;
    }
  }

  nsCOMPtr<sbIPlaylistReaderListener> lstnr =
      do_CreateInstance( "@songbirdnest.com/Songbird/PlaylistReaderListener;1",
                         &rv );
  NS_ENSURE_SUCCESS(rv, rv);

  if (mShouldScan) {
    nsRefPtr<sbRemoteMetadataScanLauncher> launcher =
                                            new sbRemoteMetadataScanLauncher();
    NS_ENSURE_TRUE( launcher, NS_ERROR_OUT_OF_MEMORY );

    nsCOMPtr<nsIObserver> observer( do_QueryInterface( launcher, &rv ) );
    NS_ENSURE_SUCCESS(rv, rv);

    rv = lstnr->SetObserver( observer );
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRInt32 dummy;
  rv = manager->LoadPlaylist( uri, mediaList, EmptyString(), true, lstnr, &dummy );
  NS_ENSURE_SUCCESS(rv, rv);

  return SB_WrapMediaList(mediaList, _retval);
}

// ---------------------------------------------------------------------------
//
//                            sbIWrappedMediaList
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP_(already_AddRefed<sbIMediaItem>)
sbRemoteLibrary::GetMediaItem()
{
  return mRemMediaList->GetMediaItem();
}

NS_IMETHODIMP_(already_AddRefed<sbIMediaList>)
sbRemoteLibrary::GetMediaList()
{
  return mRemMediaList->GetMediaList();
}

// ---------------------------------------------------------------------------
//
//                            Helper Methods
//
// ---------------------------------------------------------------------------

// create an sbRemoteMediaList object to delegate the calls for
// sbIMediaList, sbIMediaItem, sbIRemoteMedialist
nsresult
sbRemoteLibrary::InitInternalMediaList()
{
  NS_ENSURE_STATE(mLibrary);

  nsCOMPtr<sbIMediaList> mediaList = do_QueryInterface(mLibrary);
  NS_ENSURE_TRUE( mediaList, NS_ERROR_FAILURE );

  nsCOMPtr<sbIMediaListView> mediaListView;
  nsresult rv = mediaList->CreateView(getter_AddRefs(mediaListView));
  NS_ENSURE_SUCCESS(rv, rv);

  mRemMediaList = new sbRemoteMediaList( mediaList, mediaListView );
  NS_ENSURE_TRUE( mRemMediaList, NS_ERROR_OUT_OF_MEMORY );

  rv = mRemMediaList->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}

nsresult
sbRemoteLibrary::GetSiteLibraryFile( const nsAString &aDomain,
                                     const nsAString &aPath,
                                     nsIFile **aSiteDBFile )
{
  LOG1(( "sbRemoteLibrary::GetSiteLibraryFile(domain:%s path:%s)",
         NS_LossyConvertUTF16toASCII(aDomain).get(),
         NS_LossyConvertUTF16toASCII(aPath).get() ));
  NS_ENSURE_ARG_POINTER(aSiteDBFile);

  nsresult rv;
  nsCOMPtr<nsIURI> siteURI;
  rv = GetURI( getter_AddRefs(siteURI) );
  if ( NS_FAILED(rv) || !siteURI ) {
    // GetURI can return true even if there was no siteURI
    LOG2(( "sbRemoteLibrary::GetSiteLibraryFile() -- FAILED to get URI"));
    return rv;
  }

  nsAutoString domain(aDomain);
  rv = CheckDomain( domain, siteURI );
  if ( NS_FAILED(rv) ) {
    LOG2(( "sbRemoteLibrary::GetSiteLibraryFile() -- FAILED domain Check"));
    return rv;
  }

  nsAutoString path(aPath);
  rv = CheckPath( path, siteURI );
  if ( NS_FAILED(rv) ) {
    LOG2(( "sbRemoteLibrary::GetSiteLibraryFile() -- FAILED path Check"));
    return rv;
  }

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

  nsAutoString filename = domain;
  filename.Append(path);

  // hash it and smack the extension on
  nsAutoString hashedFile;
  hashedFile.AppendInt(HashString(filename));
  hashedFile.AppendLiteral(".db");
  LOG2(( "sbRemoteLibrary::GetSiteLibraryFile() -- hashed \n\t from: %s \n\t to: %s",
         NS_LossyConvertUTF16toASCII(filename).get(),
         NS_LossyConvertUTF16toASCII(hashedFile).get() ));

  // extend the path and add the file
  siteDBFile->Append( NS_LITERAL_STRING("db") );
  siteDBFile->Append(hashedFile);

  // Save the filename locally
  mFilename.Assign(hashedFile);

  NS_IF_ADDREF( *aSiteDBFile = siteDBFile );
  return NS_OK;
}

nsresult
sbRemoteLibrary::GetURI( nsIURI **aSiteURI )
{
  NS_ENSURE_ARG_POINTER(aSiteURI);
  LOG1(("sbRemoteLibrary::GetURI()"));

  nsresult rv;
  nsCOMPtr<sbISecurityMixin> mixin( do_QueryInterface( mSecurityMixin, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );

  nsCOMPtr<nsIURI> siteURI; 
  rv = mixin->GetCodebase( getter_AddRefs(siteURI) );
  NS_ENSURE_SUCCESS( rv, rv );

  NS_IF_ADDREF( *aSiteURI = siteURI );
  return NS_OK;
}

nsresult
sbRemoteLibrary::CheckDomain( nsAString &aDomain,  nsIURI *aSiteURI )
{
  NS_ENSURE_ARG_POINTER(aSiteURI);
  LOG1(( "sbRemoteLibrary::CheckDomain(%s)",
         NS_LossyConvertUTF16toASCII(aDomain).get() ));

  // get host from URI, remove leading/trailing dots, lowercase it
  nsCAutoString uriCHost;
  aSiteURI->GetHost(uriCHost);
  uriCHost.Trim(".");
  ToLowerCase(uriCHost);

  if ( !aDomain.IsEmpty() ) {
    LOG4(("sbRemoteLibrary::CheckDomain() -- Have a domain from the user"));
    // remove trailing dots - there is no ToLowerCase for AStrings
    aDomain.Trim(".");

    // Deal first with numerical ip addresses
    PRNetAddr addr;
    if ( PR_StringToNetAddr( uriCHost.get(), &addr ) == PR_SUCCESS ) {
      // numerical ip address
      LOG4(("sbRemoteLibrary::CheckDomain() -- Numerical Address "));
      if ( !aDomain.EqualsLiteral( uriCHost.get() ) ) {
        LOG3(("sbRemoteLibrary::CheckDomain() -- FAILED ip address check"));
        return NS_ERROR_FAILURE;
      }
    } else {
      // domain based host, check it against host from URI
      LOG4(("sbRemoteLibrary::CheckDomain() -- Domain based host "));

      // make sure the domain wasn't '.com' - it should have a dot in it
      PRInt32 dot = aDomain.FindChar('.');
      if ( dot == kNotFound ) {
        LOG3(("sbRemoteLibrary::CheckDomain() -- FAILED dot test "));
        return NS_ERROR_FAILURE;
      }

      // prepend a dot so bar.com doesn't match foobar.com but does foo.bar.com
      aDomain.Insert( NS_LITERAL_STRING("."), 0 );

      PRInt32 domainLength = aDomain.Length();
      PRInt32 lengthDiff = uriCHost.Length() - domainLength;
      if ( lengthDiff == 0 ) {
        LOG4(("sbRemoteLibrary::CheckDomain() -- same length check"));
        // same length better be the same strings
        if ( !aDomain.LowerCaseEqualsLiteral( uriCHost.get() ) ) {
          LOG3(("sbRemoteLibrary::CheckDomain() -- FAILED same length check"));
          return NS_ERROR_FAILURE;
        }
      } else if ( lengthDiff > 0 ) {
        LOG4(("sbRemoteLibrary::CheckDomain() -- parent domain check"));
        // normal case URI host is longer that host from user
        //   from user: .bar.com   from URI:  foo.bar.com
        nsCAutoString subHost( Substring( uriCHost, lengthDiff, domainLength ) );
        if ( !aDomain.EqualsLiteral( subHost.get() ) ) {
          LOG3(("sbRemoteLibrary::CheckDomain() -- FAILED parent domain check"));
          return NS_ERROR_FAILURE;
        }
      } else if ( lengthDiff == -1 ) {
        LOG4(("sbRemoteLibrary::CheckDomain() -- long domain check"));
        // special case:  from user: .bar.com   vs. from URI:  bar.com
        // XXXredfive - I actually think we'll see this most often because
        //             of the prepending of the dot to the user supplied domain
        if ( !Substring( aDomain, 1, domainLength - 1 )
                .LowerCaseEqualsLiteral( uriCHost.get() ) ) {
          LOG3(("sbRemoteLibrary::CheckDomain() -- FAILED long domain check"));
          return NS_ERROR_FAILURE;
        }
      } else {
        // domains are WAY off, the user domain is more than 1 char longer than
        // the URI domain. ie: user: jgaunt.com URI: ""
        LOG3(("sbRemoteLibrary::CheckDomain() -- FAILED, user domain is superset"));
        return NS_ERROR_FAILURE;
      }
     
      // remove the leading dot we added
      aDomain.Trim(".", PR_TRUE, PR_FALSE);
    }
  } else {
    LOG4(( "sbRemoteLibrary::CheckDomain() -- NO domain from the user"));
    // If the user didn't specify a host
    // if the URI host is empty, make sure we're file://
    if ( uriCHost.IsEmpty() ) {
      PRBool isFileURI = PR_FALSE;
      aSiteURI->SchemeIs("file", &isFileURI);
      if (!isFileURI) {
        // non-file URI without a host!!!
        LOG3(("sbRemoteLibrary::CheckDomain() -- FAILED file scheme check"));
        return NS_ERROR_FAILURE;
      }
    } else {
      // no domain from the user but there is a domain from the URI
      aDomain.Truncate();
      aDomain.AppendLiteral( uriCHost.get() );
    }
  }

  LOG2(("sbRemoteLibrary::CheckDomain() -- PASSED match test"));
  return NS_OK;
}

nsresult
sbRemoteLibrary::CheckPath( nsAString &aPath, nsIURI *aSiteURI )
{
  NS_ENSURE_ARG_POINTER(aSiteURI);
  LOG1(( "sbRemoteLibrary::CheckPath(%s)",
         NS_LossyConvertUTF16toASCII(aPath).get() ));

  nsCAutoString uriCPath;

  // if the user didn't specify a path, use the full path, minus the filename
  if ( aPath.IsEmpty() ) {
    nsCOMPtr<nsIURL> siteURL = do_QueryInterface(aSiteURI);
    if (siteURL) {
      LOG4(("sbRemoteLibrary::CheckPath() -- IS URL"));
      siteURL->GetDirectory(uriCPath);
    } else {
      LOG4(("sbRemoteLibrary::CheckPath() -- NOT URL"));
      aSiteURI->GetPath(uriCPath);
      PRInt32 slash = uriCPath.RFindChar('/');
      if (slash != kNotFound) {
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
  LOG4(("sbRemoteLibrary::CheckPath() -- Path Prefix Check"));
  if ( !StringBeginsWith( uriPath, aPath ) ) {
    LOG3(("sbRemoteLibrary::CheckPath() -- FAILED Path Prefix Check"));
    return NS_ERROR_FAILURE;
  }

  LOG2(( "sbRemoteLibrary::CheckPath( user: %s URI: %s ) PASSED Path Check",
         NS_LossyConvertUTF16toASCII(aPath).get(),
         NS_LossyConvertUTF16toASCII(uriPath).get() ));

  return NS_OK;
}

// If the libraryID is one of the default libraries we set the out param
// to the internal GUID of library as understood by the library manager
// and return NS_OK. If it is not one of the default libraries then
// return NS_ERROR_FAILURE. Also returns an error if the pref system is not
// available.
nsresult
sbRemoteLibrary::GetLibraryGUID( const nsAString &aLibraryID,
                                 nsAString &aLibraryGUID )
{
  LOG1(( "sbRemoteLibrary::GetLibraryGUID(%s)",
         NS_LossyConvertUTF16toASCII(aLibraryID).get() ));

  nsCAutoString prefKey;

  // match the 'magic' strings to the keys for the prefs
  if ( aLibraryID.EqualsLiteral("main") ) {
    prefKey.AssignLiteral("songbird.library.main");
  } else if ( aLibraryID.EqualsLiteral("web") ) {
    prefKey.AssignLiteral("songbird.library.web");
  }

  // right now just bail if it isn't a default
  if ( prefKey.IsEmpty() ) {
    LOG4(("sbRemoteLibrary::GetLibraryGUID() -- not a default library"));
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

