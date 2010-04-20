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
 *   Nick Kreeger <kreeger@songbirdnest.com>
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

#include "macutils.h"

// disthelper includes
#include "error.h"
#include "debug.h"
#include "commands.h"

// Mac OS X includes
#include <ApplicationServices/ApplicationServices.h>


//------------------------------------------------------------------------------
// Internal macutil functions

NSString*
GetBundlePlistPath() {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  NSString *resourcesDir =
    [NSString stringWithUTF8String:GetAppResoucesDirectory().c_str()];

  NSString *retVal =
    [[NSString alloc] initWithFormat:@"%@/%@",
       [resourcesDir stringByDeletingLastPathComponent], @"Info.plist"];

  [pool release];

  return [retVal autorelease];
}

//------------------------------------------------------------------------------
// Public macutil functions

int
UpdateInfoPlistKey(NSString *aKey, NSString *aValue) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  NSString *plistFile = GetBundlePlistPath();
  NSMutableDictionary *plistDict =
    [NSMutableDictionary dictionaryWithContentsOfFile:plistFile];

  // First, update the property in the plist.
  [plistDict setValue:aValue forKey:aKey];

  // Next, flush the updated plist file to disk.
  [plistDict writeToFile:plistFile atomically:NO];

  // Finally, update the LaunchServices database to catch the property change.
  // Since, |GetAppResoucesDirectory()| returns the path
  // "../MyApp.app/Contents/Resources/", work our way to the root ".app" folder.
  NSString *targetDir =
    [NSString stringWithUTF8String:GetAppResoucesDirectory().c_str()];
  targetDir = [targetDir stringByDeletingLastPathComponent];
  targetDir = [targetDir stringByDeletingLastPathComponent];
  NSURL *bundleURL = [NSURL URLWithString:targetDir];
  OSStatus status = LSRegisterURL((CFURLRef)bundleURL, true);
  if (status != noErr) {
#if DEBUG
    DebugMessage("Could not update the LaunchServices registry!");
#endif
    [pool release];
    return DH_ERROR_USER;
  }

  [pool release];
  return DH_ERROR_OK;
}

