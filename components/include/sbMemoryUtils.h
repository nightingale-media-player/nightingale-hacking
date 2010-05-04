/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#ifndef __SBMEMORYUTILS_H__
#define __SBMEMORYUTILS_H__

#include <nsMemory.h>
#include <stdlib.h>

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

/**
 * Auto class that automatically deallocates an external XPCOM
 * array
 */ 
template<class T>
class sbAutoFreeXPCOMArrayByRef
{
public:
  sbAutoFreeXPCOMArrayByRef(PRUint32 & aCount, T & aArray)
  : mCount(aCount),
    mArray(aArray)
  {
    if (aCount) {
      NS_ASSERTION(aArray, "Null pointer!");
    }
  }

  ~sbAutoFreeXPCOMArrayByRef()
  {
    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(mCount, mArray);
  }

private:
  PRUint32 & mCount;
  T & mArray;
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
                                                                               \
  aType get() const                                                            \
  {                                                                            \
    return mValue;                                                             \
  }                                                                            \
                                                                               \
  operator aType() const                                                       \
  {                                                                            \
    return get();                                                              \
  }                                                                            \
                                                                               \
  aType& operator=(aType aValue)                                               \
  {                                                                            \
    Clear();                                                                   \
    Set(aValue);                                                               \
    return mValue;                                                             \
  }                                                                            \
                                                                               \
  operator bool()                                                              \
  {                                                                            \
    if (!(aIsValid))                                                           \
      return PR_FALSE;                                                         \
    return PR_TRUE;                                                            \
  }                                                                            \
                                                                               \
  void Clear()                                                                 \
  {                                                                            \
    if (aIsValid) {                                                            \
      aDispose;                                                                \
      aInvalidate;                                                             \
    }                                                                          \
  }                                                                            \
                                                                               \
  aType* StartAssignment()                                                     \
  {                                                                            \
    Clear();                                                                   \
    return reinterpret_cast<aType*>(&mValue);                                  \
  }                                                                            \
                                                                               \
private:                                                                       \
  aType mValue;                                                                \
  aType2 mValue2;                                                              \
                                                                               \
  void Invalidate() { aInvalidate; }                                           \
  void operator=(const aName aValue) {}                                        \
}

#define SB_AUTO_CLASS(aName, aType, aIsValid, aDispose, aInvalidate)           \
          SB_AUTO_CLASS2(aName, aType, char, aIsValid, aDispose, aInvalidate)


/**
 * Define an auto-disposal class wrapper for data types whose invalid value is
 * NULL.
 *
 * \param aName                 Name of class.
 * \param aType                 Class data type.
 * \param aDispose              Code snippet to dispose of the data.
 */

#define SB_AUTO_NULL_CLASS(aName, aType, aDispose) \
          SB_AUTO_CLASS(aName, aType, mValue != NULL, aDispose, mValue = NULL)


//
// Auto-disposal class wrappers.
//
//   sbAutoNSMemPtr             Wrapper to auto-dispose memory blocks allocated
//                              with NS_Alloc.
//   sbAutoMemPtr               Typed wrapper to auto-dispose memory blocks
//                              allocated with malloc.
//   sbAutoNSTypePtr            Typed version of sbAutoNSMemPtr
//   sbAutoNSArray              Wrapper to auto-dispose a simple array and all
//                              of its members allocated with NS_Alloc.
//                              Example: sbAutoNSArray<char*>
//                                         autoArray(array, arrayLength);
//

SB_AUTO_CLASS(sbAutoNSMemPtr, void*, !!mValue, NS_Free(mValue), mValue = nsnull);
template<typename T> SB_AUTO_NULL_CLASS(sbAutoMemPtr, T*, free(mValue));

template<typename T>
SB_AUTO_CLASS(sbAutoNSTypePtr, T*, !!mValue, NS_Free(mValue), mValue = nsnull);

template<typename T>
SB_AUTO_CLASS2(sbAutoNSArray,
               T*,
               PRUint32,
               !!mValue,
               {
                 for (PRUint32 i = 0; i < mValue2; ++i) {
                   if (mValue[i])
                     NS_Free(mValue[i]);
                 }
                 NS_Free(mValue);
               },
               mValue = nsnull);


template <class COMPtr, class ReturnType>
inline
nsresult sbReturnCOMPtr(COMPtr & aPtr, ReturnType ** aReturn)
{
  if (!aReturn) {
    return NS_ERROR_INVALID_POINTER;
  }

  *aReturn = aPtr.get();
  NS_IF_ADDREF(*aReturn);

  return NS_OK;
}

#endif /* __SBMEMORYUTILS_H__ */

