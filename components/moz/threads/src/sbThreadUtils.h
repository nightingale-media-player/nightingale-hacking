/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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

#ifndef __SB_THREAD_UTILS_H__
#define __SB_THREAD_UTILS_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird thread utilities defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbThreadUtils.h
 * \brief Songbird Thread Utilities Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird thread utilities imported services.
//
//------------------------------------------------------------------------------

// Mozilla imports.
#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsThreadUtils.h>


//------------------------------------------------------------------------------
//
// Songbird thread utilities classes.
//
//------------------------------------------------------------------------------

/**
 *   This class provides an nsIRunnable interface, like nsRunnableMethod, that
 * invokes a provided class method from nsIRunnable::Run.
 *   This class passes a single argument to the method and collects the return
 * value.  The method return value may be read using GetReturnValue.  If any
 * error occurs attempting to invoke the method, a failure value may be
 * specified to be used as the return value.
 *
 * XXXeps need specialization for methods returning void.
 */

template <class ClassType, typename ReturnType, typename Arg1Type>
class sbRunnableMethod1 : public nsRunnable
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // SelfType                   Type for this class.
  // MethodType                 Type of method to invoke.
  //

  typedef sbRunnableMethod1<ClassType, ReturnType, Arg1Type> SelfType;
  typedef ReturnType (ClassType::*MethodType)(Arg1Type aArg1Value);


  /**
   * nsIRunnable run method.  Invoke the Songbird runnable method.
   */

  NS_IMETHOD Run()
  {
    // Do nothing if no object was provided.
    if (!mObject)
      return NS_OK;

    // Ensure lock is available.
    NS_ENSURE_TRUE(mLock, mFailureReturnValue);

    // Invoke method.
    ReturnType returnValue = (mObject->*mMethod)(mArg1Value);
    {
      nsAutoLock autoLock(mLock);
      mReturnValue = returnValue;
    }

    return NS_OK;
  }


  /**
   *   Create and initialize a Songbird runnable method for the object specified
   * by aObject.  Call the object method specified by aMethod with the argument
   * specified by aArg1Value.  Use the value specified by aFailureReturnValue as
   * the failure return value.
   *   Return the new Songbird runnable method in aRunnable.
   *
   * \param aRunnable           Returned created Songbird runnable method.
   * \param aObject             Object for which to create a Songbird runnable
   *                            method.
   * \param aMethod             Method to be invoked.
   * \param aFailureReturnValue Value to which to set runnable method return
   *                            value on internal failures.
   * \param aArg1Value          Value of first method argument.
   */

  static nsresult New(SelfType** aRunnable,
                      ClassType* aObject,
                      MethodType aMethod,
                      ReturnType aFailureReturnValue,
                      Arg1Type   aArg1Value)
  {
    // Validate arguments.
    NS_ENSURE_ARG_POINTER(aRunnable);
    NS_ENSURE_ARG_POINTER(aObject);
    NS_ENSURE_ARG_POINTER(aMethod);

    // Function variables.
    nsresult rv;

    // Create a Songbird runnable method.
    nsRefPtr<SelfType> runnable = new SelfType(aObject,
                                               aMethod,
                                               aFailureReturnValue,
                                               aArg1Value);
    NS_ENSURE_TRUE(runnable, aFailureReturnValue);

    // Initialize the Songbird runnable method.
    rv = runnable->Initialize();
    NS_ENSURE_SUCCESS(rv, rv);

    // Return results.
    runnable.forget(aRunnable);

    return NS_OK;
  }


  /**
   * Invoke the method specified by aMethod of the object specified by aObject
   * on the main thread.  Invoke the method with the argument specified by
   * aArg1Value.  Return the value returned by the invoked method.  If any error
   * occurs, return aFailureReturnValue.
   *
   * \param aObject             Object for which to invoke method.
   * \param aMethod             Method to invoke.
   * \param aFailureReturnValue Value to return on failure.
   * \param aArg1Value          Value of first method argument.
   *
   * \return                    Value returned by invoked method or
   *                            aFailureReturnValue on failure.
   */

  static ReturnType InvokeOnMainThread(ClassType* aObject,
                                       MethodType aMethod,
                                       ReturnType aFailureReturnValue,
                                       Arg1Type   aArg1Value)
  {
    nsresult rv;

    // Create a Songbird runnable method.
    nsRefPtr<SelfType> runnable;
    rv = New(getter_AddRefs(runnable),
             aObject,
             aMethod,
             aFailureReturnValue,
             aArg1Value);
    NS_ENSURE_SUCCESS(rv, aFailureReturnValue);

    // Dispatch the runnable method on the main thread.
    rv = NS_DispatchToMainThread(runnable, NS_DISPATCH_SYNC);
    NS_ENSURE_SUCCESS(rv, aFailureReturnValue);

    return runnable->GetReturnValue();
  }


  /**
   * Asynchronously, invoke the method specified by aMethod of the object
   * specified by aObject on the main thread.  Invoke the method with the
   * argument specified by aArg1Value.  Use the value specified by
   * aFailureReturnValue as the runnable method failure return value.
   *XXXeps the return value is only need to properly create the object.
   *
   * \param aObject             Object for which to invoke method.
   * \param aMethod             Method to invoke.
   * \param aFailureReturnValue Value to return on failure.
   * \param aArg1Value          Value of first method argument.
   */

  static nsresult InvokeOnMainThreadAsync(ClassType* aObject,
                                          MethodType aMethod,
                                          ReturnType aFailureReturnValue,
                                          Arg1Type   aArg1Value)
  {
    nsresult rv;

    // Create a Songbird runnable method.
    nsRefPtr<SelfType> runnable;
    rv = New(getter_AddRefs(runnable),
             aObject,
             aMethod,
             aFailureReturnValue,
             aArg1Value);
    NS_ENSURE_SUCCESS(rv, rv);

    // Dispatch the runnable method on the main thread.
    rv = NS_DispatchToMainThread(runnable, NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }


  /**
   * Get the runnable method return value.
   *
   * \return                    Runnable method return value.
   */

  ReturnType GetReturnValue()
  {
    NS_ENSURE_TRUE(mLock, mFailureReturnValue);
    nsAutoLock autoLock(mLock);
    return mReturnValue;
  }


  //----------------------------------------------------------------------------
  //
  // Protected interface.
  //
  //----------------------------------------------------------------------------

protected:

  /**
   *   Construct a Songbird runnable method for the object specified by aObject.
   * Call the object method specified by aMethod with the argument specified by
   * aArg1Value.  Use the value specified by aFailureReturnValue as the failure
   * return value.
   *
   * \param aObject             Object for which to create a Songbird runnable
   *                            method.
   * \param aMethod             Method to be invoked.
   * \param aFailureReturnValue Value to which to set runnable method return
   *                            value on internal failures.
   * \param aArg1Value          Value of first method argument.
   */

  sbRunnableMethod1(ClassType* aObject,
                    MethodType aMethod,
                    ReturnType aFailureReturnValue,
                    Arg1Type   aArg1Value) :
    mLock(nsnull),
    mObject(aObject),
    mMethod(aMethod),
    mReturnValue(aFailureReturnValue),
    mFailureReturnValue(aFailureReturnValue),
    mArg1Value(aArg1Value)
  {
  }


  /**
   * Dispose of the Songbird runnable method.
   */

  virtual ~sbRunnableMethod1()
  {
    // Dispose of the Songbird runnable method lock.
    if (mLock)
      nsAutoLock::DestroyLock(mLock);
  }


  /**
   * Initialize the Songbird runnable method.
   */

  nsresult Initialize()
  {
    // Create the runnable lock.
    mLock = nsAutoLock::NewLock("sbRunnableMethod1::mLock");
    NS_ENSURE_TRUE(mLock, NS_ERROR_OUT_OF_MEMORY);

    return NS_OK;
  }


  //
  // mLock                      Lock used to serialize field access.
  // mObject                    Object for which to invoke method.
  // mMethod                    Method to invoke.
  // mReturnValue               Method return value.
  // mFailureReturnValue        Method return value to use on failure.
  // mArg1Value                 Method argument 1 value.
  //

  PRLock*             mLock;
  nsRefPtr<ClassType> mObject;
  MethodType          mMethod;
  ReturnType          mReturnValue;
  ReturnType          mFailureReturnValue;
  Arg1Type            mArg1Value;
};


