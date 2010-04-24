/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

#include "sbRemoteAPIUtils.h"
#include "sbRemoteMediaListBase.h"
#include "sbRemoteWrappingStringEnumerator.h"
#include "sbRemotePlayer.h"
#include "sbScriptableFunction.h"

#include <sbClassInfoUtils.h>
#include <sbIRemoteMediaList.h>
#include <sbIMediaItem.h>
#include <sbIMediaList.h>
#include <sbIMediaListView.h>
#include <sbIMediaListViewTreeView.h>
#include <sbIWrappedMediaItem.h>
#include <sbIWrappedMediaList.h>
#include <sbIPropertyManager.h>
#include <sbPropertiesCID.h>
#include <sbStandardProperties.h>

#include <nsAutoPtr.h>
#include <nsITreeSelection.h>
#include <nsITreeView.h>
#include <nsStringGlue.h>
#include <prlog.h>

// includes for XPCScriptable impl
#include <nsNetUtil.h>
#include <nsIXPConnect.h>
#include <jsapi.h>
#include <jsobj.h>
#include <sbIPropertyArray.h>
#include <nsServiceManagerUtils.h>
#include <nsIScriptSecurityManager.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteMediaList:5
 */
#ifdef PR_LOGGING
PRLogModuleInfo* gRemoteMediaListLog = nsnull;
#endif

#undef LOG
#define LOG(args) LOG_LIST(args)

#define SB_ENSURE_WITH_JSTHROW( _cx, _rv, _msg )        \
  if ( NS_FAILED(_rv) ) {                               \
    ThrowJSException( _cx, NS_LITERAL_CSTRING(_msg)  ); \
    return JS_FALSE;                                    \
  }

// derived classes must impl nsIClassInfo
NS_IMPL_ISUPPORTS9(sbRemoteMediaListBase,
                   nsISecurityCheckedComponent,
                   nsIXPCScriptable,
                   sbISecurityAggregator,
                   sbIRemoteMediaList,
                   sbIMediaList,
                   sbIWrappedMediaList,
                   sbIWrappedMediaItem,
                   sbIMediaItem,
                   sbILibraryResource)

sbRemoteMediaListBase::sbRemoteMediaListBase(sbRemotePlayer* aRemotePlayer,
                                             sbIMediaList* aMediaList,
                                             sbIMediaListView* aMediaListView) :
  mRemotePlayer(aRemotePlayer),
  mMediaList(aMediaList),
  mMediaListView(aMediaListView)
{
  NS_ASSERTION(aRemotePlayer, "Null remote player!");
  NS_ASSERTION(aMediaList, "Null media list!");
  NS_ASSERTION(aMediaListView, "Null media list view!");

  mMediaItem = do_QueryInterface(mMediaList);
  NS_ASSERTION(mMediaItem, "Could not QI media list to media item");

  mMediaItem->GetLibrary(getter_AddRefs(mLibrary));

#ifdef PR_LOGGING
  if (!gRemoteMediaListLog) {
    gRemoteMediaListLog = PR_NewLogModule("sbRemoteMediaList");
  }
  LOG_LIST(("sbRemoteMediaListBase::sbRemoteMediaListBase()"));
#endif
}

sbRemoteMediaListBase::~sbRemoteMediaListBase()
{
  LOG_LIST(("sbRemoteMediaListBase::~sbRemoteMediaListBase()"));
}

// ---------------------------------------------------------------------------
//
//                          sbISecurityAggregator
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbRemoteMediaListBase::GetRemotePlayer(sbIRemotePlayer * *aRemotePlayer)
{
  NS_ENSURE_STATE(mRemotePlayer);
  NS_ENSURE_ARG_POINTER(aRemotePlayer);

  nsresult rv;
  *aRemotePlayer = nsnull;

  nsCOMPtr<sbIRemotePlayer> remotePlayer;

  rv = mRemotePlayer->QueryInterface( NS_GET_IID( sbIRemotePlayer ), getter_AddRefs( remotePlayer ) );
  NS_ENSURE_SUCCESS( rv, rv );

  remotePlayer.swap( *aRemotePlayer );

  return NS_OK;
}

// ---------------------------------------------------------------------------
//
//                        sbIWrappedMediaList
//
// ---------------------------------------------------------------------------

already_AddRefed<sbIMediaItem>
sbRemoteMediaListBase::GetMediaItem()
{
  nsresult rv;

  nsCOMPtr<sbIMediaItem> mediaItem = do_QueryInterface(mMediaList, &rv);
  NS_ASSERTION(mediaItem, "Could not QI list to item");

  return mediaItem.forget();
}

