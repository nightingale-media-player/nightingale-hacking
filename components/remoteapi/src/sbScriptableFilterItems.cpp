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

#include "sbScriptableFilterItems.h"

#include <sbClassInfoUtils.h>

#include <nsAutoPtr.h>
#include <nsMemory.h>
#include <nsIClassInfoImpl.h>
#include <nsIStringEnumerator.h>
#include <nsStringGlue.h>

#include <jsapi.h>

#include <sbIFilterableMediaListView.h>
#include <sbIMediaListView.h>
#include <sbIMediaItem.h>

#include "sbRemoteAPIUtils.h"
#include "sbRemoteIndexedMediaItem.h"
#include "sbRemotePlayer.h"

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbScriptableFilterItems:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gScriptableFilterItemsLog = nsnull;
#endif

#undef LOG
#define LOG(args) PR_LOG(gScriptableFilterItemsLog, PR_LOG_WARN, args)
#define TRACE(args) PR_LOG(gScriptableFilterItemsLog, PR_LOG_DEBUG, args)

NS_IMPL_ISUPPORTS4( sbScriptableFilterItems,
                    nsISecurityCheckedComponent,
                    nsIXPCScriptable,
                    nsISimpleEnumerator,
                    sbScriptableFilterItems )
NS_IMPL_CI_INTERFACE_GETTER3( sbScriptableFilterItems,
                              nsISecurityCheckedComponent,
                              nsIXPCScriptable,
                              nsISimpleEnumerator )

// see note in scScriptableFilterItems.h - this is for QI
NS_DEFINE_STATIC_IID_ACCESSOR(sbScriptableFilterItems, SB_SCRIPTABLE_FILETER_ITEMS_CID)

sbScriptableFilterItems::sbScriptableFilterItems( sbIFilterableMediaListView *aFilterList,
                                                  sbRemotePlayer *aPlayer )
  : mListView(aFilterList),
    mHasItems(PR_FALSE),
    mPlayer(aPlayer),
    mEnumerationIndex(0)
{
#ifdef PR_LOGGING
  if (!gScriptableFilterItemsLog) {
    gScriptableFilterItemsLog = PR_NewLogModule( "sbScriptableFilterItems" );
  }
#endif
  TRACE(("sbScriptableFilterItems::sbScriptableFilterItems()"));
}

sbScriptableFilterItems::sbScriptableFilterItems( const nsCOMArray<sbIMediaItem>& aItems,
                                                  sbRemotePlayer *aPlayer )
  : mItems(aItems),
    mHasItems(PR_TRUE),
    mPlayer(aPlayer),
    mEnumerationIndex(0)
{
#ifdef PR_LOGGING
  if (!gScriptableFilterItemsLog) {
    gScriptableFilterItemsLog = PR_NewLogModule( "sbScriptableFilterItems" );
  }
#endif
  TRACE(("sbScriptableFilterItems::sbScriptableFilterItems()"));
}

sbScriptableFilterItems::~sbScriptableFilterItems()
{
}

nsresult
sbScriptableFilterItems::ReadEnumerator()
{
  if ( mHasItems ) {
    // already read
    return NS_OK;
  }

  if (!mListView) {
    // no enumerator to resolve
    return NS_ERROR_NOT_INITIALIZED;
  }
  
  nsresult rv;
  
  nsCOMPtr<sbIMediaListView> listView = do_QueryInterface( mListView, &rv );
  NS_ENSURE_SUCCESS( rv, rv );
  
  PRUint32 length;
  rv = listView->GetLength( &length );
  NS_ENSURE_SUCCESS( rv, rv );
  
  for (PRUint32 i = 0; i < length; ++i ) {
    nsCOMPtr<sbIMediaItem> item;
    rv = listView->GetItemByIndex( i, getter_AddRefs(item) );
    NS_ENSURE_SUCCESS( rv, rv );
    mItems.AppendObject(item);
  }
  
  mHasItems = PR_TRUE;
  return NS_OK;
}