/**
 * This class is a two argument method sub-class of sbRunnableMethod1.
 */

template <class ClassType,
          typename ReturnType,
          typename Arg1Type,
          typename Arg2Type>
class sbRunnableMethod2 :
        public sbRunnableMethod1<ClassType, ReturnType, Arg1Type>
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // SelfType                   Type for this class.
  // BaseType                   Base type for this class.
  // MethodType                 Type of method to invoke.
  //

  typedef sbRunnableMethod2<ClassType, ReturnType, Arg1Type, Arg2Type> SelfType;
  typedef sbRunnableMethod1<ClassType, ReturnType, Arg1Type> BaseType;
  typedef ReturnType (ClassType::*MethodType)(Arg1Type aArg1Value,
                                              Arg2Type aArg2Value);


  /**
   * nsIRunnable run method.  Invoke the Songbird runnable method.
   */

  NS_IMETHOD Run()
  {
    // Do nothing if no object was provided.
    if (!BaseType::mObject)
      return NS_OK;

    // Invoke method.
    ReturnType
      returnValue = (BaseType::mObject->*mMethod)(BaseType::mArg1Value,
                                                  mArg2Value);
    {
      nsAutoLock autoLock(BaseType::mLock);
      BaseType::mReturnValue = returnValue;
    }

    return NS_OK;
  }


  /**
   *   Create and initialize a Songbird runnable method for the object specified
   * by aObject.  Call the object method specified by aMethod with the arguments
   * specified by aArg1Value and aArg2Value.  Use the value specified by
   * aFailureReturnValue as the failure return value.
   *   Return the new Songbird runnable method in aRunnable.
   *
   * \param aRunnable           Returned created Songbird runnable method.
   * \param aObject             Object for which to create a Songbird runnable
   *                            method.
   * \param aMethod             Method to be invoked.
   * \param aFailureReturnValue Value to which to set runnable method return
   *                            value on internal failures.
   * \param aArg1Value          Value of first method argument.
   * \param aArg2Value          Value of second method argument.
   */

  static nsresult New(SelfType** aRunnable,
                      ClassType* aObject,
                      MethodType aMethod,
                      ReturnType aFailureReturnValue,
                      Arg1Type   aArg1Value,
                      Arg2Type   aArg2Value)
  {
    // Validate arguments.
    NS_ENSURE_ARG_POINTER(aRunnable);
    NS_ENSURE_ARG_POINTER(aObject);
    NS_ENSURE_ARG_POINTER(aMethod);

    // Function variables.
    nsresult rv;

    // Create a Songbird runnable method.
    nsRefPtr<SelfType> runnable = new SelfType(aObject,
                                               aMethod,
                                               aFailureReturnValue,
                                               aArg1Value,
                                               aArg2Value);
    NS_ENSURE_TRUE(runnable, aFailureReturnValue);

    // Initialize the Songbird runnable method.
    rv = runnable->Initialize();
    NS_ENSURE_SUCCESS(rv, rv);

    // Return results.
    runnable.forget(aRunnable);

    return NS_OK;
  }


  /**
   * Invoke the method specified by aMethod of the object specified by aObject
   * on the main thread.  Invoke the method with the arguments specified by
   * aArg1Value and aArg2Value.  Return the value returned by the invoked
   * method.  If any error occurs, return aFailureReturnValue.
   *
   * \param aObject             Object for which to invoke method.
   * \param aMethod             Method to invoke.
   * \param aFailureReturnValue Value to return on failure.
   * \param aArg1Value          Value of first method argument.
   * \param aArg2Value          Value of second method argument.
   *
   * \return                    Value returned by invoked method or
   *                            aFailureReturnValue on failure.
   */

  static ReturnType InvokeOnMainThread(ClassType* aObject,
                                       MethodType aMethod,
                                       ReturnType aFailureReturnValue,
                                       Arg1Type   aArg1Value,
                                       Arg2Type   aArg2Value)
  {
    nsresult rv;

    // Create a Songbird runnable method.
    nsRefPtr<SelfType> runnable;
    rv = New(getter_AddRefs(runnable),
             aObject,
             aMethod,
             aFailureReturnValue,
             aArg1Value,
             aArg2Value);
    NS_ENSURE_SUCCESS(rv, aFailureReturnValue);

    // Dispatch the runnable method on the main thread.
    rv = NS_DispatchToMainThread(runnable, NS_DISPATCH_SYNC);
    NS_ENSURE_SUCCESS(rv, aFailureReturnValue);

    return runnable->GetReturnValue();
  }


  /**
   * Asynchronously, invoke the method specified by aMethod of the object
   * specified by aObject on the main thread.  Invoke the method with the
   * arguments specified by aArg1Value and aArg2Value.  Use the value specified
   * by aFailureReturnValue as the runnable method failure return value.
   *XXXeps the return value is only need to properly create the object.
   *
   * \param aObject             Object for which to invoke method.
   * \param aMethod             Method to invoke.
   * \param aFailureReturnValue Value to return on failure.
   * \param aArg1Value          Value of first method argument.
   * \param aArg2Value          Value of second method argument.
   */

  static nsresult InvokeOnMainThreadAsync(ClassType* aObject,
                                          MethodType aMethod,
                                          ReturnType aFailureReturnValue,
                                          Arg1Type   aArg1Value,
                                          Arg2Type   aArg2Value)
  {
    nsresult rv;

    // Create a Songbird runnable method.
    nsRefPtr<SelfType> runnable;
    rv = New(getter_AddRefs(runnable),
             aObject,
             aMethod,
             aFailureReturnValue,
             aArg1Value,
             aArg2Value);
    NS_ENSURE_SUCCESS(rv, rv);

    // Dispatch the runnable method on the main thread.
    rv = NS_DispatchToMainThread(runnable, NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }


  //----------------------------------------------------------------------------
  //
  // Protected interface.
  //
  //----------------------------------------------------------------------------

protected:

  /**
   *   Construct a Songbird runnable method for the object specified by aObject.
   * Call the object method specified by aMethod with the argument specified by
   * aArg1Value.  Use the value specified by aFailureReturnValue as the failure
   * return value.
   *
   * \param aObject             Object for which to create a Songbird runnable
   *                            method.
   * \param aMethod             Method to be invoked.
   * \param aFailureReturnValue Value to which to set runnable method return
   *                            value on internal failures.
   * \param aArg1Value          Value of first method argument.
   */

  sbRunnableMethod2(ClassType* aObject,
                    MethodType aMethod,
                    ReturnType aFailureReturnValue,
                    Arg1Type   aArg1Value,
                    Arg2Type   aArg2Value) :
    BaseType(aObject, nsnull, aFailureReturnValue, aArg1Value),
    mMethod(aMethod),
    mArg2Value(aArg2Value)
  {
  }


  /**
   * Dispose of the Songbird runnable method.
   */

  virtual ~sbRunnableMethod2()
  {
  }


  //
  // mMethod                    Method to invoke.
  // mArg2Value                 Method argument 2 value.
  //

  MethodType mMethod;
  Arg2Type   mArg2Value;
};


