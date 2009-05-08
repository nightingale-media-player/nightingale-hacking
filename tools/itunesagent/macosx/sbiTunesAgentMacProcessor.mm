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

#include "sbiTunesAgentMacProcessor.h"

#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>
#import <CoreFoundation/CoreFoundation.h>
#import "SBNSString+Utils.h"
#include <sys/param.h>
#include <sstream>
#include "LoginItemsAE.h"

// Agent constants, these should be unified soon.
#define AGENT_EXPORT_FILENAME    "songbird_export.task"
#define AGENT_ERROR_FILENAME     "itunesexporterrors.txt"
#define AGENT_LOG_FILENAME       "itunesexport.log"
#define AGENT_SHUTDOWN_FILENAME  "songbird_export.shutdown"

#define AGENT_ITUNES_SLEEP_INTERVAL 5000 


//------------------------------------------------------------------------------
// Misc utility methods

inline sbError
sbOSStatusError(const char *aMsg, OSStatus & aErr)
{
  std::ostringstream msg;
  msg << "ERROR: " << aMsg << " : OSErr == " << aErr;
  return sbError(msg.str());
}

static NSString *gSongbirdProfilePath = nil;

static NSString * 
GetSongbirdProfilePath()
{
  if (!gSongbirdProfilePath) {
    NSMutableString *profilePath = [[NSMutableString alloc] init];
    
    FSRef appSupportFolderRef;
    OSErr err = ::FSFindFolder(kUserDomain,
                               kApplicationSupportFolderType,
                               kCreateFolder,
                               &appSupportFolderRef);
    if (err != noErr) {
      return profilePath;
    }

    // Convert to a CFURL to get the file path
    CFURLRef folderUrlRef;
    folderUrlRef = CFURLCreateFromFSRef(kCFAllocatorDefault, 
                                        &appSupportFolderRef);
    if (!folderUrlRef) {
      return profilePath;
    }

    [profilePath appendFormat:@"%@/%s/", 
      [(NSURL *)folderUrlRef path],
      STRINGIZE(SB_APPNAME) STRINGIZE(SB_PROFILE_VERSION)];
    
    CFRelease(folderUrlRef);
    gSongbirdProfilePath = [[NSString alloc] initWithString:profilePath];
  }

  return gSongbirdProfilePath;
}

static NSURL *
GetSongbirdAgentURL()
{
  NSString *agentBundleID = @"org.songbirdnest.songbirditunesagent";
  NSString *agentPath = 
    [[NSWorkspace sharedWorkspace] absolutePathForAppBundleWithIdentifier:agentBundleID];
  NSURL *agentURL = [NSURL fileURLWithPath:agentPath];

  return agentURL;
}

//------------------------------------------------------------------------------

sbiTunesAgentProcessor* sbCreatesbiTunesAgentProcessor()
{
  return new sbiTunesAgentMacProcessor();
}

//------------------------------------------------------------------------------

sbiTunesAgentMacProcessor::sbiTunesAgentMacProcessor()
  : mLibraryMgr(new sbiTunesLibraryManager())
{
}

sbiTunesAgentMacProcessor::~sbiTunesAgentMacProcessor()
{
  delete mLibraryMgr;
}

std::string
sbiTunesAgentMacProcessor::GetNextTaskfilePath()
{
  std::string taskFilePath;
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  BOOL found = NO;
  NSString *curFilename = nil;
  NSString *exportFilename = 
    [NSString stringWithUTF8String:AGENT_EXPORT_FILENAME];
  NSString *songbirdProfilePath = GetSongbirdProfilePath();

  NSDirectoryEnumerator *dirEnum =
    [[NSFileManager defaultManager] enumeratorAtPath:GetSongbirdProfilePath()];
  
  while ((curFilename = [dirEnum nextObject])) {
    if ([curFilename rangeOfString:exportFilename].location != NSNotFound) {
      found = YES;
      break;
    }
  }

  if (found) {
    taskFilePath = [songbirdProfilePath UTF8String];
    taskFilePath.append([curFilename UTF8String]);
  }

  [pool release];
  return taskFilePath;
}

bool
sbiTunesAgentMacProcessor::GetIsItunesRunning()
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  
  bool isRunning = false;
  NSDictionary *curAppDict = nil;
  NSEnumerator *openAppsEnum = 
    [[[NSWorkspace sharedWorkspace] launchedApplications] objectEnumerator];
  while ((curAppDict = [openAppsEnum nextObject])) {
    NSString *curAppPath = 
      [curAppDict objectForKey:@"NSApplicationBundleIdentifier"];

    if ([curAppPath isEqualToString:@"com.apple.iTunes"]) {
      isRunning = true;
      break;
    }
  }

  [pool release];
  return isRunning;
}

//------------------------------------------------------------------------------
// sbiTunesAgentProcessor

bool
sbiTunesAgentMacProcessor::TaskFileExists()
{
  std::string nextTaskFilepath = GetNextTaskfilePath();
  return !nextTaskFilepath.empty();
}

void
sbiTunesAgentMacProcessor::RemoveTaskFile()
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  // Remove the current agent task file.
  NSString *currentTaskPath = 
    [NSString stringWithUTF8String:mCurrentTaskFile.c_str()];

  [[NSFileManager defaultManager] removeFileAtPath:currentTaskPath
                                             handler:nil];
  [pool release];
}

