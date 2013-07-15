/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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

#include "sbMediaItemControllerCleanup.h"

#include <nsIAppStartupNotifier.h>
#include <nsICategoryManager.h>
#include <nsIComponentManager.h>
#include <nsIComponentRegistrar.h>
#include <nsIEventTarget.h>
#include <nsIFile.h>
#include <nsIIdleService.h>
#include <nsILocalFile.h>
#include <nsIObserverService.h>
#include <nsIPropertyBag2.h>
#include <nsISimpleEnumerator.h>
#include <nsISupportsPrimitives.h>
#include <nsIURI.h>
#include <nsIURL.h>

#include <sbILibrary.h>
#include <sbILibraryManager.h>
#include <sbIMediaItemController.h>

#include <mozilla/Mutex.h>
#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsStringAPI.h>
#include <nsTextFormatter.h>
#include <nsThreadUtils.h>

#include <sbDebugUtils.h>
#include <sbStandardProperties.h>
#include <sbStringUtils.h>
#include <sbProxiedComponentManager.h>

#include <algorithm>

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbMediaItemControllerCleanup:5
 */

// The property name on the library we use to store the last seen set of
// track types
#define K_LAST_SEEN_TYPES_PROPERTY \
        "http://songbirdnest.com/data/1.0#libraryItemControllerLastSeenTypes"
#define K_LAST_SEEN_TYPES_SEPARATOR '\x7F'
// the property name was use to flag individual items as having been hidden
// due to their controller going away (so we can un-hide them again)
#define K_HIDDEN_FOR_CONTROLLER_PROPERTY \
        "http://songbirdnest.com/data/1.0#libraryItemControllerTypeDisappeared"

// the observer topic we fire when we're done
#define K_CLEANUP_COMPLETE_OBSERVER_TOPIC \
        "songbird-media-item-controller-cleanup-complete"
#define K_CLEANUP_INTERRUPTED_OBSERVER_TOPIC \
        "songbird-media-item-controller-cleanup-interrupted"
#define K_CLEANUP_IDLE_OBSERVER_TOPIC \
        "songbird-media-item-controller-cleanup-idle"
#define K_QUIT_APP_OBSERVER_TOPIC \
        "quit-application"

const PRUint32 K_IDLE_TIMEOUT = 5; // seconds

#define SB_THREADPOOLSERVICE_CONTRACTID \
  "@songbirdnest.com/Songbird/ThreadPoolService;1"


NS_IMPL_THREADSAFE_ISUPPORTS3(sbMediaItemControllerCleanup,
                              nsIObserver,
                              nsIRunnable,
                              sbILibraryManagerListener)

sbMediaItemControllerCleanup::sbMediaItemControllerCleanup()
  : mAvailableTypesInitialized(false),
    mIdleServiceRegistered(false),
    mState(STATE_IDLE),
    mMutex("sbMediaItemControllerCleanup::mMutex")
{
  SB_PRLOG_SETUP(sbMediaItemControllerCleanup);
  TRACE_FUNCTION("");
  NS_PRECONDITION(NS_IsMainThread(),
                  "sbMediaItemControllerCleanup::sbMediaItemControllerCleanup"
                  " should be called on the main thread!");
}

sbMediaItemControllerCleanup::~sbMediaItemControllerCleanup()
{
  TRACE_FUNCTION("");
  NS_PRECONDITION(mState == STATE_IDLE,
                  "sbMediaItemControllerCleanup being destroyed while not idle");
  // since, at this point, the thread manager has already shut down, we can't
  // check that this was called on the main thread :(
}

