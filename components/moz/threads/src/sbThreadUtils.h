/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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
#include <mozilla/ReentrantMonitor.h>
#include <nsAutoPtr.h>
#include <nsIThreadPool.h>
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


    // Invoke method.
    ReturnType returnValue = (mObject->*mMethod)(mArg1Value);
    {
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
    NS_ENSURE_SUCCESS(rv, rv);

    return runnable->GetReturnValue();
  }

  /**
   * Invoke the method specified by aMethod of the object specified by aObject
   * on the supplied thread.  Invoke the method with the argument specified by
   * aArg1Value.  Return the value returned by the invoked method.  If any error
   * occurs, return aFailureReturnValue.
   *
   * \param aObject             Object for which to invoke method.
   * \param aMethod             Method to invoke.
   * \param aFailureReturnValue Value to return on failure.
   * \param aArg1Value          Value of first method argument.
   * \param aThread             Thread to run on.
   *
   * \return                    Value returned by invoked method or
   *                            aFailureReturnValue on failure.
   */

  static ReturnType InvokeOnThread(ClassType* aObject,
                                   MethodType aMethod,
                                   ReturnType aFailureReturnValue,
                                   Arg1Type   aArg1Value,
                                   nsIEventTarget* aThread)
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

    // Dispatch the runnable method on the thread.
    rv = aThread->Dispatch(runnable, NS_DISPATCH_SYNC);
    NS_ENSURE_SUCCESS(rv, aFailureReturnValue);

    return runnable->GetReturnValue();
  }


  /**
   * Asynchronously, invoke the method specified by aMethod of the object
   * specified by aObject on the main thread.  Invoke the method with the
   * argument specified by aArg1Value.  Use the value specified by
   * aFailureReturnValue as the runnable method failure return value.
   *XXXeps the return value is only needed to properly create the object.
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
    NS_ENSURE_SUCCESS(rv, aFailureReturnValue);

    // Dispatch the runnable method on the main thread.
    rv = NS_DispatchToMainThread(runnable, NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  /**
   * Asynchronously, invoke the method specified by aMethod of the object
   * specified by aObject on the supplied thread.  Invoke the method with the
   * argument specified by aArg1Value.  Use the value specified by
   * aFailureReturnValue as the runnable method failure return value.
   *XXXeps the return value is only needed to properly create the object.
   *
   * \param aObject             Object for which to invoke method.
   * \param aMethod             Method to invoke.
   * \param aFailureReturnValue Value to return on failure.
   * \param aArg1Value          Value of first method argument.
   * \param aThread             Thread to run on.
   */

  static nsresult InvokeOnThreadAsync(ClassType* aObject,
                                      MethodType aMethod,
                                      ReturnType aFailureReturnValue,
                                      Arg1Type   aArg1Value,
                                      nsIEventTarget* aThread)
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

    // Dispatch the runnable method on the thread.
    rv = aThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, aFailureReturnValue);

    return NS_OK;
  }

  /**
   * Get the runnable method return value.
   *
   * \return                    Runnable method return value.
   */

  ReturnType GetReturnValue()
  {
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
  }


  /**
   * Initialize the Songbird runnable method.
   */

  nsresult Initialize()
  {

    return NS_OK;
  }


  //
  // mObject                    Object for which to invoke method.
  // mMethod                    Method to invoke.
  // mReturnValue               Method return value.
  // mFailureReturnValue        Method return value to use on failure.
  // mArg1Value                 Method argument 1 value.
  //

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
    NS_ENSURE_SUCCESS(rv, rv);

    return runnable->GetReturnValue();
  }

  /**
   * Invoke the method specified by aMethod of the object specified by aObject
   * on the supplied thread.  Invoke the method with the arguments specified by
   * aArg1Value and aArg2Value.  Return the value returned by the invoked
   * method.  If any error occurs, return aFailureReturnValue.
   *
   * \param aObject             Object for which to invoke method.
   * \param aMethod             Method to invoke.
   * \param aFailureReturnValue Value to return on failure.
   * \param aArg1Value          Value of first method argument.
   * \param aArg2Value          Value of second method argument.
   * \param aThread             Thread to run on.
   *
   * \return                    Value returned by invoked method or
   *                            aFailureReturnValue on failure.
   */

  static ReturnType InvokeOnThread(ClassType* aObject,
                                   MethodType aMethod,
                                   ReturnType aFailureReturnValue,
                                   Arg1Type   aArg1Value,
                                   Arg2Type   aArg2Value,
                                   nsIEventTarget* aThread)
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
    rv = aThread->Dispatch(runnable, NS_DISPATCH_SYNC);
    NS_ENSURE_SUCCESS(rv, aFailureReturnValue);

    return runnable->GetReturnValue();
  }

  /**
   * Asynchronously, invoke the method specified by aMethod of the object
   * specified by aObject on the main thread.  Invoke the method with the
   * arguments specified by aArg1Value and aArg2Value.  Use the value specified
   * by aFailureReturnValue as the runnable method failure return value.
   *XXXeps the return value is only needed to properly create the object.
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
    NS_ENSURE_SUCCESS(rv, aFailureReturnValue);

    // Dispatch the runnable method on the main thread.
    rv = NS_DispatchToMainThread(runnable, NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  /**
   * Asynchronously, invoke the method specified by aMethod of the object
   * specified by aObject on the main thread.  Invoke the method with the
   * arguments specified by aArg1Value and aArg2Value.  Use the value specified
   * by aFailureReturnValue as the runnable method failure return value.
   *XXXeps the return value is only needed to properly create the object.
   *
   * \param aObject             Object for which to invoke method.
   * \param aMethod             Method to invoke.
   * \param aFailureReturnValue Value to return on failure.
   * \param aArg1Value          Value of first method argument.
   * \param aArg2Value          Value of second method argument.
   * \param aThread             Thread to run on.
   */

  static nsresult InvokeOnThreadAsync(ClassType* aObject,
                                      MethodType aMethod,
                                      ReturnType aFailureReturnValue,
                                      Arg1Type   aArg1Value,
                                      Arg2Type   aArg2Value,
                                      nsIEventTarget* aThread)
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
    rv = aThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, aFailureReturnValue);

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



