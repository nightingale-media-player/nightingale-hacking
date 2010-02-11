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

#include <nsIGenericFactory.h>
#include "sbTranscodeManager.h"
#include "sbTranscodeAlbumArt.h"
#include "sbTranscodeProfile.h"
#include "sbTranscodeProfileLoader.h"
#include "sbTranscodingConfigurator.h"

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(sbTranscodeManager,
        sbTranscodeManager::GetSingleton)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbTranscodeAlbumArt);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbTranscodeProfile);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbTranscodeProfileLoader);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbTranscodingConfigurator);

static nsModuleComponentInfo components[] =
{
  {
    SONGBIRD_TRANSCODEMANAGER_CLASSNAME,
    SONGBIRD_TRANSCODEMANAGER_CID,
    SONGBIRD_TRANSCODEMANAGER_CONTRACTID,
    sbTranscodeManagerConstructor
  },
  {
    SONGBIRD_TRANSCODEALBUMART_CLASSNAME,
    SONGBIRD_TRANSCODEALBUMART_CID,
    SONGBIRD_TRANSCODEALBUMART_CONTRACTID,
    sbTranscodeAlbumArtConstructor
  },
  {
    SONGBIRD_TRANSCODEPROFILE_CLASSNAME,
    SONGBIRD_TRANSCODEPROFILE_CID,
    SONGBIRD_TRANSCODEPROFILE_CONTRACTID,
    sbTranscodeProfileConstructor
  },
  {
    SONGBIRD_TRANSCODEPROFILELOADER_CLASSNAME,
    SONGBIRD_TRANSCODEPROFILELOADER_CID,
    SONGBIRD_TRANSCODEPROFILELOADER_CONTRACTID,
    sbTranscodeProfileLoaderConstructor
  },
  {
    SONGBIRD_TRANSCODINGCONFIGURATOR_CLASSNAME,
    SONGBIRD_TRANSCODINGCONFIGURATOR_CID,
    SONGBIRD_TRANSCODINGCONFIGURATOR_CONTRACTID,
    sbTranscodingConfiguratorConstructor
  }
};

PR_STATIC_CALLBACK(void)
DestroyModule(nsIModule* self)
{
  sbTranscodeManager::DestroySingleton();
}

NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(SongbirdTranscodeComponent,
                                   components,
                                   nsnull,
                                   DestroyModule)
