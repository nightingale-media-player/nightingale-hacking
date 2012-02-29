/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Songbird Distribution Stub Helper.
 *
 * The Initial Developer of the Original Code is
 * POTI <http://www.songbirdnest.com/>.
 * Portions created by the Initial Developer are Copyright (C) 2008-2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Mook <mook@songbirdnest.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "commands.h"
#include "debug.h"
#include "error.h"
#include "macutils.h"
#include "stringconvert.h"
#include "tchar_compat.h"
#include "utils.h"

// unix include
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

// mac includes
#include <AvailabilityMacros.h>
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>


#define MAX_LONG_PATH 0x8000 /* 32767 + 1, maximum size for \\?\ style paths */

#define NS_ARRAY_LENGTH(x) (sizeof(x) / sizeof(x[0]))

tstring ResolvePathName(std::string aSrc) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  NSString *src = [NSString stringWithUTF8String:aSrc.c_str()];
  tstring result;
  #if DEBUG
    DebugMessage("Resolving path name %s", [src UTF8String]);
  #endif
  if (0 != [src length]) {
    if ([src hasPrefix:@"$/"]) {
      NSString *appDir =
        [NSString stringWithUTF8String:GetAppResoucesDirectory().c_str()];
      src = [appDir stringByAppendingString:[src substringFromIndex:2]];
    }
    if (![src isAbsolutePath]) {
      NSString *distIniDir =
        [NSString stringWithUTF8String:GetDistIniDirectory().c_str()];
      NSString * resolved = [distIniDir stringByAppendingPathComponent:src];
      if ([[NSFileManager defaultManager] fileExistsAtPath:src]) {
        src = resolved;
      }
      else {
        DebugMessage("Failed to resolve path name %s", [src UTF8String]);
      }
    }
    result.assign([src UTF8String]);
  }
  [pool release];
  #if DEBUG
    DebugMessage("Resolved path name %s", result.c_str());
  #endif
  return result;
}

static bool isDirectoryEmpty(std::string aPath) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  NSString *path = [NSString stringWithUTF8String:aPath.c_str()];

  NSDirectoryEnumerator *enumerator =
    [[NSFileManager defaultManager] enumeratorAtPath:path];

  NSString *child = [enumerator nextObject];
  bool hasChild = (child != nil);
  [pool release];
  return !hasChild;
}

#ifndef kCFCoreFoundationVersionNumber10_5
  @interface fileManErrorHandler : NSObject {
  }
  +(fileManErrorHandler*) handler;
  @end

  @implementation fileManErrorHandler
  +(fileManErrorHandler*) handler {
          return [[[fileManErrorHandler alloc] init] autorelease];
  }
  -(BOOL)fileManager:(NSFileManager *)manager
          shouldProceedAfterError:(NSDictionary *)errorInfo
  {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    NSString *toPath = [errorInfo objectForKey:@"ToPath"];
    if (toPath == nil) {
      toPath = [NSString stringWithString:@"<no ToPath>"];
    }

    DebugMessage("fileManager error handler: %s -> %s : %s",
                 [[errorInfo objectForKey:@"Path"] UTF8String],
                 [toPath UTF8String],
                 [[errorInfo objectForKey:@"Error"] UTF8String]);

    [pool release];
    return NO;
  }
  @end
#endif /* !kCFCoreFoundationVersionNumber10_5 */

