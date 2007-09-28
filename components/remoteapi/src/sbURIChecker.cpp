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

#include "sbURIChecker.h"

#include <prio.h>
#include <prnetdb.h>
#include <nsCOMPtr.h>
#include <nsIURL.h>
#include <nsNetUtil.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbURIChecker:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gURICheckerLog = nsnull;
#endif

#undef LOG
#define LOG(args) PR_LOG(gURICheckerLog, PR_LOG_WARN, args)

/* static */
nsresult
sbURIChecker::CheckURI(nsACString& aDomain,
                       nsACString& aPath,
                       nsIURI* aURI)
{
#ifdef PR_LOGGING
  if (!gURICheckerLog) {
    gURICheckerLog = PR_NewLogModule("sbURIChecker");
  }
  LOG(("sbURIChecker::CheckURI(domain:%s path:%s)",
       aDomain.BeginReading(), aPath.BeginReading()));
#endif
  NS_ENSURE_ARG_POINTER(aURI);
  
  nsCString domain(aDomain);
  nsresult rv = CheckDomain( domain, aURI );
  if ( NS_FAILED(rv) ) {
    LOG(("sbURIChecker::CheckURI() -- FAILED domain Check"));
    return rv;
  }

  nsCString path(aPath);
  rv = CheckPath( path, aURI );
  if ( NS_FAILED(rv) ) {
    LOG(("sbURIChecker::CheckURI() -- FAILED path Check"));
    return rv;
  }

  // everything passed, assign into the inputs
  if ( aDomain.IsEmpty() ) {
    aDomain.Assign(domain);
  }
  if ( aPath.IsEmpty() ) {
    aPath.Assign(path);
  }
  return NS_OK;
}

/* static */
nsresult
sbURIChecker::CheckDomain( nsACString &aDomain, nsIURI *aSiteURI )
{
  NS_ENSURE_ARG_POINTER(aSiteURI);
  LOG(( "sbURIChecker::CheckDomain(%s)", aDomain.BeginReading() ));

  // get host from URI
  nsCString host;
  nsresult rv = aSiteURI->GetHost(host);
  NS_ENSURE_SUCCESS( rv, rv );

  nsCString fixedHost;
  rv = sbURIChecker::FixupDomain( host, fixedHost );
  NS_ENSURE_SUCCESS( rv, rv );

  host.Assign(fixedHost);

  if ( !aDomain.IsEmpty() ) {
    LOG(("sbURIChecker::CheckDomain() -- Have a domain from the user"));
    // remove trailing dots, lowercase it
    nsCString fixedDomain;
    rv = sbURIChecker::FixupDomain( aDomain, fixedDomain );
    NS_ENSURE_SUCCESS( rv, rv );

    aDomain.Assign(fixedDomain);

    // Deal first with numerical ip addresses
    PRNetAddr addr;
    if ( PR_StringToNetAddr( host.get(), &addr ) == PR_SUCCESS ) {
      // numerical ip address
      LOG(("sbURIChecker::CheckDomain() -- Numerical Address "));
      if ( !aDomain.Equals(host) ) {
        LOG(("sbURIChecker::CheckDomain() -- FAILED ip address check"));
        return NS_ERROR_FAILURE;
      }
    } else {
      // domain based host, check it against host from URI
      LOG(("sbURIChecker::CheckDomain() -- Domain based host "));

      // make sure the domain wasn't '.com' - it should have a dot in it
      PRInt32 dot = aDomain.FindChar('.');
      if ( dot < 0 ) {
        LOG(("sbURIChecker::CheckDomain() -- FAILED dot test "));
        return NS_ERROR_FAILURE;
      }

      // prepend a dot so bar.com doesn't match foobar.com but does foo.bar.com
      aDomain.Insert( NS_LITERAL_CSTRING("."), 0 );

      PRInt32 domainLength = aDomain.Length();
      PRInt32 lengthDiff = host.Length() - domainLength;
      if ( lengthDiff == -1 ) {
        LOG(("sbURIChecker::CheckDomain() -- long domain check"));
        // special case:  from user: .bar.com   vs. from URI:  bar.com
        // XXXredfive - I actually think we'll see this most often because
        //             of the prepending of the dot to the user supplied domain
        if ( !StringEndsWith(aDomain, host) ) {
          LOG(("sbURIChecker::CheckDomain() -- FAILED long domain check"));
          return NS_ERROR_FAILURE;
        }
      } else if ( lengthDiff == 0 ) {
        LOG(("sbURIChecker::CheckDomain() -- same length check"));
        // same length better be the same strings
        if ( !aDomain.Equals(host) ) {
          LOG(("sbURIChecker::CheckDomain() -- FAILED same length check"));
          return NS_ERROR_FAILURE;
        }
      } else if ( lengthDiff > 0 ) {
        LOG(("sbURIChecker::CheckDomain() -- parent domain check"));
        // normal case URI host is longer that host from user
        //   from user: .bar.com   from URI:  foo.bar.com
        if ( !StringEndsWith(host, aDomain) ) {
          LOG(("sbURIChecker::CheckDomain() -- FAILED parent domain check"));
          return NS_ERROR_FAILURE;
        }
      } else {
        // domains are WAY off, the user domain is more than 1 char longer than
        // the URI domain. ie: user: jgaunt.com URI: ""
        LOG(("sbURIChecker::CheckDomain() -- FAILED, user domain is superset"));
        return NS_ERROR_FAILURE;
      }

      // remove the leading dot we added
      aDomain.Cut( 0, 1 );
    }
  } else {
    LOG(( "sbURIChecker::CheckDomain() -- NO domain from the user"));
    // If the user didn't specify a host
    // if the URI host is empty, make sure we're file://
    if ( host.IsEmpty() ) {
      PRBool isFileURI;
      rv = aSiteURI->SchemeIs( "file", &isFileURI );
      NS_ENSURE_SUCCESS( rv, rv );

      if (!isFileURI) {
        // non-file URI without a host!!!
        LOG(("sbURIChecker::CheckDomain() -- FAILED file scheme check"));
        return NS_ERROR_FAILURE;
      }
      
      // clear the isVoid flag if set
      aDomain.Truncate();
    } else {
      // no domain from the user but there is a domain from the URI
      aDomain.Assign(host);
    }
  }

  LOG(("sbURIChecker::CheckDomain() -- PASSED match test"));
  return NS_OK;
}

