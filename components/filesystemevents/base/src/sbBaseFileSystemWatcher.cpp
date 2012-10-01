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

#include "sbBaseFileSystemWatcher.h"

#include <nsIUUIDGenerator.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>

#include <sbDebugUtils.h>

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbBaseFSWatcher:5
 */

NS_IMPL_THREADSAFE_ISUPPORTS1(sbBaseFileSystemWatcher, sbIFileSystemWatcher)

sbBaseFileSystemWatcher::sbBaseFileSystemWatcher()
{
  SB_PRLOG_SETUP(sbBaseFSWatcher);

  mIsRecursive = PR_TRUE;
  mIsWatching = PR_FALSE;
  mIsSupported = PR_TRUE;
  mShouldLoadSession = PR_FALSE;
  mTree = nsnull;
}

sbBaseFileSystemWatcher::~sbBaseFileSystemWatcher()
{
  if (mTree) {
    nsresult SB_UNUSED_IN_RELEASE(rv) = mTree->ClearListener();
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Error could not clear tree listener!");
  }
}

//------------------------------------------------------------------------------
// sbIFileSystemWatcher

NS_IMETHODIMP 
sbBaseFileSystemWatcher::Init(sbIFileSystemListener *aListener, 
                              const nsAString & aRootPath, 
                              bool aIsRecursive)
{
  NS_ENSURE_ARG_POINTER(aListener);
  
  TRACE("%s: initing, will watch [%s]",
         __FUNCTION__,
         NS_ConvertUTF16toUTF8(aRootPath).get());

  mListener = aListener;
  mWatchPath.Assign(aRootPath);
  mIsRecursive = PR_TRUE;
  mIsWatching = PR_FALSE;
  mShouldLoadSession = PR_FALSE;

  // Generate a session GUID.
  nsresult rv;
  nsCOMPtr<nsIUUIDGenerator> uuidGen =
    do_GetService("@mozilla.org/uuid-generator;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = uuidGen->GenerateUUIDInPlace(&mSessionID);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

NS_IMETHODIMP
sbBaseFileSystemWatcher::InitWithSession(const nsACString & aSessionGuid,
                                         sbIFileSystemListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);

  mIsWatching = PR_FALSE;
  mListener = aListener;

  if (!mSessionID.Parse(nsCString(aSessionGuid).get())) {
    return NS_ERROR_FAILURE;
  }
  
  mShouldLoadSession = PR_TRUE;
  
  return NS_OK;
}

NS_IMETHODIMP 
sbBaseFileSystemWatcher::StartWatching()
{
  if (!mIsSupported) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  if (mIsWatching) {
    return NS_OK;
  }
  
  TRACE("%s: starting to watch [%s]",
         __FUNCTION__,
         NS_ConvertUTF16toUTF8(mWatchPath).get());

  // Init the tree
  mTree = new sbFileSystemTree();
  NS_ENSURE_TRUE(mTree, NS_ERROR_OUT_OF_MEMORY);

  // Add ourselves as a tree listener
  nsresult rv = mTree->SetListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  // Build the tree snapshot now, the native file system hooks will be setup
  // once the tree has been built.
  if (mShouldLoadSession) {
    rv = mTree->InitWithTreeSession(mSessionID);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = mTree->Init(mWatchPath, mIsRecursive);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseFileSystemWatcher::StopWatching(bool aShouldSaveSession)
{
  nsRefPtr<sbBaseFileSystemWatcher> kungFuDeathGrip(this);
  mIsWatching = PR_FALSE;

  // Don't worry about checking the result from the listener.
  mListener->OnWatcherStopped();

  if (aShouldSaveSession) {
    nsresult rv = mTree->SaveTreeSession(mSessionID);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  return NS_OK;
}

NS_IMETHODIMP
sbBaseFileSystemWatcher::DeleteSession(const nsACString & aSessionGuid)
{
  nsID sessionID;
  if (!sessionID.Parse(nsCString(aSessionGuid).get())) {
    return NS_ERROR_FAILURE;
  }

  nsresult rv = sbFileSystemTreeState::DeleteSavedTreeState(sessionID);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseFileSystemWatcher::GetIsWatching(bool *aIsWatching)
{
  NS_ENSURE_ARG_POINTER(aIsWatching);
  *aIsWatching = mIsWatching;
  return NS_OK;
}

NS_IMETHODIMP 
sbBaseFileSystemWatcher::GetWatchPath(nsAString & aWatchPath)
{
  aWatchPath.Assign(mWatchPath);
  return NS_OK;
}

NS_IMETHODIMP
sbBaseFileSystemWatcher::GetSessionGuid(nsACString & aSessionGuid)
{
  char idChars[NSID_LENGTH];
  mSessionID.ToProvidedString(idChars);
  aSessionGuid.Assign(idChars);
  return NS_OK;
}

NS_IMETHODIMP
sbBaseFileSystemWatcher::GetIsSupported(bool *aIsSupported)
{
  NS_ENSURE_ARG_POINTER(aIsSupported);
  *aIsSupported = mIsSupported;
  return NS_OK;
}

//------------------------------------------------------------------------------
// sbFileSystemTreeListener

NS_IMETHODIMP
sbBaseFileSystemWatcher::OnChangeFound(const nsAString & aChangePath,
                                       EChangeType aChangeType)
{
  nsresult rv;
  
  TRACE("%s: Found change in %s of type %s",
         __FUNCTION__,
         NS_ConvertUTF16toUTF8(aChangePath).get(),
         (aChangeType == eChanged) ? "change" :
         (aChangeType == eAdded)   ? "add" :
         (aChangeType == eRemoved) ? "removed" :
         "unknown");
  switch (aChangeType) {
    case eChanged:
      rv = mListener->OnFileSystemChanged(aChangePath);
      break;
    case eAdded:
      rv = mListener->OnFileSystemAdded(aChangePath);
      break;
    case eRemoved:
      rv = mListener->OnFileSystemRemoved(aChangePath);
      break;
    default:
      rv = NS_ERROR_UNEXPECTED; 
  }

  return rv;
}

NS_IMETHODIMP
sbBaseFileSystemWatcher::OnTreeReady(const nsAString & aTreeRootPath,
                                     sbStringArray & aDirPathArray)
{
  // Return fail here - since the implementor needs to implement this to start
  // its native file-system event system.
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbBaseFileSystemWatcher::OnRootPathMissing()
{
  // This callback will be notified before the tree is ready if the event
  // occurs. When this event is received, notify the listener of the error
  // and stop the tree (as per documentation in sbIFileSystemListener.idl).
  nsresult rv;
  rv = mListener->OnWatcherError(sbIFileSystemListener::ROOT_PATH_MISSING,
                                 mWatchPath);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                   "Could not notify listener of OnWatcherError()!");

  return StopWatching(PR_FALSE);  // do not save the session. 
}

NS_IMETHODIMP
sbBaseFileSystemWatcher::OnTreeSessionLoadError()
{
  char idChars[NSID_LENGTH];
  mSessionID.ToProvidedString(idChars);
  nsString sessionString;
  sessionString.Append(NS_ConvertASCIItoUTF16(idChars));
  return mListener->OnWatcherError(sbIFileSystemListener::SESSION_LOAD_ERROR,
                                   sessionString);
}