//------------------------------------------------------------------------------
//
// Songbird thread utilities macros.
//
//------------------------------------------------------------------------------

/**
 * From the main thread, invoke the method specified by aMethod on the object
 * specified by aObject of the type specified by aClassType.  Return the
 * method's return value of type aReturnType.  On any error, return the value
 * specified by aFailureReturnValue.  Pass to the method the argument value
 * specified by aArg1Value of type specified by aArg1Type.
 *
 * \param aClassType            Type of method class.
 * \param aObject               Object for which to invoke method.
 * \param aMethod               Method to invoke.
 * \param aReturnType           Type of method return value.
 * \param aFailureReturnValue   Value to return on failure.
 * \param aArg1Type             Type of first method argument.
 * \param aArg1Value            Value of first method argument.
 *
 * \return                      Value returned by invoked method or
 *                              aFailureReturnValue on failure.
 */

#define SB_INVOKE_ON_MAIN_THREAD1(aClassType,                                  \
                                  aObject,                                     \
                                  aMethod,                                     \
                                  aReturnType,                                 \
                                  aFailureReturnValue,                         \
                                  aArg1Type,                                   \
                                  aArg1Value)                                  \
  sbRunnableMethod1<aClassType, aReturnType, aArg1Type>                        \
    ::InvokeOnMainThread(aObject,                                              \
                         &aClassType::aMethod,                                 \
                         aFailureReturnValue,                                  \
                         aArg1Value)