sbError
sbiTunesAgentMacProcessor::WaitForiTunes()
{
  while (!GetIsItunesRunning()) {
    // This method needs to block for now.
    Sleep(AGENT_ITUNES_SLEEP_INTERVAL);
  }
  
  return sbNoError;
}

bool
sbiTunesAgentMacProcessor::ErrorHandler(sbError const & aError)
{
  // todo write me!
  return true;
}

sbError
sbiTunesAgentMacProcessor::RegisterForStartOnLogin()
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  // Always clean previous entries first.
  sbError error = UnregisterForStartOnLogin();
  SB_ENSURE_SUCCESS(error, error);

  NSURL *agentURL = GetSongbirdAgentURL();
  if (!agentURL) {
    [pool release];
    return sbError("Could not get the agent URL!");
  }

  OSStatus err = LIAEAddURLAtEnd((CFURLRef)agentURL, true);
  if (err != noErr) {
    [pool release];
    return sbOSStatusError("Could not add URL to startup items list!", err);
  }

  [pool release];
  return sbNoError;
}

sbError
sbiTunesAgentMacProcessor::UnregisterForStartOnLogin()
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  NSArray *loginItems = nil;
  OSStatus err = LIAECopyLoginItems((CFArrayRef *) &loginItems);
  if (err != noErr) {
    return sbOSStatusError("Could not get a refernce to the login items!", err);
  }

  NSURL *agentURL = GetSongbirdAgentURL();
  if (!agentURL) {
    return sbError("Could not get the agent URL!");
  }

  unsigned int index = 0;
  NSDictionary *curLoginItemDict = nil;
  NSEnumerator *loginItemsEnum = [loginItems objectEnumerator];
  while ((curLoginItemDict = [loginItemsEnum nextObject])) {
    NSURL *curLoginItemURL = 
      [curLoginItemDict objectForKey:(NSString *)kLIAEURL];
    NSRange agentRange =
      [[curLoginItemURL path] rangeOfString:@"songbirditunesagent"];
    
    if ([curLoginItemURL isEqualTo:agentURL] || 
        agentRange.location != NSNotFound) 
    {
      err = LIAERemove(index);
      if (!err != noErr) {
        [pool release];
        return sbOSStatusError("Could not remove item from startup list!", err);
      }
      
      break;
    }

    index++;
  }

  [pool release];
  return sbNoError;
}

sbError
sbiTunesAgentMacProcessor::AddTracks(std::string const & aSource,
                                     Tracks const & aPaths)
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  sbError error;
  sbiTunesPlaylist *sourceList = NULL;

  std::string mainLibraryName;
  error = mLibraryMgr->GetMainLibraryPlaylistName(mainLibraryName);
  SB_ENSURE_SUCCESS(error, error);

  bool shouldDeleteList = true;
  if (mainLibraryName.compare(aSource) == 0) {
    error = mLibraryMgr->GetMainLibraryPlaylist(&sourceList);
    SB_ENSURE_SUCCESS(error, error);

    // don't delete this list
    shouldDeleteList = false;
  }
  else {
    // This source list isn't the main library, add these items to the 
    // main library.
    error = mLibraryMgr->GetSongbirdPlaylist(aSource, &sourceList);
    SB_ENSURE_SUCCESS(error, error);
  }

  error = mLibraryMgr->AddTrackPaths(aPaths, sourceList);
  SB_ENSURE_SUCCESS(error, error);

  if (shouldDeleteList) {
    delete sourceList;
  }

  [pool release];
  return sbNoError;
}

sbError
sbiTunesAgentMacProcessor::RemovePlaylist(std::string const & aPlaylistName)
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  
  sbError error;
  sbiTunesPlaylist *playlist = NULL;
  error = mLibraryMgr->GetSongbirdPlaylist(aPlaylistName, &playlist);
  SB_ENSURE_SUCCESS(error, error);

  error = mLibraryMgr->DeleteSongbirdPlaylist(playlist);
  SB_ENSURE_SUCCESS(error, error);

  [pool release];
  return sbNoError;
}

sbError
sbiTunesAgentMacProcessor::CreatePlaylist(std::string const & aPlaylistName)
{
  return mLibraryMgr->CreateSongbirdPlaylist(aPlaylistName);
}

bool
sbiTunesAgentMacProcessor::OpenTaskFile(std::ifstream & aStream)
{
  std::string taskFilePath(GetNextTaskfilePath());
  if (taskFilePath.empty()) {
    return false;
  }

  mCurrentTaskFile = taskFilePath;
  aStream.open(mCurrentTaskFile.c_str());
  return true;
}

void
sbiTunesAgentMacProcessor::Log(std::string const & aMsg)
{
  if (mLogState != DEACTIVATED) {
    if (mLogState != OPENED) {
      std::string logPath = [GetSongbirdProfilePath UTF8String];
      logPath += AGENT_LOG_FILENAME;
      mLogStream.open(logPath.c_str());
      // If we can't open then don't bother trying again
      mLogState = mLogStream ? OPENED : DEACTIVATED;
    }
    mLogStream << aMsg << std::endl;
  }
}

bool
sbiTunesAgentMacProcessor::ShouldShutdown()
{
  // No cleanup needed just yet.
  return false;
}

void
sbiTunesAgentMacProcessor::Sleep(unsigned long aMilliseconds)
{
  // usleep() takes microseconds (x1000)
  usleep(aMilliseconds * 1000);
}

void
sbiTunesAgentMacProcessor::ShutdownDone()
{
  // No cleanup needed just yet.
}