int CommandCopyFile(std::string aSrc, std::string aDest, bool aRecursive) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  NSString *srcPath =
    [NSString stringWithUTF8String:ResolvePathName(aSrc).c_str()];
  NSString *destPath =
    [NSString stringWithUTF8String:ResolvePathName(aDest).c_str()];
  BOOL success, isDir;
  NSError *error = nil;
  NSFileManager* fileMan = [NSFileManager defaultManager];

  // the APIs we use don't copy the leaf name for us; we need to do this
  // manually.
  if (![fileMan fileExistsAtPath:destPath isDirectory:&isDir]) {
    isDir = NO;
  }
  if (isDir) {
   destPath =
     [destPath stringByAppendingPathComponent:[srcPath lastPathComponent]];
  }

  if (![fileMan fileExistsAtPath:srcPath isDirectory:&isDir]) {
    DebugMessage("ERROR: [%s] does not exist, cannot copy",
                 [srcPath UTF8String]);
    [pool release];
    return DH_ERROR_READ;
  }

  if ([srcPath isEqualToString:destPath]) {
    DebugMessage("Skipping copy of %s unto itself",
                 [srcPath UTF8String]);
    return DH_ERROR_OK;
  }

  DebugMessage("copying %s to %s...",
               [srcPath UTF8String],
               [destPath UTF8String]);

  // XXXMook: ugly hack to check if we're using the 10.5 SDK or higher (that is,
  // a SDK that knows about 10.5 methods).
  #ifdef kCFCoreFoundationVersionNumber10_5
    if ([fileMan fileExistsAtPath:destPath]) {
      [fileMan removeItemAtPath:destPath error:NULL];
    }
    if (isDir && !aRecursive) {
      // we don't want to recurse, but this is a directory; just create a new,
      // empty one instead
      NSDictionary* attribs = [fileMan attributesOfItemAtPath:srcPath
                                                        error:NULL];
      success = [fileMan createDirectoryAtPath:destPath
                   withIntermediateDirectories:YES
                                    attributes:attribs
                                         error:NULL];
    }
    else {
      success = [fileMan copyItemAtPath:srcPath
                                 toPath:destPath
                                  error:&error];
    }
  #else
    if ([fileMan fileExistsAtPath:destPath]) {
      [fileMan removeFileAtPath:destPath handler:nil];
    }
    if (isDir && !aRecursive) {
      // we don't want to recurse, but this is a directory; just create a new,
      // empty one instead
      NSDictionary* attribs = [fileMan fileAttributesAtPath:srcPath
                                               traverseLink:YES];
      success = [fileMan createDirectoryAtPath:destPath
                                    attributes:attribs];
    }
    else {
      // copyPath:toPath:handler: will bail if the destination already exists
      success = [fileMan copyPath:srcPath
                           toPath:destPath
                          handler:[fileManErrorHandler handler]];
    }
  #endif

  [pool release];

  return (success && !error ? DH_ERROR_OK : DH_ERROR_WRITE);
}

int CommandMoveFile(std::string aSrc, std::string aDest, bool aRecursive) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  NSString *srcPath =
    [NSString stringWithUTF8String:ResolvePathName(aSrc).c_str()];
  NSString *destPath =
    [NSString stringWithUTF8String:ResolvePathName(aDest).c_str()];
  BOOL success, isDir;
  NSError *error = nil;
  NSFileManager* fileMan = [NSFileManager defaultManager];

  if (![fileMan fileExistsAtPath:srcPath isDirectory:&isDir]) {
    DebugMessage("cannot move non-existent file %s", [srcPath UTF8String]);
    [pool release];
    return DH_ERROR_READ;
  }
  if (!aRecursive && isDir) {
    DebugMessage("We can't non-recursively move the directory %s to %s!",
                 aSrc.c_str(), aDest.c_str());
    [pool release];
    return DH_ERROR_USER;
  }

  // the APIs we use don't copy the leaf name for us; we need to do this
  // manually.
  if (![fileMan fileExistsAtPath:destPath isDirectory:&isDir]) {
    isDir = NO;
  }
  if (isDir) {
   destPath =
     [destPath stringByAppendingPathComponent:[srcPath lastPathComponent]];
  }

  DebugMessage("moving %s to %s...",
               [srcPath UTF8String],
               [destPath UTF8String]);

  // XXXMook: ugly hack to check if we're using the 10.5 SDK or higher (that is,
  // a SDK that knows about 10.5 methods).
  #ifdef kCFCoreFoundationVersionNumber10_5
    success = [fileMan moveItemAtPath:srcPath
                               toPath:destPath
                                error:&error];
  #else
    success = [fileMan movePath:srcPath
                         toPath:destPath
                        handler:[fileManErrorHandler handler]];
  #endif

  [pool release];

  return (success && !error ? DH_ERROR_OK : DH_ERROR_WRITE);
}