/**
 * From the main thread, invoke the method specified by aMethod on the object
 * specified by aObject.  Return the method's return value. On any error,
 * return the value specified by aFailureReturnValue. Pass to the method the
 * argument value specified by aArg1.
 *
 * \param aObject               Object for which to invoke method.
 * \param aMethod               Method to invoke.
 * \param aFailureReturnValue   Value to return on failure.
 * \param aArg1                 Value of first method argument.
 *
 * \return                      Value returned by invoked method or
 *                              aFailureReturnValue on failure.
 */
template <class T, class MT, class RT, class A1>
inline
RT sbInvokeOnMainThread1(T & aObject,
                        MT aMethod,
                        RT aFailureReturnValue,
                        A1 aArg1)
{
  return sbRunnableMethod1<T, RT, A1>::InvokeOnMainThread(&aObject,
                                                          aMethod,
                                                          aFailureReturnValue,
                                                          aArg1);
}

/**
 * From the main thread, invoke asynchronously the method specified by aMethod
 * on the object specified by aObject. On any error, return the value specified
 * by aFailureReturnValue. Pass to the method the argument value specified
 * by aArg1.
 *
 * \param aObject               Object for which to invoke method.
 * \param aMethod               Method to invoke.
 * \param aFailureReturnValue   Value to return on failure.
 * \param aArg1                 Value of first method argument.
 *
 * \return                      aFailureReturnValue on failure to invoke
 */