/**
 * A subclass of nsRunnable that can be waited on when synchronous
 * operation is needed.
 */
class sbRunnable : public nsRunnable
{
public:
  sbRunnable(
    const char *  aName) :
    mMonitor(aName ? aName : "sbRunnable"),
    mDone(false)
    {}


  /**
   * nsIRunnable run method.  Marks the operation as complete
   * and signals all waiters.  Subclasses must override this
   * function to perform any real work, and then delegate to
   * this function when done.
   */
  NS_IMETHOD Run();

  /**
   * Returns true if Run() completes before the timeout lapses,
   * or false otherwise.
   */
  PRBool Wait(PRIntervalTime aTimeout);

private:
  mozilla::ReentrantMonitor  mMonitor;
  PRBool            mDone;
};



/**
 * A subclass template of sbRunnable that can return a result.
 */
template <typename ResultType>
class sbRunnable_ : public sbRunnable
{
public:
  using sbRunnable::Wait;

public:
  sbRunnable_(
    const char *  aName) :
    sbRunnable(aName)
    {}


  /**
   * nsIRunnable run method.  Delegates to OnRun().  Subclasses
   * must override OnRun(), not this function.
   */
  NS_IMETHOD Run()
  {
    // Invoke method.
    mResult = OnRun();
    return sbRunnable::Run();
  }


  /**
   * Waits indefinitely for OnRun() to complete and returns its result.
   */
  ResultType
  Wait()
  {
    Wait(PR_INTERVAL_NO_TIMEOUT);
    return mResult;
  }


  /**
   * Returns true if OnRun() completes before the timeout lapses,
   * or false otherwise.  If true, returns the result of OnRun()
   * in aResult.
   */
  PRBool
  Wait(PRIntervalTime aTimeout, ResultType & aResult)
  {
    if (!Wait(aTimeout)) {
      return PR_FALSE;
    }

    aResult = mResult;
    return PR_TRUE;
  }


protected:
  /**
   * The operation to perform when invoked, as defined by a subclass.
   */
  virtual ResultType OnRun() = 0;

private:
  ResultType  mResult;
};



/**
 * A specialization of sbRunnable_<> for operations that return void.
 */
template <>
class sbRunnable_<void> : public sbRunnable
{
public:
  sbRunnable_(
    const char *  aName) :
    sbRunnable(aName)
    {}


  /**
   * nsIRunnable run method.  Delegates to OnRun().  Subclasses
   * must override OnRun(), not this function.
   */
  NS_IMETHOD Run()
  {
    // Invoke method.
    OnRun();
    return sbRunnable::Run();
  }


protected:
  /**
   * The operation to perform when invoked, as defined by a subclass.
   */
  virtual void OnRun() = 0;
};



/**
 * A subclass template of sbRunnable_<> that captures an object and
 * a method to call when Run() is invoked.  A subclass must invoke the
 * method, because this class doesn't know what arguments to pass,
 * if any.
 */
