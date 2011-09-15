/*
 //
 // BEGIN SONGBIRD GPL
 //
 // This file is part of the Songbird web player.
 //
 // Copyright(c) 2005-2009 POTI, Inc.
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

#include "sbIDispatchPtr.h"

#include <algorithm>
#include <assert.h>
#include <vector>

/**
 * Returns true if this is the busy error and we need to retry
 * also perform a sleep so we don't hog the CPU
 */
static inline
bool COMBusyError(HRESULT error) {
  HRESULT const SB_ITUNES_ERROR_BUSY = 0x8001010a;
  bool const busy = (error == SB_ITUNES_ERROR_BUSY);
  if (busy) {
    // Sleep for 1/10 second
    Sleep(100);
  }
  return busy;
}

inline
void Initialize(VARIANTARG & aVar) {
  VariantInit(&aVar);
}

inline
void Destroy(VARIANTARG & aVar) {
  VariantClear(&aVar);
}

HRESULT SBVariantCopy(VARIANT & aDestination, VARIANT & aSource) {
  VariantClear(&aDestination);
  if ((aSource.vt & VT_BYREF) == VT_BYREF) {
    return VariantCopyInd(&aDestination, &aSource);    
  }
  else if ((aSource.vt & VT_ARRAY) == VT_ARRAY) {
    aDestination.vt = aSource.vt;
    return SafeArrayCopy(aSource.parray, &aDestination.parray);
  }
  return VariantCopy(&aDestination, &aSource);
}

sbIDispatchPtr::VarArgs::VarArgs(size_t aArgs) : mVariantCount(0),
                                                 mParamsReversed(false) {
  VARIANTARG varArg;
  ::VariantInit(&varArg);
  mVariants.resize(aArgs, varArg);
  std::for_each(mVariants.begin(), mVariants.end(), Initialize);  
}

sbIDispatchPtr::VarArgs::~VarArgs() {
  Clear();
}

HRESULT sbIDispatchPtr::VarArgs::Append(std::wstring const & aArg) {
  assert(mVariantCount < mVariants.size());
  assert(!mParamsReversed);
  
  VARIANTARG & arg = mVariants[mVariantCount++];
  arg.bstrVal = SysAllocStringLen(aArg.c_str(), aArg.length());
  if (arg.bstrVal == 0) {
    return E_OUTOFMEMORY;
  }
  arg.vt = VT_BSTR;
  return S_OK;
}

HRESULT sbIDispatchPtr::VarArgs::Append(const long aLong)
{
  assert(mVariantCount < mVariants.size());
  assert(!mParamsReversed);

  VARIANTARG & arg = mVariants[mVariantCount++];
  arg.lVal = aLong;
  arg.vt = VT_I4;
  return S_OK;
}

HRESULT sbIDispatchPtr::VarArgs::Append(VARIANTARG & aVar) {
  assert(mVariantCount < mVariants.size());
  assert(!mParamsReversed);
  
  VARIANTARG & arg = mVariants[mVariantCount++];
  HRESULT hr = SBVariantCopy(arg, aVar);
  if (FAILED(hr)) {
    return hr;
  }
  return S_OK;
}

HRESULT sbIDispatchPtr::VarArgs::Append(IDispatch * aObj) {
  assert(mVariantCount < mVariants.size());
  assert(!mParamsReversed);

  VARIANTARG & arg = mVariants[mVariantCount++];
  arg.pdispVal = aObj;
  arg.vt = VT_DISPATCH;
  aObj->AddRef();
  return S_OK;
}

HRESULT sbIDispatchPtr::VarArgs::AppendOutIDispatch(IDispatch ** aOutObj) {
  assert(mVariantCount < mVariants.size());
  assert(!mParamsReversed);

  VARIANTARG & arg = mVariants[mVariantCount++];
  arg.ppdispVal = aOutObj;
  arg.vt = VT_BYREF | VT_DISPATCH;
  return S_OK;
}

