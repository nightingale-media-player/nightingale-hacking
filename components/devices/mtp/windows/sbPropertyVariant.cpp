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
#include "sbWPDCommon.h"
#include <nsIClassInfoImpl.h>
#include <nsISupportsPrimitives.h>
#include <nsIProgrammingLanguage.h>
#include <nsCOMPtr.h>
#include <nsStringAPI.h>
#include <nsMemory.h>
#include <string>
#include <sstream>
#include <limits>

NS_IMPL_THREADSAFE_ISUPPORTS2(sbPropertyVariant,
                              nsIWritableVariant,
                              nsIVariant)
NS_IMPL_CI_INTERFACE_GETTER2(sbPropertyVariant,
                             nsIWritableVariant,
                             nsIVariant)
NS_DECL_CLASSINFO(sbPropertyVariant)
NS_IMPL_THREADSAFE_CI(sbPropertyVariant)

sbPropertyVariant * sbPropertyVariant::New()
{
  return new sbPropertyVariant;
}
sbPropertyVariant * sbPropertyVariant::New(PROPVARIANT const & propVar)
{
  return new sbPropertyVariant(propVar);
}

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

#define GET_VALUE(v) (byRef ? *(mPropVariant.p##v) : mPropVariant.v) 

/* [noscript] double getAsDouble (); */
NS_IMETHODIMP sbPropertyVariant::GetAsDouble(double *retval)
{
  PRBool const byRef = mPropVariant.vt & VT_BYREF ? PR_TRUE : PR_FALSE;
  switch (mPropVariant.vt & VT_TYPEMASK)
  {
  case VT_EMPTY:
    *retval = std::numeric_limits<double>::quiet_NaN();
    break;
  case VT_NULL:
    *retval = std::numeric_limits<double>::quiet_NaN();
    break;
  case VT_I1:
    *retval = static_cast<double>(GET_VALUE(cVal));
    break;
  case VT_UI1:
    *retval = static_cast<double>(GET_VALUE(bVal));
    break;
  case VT_I2:
    *retval = static_cast<double>(GET_VALUE(iVal));
    break;
  case VT_UI2:
    *retval = static_cast<double>(GET_VALUE(uiVal));
    break;
  case VT_I4:
  case VT_INT:
    *retval = static_cast<double>(GET_VALUE(lVal));
    break;
  case VT_UI4:
  case VT_UINT:
    *retval = static_cast<double>(GET_VALUE(ulVal));
    break;
  case VT_I8:
    *retval = static_cast<double>(mPropVariant.hVal.QuadPart);
    break;
  case VT_UI8:
    *retval = static_cast<double>(mPropVariant.uhVal.QuadPart);
    break;
  case VT_R4:
    *retval = static_cast<double>(GET_VALUE(fltVal));
    break;
  case VT_R8:
    *retval = static_cast<double>(GET_VALUE(dblVal));
    break;
  case VT_BOOL:
    *retval = static_cast<double>(GET_VALUE(boolVal));
    break;
  case VT_ERROR:
    *retval = static_cast<double>(GET_VALUE(scode));
    break;
  case VT_DATE:
    *retval = static_cast<double>(GET_VALUE(date));
    break;
  case VT_FILETIME:
    NS_ERROR("Cannot convert file times to double");
    return NS_ERROR_NOT_IMPLEMENTED;
  case VT_CLSID:
    NS_ERROR("Cannot convert UUID's to double");
    return NS_ERROR_NOT_IMPLEMENTED;
  case VT_BSTR:
  {
    NS_ERROR("Cannot convert BSTR to double");
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  break;
  case VT_LPWSTR:
    NS_ERROR("Cannot convert strings to double");
    return NS_ERROR_NOT_IMPLEMENTED;
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
    NS_ERROR("Unable to convert COM type to double");
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  return NS_OK;
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
  ReturnValue(nsIDataType::VTYPE_INT16, lVal);
}

/* [notxpcom] nsresult getAsID (out nsID retval); */
NS_IMETHODIMP_(nsresult) sbPropertyVariant::GetAsID(nsID *retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

template <class T>
nsString ConvertTonsString(T val)
{
  std::wostringstream buffer;
  buffer << val;
  return nsString(buffer.str().c_str());
}

template <class T>
nsString ConvertTonsString(__int64 val)
{
  PRUinchar buffer[128];
  _snwprintf_s(buffer, sizeof(buffer), 1, "%I64i", val);
  return nsString(buffer);
}

template <class T>
nsString ConvertTonsString(unsigned __int64 val)
{
  PRUinchar buffer[128];
  _snwprintf_s(buffer, sizeof(buffer), 1, "%I64u", val);
  return nsString(buffer);
}

/* [noscript] AString getAsAString (); */
NS_IMETHODIMP sbPropertyVariant::GetAsAString(nsAString & retval)
{
  PRBool const byRef = mPropVariant.vt & VT_BYREF ? PR_TRUE : PR_FALSE;
  switch (mPropVariant.vt & VT_TYPEMASK)
  {
  case VT_EMPTY:
    retval = nsString();
    break;
  case VT_NULL:
    retval.Truncate();
    retval.SetIsVoid(PR_TRUE);
    break;
  case VT_I1:
    retval = ConvertTonsString(GET_VALUE(cVal));
    break;
  case VT_UI1:
    retval = ConvertTonsString(GET_VALUE(bVal));
    break;
  case VT_I2:
    retval = ConvertTonsString(GET_VALUE(iVal));
    break;
  case VT_UI2:
    retval = ConvertTonsString(GET_VALUE(uiVal));
    break;
  case VT_I4:
  case VT_INT:
    retval = ConvertTonsString(GET_VALUE(lVal));
    break;
  case VT_UI4:
  case VT_UINT:
    retval = ConvertTonsString(GET_VALUE(ulVal));
    break;
  case VT_I8:
    retval = ConvertTonsString(mPropVariant.hVal.QuadPart);
    break;
  case VT_UI8:
    retval = ConvertTonsString(mPropVariant.uhVal.QuadPart);
    break;
  case VT_R4:
    retval = ConvertTonsString(GET_VALUE(fltVal));
    break;
  case VT_R8:
    retval = ConvertTonsString(GET_VALUE(dblVal));
    break;
  case VT_BOOL:
    retval = ConvertTonsString(GET_VALUE(boolVal));
    break;
  case VT_ERROR:
    retval = ConvertTonsString(GET_VALUE(scode));
    break;
  case VT_DATE:
    retval = ConvertTonsString(GET_VALUE(date));
    break;
  case VT_FILETIME:
    NS_ERROR("Cannot convert file times to string");
    return NS_ERROR_NOT_IMPLEMENTED;
  case VT_CLSID:
    NS_ERROR("Cannot convert UUID's to string");
    return NS_ERROR_NOT_IMPLEMENTED;
  case VT_BSTR:
  {
    PRUint32 const length = ::SysStringLen(GET_VALUE(bstrVal));
    retval = nsString(GET_VALUE(bstrVal), length);
  }
  break;
  case VT_LPWSTR:
    NS_ASSERTION(!byRef, "LPWSTR can't be passed by reference");
    retval = mPropVariant.pwszVal;
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
    NS_ERROR("Unable to convert COM type to string");
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  return NS_OK;
}
/* [noscript] DOMString getAsDOMString (); */
NS_IMETHODIMP sbPropertyVariant::GetAsDOMString(nsAString & retval)
{
    return GetAsAString(retval);
}

/* [noscript] ACString getAsACString (); */
NS_IMETHODIMP sbPropertyVariant::GetAsACString(nsACString & retval)
{
  nsString str;
  nsresult rv = GetAsAString(str);
  NS_ENSURE_SUCCESS(rv, rv);
  retval = NS_LossyConvertUTF16toASCII(str);
  return NS_OK;
}

/* [noscript] AUTF8String getAsAUTF8String (); */
NS_IMETHODIMP sbPropertyVariant::GetAsAUTF8String(nsACString & retval)
{
  nsString str;
  nsresult rv = GetAsAString(str);
  NS_ENSURE_SUCCESS(rv,rv);
  retval = NS_ConvertUTF16toUTF8 (str);
  return NS_OK;
}

/* [noscript] string getAsString (); */
NS_IMETHODIMP sbPropertyVariant::GetAsString(char **retval)
{
  nsCString str;
  nsresult rv = GetAsACString(str);
  NS_ENSURE_SUCCESS(rv, rv);
  PRInt32 const length = str.Length();
  *retval = reinterpret_cast<char*>(NS_Alloc(length + 1));
  if (!*retval)
    return NS_ERROR_OUT_OF_MEMORY;
  memcpy(*retval, str.get(), length);
  (*retval)[length] = 0;
  return NS_OK;
}

/* [noscript] wstring getAsWString (); */
NS_IMETHODIMP sbPropertyVariant::GetAsWString(PRUnichar **retval)
{
  nsString str;
  nsresult rv = GetAsAString(str);
  NS_ENSURE_SUCCESS(rv, rv);
  PRInt32 const length = str.Length();
  *retval = reinterpret_cast<PRUnichar*>(NS_Alloc((length + 1)* sizeof(PRUnichar)));
  if (!*retval)
    return NS_ERROR_OUT_OF_MEMORY;
  memcpy(*retval, str.get(), length * sizeof(PRUnichar));
  (*retval)[length] = 0;
  return NS_OK;
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


/*** nsIWritableVariant ***/


#define SetValueByVal(type, field, value) \
  PR_BEGIN_MACRO                          \
  mPropVariant.vt = type;                 \
  mPropVariant.field = value;             \
  return NS_OK;                           \
  PR_END_MACRO

/* attribute PRBool writable; */
NS_IMETHODIMP sbPropertyVariant::GetWritable(PRBool *aWritable)
{
  NS_ENSURE_ARG_POINTER(aWritable);
  *aWritable = PR_TRUE;
  return NS_OK;
}
NS_IMETHODIMP sbPropertyVariant::SetWritable(PRBool aWritable)
{
  /* we don't support *setting* it to not writable (I'm lazy) */
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setAsInt8 (in PRUint8 aValue); */
NS_IMETHODIMP sbPropertyVariant::SetAsInt8(PRUint8 aValue)
{
  /* wtf, this is unsigned! */
  SetValueByVal(VT_UI1, bVal, aValue);
}

/* void setAsInt16 (in PRInt16 aValue); */
NS_IMETHODIMP sbPropertyVariant::SetAsInt16(PRInt16 aValue)
{
  SetValueByVal(VT_I2, iVal, aValue);
}

/* void setAsInt32 (in PRInt32 aValue); */
NS_IMETHODIMP sbPropertyVariant::SetAsInt32(PRInt32 aValue)
{
  SetValueByVal(VT_I4, lVal, aValue);
}

/* void setAsInt64 (in PRInt64 aValue); */
NS_IMETHODIMP sbPropertyVariant::SetAsInt64(PRInt64 aValue)
{
  SetValueByVal(VT_I8, hVal.QuadPart, aValue);
}

/* void setAsUint8 (in PRUint8 aValue); */
NS_IMETHODIMP sbPropertyVariant::SetAsUint8(PRUint8 aValue)
{
  SetValueByVal(VT_UI1, bVal, aValue);
}

/* void setAsUint16 (in PRUint16 aValue); */
NS_IMETHODIMP sbPropertyVariant::SetAsUint16(PRUint16 aValue)
{
  SetValueByVal(VT_UI2, uiVal, aValue);
}

/* void setAsUint32 (in PRUint32 aValue); */
NS_IMETHODIMP sbPropertyVariant::SetAsUint32(PRUint32 aValue)
{
  SetValueByVal(VT_UI4, ulVal, aValue);
}

/* void setAsUint64 (in PRUint64 aValue); */
NS_IMETHODIMP sbPropertyVariant::SetAsUint64(PRUint64 aValue)
{
  SetValueByVal(VT_UI8, uhVal.QuadPart, aValue);
}

/* void setAsFloat (in float aValue); */
NS_IMETHODIMP sbPropertyVariant::SetAsFloat(float aValue)
{
  SetValueByVal(VT_R4, fltVal, aValue);
}

/* void setAsDouble (in double aValue); */
NS_IMETHODIMP sbPropertyVariant::SetAsDouble(double aValue)
{
  SetValueByVal(VT_R8, dblVal, aValue);
}

/* void setAsBool (in PRBool aValue); */
NS_IMETHODIMP sbPropertyVariant::SetAsBool(PRBool aValue)
{
  SetValueByVal(VT_BOOL, boolVal, aValue);
}

/* void setAsChar (in char aValue); */
NS_IMETHODIMP sbPropertyVariant::SetAsChar(char aValue)
{
  SetValueByVal(VT_I1, cVal, aValue);
}

/* void setAsWChar (in wchar aValue); */
NS_IMETHODIMP sbPropertyVariant::SetAsWChar(PRUnichar aValue)
{
  SetValueByVal(VT_UI2, uiVal, aValue);
}

/* void setAsID (in nsIDRef aValue); */
NS_IMETHODIMP sbPropertyVariant::SetAsID(const nsID & aValue)
{
  HRESULT hr = PropVariantClear(&mPropVariant);
  if (FAILED(hr))
    return NS_ERROR_FAILURE;
  mPropVariant.puuid = (CLSID*)::CoTaskMemAlloc(sizeof(CLSID));
  if (!mPropVariant.puuid)
    return NS_ERROR_OUT_OF_MEMORY;
  memcpy(mPropVariant.puuid, &aValue, sizeof(CLSID));
  mPropVariant.vt = VT_CLSID;
  return NS_OK;
}

/* void setAsAString (in AString aValue); */
NS_IMETHODIMP sbPropertyVariant::SetAsAString(const nsAString & aValue)
{
  return SetAsWStringWithSize(aValue.Length(), aValue.BeginReading());
}

/* void setAsDOMString (in DOMString aValue); */
NS_IMETHODIMP sbPropertyVariant::SetAsDOMString(const nsAString & aValue)
{
  return SetAsAString(aValue);
}

/* void setAsACString (in ACString aValue); */
NS_IMETHODIMP sbPropertyVariant::SetAsACString(const nsACString & aValue)
{
  return SetAsAString(NS_ConvertASCIItoUTF16(aValue));
}

/* void setAsAUTF8String (in AUTF8String aValue); */
NS_IMETHODIMP sbPropertyVariant::SetAsAUTF8String(const nsACString & aValue)
{
  return SetAsAString(NS_ConvertUTF8toUTF16(aValue));
}

/* void setAsString (in string aValue); */
NS_IMETHODIMP sbPropertyVariant::SetAsString(const char *aValue)
{
  HRESULT hr = PropVariantClear(&mPropVariant);
  if (FAILED(hr))
    return NS_ERROR_FAILURE;
  size_t len = strlen(aValue);
  mPropVariant.pszVal = (LPSTR)::CoTaskMemAlloc(len);
  if (!mPropVariant.pszVal)
    return NS_ERROR_OUT_OF_MEMORY;
  memcpy(mPropVariant.pszVal, aValue, len);
  mPropVariant.vt = VT_LPSTR;
  return NS_OK;
}

/* void setAsWString (in wstring aValue); */
NS_IMETHODIMP sbPropertyVariant::SetAsWString(const PRUnichar *aValue)
{
  HRESULT hr = PropVariantClear(&mPropVariant);
  if (FAILED(hr))
    return NS_ERROR_FAILURE;
  size_t const len = wcslen(aValue);
  size_t const bytes = (len + 1) * sizeof(PRUnichar);
  mPropVariant.pwszVal = reinterpret_cast<LPWSTR>(::CoTaskMemAlloc(bytes));
  if (!mPropVariant.pwszVal)
    return NS_ERROR_OUT_OF_MEMORY;
  memcpy(mPropVariant.pwszVal, aValue, bytes);
  mPropVariant.vt = VT_LPWSTR;
  return NS_OK;
}

/* void setAsISupports (in nsISupports aValue); */
NS_IMETHODIMP sbPropertyVariant::SetAsISupports(nsISupports *aValue)
{
  HRESULT hr = PropVariantClear(&mPropVariant);
  if (FAILED(hr))
    return NS_ERROR_FAILURE;
  nsCOMPtr<nsISupports> supports = do_QueryInterface(aValue);
  NS_ENSURE_ARG_POINTER(aValue); // umm, you better QI to nsISupports
  supports.forget((nsISupports**)&mPropVariant.punkVal);
  mPropVariant.vt = VT_UNKNOWN;
  return NS_OK;
}

/* void setAsInterface (in nsIIDRef iid, [iid_is (iid)] in nsQIResult iface); */
NS_IMETHODIMP sbPropertyVariant::SetAsInterface(const nsIID & iid, void * iface)
{
  /* no equivalent (can't store the IID anywhere) */
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void setAsArray (in PRUint16 type, in nsIIDPtr iid, in PRUint32 count, in voidPtr ptr); */
NS_IMETHODIMP sbPropertyVariant::SetAsArray(PRUint16 type, const nsIID * iid, PRUint32 count, void * ptr)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setAsStringWithSize (in PRUint32 size, [size_is (size)] in string str); */
NS_IMETHODIMP sbPropertyVariant::SetAsStringWithSize(PRUint32 size, const char *str)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setAsWStringWithSize (in PRUint32 size, [size_is (size)] in wstring str); */
NS_IMETHODIMP sbPropertyVariant::SetAsWStringWithSize(PRUint32 size, const PRUnichar *str)
{
  HRESULT hr = PropVariantClear(&mPropVariant);
  if (FAILED(hr))
    return NS_ERROR_FAILURE;
  mPropVariant.bstrVal = ::SysAllocStringLen(str, size);
  if (!mPropVariant.bstrVal)
    return NS_ERROR_OUT_OF_MEMORY;
  mPropVariant.vt = VT_BSTR;
  return NS_OK;
}

/* void setAsVoid (); */
NS_IMETHODIMP sbPropertyVariant::SetAsVoid()
{
  /* yeah, so VT_EMPTY is JS void, and VT_NULL is js NULL */
  HRESULT hr = PropVariantClear(&mPropVariant);
  if (FAILED(hr))
    return NS_ERROR_FAILURE;
  mPropVariant.vt = VT_EMPTY;
  return NS_OK;
}

/* void setAsEmpty (); */
NS_IMETHODIMP sbPropertyVariant::SetAsEmpty()
{
  /* see SetAsVoid - yes, we use VT_EMPTY for SetAsVoid
     and VT_NULL for setAsEmpty (i.e. VT_EMPTY in SetEmpty is wrong) */
  HRESULT hr = PropVariantClear(&mPropVariant);
  if (FAILED(hr))
    return NS_ERROR_FAILURE;
  mPropVariant.vt = VT_NULL;
  return NS_OK;
}

/* void setAsEmptyArray (); */
NS_IMETHODIMP sbPropertyVariant::SetAsEmptyArray()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

#define GET_VARIANT_BYVAL(vtype, prtype, method) \
  case nsIDataType::vtype: {                     \
    prtype val;                                  \
    rv = aOther->GetAs ## method(&val);          \
    NS_ENSURE_SUCCESS(rv, rv);                   \
    rv = SetAs ## method(val);                   \
    NS_ENSURE_SUCCESS(rv, rv);                   \
    break;                                       \
  }

#define GET_VARIANT_STRING(vtype, prtype, method) \
  case nsIDataType::vtype: {                      \
    prtype val;                                   \
    rv = aOther->GetAs ## method(val);            \
    NS_ENSURE_SUCCESS(rv, rv);                    \
    rv = SetAs ## method(val);                    \
    NS_ENSURE_SUCCESS(rv, rv);                    \
    break;                                        \
  }
  

/* void setFromVariant (in nsIVariant aValue); */
NS_IMETHODIMP sbPropertyVariant::SetFromVariant(nsIVariant *aOther)
{
  HRESULT hr = PropVariantClear(&mPropVariant);
  if (FAILED(hr))
    return NS_ERROR_FAILURE;
  /* *grumble* */
  
  PRUint16 otherType;
  nsresult rv = aOther->GetDataType(&otherType);
  NS_ENSURE_SUCCESS(rv, rv);
  switch(otherType) {
    GET_VARIANT_BYVAL(VTYPE_INT8,             PRUint8,    Int8)
    GET_VARIANT_BYVAL(VTYPE_INT16,            PRInt16,    Int16)
    GET_VARIANT_BYVAL(VTYPE_INT32,            PRInt32,    Int32)
    GET_VARIANT_BYVAL(VTYPE_INT64,            PRInt64,    Int64)
    GET_VARIANT_BYVAL(VTYPE_UINT8,            PRUint8,    Uint8)
    GET_VARIANT_BYVAL(VTYPE_UINT16,           PRUint16,   Uint16)
    GET_VARIANT_BYVAL(VTYPE_UINT32,           PRUint32,   Uint32)
    GET_VARIANT_BYVAL(VTYPE_UINT64,           PRUint64,   Uint64)
    GET_VARIANT_BYVAL(VTYPE_FLOAT,            float,      Float)
    GET_VARIANT_BYVAL(VTYPE_DOUBLE,           double,     Double)
    GET_VARIANT_BYVAL(VTYPE_BOOL,             PRBool,     Bool)
    GET_VARIANT_BYVAL(VTYPE_CHAR,             char,       Char)
    GET_VARIANT_BYVAL(VTYPE_WCHAR,            PRUnichar,  WChar)
    
    case nsIDataType::VTYPE_VOID: {
      rv = SetAsVoid();
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    }
    
    /* VTYPE_ID */

    GET_VARIANT_STRING(VTYPE_DOMSTRING,       nsString,   DOMString)
    GET_VARIANT_BYVAL(VTYPE_CHAR_STR,         char*,      String)
    GET_VARIANT_BYVAL(VTYPE_WCHAR_STR,        PRUnichar*, WString)
    GET_VARIANT_STRING(VTYPE_STRING_SIZE_IS,  nsCString,  ACString)
    GET_VARIANT_STRING(VTYPE_WSTRING_SIZE_IS, nsString,   AString)
    GET_VARIANT_STRING(VTYPE_UTF8STRING,      nsCString,  ACString)
    GET_VARIANT_STRING(VTYPE_CSTRING,         nsCString,  ACString)
    GET_VARIANT_STRING(VTYPE_ASTRING,         nsString,   AString)

    default:
      return NS_ERROR_NOT_IMPLEMENTED;
  }
  return NS_OK;
}

/* Umm, yeah, shared, please don't clobber kthx */
PROPVARIANT* sbPropertyVariant::GetPropVariant()
{
  return &mPropVariant;
}