template <typename ResultType,
          typename TargetType,
          typename MethodType>
class sbRunnableMethod_ : public sbRunnable_<ResultType>
{
public:
  sbRunnableMethod_(
    TargetType &  aTarget,
    MethodType    aMethod,
    const char *  aName) :
    sbRunnable_<ResultType>(aName),
    mTarget(&aTarget),
    mMethod(aMethod)
    {}


protected:
  /**
   * The operation to perform when invoked.  Subclasses should call
   * mMethod on mTarget with the proper arguments and return the result.
   */
  virtual ResultType OnRun() = 0;

  nsRefPtr<TargetType>  mTarget;
  MethodType            mMethod;
};


/**
 * A subclass template of sbRunnableMethod_<> for methods that do not take
 * argument.
 */

template <typename ResultType,
          typename TargetType,
          typename MethodType = ResultType (TargetType::*) ()>
class sbRunnableMethod0_ :
  public sbRunnableMethod_<ResultType, TargetType, MethodType>
{
public:
  typedef sbRunnableMethod_<ResultType, TargetType, MethodType> BaseType;


  /**
   * Capture the object and method to call and the argument to pass
   */
  sbRunnableMethod0_(
    TargetType &  aTarget,
    MethodType    aMethod,
    const char *  aName = NULL) :
    BaseType(aTarget, aMethod, aName)
    {}


protected:
  /**
   * Invokes the captured method on the captured object with the
   * captured argument and returns its result.  A subclass that
   * overrides this function should eventually delegate to it.
   */
  virtual ResultType OnRun()
  {
    return ((*BaseType::mTarget).*BaseType::mMethod)();
  }
};


/**
 * A subclass template of sbRunnableMethod_<> for methods that take one
 * argument.  Use the optional Arg1Type to specify the type used to curry
 * the argument if it should differ from Param1Type.
 *
 * For example, if Param1Type is nsISupports *, Arg1Type can be set to
 * nsRefPtr<nsISupports> to ensure that the sbRunnableMethod1_<> retains
 * a reference to the argument for the life of the sbRunnableMethod1_<>.
 */
template <typename ResultType,
          typename TargetType,
          typename Param1Type,
          typename Arg1Type = Param1Type,
          typename MethodType = ResultType (TargetType::*) (Param1Type)>
class sbRunnableMethod1_ :
  public sbRunnableMethod_<ResultType, TargetType, MethodType>
{
public:
  typedef sbRunnableMethod_<ResultType, TargetType, MethodType> BaseType;


  /**
   * Capture the object and method to call and the argument to pass
   */
  sbRunnableMethod1_(
    TargetType &  aTarget,
    MethodType    aMethod,
    Param1Type    aArg1,
    const char *  aName = NULL) :
    BaseType(aTarget, aMethod, aName),
    mArg1(aArg1)
    {}


protected:
  /**
   * Invokes the captured method on the captured object with the
   * captured argument and returns its result.  A subclass that
   * overrides this function should eventually delegate to it.
   */
  virtual ResultType OnRun()
  {
    return ((*BaseType::mTarget).*BaseType::mMethod)(mArg1);
  }


private:
  Arg1Type  mArg1;
};



/**
 * A subclass template of sbRunnableMethod_<> for methods that take two
 * arguments.  Use the optional Arg*Types to specify the types used to
 * curry arguments if any should differ from the corresponding Param*Type.
 *
 * For example, if Param2Type is nsISupports *, Arg2Type can be set to
 * nsRefPtr<nsISupports> to ensure that the runnable retains a reference to
 * the argument for the life of the runnable.
 */
template <typename ResultType,
          typename TargetType,
          typename Param1Type,
          typename Param2Type,
          typename Arg1Type = Param1Type,
          typename Arg2Type = Param2Type,
          typename MethodType = ResultType (TargetType::*) (Param1Type,
                                                            Param2Type)>
class sbRunnableMethod2_ :
  public sbRunnableMethod_<ResultType, TargetType, MethodType>
{
public:
  typedef sbRunnableMethod_<ResultType, TargetType, MethodType> BaseType;


  /**
   * Capture the object and method to call and the arguments to pass
   */
  sbRunnableMethod2_(
    TargetType &  aTarget,
    MethodType    aMethod,
    Param1Type    aArg1,
    Param2Type    aArg2,
    const char *  aName = NULL) :
    BaseType(aTarget, aMethod, aName),
    mArg1(aArg1),
    mArg2(aArg2)
    {}


protected:
  /**
   * Invokes the captured method on the captured object with the
   * captured arguments and returns its result.  A subclass that
   * overrides this function should eventually delegate to it.
   */
  virtual ResultType OnRun()
  {
    return ((*BaseType::mTarget).*BaseType::mMethod)(mArg1, mArg2);
  }


private:
  Arg1Type  mArg1;
  Arg2Type  mArg2;
};



