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

#include <mozilla/ModuleUtils.h>
#include "sbFileSystemCID.h"
#include "sbFileSystemNode.h"

#if defined(XP_WIN) 
#include "win32/sbWin32FileSystemWatcher.h"
NS_GENERIC_FACTORY_CONSTRUCTOR(sbWin32FileSystemWatcher);
NS_DEFINE_NAMED_CID(SONGBIRD_FILESYSTEMWATCHER_CID);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbFileSystemNode);
NS_DEFINE_NAMED_CID(SONGBIRD_FILESYSTEMNODE_CID);

static const mozilla::Module::CIDEntry kFileSystemCIDs[] = {
  { &kSONGBIRD_FILESYSTEMWATCHER_CID, false, NULL, sbWin32FileSystemWatcherConstructor },
  { &kSONGBIRD_FILESYSTEMNODE_CID, false, NULL, sbFileSystemNodeConstructor },
  { NULL }
};

#elif defined(XP_MACOSX)
#include "macosx/sbMacFileSystemWatcher.h"
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMacFileSystemWatcher);
NS_DEFINE_NAMED_CID(SONGBIRD_FILESYSTEMWATCHER_CID);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbFileSystemNode);
NS_DEFINE_NAMED_CID(SONGBIRD_FILESYSTEMNODE_CID);

static const mozilla::Module::CIDEntry kFileSystemCIDs[] = {
  { &kSONGBIRD_FILESYSTEMWATCHER_CID, false, NULL, sbMacFileSystemWatcherConstructor },
  { &kSONGBIRD_FILESYSTEMNODE_CID, false, NULL, sbFileSystemNodeConstructor },
  { NULL }
};

#else
#include "linux/sbLinuxFileSystemWatcher.h"
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLinuxFileSystemWatcher);
NS_DEFINE_NAMED_CID(SONGBIRD_FILESYSTEMWATCHER_CID);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbFileSystemNode);
NS_DEFINE_NAMED_CID(SONGBIRD_FILESYSTEMNODE_CID);

static const mozilla::Module::CIDEntry kFileSystemCIDs[] = {
  { &kSONGBIRD_FILESYSTEMWATCHER_CID, false, NULL, sbLinuxFileSystemWatcherConstructor },
  { &kSONGBIRD_FILESYSTEMNODE_CID, false, NULL, sbFileSystemNodeConstructor },
  { NULL }
};
#endif

static const mozilla::Module::ContractIDEntry kFileSystemContracts[] = {
  { SONGBIRD_FILESYSTEMWATCHER_CONTRACTID, &kSONGBIRD_FILESYSTEMWATCHER_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kFileSystemCategories[] = {
  { NULL }
};

static const mozilla::Module kFileSystemModule = {
  mozilla::Module::kVersion,
  kFileSystemCIDs,
  kFileSystemContracts,
  kFileSystemCategories
};

NSMODULE_DEFN(sbFileSystem) = &kFileSystemModule;