template <class T, class MT, class RT, class A1>
inline
RT sbInvokeOnMainThread1Async(T & aObject,
                              MT aMethod,
                              RT aFailureReturnValue,
                              A1 aArg1)
{
  return sbRunnableMethod1<T, RT, A1>::InvokeOnMainThreadAsync(
                                                            &aObject,
                                                            aMethod,
                                                            aFailureReturnValue,
                                                            aArg1);
}

#define SB_INVOKE_ON_MAIN_THREAD2(aClassType,                                  \
                                  aObject,                                     \
                                  aMethod,                                     \
                                  aReturnType,                                 \
                                  aFailureReturnValue,                         \
                                  aArg1Type,                                   \
                                  aArg1Value,                                  \
                                  aArg2Type,                                   \
                                  aArg2Value)                                  \
  sbRunnableMethod2<aClassType, aReturnType, aArg1Type, aArg2Type>             \
    ::InvokeOnMainThread(aObject,                                              \
                         &aClassType::aMethod,                                 \
                         aFailureReturnValue,                                  \
                         aArg1Value,                                           \
                         aArg2Value)

/**
 * From the main thread, invoke the method specified by aMethod on the object
 * specified by aObject.  Return the method's return value. On any error,
 * return the value specified by aFailureReturnValue. Pass to the method the
 * argument value specified by aArg1.
 *
 * \param aObject               Object for which to invoke method.
 * \param aMethod               Method to invoke.
 * \param aFailureReturnValue   Value to return on failure.
 * \param aArg1                 Value of the first argument.
 * \param aArg2                 Value of the second argument
 *
 * \return                      aFailureReturnValue on failure.
 */
