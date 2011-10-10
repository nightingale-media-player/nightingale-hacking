/*
//
// BEGIN NIGHTINGALE GPL
// 
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

/*
// Original code is nsVariant.h from the Mozilla Foundation.
*/

/* The long avoided variant support for xpcom. */

#include "sbVariant.h"
#include <nsStringAPI.h>
#include <prprf.h>
#include <prdtoa.h>
#include <math.h>
#include <nsAutoLock.h>
#include <nsCRT.h>

#include <sbMemoryUtils.h>

/***************************************************************************/
// Helpers for static convert functions...

/**
* This is a copy of |PR_cnvtf| with a bug fixed.  (The second argument
* of PR_dtoa is 2 rather than 1.)
*
* XXX(darin): if this is the right thing, then why wasn't it fixed in NSPR?!?
*/
static void 
Modified_cnvtf(char *buf, int bufsz, int prcsn, double fval)
{
  PRIntn decpt, sign, numdigits;
  char *num, *nump;
  char *bufp = buf;
  char *endnum;

  /* If anything fails, we store an empty string in 'buf' */
  num = (char*)malloc(bufsz);
  if (num == NULL) {
    buf[0] = '\0';
    return;
  }
  if (PR_dtoa(fval, 2, prcsn, &decpt, &sign, &endnum, num, bufsz)
    == PR_FAILURE) {
      buf[0] = '\0';
      goto done;
  }
  numdigits = endnum - num;
  nump = num;

  /*
  * The NSPR code had a fancy way of checking that we weren't dealing
  * with -0.0 or -NaN, but I'll just use < instead.
  * XXX Should we check !isnan(fval) as well?  Is it portable?  We
  * probably don't need to bother since NAN isn't portable.
  */
  if (sign && fval < 0.0f) {
    *bufp++ = '-';
  }

  if (decpt == 9999) {
    while ((*bufp++ = *nump++) != 0) {} /* nothing to execute */
    goto done;
  }

  if (decpt > (prcsn+1) || decpt < -(prcsn-1) || decpt < -5) {
    *bufp++ = *nump++;
    if (numdigits != 1) {
      *bufp++ = '.';
    }

    while (*nump != '\0') {
      *bufp++ = *nump++;
    }
    *bufp++ = 'e';
    PR_snprintf(bufp, bufsz - (bufp - buf), "%+d", decpt-1);
  }
  else if (decpt >= 0) {
    if (decpt == 0) {
      *bufp++ = '0';
    }
    else {
      while (decpt--) {
        if (*nump != '\0') {
          *bufp++ = *nump++;
        }
        else {
          *bufp++ = '0';
        }
      }
    }
    if (*nump != '\0') {
      *bufp++ = '.';
      while (*nump != '\0') {
        *bufp++ = *nump++;
      }
    }
    *bufp++ = '\0';
  }
  else if (decpt < 0) {
    *bufp++ = '0';
    *bufp++ = '.';
    while (decpt++) {
      *bufp++ = '0';
    }

    while (*nump != '\0') {
      *bufp++ = *nump++;
    }
    *bufp++ = '\0';
  }
done:
  free(num);
}

static nsresult String2Double(const char* aString, double* retval)
{
  char* next;
  double value = PR_strtod(aString, &next);
  if(next == aString)
    return NS_ERROR_CANNOT_CONVERT_DATA;
  *retval = value;
  return NS_OK;
}

static nsresult AString2Double(const nsAString& aString, double* retval)
{
  char* pChars = ToNewCString(NS_ConvertUTF16toUTF8(aString));
  if(!pChars)
    return NS_ERROR_OUT_OF_MEMORY;
  nsresult rv = String2Double(pChars, retval);
  NS_Free(pChars);
  return rv;
}

static nsresult AUTF8String2Double(const nsAUTF8String& aString, double* retval)
{
  return String2Double(PromiseFlatUTF8String(aString).get(), retval);
}

static nsresult ACString2Double(const nsACString& aString, double* retval)
{
  return String2Double(PromiseFlatCString(aString).get(), retval);
}

// Fills outVariant with double, PRUint32, or PRInt32.
// Returns NS_OK, an error code, or a non-NS_OK success code
static nsresult ToManageableNumber(const nsDiscriminatedUnion& inData,
                                   nsDiscriminatedUnion* outData)
{
  nsresult rv;

  switch(inData.mType)
  {
    // This group results in a PRInt32...

#define CASE__NUMBER_INT32(type_, member_)                                    \
    case nsIDataType :: type_ :                                               \
    outData->u.mInt32Value = inData.u. member_ ;                          \
    outData->mType = nsIDataType::VTYPE_INT32;                            \
    return NS_OK;

    CASE__NUMBER_INT32(VTYPE_INT8,   mInt8Value)
    CASE__NUMBER_INT32(VTYPE_INT16,  mInt16Value)
    CASE__NUMBER_INT32(VTYPE_INT32,  mInt32Value)
    CASE__NUMBER_INT32(VTYPE_UINT8,  mUint8Value)
    CASE__NUMBER_INT32(VTYPE_UINT16, mUint16Value)
    CASE__NUMBER_INT32(VTYPE_BOOL,   mBoolValue)
    CASE__NUMBER_INT32(VTYPE_CHAR,   mCharValue)
    CASE__NUMBER_INT32(VTYPE_WCHAR,  mWCharValue)

#undef CASE__NUMBER_INT32

      // This group results in a PRUint32...

    case nsIDataType::VTYPE_UINT32:
      outData->u.mInt32Value = inData.u.mUint32Value;
      outData->mType = nsIDataType::VTYPE_INT32;
      return NS_OK;

      // This group results in a double...

    case nsIDataType::VTYPE_INT64:
    case nsIDataType::VTYPE_UINT64:
      // XXX Need boundary checking here.
      // We may need to return NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA
      LL_L2D(outData->u.mDoubleValue, inData.u.mInt64Value);
      outData->mType = nsIDataType::VTYPE_DOUBLE;
      return NS_OK;
    case nsIDataType::VTYPE_FLOAT:
      outData->u.mDoubleValue = inData.u.mFloatValue;
      outData->mType = nsIDataType::VTYPE_DOUBLE;
      return NS_OK;
    case nsIDataType::VTYPE_DOUBLE:
      outData->u.mDoubleValue = inData.u.mDoubleValue;
      outData->mType = nsIDataType::VTYPE_DOUBLE;
      return NS_OK;
    case nsIDataType::VTYPE_CHAR_STR:
    case nsIDataType::VTYPE_STRING_SIZE_IS:
      rv = String2Double(inData.u.str.mStringValue, &outData->u.mDoubleValue);
      if(NS_FAILED(rv))
        return rv;
      outData->mType = nsIDataType::VTYPE_DOUBLE;
      return NS_OK;
    case nsIDataType::VTYPE_DOMSTRING:
    case nsIDataType::VTYPE_ASTRING:
      rv = AString2Double(*inData.u.mAStringValue, &outData->u.mDoubleValue);
      if(NS_FAILED(rv))
        return rv;
      outData->mType = nsIDataType::VTYPE_DOUBLE;
      return NS_OK;
    case nsIDataType::VTYPE_UTF8STRING:
      rv = AUTF8String2Double(*inData.u.mUTF8StringValue,
        &outData->u.mDoubleValue);
      if(NS_FAILED(rv))
        return rv;
      outData->mType = nsIDataType::VTYPE_DOUBLE;
      return NS_OK;
    case nsIDataType::VTYPE_CSTRING:
      rv = ACString2Double(*inData.u.mCStringValue,
        &outData->u.mDoubleValue);
      if(NS_FAILED(rv))
        return rv;
      outData->mType = nsIDataType::VTYPE_DOUBLE;
      return NS_OK;
    case nsIDataType::VTYPE_WCHAR_STR:
    case nsIDataType::VTYPE_WSTRING_SIZE_IS:
      rv = AString2Double(nsDependentString(inData.u.wstr.mWStringValue),
        &outData->u.mDoubleValue);
      if(NS_FAILED(rv))
        return rv;
      outData->mType = nsIDataType::VTYPE_DOUBLE;
      return NS_OK;

      // This group fails...

    case nsIDataType::VTYPE_VOID:
    case nsIDataType::VTYPE_ID:
    case nsIDataType::VTYPE_INTERFACE:
    case nsIDataType::VTYPE_INTERFACE_IS:
    case nsIDataType::VTYPE_ARRAY:
    case nsIDataType::VTYPE_EMPTY_ARRAY:
    case nsIDataType::VTYPE_EMPTY:
    default:
      return NS_ERROR_CANNOT_CONVERT_DATA;
  }
}

/***************************************************************************/
// Array helpers...

static void FreeArray(nsDiscriminatedUnion* data)
{
  NS_ASSERTION(data->mType == nsIDataType::VTYPE_ARRAY, "bad FreeArray call");
  NS_ASSERTION(data->u.array.mArrayValue, "bad array");
  NS_ASSERTION(data->u.array.mArrayCount, "bad array count");

#define CASE__FREE_ARRAY_PTR(type_, ctype_)                                   \
        case nsIDataType:: type_ :                                            \
  {                                                                     \
  ctype_ ** p = (ctype_ **) data->u.array.mArrayValue;              \
  for(PRUint32 i = data->u.array.mArrayCount; i > 0; p++, i--)      \
  if(*p)                                                        \
  NS_Free((char*)*p);                                \
  break;                                                            \
  }

#define CASE__FREE_ARRAY_IFACE(type_, ctype_)                                 \
        case nsIDataType:: type_ :                                            \
  {                                                                     \
  ctype_ ** p = (ctype_ **) data->u.array.mArrayValue;              \
  for(PRUint32 i = data->u.array.mArrayCount; i > 0; p++, i--)      \
  if(*p)                                                        \
  (*p)->Release();                                          \
  break;                                                            \
  }

  switch(data->u.array.mArrayType)
  {
  case nsIDataType::VTYPE_INT8:
  case nsIDataType::VTYPE_INT16:
  case nsIDataType::VTYPE_INT32:
  case nsIDataType::VTYPE_INT64:
  case nsIDataType::VTYPE_UINT8:
  case nsIDataType::VTYPE_UINT16:
  case nsIDataType::VTYPE_UINT32:
  case nsIDataType::VTYPE_UINT64:
  case nsIDataType::VTYPE_FLOAT:
  case nsIDataType::VTYPE_DOUBLE:
  case nsIDataType::VTYPE_BOOL:
  case nsIDataType::VTYPE_CHAR:
  case nsIDataType::VTYPE_WCHAR:
    break;

    // XXX We ASSUME that "array of nsID" means "array of pointers to nsID".
    CASE__FREE_ARRAY_PTR(VTYPE_ID, nsID)
    CASE__FREE_ARRAY_PTR(VTYPE_CHAR_STR, char)
    CASE__FREE_ARRAY_PTR(VTYPE_WCHAR_STR, PRUnichar)
    CASE__FREE_ARRAY_IFACE(VTYPE_INTERFACE, nsISupports)
    CASE__FREE_ARRAY_IFACE(VTYPE_INTERFACE_IS, nsISupports)

      // The rest are illegal.
  case nsIDataType::VTYPE_VOID:
  case nsIDataType::VTYPE_ASTRING:
  case nsIDataType::VTYPE_DOMSTRING:
  case nsIDataType::VTYPE_UTF8STRING:
  case nsIDataType::VTYPE_CSTRING:
  case nsIDataType::VTYPE_WSTRING_SIZE_IS:
  case nsIDataType::VTYPE_STRING_SIZE_IS:
  case nsIDataType::VTYPE_ARRAY:
  case nsIDataType::VTYPE_EMPTY_ARRAY:
  case nsIDataType::VTYPE_EMPTY:
  default:
    NS_ERROR("bad type in array!");
    break;
  }

  // Free the array memory.
  NS_Free((char*)data->u.array.mArrayValue);

#undef CASE__FREE_ARRAY_PTR
#undef CASE__FREE_ARRAY_IFACE
}

