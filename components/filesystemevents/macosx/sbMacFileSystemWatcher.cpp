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

#include "sbMacFileSystemWatcher.h"


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
}

sbMacFileSystemWatcher::~sbMacFileSystemWatcher()
{
}

NS_IMETHODIMP 
sbMacFileSystemWatcher::StartWatching()
{
  if (mIsWatching) {
    return NS_OK;
  }

  // Init the tree
  mTree = new sbFileSystemTree();
  NS_ENSURE_TRUE(mTree, NS_ERROR_OUT_OF_MEMORY);

  // Add ourselves as a tree listener
  nsresult rv = mTree->AddListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  // Build the tree snapshot now, FSEvent stream will start once the tree
  // has been built.
  rv = mTree->Init(mWatchPath, mIsRecursive);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbMacFileSystemWatcher::StopWatching()
{
  if (!mIsWatching) {
    return NS_OK;
  }

  if (mContext) {
    free(mContext);
  }

  FSEventStreamStop(mStream);
  FSEventStreamInvalidate(mStream);
  FSEventStreamRelease(mStream);
 
  // TODO: Snapshot the tree?

  mIsWatching = PR_FALSE;
  
  return NS_OK;
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

void
sbMacFileSystemWatcher::OnTreeReady()
{
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
                                3.0,  // lag in seconds
                                kFSEventStreamCreateFlagNone);
  
  FSEventStreamScheduleWithRunLoop(mStream, 
                                   CFRunLoopGetCurrent(), 
                                   kCFRunLoopDefaultMode);
  // Now start the FSEvent stream
  mIsWatching = FSEventStreamStart(mStream);
  NS_ASSERTION(mIsWatching, "ERROR: Could not start the FSEvent stream!");
}