template <class T, class MT, class RT, class A1, class A2>
inline
RT sbInvokeOnMainThread2(T & aObject,
                         MT aMethod,
                         RT aFailureReturnValue,
                         A1 aArg1,
                         A2 aArg2)
{
  return sbRunnableMethod2<T, RT, A1, A2>::InvokeOnMainThread(
                                                            &aObject,
                                                            aMethod,
                                                            aFailureReturnValue,
                                                            aArg1,
                                                            aArg2);
}

/**
 * From the main thread, invoke asynchronously the method specified by aMethod
 * on the object specified by aObject.  Return the method's return value.
 * On any error, return the value specified by aFailureReturnValue. Pass to
 * the method the argument value specified by aArg1.
 *
 * \param aObject               Object for which to invoke method.
 * \param aMethod               Method to invoke.
 * \param aFailureReturnValue   Value to return on failure.
 * \param aArg1                 Value of the first argument.
 * \param aArg2                 Value of the second argument
 *
 * \return                      Value returned by invoked method or
 *                              aFailureReturnValue on failure.
 */
template <class T, class MT, class RT, class A1, class A2>
inline
RT sbInvokeOnMainThread2Async(T & aObject,
                              MT aMethod,
                              RT aFailureReturnValue,
                              A1 aArg1,
                              A2 aArg2)
{
  return sbRunnableMethod2<T, RT, A1, A2>::InvokeOnMainThreadAsync(
                                                            &aObject,
                                                            aMethod,
                                                            aFailureReturnValue,
                                                            aArg1,
                                                            aArg2);
}

/**
 * From the main thread, asynchronously invoke the method specified by aMethod
 * on the object specified by aObject of the type specified by aClassType.  Pass
 * to the method the argument value specified by aArg1Value of type specified by
 * aArg1Type.  Use the value specified by aFailureReturnValue as the runnable
 * method failure return value.
 *XXXeps the return value is only need to properly create the object.
 *
 * \param aClassType            Type of method class.
 * \param aObject               Object for which to invoke method.
 * \param aMethod               Method to invoke.
 * \param aReturnType           Type of method return value.
 * \param aFailureReturnValue   Value to return on failure.
 * \param aArg1Type             Type of first method argument.
 * \param aArg1Value            Value of first method argument.
 *
 * \return                      Value returned by invoked method or
 *                              aFailureReturnValue on failure.
 */

#define SB_INVOKE_ON_MAIN_THREAD_ASYNC1(aClassType,                            \
                                        aObject,                               \
                                        aMethod,                               \
                                        aReturnType,                           \
                                        aFailureReturnValue,                   \
                                        aArg1Type,                             \
                                        aArg1Value)                            \
  sbRunnableMethod1<aClassType, aReturnType, aArg1Type>                        \
    ::InvokeOnMainThreadAsync(aObject,                                         \
                              &aClassType::aMethod,                            \
                              aFailureReturnValue,                             \
                              aArg1Value)

#define SB_INVOKE_ON_MAIN_THREAD_ASYNC2(aClassType,                            \
                                        aObject,                               \
                                        aMethod,                               \
                                        aReturnType,                           \
                                        aFailureReturnValue,                   \
                                        aArg1Type,                             \
                                        aArg1Value,                            \
                                        aArg2Type,                             \
                                        aArg2Value)                            \
  sbRunnableMethod2<aClassType, aReturnType, aArg1Type, aArg2Type>             \
    ::InvokeOnMainThreadAsync(aObject,                                         \
                              &aClassType::aMethod,                            \
                              aFailureReturnValue,                             \
                              aArg1Value,                                      \
                              aArg2Value)


//------------------------------------------------------------------------------
//
// Songbird thread utilities service prototypes.
//
//------------------------------------------------------------------------------

/**
 * Check if the current thread is the main thread and return true if so.  Use
 * the thread manager object specified by aThreadManager if provided.  This
 * function can be used during XPCOM shutdown if aThreadManager is provided.
 *
 * \param aThreadManager        Optional thread manager.  Defaults to null.
 *
 * \return PR_TRUE              Current thread is main thread.
 */
PRBool SB_IsMainThread(nsIThreadManager* aThreadManager = nsnull);


#endif // __SB_THREAD_UTILS_H__