static nsresult CloneArray(PRUint16 inType, const nsIID* inIID,
                           PRUint32 inCount, void* inValue,
                           PRUint16* outType, nsIID* outIID,
                           PRUint32* outCount, void** outValue)
{
  NS_ASSERTION(inCount, "bad param");
  NS_ASSERTION(inValue, "bad param");
  NS_ASSERTION(outType, "bad param");
  NS_ASSERTION(outCount, "bad param");
  NS_ASSERTION(outValue, "bad param");

  PRUint32 allocatedValueCount = 0;
  nsresult rv = NS_OK;
  PRUint32 i;

  // First we figure out the size of the elements for the new u.array.

  size_t elementSize;
  size_t allocSize;

  switch(inType)
  {
  case nsIDataType::VTYPE_INT8:
    elementSize = sizeof(PRInt8);
    break;
  case nsIDataType::VTYPE_INT16:
    elementSize = sizeof(PRInt16);
    break;
  case nsIDataType::VTYPE_INT32:
    elementSize = sizeof(PRInt32);
    break;
  case nsIDataType::VTYPE_INT64:
    elementSize = sizeof(PRInt64);
    break;
  case nsIDataType::VTYPE_UINT8:
    elementSize = sizeof(PRUint8);
    break;
  case nsIDataType::VTYPE_UINT16:
    elementSize = sizeof(PRUint16);
    break;
  case nsIDataType::VTYPE_UINT32:
    elementSize = sizeof(PRUint32);
    break;
  case nsIDataType::VTYPE_UINT64:
    elementSize = sizeof(PRUint64);
    break;
  case nsIDataType::VTYPE_FLOAT:
    elementSize = sizeof(float);
    break;
  case nsIDataType::VTYPE_DOUBLE:
    elementSize = sizeof(double);
    break;
  case nsIDataType::VTYPE_BOOL:
    elementSize = sizeof(PRBool);
    break;
  case nsIDataType::VTYPE_CHAR:
    elementSize = sizeof(char);
    break;
  case nsIDataType::VTYPE_WCHAR:
    elementSize = sizeof(PRUnichar);
    break;

    // XXX We ASSUME that "array of nsID" means "array of pointers to nsID".
  case nsIDataType::VTYPE_ID:
  case nsIDataType::VTYPE_CHAR_STR:
  case nsIDataType::VTYPE_WCHAR_STR:
  case nsIDataType::VTYPE_INTERFACE:
  case nsIDataType::VTYPE_INTERFACE_IS:
    elementSize = sizeof(void*);
    break;

    // The rest are illegal.
  case nsIDataType::VTYPE_ASTRING:
  case nsIDataType::VTYPE_DOMSTRING:
  case nsIDataType::VTYPE_UTF8STRING:
  case nsIDataType::VTYPE_CSTRING:
  case nsIDataType::VTYPE_STRING_SIZE_IS:
  case nsIDataType::VTYPE_WSTRING_SIZE_IS:
  case nsIDataType::VTYPE_VOID:
  case nsIDataType::VTYPE_ARRAY:
  case nsIDataType::VTYPE_EMPTY_ARRAY:
  case nsIDataType::VTYPE_EMPTY:
  default:
    NS_ERROR("bad type in array!");
    return NS_ERROR_CANNOT_CONVERT_DATA;
  }


  // Alloc the u.array.

  allocSize = inCount * elementSize;
  *outValue = NS_Alloc(allocSize);
  if(!*outValue)
    return NS_ERROR_OUT_OF_MEMORY;

  // Clone the elements.

  switch(inType)
  {
  case nsIDataType::VTYPE_INT8:
  case nsIDataType::VTYPE_INT16:
  case nsIDataType::VTYPE_INT32:
  case nsIDataType::VTYPE_INT64:
  case nsIDataType::VTYPE_UINT8:
  case nsIDataType::VTYPE_UINT16:
  case nsIDataType::VTYPE_UINT32:
  case nsIDataType::VTYPE_UINT64:
  case nsIDataType::VTYPE_FLOAT:
  case nsIDataType::VTYPE_DOUBLE:
  case nsIDataType::VTYPE_BOOL:
  case nsIDataType::VTYPE_CHAR:
  case nsIDataType::VTYPE_WCHAR:
    memcpy(*outValue, inValue, allocSize);
    break;

  case nsIDataType::VTYPE_INTERFACE_IS:
    if(outIID)
      *outIID = *inIID;
    // fall through...
  case nsIDataType::VTYPE_INTERFACE:
    {
      memcpy(*outValue, inValue, allocSize);

      nsISupports** p = (nsISupports**) *outValue;
      for(i = inCount; i > 0; p++, i--)
        if(*p)
          (*p)->AddRef();
      break;
    }

    // XXX We ASSUME that "array of nsID" means "array of pointers to nsID".
  case nsIDataType::VTYPE_ID:
    {
      nsID** inp  = (nsID**) inValue;
      nsID** outp = (nsID**) *outValue;
      for(i = inCount; i > 0; i--)
      {
        nsID* idp = *(inp++);
        if(idp)
        {
          if(nsnull == (*(outp++) = (nsID*)
            SB_CloneMemory((char*)idp, sizeof(nsID))))
            goto bad;
        }
        else
          *(outp++) = nsnull;
        allocatedValueCount++;
      }
      break;
    }

  case nsIDataType::VTYPE_CHAR_STR:
    {
      char** inp  = (char**) inValue;
      char** outp = (char**) *outValue;
      for(i = inCount; i > 0; i--)
      {
        char* str = *(inp++);
        if(str)
        {
          if(nsnull == (*(outp++) = (char*)
            SB_CloneMemory(str, (strlen(str)+1)*sizeof(char))))
            goto bad;
        }
        else
          *(outp++) = nsnull;
        allocatedValueCount++;
      }
      break;
    }

  case nsIDataType::VTYPE_WCHAR_STR:
    {
      PRUnichar** inp  = (PRUnichar**) inValue;
      PRUnichar** outp = (PRUnichar**) *outValue;
      for(i = inCount; i > 0; i--)
      {
        PRUnichar* str = *(inp++);
        if(str)
        {
          if(nsnull == (*(outp++) = (PRUnichar*)
            SB_CloneMemory(str,
            (nsCRT::strlen(str)+1)*sizeof(PRUnichar))))
            goto bad;
        }
        else
          *(outp++) = nsnull;
        allocatedValueCount++;
      }
      break;
    }

    // The rest are illegal.
  case nsIDataType::VTYPE_VOID:
  case nsIDataType::VTYPE_ARRAY:
  case nsIDataType::VTYPE_EMPTY_ARRAY:
  case nsIDataType::VTYPE_EMPTY:
  case nsIDataType::VTYPE_ASTRING:
  case nsIDataType::VTYPE_DOMSTRING:
  case nsIDataType::VTYPE_UTF8STRING:
  case nsIDataType::VTYPE_CSTRING:
  case nsIDataType::VTYPE_STRING_SIZE_IS:
  case nsIDataType::VTYPE_WSTRING_SIZE_IS:
  default:
    NS_ERROR("bad type in array!");
    return NS_ERROR_CANNOT_CONVERT_DATA;
  }

  *outType = inType;
  *outCount = inCount;
  return NS_OK;

bad:
  if(*outValue)
  {
    char** p = (char**) *outValue;
    for(i = allocatedValueCount; i > 0; p++, i--)
      if(*p)
        NS_Free(*p);
    NS_Free((char*)*outValue);
    *outValue = nsnull;
  }
  return rv;
}

/***************************************************************************/

#define TRIVIAL_DATA_CONVERTER(type_, data_, member_, retval_)                \
  if(data_.mType == nsIDataType :: type_) {                                 \
  *retval_ = data_.u.member_;                                           \
  return NS_OK;                                                         \
  }

