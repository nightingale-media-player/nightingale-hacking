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

#ifndef _SB_MEDIAITEMCONTROLLERCLEANUP_H_
#define _SB_MEDIAITEMCONTROLLERCLEANUP_H_

/**
 * \file sbMediaItemControllerCleanup.h
 * \brief A component used to automatically hide items that require media item
 * controllers which are no longer available
 */

#include <mozilla/ModuleUtils.h>
#include <nsIObserver.h>
#include <nsIRunnable.h>

#include <sbILibrary.h>
#include <sbILibraryManagerListener.h>
#include <sbIMediaListListener.h>
#include <sbIPropertyArray.h>

#include <mozilla/Mutex.h>
#include <nsAutoPtr.h>
#include <nsCOMPtr.h>

#include <map>
#include <set>
#include <string>

class nsIEventTarget;

class sbMediaItemControllerCleanup : public nsIObserver,
                                     public nsIRunnable,
                                     public sbILibraryManagerListener
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIRUNNABLE
  NS_DECL_SBILIBRARYMANAGERLISTENER

public:
  sbMediaItemControllerCleanup();
  ~sbMediaItemControllerCleanup();

  // static NS_METHOD RegisterSelf(nsIComponentManager* aCompMgr,
  //                               nsIFile* aPath,
  //                               const char* aLoaderStr,
  //                               const char* aType,
  //                               const nsModuleComponentInfo *aInfo);

protected:
  ///// helper class
  class sbEnumerationHelper : public sbIMediaListEnumerationListener,
                              public sbIMediaListBatchCallback
  {
      NS_DECL_ISUPPORTS
      NS_DECL_SBIMEDIALISTENUMERATIONLISTENER
      NS_DECL_SBIMEDIALISTBATCHCALLBACK
    public:
      /**
       * Create a new enumeration helper used to set properties on items which
       * match a given set of properties
       * @param aMediaList the media list to operate on
       * @param aPropertiesToFilter the set of properties describing which
       *        items to set
       * @param aPropertiesToSet the set of properties to set on the found items
       */
      sbEnumerationHelper(sbIMediaList *aMediaList,
                          sbIPropertyArray *aPropertiesToFilter,
                          sbIPropertyArray *aPropertiesToSet);
      /**
       * Inform the listener that it should try to stop the enumeration
       * as soon as possible.  Note that this is asynchronous and will
       * not actually stop until some time later.
       */
      void Stop();
      
      /**
       * Ask the listener to abort the stop attempt.  This may not actually
       * abort the attempt if it has already stopped.
       */
      void Resume();

      /**
       * Check if the run was successfully completed.  This may be false if the
       * operation was aborted via Stop().
       */
      bool Completed();

      already_AddRefed<sbIMediaList> GetMediaList();

    protected:
      nsCOMPtr<sbIMediaList> mList;
      nsCOMPtr<sbIPropertyArray> mPropsToFilter;
      nsCOMPtr<sbIPropertyArray> mPropsToSet;
      bool mStop;
      bool mCompleted;
      PRUint32 mCount;
  };
  nsRefPtr<sbEnumerationHelper> mListener;

protected:
  /**
   * Map of types that are interesting
   * The key is the type name
   * The value is true if the type has been added, and false if it has been
   * removed. This means the types which didn't change are not listed.
   */
  typedef std::map<std::string, bool> types_t;
  
  /**
   * Makes sure the mAvailableTypes member is correctly filled out
   */
  nsresult EnsureAvailableTypes();

  /**
   * Goes through the libraries we need to process and hides items with
   * unavailable track types
   */
  nsresult ProcessLibraries();

  types_t mAvailableTypes;
  bool mAvailableTypesInitialized;
  
  // whether we have been registered with the idle service
  bool mIdleServiceRegistered;
  
  // the set of libraries we need to process
  template <class T> struct nsCOMPtrComp {
    bool operator()(const nsCOMPtr<T> l, const nsCOMPtr<T> r) const {
      return l.get() < r.get();
    }
  };
  typedef std::map<nsCOMPtr<sbILibrary>,  // key is the library
                   types_t,               // value is interesting types
                   nsCOMPtrComp<sbILibrary> > libraries_t;
  libraries_t mLibraries;

  /**
   * The thread we will run our jobs on
   */
  nsCOMPtr<nsIEventTarget> mBackgroundEventTarget;
  
  /**
   * The current state of the jobs
   */
  enum {
    STATE_IDLE,     // nothing queued
    STATE_QUEUED,   // things queued, but not yet running
    STATE_RUNNING,  // the jobs have been dispatched to the event target and
                    // are not yet finished
    STATE_STOPPING, // idle was interrupted and we should try to stop now
                    // (but it's still running; will probably go to QUEUED)
  } mState;
  
  /**
   * The lock guards the following variables:
   *
   *  - mState
   *  - mLibraries
   */
  mozilla::Mutex mMutex;
};

#define SONGBIRD_MEDIAITEMCONTROLLERCLEANUP_CONTRACTID \
  "@songbirdnest.com/Songbird/Library/MediaItemControllerCleanup;1"
#define SONGBIRD_MEDIAITEMCONTROLLERCLEANUP_CLASSNAME "sbMediaItemControllerCleanup"
#define SONGBIRD_MEDIAITEMCONTROLLERCLEANUP_CID                                \
  /* {7F29A8D6-B4CF-4d6e-8F86-3145A3DEFF88} */                                 \
  { 0x7f29a8d6,                                                                \
    0xb4cf,                                                                    \
    0x4d6e,                                                                    \
    { 0x8f, 0x86, 0x31, 0x45, 0xa3, 0xde, 0xff, 0x88 }                         \
  }


#endif /* _SB_MEDIAITEMCONTROLLERCLEANUP_H_ */