/**
 * A subclass template of sbRunnableMethod_<> for methods that take three
 * arguments.  Use the optional Arg*Types to specify the types used to
 * curry arguments if any should differ from the corresponding Param*Type.
 *
 * For example, if Param2Type is nsISupports *, Arg2Type can be set to
 * nsRefPtr<nsISupports> to ensure that the runnable retains a reference to
 * the argument for the life of the runnable.
 */
template <typename ResultType,
          typename TargetType,
          typename Param1Type,
          typename Param2Type,
          typename Param3Type,
          typename Arg1Type = Param1Type,
          typename Arg2Type = Param2Type,
          typename Arg3Type = Param3Type,
          typename MethodType = ResultType (TargetType::*) (Param1Type,
                                                            Param2Type,
                                                            Param3Type)>
class sbRunnableMethod3_ :
  public sbRunnableMethod_<ResultType, TargetType, MethodType>
{
public:
  typedef sbRunnableMethod_<ResultType, TargetType, MethodType> BaseType;


  /**
   * Capture the object and method to call and the arguments to pass
   */
  sbRunnableMethod3_(
    TargetType &  aTarget,
    MethodType    aMethod,
    Param1Type    aArg1,
    Param2Type    aArg2,
    Param3Type    aArg3,
    const char *  aName = NULL) :
    BaseType(aTarget, aMethod, aName),
    mArg1(aArg1),
    mArg2(aArg2),
    mArg3(aArg3)
    {}


protected:
  /**
   * Invokes the captured method on the captured object with the
   * captured arguments and returns its result.  A subclass that
   * overrides this function should eventually delegate to it.
   */
  virtual ResultType OnRun()
  {
    return ((*BaseType::mTarget).*BaseType::mMethod)(mArg1, mArg2, mArg3);
  }


private:
  Arg1Type  mArg1;
  Arg2Type  mArg2;
  Arg3Type  mArg3;
};



/**
 * A subclass template of sbRunnableMethod_<> for methods that take four
 * arguments.  Use the optional Arg*Types to specify the types used to
 * curry arguments if any should differ from the corresponding Param*Type.
 *
 * For example, if Param2Type is nsISupports *, Arg2Type can be set to
 * nsRefPtr<nsISupports> to ensure that the runnable retains a reference to
 * the argument for the life of the runnable.
 */
template <typename ResultType,
          typename TargetType,
          typename Param1Type,
          typename Param2Type,
          typename Param3Type,
          typename Param4Type,
          typename Arg1Type = Param1Type,
          typename Arg2Type = Param2Type,
          typename Arg3Type = Param3Type,
          typename Arg4Type = Param4Type,
          typename MethodType = ResultType (TargetType::*) (Param1Type,
                                                            Param2Type,
                                                            Param3Type,
                                                            Param4Type)>
class sbRunnableMethod4_ :
  public sbRunnableMethod_<ResultType, TargetType, MethodType>
{
public:
  typedef sbRunnableMethod_<ResultType, TargetType, MethodType> BaseType;


  /**
   * Capture the object and method to call and the arguments to pass
   */
  sbRunnableMethod4_(
    TargetType &  aTarget,
    MethodType    aMethod,
    Param1Type    aArg1,
    Param2Type    aArg2,
    Param3Type    aArg3,
    Param4Type    aArg4,
    const char *  aName = NULL) :
    BaseType(aTarget, aMethod, aName),
    mArg1(aArg1),
    mArg2(aArg2),
    mArg3(aArg3),
    mArg4(aArg4)
    {}


protected:
  /**
   * Invokes the captured method on the captured object with the
   * captured arguments and returns its result.  A subclass that
   * overrides this function should eventually delegate to it.
   */
  virtual ResultType OnRun()
  {
    return ((*BaseType::mTarget).*BaseType::mMethod)(mArg1, mArg2, mArg3, mArg4);
  }


private:
  Arg1Type  mArg1;
  Arg2Type  mArg2;
  Arg3Type  mArg3;
  Arg4Type  mArg4;
};



/**
 * A subclass template of sbRunnableMethod_<> for methods that take five
 * arguments.  Use the optional Arg*Types to specify the types used to
 * curry arguments if any should differ from the corresponding Param*Type.
 *
 * For example, if Param2Type is nsISupports *, Arg2Type can be set to
 * nsRefPtr<nsISupports> to ensure that the runnable retains a reference to
 * the argument for the life of the runnable.
 */
