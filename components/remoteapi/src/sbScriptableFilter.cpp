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

#include "sbScriptableFilter.h"

#include <sbClassInfoUtils.h>

#include <nsAutoPtr.h>
#include <nsMemory.h>
#include <nsIClassInfoImpl.h>
#include <nsIStringEnumerator.h>
#include <nsIXPConnect.h>
#include <nsStringGlue.h>

#include <jsapi.h>

#include <sbIFilterableMediaListView.h>
#include <sbILibraryConstraints.h>
#include <sbIScriptableFilterResult.h>
#include <sbIMediaListView.h>
#include <sbIPropertyArray.h>

#include "sbRemotePlayer.h"
#include "sbScriptableFilterResult.h"

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbScriptableFilter:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gScriptableFilterLog = nsnull;
#endif

#undef LOG
#define LOG(args) PR_LOG(gScriptableFilterLog, PR_LOG_WARN, args)
#define TRACE(args) PR_LOG(gScriptableFilterLog, PR_LOG_DEBUG, args)

NS_IMPL_ISUPPORTS3_CI( sbScriptableFilter,
                       nsISecurityCheckedComponent,
                       nsIXPCScriptable,
                       nsIStringEnumerator )

sbScriptableFilter::sbScriptableFilter( sbIFilterableMediaListView *aMediaListView,
                                        const nsAString &aPropertyName,
                                        sbRemotePlayer *aPlayer)
  : mListView(aMediaListView),
    mPropertyName(aPropertyName),
    mEnumeratorIndex(-1),
    mHasProps(PR_FALSE),
    mPlayer(aPlayer)
{
#ifdef PR_LOGGING
  if (!gScriptableFilterLog) {
    gScriptableFilterLog = PR_NewLogModule( "sbScriptableFilter" );
  }
#endif
  TRACE(("sbScriptableFilter::sbScriptableFilter()"));
}

sbScriptableFilter::~sbScriptableFilter()
{
}

nsresult
sbScriptableFilter::ReadEnumerator()
{
  if ( mEnumeratorIndex != -1 ) {
    // already read
    return NS_OK;
  }
  
  if (!mListView) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  
  nsresult rv;
  
  nsCOMPtr<sbIMediaListView> view = do_QueryInterface( mListView, &rv );
  NS_ENSURE_SUCCESS( rv, rv );
  
  nsCOMPtr<nsIStringEnumerator> enumerator;
  rv = view->GetDistinctValuesForProperty( mPropertyName, getter_AddRefs(enumerator) );
  NS_ENSURE_SUCCESS( rv, rv );
  
  while (true) {
    PRBool hasMore;

    rv = enumerator->HasMore(&hasMore);
    NS_ENSURE_SUCCESS( rv, rv );
    if (!hasMore) {
      break;
    }

    nsString value;
    rv = enumerator->GetNext(value);
    NS_ENSURE_SUCCESS( rv, rv );
    
    mStrings.AppendString(value);
  }
  
  // mark that we have finished reading the enumerator, and that the next item
  // we should enumerate would be the first item.
  mEnumeratorIndex = 0;
  return NS_OK;
}

// ---------------------------------------------------------------------------
//
//                          nsIStringEnumerator
//
// ---------------------------------------------------------------------------


/* boolean hasMore (); */
NS_IMETHODIMP sbScriptableFilter::HasMore(PRBool *_retval)
{
  TRACE(("sbScriptableFilter::HasMore()"));
  NS_ENSURE_ARG_POINTER(_retval);
  
  nsresult rv = ReadEnumerator();
  NS_ENSURE_SUCCESS( rv, rv );
  
  *_retval = ( mEnumeratorIndex < mStrings.Count() );

  TRACE(("sbScriptableFilter::HasMore() - got %s",
         *_retval ? "YES" : "no"));

  return NS_OK;
}

