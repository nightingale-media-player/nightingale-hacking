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

#ifndef __SB_XPCSCRIPTABLE_STUB_H__
#define __SB_XPCSCRIPTABLE_STUB_H__

#include <nsIXPCScriptable.h>

/**
 * This is a stub implementation of most of the methods in nsIXPCScriptable
 * note that this is expected to be subclassed and appropriate methods
 * overridden - this doesn't even implement nsISupports
 */
class sbXPCScriptableStub : public nsIXPCScriptable
{
public:
  /* GetClassName(char * *aClassName) needs to be implemented by the user */

  /* GetScriptableFlags(PRUint32 *aScriptableFlags) needs to be implemented by the user */

  NS_IMETHOD PreCreate( nsISupports *nativeObj,
                        JSContext * cx,
                        JSObject * globalObj,
                        JSObject * *parentObj )
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD Create( nsIXPConnectWrappedNative *wrapper,
                     JSContext * cx,
                     JSObject * obj )
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD PostCreate( nsIXPConnectWrappedNative *wrapper,
                         JSContext * cx,
                         JSObject * obj )
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  /*
   * AddProperty is called whenever a new property should be added to the object
   * (note that this will be called even if the called was C/C++).
   */
  NS_IMETHOD AddProperty( nsIXPConnectWrappedNative *wrapper,
                          JSContext * cx,
                          JSObject * obj,
                          jsval id,
                          jsval * vp,
                          PRBool *_retval )
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD DelProperty( nsIXPConnectWrappedNative *wrapper,
                          JSContext * cx,
                          JSObject * obj,
                          jsval id,
                          jsval * vp,
                          PRBool *_retval )
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  /*
   * GetProperty is called every time JS attempts to get a property; that is, this
   * gets invoked a lot and can be used to override properties on the JS object.
   */
  NS_IMETHOD GetProperty( nsIXPConnectWrappedNative *wrapper,
                          JSContext * cx,
                          JSObject * obj,
                          jsval id,
                          jsval * vp,
                          PRBool *_retval )
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD SetProperty( nsIXPConnectWrappedNative *wrapper,
                          JSContext * cx,
                          JSObject * obj,
                          jsval id,
                          jsval * vp,
                          PRBool *_retval )
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD Enumerate( nsIXPConnectWrappedNative *wrapper,
                        JSContext * cx,
                        JSObject * obj,
                        PRBool *_retval )
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  /*
   * NewEnumerate is called when JS does a for..in loop to find properties on the
   * object.  It only returns property names, not values (see NewResolve below).
   */
  NS_IMETHOD NewEnumerate( nsIXPConnectWrappedNative *wrapper,
                           JSContext * cx,
                           JSObject * obj,
                           PRUint32 enum_op,
                           jsval * statep,
                           jsid *idp,
                           PRBool *_retval )
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  /*
   * NewResolve is invoked when the JS engine needs to fetch a property (before
   * GetProperty is called).  It it used to find where in the prototype chain a
   * given property exists.
   */
  NS_IMETHOD NewResolve( nsIXPConnectWrappedNative *wrapper,
                         JSContext * cx,
                         JSObject * obj,
                         jsval id,
                         PRUint32 flags,
                         JSObject * *objp,
                         PRBool *_retval )
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD Convert( nsIXPConnectWrappedNative *wrapper,
                      JSContext * cx,
                      JSObject * obj,
                      PRUint32 type,
                      jsval * vp,
                      PRBool *_retval )
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD Finalize( nsIXPConnectWrappedNative *wrapper,
                       JSContext * cx,
                       JSObject * obj )
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  /* XXX Mook: I can't figure out when the heck CheckAccess gets called :( */
  NS_IMETHOD CheckAccess( nsIXPConnectWrappedNative *wrapper,
                          JSContext * cx,
                          JSObject * obj,
                          jsval id,
                          PRUint32 mode,
                          jsval * vp,
                          PRBool *_retval )
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  /* [[Call]] is invoked when the JS object is being called as a function
   * (i.e. similiar to a C++ functor object)
   */
  NS_IMETHOD Call( nsIXPConnectWrappedNative *wrapper,
                   JSContext * cx,
                   JSObject * obj,
                   PRUint32 argc,
                   jsval * argv,
                   jsval * vp,
                   PRBool *_retval )
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD Construct( nsIXPConnectWrappedNative *wrapper,
                        JSContext * cx,
                        JSObject * obj,
                        PRUint32 argc,
                        jsval * argv,
                        jsval * vp,
                        PRBool *_retval )
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD HasInstance( nsIXPConnectWrappedNative *wrapper,
                          JSContext * cx,
                          JSObject * obj,
                          jsval val,
                          PRBool *bp,
                          PRBool *_retval )
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD Trace( nsIXPConnectWrappedNative *wrapper,
                    JSTracer * trc,
                    JSObject * obj )
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD Equality( nsIXPConnectWrappedNative *wrapper,
                       JSContext * cx,
                       JSObject * obj,
                       jsval val,
                       PRBool *_retval )
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD OuterObject( nsIXPConnectWrappedNative *wrapper,
                          JSContext * cx,
                          JSObject * obj,
                          JSObject * *_retval )
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD InnerObject( nsIXPConnectWrappedNative *wrapper,
                          JSContext * cx,
                          JSObject * obj,
                          JSObject * *_retval )
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
};

#endif // __SB_XPCSCRIPTABLE_STUB_H__