#define NUMERIC_CONVERSION_METHOD_BEGIN(type_, Ctype_, name_)                 \
  /* static */ nsresult                                                         \
  sbVariant::ConvertTo##name_ (const nsDiscriminatedUnion& data,                \
  Ctype_ *_retval)                                 \
{                                                                             \
  TRIVIAL_DATA_CONVERTER(type_, data, m##name_##Value, _retval)             \
  nsDiscriminatedUnion tempData;                                            \
  sbVariant::Initialize(&tempData);                                         \
  nsresult rv = ToManageableNumber(data, &tempData);                        \
  /*                                                                     */ \
  /* NOTE: rv may indicate a success code that we want to preserve       */ \
  /* For the final return. So all the return cases below should return   */ \
  /* this rv when indicating success.                                    */ \
  /*                                                                     */ \
  if(NS_FAILED(rv))                                                         \
  return rv;                                                            \
  switch(tempData.mType)                                                    \
{

#define CASE__NUMERIC_CONVERSION_INT32_JUST_CAST(Ctype_)                      \
    case nsIDataType::VTYPE_INT32:                                            \
    *_retval = ( Ctype_ ) tempData.u.mInt32Value;                         \
    return rv;

#define CASE__NUMERIC_CONVERSION_INT32_MIN_MAX(Ctype_, min_, max_)            \
    case nsIDataType::VTYPE_INT32:                                            \
{                                                                         \
  PRInt32 value = tempData.u.mInt32Value;                               \
  if(value < min_ || value > max_)                                      \
  return NS_ERROR_LOSS_OF_SIGNIFICANT_DATA;                         \
  *_retval = ( Ctype_ ) value;                                          \
  return rv;                                                            \
}

#define CASE__NUMERIC_CONVERSION_UINT32_JUST_CAST(Ctype_)                     \
    case nsIDataType::VTYPE_UINT32:                                           \
    *_retval = ( Ctype_ ) tempData.u.mUint32Value;                        \
    return rv;

#define CASE__NUMERIC_CONVERSION_UINT32_MAX(Ctype_, max_)                     \
    case nsIDataType::VTYPE_UINT32:                                           \
{                                                                         \
  PRUint32 value = tempData.u.mUint32Value;                             \
  if(value > max_)                                                      \
  return NS_ERROR_LOSS_OF_SIGNIFICANT_DATA;                         \
  *_retval = ( Ctype_ ) value;                                          \
  return rv;                                                            \
}

#define CASE__NUMERIC_CONVERSION_DOUBLE_JUST_CAST(Ctype_)                     \
    case nsIDataType::VTYPE_DOUBLE:                                           \
    *_retval = ( Ctype_ ) tempData.u.mDoubleValue;                        \
    return rv;

#define CASE__NUMERIC_CONVERSION_DOUBLE_MIN_MAX(Ctype_, min_, max_)           \
    case nsIDataType::VTYPE_DOUBLE:                                           \
{                                                                         \
  double value = tempData.u.mDoubleValue;                               \
  if(value < min_ || value > max_)                                      \
  return NS_ERROR_LOSS_OF_SIGNIFICANT_DATA;                         \
  *_retval = ( Ctype_ ) value;                                          \
  return rv;                                                            \
}

#define CASE__NUMERIC_CONVERSION_DOUBLE_MIN_MAX_INT(Ctype_, min_, max_)       \
    case nsIDataType::VTYPE_DOUBLE:                                           \
{                                                                         \
  double value = tempData.u.mDoubleValue;                               \
  if(value < min_ || value > max_)                                      \
  return NS_ERROR_LOSS_OF_SIGNIFICANT_DATA;                         \
  *_retval = ( Ctype_ ) value;                                          \
  return (0.0 == fmod(value,1.0)) ?                                     \
rv : NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA;                       \
}

#define CASES__NUMERIC_CONVERSION_NORMAL(Ctype_, min_, max_)                  \
  CASE__NUMERIC_CONVERSION_INT32_MIN_MAX(Ctype_, min_, max_)                \
  CASE__NUMERIC_CONVERSION_UINT32_MAX(Ctype_, max_)                         \
  CASE__NUMERIC_CONVERSION_DOUBLE_MIN_MAX_INT(Ctype_, min_, max_)

#define NUMERIC_CONVERSION_METHOD_END                                         \
    default:                                                                  \
    NS_ERROR("bad type returned from ToManageableNumber");                \
    return NS_ERROR_CANNOT_CONVERT_DATA;                                  \
}                                                                         \
}

#define NUMERIC_CONVERSION_METHOD_NORMAL(type_, Ctype_, name_, min_, max_)    \
  NUMERIC_CONVERSION_METHOD_BEGIN(type_, Ctype_, name_)                     \
  CASES__NUMERIC_CONVERSION_NORMAL(Ctype_, min_, max_)                  \
  NUMERIC_CONVERSION_METHOD_END

/***************************************************************************/
// These expand into full public methods...

NUMERIC_CONVERSION_METHOD_NORMAL(VTYPE_INT8, PRUint8, Int8, (-127-1), 127)
NUMERIC_CONVERSION_METHOD_NORMAL(VTYPE_INT16, PRInt16, Int16, (-32767-1), 32767)

NUMERIC_CONVERSION_METHOD_BEGIN(VTYPE_INT32, PRInt32, Int32)
CASE__NUMERIC_CONVERSION_INT32_JUST_CAST(PRInt32)
CASE__NUMERIC_CONVERSION_UINT32_MAX(PRInt32, 2147483647)
CASE__NUMERIC_CONVERSION_DOUBLE_MIN_MAX_INT(PRInt32, (-2147483647-1), 2147483647)
NUMERIC_CONVERSION_METHOD_END

NUMERIC_CONVERSION_METHOD_NORMAL(VTYPE_UINT8, PRUint8, Uint8, 0, 255)
NUMERIC_CONVERSION_METHOD_NORMAL(VTYPE_UINT16, PRUint16, Uint16, 0, 65535)

NUMERIC_CONVERSION_METHOD_BEGIN(VTYPE_UINT32, PRUint32, Uint32)
CASE__NUMERIC_CONVERSION_INT32_MIN_MAX(PRUint32, 0, 2147483647)
CASE__NUMERIC_CONVERSION_UINT32_JUST_CAST(PRUint32)
CASE__NUMERIC_CONVERSION_DOUBLE_MIN_MAX_INT(PRUint32, 0, 4294967295U)
NUMERIC_CONVERSION_METHOD_END

// XXX toFloat convertions need to be fixed!
NUMERIC_CONVERSION_METHOD_BEGIN(VTYPE_FLOAT, float, Float)
CASE__NUMERIC_CONVERSION_INT32_JUST_CAST(float)
CASE__NUMERIC_CONVERSION_UINT32_JUST_CAST(float)
CASE__NUMERIC_CONVERSION_DOUBLE_JUST_CAST(float)
NUMERIC_CONVERSION_METHOD_END

NUMERIC_CONVERSION_METHOD_BEGIN(VTYPE_DOUBLE, double, Double)
CASE__NUMERIC_CONVERSION_INT32_JUST_CAST(double)
CASE__NUMERIC_CONVERSION_UINT32_JUST_CAST(double)
CASE__NUMERIC_CONVERSION_DOUBLE_JUST_CAST(double)
NUMERIC_CONVERSION_METHOD_END

// XXX toChar convertions need to be fixed!
NUMERIC_CONVERSION_METHOD_BEGIN(VTYPE_CHAR, char, Char)
CASE__NUMERIC_CONVERSION_INT32_JUST_CAST(char)
CASE__NUMERIC_CONVERSION_UINT32_JUST_CAST(char)
CASE__NUMERIC_CONVERSION_DOUBLE_JUST_CAST(char)
NUMERIC_CONVERSION_METHOD_END

// XXX toWChar convertions need to be fixed!
NUMERIC_CONVERSION_METHOD_BEGIN(VTYPE_WCHAR, PRUnichar, WChar)
CASE__NUMERIC_CONVERSION_INT32_JUST_CAST(PRUnichar)
CASE__NUMERIC_CONVERSION_UINT32_JUST_CAST(PRUnichar)
CASE__NUMERIC_CONVERSION_DOUBLE_JUST_CAST(PRUnichar)
NUMERIC_CONVERSION_METHOD_END

#undef NUMERIC_CONVERSION_METHOD_BEGIN
#undef CASE__NUMERIC_CONVERSION_INT32_JUST_CAST
#undef CASE__NUMERIC_CONVERSION_INT32_MIN_MAX
#undef CASE__NUMERIC_CONVERSION_UINT32_JUST_CAST
#undef CASE__NUMERIC_CONVERSION_UINT32_MIN_MAX
#undef CASE__NUMERIC_CONVERSION_DOUBLE_JUST_CAST
#undef CASE__NUMERIC_CONVERSION_DOUBLE_MIN_MAX
#undef CASE__NUMERIC_CONVERSION_DOUBLE_MIN_MAX_INT
#undef CASES__NUMERIC_CONVERSION_NORMAL
#undef NUMERIC_CONVERSION_METHOD_END
#undef NUMERIC_CONVERSION_METHOD_NORMAL

/***************************************************************************/

// Just leverage a numeric converter for bool (but restrict the values).
// XXX Is this really what we want to do?

/* static */ nsresult
sbVariant::ConvertToBool(const nsDiscriminatedUnion& data, PRBool *_retval)
{
  TRIVIAL_DATA_CONVERTER(VTYPE_BOOL, data, mBoolValue, _retval)

    double val;
  nsresult rv = sbVariant::ConvertToDouble(data, &val);
  if(NS_FAILED(rv))
    return rv;
  *_retval = 0.0 != val;
  return rv;
}

/***************************************************************************/

/* static */ nsresult
sbVariant::ConvertToInt64(const nsDiscriminatedUnion& data, PRInt64 *_retval)
{
  TRIVIAL_DATA_CONVERTER(VTYPE_INT64, data, mInt64Value, _retval)
    TRIVIAL_DATA_CONVERTER(VTYPE_UINT64, data, mUint64Value, _retval)

    nsDiscriminatedUnion tempData;
  sbVariant::Initialize(&tempData);
  nsresult rv = ToManageableNumber(data, &tempData);
  if(NS_FAILED(rv))
    return rv;
  switch(tempData.mType)
  {
  case nsIDataType::VTYPE_INT32:
    LL_I2L(*_retval, tempData.u.mInt32Value);
    return rv;
  case nsIDataType::VTYPE_UINT32:
    LL_UI2L(*_retval, tempData.u.mUint32Value);
    return rv;
  case nsIDataType::VTYPE_DOUBLE:
    // XXX should check for data loss here!
    LL_D2L(*_retval, tempData.u.mDoubleValue);
    return rv;
  default:
    NS_ERROR("bad type returned from ToManageableNumber");
    return NS_ERROR_CANNOT_CONVERT_DATA;
  }
}

/* static */ nsresult
sbVariant::ConvertToUint64(const nsDiscriminatedUnion& data, PRUint64 *_retval)
{
  return sbVariant::ConvertToInt64(data, (PRInt64 *)_retval);
}

/***************************************************************************/

static PRBool String2ID(const nsDiscriminatedUnion& data, nsID* pid)
{
  nsAutoString tempString;
  nsAString* pString;

  switch(data.mType)
  {
  case nsIDataType::VTYPE_CHAR_STR:
  case nsIDataType::VTYPE_STRING_SIZE_IS:
    return pid->Parse(data.u.str.mStringValue);
  case nsIDataType::VTYPE_CSTRING:
    return pid->Parse(PromiseFlatCString(*data.u.mCStringValue).get());
  case nsIDataType::VTYPE_UTF8STRING:
    return pid->Parse(PromiseFlatUTF8String(*data.u.mUTF8StringValue).get());
  case nsIDataType::VTYPE_ASTRING:
  case nsIDataType::VTYPE_DOMSTRING:
    pString = data.u.mAStringValue;
    break;
  case nsIDataType::VTYPE_WCHAR_STR:
  case nsIDataType::VTYPE_WSTRING_SIZE_IS:
    tempString.Assign(data.u.wstr.mWStringValue);
    pString = &tempString;
    break;
  default:
    NS_ERROR("bad type in call to String2ID");
    return PR_FALSE;
  }

  char* pChars = ToNewCString(NS_LossyConvertUTF16toASCII(*pString));
  if(!pChars)
    return PR_FALSE;
  PRBool result = pid->Parse(pChars);
  NS_Free(pChars);
  return result;
}

/* static */ nsresult
sbVariant::ConvertToID(const nsDiscriminatedUnion& data, nsID * _retval)
{
  nsID id;

  switch(data.mType)
  {
  case nsIDataType::VTYPE_ID:
    *_retval = data.u.mIDValue;
    return NS_OK;
  case nsIDataType::VTYPE_INTERFACE:
    *_retval = NS_GET_IID(nsISupports);
    return NS_OK;
  case nsIDataType::VTYPE_INTERFACE_IS:
    *_retval = data.u.iface.mInterfaceID;
    return NS_OK;
  case nsIDataType::VTYPE_ASTRING:
  case nsIDataType::VTYPE_DOMSTRING:
  case nsIDataType::VTYPE_UTF8STRING:
  case nsIDataType::VTYPE_CSTRING:
  case nsIDataType::VTYPE_CHAR_STR:
  case nsIDataType::VTYPE_WCHAR_STR:
  case nsIDataType::VTYPE_STRING_SIZE_IS:
  case nsIDataType::VTYPE_WSTRING_SIZE_IS:
    if(!String2ID(data, &id))
      return NS_ERROR_CANNOT_CONVERT_DATA;
    *_retval = id;
    return NS_OK;
  default:
    return NS_ERROR_CANNOT_CONVERT_DATA;
  }
}

/***************************************************************************/

static nsresult ToString(const nsDiscriminatedUnion& data,
                         nsACString & outString)
{
  char* ptr;

  switch(data.mType)
  {
    // all the stuff we don't handle...
  case nsIDataType::VTYPE_ASTRING:
  case nsIDataType::VTYPE_DOMSTRING:
  case nsIDataType::VTYPE_UTF8STRING:
  case nsIDataType::VTYPE_CSTRING:
  case nsIDataType::VTYPE_CHAR_STR:
  case nsIDataType::VTYPE_WCHAR_STR:
  case nsIDataType::VTYPE_STRING_SIZE_IS:
  case nsIDataType::VTYPE_WSTRING_SIZE_IS:
  case nsIDataType::VTYPE_WCHAR:
    NS_ERROR("ToString being called for a string type - screwy logic!");
    // fall through...

    // XXX We might want stringified versions of these... ???

  case nsIDataType::VTYPE_VOID:
  case nsIDataType::VTYPE_EMPTY:
    outString.Truncate();
    outString.SetIsVoid(true);
    return NS_OK;

  case nsIDataType::VTYPE_EMPTY_ARRAY:
  case nsIDataType::VTYPE_ARRAY:
  case nsIDataType::VTYPE_INTERFACE:
  case nsIDataType::VTYPE_INTERFACE_IS:
  default:
    return NS_ERROR_CANNOT_CONVERT_DATA;

    // nsID has its own text formater.

  case nsIDataType::VTYPE_ID:
    ptr = data.u.mIDValue.ToString();
    if(!ptr)
      return NS_ERROR_OUT_OF_MEMORY;
    outString.Assign(ptr);
    NS_Free(ptr);
    return NS_OK;

    // Can't use PR_smprintf for floats, since it's locale-dependent
#define CASE__APPENDFLOAT_NUMBER(type_, member_)                        \
  case nsIDataType :: type_ :                                           \
    {                                                                   \
      char buf[40];                                                     \
      Modified_cnvtf(buf, sizeof(buf), 6, data.u. member_);             \
      outString.Assign(buf);                                            \
      return NS_OK;                                                     \
    }

#define CASE__APPENDDOUBLE_NUMBER(type_, member_)                       \
  case nsIDataType :: type_ :                                           \
    {                                                                   \
      char buf[40];                                                     \
      Modified_cnvtf(buf, sizeof(buf), 15, data.u. member_);            \
      outString.Assign(buf);                                            \
      return NS_OK;                                                     \
    }

    CASE__APPENDFLOAT_NUMBER(VTYPE_FLOAT,  mFloatValue)
    CASE__APPENDDOUBLE_NUMBER(VTYPE_DOUBLE, mDoubleValue)

#undef CASE__APPENDFLOAT_NUMBER
#undef CASE__APPENDDOUBLE_NUMBER

      // the rest can be PR_smprintf'd and use common code.

#define CASE__SMPRINTF_NUMBER(type_, format_, cast_, member_)               \
  case nsIDataType :: type_ :                                               \
    ptr = PR_smprintf( format_ , (cast_) data.u. member_ );                 \
  break;

      CASE__SMPRINTF_NUMBER(VTYPE_INT8,   "%d",   int,      mInt8Value)
      CASE__SMPRINTF_NUMBER(VTYPE_INT16,  "%d",   int,      mInt16Value)
      CASE__SMPRINTF_NUMBER(VTYPE_INT32,  "%d",   int,      mInt32Value)
      CASE__SMPRINTF_NUMBER(VTYPE_INT64,  "%lld", PRInt64,  mInt64Value)

      CASE__SMPRINTF_NUMBER(VTYPE_UINT8,  "%u",   unsigned, mUint8Value)
      CASE__SMPRINTF_NUMBER(VTYPE_UINT16, "%u",   unsigned, mUint16Value)
      CASE__SMPRINTF_NUMBER(VTYPE_UINT32, "%u",   unsigned, mUint32Value)
      CASE__SMPRINTF_NUMBER(VTYPE_UINT64, "%llu", PRInt64,  mUint64Value)

      // XXX Would we rather print "true" / "false" ?
      CASE__SMPRINTF_NUMBER(VTYPE_BOOL,   "%d",   int,      mBoolValue)

      CASE__SMPRINTF_NUMBER(VTYPE_CHAR,   "%c",   char,     mCharValue)

#undef CASE__SMPRINTF_NUMBER
  }

  if(!ptr)
    return NS_ERROR_OUT_OF_MEMORY;
  outString.Assign(ptr);
  PR_smprintf_free(ptr);
  return NS_OK;
}

/* static */ nsresult
sbVariant::ConvertToAString(const nsDiscriminatedUnion& data,
                            nsAString & _retval)
{
  switch(data.mType)
  {
  case nsIDataType::VTYPE_ASTRING:
  case nsIDataType::VTYPE_DOMSTRING:
    _retval.Assign(*data.u.mAStringValue);
    return NS_OK;
  case nsIDataType::VTYPE_CSTRING:
    CopyASCIItoUTF16(*data.u.mCStringValue, _retval);
    return NS_OK;
  case nsIDataType::VTYPE_UTF8STRING:
    CopyUTF8toUTF16(*data.u.mUTF8StringValue, _retval);
    return NS_OK;
  case nsIDataType::VTYPE_CHAR_STR:
    CopyASCIItoUTF16(nsDependentCString(data.u.str.mStringValue), _retval);
    return NS_OK;
  case nsIDataType::VTYPE_WCHAR_STR:
    _retval.Assign(data.u.wstr.mWStringValue);
    return NS_OK;
  case nsIDataType::VTYPE_STRING_SIZE_IS:
    CopyASCIItoUTF16(nsDependentCString(data.u.str.mStringValue,
      data.u.str.mStringLength),
      _retval);
    return NS_OK;
  case nsIDataType::VTYPE_WSTRING_SIZE_IS:
    _retval.Assign(data.u.wstr.mWStringValue, data.u.wstr.mWStringLength);
    return NS_OK;
  case nsIDataType::VTYPE_WCHAR:
    _retval.Assign(data.u.mWCharValue);
    return NS_OK;
  default:
    {
      nsCAutoString tempCString;
      nsresult rv = ToString(data, tempCString);
      if(NS_FAILED(rv))
        return rv;
      CopyASCIItoUTF16(tempCString, _retval);
      return NS_OK;
    }
  }
}

/* static */ nsresult
sbVariant::ConvertToACString(const nsDiscriminatedUnion& data,
                             nsACString & _retval)
{
  switch(data.mType)
  {
  case nsIDataType::VTYPE_ASTRING:
  case nsIDataType::VTYPE_DOMSTRING:
    LossyCopyUTF16toASCII(*data.u.mAStringValue, _retval);
    return NS_OK;
  case nsIDataType::VTYPE_CSTRING:
    _retval.Assign(*data.u.mCStringValue);
    return NS_OK;
  case nsIDataType::VTYPE_UTF8STRING:
    // XXX This is an extra copy that should be avoided
    // once Jag lands support for UTF8String and associated
    // conversion methods.
    LossyCopyUTF16toASCII(NS_ConvertUTF8toUTF16(*data.u.mUTF8StringValue),
      _retval);
    return NS_OK;
  case nsIDataType::VTYPE_CHAR_STR:
    _retval.Assign(*data.u.str.mStringValue);
    return NS_OK;
  case nsIDataType::VTYPE_WCHAR_STR:
    LossyCopyUTF16toASCII(nsDependentString(data.u.wstr.mWStringValue),
      _retval);
    return NS_OK;
  case nsIDataType::VTYPE_STRING_SIZE_IS:
    _retval.Assign(data.u.str.mStringValue, data.u.str.mStringLength);
    return NS_OK;
  case nsIDataType::VTYPE_WSTRING_SIZE_IS:
    LossyCopyUTF16toASCII(nsDependentString(data.u.wstr.mWStringValue,
      data.u.wstr.mWStringLength), _retval);
    return NS_OK;
  case nsIDataType::VTYPE_WCHAR:
    {
      const PRUnichar* str = &data.u.mWCharValue;
      LossyCopyUTF16toASCII(Substring(str, str + 1), _retval);
      return NS_OK;
    }
  default:
    return ToString(data, _retval);
  }
}

/* static */ nsresult
sbVariant::ConvertToAUTF8String(const nsDiscriminatedUnion& data,
                                nsAUTF8String & _retval)
{
  switch(data.mType)
  {
  case nsIDataType::VTYPE_ASTRING:
  case nsIDataType::VTYPE_DOMSTRING:
    CopyUTF16toUTF8(*data.u.mAStringValue, _retval);
    return NS_OK;
  case nsIDataType::VTYPE_CSTRING:
    // XXX Extra copy, can be removed if we're sure CSTRING can
    //     only contain ASCII.
    CopyUTF16toUTF8(NS_ConvertASCIItoUTF16(*data.u.mCStringValue),
      _retval);
    return NS_OK;
  case nsIDataType::VTYPE_UTF8STRING:
    _retval.Assign(*data.u.mUTF8StringValue);
    return NS_OK;
  case nsIDataType::VTYPE_CHAR_STR:
    // XXX Extra copy, can be removed if we're sure CHAR_STR can
    //     only contain ASCII.
    CopyUTF16toUTF8(NS_ConvertASCIItoUTF16(data.u.str.mStringValue),
      _retval);
    return NS_OK;
  case nsIDataType::VTYPE_WCHAR_STR:
    CopyUTF16toUTF8(nsDependentString(data.u.wstr.mWStringValue), _retval);
    return NS_OK;
  case nsIDataType::VTYPE_STRING_SIZE_IS:
    // XXX Extra copy, can be removed if we're sure CHAR_STR can
    //     only contain ASCII.
    CopyUTF16toUTF8(NS_ConvertASCIItoUTF16(
      nsDependentCString(data.u.str.mStringValue,
      data.u.str.mStringLength)), _retval);
    return NS_OK;
  case nsIDataType::VTYPE_WSTRING_SIZE_IS:
    CopyUTF16toUTF8(nsDependentString(data.u.wstr.mWStringValue,
      data.u.wstr.mWStringLength),
      _retval);
    return NS_OK;
  case nsIDataType::VTYPE_WCHAR:
    {
      const PRUnichar* str = &data.u.mWCharValue;
      CopyUTF16toUTF8(Substring(str, str + 1), _retval);
      return NS_OK;
    }
  default:
    {
      nsCAutoString tempCString;
      nsresult rv = ToString(data, tempCString);
      if(NS_FAILED(rv))
        return rv;
      // XXX Extra copy, can be removed if we're sure tempCString can
      //     only contain ASCII.
      CopyUTF16toUTF8(NS_ConvertASCIItoUTF16(tempCString), _retval);
      return NS_OK;
    }
  }
}

/* static */ nsresult
sbVariant::ConvertToString(nsDiscriminatedUnion& data, char **_retval)
{
  PRUint32 ignored;
  return sbVariant::ConvertToStringWithSize(data, &ignored, _retval);
}

/* static */ nsresult
sbVariant::ConvertToWString(const nsDiscriminatedUnion& data, PRUnichar **_retval)
{
  PRUint32 ignored;
  return sbVariant::ConvertToWStringWithSize(data, &ignored, _retval);
}

/* static */ nsresult
sbVariant::ConvertToStringWithSize(nsDiscriminatedUnion& data,
                                   PRUint32 *size, char **str)
{
  nsAutoString  tempString;
  nsCAutoString tempCString;
  nsresult rv;

  switch(data.mType)
  {
  case nsIDataType::VTYPE_ASTRING:
  case nsIDataType::VTYPE_DOMSTRING:
    *size = data.u.mAStringValue->Length();
    tempString = *data.u.mAStringValue;
    *str = ToNewCString(NS_ConvertUTF16toUTF8(tempString));
    break;
  case nsIDataType::VTYPE_CSTRING:
    *size = data.u.mCStringValue->Length();
    tempCString = *data.u.mCStringValue;
    *str = ToNewCString(tempCString);
    break;
  case nsIDataType::VTYPE_UTF8STRING:
    {
      // XXX This is doing 1 extra copy.  Need to fix this
      // when Jag lands UTF8String
      // we want:
      // *size = *data.mUTF8StringValue->Length();
      // *str = ToNewCString(*data.mUTF8StringValue);
      // But this will have to do for now.
      NS_ConvertUTF8toUTF16 tempString(*data.u.mUTF8StringValue);
      *size = tempString.Length();

      tempCString = *data.u.mUTF8StringValue;
      *str = ToNewCString(tempCString );
      break;
    }
  case nsIDataType::VTYPE_CHAR_STR:
    {
      nsDependentCString cString(data.u.str.mStringValue);
      *size = cString.Length();
      *str = ToNewCString(cString);
      break;
    }
  case nsIDataType::VTYPE_WCHAR_STR:
    {
      nsDependentString string(data.u.wstr.mWStringValue);
      *size = string.Length();
      *str = ToNewCString(NS_ConvertUTF16toUTF8(string));
      break;
    }
  case nsIDataType::VTYPE_STRING_SIZE_IS:
    {
      nsDependentCString cString(data.u.str.mStringValue,
        data.u.str.mStringLength);
      *size = cString.Length();
      *str = ToNewCString(cString);
      break;
    }
  case nsIDataType::VTYPE_WSTRING_SIZE_IS:
    {
      nsDependentString string(data.u.wstr.mWStringValue,
        data.u.wstr.mWStringLength);
      *size = string.Length();
      *str = ToNewCString(NS_ConvertUTF16toUTF8(string));
      break;
    }
  case nsIDataType::VTYPE_WCHAR:
    tempString.Assign(data.u.mWCharValue);
    *size = tempString.Length();
    *str = ToNewCString(NS_ConvertUTF16toUTF8(tempString));
    break;
  default:
    rv = ToString(data, tempCString);
    if(NS_FAILED(rv))
      return rv;
    *size = tempCString.Length();
    *str = ToNewCString(tempCString);
    break;
  }

  return *str ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}
/* static */ nsresult
sbVariant::ConvertToWStringWithSize(const nsDiscriminatedUnion& data,
                                    PRUint32 *size, PRUnichar **str)
{
  nsAutoString  tempString;
  nsCAutoString tempCString;
  nsresult rv;

  switch(data.mType)
  {
  case nsIDataType::VTYPE_ASTRING:
  case nsIDataType::VTYPE_DOMSTRING:
    *size = data.u.mAStringValue->Length();
    *str = ToNewUnicode(*data.u.mAStringValue);
    break;
  case nsIDataType::VTYPE_CSTRING:
    *size = data.u.mCStringValue->Length();
    *str = ToNewUnicode(NS_ConvertUTF8toUTF16(*data.u.mCStringValue));
    break;
  case nsIDataType::VTYPE_UTF8STRING:
    {
      *str = ToNewUnicode(NS_ConvertUTF8toUTF16(*data.u.mUTF8StringValue));
      break;
    }
  case nsIDataType::VTYPE_CHAR_STR:
    {
      nsDependentCString cString(data.u.str.mStringValue);
      *size = cString.Length();
      *str = ToNewUnicode(NS_ConvertUTF8toUTF16(cString));
      break;
    }
  case nsIDataType::VTYPE_WCHAR_STR:
    {
      nsDependentString string(data.u.wstr.mWStringValue);
      *size = string.Length();
      *str = ToNewUnicode(string);
      break;
    }
  case nsIDataType::VTYPE_STRING_SIZE_IS:
    {
      nsDependentCString cString(data.u.str.mStringValue,
        data.u.str.mStringLength);
      *size = cString.Length();
      *str = ToNewUnicode(NS_ConvertUTF8toUTF16(cString));
      break;
    }
  case nsIDataType::VTYPE_WSTRING_SIZE_IS:
    {
      nsDependentString string(data.u.wstr.mWStringValue,
        data.u.wstr.mWStringLength);
      *size = string.Length();
      *str = ToNewUnicode(string);
      break;
    }
  case nsIDataType::VTYPE_WCHAR:
    tempString.Assign(data.u.mWCharValue);
    *size = tempString.Length();
    *str = ToNewUnicode(tempString);
    break;
  default:
    rv = ToString(data, tempCString);
    if(NS_FAILED(rv))
      return rv;
    *size = tempCString.Length();
    *str = ToNewUnicode(NS_ConvertUTF8toUTF16(tempCString));
    break;
  }

  return *str ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

/* static */ nsresult
sbVariant::ConvertToISupports(const nsDiscriminatedUnion& data,
                              nsISupports **_retval)
{
  switch(data.mType)
  {
  case nsIDataType::VTYPE_INTERFACE:
  case nsIDataType::VTYPE_INTERFACE_IS:
    if (data.u.iface.mInterfaceValue) {
      return data.u.iface.mInterfaceValue->
        QueryInterface(NS_GET_IID(nsISupports), (void**)_retval);
    } else {
      *_retval = nsnull;
      return NS_OK;
    }
  default:
    return NS_ERROR_CANNOT_CONVERT_DATA;
  }
}

/* static */ nsresult
sbVariant::ConvertToInterface(const nsDiscriminatedUnion& data, nsIID * *iid,
                              void * *iface)
{
  const nsIID* piid;

  switch(data.mType)
  {
  case nsIDataType::VTYPE_INTERFACE:
    piid = &NS_GET_IID(nsISupports);
    break;
  case nsIDataType::VTYPE_INTERFACE_IS:
    piid = &data.u.iface.mInterfaceID;
    break;
  default:
    return NS_ERROR_CANNOT_CONVERT_DATA;
  }

  *iid = (nsIID*) SB_CloneMemory(piid, sizeof(nsIID));
  if(!*iid)
    return NS_ERROR_OUT_OF_MEMORY;

  if (data.u.iface.mInterfaceValue) {
    return data.u.iface.mInterfaceValue->QueryInterface(*piid, iface);
  }

  *iface = nsnull;
  return NS_OK;
}

/* static */ nsresult
sbVariant::ConvertToArray(const nsDiscriminatedUnion& data, PRUint16 *type,
                          nsIID* iid, PRUint32 *count, void * *ptr)
{
  // XXX perhaps we'd like to add support for converting each of the various
  // types into an array containing one element of that type. We can leverage
  // CloneArray to do this if we want to support this.

  if(data.mType == nsIDataType::VTYPE_ARRAY)
    return CloneArray(data.u.array.mArrayType, &data.u.array.mArrayInterfaceID,
    data.u.array.mArrayCount, data.u.array.mArrayValue,
    type, iid, count, ptr);
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

/***************************************************************************/
// static setter functions...

#define DATA_SETTER_PROLOGUE(data_)                                           \
  sbVariant::Cleanup(data_);

#define DATA_SETTER_EPILOGUE(data_, type_)                                    \
  data_->mType = nsIDataType :: type_;                                      \
  return NS_OK;

#define DATA_SETTER(data_, type_, member_, value_)                            \
  DATA_SETTER_PROLOGUE(data_)                                               \
  data_->u.member_ = value_;                                                \
  DATA_SETTER_EPILOGUE(data_, type_)

#define DATA_SETTER_WITH_CAST(data_, type_, member_, cast_, value_)           \
  DATA_SETTER_PROLOGUE(data_)                                               \
  data_->u.member_ = cast_ value_;                                          \
  DATA_SETTER_EPILOGUE(data_, type_)


/********************************************/

#define CASE__SET_FROM_VARIANT_VTYPE_PROLOGUE(type_)                          \
{                                                                         \

#define CASE__SET_FROM_VARIANT_VTYPE__GETTER(member_, name_)                  \
  rv = aValue->GetAs##name_ (&(data->u. member_ ));

#define CASE__SET_FROM_VARIANT_VTYPE__GETTER_CAST(cast_, member_, name_)      \
  rv = aValue->GetAs##name_ ( cast_ &(data->u. member_ ));

#define CASE__SET_FROM_VARIANT_VTYPE_EPILOGUE(type_)                          \
  if(NS_SUCCEEDED(rv))                                                  \
{                                                                     \
  data->mType  = nsIDataType :: type_ ;                             \
}                                                                     \
  break;                                                                \
}

#define CASE__SET_FROM_VARIANT_TYPE(type_, member_, name_)                    \
    case nsIDataType :: type_ :                                               \
    CASE__SET_FROM_VARIANT_VTYPE_PROLOGUE(type_)                          \
    CASE__SET_FROM_VARIANT_VTYPE__GETTER(member_, name_)                  \
    CASE__SET_FROM_VARIANT_VTYPE_EPILOGUE(type_)

#define CASE__SET_FROM_VARIANT_VTYPE_CAST(type_, cast_, member_, name_)       \
    case nsIDataType :: type_ :                                               \
    CASE__SET_FROM_VARIANT_VTYPE_PROLOGUE(type_)                          \
    CASE__SET_FROM_VARIANT_VTYPE__GETTER_CAST(cast_, member_, name_)      \
    CASE__SET_FROM_VARIANT_VTYPE_EPILOGUE(type_)


/* static */ nsresult
sbVariant::SetFromVariant(nsDiscriminatedUnion* data, nsIVariant* aValue)
{
  PRUint16 type;
  nsresult rv;

  sbVariant::Cleanup(data);

  rv = aValue->GetDataType(&type);
  if(NS_FAILED(rv))
    return rv;

  switch(type)
  {
    CASE__SET_FROM_VARIANT_VTYPE_CAST(VTYPE_INT8, (PRUint8*), mInt8Value,
      Int8)
      CASE__SET_FROM_VARIANT_TYPE(VTYPE_INT16,  mInt16Value,  Int16)
      CASE__SET_FROM_VARIANT_TYPE(VTYPE_INT32,  mInt32Value,  Int32)
      CASE__SET_FROM_VARIANT_TYPE(VTYPE_UINT8,  mUint8Value,  Uint8)
      CASE__SET_FROM_VARIANT_TYPE(VTYPE_UINT16, mUint16Value, Uint16)
      CASE__SET_FROM_VARIANT_TYPE(VTYPE_UINT32, mUint32Value, Uint32)
      CASE__SET_FROM_VARIANT_TYPE(VTYPE_FLOAT,  mFloatValue,  Float)
      CASE__SET_FROM_VARIANT_TYPE(VTYPE_DOUBLE, mDoubleValue, Double)
      CASE__SET_FROM_VARIANT_TYPE(VTYPE_BOOL ,  mBoolValue,   Bool)
      CASE__SET_FROM_VARIANT_TYPE(VTYPE_CHAR,   mCharValue,   Char)
      CASE__SET_FROM_VARIANT_TYPE(VTYPE_WCHAR,  mWCharValue,  WChar)
      CASE__SET_FROM_VARIANT_TYPE(VTYPE_ID,     mIDValue,     ID)

  case nsIDataType::VTYPE_ASTRING:
  case nsIDataType::VTYPE_DOMSTRING:
  case nsIDataType::VTYPE_WCHAR_STR:
  case nsIDataType::VTYPE_WSTRING_SIZE_IS:
    CASE__SET_FROM_VARIANT_VTYPE_PROLOGUE(VTYPE_ASTRING);
    data->u.mAStringValue = new nsString();
    if(!data->u.mAStringValue)
      return NS_ERROR_OUT_OF_MEMORY;
    rv = aValue->GetAsAString(*data->u.mAStringValue);
    if(NS_FAILED(rv)) {
      nsString *p = static_cast<nsString *>(data->u.mAStringValue);
      delete p;
      data->u.mAStringValue = nsnull;
    }
    CASE__SET_FROM_VARIANT_VTYPE_EPILOGUE(VTYPE_ASTRING)

  case nsIDataType::VTYPE_CSTRING:
    CASE__SET_FROM_VARIANT_VTYPE_PROLOGUE(VTYPE_CSTRING);
    data->u.mCStringValue = new nsCString();
    if(!data->u.mCStringValue)
      return NS_ERROR_OUT_OF_MEMORY;
    rv = aValue->GetAsACString(*data->u.mCStringValue);
    if(NS_FAILED(rv)) {
      nsCString *p = static_cast<nsCString *>(data->u.mCStringValue);
      delete p;
      data->u.mCStringValue = nsnull;
    }
    CASE__SET_FROM_VARIANT_VTYPE_EPILOGUE(VTYPE_CSTRING)

  case nsIDataType::VTYPE_UTF8STRING:
    CASE__SET_FROM_VARIANT_VTYPE_PROLOGUE(VTYPE_UTF8STRING);
    data->u.mUTF8StringValue = new nsUTF8String();
    if(!data->u.mUTF8StringValue)
      return NS_ERROR_OUT_OF_MEMORY;
    rv = aValue->GetAsAUTF8String(*data->u.mUTF8StringValue);
    if(NS_FAILED(rv)) {
      nsUTF8String *p = static_cast<nsUTF8String *>(data->u.mUTF8StringValue);
      delete p;
      data->u.mUTF8StringValue = nsnull;
    }
    CASE__SET_FROM_VARIANT_VTYPE_EPILOGUE(VTYPE_UTF8STRING)

  case nsIDataType::VTYPE_CHAR_STR:
  case nsIDataType::VTYPE_STRING_SIZE_IS:
    CASE__SET_FROM_VARIANT_VTYPE_PROLOGUE(VTYPE_STRING_SIZE_IS);
    rv = aValue->GetAsStringWithSize(&data->u.str.mStringLength,
      &data->u.str.mStringValue);
    CASE__SET_FROM_VARIANT_VTYPE_EPILOGUE(VTYPE_STRING_SIZE_IS)

  case nsIDataType::VTYPE_INTERFACE:
  case nsIDataType::VTYPE_INTERFACE_IS:
    CASE__SET_FROM_VARIANT_VTYPE_PROLOGUE(VTYPE_INTERFACE_IS);
    // XXX This iid handling is ugly!
    nsIID* iid;
    rv = aValue->GetAsInterface(&iid, (void**)&data->u.iface.mInterfaceValue);
    if(NS_SUCCEEDED(rv))
    {
      data->u.iface.mInterfaceID = *iid;
      NS_Free((char*)iid);
    }
    CASE__SET_FROM_VARIANT_VTYPE_EPILOGUE(VTYPE_INTERFACE_IS)

  case nsIDataType::VTYPE_ARRAY:
    CASE__SET_FROM_VARIANT_VTYPE_PROLOGUE(VTYPE_ARRAY);
    rv = aValue->GetAsArray(&data->u.array.mArrayType,
      &data->u.array.mArrayInterfaceID,
      &data->u.array.mArrayCount,
      &data->u.array.mArrayValue);
    CASE__SET_FROM_VARIANT_VTYPE_EPILOGUE(VTYPE_ARRAY)

  case nsIDataType::VTYPE_VOID:
    rv = sbVariant::SetToVoid(data);
    break;
  case nsIDataType::VTYPE_EMPTY_ARRAY:
    rv = sbVariant::SetToEmptyArray(data);
    break;
  case nsIDataType::VTYPE_EMPTY:
    rv = sbVariant::SetToEmpty(data);
    break;
  default:
    NS_ERROR("bad type in variant!");
    rv = NS_ERROR_FAILURE;
    break;
  }
  return rv;
}

/* static */ nsresult
sbVariant::SetFromInt8(nsDiscriminatedUnion* data, PRUint8 aValue)
{
  DATA_SETTER_WITH_CAST(data, VTYPE_INT8, mInt8Value, (PRUint8), aValue)
}
/* static */ nsresult
sbVariant::SetFromInt16(nsDiscriminatedUnion* data, PRInt16 aValue)
{
  DATA_SETTER(data, VTYPE_INT16, mInt16Value, aValue)
}
/* static */ nsresult
sbVariant::SetFromInt32(nsDiscriminatedUnion* data, PRInt32 aValue)
{
  DATA_SETTER(data, VTYPE_INT32, mInt32Value, aValue)
}
/* static */ nsresult
sbVariant::SetFromInt64(nsDiscriminatedUnion* data, PRInt64 aValue)
{
  DATA_SETTER(data, VTYPE_INT64, mInt64Value, aValue)
}
/* static */ nsresult
sbVariant::SetFromUint8(nsDiscriminatedUnion* data, PRUint8 aValue)
{
  DATA_SETTER(data, VTYPE_UINT8, mUint8Value, aValue)
}
/* static */ nsresult
sbVariant::SetFromUint16(nsDiscriminatedUnion* data, PRUint16 aValue)
{
  DATA_SETTER(data, VTYPE_UINT16, mUint16Value, aValue)
}
/* static */ nsresult
sbVariant::SetFromUint32(nsDiscriminatedUnion* data, PRUint32 aValue)
{
  DATA_SETTER(data, VTYPE_UINT32, mUint32Value, aValue)
}
/* static */ nsresult
sbVariant::SetFromUint64(nsDiscriminatedUnion* data, PRUint64 aValue)
{
  DATA_SETTER(data, VTYPE_UINT64, mUint64Value, aValue)
}
/* static */ nsresult
sbVariant::SetFromFloat(nsDiscriminatedUnion* data, float aValue)
{
  DATA_SETTER(data, VTYPE_FLOAT, mFloatValue, aValue)
}
/* static */ nsresult
sbVariant::SetFromDouble(nsDiscriminatedUnion* data, double aValue)
{
  DATA_SETTER(data, VTYPE_DOUBLE, mDoubleValue, aValue)
}
/* static */ nsresult
sbVariant::SetFromBool(nsDiscriminatedUnion* data, PRBool aValue)
{
  DATA_SETTER(data, VTYPE_BOOL, mBoolValue, aValue)
}
/* static */ nsresult
sbVariant::SetFromChar(nsDiscriminatedUnion* data, char aValue)
{
  DATA_SETTER(data, VTYPE_CHAR, mCharValue, aValue)
}
/* static */ nsresult
sbVariant::SetFromWChar(nsDiscriminatedUnion* data, PRUnichar aValue)
{
  DATA_SETTER(data, VTYPE_WCHAR, mWCharValue, aValue)
}
/* static */ nsresult
sbVariant::SetFromID(nsDiscriminatedUnion* data, const nsID & aValue)
{
  DATA_SETTER(data, VTYPE_ID, mIDValue, aValue)
}
/* static */ nsresult
sbVariant::SetFromAString(nsDiscriminatedUnion* data, const nsAString & aValue)
{
  DATA_SETTER_PROLOGUE(data);
  if(!(data->u.mAStringValue = new nsString(aValue)))
    return NS_ERROR_OUT_OF_MEMORY;
  DATA_SETTER_EPILOGUE(data, VTYPE_ASTRING);
}

/* static */ nsresult
sbVariant::SetFromACString(nsDiscriminatedUnion* data,
                           const nsACString & aValue)
{
  DATA_SETTER_PROLOGUE(data);
  if(!(data->u.mCStringValue = new nsCString(aValue)))
    return NS_ERROR_OUT_OF_MEMORY;
  DATA_SETTER_EPILOGUE(data, VTYPE_CSTRING);
}

/* static */ nsresult
sbVariant::SetFromAUTF8String(nsDiscriminatedUnion* data,
                              const nsAUTF8String & aValue)
{
  DATA_SETTER_PROLOGUE(data);
  if(!(data->u.mUTF8StringValue = new nsUTF8String(aValue)))
    return NS_ERROR_OUT_OF_MEMORY;
  DATA_SETTER_EPILOGUE(data, VTYPE_UTF8STRING);
}

/* static */ nsresult
sbVariant::SetFromString(nsDiscriminatedUnion* data, const char *aValue)
{
  DATA_SETTER_PROLOGUE(data);
  if(!aValue)
    return NS_ERROR_NULL_POINTER;
  return SetFromStringWithSize(data, strlen(aValue), aValue);
}
/* static */ nsresult
sbVariant::SetFromWString(nsDiscriminatedUnion* data, const PRUnichar *aValue)
{
  DATA_SETTER_PROLOGUE(data);
  if(!aValue)
    return NS_ERROR_NULL_POINTER;
  return SetFromWStringWithSize(data, nsCRT::strlen(aValue), aValue);
}
/* static */ nsresult
sbVariant::SetFromISupports(nsDiscriminatedUnion* data, nsISupports *aValue)
{
  return SetFromInterface(data, NS_GET_IID(nsISupports), aValue);
}
/* static */ nsresult
sbVariant::SetFromInterface(nsDiscriminatedUnion* data, const nsIID& iid,
                            nsISupports *aValue)
{
  DATA_SETTER_PROLOGUE(data);
  NS_IF_ADDREF(aValue);
  data->u.iface.mInterfaceValue = aValue;
  data->u.iface.mInterfaceID = iid;
  DATA_SETTER_EPILOGUE(data, VTYPE_INTERFACE_IS);
}
/* static */ nsresult
sbVariant::SetFromArray(nsDiscriminatedUnion* data, PRUint16 type,
                        const nsIID* iid, PRUint32 count, void * aValue)
{
  DATA_SETTER_PROLOGUE(data);
  if(!aValue || !count)
    return NS_ERROR_NULL_POINTER;

  nsresult rv = CloneArray(type, iid, count, aValue,
    &data->u.array.mArrayType,
    &data->u.array.mArrayInterfaceID,
    &data->u.array.mArrayCount,
    &data->u.array.mArrayValue);
  if(NS_FAILED(rv))
    return rv;
  DATA_SETTER_EPILOGUE(data, VTYPE_ARRAY);
}
/* static */ nsresult
sbVariant::SetFromStringWithSize(nsDiscriminatedUnion* data, PRUint32 size, const char *aValue)
{
  DATA_SETTER_PROLOGUE(data);
  if(!aValue)
    return NS_ERROR_NULL_POINTER;
  if(!(data->u.str.mStringValue =
    (char*) SB_CloneMemory(aValue, (size+1)*sizeof(char))))
    return NS_ERROR_OUT_OF_MEMORY;
  data->u.str.mStringLength = size;
  DATA_SETTER_EPILOGUE(data, VTYPE_STRING_SIZE_IS);
}
/* static */ nsresult
sbVariant::SetFromWStringWithSize(nsDiscriminatedUnion* data, PRUint32 size, const PRUnichar *aValue)
{
  DATA_SETTER_PROLOGUE(data);
  if(!aValue)
    return NS_ERROR_NULL_POINTER;
  if(!(data->u.wstr.mWStringValue =
    (PRUnichar*) SB_CloneMemory(aValue, (size+1)*sizeof(PRUnichar))))
    return NS_ERROR_OUT_OF_MEMORY;
  data->u.wstr.mWStringLength = size;
  DATA_SETTER_EPILOGUE(data, VTYPE_WSTRING_SIZE_IS);
}
/* static */ nsresult
sbVariant::SetToVoid(nsDiscriminatedUnion* data)
{
  DATA_SETTER_PROLOGUE(data);
  DATA_SETTER_EPILOGUE(data, VTYPE_VOID);
}
/* static */ nsresult
sbVariant::SetToEmpty(nsDiscriminatedUnion* data)
{
  DATA_SETTER_PROLOGUE(data);
  DATA_SETTER_EPILOGUE(data, VTYPE_EMPTY);
}
/* static */ nsresult
sbVariant::SetToEmptyArray(nsDiscriminatedUnion* data)
{
  DATA_SETTER_PROLOGUE(data);
  DATA_SETTER_EPILOGUE(data, VTYPE_EMPTY_ARRAY);
}

/***************************************************************************/

/* static */ nsresult
sbVariant::Initialize(nsDiscriminatedUnion* data)
{
  data->mType = nsIDataType::VTYPE_EMPTY;
  return NS_OK;
}

/* static */ nsresult
sbVariant::Cleanup(nsDiscriminatedUnion* data)
{
  switch(data->mType)
  {
  case nsIDataType::VTYPE_INT8:
  case nsIDataType::VTYPE_INT16:
  case nsIDataType::VTYPE_INT32:
  case nsIDataType::VTYPE_INT64:
  case nsIDataType::VTYPE_UINT8:
  case nsIDataType::VTYPE_UINT16:
  case nsIDataType::VTYPE_UINT32:
  case nsIDataType::VTYPE_UINT64:
  case nsIDataType::VTYPE_FLOAT:
  case nsIDataType::VTYPE_DOUBLE:
  case nsIDataType::VTYPE_BOOL:
  case nsIDataType::VTYPE_CHAR:
  case nsIDataType::VTYPE_WCHAR:
  case nsIDataType::VTYPE_VOID:
  case nsIDataType::VTYPE_ID:
    break;
  case nsIDataType::VTYPE_ASTRING:
  case nsIDataType::VTYPE_DOMSTRING:
    {
      nsString *p = static_cast<nsString *>(data->u.mAStringValue);
      delete p;
      data->u.mAStringValue = nsnull;
    }
    break;
  case nsIDataType::VTYPE_CSTRING:
    {
      nsCString *p = static_cast<nsCString *>(data->u.mCStringValue);
      delete p;
      data->u.mCStringValue = nsnull;
    }
    break;
  case nsIDataType::VTYPE_UTF8STRING:
    {
      nsUTF8String *p = static_cast<nsUTF8String *>(data->u.mUTF8StringValue);
      delete p;
      data->u.mUTF8StringValue= nsnull;
    }
    break;
  case nsIDataType::VTYPE_CHAR_STR:
  case nsIDataType::VTYPE_STRING_SIZE_IS:
    NS_Free((char*)data->u.str.mStringValue);
    break;
  case nsIDataType::VTYPE_WCHAR_STR:
  case nsIDataType::VTYPE_WSTRING_SIZE_IS:
    NS_Free((char*)data->u.wstr.mWStringValue);
    break;
  case nsIDataType::VTYPE_INTERFACE:
  case nsIDataType::VTYPE_INTERFACE_IS:
    NS_IF_RELEASE(data->u.iface.mInterfaceValue);
    break;
  case nsIDataType::VTYPE_ARRAY:
    FreeArray(data);
    break;
  case nsIDataType::VTYPE_EMPTY_ARRAY:
  case nsIDataType::VTYPE_EMPTY:
    break;
  default:
    NS_ERROR("bad type in variant!");
    break;
  }

  data->mType = nsIDataType::VTYPE_EMPTY;
  return NS_OK;
}

/***************************************************************************/
/***************************************************************************/
// members...

NS_IMPL_THREADSAFE_ISUPPORTS2(sbVariant, 
                              nsIVariant, 
                              nsIWritableVariant)

sbVariant::sbVariant()
: mWritable(PR_TRUE)
, mDataLock(nsnull)
{
  sbVariant::Initialize(&mData);
  mDataLock = nsAutoLock::NewLock("sbVariant::mLock");
  NS_WARN_IF_FALSE(mDataLock, "Failed to create data lock.");

#ifdef DEBUG
  {
    // Assert that the nsIDataType consts match the values #defined in
    // xpt_struct.h. Bad things happen somewhere if they don't.
    struct THE_TYPES {PRUint16 a; PRUint16 b;};
    static const THE_TYPES array[] = {
      {nsIDataType::VTYPE_INT8              , TD_INT8             },
      {nsIDataType::VTYPE_INT16             , TD_INT16            },
      {nsIDataType::VTYPE_INT32             , TD_INT32            },
      {nsIDataType::VTYPE_INT64             , TD_INT64            },
      {nsIDataType::VTYPE_UINT8             , TD_UINT8            },
      {nsIDataType::VTYPE_UINT16            , TD_UINT16           },
      {nsIDataType::VTYPE_UINT32            , TD_UINT32           },
      {nsIDataType::VTYPE_UINT64            , TD_UINT64           },
      {nsIDataType::VTYPE_FLOAT             , TD_FLOAT            },
      {nsIDataType::VTYPE_DOUBLE            , TD_DOUBLE           },
      {nsIDataType::VTYPE_BOOL              , TD_BOOL             },
      {nsIDataType::VTYPE_CHAR              , TD_CHAR             },
      {nsIDataType::VTYPE_WCHAR             , TD_WCHAR            },
      {nsIDataType::VTYPE_VOID              , TD_VOID             },
      {nsIDataType::VTYPE_ID                , TD_PNSIID           },
      {nsIDataType::VTYPE_DOMSTRING         , TD_DOMSTRING        },
      {nsIDataType::VTYPE_CHAR_STR          , TD_PSTRING          },
      {nsIDataType::VTYPE_WCHAR_STR         , TD_PWSTRING         },
      {nsIDataType::VTYPE_INTERFACE         , TD_INTERFACE_TYPE   },
      {nsIDataType::VTYPE_INTERFACE_IS      , TD_INTERFACE_IS_TYPE},
      {nsIDataType::VTYPE_ARRAY             , TD_ARRAY            },
      {nsIDataType::VTYPE_STRING_SIZE_IS    , TD_PSTRING_SIZE_IS  },
      {nsIDataType::VTYPE_WSTRING_SIZE_IS   , TD_PWSTRING_SIZE_IS },
      {nsIDataType::VTYPE_UTF8STRING        , TD_UTF8STRING       },
      {nsIDataType::VTYPE_CSTRING           , TD_CSTRING          },
      {nsIDataType::VTYPE_ASTRING           , TD_ASTRING          }
    };
    static const int length = sizeof(array)/sizeof(array[0]);
    static PRBool inited = PR_FALSE;
    if(!inited)
    {
      for(int i = 0; i < length; i++)
        NS_ASSERTION(array[i].a == array[i].b, "bad const declaration");
      inited = PR_TRUE;
    }
  }
#endif
}

sbVariant::~sbVariant()
{
  sbVariant::Cleanup(&mData);
  if(mDataLock) {
    nsAutoLock::DestroyLock(mDataLock);
  }
}

// For all the data getters we just forward to the static (and sharable)
// 'ConvertTo' functions.

/* readonly attribute PRUint16 dataType; */
NS_IMETHODIMP sbVariant::GetDataType(PRUint16 *aDataType)
{
  nsAutoLock lock(mDataLock);
  *aDataType = mData.mType;
  return NS_OK;
}

/* PRUint8 getAsInt8 (); */
NS_IMETHODIMP sbVariant::GetAsInt8(PRUint8 *_retval)
{
  nsAutoLock lock(mDataLock);
  return sbVariant::ConvertToInt8(mData, _retval);
}

/* PRInt16 getAsInt16 (); */
NS_IMETHODIMP sbVariant::GetAsInt16(PRInt16 *_retval)
{
  nsAutoLock lock(mDataLock);
  return sbVariant::ConvertToInt16(mData, _retval);
}

/* PRInt32 getAsInt32 (); */
NS_IMETHODIMP sbVariant::GetAsInt32(PRInt32 *_retval)
{
  nsAutoLock lock(mDataLock);
  return sbVariant::ConvertToInt32(mData, _retval);
}

/* PRInt64 getAsInt64 (); */
NS_IMETHODIMP sbVariant::GetAsInt64(PRInt64 *_retval)
{
  nsAutoLock lock(mDataLock);
  return sbVariant::ConvertToInt64(mData, _retval);
}

/* PRUint8 getAsUint8 (); */
NS_IMETHODIMP sbVariant::GetAsUint8(PRUint8 *_retval)
{
  nsAutoLock lock(mDataLock);
  return sbVariant::ConvertToUint8(mData, _retval);
}

/* PRUint16 getAsUint16 (); */
NS_IMETHODIMP sbVariant::GetAsUint16(PRUint16 *_retval)
{
  nsAutoLock lock(mDataLock);
  return sbVariant::ConvertToUint16(mData, _retval);
}

/* PRUint32 getAsUint32 (); */
NS_IMETHODIMP sbVariant::GetAsUint32(PRUint32 *_retval)
{
  nsAutoLock lock(mDataLock);
  return sbVariant::ConvertToUint32(mData, _retval);
}

/* PRUint64 getAsUint64 (); */
NS_IMETHODIMP sbVariant::GetAsUint64(PRUint64 *_retval)
{
  nsAutoLock lock(mDataLock);
  return sbVariant::ConvertToUint64(mData, _retval);
}

/* float getAsFloat (); */
NS_IMETHODIMP sbVariant::GetAsFloat(float *_retval)
{
  nsAutoLock lock(mDataLock);
  return sbVariant::ConvertToFloat(mData, _retval);
}

/* double getAsDouble (); */
NS_IMETHODIMP sbVariant::GetAsDouble(double *_retval)
{
  nsAutoLock lock(mDataLock);
  return sbVariant::ConvertToDouble(mData, _retval);
}

/* PRBool getAsBool (); */
NS_IMETHODIMP sbVariant::GetAsBool(PRBool *_retval)
{
  nsAutoLock lock(mDataLock);
  return sbVariant::ConvertToBool(mData, _retval);
}

/* char getAsChar (); */
NS_IMETHODIMP sbVariant::GetAsChar(char *_retval)
{
  nsAutoLock lock(mDataLock);
  return sbVariant::ConvertToChar(mData, _retval);
}

/* wchar getAsWChar (); */
NS_IMETHODIMP sbVariant::GetAsWChar(PRUnichar *_retval)
{
  nsAutoLock lock(mDataLock);
  return sbVariant::ConvertToWChar(mData, _retval);
}

/* [notxpcom] nsresult getAsID (out nsID retval); */
NS_IMETHODIMP_(nsresult) sbVariant::GetAsID(nsID *retval)
{
  nsAutoLock lock(mDataLock);
  return sbVariant::ConvertToID(mData, retval);
}

/* AString getAsAString (); */
NS_IMETHODIMP sbVariant::GetAsAString(nsAString & _retval)
{
  nsAutoLock lock(mDataLock);
  return sbVariant::ConvertToAString(mData, _retval);
}

/* DOMString getAsDOMString (); */
NS_IMETHODIMP sbVariant::GetAsDOMString(nsAString & _retval)
{
  nsAutoLock lock(mDataLock);

  // A DOMString maps to an AString internally, so we can re-use
  // ConvertToAString here.
  return sbVariant::ConvertToAString(mData, _retval);
}

/* ACString getAsACString (); */
NS_IMETHODIMP sbVariant::GetAsACString(nsACString & _retval)
{
  nsAutoLock lock(mDataLock);
  return sbVariant::ConvertToACString(mData, _retval);
}

/* AUTF8String getAsAUTF8String (); */
NS_IMETHODIMP sbVariant::GetAsAUTF8String(nsAUTF8String & _retval)
{
  nsAutoLock lock(mDataLock);
  return sbVariant::ConvertToAUTF8String(mData, _retval);
}

/* string getAsString (); */
NS_IMETHODIMP sbVariant::GetAsString(char **_retval)
{
  nsAutoLock lock(mDataLock);
  return sbVariant::ConvertToString(mData, _retval);
}

/* wstring getAsWString (); */
NS_IMETHODIMP sbVariant::GetAsWString(PRUnichar **_retval)
{
  nsAutoLock lock(mDataLock);
  return sbVariant::ConvertToWString(mData, _retval);
}

/* nsISupports getAsISupports (); */
NS_IMETHODIMP sbVariant::GetAsISupports(nsISupports **_retval)
{
  nsAutoLock lock(mDataLock);
  return sbVariant::ConvertToISupports(mData, _retval);
}

/* void getAsInterface (out nsIIDPtr iid, [iid_is (iid), retval] out nsQIResult iface); */
NS_IMETHODIMP sbVariant::GetAsInterface(nsIID * *iid, void * *iface)
{
  nsAutoLock lock(mDataLock);
  return sbVariant::ConvertToInterface(mData, iid, iface);
}

/* [notxpcom] nsresult getAsArray (out PRUint16 type, out nsIID iid, out PRUint32 count, out voidPtr ptr); */
NS_IMETHODIMP_(nsresult) sbVariant::GetAsArray(PRUint16 *type, nsIID *iid, PRUint32 *count, void * *ptr)
{
  nsAutoLock lock(mDataLock);
  return sbVariant::ConvertToArray(mData, type, iid, count, ptr);
}

/* void getAsStringWithSize (out PRUint32 size, [size_is (size), retval] out string str); */
NS_IMETHODIMP sbVariant::GetAsStringWithSize(PRUint32 *size, char **str)
{
  nsAutoLock lock(mDataLock);
  return sbVariant::ConvertToStringWithSize(mData, size, str);
}

/* void getAsWStringWithSize (out PRUint32 size, [size_is (size), retval] out wstring str); */
NS_IMETHODIMP sbVariant::GetAsWStringWithSize(PRUint32 *size, PRUnichar **str)
{
  nsAutoLock lock(mDataLock);
  return sbVariant::ConvertToWStringWithSize(mData, size, str);
}

/***************************************************************************/

/* attribute PRBool writable; */
NS_IMETHODIMP sbVariant::GetWritable(PRBool *aWritable)
{
  nsAutoLock lock(mDataLock);
  *aWritable = mWritable;
  return NS_OK;
}
NS_IMETHODIMP sbVariant::SetWritable(PRBool aWritable)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable && aWritable)
    return NS_ERROR_FAILURE;
  mWritable = aWritable;
  return NS_OK;
}

/***************************************************************************/

// For all the data setters we just forward to the static (and sharable)
// 'SetFrom' functions.

/* void setAsInt8 (in PRUint8 aValue); */
NS_IMETHODIMP sbVariant::SetAsInt8(PRUint8 aValue)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetFromInt8(&mData, aValue);
}

