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

#include "sbMacFSObserverService.h"

#include "nsIFile.h"
#include "nsStringAPI.h"
#include "nsCOMPtr.h"
#include <sys/types.h>
#include <sys/event.h>
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#import "unistd.h"
#import "fcntl.h"


// don't use all the file-descriptors
static const unsigned int kNumMaxFileDesc = 25;  
static const int kSecondPollingInterval = 60;

// Watched directory dictionary keys
static NSString *const kObserversKey = @"observers";
static NSString *const kFileDescriptorKey = @"fdes";


/*****************************************************************************/


// Observer ObjC Wrapper:
@interface SBObserverWrapper : NSObject
{
@protected
  sbIFileSystemObserver *mObserver;  // add-ref's
  nsIFile               *mFile;      // add-ref's
}

- (id)initWithObserver:(sbIFileSystemObserver *)aObserver 
               forFile:(nsIFile *)aFile;

// Event callbacks:
- (void)onFileSystemChanged;
- (void)onFileSystemRenamed;
- (void)onFileSystemRemoved;

@end


@implementation SBObserverWrapper

- (id)initWithObserver:(sbIFileSystemObserver *)aObserver forFile:(nsIFile *)aFile
{
  if ((self = [super init])) {
    NS_IF_ADDREF(mObserver = aObserver);
    NS_IF_ADDREF(mFile = aFile);
  }
  
  return self;
}


- (void)dealloc
{
  NS_IF_RELEASE(mObserver);
  NS_IF_RELEASE(mFile);
  [super dealloc];
}


- (BOOL)isEqual:(id)anObject
{
  SBObserverWrapper *observer = (SBObserverWrapper *)anObject;
  if (!observer)
    return NO;
  
  return (observer->mObserver == mObserver);
}


- (void)onFileSystemChanged
{
  mObserver->OnFileSystemChanged(mFile);
}


- (void)onFileSystemRenamed
{
  mObserver->OnFileSystemRenamed(mFile);
}


- (void)onFileSystemRemoved
{
  mObserver->OnFileSystemRemoved(mFile);
}

@end


/*****************************************************************************/


// The Obj-C watcher thingy...
@interface SBFileSystemWatcher : NSObject
{
@private
  NSMutableDictionary *mWatchInfoDict;  // strong ref
  NSMutableArray      *mWatchArray;     // strong ref
  
  BOOL mShouldRunThread;
  BOOL mThreadIsRunning;
  int  mQueueFileDesc;
}

- (void)startWatchingResource:(nsIFile *)aFileSpec
                     observer:(sbIFileSystemObserver *)aObserver;

- (void)stopWatchingResource:(nsIFile *)aFileSpec
                     observer:(sbIFileSystemObserver *)aObserver;

- (void)notifyObserversFileChanged:(NSArray *)inObservers;
- (void)notifyObserversFileRenamed:(NSArray *)inObservers;
- (void)notifyObserversFileRemoved:(NSArray *)inObservers;

@end


@interface SBFileSystemWatcher (Private)

- (void)addDirectoryToQueue:(NSString *)aDirPath 
               withObserver:(SBObserverWrapper *)aObserver 
              isDirListener:(BOOL)inIsDirListener;

- (void)startPolling;
- (void)stopPolling;
- (void)pollWatchedDirectories;
- (void)performSelector:(SEL)aSelector forObjects:(NSEnumerator *)aEnum;

@end


@implementation SBFileSystemWatcher

- (id)init
{
  if ((self = [super init])) {
    mWatchInfoDict = [[NSMutableDictionary alloc] init];
    mWatchArray = [[NSMutableArray alloc] init];
    mQueueFileDesc = kqueue();
  }
  
  return self;
}


- (void)dealloc
{
  close(mQueueFileDesc);
  [mWatchInfoDict release];
  [mWatchArray release];
  [super dealloc];
}


