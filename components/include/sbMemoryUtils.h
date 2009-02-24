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

#ifndef __SBMEMORYUTILS_H__
#define __SBMEMORYUTILS_H__

#include <nsMemory.h>

template<class T>
class sbAutoFreeXPCOMPointerArray
{
public:
  sbAutoFreeXPCOMPointerArray(PRUint32 aCount, T** aArray) :
    mCount(aCount),
    mArray(aArray)
  {}

  ~sbAutoFreeXPCOMPointerArray()
  {
    NS_FREE_XPCOM_ISUPPORTS_POINTER_ARRAY(mCount, mArray);
  }

private:
  PRUint32 mCount;
  T** mArray;
};

template <class T>
class sbAutoFreeXPCOMArray
{
public:
  sbAutoFreeXPCOMArray(PRUint32 aCount, T aArray)
  : mCount(aCount),
    mArray(aArray)
  {
    if (aCount) {
      NS_ASSERTION(aArray, "Null pointer!");
    }
  }

  ~sbAutoFreeXPCOMArray()
  {
    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(mCount, mArray);
  }

private:
  PRUint32 mCount;
  T mArray;
};


/**
 * Define a class that wraps a data type and auto-disposes it when going out of
 * scope.  The class name is specified by aName, and the data type is specified
 * by aType.  An additional class field type may be specified by aType2.  A code
 * snippet that checks for valid data is specified by aIsValid.  A code snippet
 * that disposes of the data is specified by aDispose.  A code snippet that
 * invalidates the data is specified by aInvalidate.  The code snippets may use
 * mValue to refer to the wrapped data and mValue2 to refer to the additional
 * class field.
 *
 * \param aName                 Name of class.
 * \param aType                 Class data type.
 * \param aType2                Additional class field data type.
 * \param aIsValid              Code snippet to test for data validity.
 * \param aDispose              Code snippet to dispose of the data.
 * \param aInvalidate           Code snippet to invalidate the data.
 *
 * Example:
 *   SB_AUTO_CLASS(sbAutoMemPtr,
 *                 void*,
 *                 !!mValue,
 *                 NS_Free(mValue),
 *                 mValue = nsnull)
 *   sbAutoMemPtr autoMem(memPtr);
 *
 *   SB_AUTO_CLASS2(sbAutoClassData,
 *                  void*,
 *                  classType*,
 *                  !!mValue,
 *                  mValue2->Delete(mValue),
 *                  mValue = nsnull)
 *   sbAutoClassData(data, this);
 */

#define SB_AUTO_CLASS2(aName, aType, aType2, aIsValid, aDispose, aInvalidate)  \
class aName                                                                    \
{                                                                              \
public:                                                                        \
  aName() { Invalidate(); }                                                    \
                                                                               \
  aName(aType aValue) : mValue(aValue) {}                                      \
                                                                               \
  aName(aType aValue, aType2 aValue2) : mValue(aValue), mValue2(aValue2) {}    \
                                                                               \
  virtual ~aName()                                                             \
  {                                                                            \
    if (aIsValid) {                                                            \
      aDispose;                                                                \
    }                                                                          \
  }                                                                            \
                                                                               \
  void Set(aType aValue) { mValue = aValue; }                                  \
                                                                               \
  void Set(aType aValue, aType2 aValue2)                                       \
         { mValue = aValue; mValue2 = aValue2; }                               \
                                                                               \
  aType forget()                                                               \
  {                                                                            \
    aType value = mValue;                                                      \
    Invalidate();                                                              \
    return value;                                                              \
  }                                                                            \
  aType get()                                                                  \
  {                                                                            \
    return mValue;                                                             \
  }                                                                            \
  aType& operator=(aType aValue)                                               \
  {                                                                            \
    if (aIsValid) {                                                            \
      aDispose;                                                                \
      aInvalidate;                                                             \
    }                                                                          \
    Set(aValue);                                                               \
    return mValue;                                                             \
  }                                                                            \
  operator bool()                                                              \
  {                                                                            \
    return (aIsValid);                                                         \
  }                                                                            \
private:                                                                       \
  aType mValue;                                                                \
  aType2 mValue2;                                                              \
                                                                               \
  void Invalidate() { aInvalidate; }                                           \
  aName& operator=(const aName aValue) { /* compile error, didn't addref */ }  \
}

#define SB_AUTO_CLASS(aName, aType, aIsValid, aDispose, aInvalidate)           \
          SB_AUTO_CLASS2(aName, aType, char, aIsValid, aDispose, aInvalidate)


//
// Auto-disposal class wrappers.
//
//   sbAutoNSMemPtr             Wrapper to auto-dispose memory blocks allocated
//                              with NS_Alloc.
//

SB_AUTO_CLASS(sbAutoNSMemPtr, void*, !!mValue, NS_Free(mValue), mValue = nsnull);


#endif /* __SBMEMORYUTILS_H__ */