HRESULT sbIDispatchPtr::VarArgs::Clear() {
  std::for_each(mVariants.begin(), mVariants.end(), Destroy);
  mVariantCount = 0;
  mParamsReversed = false;
  return S_OK;
}

HRESULT sbIDispatchPtr::VarArgs::GetValueAt(const size_t aIndex,
                                            std::wstring & aResult) {
  assert(mVariantCount <= mVariants.size());
  if (mVariantCount <= aIndex || mVariants[aIndex].vt != VT_BSTR) {
    return E_FAIL;
  }
  aResult = std::wstring(mVariants[aIndex].bstrVal,
                         SysStringLen(mVariants[aIndex].bstrVal));
  return S_OK;
}

HRESULT sbIDispatchPtr::VarArgs::GetValueAt(const size_t aIndex,
                                            long & aResult) {
  assert(mVariantCount <= mVariants.size());
  if (mVariantCount <= aIndex || mVariants[aIndex].vt != VT_I4) {
    return E_FAIL;
  }
  aResult = mVariants[aIndex].lVal;
  return S_OK;
}

HRESULT sbIDispatchPtr::VarArgs::GetValueAt(const size_t aIndex,
                                            VARIANTARG & aResult) {
  assert(mVariantCount <= mVariants.size());
  if (mVariantCount <= aIndex) {
    return E_FAIL;
  }
  return ::VariantCopy(&aResult, &mVariants[aIndex]);
}

DISPPARAMS * sbIDispatchPtr::VarArgs::GetDispParams() {
  // IDispatch reverses the order of the parameters. This check allows
  // GetDispParams to be called multiple times
  if (!mParamsReversed) {
    std::reverse(mVariants.begin(), mVariants.begin() + mVariantCount);
    mParamsReversed = true;
  }
  // Set to null if the array is empty
  mDispParams.rgvarg = mVariants.empty() ? 0 : &(*mVariants.begin());
  mDispParams.rgdispidNamedArgs = 0;
  mDispParams.cArgs = mVariantCount;
  mDispParams.cNamedArgs = 0;
  return & mDispParams;
}

HRESULT sbIDispatchPtr::Invoke(wchar_t const * aMethodName, 
                               VarArgs & aVarArgs,
                               VARIANTARG & aResult,
                               WORD aFlags) {
  EXCEPINFO exceptionInfo;
  unsigned int argErr;
  
  DISPID dispID = GetDispID(aMethodName);
  if (dispID == UNKNOWN_DISPID) {
    return DISP_E_MEMBERNOTFOUND;
  }
  
  HRESULT hr;
  do {
    hr = mPtr->Invoke(dispID,
                      IID_NULL,
                      LOCALE_SYSTEM_DEFAULT,
                      aFlags,
                      aVarArgs.GetDispParams(),
                      &aResult,
                      &exceptionInfo,
                      &argErr);
  } while (COMBusyError(hr));
  return hr;
}

HRESULT sbIDispatchPtr::GetProperty(wchar_t const * aPropertyName, 
                                    VARIANTARG & aVar) {
  DISPPARAMS dispParams = { 0, 0, 0, 0 };
  EXCEPINFO exceptionInfo;
  unsigned int argErr;
  
  DISPID dispID = GetDispID(aPropertyName);
  if (dispID == UNKNOWN_DISPID) {
    return DISP_E_MEMBERNOTFOUND;
  }
  
  HRESULT hr;
  do {
    hr = mPtr->Invoke(dispID,
                      IID_NULL,
                      LOCALE_SYSTEM_DEFAULT,
                      DISPATCH_PROPERTYGET,
                      &dispParams,
                      &aVar,
                      &exceptionInfo,
                      &argErr);
  } while (COMBusyError(hr));
  return hr;
}

