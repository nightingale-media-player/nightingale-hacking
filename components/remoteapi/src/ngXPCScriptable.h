/*
 * BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale Media Player.
 *
 * Copyright(c) 2013
 * http://getnightingale.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the "GPL").
 *
 * Software distributed under the License is distributed
 * on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END NIGHTINGALE GPL
 */

#ifndef __NG_XPCSCRIPTABLE_H__
#define __NG_XPCSCRIPTABLE_H__

#include <nsIXPCScriptable.h>


class ngXPCScriptable : public nsIXPCScriptable
{
public:

  virtual ~ngXPCScriptable() {};


  /* GetClassName(char * *aClassName) needs to be implemented by the user */

  /* GetScriptableFlags(PRUint32 *aScriptableFlags) needs to be implemented by the user */

  NS_IMETHOD PreCreate(nsISupports *nativeObj,
                          JSContext *cx,
                          JSObject *globalObj,
                          JSObject **parentObj NS_OUTPARAM)
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD Create(nsIXPConnectWrappedNative *wrapper,
                       JSContext *cx,
                       JSObject *obj)
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD PostCreate(nsIXPConnectWrappedNative *wrapper,
                           JSContext *cx,
                           JSObject *obj)
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  /*
   * AddProperty is called whenever a new property should be added to the object
   * (note that this will be called even if the called was C/C++).
   */
  NS_IMETHOD AddProperty(nsIXPConnectWrappedNative *wrapper,
                            JSContext *cx,
                            JSObject *obj,
                            jsid id,
                            jsval *vp,
                            PRBool *_retval NS_OUTPARAM)
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD DelProperty(nsIXPConnectWrappedNative *wrapper,
                            JSContext *cx,
                            JSObject *obj,
                            jsid id,
                            jsval *vp,
                            PRBool *_retval NS_OUTPARAM)
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  /*
  * GetProperty is called every time JS attempts to get a property; that is this
  * gets invoked a lot and can be used to override properties on the JS object.
  */
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper,
                            JSContext *cx,
                            JSObject *obj,
                            jsid id,
                            jsval *vp,
                            PRBool *_retval NS_OUTPARAM)
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD SetProperty(nsIXPConnectWrappedNative *wrapper,
                            JSContext *cx,
                            JSObject *obj,
                            jsid id,
                            jsval *vp,
                            PRBool *_retval NS_OUTPARAM)
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD Enumerate(nsIXPConnectWrappedNative *wrapper,
                          JSContext *cx,
                          JSObject *obj,
                          PRBool *_retval NS_OUTPARAM)
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }


  /*
   * NewEnumerate is called when JS does a for..in loop to find properties on the
   * object.  It only returns property names not values (see NewResolve below).
   */
  NS_IMETHOD NewEnumerate(nsIXPConnectWrappedNative *wrapper,
                             JSContext *cx,
                             JSObject *obj,
                             PRUint32 enum_op,
                             jsval *statep, jsid *idp NS_OUTPARAM,
                             PRBool *_retval NS_OUTPARAM)
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD NewResolve(nsIXPConnectWrappedNative *wrapper,
                           JSContext *cx,
                           JSObject *obj,
                           jsid id,
                           PRUint32 flags,
                           JSObject **objp NS_OUTPARAM,
                           PRBool *_retval NS_OUTPARAM)
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD Convert(nsIXPConnectWrappedNative *wrapper,
                        JSContext *cx,
                        JSObject *obj,
                        PRUint32 type,
                        jsval *vp,
                        PRBool *_retval NS_OUTPARAM)
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD Finalize(nsIXPConnectWrappedNative *wrapper,
                         JSContext *cx,
                         JSObject *obj)
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD CheckAccess(nsIXPConnectWrappedNative *wrapper,
                            JSContext *cx,
                            JSObject *obj,
                            jsid id,
                            PRUint32 mode,
                            jsval *vp,
                            PRBool *_retval NS_OUTPARAM)
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD Call(nsIXPConnectWrappedNative *wrapper,
                     JSContext *cx,
                     JSObject *obj,
                     PRUint32 argc,
                     jsval *argv,
                     jsval *vp,
                     PRBool *_retval NS_OUTPARAM)
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD Construct(nsIXPConnectWrappedNative *wrapper,
                          JSContext *cx,
                          JSObject *obj,
                          PRUint32 argc,
                          jsval *argv,
                          jsval *vp,
                          PRBool *_retval NS_OUTPARAM)
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD HasInstance(nsIXPConnectWrappedNative *wrapper,
                            JSContext *cx,
                            JSObject *obj,
                            const JS::Value & val,
                            PRBool *bp NS_OUTPARAM,
                            PRBool *_retval NS_OUTPARAM)
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD Trace(nsIXPConnectWrappedNative *wrapper,
                      JSTracer *trc,
                                       JSObject *obj)
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD Equality(nsIXPConnectWrappedNative *wrapper,
                         JSContext *cx,
                         JSObject *obj,
                         const JS::Value & val,
                         PRBool *_retval NS_OUTPARAM)
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD OuterObject(nsIXPConnectWrappedNative *wrapper,
                            JSContext *cx,
                            JSObject *obj,
                            JSObject **_retval NS_OUTPARAM)
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD PostCreatePrototype(JSContext *cx,
                                    JSObject *proto)
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

};


//NS_IMPL_ISUPPORTS1(ngXPCScriptable, nsIXPCScriptable);

#endif // __NG_XPCSCRIPTABLE_H__
