/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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

#include "sbiTunesAgentMacProcessor.h"

#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>
#import <CoreFoundation/CoreFoundation.h>
#import "SBNSString+Utils.h"
#import "SBNSWorkspace+Utils.h"
#include <sys/param.h>
#include <sstream>
#include "LoginItemsAE.h"

// Agent constants, these should be unified soon.
#define AGENT_EXPORT_FILENAME    "songbird_export.task"
#define AGENT_ERROR_FILENAME     "itunesexporterrors.txt"
#define AGENT_LOG_FILENAME       "itunesexport.log"
#define AGENT_RESULT_FILENAME    "itunesexportresults.txt"
#define AGENT_SHUTDOWN_FILENAME  "songbird_export.shutdown"
#define AGENT_ERROR_FILENAME     "itunesexporterrors.txt"

#define AGENT_ITUNES_SLEEP_INTERVAL 5000

static const NSString *kAgentBundleId =
  [[NSString alloc] initWithFormat:@"%s.%s", STRINGIZE(SB_APP_BUNDLE_BASENAME),
                                       STRINGIZE(SB_SIMPLE_PROGRAM)];


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
  NSURL *agentURL = nil;
  NSWorkspace *workspace = [NSWorkspace sharedWorkspace];
  NSString *agentPath = 
    [workspace absolutePathForAppBundleWithIdentifier:kAgentBundleId];

  if (!agentPath) {
    // If no path is returned for the agent bundle identifier, it's because 
    // the agent has not been registered with Launch Services yet.
    // This usually happens because the Finder does not look for additional
    // applications inside of another .app. To fix this, simply register
    // the path to the agent based on the parent apps path.
    NSString *parentAppBundleID = 
      [NSString stringWithFormat:@"%s.songbird", 
                                 STRINGIZE(SB_APP_BUNDLE_BASENAME)];

    NSString *songbirdURL =
      [workspace absolutePathForAppBundleWithIdentifier:parentAppBundleID];

    // Build the path to the agent.
    NSString *agentPath = 
      [NSString stringWithFormat:@"%@/Contents/Resources/%s.app",
                                 songbirdURL,
                                 STRINGIZE(SB_SIMPLE_PROGRAM)];
    // Now register with LS
    agentURL = [NSURL fileURLWithPath:agentPath];
    LSRegisterURL((CFURLRef)agentURL, true);
  }
  else {
    agentURL = [NSURL fileURLWithPath:agentPath];
  }

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

sbError
sbiTunesAgentMacProcessor::KillAllAgents()
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  NSString *agentName = 
    [NSString stringWithUTF8String:STRINGIZE(SB_SIMPLE_PROGRAM)];
  [NSWorkspace killAllRunningProcesses:agentName];
  
  [pool release];
  return sbNoError;
}

sbError
sbiTunesAgentMacProcessor::ProcessTaskFile()
{
  // Setup the library manager now.
  sbError error = mLibraryMgr->Init();
  SB_ENSURE_SUCCESS(error, error);

  return sbiTunesAgentProcessor::ProcessTaskFile();
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
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        
  std::string path([GetSongbirdProfilePath() UTF8String]);
  path += AGENT_ERROR_FILENAME;

  if (aError == sbiTunesNotRunningError) {
    // If this message is received, the user shutdown iTunes during an export
    // operation. Wait for iTunes to pop back up before trying to send any
    // more data.
    WaitForiTunes();

    // Since iTunes quit in the middle of our expor operation, all of the 
    // descriptors for resources (like playlists) are now invalid, reload 
    // the data with the library manager before pressing on.
    sbError error = mLibraryMgr->ReloadManager();
    if (error != sbNoError) {
      // Something really bad has gone on, just kill the agent now.
      std::ofstream errorStream(path.c_str());
      errorStream << "ERROR: Could not reload the Songbird data!" << std::endl
                  << "  - MESSAGE: " << error.Message() << std::endl;
      [pool release];
      return false;
    }
  }
  else {
    std::ofstream errorStream(path.c_str());
    if (errorStream) {
      errorStream << "ERROR: " << aError.Message() << std::endl;
    }
  }

  [pool release];
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
    return sbOSStatusError("Could not get a reference to the login items!", err);
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
      if (err != noErr) {
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

bool
sbiTunesAgentMacProcessor::GetIsAgentRunning()
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  NSString *agentName = 
    [NSString stringWithUTF8String:STRINGIZE(SB_SIMPLE_PROGRAM)];
  bool retval = [NSWorkspace isProcessAlreadyRunning:agentName];
  
  [pool release];
  return retval;
}

struct __attribute__ ((visibility ("hidden") )) PathCopier {
  std::string operator()(const sbiTunesAgentProcessor::Track t) {
    return t.path;
  }
};

sbError
sbiTunesAgentMacProcessor::AddTracks(std::string const & aSource,
                                     TrackList const & aPaths)
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  sbError error;
  sbiTunesPlaylist *sourceList = NULL;

  if (aSource.compare(SONGBIRD_MAIN_LIBRARY_NAME) == 0) {
    error = mLibraryMgr->GetMainLibraryPlaylist(&sourceList);
    SB_ENSURE_SUCCESS(error, error);
  }
  else {
    // This source list isn't the main library, add these items to the 
    // main library.
    error = mLibraryMgr->GetSongbirdPlaylist(aSource, &sourceList);
    SB_ENSURE_SUCCESS(error, error);
  }

  PathCopier copier;
  std::deque<std::string> paths, dbIds;
  std::insert_iterator<std::deque<std::string> > inserter(paths, paths.begin());
  std::transform(aPaths.begin(), aPaths.end(), inserter, copier);
  error = mLibraryMgr->AddTrackPaths(sourceList, paths, dbIds);

  if (!OpenResultsFile()) {
    [pool release];
    return sbError("Failed to open output file");
  }

  std::string libraryId;
  error = mLibraryMgr->GetLibraryId(libraryId);
  SB_ENSURE_SUCCESS(error, error);

  TrackList::const_iterator trackIter = aPaths.begin();
  std::deque<std::string>::const_iterator idIter;
  for (idIter = dbIds.begin();
       idIter != dbIds.end();
       ++idIter, ++trackIter) {
    OpenResultsFile() << trackIter->id
                      << "="
                      << libraryId
                      << ","
                      << *idIter
                      << std::endl;
  }

  [pool release];
  return error;
}

