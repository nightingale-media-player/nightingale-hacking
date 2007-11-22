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

#include "sbScriptableFunction.h"

#include <sbClassInfoUtils.h>

#include <nsMemory.h>
#include <nsIClassInfoImpl.h>
#include <nsStringGlue.h>

#include <jsapi.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbScriptableFunction:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gScriptableFunctionLog = nsnull;
#endif

#undef LOG
#define LOG(args) PR_LOG(gScriptableFunctionLog, PR_LOG_WARN, args)
#define TRACE(args) PR_LOG(gScriptableFunctionLog, PR_LOG_DEBUG, args)

NS_IMPL_ISUPPORTS2_CI( sbScriptableFunction,
                       nsISecurityCheckedComponent,
                       nsIXPCScriptable )

sbScriptableFunction::sbScriptableFunction( nsISupports *aObject,
                                            const nsIID& aIID )
  : mObject(aObject),
    mIID(aIID)
{
#ifdef PR_LOGGING
  if (!gScriptableFunctionLog) {
    gScriptableFunctionLog = PR_NewLogModule( "sbScriptableFunction" );
  }
#endif
  TRACE(("sbScriptableFunction::sbScriptableFunction()"));
}

sbScriptableFunction::~sbScriptableFunction()
{
}

// ---------------------------------------------------------------------------
//
//                          nsIXPCScriptable
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP sbScriptableFunction::GetClassName(char * *aClassName)
{
  NS_ENSURE_ARG_POINTER(aClassName);
  
  NS_NAMED_LITERAL_CSTRING( kClassName, "sbScriptableFunction" );
  *aClassName = ToNewCString(kClassName);
  return aClassName ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

/* readonly attribute PRUint32 scriptableFlags; */
NS_IMETHODIMP sbScriptableFunction::GetScriptableFlags(PRUint32 *aScriptableFlags)
{
  NS_ENSURE_ARG_POINTER(aScriptableFlags);
  // XXX Mook: USE_JSSTUB_FOR_ADDPROPERTY is needed to define things on the
  //           prototype properly; even with it set scripts cannot add
  //           properties onto the object (because they're not allow to *set*)
  *aScriptableFlags = DONT_ENUM_STATIC_PROPS |
                      DONT_ENUM_QUERY_INTERFACE |
                      WANT_CALL |
                      DONT_REFLECT_INTERFACE_NAMES ;
  return NS_OK;
}

NS_IMETHODIMP sbScriptableFunction::Call( nsIXPConnectWrappedNative *wrapper,
                                          JSContext * cx,
                                          JSObject * obj,
                                          PRUint32 argc,
                                          jsval * argv,
                                          jsval * vp,
                                          PRBool *_retval)
{
  TRACE(("sbScriptableFunction::Call()"));
  
  // this can get called because library.getArtists should map to a function
  // that returns library.artists, but instead it returns the object itself.
  // So we make the object callable and return itself.
  NS_ENSURE_ARG_POINTER(obj);
  NS_ENSURE_ARG_POINTER(vp);
  NS_ENSURE_ARG_POINTER(_retval);
  
  nsresult rv;
  
  nsCOMPtr<nsIXPConnect> xpc;
  rv = wrapper->GetXPConnect( getter_AddRefs(xpc) );
  NS_ENSURE_SUCCESS( rv, rv );
  
  nsCOMPtr<nsIXPConnectJSObjectHolder> objectHolder;
  rv = xpc->WrapNative( cx, obj, mObject, mIID, getter_AddRefs(objectHolder) );
  NS_ENSURE_SUCCESS( rv, rv );
  
  JSObject* object;
  rv = objectHolder->GetJSObject( &object );
  NS_ENSURE_SUCCESS( rv, rv );
  
  *vp = OBJECT_TO_JSVAL(object);
  *_retval = PR_TRUE;
  
  return NS_OK;
}

// ---------------------------------------------------------------------------
//
//                          nsISecurityCheckedComponent
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP sbScriptableFunction::CanCreateWrapper( const nsIID * iid,
                                                     char **_retval)
{
  TRACE(("sbScriptableFunction::CanCreateWrapper()"));

  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = ToNewCString( NS_LITERAL_CSTRING("AllAccess") );
  return NS_OK;
}

NS_IMETHODIMP sbScriptableFunction::CanCallMethod( const nsIID * iid,
                                                  const PRUnichar *methodName,
                                                  char **_retval)
{
  TRACE(("sbScriptableFunction::CanCallMethod() - %s",
         NS_LossyConvertUTF16toASCII(methodName).BeginReading()));

  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = ToNewCString( NS_LITERAL_CSTRING("AllAccess") );
  return NS_OK;
}

NS_IMETHODIMP sbScriptableFunction::CanGetProperty( const nsIID * iid,
                                                   const PRUnichar *propertyName,
                                                   char **_retval)
{
  TRACE(("sbScriptableFunction::CanGetProperty() - %s",
         NS_LossyConvertUTF16toASCII(propertyName).BeginReading()));

  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = ToNewCString( NS_LITERAL_CSTRING("AllAccess") );
  return NS_OK;
}

NS_IMETHODIMP sbScriptableFunction::CanSetProperty( const nsIID * iid,
                                                   const PRUnichar *propertyName,
                                                   char **_retval)
{
  TRACE(("sbScriptableFunction::CanSetProperty() - %s",
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
NS_DECL_CLASSINFO(sbScriptableFunction)

SB_IMPL_CLASSINFO_INTERFACES_ONLY(sbScriptableFunction)