/* AString getNext (); */
NS_IMETHODIMP sbScriptableFilter::GetNext(nsAString & _retval)
{
  TRACE(("sbScriptableFilter::GetNext()"));
  nsresult rv = ReadEnumerator();
  NS_ENSURE_SUCCESS( rv, rv );
  
  if ( !( mEnumeratorIndex < mStrings.Count() ) ) {
    return NS_ERROR_FAILURE;
  }
  
  mStrings.StringAt( mEnumeratorIndex, _retval );
  ++mEnumeratorIndex;
  
  TRACE(("sbScriptableFilter::GetNext() - got %s",
         NS_LossyConvertUTF16toASCII(_retval).BeginReading()));
  
  return NS_OK;
}


// ---------------------------------------------------------------------------
//
//                          nsIXPCScriptable
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP sbScriptableFilter::GetClassName(char * *aClassName)
{
  NS_ENSURE_ARG_POINTER(aClassName);
  
  NS_NAMED_LITERAL_CSTRING( kClassName, "sbScriptableFilter" );
  *aClassName = ToNewCString(kClassName);
  return aClassName ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

/* readonly attribute PRUint32 scriptableFlags; */
NS_IMETHODIMP sbScriptableFilter::GetScriptableFlags(PRUint32 *aScriptableFlags)
{
  NS_ENSURE_ARG_POINTER(aScriptableFlags);
  // XXX Mook: USE_JSSTUB_FOR_ADDPROPERTY is needed to define things on the
  //           prototype properly; even with it set scripts cannot add
  //           properties onto the object (because they're not allow to *set*)
  *aScriptableFlags = USE_JSSTUB_FOR_ADDPROPERTY |
                      DONT_ENUM_STATIC_PROPS |
                      DONT_ENUM_QUERY_INTERFACE |
                      WANT_GETPROPERTY |
                      WANT_NEWENUMERATE |
                      WANT_NEWRESOLVE |
                      ALLOW_PROP_MODS_DURING_RESOLVE |
                      DONT_REFLECT_INTERFACE_NAMES ;
  return NS_OK;
}

NS_IMETHODIMP sbScriptableFilter::GetProperty( nsIXPConnectWrappedNative *wrapper,
                                               JSContext * cx,
                                               JSObject * obj,
                                               jsval id,
                                               jsval * vp,
                                               PRBool *_retval)
{
  TRACE(("sbScriptableFilter::GetProperty()"));
  NS_ENSURE_ARG_POINTER(_retval);

  JSString* jsstr = JS_ValueToString( cx, id );
  if (!jsstr) {
    return NS_OK;
  }

  nsresult rv = ReadEnumerator();
  NS_ENSURE_SUCCESS( rv, rv );

  *_retval = PR_TRUE;
  
  nsDependentString jsid( (PRUnichar *)::JS_GetStringChars(jsstr),
                          ::JS_GetStringLength(jsstr));
  TRACE(( "  getting property %s", NS_LossyConvertUTF16toASCII(jsid).get() ));

  PRInt32 length = mStrings.Count();
  for (PRInt32 i = 0; i < length; ++i) {
    if ( mStrings[i]->Equals(jsid) ) {

      // make a clone of the filter view...
      nsCOMPtr<sbIMediaListView> view = do_QueryInterface( mListView, &rv );
      NS_ENSURE_SUCCESS( rv, rv );
      nsCOMPtr<sbIMediaListView> clonedView;
      rv = view->Clone( getter_AddRefs(clonedView) );
      NS_ENSURE_SUCCESS( rv, rv );
      nsCOMPtr<sbIFilterableMediaListView> filterView =
        do_QueryInterface( clonedView, &rv );
      NS_ENSURE_SUCCESS( rv, rv );
      
      // get the old constraints
      nsCOMPtr<sbILibraryConstraint> constraint;
      rv = filterView->GetFilterConstraint( getter_AddRefs(constraint) );
      NS_ENSURE_SUCCESS( rv, rv );

      nsCOMPtr<sbILibraryConstraintBuilder> builder =
        do_CreateInstance( "@songbirdnest.com/Songbird/Library/ConstraintBuilder;1",
                           &rv );
      NS_ENSURE_SUCCESS( rv, rv );
      
      // push the selected property into it if there's an existing one
      if (constraint) {
        rv = builder->IncludeConstraint( constraint, nsnull );
        NS_ENSURE_SUCCESS( rv, rv );
        rv = builder->Intersect(nsnull);
        NS_ENSURE_SUCCESS( rv, rv );
      }
      
      rv = builder->Include( mPropertyName, jsid, nsnull );
      NS_ENSURE_SUCCESS( rv, rv );
      rv = builder->Get( getter_AddRefs(constraint) );
      NS_ENSURE_SUCCESS( rv, rv );
      rv = filterView->SetFilterConstraint( constraint );
      NS_ENSURE_SUCCESS( rv, rv );

      // make a filter result
      nsCOMPtr<sbIScriptableFilterResult> result =
        new sbScriptableFilterResult( filterView, mPlayer );
      if (!result) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      
      nsCOMPtr<nsIXPConnect> xpc;
      rv = wrapper->GetXPConnect( getter_AddRefs(xpc) );
      NS_ENSURE_SUCCESS( rv, rv );
      
      nsCOMPtr<nsIXPConnectJSObjectHolder> objHolder;
      
      rv = xpc->WrapNative( cx,
                            obj,
                            result,
                            sbIScriptableFilterResult::GetIID(),
                            getter_AddRefs(objHolder) );
      NS_ENSURE_SUCCESS( rv, rv );
      
      JSObject* object = nsnull;
      rv = objHolder->GetJSObject( &object );
      NS_ENSURE_SUCCESS( rv, rv );

      *vp = OBJECT_TO_JSVAL(object);
      return NS_SUCCESS_I_DID_SOMETHING;
    }
  }

  // check for "length"
  if ( jsid.EqualsLiteral("length") ) {
    *vp = INT_TO_JSVAL( mStrings.Count() );
    return NS_SUCCESS_I_DID_SOMETHING;
  }

  return NS_OK;
}

NS_IMETHODIMP sbScriptableFilter::NewEnumerate( nsIXPConnectWrappedNative *wrapper,
                                                JSContext * cx,
                                                JSObject * obj,
                                                PRUint32 enum_op,
                                                jsval * statep,
                                                jsid *idp,
                                                PRBool *_retval)
{
  TRACE(("sbScriptableFilter::NewEnumerate()"));
  
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_ARG_POINTER(statep);

  nsresult rv = ReadEnumerator();
  NS_ENSURE_SUCCESS( rv, rv );

  *_retval = PR_TRUE;
  
  switch(enum_op) {
    case JSENUMERATE_INIT: {
      *statep = INT_TO_JSVAL(0);
      if (idp) {
        *idp = INT_TO_JSVAL(mStrings.Count());
      }
      TRACE(("  init: count %i", mStrings.Count()));
      break;
    }
    case JSENUMERATE_NEXT: {
      // prevent GC from confusing us
      JSAutoRequest ar(cx);

      PRInt32 i = JSVAL_TO_INT(*statep);
      if ( i < 0 || i > mStrings.Count() ) {
        TRACE(( "  invalid state %i of %i", i, mStrings.Count() ));
        *_retval = PR_FALSE;
        *statep = JSVAL_NULL;
        return NS_ERROR_INVALID_ARG;
      } else if ( i == mStrings.Count() ) {
        TRACE(("  finished iteration"));
        *_retval = PR_TRUE;
        *statep = JSVAL_NULL;
        return NS_OK;
      }
      
      nsString *str = mStrings[i];
      
      JSString *jsstr = JS_NewUCStringCopyN( cx,
                                             str->BeginReading(),
                                             str->Length() );
      if (!jsstr) {
        TRACE(("  failed to alloc string"));
        *_retval = PR_FALSE;
        return NS_ERROR_OUT_OF_MEMORY;
      }
      
      // define the property while we're here
      *_retval = JS_DefineUCProperty( cx,
                                      obj,
                                      JS_GetStringChars(jsstr),
                                      JS_GetStringLength(jsstr),
                                      JSVAL_VOID,
                                      nsnull,
                                      nsnull,
                                      JSPROP_ENUMERATE |
                                        JSPROP_READONLY |
                                        JSPROP_PERMANENT );
      if (!*_retval) {
        return NS_ERROR_FAILURE;
      }
      
      *_retval = JS_ValueToId( cx, STRING_TO_JSVAL(jsstr), idp );
      if (!*_retval) {
        TRACE(("  failed to get id"));
        return NS_ERROR_FAILURE;
      }
      
      *statep = INT_TO_JSVAL(++i);
      TRACE(("  next: %i", JSVAL_TO_INT(*statep)));
      break;
    }
    case JSENUMERATE_DESTROY: {
      // nothing to do
      break;
    }
    default:
      // umm, should not happen
      *_retval = PR_FALSE;
      return NS_ERROR_INVALID_ARG;
  }
  return NS_OK;
}

NS_IMETHODIMP sbScriptableFilter::NewResolve( nsIXPConnectWrappedNative *wrapper,
                                              JSContext * cx,
                                              JSObject * obj,
                                              jsval id,
                                              PRUint32 flags,
                                              JSObject * *objp,
                                              PRBool *_retval)
{
  TRACE(("sbScriptableFilter::NewResolve()"));
  NS_ENSURE_ARG_POINTER(_retval);
  
  nsresult rv = ReadEnumerator();
  NS_ENSURE_SUCCESS( rv, rv );

  jsval v;
  *_retval = JS_IdToValue( cx, id, &v );
  NS_ENSURE_TRUE( *_retval, NS_ERROR_INVALID_ARG );
  
  // we only consider string properties
  JSString *jsstr = JS_ValueToString( cx, id );
  if (!jsstr) {
    if (objp) {
      *objp = nsnull;
    }
    return NS_OK;
  }
  
  nsDependentString prop( JS_GetStringChars(jsstr) );
  TRACE(("  Resolving property %s",
         NS_LossyConvertUTF16toASCII(prop).BeginReading() ));
  
  PRInt32 length = mStrings.Count();
  for (PRInt32 i = 0; i < length; ++i) {
    if ( mStrings[i]->Equals(prop) ) {
      *_retval = JS_DefineUCProperty( cx,
                                      obj,
                                      JS_GetStringChars(jsstr),
                                      JS_GetStringLength(jsstr),
                                      JSVAL_VOID,
                                      nsnull,
                                      nsnull,
                                      JSPROP_ENUMERATE |
                                        JSPROP_READONLY |
                                        JSPROP_PERMANENT );
      if (objp) {
        *objp = obj;
      }
      return NS_OK;
    }
  }
  if (objp) {
    *objp = nsnull;
  }

  return NS_OK;
}

// ---------------------------------------------------------------------------
//
//                          nsISecurityCheckedComponent
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP sbScriptableFilter::CanCreateWrapper( const nsIID * iid,
                                                    char **_retval)
{
  TRACE(("sbScriptableFilter::CanCreateWrapper()"));

  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = ToNewCString( NS_LITERAL_CSTRING("AllAccess") );
  return NS_OK;
}

NS_IMETHODIMP sbScriptableFilter::CanCallMethod( const nsIID * iid,
                                                 const PRUnichar *methodName,
                                                 char **_retval)
{
  TRACE(("sbScriptableFilter::CanCallMethod() - %s",
         NS_LossyConvertUTF16toASCII(methodName).BeginReading()));

  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = ToNewCString( NS_LITERAL_CSTRING("AllAccess") );
  return NS_OK;
}

NS_IMETHODIMP sbScriptableFilter::CanGetProperty( const nsIID * iid,
                                                  const PRUnichar *propertyName,
                                                  char **_retval)
{
  TRACE(("sbScriptableFilter::CanGetProperty() - %s",
         NS_LossyConvertUTF16toASCII(propertyName).BeginReading()));

  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = ToNewCString( NS_LITERAL_CSTRING("AllAccess") );
  return NS_OK;
}

NS_IMETHODIMP sbScriptableFilter::CanSetProperty( const nsIID * iid,
                                                  const PRUnichar *propertyName,
                                                  char **_retval)
{
  TRACE(("sbScriptableFilter::CanSetProperty() - %s",
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
NS_DECL_CLASSINFO(sbScriptableFilter)

SB_IMPL_CLASSINFO_INTERFACES_ONLY(sbScriptableFilter)