template <typename ResultType,
          typename TargetType,
          typename Param1Type,
          typename Param2Type,
          typename Param3Type,
          typename Param4Type,
          typename Param5Type,
          typename Arg1Type = Param1Type,
          typename Arg2Type = Param2Type,
          typename Arg3Type = Param3Type,
          typename Arg4Type = Param4Type,
          typename Arg5Type = Param5Type,
          typename MethodType = ResultType (TargetType::*) (Param1Type,
                                                            Param2Type,
                                                            Param3Type,
                                                            Param4Type,
                                                            Param5Type)>
class sbRunnableMethod5_ :
  public sbRunnableMethod_<ResultType, TargetType, MethodType>
{
public:
  typedef sbRunnableMethod_<ResultType, TargetType, MethodType> BaseType;


  /**
   * Capture the object and method to call and the arguments to pass
   */
  sbRunnableMethod5_(
    TargetType &  aTarget,
    MethodType    aMethod,
    Param1Type    aArg1,
    Param2Type    aArg2,
    Param3Type    aArg3,
    Param4Type    aArg4,
    Param5Type    aArg5,
    const char *  aName = NULL) :
    BaseType(aTarget, aMethod, aName),
    mArg1(aArg1),
    mArg2(aArg2),
    mArg3(aArg3),
    mArg4(aArg4),
    mArg5(aArg5)
    {}


protected:
  /**
   * Invokes the captured method on the captured object with the
   * captured arguments and returns its result.  A subclass that
   * overrides this function should eventually delegate to it.
   */
  virtual ResultType OnRun()
  {
    return ((*BaseType::mTarget).*BaseType::mMethod)(mArg1, mArg2, mArg3, mArg4, mArg5);
  }


private:
  Arg1Type  mArg1;
  Arg2Type  mArg2;
  Arg3Type  mArg3;
  Arg4Type  mArg4;
  Arg5Type  mArg5;
};

/**
 * A subclass template of sbRunnableMethod_<> for methods that take six
 * arguments.  Use the optional Arg*Types to specify the types used to
 * curry arguments if any should differ from the corresponding Param*Type.
 *
 * For example, if Param2Type is nsISupports *, Arg2Type can be set to
 * nsRefPtr<nsISupports> to ensure that the runnable retains a reference to
 * the argument for the life of the runnable.
 */
template <typename ResultType,
          typename TargetType,
          typename Param1Type,
          typename Param2Type,
          typename Param3Type,
          typename Param4Type,
          typename Param5Type,
          typename Param6Type,
          typename Arg1Type = Param1Type,
          typename Arg2Type = Param2Type,
          typename Arg3Type = Param3Type,
          typename Arg4Type = Param4Type,
          typename Arg5Type = Param5Type,
          typename Arg6Type = Param6Type,
          typename MethodType = ResultType (TargetType::*) (Param1Type,
                                                            Param2Type,
                                                            Param3Type,
                                                            Param4Type,
                                                            Param5Type,
                                                            Param6Type)>
class sbRunnableMethod6_:
  public sbRunnableMethod_<ResultType, TargetType, MethodType>
{
public:
  typedef sbRunnableMethod_<ResultType, TargetType, MethodType> BaseType;


  /**
   * Capture the object and method to call and the arguments to pass
   */
  sbRunnableMethod6_(
    TargetType &  aTarget,
    MethodType    aMethod,
    Param1Type    aArg1,
    Param2Type    aArg2,
    Param3Type    aArg3,
    Param4Type    aArg4,
    Param5Type    aArg5,
    Param6Type    aArg6,
    const char *  aName = NULL) :
    BaseType(aTarget, aMethod, aName),
    mArg1(aArg1),
    mArg2(aArg2),
    mArg3(aArg3),
    mArg4(aArg4),
    mArg5(aArg5),
    mArg6(aArg6)
    {}


protected:
  /**
   * Invokes the captured method on the captured object with the
   * captured arguments and returns its result.  A subclass that
   * overrides this function should eventually delegate to it.
   */
  virtual ResultType OnRun()
  {
    return ((*BaseType::mTarget).*BaseType::mMethod)
             (mArg1, mArg2, mArg3, mArg4, mArg5, mArg6);
  }


private:
  Arg1Type  mArg1;
  Arg2Type  mArg2;
  Arg3Type  mArg3;
  Arg4Type  mArg4;
  Arg5Type  mArg5;
  Arg6Type  mArg6;
};