- (void)startWatchingResource:(nsIFile *)aFileSpec
                     observer:(sbIFileSystemObserver *)aObserver
{
  nsString nativePath;
  aFileSpec->GetPath(nativePath);
  NSString *resourcePath = 
    [NSString stringWithCharacters:(UniChar *)nativePath.get() 
                            length:nativePath.Length()];
  
  SBObserverWrapper *observer = 
    [[SBObserverWrapper alloc] initWithObserver:aObserver forFile:aFileSpec];
  
  // See if the resource is currently being watched.
  @synchronized(mWatchInfoDict) {    
    NSMutableDictionary *pathInfoDict = [mWatchInfoDict objectForKey:resourcePath];
    
    if (pathInfoDict) {
      NSMutableArray *observers = [pathInfoDict objectForKey:kObserversKey];
      
      // Looks like this resource is already being watched, add another 
      // observer if the in-param observer isn't already in the observer list. 
      // Note: Compares |sbIFileSystemObserver| pointers
      if (![observers containsObject:observer])
        [observers addObject:observer];
    }

    // Since we don't have an entry, add one and setup the kqueue event.
    else {
      // Cap the number of kqueues so we don't use all the file-descriptors.
      if ([mWatchInfoDict count] >= kNumMaxFileDesc)
        return;
      
      int fileDesc = open([resourcePath fileSystemRepresentation], O_EVTONLY, 0);
      if (fileDesc >= 0) {
        struct timespec nullts = { 0, 0 };
        struct kevent ev;
        u_int fflags = NOTE_RENAME | NOTE_WRITE | NOTE_DELETE;
        
        // |mWatchArray| needs to own pathContext for the lifetime of
        // this kqueue, so that it is safe to use as context data. We need to
        // be sure that we are using the *same* string object when we are
        // tracking multiple files in the same folder.
        NSString *pathContext = resourcePath;
        unsigned int pathIndex = [mWatchArray indexOfObject:pathContext];
        if ([mWatchArray indexOfObject:pathContext] == NSNotFound)
          [mWatchArray addObject:pathContext];
        else
          pathContext = [mWatchArray objectAtIndex:pathIndex];
        
        EV_SET(&ev, fileDesc, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_CLEAR, 
               fflags, 0, (void *)pathContext);
        
        kevent(mQueueFileDesc, &ev, 1, NULL, 0, &nullts);
        if (!mThreadIsRunning)
          [self startPolling];
        
        NSMutableDictionary *resourceInfoDict = 
          [NSMutableDictionary dictionaryWithObjectsAndKeys:
            [NSMutableArray arrayWithObject:observer], kObserversKey,
            [NSNumber numberWithInt:fileDesc], kFileDescriptorKey,
            nil];
        
        // Finally, add this to the main watched dict
        [mWatchInfoDict setObject:resourceInfoDict forKey:pathContext];
      }
    }
  }
  
  [observer release];
}


- (void)stopWatchingResource:(nsIFile *)aFileSpec
                    observer:(sbIFileSystemObserver *)aObserver
{
  nsString nativePath;
  aFileSpec->GetPath(nativePath);
  NSString *resourcePath = 
    [NSString stringWithCharacters:(UniChar *)nativePath.get() 
                            length:nativePath.Length()];
  
  SBObserverWrapper *observer = 
    [[SBObserverWrapper alloc] initWithObserver:aObserver forFile:aFileSpec];
  
  // Find the instance in the data-structure.
  @synchronized(mWatchInfoDict) {
    NSMutableDictionary *pathInfoDict = [mWatchInfoDict objectForKey:resourcePath];
    if (pathInfoDict) {
      NSMutableArray *observers = [pathInfoDict objectForKey:kObserversKey];
      // Note: Compares |sbIFileSystemObserver| pointers
      int observerIndex = [observers indexOfObject:observer];
      if (observerIndex != NSNotFound) {
        [observers removeObjectAtIndex:observerIndex];
        
        if ([observers count] == 0) {
          int fileDesc = [[pathInfoDict objectForKey:kFileDescriptorKey] intValue];
          close(fileDesc);
          
          [mWatchInfoDict removeObjectForKey:resourcePath];
          
          // Since there isn't anything left to watch, stop our polling.
          if (mShouldRunThread && [mWatchInfoDict count] == 0)
            [self stopPolling];
        }
      }
    }
  }
  
  [observer release];
}


- (void)startPolling
{
  @synchronized(self) {
    mShouldRunThread = YES;
    if (!mThreadIsRunning) {
      mThreadIsRunning = YES;
      [NSThread detachNewThreadSelector:@selector(pollWatchedDirectories) 
                               toTarget:self 
                             withObject:nil];
    }
  }
}


- (void)stopPolling
{
  @synchronized(self) {
    mShouldRunThread = NO;
  }
}


