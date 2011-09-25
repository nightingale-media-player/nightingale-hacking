/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

#include "sbMacFileSystemWatcher.h"

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbMacFSWatcher:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gMacFSWatcherLog = nsnull;
#define TRACE(args) PR_LOG(gMacFSWatcherLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gMacFSWatcherLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */

/* make GCC pretty */
#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif

//------------------------------------------------------------------------------
// FSEvents Callback

/* static */ void 
sbMacFileSystemWatcher::FSEventCallback(ConstFSEventStreamRef aStreamRef,
                                        void *aClientCallbackInfo,
                                        size_t aNumEvents,
                                        const char *const aEventPaths[],
                                        const FSEventStreamEventFlags aEventFlags[],
                                        const FSEventStreamEventId aEventIds[])
{
  TRACE(("%s: %i events", __FUNCTION__, aNumEvents));
  sbMacFileSystemWatcher *watcher = 
    static_cast<sbMacFileSystemWatcher *>(aClientCallbackInfo);
  if (!watcher) {
    return;
  }

  sbStringArray paths;
  for (unsigned int i = 0; i < aNumEvents; i++) {
    nsString curPath;
    curPath.AppendLiteral(aEventPaths[i]);
    paths.AppendElement(curPath);
  }

  watcher->OnFileSystemEvents(paths);
}

//------------------------------------------------------------------------------

sbMacFileSystemWatcher::sbMacFileSystemWatcher()
{
#ifdef PR_LOGGING
  if (!gMacFSWatcherLog) {
    gMacFSWatcherLog = PR_NewLogModule("sbMacFSWatcher");
  }
#endif
  // Check to see if the current runtime is at least 10.5 or higher.
  mIsSupported = PR_FALSE;
  SInt32 macVersion;
  if (Gestalt(gestaltSystemVersion, &macVersion) == noErr) {
    if (macVersion >= 0x1050) {
      mIsSupported = PR_TRUE;
    }
  }
}

sbMacFileSystemWatcher::~sbMacFileSystemWatcher()
{
}

NS_IMETHODIMP
sbMacFileSystemWatcher::Init(sbIFileSystemListener *aListener, 
                             const nsAString & aRootPath, 
                             PRBool aIsRecursive)
{
  TRACE(("%s: path=%s", __FUNCTION__, NS_ConvertUTF16toUTF8(aRootPath).get()));
  if (!mIsSupported) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  return sbBaseFileSystemWatcher::Init(aListener, aRootPath, aIsRecursive);
}

NS_IMETHODIMP
sbMacFileSystemWatcher::InitWithSession(const nsACString & aSessionGuid,
                                        sbIFileSystemListener *aListener)
{
  TRACE(("%s: session %s", __FUNCTION__, aSessionGuid.BeginReading()));
  if (!mIsSupported) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  return sbBaseFileSystemWatcher::InitWithSession(aSessionGuid, aListener);
}

NS_IMETHODIMP 
sbMacFileSystemWatcher::StopWatching(PRBool aShouldSaveSession)
{
  TRACE(("%s: save %i", __FUNCTION__, aShouldSaveSession));
  if (!mIsSupported) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  
  if (!mIsWatching) {
    return NS_OK;
  }

  if (mContext) {
    free(mContext);
  }

  FSEventStreamStop(mStream);
  FSEventStreamInvalidate(mStream);
  FSEventStreamRelease(mStream); 

  // The base class takes care of cleaning up the tree and notifying the
  // listener.
  return sbBaseFileSystemWatcher::StopWatching(aShouldSaveSession);
}

void 
sbMacFileSystemWatcher::OnFileSystemEvents(const sbStringArray &aEventPaths)
{
  PRUint32 length = aEventPaths.Length();
  for (PRUint32 i = 0; i < length; i++) {
    mTree->Update(aEventPaths[i]);
  }
}

//------------------------------------------------------------------------------
// sbFileSystemTreeListener

NS_IMETHODIMP
sbMacFileSystemWatcher::OnTreeReady(const nsAString & aTreeRootPath,
                                    sbStringArray & aDirPathArray)
{
  TRACE(("%s: tree at %s ready", __FUNCTION__,
         NS_ConvertUTF16toUTF8(aTreeRootPath).get()));
  if (mWatchPath.IsEmpty()) {
    // If the watch path is empty here, this means that the tree was loaded
    // from a previous session. Set the watch path now.
    mWatchPath.Assign(aTreeRootPath);
  }

  // Now start watching
  CFStringRef path = 
    CFStringCreateWithCString(NULL, 
                              NS_ConvertUTF16toUTF8(mWatchPath).get(), 
                              kCFStringEncodingUTF8);
  CFArrayRef paths = CFArrayCreate(NULL, (const void **)&path, 1, NULL);
  
  // Setup callback reference
  mContext = (FSEventStreamContext *) malloc(sizeof(FSEventStreamContext));
  mContext->info = (void *)this;
  mContext->version = 0;
  mContext->retain = NULL;
  mContext->release = NULL;
  mContext->copyDescription = NULL;
  
  mStream = FSEventStreamCreate(NULL, 
                                (FSEventStreamCallback)&FSEventCallback, 
                                mContext,
                                paths, 
                                kFSEventStreamEventIdSinceNow,
                                1.0,  // lag in seconds
                                kFSEventStreamCreateFlagNone);
  
  FSEventStreamScheduleWithRunLoop(mStream, 
                                   CFRunLoopGetCurrent(), 
                                   kCFRunLoopDefaultMode);
  // Now start the FSEvent stream
  mIsWatching = FSEventStreamStart(mStream);
  NS_ENSURE_TRUE(mIsWatching, NS_ERROR_UNEXPECTED);

  nsresult rv = mListener->OnWatcherStarted();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