/**
 * A subclass template of sbRunnableMethod_<> for methods that take seven
 * arguments.  Use the optional Arg*Types to specify the types used to
 * curry arguments if any should differ from the corresponding Param*Type.
 *
 * For example, if Param2Type is nsISupports *, Arg2Type can be set to
 * nsRefPtr<nsISupports> to ensure that the runnable retains a reference to
 * the argument for the life of the runnable.
 */
template <typename ResultType,
          typename TargetType,
          typename Param1Type,
          typename Param2Type,
          typename Param3Type,
          typename Param4Type,
          typename Param5Type,
          typename Param6Type,
          typename Param7Type,
          typename Arg1Type = Param1Type,
          typename Arg2Type = Param2Type,
          typename Arg3Type = Param3Type,
          typename Arg4Type = Param4Type,
          typename Arg5Type = Param5Type,
          typename Arg6Type = Param6Type,
          typename Arg7Type = Param7Type,
          typename MethodType = ResultType (TargetType::*) (Param1Type,
                                                            Param2Type,
                                                            Param3Type,
                                                            Param4Type,
                                                            Param5Type,
                                                            Param6Type,
                                                            Param7Type)>
class sbRunnableMethod7_:
  public sbRunnableMethod_<ResultType, TargetType, MethodType>
{
public:
  typedef sbRunnableMethod_<ResultType, TargetType, MethodType> BaseType;


  /**
   * Capture the object and method to call and the arguments to pass
   */
  sbRunnableMethod7_(
    TargetType &  aTarget,
    MethodType    aMethod,
    Param1Type    aArg1,
    Param2Type    aArg2,
    Param3Type    aArg3,
    Param4Type    aArg4,
    Param5Type    aArg5,
    Param6Type    aArg6,
    Param7Type    aArg7,
    const char *  aName = NULL) :
    BaseType(aTarget, aMethod, aName),
    mArg1(aArg1),
    mArg2(aArg2),
    mArg3(aArg3),
    mArg4(aArg4),
    mArg5(aArg5),
    mArg6(aArg6),
    mArg7(aArg7)
    {}


protected:
  /**
   * Invokes the captured method on the captured object with the
   * captured arguments and returns its result.  A subclass that
   * overrides this function should eventually delegate to it.
   */
  virtual ResultType OnRun()
  {
    return ((*BaseType::mTarget).*BaseType::mMethod)
             (mArg1, mArg2, mArg3, mArg4, mArg5, mArg6, mArg7);
  }


private:
  Arg1Type  mArg1;
  Arg2Type  mArg2;
  Arg3Type  mArg3;
  Arg4Type  mArg4;
  Arg5Type  mArg5;
  Arg6Type  mArg6;
  Arg7Type  mArg7;
};

/**
 * A subclass template of sbRunnableMethod_<> for methods that take eight
 * arguments.  Use the optional Arg*Types to specify the types used to
 * curry arguments if any should differ from the corresponding Param*Type.
 *
 * For example, if Param2Type is nsISupports *, Arg2Type can be set to
 * nsRefPtr<nsISupports> to ensure that the runnable retains a reference to
 * the argument for the life of the runnable.
 */
template <typename ResultType,
          typename TargetType,
          typename Param1Type,
          typename Param2Type,
          typename Param3Type,
          typename Param4Type,
          typename Param5Type,
          typename Param6Type,
          typename Param7Type,
          typename Param8Type,
          typename Arg1Type = Param1Type,
          typename Arg2Type = Param2Type,
          typename Arg3Type = Param3Type,
          typename Arg4Type = Param4Type,
          typename Arg5Type = Param5Type,
          typename Arg6Type = Param6Type,
          typename Arg7Type = Param7Type,
          typename Arg8Type = Param8Type,
          typename MethodType = ResultType (TargetType::*) (Param1Type,
                                                            Param2Type,
                                                            Param3Type,
                                                            Param4Type,
                                                            Param5Type,
                                                            Param6Type,
                                                            Param7Type,
                                                            Param8Type)>
