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

/**
 * \file  sbTranscodeModule.cpp
 * \brief Songbird Transcode Component Factory and Main Entry Point.
 */

#include <mozilla/ModuleUtils.h>
#include "sbTranscodeManager.h"
#include "sbTranscodeAlbumArt.h"
#include "sbTranscodeError.h"
#include "sbTranscodeProfile.h"
#include "sbTranscodeProfileLoader.h"
#include "sbTranscodingConfigurator.h"

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(sbTranscodeManager,
        sbTranscodeManager::GetSingleton);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbTranscodeAlbumArt);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbTranscodeError);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbTranscodeProfile);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbTranscodeProfileLoader);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbTranscodingConfigurator);

NS_DEFINE_NAMED_CID(SONGBIRD_TRANSCODEMANAGER_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_TRANSCODEALBUMART_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_TRANSCODEERROR_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_TRANSCODEPROFILE_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_TRANSCODEPROFILELOADER_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_TRANSCODINGCONFIGURATOR_CID);

static const mozilla::Module::CIDEntry kSongbirdTranscodeComponentCIDs[] = {
  { &kSONGBIRD_TRANSCODEMANAGER_CID, false, NULL, sbTranscodeManagerConstructor },
  { &kSONGBIRD_TRANSCODEALBUMART_CID, false, NULL, sbTranscodeAlbumArtConstructor },
  { &kSONGBIRD_TRANSCODEERROR_CID, false, NULL, sbTranscodeErrorConstructor },
  { &kSONGBIRD_TRANSCODEPROFILE_CID, false, NULL, sbTranscodeProfileConstructor },
  { &kSONGBIRD_TRANSCODEPROFILELOADER_CID, false, NULL, sbTranscodeProfileLoaderConstructor },
  { &kSONGBIRD_TRANSCODINGCONFIGURATOR_CID, false, NULL, sbTranscodingConfiguratorConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kSongbirdTranscodeComponentContracts[] = {
  { SONGBIRD_TRANSCODEMANAGER_CONTRACTID, &kSONGBIRD_TRANSCODEMANAGER_CID },
  { SONGBIRD_TRANSCODEALBUMART_CONTRACTID, &kSONGBIRD_TRANSCODEALBUMART_CID },
  { SONGBIRD_TRANSCODEERROR_CONTRACTID, &kSONGBIRD_TRANSCODEERROR_CID },
  { SONGBIRD_TRANSCODEPROFILE_CONTRACTID, &kSONGBIRD_TRANSCODEPROFILE_CID },
  { SONGBIRD_TRANSCODEPROFILELOADER_CONTRACTID, &kSONGBIRD_TRANSCODEPROFILELOADER_CID },
  { SONGBIRD_TRANSCODINGCONFIGURATOR_CONTRACTID, &kSONGBIRD_TRANSCODINGCONFIGURATOR_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kSongbirdTranscodeComponentCategories[] = {
  { NULL }
};

static const mozilla::Module kSongbirdTranscodeComponentModule = {
  mozilla::Module::kVersion,
  kSongbirdTranscodeComponentCIDs,
  kSongbirdTranscodeComponentContracts,
  kSongbirdTranscodeComponentCategories
};

NSMODULE_DEFN(SongbirdTranscodeComponent) = &kSongbirdTranscodeComponentModule;