/* void setAsInt16 (in PRInt16 aValue); */
NS_IMETHODIMP sbVariant::SetAsInt16(PRInt16 aValue)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetFromInt16(&mData, aValue);
}

/* void setAsInt32 (in PRInt32 aValue); */
NS_IMETHODIMP sbVariant::SetAsInt32(PRInt32 aValue)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetFromInt32(&mData, aValue);
}

/* void setAsInt64 (in PRInt64 aValue); */
NS_IMETHODIMP sbVariant::SetAsInt64(PRInt64 aValue)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetFromInt64(&mData, aValue);
}

/* void setAsUint8 (in PRUint8 aValue); */
NS_IMETHODIMP sbVariant::SetAsUint8(PRUint8 aValue)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetFromUint8(&mData, aValue);
}

/* void setAsUint16 (in PRUint16 aValue); */
NS_IMETHODIMP sbVariant::SetAsUint16(PRUint16 aValue)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetFromUint16(&mData, aValue);
}

/* void setAsUint32 (in PRUint32 aValue); */
NS_IMETHODIMP sbVariant::SetAsUint32(PRUint32 aValue)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetFromUint32(&mData, aValue);
}

/* void setAsUint64 (in PRUint64 aValue); */
NS_IMETHODIMP sbVariant::SetAsUint64(PRUint64 aValue)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetFromUint64(&mData, aValue);
}