- (void)pollWatchedDirectories
{
  const struct timespec timeInterval = { kSecondPollingInterval, 0 };
  
  while (1) {
    // break the loop if the thread is supposed to stop
    @synchronized(self) {
      if (!mShouldRunThread) {
        mThreadIsRunning = NO;
        break;
      }
    }
    
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    // Sync the watched directories with the current watched info state.
    // Doing this here ensures that the |kevent| data will never vanish out
    // from under a triggering kqueue.
    @synchronized(mWatchInfoDict) {
      for (int i = [mWatchArray count] - 1; i >= 0; --i) {
        NSString *curDir = [mWatchArray objectAtIndex:i];
        if (![mWatchInfoDict objectForKey:curDir]) {
          [mWatchArray removeObjectAtIndex:i];
        }
      }
    }
    
    @try {
      struct kevent event;
      int n = kevent(mQueueFileDesc, NULL, 0, &event, 1,
                     (const struct timespec*)&timeInterval);
      
      BOOL inShouldRemove = NO;
      if (n > 0 && event.filter == EVFILT_VNODE && event.fflags) {
        SEL notifySelector = nil;
        
        if (event.fflags & NOTE_WRITE) {
          notifySelector = @selector(notifyObserversFileChanged:);
        }
        else if (event.fflags & NOTE_RENAME) {
          notifySelector = @selector(notifyObserversFileRenamed:);
        }
        else if (event.fflags & NOTE_DELETE) {
          notifySelector = @selector(notifyObserversFileRemoved:);
          inShouldRemove = YES;
        }
        
        if (notifySelector != nil) {
          @synchronized(mWatchInfoDict) {
            NSString *eventPath = (NSString *)event.udata;
            NSDictionary *resourceDict = [mWatchInfoDict objectForKey:eventPath];
            NSArray *observers = [resourceDict objectForKey:kObserversKey];
            
            [self performSelectorOnMainThread:notifySelector
                                   withObject:observers 
                                waitUntilDone:NO];
            
            // Since the resource has gone away, we can remove the listener
            // from the cache..
            if (inShouldRemove) {
              [mWatchInfoDict removeObjectForKey:eventPath];
              
              // If we removed the last item from the watch list, set the
              // thread to shutdown.
              if ([mWatchInfoDict count] == 0) {
                mThreadIsRunning = NO;
                break;
              }
            }
          }
        }
      }
    }
    @catch (id exception) {
      NSLog(@"Error in watcherThread: %@", exception);
    }
    
    [pool release];
  }
}


- (void)notifyObserversFileChanged:(NSArray *)inObservers 
{
  [self performSelector:@selector(onFileSystemChanged) 
             forObjects:[inObservers objectEnumerator]];
}


- (void)notifyObserversFileRemoved:(NSArray *)inObservers
{
  [self performSelector:@selector(onFileSystemRemoved) 
             forObjects:[inObservers objectEnumerator]];
}


- (void)notifyObserversFileRenamed:(NSArray *)inObservers
{
  [self performSelector:@selector(onFileSystemRenamed) 
             forObjects:[inObservers objectEnumerator]];
}


- (void)performSelector:(SEL)aSelector forObjects:(NSEnumerator *)aEnum
{
  NSObject *curObject = nil;
  while ((curObject = [aEnum nextObject])) {
    if ([curObject respondsToSelector:aSelector])
      [curObject performSelector:aSelector];
  }
}

@end

/*****************************************************************************/

NS_IMPL_ISUPPORTS1(sbMacFileSystemObserverService, 
                   sbIFileSystemObserverService)


sbMacFileSystemObserverService::sbMacFileSystemObserverService()
{
  mFSWatcher = [[SBFileSystemWatcher alloc] init];
}


sbMacFileSystemObserverService::~sbMacFileSystemObserverService()
{
  [mFSWatcher release];
}


NS_IMETHODIMP 
sbMacFileSystemObserverService::AddResourceObserver(nsIFile *aFileSpec, 
                                                    sbIFileSystemObserver *aObserver)
{
  NS_ENSURE_ARG_POINTER(aFileSpec);
  NS_ENSURE_ARG_POINTER(aObserver);
  
  [mFSWatcher startWatchingResource:aFileSpec observer:aObserver];
  return NS_OK;
}


NS_IMETHODIMP 
sbMacFileSystemObserverService::RemoveResourceObserver(nsIFile *aFileSpec, 
                                                       sbIFileSystemObserver *aObserver)
{
  NS_ENSURE_ARG_POINTER(aFileSpec);
  NS_ENSURE_ARG_POINTER(aObserver);
  
  [mFSWatcher stopWatchingResource:aFileSpec observer:aObserver];
  return NS_OK;
}