int CommandDeleteFile(std::string aFile, bool aRecursive) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  NSString *path =
    [NSString stringWithUTF8String:ResolvePathName(aFile).c_str()];
  BOOL success, isDir;
  NSError *error = nil;
  NSFileManager* fileMan = [NSFileManager defaultManager];

  success = [fileMan fileExistsAtPath:path isDirectory:&isDir];
  if (!success) {
    DebugMessage("File %s does not exist, cannot delete", [path UTF8String]);
    [pool release];
    return DH_ERROR_OK;
  }

  if (isDir && !aRecursive && !isDirectoryEmpty(aFile)) {
    LogMessage("Failed to recursively delete %s, not empty", aFile.c_str());
    [pool release];
    return DH_ERROR_WRITE;
  }

  // XXXMook: ugly hack to check if we're using the 10.5 SDK or higher (that is,
  // a SDK that knows about 10.5 methods).
  #ifdef kCFCoreFoundationVersionNumber10_5
    success = [fileMan removeItemAtPath:path
                                  error:&error];
  #else
    success = [fileMan removeFileAtPath:path
                                handler:nil];
  #endif

  [pool release];

  return (success && !error ? DH_ERROR_OK : DH_ERROR_WRITE);
}

int CommandExecuteFile(const std::string& aExecutable,
                       const std::string& aArgs) {
  tstring executable(FilterSubstitution(ConvertUTF8toUTFn(aExecutable)));
  tstring arg(FilterSubstitution(ConvertUTF8toUTFn(aArgs)));

  DebugMessage("<%s>",
               ConvertUTFnToUTF8(arg).c_str());
  int result = system(arg.c_str());
  return (result ? DH_ERROR_UNKNOWN : DH_ERROR_OK);
}

std::vector<std::string> ParseCommandLine(const std::string& aCommandLine) {
  static const char WHITESPACE[] = " \t\r\n";
  std::vector<std::string> args;
  std::string::size_type prev = 0, offset;
  offset = aCommandLine.find_last_not_of(WHITESPACE);
  if (offset == std::string::npos) {
    // there's nothing that's not whitespace, don't bother
    return args;
  }
  std::string commandLine = aCommandLine.substr(0, offset + 1);
  std::string::size_type length = commandLine.length();
  do {
    prev = commandLine.find_first_not_of(WHITESPACE, prev);
    if (prev == std::string::npos) {
      // nothing left that's not whitespace
      break;
    }
    if (commandLine[prev] == '"') {
      // start of quoted param
      ++prev; // eat the quote
      offset = commandLine.find('"', prev);
      if (offset == std::string::npos) {
        // no matching end quote; assume it lasts to the end of the command
        offset = commandLine.length();
      }
    } else {
      // unquoted
      offset = commandLine.find_first_of(WHITESPACE, prev);
      if (offset == std::string::npos) {
        offset = commandLine.length();
      }
    }
    args.push_back(commandLine.substr(prev, offset - prev));
    prev = offset + 1;
  } while (prev < length);

  return args;
}

void ParseExecCommandLine(const std::string& aCommandLine,
                          std::string&       aExecutable,
                          std::string&       aArgs)
{
  // we don't need to do anything funny here; only Windows needs that
  aExecutable.clear();
  aArgs = aCommandLine;
}

tstring
GetAppResoucesDirectory() {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  NSProcessInfo* process = [NSProcessInfo processInfo];
  NSString* execPath = [[process arguments] objectAtIndex:0];
  NSString* dirPath = [execPath stringByDeletingLastPathComponent];
  dirPath = [dirPath stringByAppendingString:@"/"];
#if DEBUG
  DebugMessage("Found app directory %s", [dirPath UTF8String]);
#endif
  tstring retval([dirPath UTF8String]);
  [pool release];
  return retval; 
}