/* void setAsFloat (in float aValue); */
NS_IMETHODIMP sbVariant::SetAsFloat(float aValue)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetFromFloat(&mData, aValue);
}

/* void setAsDouble (in double aValue); */
NS_IMETHODIMP sbVariant::SetAsDouble(double aValue)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetFromDouble(&mData, aValue);
}

/* void setAsBool (in PRBool aValue); */
NS_IMETHODIMP sbVariant::SetAsBool(PRBool aValue)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetFromBool(&mData, aValue);
}

/* void setAsChar (in char aValue); */
NS_IMETHODIMP sbVariant::SetAsChar(char aValue)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetFromChar(&mData, aValue);
}

/* void setAsWChar (in wchar aValue); */
NS_IMETHODIMP sbVariant::SetAsWChar(PRUnichar aValue)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetFromWChar(&mData, aValue);
}

/* void setAsID (in nsIDRef aValue); */
NS_IMETHODIMP sbVariant::SetAsID(const nsID & aValue)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetFromID(&mData, aValue);
}

/* void setAsAString (in AString aValue); */
NS_IMETHODIMP sbVariant::SetAsAString(const nsAString & aValue)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetFromAString(&mData, aValue);
}

/* void setAsDOMString (in DOMString aValue); */
NS_IMETHODIMP sbVariant::SetAsDOMString(const nsAString & aValue)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;

  DATA_SETTER_PROLOGUE((&mData));
  if(!(mData.u.mAStringValue = new nsString(aValue)))
    return NS_ERROR_OUT_OF_MEMORY;
  DATA_SETTER_EPILOGUE((&mData), VTYPE_DOMSTRING);
}

