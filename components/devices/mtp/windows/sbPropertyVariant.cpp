/* vim: set sw=2 :miv */
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

#include "sbPropertyVariant.h"
#include <nsIClassInfoImpl.h>
#include <nsISupportsPrimitives.h>
#include <nsIProgrammingLanguage.h>
#include <nsStringAPI.h>
#include <nsMemory.h>

#ifndef WINVER              // Allow use of features specific to Windows 95 and Windows NT 4 or later.
#define WINVER 0x0600       // Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif

#ifndef _WIN32_WINNT        // Allow use of features specific to Windows NT 4 or later.
#define _WIN32_WINNT 0x0600     // Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif

#ifndef _WIN32_WINDOWS      // Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0600 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE           // Allow use of features specific to IE 4.0 or later.
#define _WIN32_IE 0x0400    // Change this to the appropriate value to target IE 5.0 or later.
#endif

#include <atlbase.h>
#include <wtypes.h>

NS_IMPL_THREADSAFE_ISUPPORTS1(sbPropertyVariant,
                              nsIVariant)
NS_IMPL_CI_INTERFACE_GETTER1(sbPropertyVariant,
                             nsIVariant)
NS_DECL_CLASSINFO(sbPropertyVariant)
NS_IMPL_THREADSAFE_CI(sbPropertyVariant)


sbPropertyVariant::~sbPropertyVariant()
{
  PropVariantClear(&mPropVariant);
}

sbPropertyVariant::TypeType sbPropertyVariant::AssertSupportedType(PRUint16 dataType)
{
  PRUint16 ourDataType;
  
  bool const sameType = NS_SUCCEEDED(GetDataType(&ourDataType)) && ourDataType == dataType;
  NS_ASSERTION(sameType, "An unsupported type was requested from sbPropertyVariant");
  if (sameType) {
    return mPropVariant.vt & VT_BYREF ?
        ByRef :
        ByVal;
  }
  return Unsupported;
}

