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


NS_IMPL_ISUPPORTS1(sbBaseFileSystemWatcher, sbIFileSystemWatcher)

sbBaseFileSystemWatcher::sbBaseFileSystemWatcher()
{
  // Assume the file system watcher is supported and let sub-class override.
  mIsSupported = PR_TRUE;
}

sbBaseFileSystemWatcher::~sbBaseFileSystemWatcher()
{
}

NS_IMETHODIMP 
sbBaseFileSystemWatcher::Init(sbIFileSystemListener *aListener, 
                              const nsAString & aRootPath, 
                              PRBool aIsRecursive)
{
  NS_ENSURE_ARG_POINTER(aListener);

  mListener = aListener;
  mWatchPath.Assign(aRootPath);
  mIsRecursive = PR_TRUE;
  mIsWatching = PR_FALSE;

  // Generate a session GUID.
  nsresult rv;
  nsCOMPtr<nsIUUIDGenerator> uuidGen =
    do_GetService("@mozilla.org/uuid-generator;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsID id;
  rv = uuidGen->GenerateUUIDInPlace(&id);
  NS_ENSURE_SUCCESS(rv, rv);

  char guidChars[NSID_LENGTH];
  id.ToProvidedString(guidChars);
  mSessionGuid.Assign(NS_ConvertASCIItoUTF16(guidChars));

  return NS_OK;
}

NS_IMETHODIMP
sbBaseFileSystemWatcher::InitWithSession(const nsAString & aSessionGuid,
                                         sbIFileSystemListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);

  mListener = aListener;
  mSessionGuid.Assign(aSessionGuid);

  //
  // TODO: Load the tree from the saved session GUID.
  //

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseFileSystemWatcher::StartWatching()
{
  // This function is defined by the inherited class
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbBaseFileSystemWatcher::StopWatching(PRBool aShouldSaveSession)
{
  // This function is defined by the inherited class
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbBaseFileSystemWatcher::GetIsWatching(PRBool *aIsWatching)
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
sbBaseFileSystemWatcher::GetSessionGuid(nsAString & aSessionGuid)
{
  aSessionGuid.Assign(mSessionGuid);
  return NS_OK;
}

NS_IMETHODIMP
sbBaseFileSystemWatcher::GetIsSupported(PRBool *aIsSupported)
{
  NS_ENSURE_ARG_POINTER(aIsSupported);
  *aIsSupported = mIsSupported;
  return NS_OK;
}

//------------------------------------------------------------------------------
// sbFileSystemTreeListener

NS_IMETHODIMP
sbBaseFileSystemWatcher::OnChangeFound(nsAString & aChangePath,
                                       EChangeType aChangeType)
{
  nsresult rv;
  
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
sbBaseFileSystemWatcher::OnTreeReady(sbStringArray & aDirPathArray)
{
  // Return fail here - since the implementor needs to implement this to start
  // its native file-system event system.
  return NS_ERROR_NOT_IMPLEMENTED;
}