/* void setAsACString (in ACString aValue); */
NS_IMETHODIMP sbVariant::SetAsACString(const nsACString & aValue)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetFromACString(&mData, aValue);
}

/* void setAsAUTF8String (in AUTF8String aValue); */
NS_IMETHODIMP sbVariant::SetAsAUTF8String(const nsAUTF8String & aValue)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetFromAUTF8String(&mData, aValue);
}

/* void setAsString (in string aValue); */
NS_IMETHODIMP sbVariant::SetAsString(const char *aValue)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetFromString(&mData, aValue);
}

/* void setAsWString (in wstring aValue); */
NS_IMETHODIMP sbVariant::SetAsWString(const PRUnichar *aValue)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetFromWString(&mData, aValue);
}

/* void setAsISupports (in nsISupports aValue); */
NS_IMETHODIMP sbVariant::SetAsISupports(nsISupports *aValue)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetFromISupports(&mData, aValue);
}

/* void setAsInterface (in nsIIDRef iid, [iid_is (iid)] in nsQIResult iface); */
NS_IMETHODIMP sbVariant::SetAsInterface(const nsIID & iid, void * iface)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetFromInterface(&mData, iid, (nsISupports*)iface);
}

/* [noscript] void setAsArray (in PRUint16 type, in nsIIDPtr iid, in PRUint32 count, in voidPtr ptr); */
NS_IMETHODIMP sbVariant::SetAsArray(PRUint16 type, const nsIID * iid, PRUint32 count, void * ptr)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetFromArray(&mData, type, iid, count, ptr);
}