/* [noscript] readonly attribute PRUint16 dataType; */
NS_IMETHODIMP sbPropertyVariant::GetDataType(PRUint16 *aDataType)
{
  nsresult rv = NS_OK;
  NS_ENSURE_ARG(aDataType);
  switch (mPropVariant.vt & VT_TYPEMASK)
  {
  case VT_EMPTY:
    *aDataType = nsIDataType::VTYPE_EMPTY;
    break;
  case VT_NULL:
    *aDataType = nsIDataType::VTYPE_VOID;
    break;
  case VT_I1:
    *aDataType = nsIDataType::VTYPE_INT8;
    break;
  case VT_UI1:
    *aDataType = nsIDataType::VTYPE_UINT8;
    break;
  case VT_I2:
    *aDataType = nsIDataType::VTYPE_INT16;
    break;
  case VT_UI2:
    *aDataType = nsIDataType::VTYPE_UINT16;
    break;
  case VT_I4:
  case VT_INT:
    *aDataType = nsIDataType::VTYPE_INT32;
    break;
  case VT_UI4:
  case VT_UINT:
    *aDataType = nsIDataType::VTYPE_UINT32;
    break;
  case VT_I8:
    *aDataType = nsIDataType::VTYPE_INT64;
    break;
  case VT_UI8:
    *aDataType = nsIDataType::VTYPE_UINT64;
    break;
  case VT_R4:
    *aDataType = nsIDataType::VTYPE_FLOAT;
    break;
  case VT_R8:
    *aDataType = nsIDataType::VTYPE_DOUBLE;
    break;
  case VT_BOOL:
    *aDataType = nsIDataType::VTYPE_BOOL;
    break;
  case VT_ERROR:
    *aDataType = nsIDataType::VTYPE_UINT32;
    break;
  case VT_DATE:
    *aDataType = nsIDataType::VTYPE_DOUBLE;
    break;
  case VT_FILETIME:
    *aDataType = nsIDataType::VTYPE_UINT64;
    break;
  case VT_CLSID:
    *aDataType = nsIDataType::VTYPE_ID;
    break;
  case VT_BSTR:
    *aDataType = nsIDataType::VTYPE_ASTRING;
    break;
  case VT_LPSTR:
    *aDataType = nsIDataType::VTYPE_CSTRING;
    break;
  case VT_LPWSTR:
    *aDataType = nsIDataType::VTYPE_ASTRING;
    break;
  case VT_CY:
  case VT_CF:
  case VT_BSTR_BLOB:
  case VT_BLOB:
//  case VT_BLOBOBJECT:
  case VT_UNKNOWN:
  case VT_DISPATCH:
  case VT_STREAM:
  case VT_STREAMED_OBJECT:
  case VT_STORAGE:
  case VT_STORED_OBJECT:
  case VT_VERSIONED_STREAM:
  case VT_DECIMAL:
  case VT_VECTOR:
  default:
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  return rv;
}

#define ReturnValue(type, name)           \
  switch (AssertSupportedType(type))      \
  {                                       \
    case ByVal:                           \
      *retval = mPropVariant.name;        \
      return NS_OK;                       \
    case ByRef:                           \
      *retval = *(mPropVariant.p##name);  \
      return NS_OK;                       \
    default:                              \
      return NS_ERROR_NOT_IMPLEMENTED;    \
  }

#define ReturnValueNP(type, name)         \
  switch (AssertSupportedType(type))      \
  {                                       \
    case ByVal:                           \
      *retval = mPropVariant.name;        \
      return NS_OK;                       \
    case ByRef:                           \
    default:                              \
      return NS_ERROR_NOT_IMPLEMENTED;    \
  }

/* [noscript] PRUint8 getAsInt8 (); */
NS_IMETHODIMP sbPropertyVariant::GetAsInt8(PRUint8 *retval)
{
  ReturnValue(nsIDataType::VTYPE_INT8, cVal);
}

/* [noscript] PRInt16 getAsInt16 (); */
NS_IMETHODIMP sbPropertyVariant::GetAsInt16(PRInt16 *retval)
{
    ReturnValue(nsIDataType::VTYPE_INT16, iVal);
}

/* [noscript] PRInt32 getAsInt32 (); */
NS_IMETHODIMP sbPropertyVariant::GetAsInt32(PRInt32 *retval)
{
  ReturnValue(nsIDataType::VTYPE_INT32, lVal);
}

/* [noscript] PRInt64 getAsInt64 (); */
NS_IMETHODIMP sbPropertyVariant::GetAsInt64(PRInt64 *rv)
{
  LARGE_INTEGER * retval = reinterpret_cast<LARGE_INTEGER*>(rv);
  ReturnValueNP(nsIDataType::VTYPE_INT64, hVal);
}

/* [noscript] PRUint8 getAsUint8 (); */
NS_IMETHODIMP sbPropertyVariant::GetAsUint8(PRUint8 *retval)
{
  ReturnValue(nsIDataType::VTYPE_UINT8, bVal);
}

/* [noscript] PRUint16 getAsUint16 (); */
NS_IMETHODIMP sbPropertyVariant::GetAsUint16(PRUint16 *retval)
{
  ReturnValue(nsIDataType::VTYPE_UINT16, uiVal);
}

/* [noscript] PRUint32 getAsUint32 (); */
NS_IMETHODIMP sbPropertyVariant::GetAsUint32(PRUint32 *retval)
{
  ReturnValue(nsIDataType::VTYPE_UINT32, ulVal);
}

/* [noscript] PRUint64 getAsUint64 (); */
NS_IMETHODIMP sbPropertyVariant::GetAsUint64(PRUint64 *rv)
{
  ULARGE_INTEGER * retval = reinterpret_cast<ULARGE_INTEGER*>(rv);
  ReturnValueNP(nsIDataType::VTYPE_UINT64, uhVal);
}

/* [noscript] float getAsFloat (); */
NS_IMETHODIMP sbPropertyVariant::GetAsFloat(float *retval)
{
  ReturnValue(nsIDataType::VTYPE_FLOAT, fltVal);
}

/* [noscript] double getAsDouble (); */
NS_IMETHODIMP sbPropertyVariant::GetAsDouble(double *retval)
{
  ReturnValue(nsIDataType::VTYPE_DOUBLE, dblVal);
}

/* [noscript] PRBool getAsBool (); */
NS_IMETHODIMP sbPropertyVariant::GetAsBool(PRBool *retval)
{
  ReturnValue(nsIDataType::VTYPE_BOOL, boolVal);
}

/* [noscript] char getAsChar (); */
NS_IMETHODIMP sbPropertyVariant::GetAsChar(char *retval)
{
  ReturnValue(nsIDataType::VTYPE_CHAR, cVal);
}

/* [noscript] wchar getAsWChar (); */
NS_IMETHODIMP sbPropertyVariant::GetAsWChar(PRUnichar *retval)
{
  ReturnValue(nsIDataType::VTYPE_INT32, lVal);
}

/* [notxpcom] nsresult getAsID (out nsID retval); */
NS_IMETHODIMP_(nsresult) sbPropertyVariant::GetAsID(nsID *retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] AString getAsAString (); */
NS_IMETHODIMP sbPropertyVariant::GetAsAString(nsAString & retval)
{
  switch (AssertSupportedType(nsIDataType::VTYPE_ASTRING))
  {
    case ByVal:
      switch (mPropVariant.vt & VT_TYPEMASK)
      {
        case VT_BSTR:
        {
          CComBSTR temp(mPropVariant.bstrVal);
          retval = nsString(mPropVariant.bstrVal, temp.Length());
        }
        break;
        case VT_LPWSTR:
          retval = mPropVariant.pwszVal;
          break;
        default:
          return NS_ERROR_NOT_IMPLEMENTED;
      }
      return NS_OK;
    case ByRef:
      switch (mPropVariant.vt & VT_TYPEMASK)
      {
        case VT_BSTR:
        {
          CComBSTR temp(*(mPropVariant.pbstrVal));
          retval = nsString(*(mPropVariant.pbstrVal), temp.Length());
        }
        break;
        case VT_LPWSTR:
        default:
          return NS_ERROR_NOT_IMPLEMENTED;
      }
      return NS_OK;
    default:
      return NS_ERROR_NOT_IMPLEMENTED;
  }
}
/* [noscript] DOMString getAsDOMString (); */
NS_IMETHODIMP sbPropertyVariant::GetAsDOMString(nsAString & retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] ACString getAsACString (); */
NS_IMETHODIMP sbPropertyVariant::GetAsACString(nsACString & retval)
{
  switch (AssertSupportedType(nsIDataType::VTYPE_CSTRING))
  {
    case ByVal:
      retval = mPropVariant.pszVal;
      break;
    case ByRef:
    default:
      return NS_ERROR_NOT_IMPLEMENTED;
  }
  return NS_OK;
}

/* [noscript] AUTF8String getAsAUTF8String (); */
NS_IMETHODIMP sbPropertyVariant::GetAsAUTF8String(nsACString & retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] string getAsString (); */
NS_IMETHODIMP sbPropertyVariant::GetAsString(char **retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] wstring getAsWString (); */
NS_IMETHODIMP sbPropertyVariant::GetAsWString(PRUnichar **retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] nsISupports getAsISupports (); */
NS_IMETHODIMP sbPropertyVariant::GetAsISupports(nsISupports **retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void getAsInterface (out nsIIDPtr iid, [iid_is (iid), retval] out nsQIResult iface); */
NS_IMETHODIMP sbPropertyVariant::GetAsInterface(nsIID * *iid, void * *iface)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [notxpcom] nsresult getAsArray (out PRUint16 type, out nsIID iid, out PRUint32 count, out voidPtr ptr); */
NS_IMETHODIMP_(nsresult) sbPropertyVariant::GetAsArray(PRUint16 *type, nsIID *iid, PRUint32 *count, void * *ptr)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void getAsStringWithSize (out PRUint32 size, [size_is (size), retval] out string str); */
NS_IMETHODIMP sbPropertyVariant::GetAsStringWithSize(PRUint32 *size, char **str)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void getAsWStringWithSize (out PRUint32 size, [size_is (size), retval] out wstring str); */
NS_IMETHODIMP sbPropertyVariant::GetAsWStringWithSize(PRUint32 *size, PRUnichar **str)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