class sbRunnableMethod8_:
  public sbRunnableMethod_<ResultType, TargetType, MethodType>
{
public:
  typedef sbRunnableMethod_<ResultType, TargetType, MethodType> BaseType;


  /**
   * Capture the object and method to call and the arguments to pass
   */
  sbRunnableMethod8_(
    TargetType &  aTarget,
    MethodType    aMethod,
    Param1Type    aArg1,
    Param2Type    aArg2,
    Param3Type    aArg3,
    Param4Type    aArg4,
    Param5Type    aArg5,
    Param6Type    aArg6,
    Param7Type    aArg7,
    Param8Type    aArg8,
    const char *  aName = NULL) :
    BaseType(aTarget, aMethod, aName),
    mArg1(aArg1),
    mArg2(aArg2),
    mArg3(aArg3),
    mArg4(aArg4),
    mArg5(aArg5),
    mArg6(aArg6),
    mArg7(aArg7),
    mArg8(aArg8)
    {}


protected:
  /**
   * Invokes the captured method on the captured object with the
   * captured arguments and returns its result.  A subclass that
   * overrides this function should eventually delegate to it.
   */
  virtual ResultType OnRun()
  {
    return ((*BaseType::mTarget).*BaseType::mMethod)
             (mArg1, mArg2, mArg3, mArg4, mArg5, mArg6, mArg7, mArg8);
  }


private:
  Arg1Type  mArg1;
  Arg2Type  mArg2;
  Arg3Type  mArg3;
  Arg4Type  mArg4;
  Arg5Type  mArg5;
  Arg6Type  mArg6;
  Arg7Type  mArg7;
  Arg8Type  mArg8;
};

/**
 * A subclass template of sbRunnableMethod_<> for methods that take ten
 * arguments.  Use the optional Arg*Types to specify the types used to
 * curry arguments if any should differ from the corresponding Param*Type.
 *
 * For example, if Param2Type is nsISupports *, Arg2Type can be set to
 * nsRefPtr<nsISupports> to ensure that the runnable retains a reference to
 * the argument for the life of the runnable.
 */
template <typename ResultType,
          typename TargetType,
          typename Param1Type,
          typename Param2Type,
          typename Param3Type,
          typename Param4Type,
          typename Param5Type,
          typename Param6Type,
          typename Param7Type,
          typename Param8Type,
          typename Param9Type,
          typename Param10Type,
          typename Arg1Type = Param1Type,
          typename Arg2Type = Param2Type,
          typename Arg3Type = Param3Type,
          typename Arg4Type = Param4Type,
          typename Arg5Type = Param5Type,
          typename Arg6Type = Param6Type,
          typename Arg7Type = Param7Type,
          typename Arg8Type = Param8Type,
          typename Arg9Type = Param9Type,
          typename Arg10Type = Param10Type,
          typename MethodType = ResultType (TargetType::*) (Param1Type,
                                                            Param2Type,
                                                            Param3Type,
                                                            Param4Type,
                                                            Param5Type,
                                                            Param6Type,
                                                            Param7Type,
                                                            Param8Type,
                                                            Param9Type,
                                                            Param10Type)>
