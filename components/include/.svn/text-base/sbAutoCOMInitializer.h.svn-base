/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2011 POTI, Inc.
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

#ifndef SBAUTOCOMINITIALIZER_H_
#define SBAUTOCOMINITIALIZER_H_

// Platform includes
#include <assert.h>
#include <Objbase.h>
#include <windows.h>

/**
 * This class manages COM Initialization and uninitialization. You can use
 * the constructor to initialize and destructor to uninitialize COM. Or
 * you can use the Initialize and Uninitialize methods to manually manage.
 * This class should be owned by a single thread and should never be called
 * from multiple threads.
 */
class sbAutoCOMInitializer
{
public:
  /**
   * Initializes COM on construction
   */
  sbAutoCOMInitializer(DWORD aInitFlags) : mInitialized(0), mFailureResult(S_OK)
  {
    Initialize(aInitFlags);
  }

  /**
   * Initializes the object, but does not initialize COM. Iniialize must be
   * called
   */
  sbAutoCOMInitializer() : mInitialized(0),
                           mFailureResult(S_OK),
                           mInitThreadId(0)
  {
  }

  /**
   * Uninitializes COM on destruction of the object if COM has been initialized
   */
  ~sbAutoCOMInitializer()
  {
    // Only uninitialized if we have been initialized. Some users will
    // manually manage COM initialization and uninitialization
    if (Initialized()) {
      Uninitialize();
    }
    // While we might have never been uninitialized we should never leave
    // an Initialize call outstanding.
    assert(mInitialized == 0);
  }

  /**
   * Initializes COM. You may call this multiple times, it keeps track
   * internally and only initializes COM on the first call. You MUST
   * call Uninitialize for each extra call to Initialize in order for COM to be
   * property uninitialized.
   */
  HRESULT Initialize(DWORD aInitFlags = COINIT_MULTITHREADED)
  {
    HRESULT result = S_OK;
    if (++mInitialized == 1) {
      // Save off the thread ID we're initialize COM so we can test on
      // tear down
      mInitThreadId = ::GetCurrentThreadId();
      result = ::CoInitializeEx(NULL, aInitFlags);
      // If we fail for any reason then we don't want to call CoUninitialize
      if (FAILED(result)) {
        --mInitialized;
      }
      // If we get a change mode error, then COM was previously initialized on
      // this thread using a different threading model. Changing the threading
      // model could have negative impact on the code that actually set it.
      // We don't need to actually error in this case since the thread is
      // already COM initialized by something else.
      // This happens with XULRunner initializes COM with the apartment model
      // on the main thread. And then if some code calls this it gets that
      // error.
      if (FAILED(result) && result != RPC_E_CHANGED_MODE) {
        mFailureResult = result;
      }
      else {
        mFailureResult = S_OK;
        result = S_OK;
      }
    }
    return result;
  }

  /**
   * Called to uninitialize COM. Will uninitialze on the last call that
   * matches the first call to Initialize.
   */
  void Uninitialize()
  {
    // There are cases where if an error occurs and Uninitialize can get called
    // manually when no Initialize call has been made. It's not ideal but
    // we need to handle it.
    if (Initialized()) {
      if (--mInitialized == 0) {
        // Make sure we're tearing down COM on the thread we initialized it on
        // There could be some edge cases in an error path where Uninitialize
        // gets called from the destructor but not on the thread it was
        // initialized on. It's better to leak in this case than to tear down
        // COM on the wrong thread
        if (mInitThreadId == ::GetCurrentThreadId()) {
          ::CoUninitialize();
        }
      }
    }
  }

  /**
   * Returns the HRESULT failure code that was returned from CoInitializeEx
   * RPC_E_CHANGED_MODE is never returned as it's considered success.
   */
  HRESULT GetHRESULT() const
  {
    return mFailureResult;
  }

  /**
   * Determines whether initialization was successful. NOTE: Already initialized
   * error where a different thread model was specified is considered success.
   */
  bool Succeeded() const
  {
    return SUCCEEDED(mFailureResult);
  }

  bool Initialized() const
  {
    return mInitialized > 0;
  }
public:
  /**
   * Tracks whether and how many times the COM initialization has been
   * requested.
   */
  int mInitialized;

  /**
   * The HRESULT that CoInitializeEx returned.
   */
  HRESULT mFailureResult;

  /**
   * The thread ID of the thread that called Initialize. Used to ensure we call
   * CoUninitialize on the same thread.
   */
  DWORD mInitThreadId;
};

#endif