tstring gDistIniDirectory;
tstring GetDistIniDirectory(const TCHAR *aPath) {
  if (aPath) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    NSString * path = [NSString stringWithUTF8String:aPath];

    if (![path isAbsolutePath]) {
      // given path is not absolute; this is relative to the appdir
      NSProcessInfo* process = [NSProcessInfo processInfo];
      NSString * execPath = [[process arguments] objectAtIndex:0];
      NSString * appDir = [execPath stringByDeletingLastPathComponent];
      path = [appDir stringByAppendingPathComponent:path];
    }

    NSFileManager * fileMan = [NSFileManager defaultManager];
    // if the given file doesn't exist, bail (because there are no actions)
    if (![fileMan fileExistsAtPath:path isDirectory:NULL]) {
      DebugMessage("File %s doesn't exist, bailing", [path UTF8String]);
      [pool release];
      return tstring(_T(""));
    }

    // now remove the file name
    path = [path stringByDeletingLastPathComponent];

    #if DEBUG
      DebugMessage("found distribution path %s", [path UTF8String]);
    #endif
    gDistIniDirectory = tstring([path UTF8String]);
    // we expect a path separator at the end of the string
    gDistIniDirectory.append(_T("/"));
    [pool release];
  }
  return gDistIniDirectory;
}

tstring GetLeafName(tstring aSrc) {
  tstring leafName;
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  NSString *src = [NSString stringWithUTF8String:aSrc.c_str()];
  leafName.assign([[src lastPathComponent] UTF8String]);
  [pool release];
  return leafName;
}

void ShowFatalError(const char* fmt, ...) {
  va_list args;

  // Rename application.ini since if we get to this point the app is borked
  // and we'd really rather the user attempt to uninstall/reinstall instead of
  // trying to use a half-updated (and likely crashy) app.
  tstring appIni = ResolvePathName("$/application.ini");
  tstring bakIni = ResolvePathName("$/broken.application.ini");
  unlink(bakIni.c_str());
  rename(appIni.c_str(), bakIni.c_str());

  // log the fact that the fatal error occurred into the log file
  va_start(args, fmt);
  vLogMessage(fmt, args);
  va_end(args);

  if (_tgetenv(_T("DISTHELPER_SILENT_FAILURE"))) {
    // we should be silent (perhaps this is a unit test?), don't show the dialog
    return;
  }

  TCHAR *buffer;

  // build the message string (prefixing it with a generic fatal message)
  tstring msg("An application update error has occurred; please re-install "
              "the application.  Your media has not been affected.\n\n"
              "Related deatails:\n\n");
  msg.append(fmt);

  // build the output string and show it
  va_start(args, fmt);
  vasprintf(&buffer, fmt, args);
  va_end(args);

  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  NSAlert *alert = [[NSAlert alloc] init];
  [alert setMessageText:@"Update Distribution Helper"];
  [alert setInformativeText:[NSString stringWithUTF8String:buffer]];
  [alert runModal];
  [pool release];
  free(buffer);
}

int
ReplacePlistProperty(std::string & aKey, std::string & aValue) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  NSString *key = [NSString stringWithUTF8String:aKey.c_str()];
  NSString *value = [NSString stringWithUTF8String:aValue.c_str()];
#if DEBUG
  DebugMessage("Updating key '%s' with value '%s",
      [key UTF8String], [value UTF8String]);
#endif
  int result = UpdateInfoPlistKey(key, value);
  [pool release];

  return result;
}

int CommandSetIcon(std::string aExecutable,
                   std::string aIconFile,
                   std::string aIconName) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  NSString *newIconPath =
    [NSString stringWithUTF8String:ResolvePathName(aIconFile).c_str()];

  int result = UpdateInfoPlistKey(@"CFBundleIconFile",
                                  [newIconPath lastPathComponent]);

  [pool release];
  return result; 
}