// ---------------------------------------------------------------------------
//
//                          nsIXPCScriptable
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP sbScriptableFilterItems::GetClassName(char * *aClassName)
{
  NS_ENSURE_ARG_POINTER(aClassName);
  
  NS_NAMED_LITERAL_CSTRING( kClassName, "sbScriptableFilterItems" );
  *aClassName = ToNewCString(kClassName);
  return aClassName ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

/* readonly attribute PRUint32 scriptableFlags; */
NS_IMETHODIMP sbScriptableFilterItems::GetScriptableFlags(PRUint32 *aScriptableFlags)
{
  NS_ENSURE_ARG_POINTER(aScriptableFlags);
  // XXX Mook: USE_JSSTUB_FOR_ADDPROPERTY is needed to define things on the
  //           prototype properly; even with it set scripts cannot add
  //           properties onto the object (because they're not allow to *set*)
  *aScriptableFlags = USE_JSSTUB_FOR_ADDPROPERTY |
                      DONT_ENUM_STATIC_PROPS |
                      DONT_ENUM_QUERY_INTERFACE |
                      CLASSINFO_INTERFACES_ONLY |
                      WANT_GETPROPERTY |
                      WANT_NEWENUMERATE |
                      WANT_NEWRESOLVE |
                      WANT_EQUALITY |
                      ALLOW_PROP_MODS_DURING_RESOLVE |
                      DONT_REFLECT_INTERFACE_NAMES ;
  return NS_OK;
}

NS_IMETHODIMP sbScriptableFilterItems::GetProperty( nsIXPConnectWrappedNative *wrapper,
                                                    JSContext * cx,
                                                    JSObject * obj,
                                                    jsval id,
                                                    jsval * vp,
                                                    PRBool *_retval)
{
  TRACE(("sbScriptableFilterItems::GetProperty()"));
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

  PRInt32 length = mItems.Count();
  for (PRInt32 i = 0; i < length; ++i) {
    nsString guid;
    rv = mItems[i]->GetGuid(guid);
    NS_ENSURE_SUCCESS( rv, rv );
    if ( guid.Equals(jsid) ) {
      nsCOMPtr<sbIMediaItem> remoteItem;
      rv = SB_WrapMediaItem( mPlayer, mItems[i], getter_AddRefs(remoteItem) );
      NS_ENSURE_SUCCESS( rv, rv );
      
      nsCOMPtr<nsIXPConnect> xpc;
      rv = wrapper->GetXPConnect( getter_AddRefs(xpc) );
      NS_ENSURE_SUCCESS( rv, rv );
      
      nsCOMPtr<nsIXPConnectJSObjectHolder> objHolder;
      
      rv = xpc->WrapNative( cx,
                            obj,
                            remoteItem,
                            sbIMediaItem::GetIID(),
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
    *vp = INT_TO_JSVAL( mItems.Count() );
    return NS_SUCCESS_I_DID_SOMETHING;
  }

  return NS_OK;
}

NS_IMETHODIMP sbScriptableFilterItems::NewEnumerate( nsIXPConnectWrappedNative *wrapper,
                                                     JSContext * cx,
                                                     JSObject * obj,
                                                     PRUint32 enum_op,
                                                     jsval * statep,
                                                     jsid *idp,
                                                     PRBool *_retval )
{
  TRACE(("sbScriptableFilterItems::NewEnumerate()"));
  
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_ARG_POINTER(statep);

  nsresult rv = ReadEnumerator();
  NS_ENSURE_SUCCESS( rv, rv );

  *_retval = PR_TRUE;
  
  switch(enum_op) {
    case JSENUMERATE_INIT: {
      *statep = INT_TO_JSVAL(0);
      if (idp) {
        *idp = INT_TO_JSVAL(mItems.Count());
      }
      TRACE(("  init: count %i", mItems.Count()));
      break;
    }
    case JSENUMERATE_NEXT: {
      // prevent GC from confusing us
      JSAutoRequest ar(cx);

      PRInt32 i = JSVAL_TO_INT(*statep);
      if ( i < 0 || i > mItems.Count() ) {
        TRACE(( "  invalid state %i of %i", i, mItems.Count() ));
        *_retval = PR_FALSE;
        *statep = JSVAL_NULL;
        return NS_ERROR_INVALID_ARG;
      } else if ( i == mItems.Count() ) {
        TRACE(("  finished iteration"));
        *_retval = PR_TRUE;
        *statep = JSVAL_NULL;
        return NS_OK;
      }
      
      nsCOMPtr<sbIMediaItem> item = mItems[i];
      
      nsString guid;
      rv = item->GetGuid(guid);
      NS_ENSURE_SUCCESS( rv, rv );
      
      JSString *jsstr = JS_NewUCStringCopyN( cx,
                                             guid.BeginReading(),
                                             guid.Length() );
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

NS_IMETHODIMP sbScriptableFilterItems::NewResolve( nsIXPConnectWrappedNative *wrapper,
                                                   JSContext * cx,
                                                   JSObject * obj,
                                                   jsval id,
                                                   PRUint32 flags,
                                                   JSObject * *objp,
                                                   PRBool *_retval)
{
  TRACE(("sbScriptableFilterItems::NewResolve()"));
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
  
  PRInt32 length = mItems.Count();
  for (PRInt32 i = 0; i < length; ++i) {
    nsString guid;
    rv = mItems[i]->GetGuid(guid);
    NS_ENSURE_SUCCESS( rv, rv );
    if ( guid.Equals(prop) ) {
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

NS_IMETHODIMP sbScriptableFilterItems::Equality( nsIXPConnectWrappedNative *wrapper,
                                                 JSContext * cx,
                                                 JSObject * obj,
                                                 jsval val,
                                                 PRBool *_retval )
{
  LOG(("sbScriptableFilterItems::Equality()"));
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
  
  nsRefPtr<sbScriptableFilterItems> other;
  sbScriptableFilterItems* otherPtr;
  rv = CallQueryInterface( otherWrapper->Native(), &otherPtr );
  if ( NS_FAILED(rv) ) {
    // not us
    return NS_OK;
  }
  other = already_AddRefed<sbScriptableFilterItems>(otherPtr);
  
  rv = ReadEnumerator();
  NS_ENSURE_SUCCESS( rv, rv );
  rv = other->ReadEnumerator();
  NS_ENSURE_SUCCESS( rv, rv );
  
  if ( mItems.Count() != other->mItems.Count() ) {
    // lengths don't match, not the same
    return NS_OK;
  }
  
  // XXX Mook: this is pretty sucky.  We take all the items in the other set
  //           and compare against all the items in this set.

  std::multiset<nsString> itemSet;
  for (PRInt32 i = 0; i < other->mItems.Count(); ++i) {
    nsString guid;
    rv = other->mItems[i]->GetGuid(guid);
    NS_ENSURE_SUCCESS( rv, rv );
    itemSet.insert(guid);
  }
  
  PRInt32 itemsCount = mItems.Count();
  for (PRInt32 i = 0; i < itemsCount; ++i) {
    nsString guid;
    rv = mItems[i]->GetGuid(guid);
    NS_ENSURE_SUCCESS( rv, rv );
    std::multiset<nsString>::iterator found;
    found = itemSet.find(guid);
    if ( found == itemSet.end() ) {
      // not found
      return NS_OK;
    }
    itemSet.erase(found);
  }
  if ( itemSet.empty() ) {
    // no more items left, the two are the same
    *_retval = PR_TRUE;
  }
  
  return NS_OK;
}


// ---------------------------------------------------------------------------
//
//                               nsISimpleEnumerator
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP sbScriptableFilterItems::HasMoreElements(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE( mHasItems, NS_ERROR_NOT_INITIALIZED );
  NS_ENSURE_STATE( mEnumerationIndex >= 0 );

  *_retval = mEnumerationIndex < mItems.Count();
  return NS_OK;
}

NS_IMETHODIMP sbScriptableFilterItems::GetNext(nsISupports **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE( mHasItems, NS_ERROR_NOT_INITIALIZED );
  NS_ENSURE_STATE( mEnumerationIndex >= 0 );
  NS_ENSURE_TRUE( mEnumerationIndex < mItems.Count(), NS_ERROR_FAILURE );

  nsresult rv;
  nsCOMPtr<sbIMediaItem> rawItem = mItems[mEnumerationIndex];

  nsCOMPtr<sbIIndexedMediaItem> item = do_QueryInterface(rawItem, &rv);
  if (NS_SUCCEEDED(rv)) {
    // we have an IndexedMediaItem, we're handling a selection enumeration
    nsRefPtr<sbRemoteIndexedMediaItem> indexedMediaItem =
      new sbRemoteIndexedMediaItem(mPlayer, item);
    NS_ENSURE_TRUE(indexedMediaItem, NS_ERROR_OUT_OF_MEMORY);

    rv = indexedMediaItem->Init();
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ADDREF( *_retval = NS_ISUPPORTS_CAST(sbIIndexedMediaItem*,
                                            indexedMediaItem) );
  }
  else {
    nsCOMPtr<sbIMediaItem> remoteItem;
    rv = SB_WrapMediaItem(mPlayer, rawItem, getter_AddRefs(remoteItem));
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ADDREF( *_retval = NS_ISUPPORTS_CAST(sbIMediaItem*,
                                            remoteItem) );
  }
  
  TRACE(( "sbScriptableFilterItems::GetNext(): got %i = %08x",
          mEnumerationIndex,
          *_retval ));
  ++mEnumerationIndex;
  return NS_OK;
}

// ---------------------------------------------------------------------------
//
//                          nsISecurityCheckedComponent
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP sbScriptableFilterItems::CanCreateWrapper( const nsIID * iid,
                                                         char **_retval)
{
  TRACE(("sbScriptableFilterItems::CanCreateWrapper()"));

  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = ToNewCString( NS_LITERAL_CSTRING("AllAccess") );
  return NS_OK;
}

NS_IMETHODIMP sbScriptableFilterItems::CanCallMethod( const nsIID * iid,
                                                      const PRUnichar *methodName,
                                                      char **_retval)
{
  TRACE(("sbScriptableFilterItems::CanCallMethod() - %s",
         NS_LossyConvertUTF16toASCII(methodName).BeginReading()));

  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = ToNewCString( NS_LITERAL_CSTRING("AllAccess") );
  return NS_OK;
}

NS_IMETHODIMP sbScriptableFilterItems::CanGetProperty( const nsIID * iid,
                                                       const PRUnichar *propertyName,
                                                       char **_retval)
{
  TRACE(("sbScriptableFilterItems::CanGetProperty() - %s",
         NS_LossyConvertUTF16toASCII(propertyName).BeginReading()));

  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = ToNewCString( NS_LITERAL_CSTRING("AllAccess") );
  return NS_OK;
}

NS_IMETHODIMP sbScriptableFilterItems::CanSetProperty( const nsIID * iid,
                                                       const PRUnichar *propertyName,
                                                       char **_retval)
{
  TRACE(("sbScriptableFilterItems::CanSetProperty() - %s",
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
NS_DECL_CLASSINFO(sbScriptableFilterItems)

SB_IMPL_CLASSINFO_INTERFACES_ONLY(sbScriptableFilterItems)