///// nsIObserver
/* void observe (in nsISupports aSubject, in string aTopic, in wstring aData); */
NS_IMETHODIMP
sbMediaItemControllerCleanup::Observe(nsISupports *aSubject,
                                      const char *aTopic,
                                      const PRUnichar *aData)
{
  TRACE_FUNCTION("topic=\"%s\"", aTopic);
  NS_PRECONDITION(NS_IsMainThread(),
                  "sbMediaItemControllerCleanup::Observe"
                  " should be called on the main thread!");
  nsresult rv;

  if (!strcmp(aTopic, "idle")) {
    // idle observer: we are now idle
    mozilla::MutexAutoLock lock(mMutex);
    if (mState == STATE_QUEUED) {
      TRACE("preparing to run: STATE->RUNNING");
      mState = STATE_RUNNING;
      rv = mBackgroundEventTarget->Dispatch(static_cast<nsIRunnable*>(this),
                                            NS_DISPATCH_NORMAL);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else if (mState == STATE_STOPPING) {
      // we can un-pause
      if (mListener) {
        mListener->Resume();
      }
      TRACE("Resuming: STATE->RUNNING");
      mState = STATE_RUNNING;
    }
    else {
      // we no longer need to hold on to the lock for anything; let it go now
      // because do_ProxiedGetService may cause a nested event loop
      // lock.unlock();
      TRACE("nothing to do, ignoring idle notification");
      nsCOMPtr<nsIObserverService> obs =
        do_ProxiedGetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      
      nsString notificationData;
      #if PR_LOGGING
        notificationData.Adopt(nsTextFormatter::smprintf((PRUnichar*)NS_LL("%llx"),
                                                         PR_Now()));
        TRACE("Notifying observers (%s) [%s]",
              K_CLEANUP_IDLE_OBSERVER_TOPIC,
              NS_ConvertUTF16toUTF8(notificationData).get());
      #endif /* PR_LOGGING */
      rv = obs->NotifyObservers(NS_ISUPPORTS_CAST(nsIObserver*, this),
                                K_CLEANUP_IDLE_OBSERVER_TOPIC,
                                notificationData.get());
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else if (!strcmp(aTopic, "back")) {
    // idle observer: we are busy again
    mozilla::MutexAutoLock lock(mMutex);
    if (mState == STATE_RUNNING) {
      TRACE("Stopping due to activity: STATE->STOPPING");
      mState = STATE_STOPPING;
    }
    if (mListener) {
      // ask the processor to stop
      mListener->Stop();
    }
  }
  else if (!strcmp(aTopic, APPSTARTUP_TOPIC)) {
    nsCOMPtr<nsIObserverService> obs =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = obs->AddObserver(static_cast<nsIObserver*>(this),
                          SB_LIBRARY_MANAGER_READY_TOPIC,
                          false);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = obs->AddObserver(static_cast<nsIObserver*>(this),
                          K_QUIT_APP_OBSERVER_TOPIC,
                          false);
    NS_ENSURE_SUCCESS(rv, rv);
    mBackgroundEventTarget = do_GetService(SB_THREADPOOLSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    mozilla::MutexAutoLock lock(mMutex);
  }
  else if (!strcmp(aTopic, SB_LIBRARY_MANAGER_READY_TOPIC)) {
    nsCOMPtr<sbILibraryManager> libManager =
      do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = libManager->AddListener(static_cast<sbILibraryManagerListener*>(this));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIObserverService> obs =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = obs->RemoveObserver(static_cast<nsIObserver*>(this),
                             SB_LIBRARY_MANAGER_READY_TOPIC);
    NS_ENSURE_SUCCESS(rv, rv);

    // since the library manager doesn't tell us about libraries loaded before
    // this point, we have to tell ourself about them :|
    nsCOMPtr<nsISimpleEnumerator> libs;
    rv = libManager->GetLibraries(getter_AddRefs(libs));
    NS_ENSURE_SUCCESS(rv, rv);
    PRBool hasLib;
    while (NS_SUCCEEDED(libs->HasMoreElements(&hasLib)) && hasLib) {
      nsCOMPtr<nsISupports> supports;
      rv = libs->GetNext(getter_AddRefs(supports));
      NS_ENSURE_SUCCESS(rv, rv);
      nsCOMPtr<sbILibrary> lib = do_QueryInterface(supports, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = OnLibraryRegistered(lib);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else if (!strcmp(aTopic, K_QUIT_APP_OBSERVER_TOPIC)) {
    nsCOMPtr<nsIIdleService> idleService =
      do_GetService("@mozilla.org/widget/idleservice;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    (void)idleService->RemoveIdleObserver(static_cast<nsIObserver*>(this),
                                          K_IDLE_TIMEOUT);
    // we don't care about failures to remove because we may have done it already
    mIdleServiceRegistered = false;

    nsCOMPtr<sbILibraryManager> libManager =
      do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = libManager->RemoveListener(static_cast<sbILibraryManagerListener*>(this));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIObserverService> obs =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = obs->RemoveObserver(static_cast<nsIObserver*>(this),
                             K_QUIT_APP_OBSERVER_TOPIC);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    NS_WARNING("unexpected topic");
  }
  return NS_OK;
}

///// nsIRunnable
/* void run (); */
NS_IMETHODIMP
sbMediaItemControllerCleanup::Run()
{
  TRACE_FUNCTION("");
  NS_PRECONDITION(!NS_IsMainThread(),
                  "sbMediaItemControllerCleanup::Run"
                  " should not be called on the main thread!");
  
  nsresult rv = NS_OK;
  bool stateOK = true;
  { /* scope */
    mozilla::MutexAutoLock lock(mMutex);
    NS_PRECONDITION(mState == STATE_RUNNING || mState == STATE_STOPPING,
                    "the act of getting things queued should have put"
                    " us into the running state");
    if (mState == STATE_STOPPING) {
      // the idle service noticed the user came back or something, before
      // we got a chance to run anything; skip the run completely
      stateOK = false;
    }
  }

  if (stateOK) {
    rv = ProcessLibraries();
  }

  PRBool complete = true;
  { /* scope */
    mozilla::MutexAutoLock lock(mMutex);
    mListener = nsnull;
    NS_POSTCONDITION(mState == STATE_RUNNING || mState == STATE_STOPPING,
                    "should not have exit running!?");
    if (mLibraries.empty()) {
      TRACE("Run complete: STATE->IDLE");
      mState = STATE_IDLE;
      complete = true;
    }
    else {
      // we can get here if the machine gets out of the idle state before the
      // jobs are all done
      TRACE("Run incomplete: STATE->QUEUED");
      mState = STATE_QUEUED;
      complete = false;
    }
  }
  NS_ENSURE_SUCCESS(rv, rv);

  // Getting here means one of two things:
  // 1) all libraries have been processed
  // 2) we were interrupted when the idle service reports the user is back

  NS_ASSERTION(mState == STATE_IDLE || mState == STATE_QUEUED,
               "Unexpected state while stopping process!");
  nsCString topic;
  if (complete) {
    topic.AssignLiteral(K_CLEANUP_COMPLETE_OBSERVER_TOPIC);
  }
  else {
    topic.AssignLiteral(K_CLEANUP_INTERRUPTED_OBSERVER_TOPIC);
  }
  nsString notificationData;
  #if PR_LOGGING
    notificationData.Adopt(nsTextFormatter::smprintf((PRUnichar*)NS_LL("%llx"),
                                                     PR_Now()));
    TRACE("Notifying observers (%s) [%s]",
          topic.get(),
          NS_ConvertUTF16toUTF8(notificationData).get());
  #endif /* PR_LOGGING */
  nsCOMPtr<nsIObserverService> obs =
    do_ProxiedGetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = obs->NotifyObservers(NS_ISUPPORTS_CAST(nsIObserver*, this),
                            topic.get(),
                            notificationData.get());
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}


// helper class for onLibraryRegistered logging
#if PR_LOGGING
namespace {
  template <typename T>
  struct STLIteratorValueIs {
    STLIteratorValueIs(typename T::second_type v) :m(v) {}
    bool operator() (const T& t) {
      return t.second == m;
    }
    typename T::second_type m;
  };
}
#endif

///// sbILibraryManagerListener
/* void onLibraryRegistered (in sbILibrary aLibrary); */
NS_IMETHODIMP
sbMediaItemControllerCleanup::OnLibraryRegistered(sbILibrary *aLibrary)
{
  TRACE_FUNCTION("");
  NS_PRECONDITION(NS_IsMainThread(),
                  "sbMediaItemControllerCleanup::OnLibraryRegistered"
                  " should be called on the main thread!");
  NS_ENSURE_ARG_POINTER(aLibrary);

  #if PR_LOGGING
  do {
    nsresult rv;
    nsString contentSrc;
    rv = aLibrary->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
                               contentSrc);
    NS_ENSURE_SUCCESS(rv, rv);
    contentSrc.Cut(0, contentSrc.RFindChar(PRUnichar('/')) + 1);
    TRACE("library: %s", NS_ConvertUTF16toUTF8(contentSrc).BeginReading());
  } while(false);
  #endif /* PR_LOGGING */

  nsresult rv;

  ///// Figure out if we need to do anything
  rv = EnsureAvailableTypes();
  NS_ENSURE_SUCCESS(rv, rv);

  nsString cachedTypesUnicode;
  rv = aLibrary->GetProperty(NS_LITERAL_STRING(K_LAST_SEEN_TYPES_PROPERTY),
                             cachedTypesUnicode);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString cachedTypes;
  LossyCopyUTF16toASCII(cachedTypesUnicode, cachedTypes);
  cachedTypes.Append(K_LAST_SEEN_TYPES_SEPARATOR);
  TRACE("cached types: %s", cachedTypes.BeginReading());

  PRInt32 start = 0, end;
  
  types_t types(mAvailableTypes);

  while ((end = cachedTypes.FindChar(K_LAST_SEEN_TYPES_SEPARATOR, start)) >= 0) {
    std::string type(cachedTypes.BeginReading() + start, end - start);
    start = end + 1;
    if (type.empty()) {
      continue;
    }
    TRACE("found cached type %s", type.c_str());
    if (mAvailableTypes.find(type) != mAvailableTypes.end()) {
      // this type used to exist, and still does
      types.erase(type);
    }
    else {
      // this type used to exist, but doe not any more
      types.insert(types_t::value_type(type, false));
    }
  }
  
  TRACE("cache update check: removed types? (%i) added types? (%i)",
        std::count_if(types.begin(),
                      types.end(),
                      STLIteratorValueIs<types_t::value_type>(false)),
        std::count_if(types.begin(),
                      types.end(),
                      STLIteratorValueIs<types_t::value_type>(true)));
  #if PR_LOGGING
  {
    types_t::const_iterator it;
    for (it = types.begin(); it != types.end(); ++it) {
      if (!it->second) {
        TRACE("type removed: %s", it->first.c_str());
      }
    }
    for (it = types.begin(); it != types.end(); ++it) {
      if (it->second) {
        TRACE("type added: %s", it->first.c_str());
      }
    }
  }
  #endif /* PR_LOGGING */

  if (!types.empty()) {
    // getting here means we have a type that was previously available but no
    // longer is, or types that didn't exist but now do.

    ///// update the cache
    // we don't need to worry about purely removing items yet; we do it after
    // we actually hide the items there. so put the union of (available types)
    // and (types removed) in the cache.
    nsCString newCache;
    types_t::const_iterator it;
    for (it = mAvailableTypes.begin(); it != mAvailableTypes.end(); ++it) {
      newCache.Append(it->first.c_str(), it->first.length());
      newCache.Append(K_LAST_SEEN_TYPES_SEPARATOR);
    }
    for (it = types.begin(); it != types.end(); ++it) {
      if (it->second) {
        // this type was not a removed type, it's still available
        continue;
      }
      newCache.Append(it->first.c_str(), it->first.length());
      newCache.Append(K_LAST_SEEN_TYPES_SEPARATOR);
    }
    newCache.SetLength(newCache.Length() - 1); // trailing separator
    TRACE("updating cache to %s", newCache.BeginReading());
    rv = aLibrary->SetProperty(NS_LITERAL_STRING(K_LAST_SEEN_TYPES_PROPERTY),
                               NS_ConvertASCIItoUTF16(newCache));
    NS_ENSURE_SUCCESS(rv, rv);
  
    ///// queue the library to be processed
    // We need to either hide items (when types are removed), or possibly
    // un-hide items (when types are added).

    nsCOMPtr<nsIIdleService> idleService =
      do_GetService("@mozilla.org/widget/idleservice;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!mIdleServiceRegistered) {
      rv = idleService->AddIdleObserver(static_cast<nsIObserver*>(this),
                                        K_IDLE_TIMEOUT);
      NS_ENSURE_SUCCESS(rv, rv);
      mIdleServiceRegistered = true;
    }
    { /* scope */
      mozilla::MutexAutoLock lock(mMutex);
      mLibraries[aLibrary] = types;
      if (mState == STATE_IDLE) {
        TRACE("Libraries added: STATE->QUEUED");
        mState = STATE_QUEUED;
      }
    }

    #if PR_LOGGING
    {
      nsString fileName((const PRUnichar*)NS_LL("<ERROR>"));
      nsCOMPtr<nsIPropertyBag2> creationParams;
      rv = aLibrary->GetCreationParameters(getter_AddRefs(creationParams));
      NS_ENSURE_SUCCESS(rv, rv);
      NS_NAMED_LITERAL_STRING(fileKey, "databaseFile");
      nsCOMPtr<nsILocalFile> databaseFile;
      rv = creationParams->GetPropertyAsInterface(fileKey,
                                                  NS_GET_IID(nsILocalFile),
                                                  getter_AddRefs(databaseFile));
      if (NS_SUCCEEDED(rv)) {
        rv = databaseFile->GetLeafName(fileName);
      }
      TRACE("queued library %s for processing",
            NS_ConvertUTF16toUTF8(fileName).get());
    }
    #endif /* PR_LOGGING */

    PRUint32 idleTime;
    rv = idleService->GetIdleTime(&idleTime);
    NS_ENSURE_SUCCESS(rv, rv);
    if (idleTime > K_IDLE_TIMEOUT * PR_MSEC_PER_SEC) {
      // we've been idle for long enough already
      rv = Observe(idleService, "idle", nsnull);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  
  return NS_OK;
}

/* void onLibraryUnregistered (in sbILibrary aLibrary); */
NS_IMETHODIMP
sbMediaItemControllerCleanup::OnLibraryUnregistered(sbILibrary *aLibrary)
{
  TRACE_FUNCTION("");
  NS_PRECONDITION(NS_IsMainThread(),
                  "sbMediaItemControllerCleanup::OnLibraryUnregistered"
                  " should be called on the main thread!");
  NS_ENSURE_ARG_POINTER(aLibrary);

  mozilla::MutexAutoLock lock(mMutex);
  
  if (mListener) {
    nsCOMPtr<sbIMediaList> list = mListener->GetMediaList();
    PRBool equals;
    if (list && NS_SUCCEEDED(list->Equals(aLibrary, &equals)) && equals) {
      mListener->Stop();
    }
  }
  
  libraries_t::iterator it = mLibraries.find(aLibrary);
  if (it != mLibraries.end()) {
    mLibraries.erase(it);
  }

  return NS_OK;
}

///// helper methods
nsresult
sbMediaItemControllerCleanup::EnsureAvailableTypes()
{
  TRACE_FUNCTION("");
  NS_PRECONDITION(NS_IsMainThread(),
                 "sbMediaItemControllerCleanup::EnsureAvailableTypes"
                 " should be called on the main thread!");
  if (mAvailableTypesInitialized) {
    return NS_OK;
  }

  nsresult rv;

  nsCOMPtr<nsIComponentRegistrar> registrar;
  rv = NS_GetComponentRegistrar(getter_AddRefs(registrar));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISimpleEnumerator> enumerator;
  rv = registrar->EnumerateContractIDs(getter_AddRefs(enumerator));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore;
  NS_NAMED_LITERAL_CSTRING(CONTRACT_PREFIX,
                           SB_MEDIAITEMCONTROLLER_PARTIALCONTRACTID);
  while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> supports;
    rv = enumerator->GetNext(getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsISupportsCString> string = do_QueryInterface(supports, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCString value;
    rv = string->GetData(value);
    NS_ENSURE_SUCCESS(rv, rv);
    if (StringBeginsWith(value, CONTRACT_PREFIX))
    {
      std::string type(value.BeginReading() + CONTRACT_PREFIX.Length(),
                       value.Length() - CONTRACT_PREFIX.Length());
      mAvailableTypes.insert(types_t::value_type(type, true));
    }
  }

  mAvailableTypesInitialized = true;
  return NS_OK;
}

nsresult
sbMediaItemControllerCleanup::ProcessLibraries()
{
  TRACE_FUNCTION("");
  NS_PRECONDITION(!NS_IsMainThread(),
                  "sbMediaItemControllerCleanup::ProcessLibraries"
                  " was expected to run on a background thread!");

  nsresult rv;

  while (true) {
    // grab the next library to look at.
    // note that mLibraries may be modified from elsewhere during our run, so
    // we don't hold on to the iterator beyond the lock.
    nsCOMPtr<sbILibrary> lib;
    types_t types;
    { /* scope */
      mozilla::MutexAutoLock lock(mMutex);
      if (mLibraries.empty()) {
        break;
      }
      libraries_t::const_iterator it = mLibraries.begin();
      lib = it->first;
      types = it->second;
    }
    NS_ASSERTION(!types.empty(),
                 "Attempted to process a library with neither added nor removed types!");
    #if PR_LOGGING
    {
      nsString fileName((const PRUnichar*)NS_LL("<ERROR>"));
      nsCOMPtr<nsIPropertyBag2> creationParams;
      rv = lib->GetCreationParameters(getter_AddRefs(creationParams));
      NS_ENSURE_SUCCESS(rv, rv);
      NS_NAMED_LITERAL_STRING(fileKey, "databaseFile");
      nsCOMPtr<nsILocalFile> databaseFile;
      rv = creationParams->GetPropertyAsInterface(fileKey,
                                                  NS_GET_IID(nsILocalFile),
                                                  getter_AddRefs(databaseFile));
      if (NS_SUCCEEDED(rv)) {
        rv = databaseFile->GetLeafName(fileName);
      }
      TRACE("got new library %p to process", NS_ConvertUTF16toUTF8(fileName).get());
    }
    #endif /* PR_LOGGING */

    types_t::const_iterator it;

    for (it = types.begin(); it != types.end(); ++it) {
      TRACE("%s type %s",
            (it->second ? "adding" : "removing"),
            it->first.c_str());
      NS_ConvertASCIItoUTF16 type(it->first.c_str(), it->first.length());

      nsCOMPtr<sbIMutablePropertyArray> mutableProp =
        do_CreateInstance("@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mutableProp->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                                       it->second ? NS_LITERAL_STRING("0")
                                                  : NS_LITERAL_STRING("1"));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mutableProp->AppendProperty(NS_LITERAL_STRING(K_HIDDEN_FOR_CONTROLLER_PROPERTY),
                                       it->second ? SBVoidString()
                                                  : NS_LITERAL_STRING("1"));
      NS_ENSURE_SUCCESS(rv, rv);
      nsCOMPtr<sbIPropertyArray> propsToSet = do_QueryInterface(mutableProp, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      mutableProp =
        do_CreateInstance("@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mutableProp->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_TRACKTYPE),
                                       type);
      NS_ENSURE_SUCCESS(rv, rv);
      if (it->second) {
        // turning things back on; only do it if we had hidden it in the first place
        rv = mutableProp->AppendProperty(NS_LITERAL_STRING(K_HIDDEN_FOR_CONTROLLER_PROPERTY),
                                         NS_LITERAL_STRING("1"));
        NS_ENSURE_SUCCESS(rv, rv);
      }
      nsCOMPtr<sbIPropertyArray> propsToFilter =
        do_QueryInterface(mutableProp, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<sbIMediaList> list = do_QueryInterface(lib, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      { /* scope */
        mozilla::MutexAutoLock lock(mMutex);
        mListener = new sbEnumerationHelper(list, propsToFilter, propsToSet);
        NS_ENSURE_TRUE(mListener, NS_ERROR_OUT_OF_MEMORY);
      }

      rv = lib->RunInBatchMode(mListener, nsnull);
      NS_ENSURE_SUCCESS(rv, rv);
      TRACE("batch complete");

      { /* scope */
        mozilla::MutexAutoLock lock(mMutex);
        if (!mListener->Completed() && mLibraries.find(lib) != mLibraries.end()) {
          // abort the run; since we didn't actually finish this type,
          // do not remove it from the list, nor do we remove the library from
          // the queue of things to process
          TRACE("run aborted");
          return NS_OK;
        }
      }

      if (!it->second) {
        // the type was removed; get rid of it from the cache
        nsString cachedTypes;
        rv = lib->GetProperty(NS_LITERAL_STRING(K_LAST_SEEN_TYPES_PROPERTY),
                              cachedTypes);
        NS_ENSURE_SUCCESS(rv, rv);
        cachedTypes.Insert(K_LAST_SEEN_TYPES_SEPARATOR, 0);
        cachedTypes.Append(K_LAST_SEEN_TYPES_SEPARATOR);
  
        type.Insert(K_LAST_SEEN_TYPES_SEPARATOR, 0);
        type.Append(K_LAST_SEEN_TYPES_SEPARATOR);
  
        PRInt32 pos = cachedTypes.Find(type);
        if (pos >= 0) {
          cachedTypes.Cut(pos, type.Length() - 1);
          char SEP[2] = {K_LAST_SEEN_TYPES_SEPARATOR, 0};
          cachedTypes.Trim(SEP);
          TRACE("setting types to \"%s\"",
                NS_LossyConvertUTF16toASCII(cachedTypes).BeginReading());
          rv = lib->SetProperty(NS_LITERAL_STRING(K_LAST_SEEN_TYPES_PROPERTY),
                                cachedTypes);
          NS_ENSURE_SUCCESS(rv, rv);
        }
        else {
          TRACE("Failed to find \"%s\" in \"%s\"",
                NS_LossyConvertUTF16toASCII(type).BeginReading(),
                NS_LossyConvertUTF16toASCII(cachedTypes).BeginReading());
        }
      }
    }

    // all types in this library are processed
    { /* scope */
      mozilla::MutexAutoLock lock(mMutex);
      mLibraries.erase(lib);
    }
  }
  
  #if PR_LOGGING
  {
    mozilla::MutexAutoLock lock(mMutex);
    TRACE("completed? %s",
          mLibraries.empty() ? "yes" : "no");
  }
  #endif /* PR_LOGGING */
  
  return NS_OK;
}

///// static
/* static */
// NS_METHOD
// sbMediaItemControllerCleanup::RegisterSelf(nsIComponentManager* aCompMgr,
//                                            nsIFile* aPath,
//                                            const char* aLoaderStr,
//                                            const char* aType,
//                                            const nsModuleComponentInfo *aInfo)
// {
//   SB_PRLOG_SETUP(sbMediaItemControllerCleanup);
//   TRACE_FUNCTION("");

//   nsresult rv = NS_ERROR_UNEXPECTED;
//   nsCOMPtr<nsICategoryManager> categoryManager =
//     do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
//   NS_ENSURE_SUCCESS(rv, rv);

//   rv = categoryManager->AddCategoryEntry(APPSTARTUP_TOPIC,
//                                          SONGBIRD_MEDIAITEMCONTROLLERCLEANUP_CLASSNAME,
//                                          "service,"
//                                          SONGBIRD_MEDIAITEMCONTROLLERCLEANUP_CONTRACTID,
//                                          PR_TRUE, PR_TRUE, nsnull);
//   NS_ENSURE_SUCCESS(rv, rv);

//   return NS_OK;
// }


///// sbIMediaListEnumerationListener helper class impl

NS_IMPL_THREADSAFE_ISUPPORTS2(sbMediaItemControllerCleanup::sbEnumerationHelper,
                              sbIMediaListEnumerationListener,
                              sbIMediaListBatchCallback)

sbMediaItemControllerCleanup::sbEnumerationHelper::sbEnumerationHelper(
    sbIMediaList *aMediaList,
    sbIPropertyArray *aPropertiesToFilter,
    sbIPropertyArray *aPropertiesToSet)
  : mStop(false),
    mCompleted(false),
    mCount(0)
{
  TRACE_FUNCTION("");
  mList = aMediaList;
  mPropsToFilter = aPropertiesToFilter;
  mPropsToSet = aPropertiesToSet;
}

/* unsigned short onEnumerationBegin (in sbIMediaList aMediaList); */
NS_IMETHODIMP
sbMediaItemControllerCleanup::sbEnumerationHelper::OnEnumerationBegin(
    sbIMediaList *aMediaList,
    PRUint16 *_retval NS_OUTPARAM)
{
  TRACE_FUNCTION("");
  NS_PRECONDITION(!NS_IsMainThread(),
                  "sbEnumerationHelper::OnEnumerationBegin"
                  " should be called on the background thread!");
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = sbIMediaListEnumerationListener::CONTINUE;
  return NS_OK;
}

/* unsigned short onEnumeratedItem (in sbIMediaList aMediaList, in sbIMediaItem aMediaItem); */
NS_IMETHODIMP
sbMediaItemControllerCleanup::sbEnumerationHelper::OnEnumeratedItem(
    sbIMediaList *aMediaList,
    sbIMediaItem *aMediaItem,
    PRUint16 *_retval NS_OUTPARAM)
{
  TRACE_FUNCTION("count: %i", ++mCount);
  NS_PRECONDITION(!NS_IsMainThread(),
                  "sbEnumerationHelper::OnEnumeratedItem"
                  " should be called on the background thread!");
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  // don't process any items beyond this one if we were asked to stop
  if (mStop)
    *_retval = sbIMediaListEnumerationListener::CANCEL;
  else
    *_retval = sbIMediaListEnumerationListener::CONTINUE;

  nsresult rv;
  rv = aMediaItem->SetProperties(mPropsToSet);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* void onEnumerationEnd (in sbIMediaList aMediaList, in nsresult aStatusCode); */
NS_IMETHODIMP
sbMediaItemControllerCleanup::sbEnumerationHelper::OnEnumerationEnd(
    sbIMediaList *aMediaList,
    nsresult aStatusCode)
{
  TRACE_FUNCTION("total: %i", mCount);
  NS_PRECONDITION(!NS_IsMainThread(),
                  "sbEnumerationHelper::OnEnumerationEnd"
                  " should be called on the background thread!");
  mCompleted = NS_SUCCEEDED(aStatusCode);
  #if PR_LOGGING
  { /* scope */
    nsresult rv;
    nsString filter, set;
    rv = mPropsToFilter->ToString(filter);
    if (NS_FAILED(rv)) filter.AssignLiteral("<error>");
    rv = mPropsToSet->ToString(filter);
    if (NS_FAILED(rv)) filter.AssignLiteral("<error>");
    TRACE("filter: %s set: %s complete? %s",
          NS_ConvertUTF16toUTF8(filter).get(),
          NS_ConvertUTF16toUTF8(set).get(),
          mCompleted ? "yes" : "no");
  }
  #endif
  return NS_OK;
}

/* void runBatched ([optional] in nsISupports aUserData); */
NS_IMETHODIMP
sbMediaItemControllerCleanup::sbEnumerationHelper::RunBatched(
    nsISupports *aUserData)
{
  TRACE_FUNCTION("stop? %s", mStop ? "yes" : "no");
  if (!mStop) {
    nsresult rv;
    rv = mList->EnumerateItemsByProperties(mPropsToFilter,
                                           this,
                                           sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  return NS_OK;
}

void
sbMediaItemControllerCleanup::sbEnumerationHelper::Stop()
{
  TRACE_FUNCTION("");
  mStop = true;
}

void
sbMediaItemControllerCleanup::sbEnumerationHelper::Resume()
{
  TRACE_FUNCTION("");
  mStop = false;
}

bool
sbMediaItemControllerCleanup::sbEnumerationHelper::Completed()
{
  TRACE_FUNCTION("");
  return mCompleted;
}

already_AddRefed<sbIMediaList>
sbMediaItemControllerCleanup::sbEnumerationHelper::GetMediaList()
{
  TRACE_FUNCTION("");
  return nsCOMPtr<sbIMediaList>(mList).forget();
}
