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

#include "sbScriptableFilterResult.h"

#include <sbClassInfoUtils.h>
#include <sbStandardProperties.h>

#include <nsAutoPtr.h>
#include <nsMemory.h>
#include <nsIClassInfoImpl.h>
#include <nsIStringEnumerator.h>
#include <nsStringGlue.h>

#include <jsapi.h>

#include <sbIFilterableMediaListView.h>
#include <sbILibraryConstraints.h>

#include "sbRemotePlayer.h"
#include "sbScriptableFilter.h"
#include "sbScriptableFilterItems.h"

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbScriptableFilterResult:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gScriptableFilterResultLog = nsnull;
#endif

#undef LOG
#define LOG(args) PR_LOG(gScriptableFilterResultLog, PR_LOG_WARN, args)
#define TRACE(args) PR_LOG(gScriptableFilterResultLog, PR_LOG_DEBUG, args)

NS_IMPL_ISUPPORTS3_CI( sbScriptableFilterResult,
                       sbIScriptableFilterResult,
                       nsISecurityCheckedComponent,
                       nsIXPCScriptable )

sbScriptableFilterResult::sbScriptableFilterResult( sbIFilterableMediaListView *aMediaListView,
                                                    sbRemotePlayer *aRemotePlayer )
  : mListView(aMediaListView),
    mPlayer(aRemotePlayer)
{
#ifdef PR_LOGGING
  if (!gScriptableFilterResultLog) {
    gScriptableFilterResultLog = PR_NewLogModule( "sbScriptableFilterResult" );
  }
#endif
  TRACE(("sbScriptableFilterResult::sbScriptableFilterResult()"));
}

sbScriptableFilterResult::~sbScriptableFilterResult()
{
}


// ---------------------------------------------------------------------------
//
//                          sbIScriptableFilterResult
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP sbScriptableFilterResult::GetArtists(nsIStringEnumerator * *aArtists)
{
  LOG(("sbScriptableFilterResult::GetArtists()"));
  
  nsRefPtr<sbScriptableFilter> filter =
    new sbScriptableFilter( mListView,
                            NS_LITERAL_STRING(SB_PROPERTY_ARTISTNAME),
                            mPlayer );
  NS_ENSURE_TRUE( filter, NS_ERROR_OUT_OF_MEMORY );
  
  NS_ADDREF( *aArtists = filter );
  return NS_OK;
}

NS_IMETHODIMP sbScriptableFilterResult::GetAlbums(nsIStringEnumerator * *aAlbums)
{
  LOG(("sbScriptableFilterResult::GetAlbums()"));
  
  nsRefPtr<sbScriptableFilter> filter =
    new sbScriptableFilter( mListView,
                            NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME),
                            mPlayer );
  NS_ENSURE_TRUE( filter, NS_ERROR_OUT_OF_MEMORY );
  
  NS_ADDREF( *aAlbums = filter );
  return NS_OK;
}

NS_IMETHODIMP sbScriptableFilterResult::GetGenres(nsIStringEnumerator * *aGenres)
{
  LOG(("sbScriptableFilterResult::GetGenres()"));
  
  nsRefPtr<sbScriptableFilter> filter =
    new sbScriptableFilter( mListView,
                            NS_LITERAL_STRING(SB_PROPERTY_GENRE),
                            mPlayer );
  NS_ENSURE_TRUE( filter, NS_ERROR_OUT_OF_MEMORY );
  
  NS_ADDREF( *aGenres = filter );
  return NS_OK;
}

NS_IMETHODIMP sbScriptableFilterResult::GetYears(nsIStringEnumerator * *aYears)
{
  LOG(("sbScriptableFilterResult::GetYears()"));
  
  nsRefPtr<sbScriptableFilter> filter =
    new sbScriptableFilter( mListView,
                            NS_LITERAL_STRING(SB_PROPERTY_YEAR),
                            mPlayer );
  NS_ENSURE_TRUE( filter, NS_ERROR_OUT_OF_MEMORY );
  
  NS_ADDREF( *aYears = filter );
  return NS_OK;
}

NS_IMETHODIMP sbScriptableFilterResult::GetItems(nsISupports * *aItems)
{
  NS_ENSURE_ARG_POINTER(aItems);
  nsRefPtr<sbScriptableFilterItems> items =
    new sbScriptableFilterItems( mListView, mPlayer );

  return CallQueryInterface( items.get(), aItems );
}

