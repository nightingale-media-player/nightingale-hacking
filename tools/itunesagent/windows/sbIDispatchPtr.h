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
#ifndef SBIDISPATCHPTR_H_
#define SBIDISPATCHPTR_H_

#include <oaidl.h>
#include <map>
#include <string>
#include <vector>

/**
 * This wraps an IDispatch pointer and provides conveniences methods as well
 * addref'ing and releasing the pointer when appropriate. This is far from a 
 * complete implementation. This contains the necessary methods for what
 * the iTunes agent needed.
 */
class sbIDispatchPtr
{
public:
  /**
   * Constant denoting an unknown DISPID value
   */
  static DISPID const UNKNOWN_DISPID = -1;
  /**
   * This class manages a collection of arguments. It owns the arguments
   * it is holding and releases them when it's destructed.
   */
  class VarArgs {
  public:
    /**
     * Allows sbIDispatchPtr access to GetDispParams
     */ 
    friend class sbIDispatchPtr;
    
    /**
     * Initializes the variant array
     */
    VarArgs(size_t aArgs = 0);
    
    /**
     * Cleans up and releases all the variants we hold
     */
    ~VarArgs();
    
    /**
     * Appends a string argument as a BSTR
     */
    HRESULT Append(std::wstring const & aArg);

    /**
     * Appends a long integer argument.
     */
    HRESULT Append(const long aLong);

    /**
     * Appends an IDispatch object argument
     */
    HRESULT Append(IDispatch * aObj);

    /**
     * Append a variant value to the args. The aVar is copied so the caller
     * should free the one passed in.
     */
    HRESULT Append(VARIANTARG & aVar);

    /**
     * Appends and out IDispatch argument
     */
    HRESULT AppendOutIDispatch(IDispatch ** aObj);

    /**
     * Clear this argument array, reinitializing it
     */
    HRESULT Clear();

    /**
     * Reads out a string argument at a given index
     */
    HRESULT GetValueAt(const size_t aIndex, std::wstring & aResult);

    /**
     * Reads out a long argument at a given index
     */
    HRESULT GetValueAt(const size_t aIndex, long & aResult);

    /**
     * Reads out a variant argument at a given index
     */
    HRESULT GetValueAt(const size_t aIndex, VARIANTARG & aResult);
    
  private:
    typedef std::vector<VARIANTARG> Variants;
    Variants mVariants;
    size_t mVariantCount;
    DISPPARAMS mDispParams;
    bool mParamsReversed;
    
    /**
     * returns the DISPARAMS object to be used by Invoke. This can be
     * called multiple times if needed, though from an IDispatch perspective
     * that may not be wise. Also after you call this method, you can not
     * append more arguments.
     */
    inline
    DISPPARAMS * GetDispParams();
  };

  /**
   * Initializes our pointer and addref's it if not null
   */
  sbIDispatchPtr(IDispatch * aPtr = 0) : mPtr(aPtr) {
    if (mPtr) {
      mPtr->AddRef();
    }
  }
  /**
   * Makes a copy of the given pointer
   */
  sbIDispatchPtr(sbIDispatchPtr const & aPtr) : mPtr(aPtr.mPtr) {
    if (mPtr) {
      mPtr->AddRef();
    }
  }
  /**
   * Releases our reference
   */
  ~sbIDispatchPtr() {
    if (mPtr) {
      mPtr->Release();
    }
  }
  /**
   * Assigns a pointer to us
   */
  sbIDispatchPtr const & operator =(sbIDispatchPtr const & aPtr) {
    // Make sure we're not assigning to ourselves
    if (&aPtr != this) {
      mPtr = aPtr.mPtr;
      if (mPtr) {
        mPtr->AddRef();
      }
    }
    return *this;
  }
  /**
   * Assigns an IDispatch pointer to us, we will addref it
   */
  sbIDispatchPtr const & operator =(IDispatch * aPtr) {
    // Do addref first to prevent release if assigning the same pointer
    if (aPtr) {
      aPtr->AddRef();
    }
    if (mPtr) {
      mPtr->Release();
    }
    mPtr = aPtr;
    return *this;
  }
  /**
   * Used for getter type functions
   */
  operator void **() {
    return reinterpret_cast<void**>(&mPtr);
  }
  /**
   * Returns the address of our internal pointer
   */
  IDispatch ** operator &() {
    return &mPtr;
  }
  /**
   * Returns our IDispatch pointer
   */
  IDispatch * get() const {
    return mPtr;
  }
  /**
   * Simple version of invoking an IDispatch method
   */
  HRESULT Invoke(wchar_t const * aMethodName, 
                 VarArgs & aVarArgs,
                 VARIANTARG & aResult,
                 WORD aFlags = DISPATCH_METHOD);
  HRESULT Invoke(wchar_t const * aMethodName) {
    VarArgs none;
    VARIANTARG empty;
    VariantInit(&empty);
    return Invoke(aMethodName, none, empty);
  }
  /**
   * Retrieves a property value
   */
  HRESULT GetProperty(wchar_t const * aPropertyName, VARIANTARG & aVar);
  /**
   * Retrieves a property that is an IDispatch value
   */
  HRESULT GetProperty(wchar_t const * aPropertyName, 
                      IDispatch ** aPtr);
  /**
   * Returns a boolean property
   */
  HRESULT GetProperty(wchar_t const * aPropertyName, bool & aBool);
  /**
   * Returns a std::wstring property.
   */
  HRESULT GetProperty(wchar_t const * aPropertyName, std::wstring & aString);
  /**
   * Returns a long property.
   */
  HRESULT GetProperty(wchar_t const * aPropertyName, long & aLong);
  /**
   * Sets a property value
   */
  HRESULT SetProperty(wchar_t const * aPropertyName, VARIANTARG & aVar);
private:
  typedef std::map<std::wstring, DISPID> MethodNames;
  
  IDispatch * mPtr;
  MethodNames mMethodNames;
  
  DISPID GetDispID(wchar_t const * aName);
};

#endif /* SBIDISPATCHPTR_H_ */