/* static */
nsresult
sbURIChecker::CheckPath( nsACString &aPath, nsIURI *aSiteURI )
{
  // aPath may be empty
  NS_ENSURE_ARG_POINTER(aSiteURI);
  LOG(( "sbURIChecker::CheckPath(%s)", aPath.BeginReading() ));

  nsresult rv;

  nsCString fixedSitePath;
  rv = sbURIChecker::FixupPath( aSiteURI, fixedSitePath );
  NS_ENSURE_SUCCESS( rv, rv );

  if ( aPath.IsEmpty() ) {
    // If the path was empty we use aSiteURI that was retrieved from the system.
    aPath.Assign(fixedSitePath);

    // The rest of this function will ensure that the path is actually a subpath
    // of aURI. If nothing was passed in aPath then we already know that this
    // is true because we constructed aPath from aURI. Therefore we're done and
    // can go ahead and return.
    return NS_OK;
  }

  // Compare fixedPath to fixedSitePath to make sure that fixedPath is within
  // fixedSitePath.
  nsCString fixedPath;
  rv = sbURIChecker::FixupPath( aPath, fixedPath );
  NS_ENSURE_SUCCESS( rv, rv );

  // Verify that this path is indeed part of the site URI.
  if ( !StringBeginsWith( fixedSitePath, fixedPath ) ) {
    LOG(("sbURIChecker::CheckPath() -- FAILED Path Prefix Check"));
    return NS_ERROR_FAILURE;
  }

  LOG(("sbURIChecker::CheckPath() -- PASSED Path Prefix Check"));

  aPath.Assign(fixedPath);
  return NS_OK;
}

/* static */
nsresult
sbURIChecker::FixupDomain( const nsACString& aDomain,
                           nsACString& _retval )
{
  // aDomain may be empty
  if ( aDomain.IsEmpty() ) {
    _retval.Truncate();
    return NS_OK;
  }

  nsCString domain(aDomain);

  domain.Trim("./");
  ToLowerCase(domain);

  _retval.Assign(domain);
  return NS_OK;
}

/* static */
nsresult
sbURIChecker::FixupPath( nsIURI* aURI,
                         nsACString& _retval )
{
  NS_ASSERTION( aURI, "Don't you dare pass me a null pointer!" );

  nsresult rv;
  nsCOMPtr<nsIURL> url( do_QueryInterface( aURI, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );

  // According to nsIURL the directory will always have a trailing slash (???).
  // However, if the spec from aURI did *not* have a trailing slash then the
  // fileName attribute of url may be set. If it is then we assume it is
  // actually a directory as long as the fileExtension is empty. Thus the
  // following transformations will result:
  //
  //    Original spec from aURI ---------> path
  //
  //    http://www.foo.com/bar/baz/ -----> /bar/baz/
  //    http://www.foo.com/bar/baz ------> /bar/baz/
  //    http://www.foo.com/bar/baz.ext --> /bar/

  nsCString path;
  rv = url->GetDirectory(path);
  NS_ENSURE_SUCCESS( rv, rv );

  nsCString fileName;
  rv = url->GetFileName(fileName);
  NS_ENSURE_SUCCESS( rv, rv );

  if ( !fileName.IsEmpty() ) {
    nsCString fileExt;
    rv = url->GetFileExtension(fileExt);
    NS_ENSURE_SUCCESS( rv, rv );

    if ( fileExt.IsEmpty() ) {
      // If there is no file extension then assume it is actually a directory
      // and append a trailing slash.
      path.Append(fileName);
      path.AppendLiteral("/");
    }
  }

  _retval.Assign(path);
  return NS_OK;
}

/* static */
nsresult
sbURIChecker::FixupPath( const nsACString& aPath,
                               nsACString& _retval )
{
  // aPath may be empty
  if ( aPath.IsEmpty() ) {
    _retval.Truncate();
    return NS_OK;
  }

  NS_NAMED_LITERAL_CSTRING( slashString, "/" );

  // Construct a dummy URL that incorporates the path that the user passed in.
  nsCString dummyURL("http://dummy.com");

  // Make sure that aPath begins with a slash. Otherwise we could end up with
  // something like "foo.combar" rather than "foo.com/bar".
  if ( !StringBeginsWith( aPath, slashString ) ) {
    dummyURL.Append(slashString);
  }

  dummyURL.Append(aPath);

  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI( getter_AddRefs(uri), dummyURL );
  NS_ENSURE_SUCCESS( rv, rv );

  // Hand off.
  return sbURIChecker::FixupPath( uri, _retval );
}