// ---------------------------------------------------------------------------
//
//                          nsIXPCScriptable
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP sbScriptableFilterResult::GetClassName(char * *aClassName)
{
  NS_ENSURE_ARG_POINTER(aClassName);
  
  NS_NAMED_LITERAL_CSTRING( kClassName, "sbScriptableFilterResult" );
  *aClassName = ToNewCString(kClassName);
  return aClassName ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

/* readonly attribute PRUint32 scriptableFlags; */
NS_IMETHODIMP sbScriptableFilterResult::GetScriptableFlags(PRUint32 *aScriptableFlags)
{
  NS_ENSURE_ARG_POINTER(aScriptableFlags);
  // XXX Mook: USE_JSSTUB_FOR_ADDPROPERTY is needed to define things on the
  //           prototype properly; even with it set scripts cannot add
  //           properties onto the object (because they're not allow to *set*)
  *aScriptableFlags = DONT_ENUM_STATIC_PROPS |
                      DONT_ENUM_QUERY_INTERFACE |
                      DONT_REFLECT_INTERFACE_NAMES |
                      CLASSINFO_INTERFACES_ONLY |
                      USE_JSSTUB_FOR_ADDPROPERTY |
                      WANT_EQUALITY;
  return NS_OK;
}

NS_IMETHODIMP sbScriptableFilterResult::Equality( nsIXPConnectWrappedNative *wrapper,
                                                  JSContext * cx,
                                                  JSObject * obj,
                                                  jsval val,
                                                  PRBool *_retval )
{
  LOG(("sbScriptableFilterResult::Equality()"));
  nsresult rv;
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_ARG_POINTER(obj);
  NS_ENSURE_ARG_POINTER(wrapper);
  
  *_retval = PR_FALSE;
  
  if ( !JSVAL_IS_OBJECT(val) ) {
    return NS_OK;
  }
  JSObject* otherObj = JSVAL_TO_OBJECT(val);
  
  nsCOMPtr<nsIXPConnect> xpc;
  rv = wrapper->GetXPConnect( getter_AddRefs(xpc) );
  NS_ENSURE_SUCCESS( rv, rv );
  
  nsCOMPtr<nsIXPConnectWrappedNative> otherWrapper;
  rv = xpc->GetWrappedNativeOfJSObject( cx, otherObj, getter_AddRefs(otherWrapper) );
  if ( NS_FAILED(rv) ) {
    // not a C++ XPCOM object
    return NS_OK;
  }
  
  nsCOMPtr<sbIScriptableFilterResult> otherIface;
  otherIface = do_QueryInterface( otherWrapper->Native(), &rv );
  if ( NS_FAILED(rv) ) {
    // not us
    return NS_OK;
  }
  
  nsCOMPtr<sbILibraryConstraint> constraint;
  rv = mListView->GetFilterConstraint( getter_AddRefs(constraint) );
  NS_ENSURE_SUCCESS( rv, rv );
  NS_ENSURE_STATE(constraint);
  
  nsCOMPtr<sbILibraryConstraint> otherConstraint;
  rv = otherIface->GetConstraint( getter_AddRefs(otherConstraint) );
  NS_ENSURE_SUCCESS( rv, rv );

  rv = constraint->Equals( otherConstraint, _retval );
  NS_ENSURE_SUCCESS( rv, rv );
  
  return NS_OK;
}

NS_IMETHODIMP sbScriptableFilterResult::GetConstraint(sbILibraryConstraint * *aConstraint)
{
  nsresult rv;
  rv = mListView->GetFilterConstraint(aConstraint);
  NS_ENSURE_SUCCESS( rv, rv );
  NS_ENSURE_STATE(aConstraint);
  return NS_OK;
}


// ---------------------------------------------------------------------------
//
//                          nsISecurityCheckedComponent
//
// ---------------------------------------------------------------------------

// note that this class is only obtainable by going through the security checks
// so we allow access for everything (other than writing) without looking at
// what the user is asking.

NS_IMETHODIMP sbScriptableFilterResult::CanCreateWrapper( const nsIID * iid,
                                                     char **_retval)
{
  TRACE(("sbScriptableFilterResult::CanCreateWrapper()"));

  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = ToNewCString( NS_LITERAL_CSTRING("AllAccess") );
  return NS_OK;
}

NS_IMETHODIMP sbScriptableFilterResult::CanCallMethod( const nsIID * iid,
                                                  const PRUnichar *methodName,
                                                  char **_retval)
{
  TRACE(("sbScriptableFilterResult::CanCallMethod() - %s",
         NS_LossyConvertUTF16toASCII(methodName).BeginReading()));

  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = ToNewCString( NS_LITERAL_CSTRING("AllAccess") );
  return NS_OK;
}

NS_IMETHODIMP sbScriptableFilterResult::CanGetProperty( const nsIID * iid,
                                                   const PRUnichar *propertyName,
                                                   char **_retval)
{
  TRACE(("sbScriptableFilterResult::CanGetProperty() - %s",
         NS_LossyConvertUTF16toASCII(propertyName).BeginReading()));

  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = ToNewCString( NS_LITERAL_CSTRING("AllAccess") );
  return NS_OK;
}

NS_IMETHODIMP sbScriptableFilterResult::CanSetProperty( const nsIID * iid,
                                                   const PRUnichar *propertyName,
                                                   char **_retval)
{
  TRACE(("sbScriptableFilterResult::CanSetProperty() - %s",
         NS_LossyConvertUTF16toASCII(propertyName).BeginReading()));

  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = ToNewCString( NS_LITERAL_CSTRING("NoAccess") );
  return NS_OK;
}

// ---------------------------------------------------------------------------
//
//                          nsIClassInfo
//
// ---------------------------------------------------------------------------
NS_DECL_CLASSINFO(sbScriptableFilterResult)

SB_IMPL_CLASSINFO_INTERFACES_ONLY(sbScriptableFilterResult)