HRESULT sbIDispatchPtr::GetProperty(wchar_t const * aPropertyName,
                                    IDispatch ** aPtr) {
  assert(aPtr);
  
  VARIANTARG var;
  VariantClear(&var);
  HRESULT hr = GetProperty(aPropertyName, var);
  if (SUCCEEDED(hr)) {
    if (var.vt == VT_DISPATCH) {
      if (var.pdispVal) {
        *aPtr = var.pdispVal;
        (*aPtr)->AddRef();
        VariantClear(&var);
      }
      else {
        hr = DISP_E_MEMBERNOTFOUND;
      }
    }
    else {
      hr = DISP_E_TYPEMISMATCH;
    }
  }
  return hr;
}

HRESULT sbIDispatchPtr::GetProperty(wchar_t const * aPropertyName, 
                                    bool & aBool) {
  
  VARIANTARG var;
  VariantClear(&var);
  HRESULT hr = GetProperty(aPropertyName, var);
  if (SUCCEEDED(hr)) {
    if (var.vt == VT_BOOL) {
      aBool = var.pboolVal ? true : false;
      VariantClear(&var);
    }
    else {
      hr = DISP_E_TYPEMISMATCH;
    }
  }
  return hr;
}

HRESULT sbIDispatchPtr::GetProperty(wchar_t const * aPropertyName,
                                    long & aLong)
{
  VARIANTARG var;
  VariantClear(&var);

  HRESULT hr = GetProperty(aPropertyName, var);
  if (SUCCEEDED(hr)) {
    if (var.vt == VT_I4) {
      aLong = var.lVal;
      VariantClear(&var);
    }
    else {
      hr = DISP_E_TYPEMISMATCH;
    }
  }
  return hr;
}

HRESULT sbIDispatchPtr::GetProperty(wchar_t const * aPropertyName,
                                    std::wstring & aString)
{
  VARIANTARG var;
  VariantClear(&var);

  HRESULT hr = GetProperty(aPropertyName, var);
  if (SUCCEEDED(hr)) {
    if (var.vt == VT_BSTR) {
      aString = std::wstring(var.bstrVal);
      VariantClear(&var);
    }
    else {
      hr = DISP_E_TYPEMISMATCH;
    }
  }
  
  return hr;
}

HRESULT sbIDispatchPtr::SetProperty(wchar_t const * aPropertyName, VARIANTARG & aVar) {
  DISPID dispidNamed = DISPID_PROPERTYPUT;
  DISPPARAMS dispParams = { &aVar, &dispidNamed, 1, 1 };
  EXCEPINFO exceptionInfo;
  unsigned int argErr;
  
  DISPID dispID = GetDispID(aPropertyName);
  if (dispID == UNKNOWN_DISPID) {
    return DISP_E_MEMBERNOTFOUND;
  }
  
  HRESULT hr;
  do {
    hr = mPtr->Invoke(dispID,
                      IID_NULL,
                      LOCALE_SYSTEM_DEFAULT,
                      DISPATCH_PROPERTYPUT,
                      &dispParams,
                      &aVar,
                      &exceptionInfo,
                      &argErr);
  } while (COMBusyError(hr));
  return hr;
}

DISPID sbIDispatchPtr::GetDispID(wchar_t const * aName) {
  DISPID dispID = UNKNOWN_DISPID;
  MethodNames::const_iterator iter = mMethodNames.find(std::wstring(aName));
  if (iter != mMethodNames.end()) {
   dispID = iter->second; 
  }
  else {
    // Ugh, have to cast away constantness
    LPOLESTR name = const_cast<LPOLESTR>(aName);
    HRESULT hr = mPtr->GetIDsOfNames(IID_NULL, 
                                     &name, 
                                     1, 
                                     LOCALE_SYSTEM_DEFAULT, 
                                     &dispID);
    // Just make sure we indicate that it wasn't found
    if (SUCCEEDED(hr) && dispID != UNKNOWN_DISPID) {
      mMethodNames.insert(MethodNames::value_type(aName, dispID));
    } 
    else {
      dispID = UNKNOWN_DISPID;
    }
  }
  return dispID;
}