/* void setAsStringWithSize (in PRUint32 size, [size_is (size)] in string str); */
NS_IMETHODIMP sbVariant::SetAsStringWithSize(PRUint32 size, const char *str)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetFromStringWithSize(&mData, size, str);
}

/* void setAsWStringWithSize (in PRUint32 size, [size_is (size)] in wstring str); */
NS_IMETHODIMP sbVariant::SetAsWStringWithSize(PRUint32 size, const PRUnichar *str)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetFromWStringWithSize(&mData, size, str);
}

/* void setAsVoid (); */
NS_IMETHODIMP sbVariant::SetAsVoid()
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetToVoid(&mData);
}

/* void setAsEmpty (); */
NS_IMETHODIMP sbVariant::SetAsEmpty()
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetToEmpty(&mData);
}

/* void setAsEmptyArray (); */
NS_IMETHODIMP sbVariant::SetAsEmptyArray()
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetToEmptyArray(&mData);
}

/* void setFromVariant (in nsIVariant aValue); */
NS_IMETHODIMP sbVariant::SetFromVariant(nsIVariant *aValue)
{
  nsAutoLock lock(mDataLock);
  if(!mWritable) return NS_ERROR_OBJECT_IS_IMMUTABLE;
  return sbVariant::SetFromVariant(&mData, aValue);
}