already_AddRefed<sbIMediaList>
sbRemoteMediaListBase::GetMediaList()
{
  sbIMediaList* list = mMediaList;
  NS_ADDREF(list);
  return list;
}

// ---------------------------------------------------------------------------
//
//                           nsIXPCScriptable
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbRemoteMediaListBase::GetClassName( char **aClassName )
{
  LOG_LIST(("sbRemoteMediaListBase::GetClassName()"));
  NS_ENSURE_ARG_POINTER(aClassName);
  *aClassName = ToNewCString( NS_LITERAL_CSTRING("SongbirdMediaList") );
  NS_ENSURE_TRUE( aClassName, NS_ERROR_OUT_OF_MEMORY );
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteMediaListBase::GetScriptableFlags( PRUint32 *aScriptableFlags )
{
  LOG_LIST(("sbRemoteMediaListBase::GetScriptableFlags()"));
  NS_ENSURE_ARG_POINTER(aScriptableFlags);
  *aScriptableFlags = WANT_NEWRESOLVE |
                      ALLOW_PROP_MODS_DURING_RESOLVE |
                      DONT_ENUM_STATIC_PROPS |
                      DONT_ENUM_QUERY_INTERFACE |
                      DONT_REFLECT_INTERFACE_NAMES ;
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteMediaListBase::NewResolve( nsIXPConnectWrappedNative *wrapper,
                                   JSContext *cx,
                                   JSObject *obj,
                                   jsval id,
                                   PRUint32 flags,
                                   JSObject **objp,
                                   PRBool *_retval )
{
  LOG_LIST(("sbRemoteMediaListBase::NewResolve()"));
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_ARG_POINTER(objp);

  if ( JSVAL_IS_STRING(id) ) {
    nsDependentString jsid( (PRUnichar *)
                            ::JS_GetStringChars( JSVAL_TO_STRING(id) ),
                            ::JS_GetStringLength( JSVAL_TO_STRING(id) ) );

    TRACE_LIB(( "   resolving %s", NS_LossyConvertUTF16toASCII(jsid).get() ));

    // If we're being asked for add, define the function and point the
    // caller to the AddHelper method.
    if ( jsid.EqualsLiteral("add") ) {
      JSString *str = JSVAL_TO_STRING(id);
      JSFunction *fnc = ::JS_DefineFunction( cx,
                                             obj,
                                             ::JS_GetStringBytes(str),
                                             AddHelper,
                                             1,
                                             JSPROP_ENUMERATE );

      *objp = obj;

      return fnc ? NS_OK : NS_ERROR_UNEXPECTED;
    }
  }
  return NS_OK;
}

// static
nsresult
sbRemoteMediaListBase::ThrowJSException( JSContext *cx,
                                         const nsACString &aExceptionMsg ) {
  JSAutoRequest ar(cx);

  JSString *str = JS_NewStringCopyN( cx,
                                     aExceptionMsg.BeginReading(),
                                     aExceptionMsg.Length() );
  if (!str) {
    // JS_NewStringCopyN reported the error for us.
    return NS_OK;
  }

  JS_SetPendingException( cx, STRING_TO_JSVAL(str) );
  return NS_OK;
}

/*
 * This method is a general helper for the add functionality. Through this
 *   method we support the following types of adding:
 *
 *   add( item, (opt) boolean );
 *   add( string, (opt) boolean );
 *   add( [ string... ], (opt) boolean );
 *
 * We are argc agnostic here, so if you pass in more than 2 args
 *   we ignore them and if the second arg isn't a media item (QI-able to
 *   sbIMediaItem) then we convert it to it's boolean representation and use
 *   it as an arg to determine if we should download. We only hard fail in the
 *   case of:
 *
 *   add( item, item )
 *   add( string, item )
 *   add( [string...], item )
 *   add( [item...], item )
 *   add( [item...] ) - we don't allow arrays of items yet.
 *
 * We silently do nothing for these cases:
 *
 *   add( [] ) - empty array
 *   add( "" ) - empty string
 *   add( number )
 *
 */
// static
JSBool
sbRemoteMediaListBase::AddHelper( JSContext *cx,
                                  JSObject *obj,
                                  uintN argc,
                                  jsval *argv,
                                  jsval *rval )
{
  // Returning JS_FALSE from this method without setting an exception will
  // cause XPConnect to hang. Either call ThrowJSExcpeption first, or
  // return JS_TRUE if you want to fail silently.

  LOG_LIST(( "sbRemoteMediaListBase::AddHelper() - argc is %d", argc ));
  nsresult rv;

  // we expect one or two arguments, less then one is a hard error, for
  // extra arguments we just ignore them.
  if ( argc < 1 ) {
    ThrowJSException( cx, NS_LITERAL_CSTRING("Wrong number of arguments.") );
    return JS_FALSE;
  }

  //
  // Make sure the object is cleared to call this method
  //

  OBJ_TO_INNER_OBJECT(cx, obj);

  nsCOMPtr<nsIXPConnect> xpc(
    do_GetService( "@mozilla.org/js/xpc/XPConnect;1", &rv ) );
  SB_ENSURE_WITH_JSTHROW( cx, rv, "Failed to get XPConnect service." )

  nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
  rv = xpc->GetWrappedNativeOfJSObject( cx, obj, getter_AddRefs(wrapper) );
  SB_ENSURE_WITH_JSTHROW( cx, rv, "No wrapper for native object." )

  nsCOMPtr<nsISecurityCheckedComponent> checkedComponent(
    do_QueryWrappedNative( wrapper, &rv ) );
  SB_ENSURE_WITH_JSTHROW( cx, rv, "Not a checked object.")

  // do the security check
  char* access;
  nsIID iid = NS_GET_IID(nsISupports);
  rv = checkedComponent->CanCallMethod( &iid,
                                        NS_LITERAL_STRING("add").get(),
                                        &access );

  // note that an error return value or empty access means access denied
  PRBool canCallMethod = NS_SUCCEEDED(rv);
  if (canCallMethod) {
    if (!access) {
      canCallMethod = PR_FALSE;
    } else {
      canCallMethod = !strcmp( access, "AllAccess" );
      NS_Free(access);
    }
  }

  if (!canCallMethod) {
    ThrowJSException( cx, NS_LITERAL_CSTRING("Permission Denied to call method RemoteMediaList.add()")  );
    return JS_FALSE;
  }

  //
  // Passed the security check, do the work of calling Add() or doing
  // the additions by hand from the array of strings passed in
  //

  // Need to QI ourself to get to our library, create the item, check target
  nsCOMPtr<sbIMediaItem> selfItem( do_QueryWrappedNative( wrapper, &rv ) );
  SB_ENSURE_WITH_JSTHROW( cx, rv, "Not a valid MediaItem.")

  // Find out if we're targetting the main library early, to know if we should
  // download the track at the end, and have the arg ready for recursion.
  PRBool isTargetMain = PR_FALSE;
  rv = SB_IsFromLibName( selfItem, NS_LITERAL_STRING("main"), &isTargetMain );
  SB_ENSURE_WITH_JSTHROW( cx, rv, "Not able to determine mainLibrariness." );

  // Find out if we should download the tracking after adding. Only download if
  //   -- the list being added to is in the main library (isTargetMain).
  //   -- the 2nd arg is set to something true and is not a mediaitem
  JSBool shouldDownload = JS_FALSE;
  if ( 1 < argc ) {
    LOG_LIST(("sbRemoteMediaListBase::AddHelper() - argv[1] exists, make sure it's not an item"));
    if ( JSVAL_IS_OBJECT( argv[1] ) ) {

      // see if we can get the wrapped native
      nsCOMPtr<nsIXPConnectWrappedNative> wn;
      rv = xpc->GetWrappedNativeOfJSObject(cx, JSVAL_TO_OBJECT( argv[1] ), getter_AddRefs(wn) );
      if ( NS_SUCCEEDED(rv) && wn ) {
        // There is a wrapped native, find out if it's a media item
        nsCOMPtr<sbIMediaItem> tempItem;
        tempItem = do_QueryWrappedNative( wn, &rv );
        if ( NS_SUCCEEDED(rv) ) {
          // Hard fail so devs don't expect add(item,item) to add multiple items.
          ThrowJSException( cx, NS_LITERAL_CSTRING("Second arg should NOT be a media item.")  );
          return JS_FALSE;
        }
      }
    }

    LOG_LIST(("sbRemoteMediaListBase::AddHelper() - argv[1] exists, is not a mediaitem"));
    if (!JS_ValueToBoolean(cx, argv[1], &shouldDownload)) {
      shouldDownload = JS_FALSE;
    }

    if ( shouldDownload && !isTargetMain ) {
      // not targetting main library, no download
      shouldDownload = PR_FALSE;
      NS_WARNING(("Non-Fatal Error: Tried to download an item to non-mainLibrary."));
    }
  }

  // Need the Aggregator interface to get to the remotePlayer
  //nsCOMPtr<sbISecurityAggregator> secAgg( do_QueryWrappedNative( wrapper, &rv ) );
  nsCOMPtr<sbISecurityAggregator> secAgg( do_QueryInterface( selfItem, &rv ) );
  SB_ENSURE_WITH_JSTHROW( cx, rv, "Object not valid security aggregator.")

  // Get the remote player
  nsCOMPtr<sbIRemotePlayer> remotePlayer;
  rv = secAgg->GetRemotePlayer( getter_AddRefs(remotePlayer) );
  SB_ENSURE_WITH_JSTHROW( cx, rv, "Could not get RemotePlayer.")

  // declare here, it gets set inside the next conditional and used aftewards
  nsCOMPtr<sbIMediaItem> item;

  JSAutoRequest ar(cx);

  // Handle the args passed in. If we are passed an array, recurse over the
  // contents, otherwise set |item| to be the item to add to the target.
  if ( JSVAL_IS_STRING(argv[0]) ) {

    // If we have a string arg it should be a URI, use our library to create
    // an item.

    LOG_LIST(("sbRemoteMediaListBase::AddHelper() - argv[0] exists, is a string"));
    LOG_LIST(("          length: %d", ::JS_GetStringLength( JSVAL_TO_STRING(argv[0]) ) ) );

    if ( ::JS_GetStringLength( JSVAL_TO_STRING(argv[0]) ) == 0 ) {
      // light, silent failure on empty strings
      return JS_TRUE;
    }

    // Convert the JS string into an XPCOM string.
    nsDependentString url( (PRUnichar *)
                           ::JS_GetStringChars( JSVAL_TO_STRING(argv[0]) ),
                           ::JS_GetStringLength( JSVAL_TO_STRING(argv[0]) ) );

    LOG_LIST(("sbRemoteMediaListBase::AddHelper() - argv[0] exists, is a string"));
    LOG_LIST(( "   str %s", NS_LossyConvertUTF16toASCII(url).get() ));

    // Create a URI object to pass into the library for item creation.
    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI( getter_AddRefs(uri), url );
    SB_ENSURE_WITH_JSTHROW( cx, rv, "Could not create new URI object.")

    // The item will return an unwrapped library so find out where we came from
    // and get the appropriately wrapped library from the remotePlayer.
    nsCOMPtr<sbIRemoteLibrary> library;

    // isTargetMain set well above
    if ( isTargetMain ) {
      // from Main, get the main library
      rv = remotePlayer->GetMainLibrary( getter_AddRefs(library) );
      SB_ENSURE_WITH_JSTHROW( cx, rv, "Could not get remote library.")
    } else {
      PRBool isTargetWeb = PR_FALSE;
      rv = SB_IsFromLibName( selfItem, NS_LITERAL_STRING("web"), &isTargetWeb );
      SB_ENSURE_WITH_JSTHROW( cx, rv, "Not able to determine webLibrariness." );

      if ( !isTargetMain ) {
        // from web, get the web library
        rv = remotePlayer->GetWebLibrary( getter_AddRefs(library) );
        SB_ENSURE_WITH_JSTHROW( cx, rv, "Could not get web library.")
      } else {
        // get the Site Library
        rv = remotePlayer->GetSiteLibrary( getter_AddRefs(library) );
        SB_ENSURE_WITH_JSTHROW( cx, rv, "Could not get site library.")
      }
    }

    nsCString uriCStr;
    rv = uri->GetSpec(uriCStr);
    SB_ENSURE_WITH_JSTHROW( cx, rv, "Could not get spec from uri.")

    // Create the item.
    rv = library->CreateMediaItem( NS_ConvertUTF8toUTF16(uriCStr),
                                   getter_AddRefs(item) );
    SB_ENSURE_WITH_JSTHROW( cx, rv, "Could not create new Media Item.")

  } else if ( JSVAL_IS_OBJECT(argv[0]) ) {
    JSObject* jsobj = JSVAL_TO_OBJECT(argv[0]);
    if (!jsobj) {
      ThrowJSException( cx, NS_LITERAL_CSTRING("Failed to convert object.")  );
      return JS_FALSE;
    }

    LOG_LIST(("sbRemoteMediaListBase::AddHelper() - argv[0] exists, is an object"));

    if ( JS_IsArrayObject(cx, jsobj) ) {
      LOG_LIST(("sbRemoteMediaListBase::AddHelper() - argv[0] exists, is an array"));

      // loop over the array and take each string, create an item.
      jsuint len;
      jsval val;
      if ( JS_GetArrayLength( cx, jsobj, &len ) ) {
        for ( JSUint32 index = 0; index < len; index++ ) {
          LOG_LIST(("sbRemoteMediaListBase::AddHelper() - checking index; %d.", index));
          if ( JS_GetElement( cx, jsobj, index, &val ) ) {
            LOG_LIST(("sbRemoteMediaListBase::AddHelper() - found a val in the array."));
            // got the element
            if ( JSVAL_IS_STRING(val) ) {
              LOG_LIST(("sbRemoteMediaListBase::AddHelper() - found a val in the array, recursing"));

              // We're going to recurse, keep args to a set we support
              uintN length = (shouldDownload ? 2 : 1);
              jsval *newval = (jsval*) JS_malloc( cx, sizeof(jsval) * length );
              newval[0] = val;
              if (shouldDownload) {
                LOG_LIST(("sbRemoteMediaListBase::AddHelper() - should download, setting value"));
                newval[1] = BOOLEAN_TO_JSVAL(shouldDownload);
              }
              if ( !AddHelper( cx, obj, length, newval, rval ) ) {
                LOG_LIST(("sbRemoteMediaListBase::AddHelper() - failed return from recursing"));
                // AddHelper will already have set the exception for us.
                JS_free( cx, newval );
                return JS_FALSE;
              }
              LOG_LIST(("sbRemoteMediaListBase::AddHelper() - successful return from recursing"));
              JS_free( cx, newval );
            } else {
              ThrowJSException( cx, NS_LITERAL_CSTRING("Arrays should only contain strings.") );
              return JS_FALSE;
            }
          }
        }
      }
    } else {
      LOG_LIST(("sbRemoteMediaListBase::AddHelper() - argv[0] exists, is an object, not an array"));

      // check to make sure it's an sbIRemoteMediaItem
      nsCOMPtr<nsIXPConnectWrappedNative> wn;
      rv = xpc->GetWrappedNativeOfJSObject(cx, JSVAL_TO_OBJECT( argv[0] ), getter_AddRefs(wn) );
      SB_ENSURE_WITH_JSTHROW( cx, rv, "Failed to get wrapper for argument." )

      item = do_QueryWrappedNative( wn, &rv );
      SB_ENSURE_WITH_JSTHROW( cx, rv, "Argument not a proper MediaItem." );

    }
  } else {
    LOG_LIST(("sbRemoteMediaListBase::AddHelper() - argv[0] not a string, or object, fail silently"));
    return JS_TRUE;
  }

  // If we have an item here, add it to ourself
  // this works if we are adding an item that was created outside of ourself.
  // If we are adding a mainlib item to the mainlib/list and downloading then
  // it doesn't work because an add isn't done, instead we need to just download
  // it.
  if ( item ) {
    LOG_LIST(("sbRemoteMediaListBase::AddHelper() - Have item to add."));

    nsCOMPtr<sbIMediaList> selfList( do_QueryWrappedNative( wrapper, &rv ) );
    SB_ENSURE_WITH_JSTHROW( cx, rv, "Object not valid MediaList.")

    // we only download into the mainLib with the correct param (see above)
    if (shouldDownload) {
      LOG_LIST(("sbRemoteMediaListBase::AddHelper() - Downloading item."));

      // We need to know if this is the main library for downloading
      PRBool isItemMain = PR_FALSE;
      rv = SB_IsFromLibName( item, NS_LITERAL_STRING("main"), &isItemMain );
      SB_ENSURE_WITH_JSTHROW( cx, rv, "Not able to determine mainLibrariness." );

      if (!isItemMain) {
        // set autodownload so addition will kick off a download
        rv = item->SetProperty( NS_LITERAL_STRING(SB_PROPERTY_ENABLE_AUTO_DOWNLOAD),
                                NS_LITERAL_STRING("1") );
      } else {
        // item is already in mainLib so just add it to the download list
        rv = remotePlayer->DownloadItem(item);
      }

      if (NS_FAILED(rv)) {
        // don't throw if we can't download
        NS_WARNING(("sbRemoteMediaListBase::AddHelper() - Failed to download."));
      }
    } else {
      LOG_LIST(("sbRemoteMediaListBase::AddHelper() - Not downloading"));
    }

    rv = selfList->Add(item);
    if ( NS_FAILED(rv) ) {
      NS_WARNING(("sbRemoteMediaListBase::AddHelper() - Failed to add the item."));
    }
  }

  LOG_LIST(("sbRemoteMediaListBase::AddHelper() - returning JS_TRUE"));
  return JS_TRUE;
}

// ---------------------------------------------------------------------------
//
//                        sbIMediaList
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbRemoteMediaListBase::GetItemByGuid(const nsAString& aGuid,
                                     sbIMediaItem** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIMediaItem> item;
  nsresult rv = mMediaList->GetItemByGuid(aGuid, getter_AddRefs(item));
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    // This means that the item doesn't exist. Return null to be friendly to the
    // nice JS callers.
    *_retval = nsnull;
    return NS_OK;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return SB_WrapMediaItem(mRemotePlayer, item, _retval);
}

NS_IMETHODIMP
sbRemoteMediaListBase::GetItemByIndex(PRUint32 aIndex,
                                      sbIMediaItem **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIMediaItem> item;
  nsresult rv = mMediaList->GetItemByIndex(aIndex, getter_AddRefs(item));
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    // This means that the item doesn't exist. Return null to be friendly to the
    // nice JS callers.
    *_retval = nsnull;
    return NS_OK;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return SB_WrapMediaItem(mRemotePlayer, item, _retval);
}

NS_IMETHODIMP
sbRemoteMediaListBase::GetItemCountByProperty(const nsAString & aPropertyID,
                                              const nsAString & aPropertyValue,
                                              PRUint32 *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbRemoteMediaListBase::GetListContentType(PRUint16* aContentType)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbRemoteMediaListBase::EnumerateAllItems(sbIMediaListEnumerationListener *aEnumerationListener,
                                         PRUint16 aEnumerationType)
{
  NS_ENSURE_ARG_POINTER(aEnumerationListener);

  nsRefPtr<sbMediaListEnumerationListenerWrapper> wrapper(
    new sbMediaListEnumerationListenerWrapper(mRemotePlayer, aEnumerationListener));
  NS_ENSURE_TRUE(wrapper, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = mMediaList->EnumerateAllItems(wrapper, aEnumerationType);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbRemoteMediaListBase::EnumerateItemsByProperty(const nsAString& aPropertyID,
                                                const nsAString& aPropertyValue,
                                                sbIMediaListEnumerationListener* aEnumerationListener,
                                                PRUint16 aEnumerationType)
{
  NS_ENSURE_ARG_POINTER(aEnumerationListener);

  nsRefPtr<sbMediaListEnumerationListenerWrapper> wrapper(
    new sbMediaListEnumerationListenerWrapper(mRemotePlayer, aEnumerationListener));
  NS_ENSURE_TRUE(wrapper, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = mMediaList->EnumerateItemsByProperty(aPropertyID,
                                                     aPropertyValue,
                                                     wrapper,
                                                     aEnumerationType);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbRemoteMediaListBase::IndexOf(sbIMediaItem* aMediaItem,
                               PRUint32 aStartFrom,
                               PRUint32* _retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIMediaItem> mediaItem;
  nsresult rv = SB_WrapMediaItem(mRemotePlayer,
                                 aMediaItem,
                                 getter_AddRefs(mediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  return mMediaList->IndexOf(mediaItem, aStartFrom, _retval);
}

NS_IMETHODIMP
sbRemoteMediaListBase::LastIndexOf(sbIMediaItem* aMediaItem,
                                   PRUint32 aStartFrom,
                                   PRUint32* _retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIMediaItem> mediaItem;
  nsresult rv = SB_WrapMediaItem(mRemotePlayer,
                                 aMediaItem,
                                 getter_AddRefs(mediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  return mMediaList->LastIndexOf(mediaItem, aStartFrom, _retval);
}

NS_IMETHODIMP
sbRemoteMediaListBase::Contains(sbIMediaItem* aMediaItem, PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIMediaItem> mediaItem;
  nsresult rv = SB_WrapMediaItem(mRemotePlayer,
                                 aMediaItem,
                                 getter_AddRefs(mediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  return mMediaList->Contains(mediaItem, _retval);
}

NS_IMETHODIMP
sbRemoteMediaListBase::Add(sbIMediaItem *aMediaItem)
{
  return AddItem(aMediaItem, nsnull);
}

NS_IMETHODIMP
sbRemoteMediaListBase::AddItem(sbIMediaItem *aMediaItem,
                               sbIMediaItem ** aNewItem)
{
  LOG_LIST(("sbRemoteMediaListBase::Add()"));
  NS_ENSURE_ARG_POINTER(aMediaItem);

  nsresult rv;
  nsCOMPtr<sbIWrappedMediaItem> wrappedMediaItem =
    do_QueryInterface(aMediaItem, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> internalMediaItem = wrappedMediaItem->GetMediaItem();
  NS_ENSURE_TRUE(internalMediaItem, NS_ERROR_FAILURE);

  rv = mMediaList->AddItem(internalMediaItem, aNewItem);
  if (NS_SUCCEEDED(rv)) {
    LOG_LIST(("sbRemoteMediaListBase::Add() - added the item"));
    mRemotePlayer->GetNotificationManager()
      ->Action(sbRemoteNotificationManager::eEditedPlaylist, mLibrary);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbRemoteMediaListBase::AddAll(sbIMediaList *aMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  nsresult rv;
  nsCOMPtr<sbIWrappedMediaList> wrappedMediaList =
    do_QueryInterface(aMediaList, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaList> internalMediaList = wrappedMediaList->GetMediaList();
  NS_ENSURE_TRUE(internalMediaList, NS_ERROR_FAILURE);

  rv = mMediaList->AddAll(internalMediaList);
  if (NS_SUCCEEDED(rv)) {
    mRemotePlayer->GetNotificationManager()
      ->Action(sbRemoteNotificationManager::eEditedPlaylist, mLibrary);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbRemoteMediaListBase::AddSome(nsISimpleEnumerator* aMediaItems)
{
  NS_ENSURE_ARG_POINTER(aMediaItems);

  nsRefPtr<sbUnwrappingSimpleEnumerator> wrapper(
    new sbUnwrappingSimpleEnumerator(aMediaItems));
  NS_ENSURE_TRUE(wrapper, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = mMediaList->AddSome(wrapper);
  if (NS_SUCCEEDED(rv)) {
    mRemotePlayer->GetNotificationManager()
      ->Action(sbRemoteNotificationManager::eEditedPlaylist, mLibrary);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbRemoteMediaListBase::Remove(sbIMediaItem* aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  LOG_LIST(("sbRemoteMediaListBase::Remove()"));

  nsresult rv;
  nsCOMPtr<sbIWrappedMediaItem> wrappedMediaItem =
    do_QueryInterface(aMediaItem, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> internalMediaItem = wrappedMediaItem->GetMediaItem();
  NS_ENSURE_TRUE(internalMediaItem, NS_ERROR_FAILURE);

  rv = mMediaList->Remove(internalMediaItem);
  if (NS_SUCCEEDED(rv)) {
    mRemotePlayer->GetNotificationManager()
      ->Action(sbRemoteNotificationManager::eEditedPlaylist, mLibrary);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbRemoteMediaListBase::GetDistinctValuesForProperty(const nsAString &aPropertyID,
                                                    nsIStringEnumerator **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  LOG_LIST(("sbRemoteMediaListBase::GetDistinctValuesForProperty()"));

  // get enumeration of stuff
  nsCOMPtr<nsIStringEnumerator> enumeration;
  nsresult rv =
    mMediaList->GetDistinctValuesForProperty( aPropertyID,
                                              getter_AddRefs(enumeration) );
  NS_ENSURE_SUCCESS( rv, rv );

  nsRefPtr<sbRemoteWrappingStringEnumerator> wrapped(
    new sbRemoteWrappingStringEnumerator( enumeration, mRemotePlayer ) );
  NS_ENSURE_TRUE( wrapped, NS_ERROR_OUT_OF_MEMORY );

  rv = wrapped->Init();
  NS_ENSURE_SUCCESS( rv, rv );

  NS_ADDREF( *_retval = wrapped );

  return NS_OK;
}

// ---------------------------------------------------------------------------
//
//                        sbIRemoteMediaList
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbRemoteMediaListBase::GetView( sbIMediaListView **aView )
{
  LOG_LIST(("sbRemoteMediaListBase::GetView()"));
  NS_ENSURE_ARG_POINTER(aView);
  NS_ASSERTION(mMediaListView, "No View");
  NS_ADDREF( *aView = mMediaListView );
  return NS_OK;
}


// ---------------------------------------------------------------------------
//
//                    sbIMediaListEnumerationListener
//
// ---------------------------------------------------------------------------

NS_IMPL_ISUPPORTS1(sbMediaListEnumerationListenerWrapper,
                   sbIMediaListEnumerationListener)

sbMediaListEnumerationListenerWrapper::sbMediaListEnumerationListenerWrapper(sbRemotePlayer* aRemotePlayer,
                                                                             sbIMediaListEnumerationListener* aWrapped) :
  mRemotePlayer(aRemotePlayer),
  mWrapped(aWrapped)
{
  NS_ASSERTION(aRemotePlayer, "Null remote player!");
  NS_ASSERTION(aWrapped, "Null wrapped enumerator!");
}

NS_IMETHODIMP
sbMediaListEnumerationListenerWrapper::OnEnumerationBegin(sbIMediaList *aMediaList,
                                                          PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIMediaList> mediaList;
  nsresult rv = SB_WrapMediaList(mRemotePlayer,
                                 aMediaList,
                                 getter_AddRefs(mediaList));
  NS_ENSURE_SUCCESS(rv, rv);

  return mWrapped->OnEnumerationBegin(mediaList, _retval);
}

NS_IMETHODIMP
sbMediaListEnumerationListenerWrapper::OnEnumeratedItem(sbIMediaList *aMediaList,
                                                         sbIMediaItem *aMediaItem,
                                                         PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIMediaList> mediaList;
  nsresult rv = SB_WrapMediaList(mRemotePlayer,
                                 aMediaList,
                                 getter_AddRefs(mediaList));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> mediaItem;
  rv = SB_WrapMediaItem(mRemotePlayer, aMediaItem, getter_AddRefs(mediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  return mWrapped->OnEnumeratedItem(mediaList, mediaItem, _retval);
}

NS_IMETHODIMP
sbMediaListEnumerationListenerWrapper::OnEnumerationEnd(sbIMediaList *aMediaList,
                                                         nsresult aStatusCode)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  nsCOMPtr<sbIMediaList> mediaList;
  nsresult rv = SB_WrapMediaList(mRemotePlayer,
                                 aMediaList,
                                 getter_AddRefs(mediaList));
  NS_ENSURE_SUCCESS(rv, rv);

  return mWrapped->OnEnumerationEnd(mediaList, aStatusCode);;
}

// ---------------------------------------------------------------------------
//
//                          nsISimpleEnumerator
//
// ---------------------------------------------------------------------------

NS_IMPL_ISUPPORTS1(sbUnwrappingSimpleEnumerator, nsISimpleEnumerator)

sbUnwrappingSimpleEnumerator::sbUnwrappingSimpleEnumerator(nsISimpleEnumerator* aWrapped) :
  mWrapped(aWrapped)
{
  NS_ASSERTION(aWrapped, "Null wrapped enumerator!");
}

NS_IMETHODIMP
sbUnwrappingSimpleEnumerator::HasMoreElements(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  return mWrapped->HasMoreElements(_retval);
}

NS_IMETHODIMP
sbUnwrappingSimpleEnumerator::GetNext(nsISupports **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  nsCOMPtr<nsISupports> supports;
  rv = mWrapped->GetNext(getter_AddRefs(supports));
  NS_ENSURE_SUCCESS(rv, rv);

  // if it's an indexed mediaItem, get the wrapped internal item.
  nsCOMPtr<sbIIndexedMediaItem> indexedMediaItem = do_QueryInterface(supports, &rv);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<sbIMediaItem> item;
    indexedMediaItem->GetMediaItem( getter_AddRefs(item) );
    NS_ENSURE_SUCCESS( rv, rv );

    // make the supports that item
    supports = do_QueryInterface( item, &rv );
    NS_ENSURE_SUCCESS( rv, rv );
  }

  nsCOMPtr<sbIWrappedMediaList> mediaList = do_QueryInterface(supports, &rv);
  if (NS_SUCCEEDED(rv)) {
    *_retval = mediaList->GetMediaList().get();
    return NS_OK;
  }

  nsCOMPtr<sbIWrappedMediaItem> mediaItem = do_QueryInterface(supports, &rv);
  if (NS_SUCCEEDED(rv)) {
    *_retval = mediaItem->GetMediaItem().get();
    return NS_OK;
  }

  return NS_ERROR_UNEXPECTED;
}
