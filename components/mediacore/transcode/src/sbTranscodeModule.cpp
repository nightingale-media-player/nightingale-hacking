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

/**
 * \file  sbTranscodeModule.cpp
 * \brief Nightingale Transcode Component Factory and Main Entry Point.
 */

#include <nsIGenericFactory.h>
#include "sbTranscodeManager.h"
#include "sbTranscodeAlbumArt.h"
#include "sbTranscodeError.h"
#include "sbTranscodeProfile.h"
#include "sbTranscodeProfileLoader.h"
#include "sbTranscodingConfigurator.h"

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(sbTranscodeManager,
        sbTranscodeManager::GetSingleton)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbTranscodeAlbumArt);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbTranscodeError);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbTranscodeProfile);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbTranscodeProfileLoader);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbTranscodingConfigurator);

static nsModuleComponentInfo components[] =
{
  {
    NIGHTINGALE_TRANSCODEMANAGER_CLASSNAME,
    NIGHTINGALE_TRANSCODEMANAGER_CID,
    NIGHTINGALE_TRANSCODEMANAGER_CONTRACTID,
    sbTranscodeManagerConstructor
  },
  {
    NIGHTINGALE_TRANSCODEALBUMART_CLASSNAME,
    NIGHTINGALE_TRANSCODEALBUMART_CID,
    NIGHTINGALE_TRANSCODEALBUMART_CONTRACTID,
    sbTranscodeAlbumArtConstructor
  },
  {
    NIGHTINGALE_TRANSCODEERROR_CLASSNAME,
    NIGHTINGALE_TRANSCODEERROR_CID,
    NIGHTINGALE_TRANSCODEERROR_CONTRACTID,
    sbTranscodeErrorConstructor
  },
  {
    NIGHTINGALE_TRANSCODEPROFILE_CLASSNAME,
    NIGHTINGALE_TRANSCODEPROFILE_CID,
    NIGHTINGALE_TRANSCODEPROFILE_CONTRACTID,
    sbTranscodeProfileConstructor
  },
  {
    NIGHTINGALE_TRANSCODEPROFILELOADER_CLASSNAME,
    NIGHTINGALE_TRANSCODEPROFILELOADER_CID,
    NIGHTINGALE_TRANSCODEPROFILELOADER_CONTRACTID,
    sbTranscodeProfileLoaderConstructor
  },
  {
    NIGHTINGALE_TRANSCODINGCONFIGURATOR_CLASSNAME,
    NIGHTINGALE_TRANSCODINGCONFIGURATOR_CID,
    NIGHTINGALE_TRANSCODINGCONFIGURATOR_CONTRACTID,
    sbTranscodingConfiguratorConstructor
  }
};

PR_STATIC_CALLBACK(void)
DestroyModule(nsIModule* self)
{
  sbTranscodeManager::DestroySingleton();
}

NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(NightingaleTranscodeComponent,
                                   components,
                                   nsnull,
                                   DestroyModule)