sbError
sbiTunesAgentMacProcessor::UpdateTracks(TrackList const & aPaths)
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  sbError error(sbNoError);

  sbiTunesLibraryManager::TrackPropertyList tracks;
  TrackList::const_iterator const end = aPaths.end();
  for (TrackList::const_iterator iter = aPaths.begin(); iter != end; ++iter) {
    unsigned long long id;
    int count = 0;
    assert(iter->type == Track::TYPE_ITUNES);
    count = sscanf(iter->id.c_str(), "%llx", &id);
    if (count < 1) {
      std::string msg("Unable to parse itunes ID ");
      msg.append(iter->id);
      return sbError(msg);
    }
    tracks.push_back(sbiTunesLibraryManager::TrackProperty(id, iter->path));
  }
  return mLibraryMgr->UpdateTracks(tracks);

  [pool release];
  return error;
}

sbError
sbiTunesAgentMacProcessor::RemovePlaylist(std::string const & aPlaylistName)
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  
  sbError error;
  sbiTunesPlaylist *playlist = NULL;
  error = mLibraryMgr->GetSongbirdPlaylist(aPlaylistName, &playlist);
  if (error == sbNoError) {
    error = mLibraryMgr->DeleteSongbirdPlaylist(playlist);
  }

  [pool release];
  return error;
}

sbError
sbiTunesAgentMacProcessor::CreatePlaylist(std::string const & aPlaylistName)
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  // Only create a playlist if it doesn't already exist.
  sbError error;
  sbiTunesPlaylist *playlist;
  error = mLibraryMgr->GetSongbirdPlaylist(aPlaylistName, &playlist);
  if (error != sbNoError) {
    error = mLibraryMgr->CreateSongbirdPlaylist(aPlaylistName);
  }

  [pool release];
  return error;
}

sbError
sbiTunesAgentMacProcessor::ClearPlaylist(std::string const & aListName)
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  // Simply remove and create a new playlist.
  sbError error;
  error = RemovePlaylist(aListName);
  SB_ENSURE_SUCCESS(error, error);

  error = CreatePlaylist(aListName);
  SB_ENSURE_SUCCESS(error, error);

  [pool release];
  return sbNoError;
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

std::ofstream &
sbiTunesAgentMacProcessor::OpenResultsFile()
{
  if (!!mOutputStream && !mOutputStream.is_open()) {
    std::string path;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    NSString *songbirdProfilePath = GetSongbirdProfilePath();

    path = [songbirdProfilePath UTF8String];
    path.append(AGENT_RESULT_FILENAME);

    [pool release];
    mOutputStream.open(path.c_str(), std::ios_base::ate);
  }
  return mOutputStream;
}


void
sbiTunesAgentMacProcessor::Log(std::string const & aMsg)
{
  if (mLogState != DEACTIVATED) {
    if (mLogState != OPENED) {
      std::string logPath = [GetSongbirdProfilePath() UTF8String];
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