class sbRunnableMethod10_:
  public sbRunnableMethod_<ResultType, TargetType, MethodType>
{
public:
  typedef sbRunnableMethod_<ResultType, TargetType, MethodType> BaseType;


  /**
   * Capture the object and method to call and the arguments to pass
   */
  sbRunnableMethod10_(
    TargetType &  aTarget,
    MethodType    aMethod,
    Param1Type    aArg1,
    Param2Type    aArg2,
    Param3Type    aArg3,
    Param4Type    aArg4,
    Param5Type    aArg5,
    Param6Type    aArg6,
    Param7Type    aArg7,
    Param8Type    aArg8,
    Param9Type    aArg9,
    Param10Type    aArg10,
    const char *  aName = NULL) :
    BaseType(aTarget, aMethod, aName),
    mArg1(aArg1),
    mArg2(aArg2),
    mArg3(aArg3),
    mArg4(aArg4),
    mArg5(aArg5),
    mArg6(aArg6),
    mArg7(aArg7),
    mArg8(aArg8),
    mArg9(aArg9),
    mArg10(aArg10)
    {}


protected:
  /**
   * Invokes the captured method on the captured object with the
   * captured arguments and returns its result.  A subclass that
   * overrides this function should eventually delegate to it.
   */
  virtual ResultType OnRun()
  {
    return ((*BaseType::mTarget).*BaseType::mMethod)
             (mArg1, mArg2, mArg3, mArg4, mArg5, mArg6, mArg7, mArg8, mArg9, mArg10);
  }


private:
  Arg1Type  mArg1;
  Arg2Type  mArg2;
  Arg3Type  mArg3;
  Arg4Type  mArg4;
  Arg5Type  mArg5;
  Arg6Type  mArg6;
  Arg7Type  mArg7;
  Arg8Type  mArg8;
  Arg9Type  mArg9;
  Arg10Type  mArg10;
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
 * From the supplied thread, invoke the method specified by aMethod on the
 * object specified by aObject.  Return the method's return value. On any error,
 * return the value specified by aFailureReturnValue. Pass to the method the
 * argument value specified by aArg1.
 *
 * \param aObject               Object for which to invoke method.
 * \param aMethod               Method to invoke.
 * \param aFailureReturnValue   Value to return on failure.
 * \param aArg1                 Value of first method argument.
 * \param aThread               Thread to run on.
 *
 * \return                      Value returned by invoked method or
 *                              aFailureReturnValue on failure.
 */
template <class T, class MT, class RT, class A1, class TH>
inline
RT sbInvokeOnThread1(T & aObject,
                     MT aMethod,
                     RT aFailureReturnValue,
                     A1 aArg1,
                     TH aThread)
{
  return sbRunnableMethod1<T, RT, A1>::InvokeOnThread(&aObject,
                                                      aMethod,
                                                      aFailureReturnValue,
                                                      aArg1,
                                                      aThread);
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
nsresult sbInvokeOnMainThread1Async(T & aObject,
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

/**
 * From the supplied thread, invoke asynchronously the method specified by
 * aMethod on the object specified by aObject. On any error, return the value
 * specified by aFailureReturnValue. Pass to the method the argument value
 * specified by aArg1.
 *
 * \param aObject               Object for which to invoke method.
 * \param aMethod               Method to invoke.
 * \param aFailureReturnValue   Value to return on failure.
 * \param aArg1                 Value of first method argument.
 * \param aThread               Thread to run on.
 *
 * \return                      aFailureReturnValue on failure to invoke
 */
template <class T, class MT, class RT, class A1, class TH>
inline
nsresult sbInvokeOnThread1Async(T & aObject,
                                MT aMethod,
                                RT aFailureReturnValue,
                                A1 aArg1,
                                TH aThread)
{
  return sbRunnableMethod1<T, RT, A1>::InvokeOnThreadAsync(&aObject,
                                                           aMethod,
                                                           aFailureReturnValue,
                                                           aArg1,
                                                           aThread);
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
 * From the supplied thread, invoke the method specified by aMethod on the
 * object specified by aObject.  Return the method's return value. On any error,
 * return the value specified by aFailureReturnValue. Pass to the method the
 * argument value specified by aArg1.
 *
 * \param aObject               Object for which to invoke method.
 * \param aMethod               Method to invoke.
 * \param aFailureReturnValue   Value to return on failure.
 * \param aArg1                 Value of the first argument.
 * \param aArg2                 Value of the second argument
 * \param aThread               Thread to run on.
 *
 * \return                      aFailureReturnValue on failure.
 */
template <class T, class MT, class RT, class A1, class A2, class TH>
inline
RT sbInvokeOnThread2(T & aObject,
                     MT aMethod,
                     RT aFailureReturnValue,
                     A1 aArg1,
                     A2 aArg2,
                     TH aThread)
{
  return sbRunnableMethod2<T, RT, A1, A2>::InvokeOnThread(&aObject,
                                                          aMethod,
                                                          aFailureReturnValue,
                                                          aArg1,
                                                          aArg2,
                                                          aThread);
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
nsresult sbInvokeOnMainThread2Async(T & aObject,
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
 * From the supplied thread, invoke asynchronously the method specified by
 * aMethod on the object specified by aObject.  Return the method's return
 * value. On any error, return the value specified by aFailureReturnValue. Pass
 * to the method the argument value specified by aArg1.
 *
 * \param aObject               Object for which to invoke method.
 * \param aMethod               Method to invoke.
 * \param aFailureReturnValue   Value to return on failure.
 * \param aArg1                 Value of the first argument.
 * \param aArg2                 Value of the second argument
 * \param aThread               Thread to run on.
 *
 * \return                      Value returned by invoked method or
 *                              aFailureReturnValue on failure.
 */
template <class T, class MT, class RT, class A1, class A2, class TH>
inline
nsresult sbInvokeOnThread2Async(T & aObject,
                                MT aMethod,
                                RT aFailureReturnValue,
                                A1 aArg1,
                                A2 aArg2,
                                TH aThread)
{
  return sbRunnableMethod2<T, RT, A1, A2>::InvokeOnThreadAsync(
                                                            &aObject,
                                                            aMethod,
                                                            aFailureReturnValue,
                                                            aArg1,
                                                            aArg2,
                                                            aThread);
}

/**
 * From the main thread, asynchronously invoke the method specified by aMethod
 * on the object specified by aObject of the type specified by aClassType.  Pass
 * to the method the argument value specified by aArg1Value of type specified by
 * aArg1Type.  Use the value specified by aFailureReturnValue as the runnable
 * method failure return value.
 *XXXeps the return value is only needed to properly create the object.
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

