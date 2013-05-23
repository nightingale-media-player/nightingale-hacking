/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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

/** 
* \file  sbGStreamerMediacoreModule.cpp
* \brief Songbird GStreamer Mediacore Module Factory and Main Entry Point.
*/

#include <mozilla/ModuleUtils.h>

#include "sbGStreamerMediacoreCID.h"
#include "sbGStreamerMediacoreFactory.h"
#include "sbGStreamerMediaContainer.h"
#include "sbGStreamerMediaInspector.h"
#include "sbGStreamerService.h"
#include "sbGStreamerRTPStreamer.h"
#include "sbGStreamerTranscode.h"
#include "sbGStreamerVideoTranscode.h"
#include "sbGStreamerTranscodeDeviceConfigurator.h"
#include "sbGStreamerTranscodeAudioConfigurator.h"
#include "sbGStreamerAudioProcessor.h"
#include "metadata/sbGStreamerMetadataHandler.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbGStreamerService, Init);
NS_DEFINE_NAMED_CID(SBGSTREAMERSERVICE_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbGStreamerMediacoreFactory, Init);
NS_DEFINE_NAMED_CID(SB_GSTREAMERMEDIACOREFACTORY_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbGStreamerMetadataHandler, Init);
NS_DEFINE_NAMED_CID(SB_GSTREAMER_METADATA_HANDLER_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbGStreamerRTPStreamer, InitGStreamer);
NS_DEFINE_NAMED_CID(SB_GSTREAMERRTPSTREAMER_CID);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbGStreamerTranscode, InitGStreamer);
NS_DEFINE_NAMED_CID(SB_GSTREAMER_TRANSCODE_CID);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbGStreamerVideoTranscoder, InitGStreamer);
NS_DEFINE_NAMED_CID(SB_GSTREAMER_VIDEO_TRANSCODE_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbGStreamerMediaInspector, InitGStreamer);
NS_DEFINE_NAMED_CID(SB_GSTREAMER_MEDIAINSPECTOR_CID);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbGStreamerMediaContainer, Init);
NS_DEFINE_NAMED_CID(SB_GSTREAMER_MEDIACONTAINER_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(sbGStreamerTranscodeDeviceConfigurator);
NS_DEFINE_NAMED_CID(SB_GSTREAMER_TRANSCODE_DEVICE_CONFIGURATOR_CID);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbGStreamerTranscodeAudioConfigurator);
NS_DEFINE_NAMED_CID(SB_GSTREAMER_TRANSCODE_AUDIO_CONFIGURATOR_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbGStreamerAudioProcessor, InitGStreamer);
NS_DEFINE_NAMED_CID(SB_GSTREAMER_AUDIO_PROCESSOR_CID);


static const mozilla::Module::CIDEntry kGstreamerMediacoreCIDs[] = {
  { &kSBGSTREAMERSERVICE_CID, false, NULL, sbGStreamerServiceConstructor },
  { &kSB_GSTREAMERMEDIACOREFACTORY_CID, false, NULL, sbGStreamerMediacoreFactoryConstructor, },
  { &kSB_GSTREAMER_MEDIAINSPECTOR_CID, false, NULL, sbGStreamerMediaInspectorConstructor },
  { &kSB_GSTREAMER_MEDIACONTAINER_CID, false, NULL, sbGStreamerMediaContainerConstructor },
  { &kSB_GSTREAMER_METADATA_HANDLER_CID, false, NULL, sbGStreamerMetadataHandlerConstructor },
  { &kSB_GSTREAMERRTPSTREAMER_CID, false, NULL, sbGStreamerRTPStreamerConstructor },
  { &kSB_GSTREAMER_TRANSCODE_CID, false, NULL, sbGStreamerTranscodeConstructor },
  { &kSB_GSTREAMER_VIDEO_TRANSCODE_CID, false, NULL, sbGStreamerVideoTranscoderConstructor },
  { &kSB_GSTREAMER_TRANSCODE_AUDIO_CONFIGURATOR_CID, false, NULL, sbGStreamerTranscodeAudioConfiguratorConstructor },
  { &kSB_GSTREAMER_TRANSCODE_DEVICE_CONFIGURATOR_CID, false, NULL, sbGStreamerTranscodeDeviceConfiguratorConstructor },
  { &kSB_GSTREAMER_AUDIO_PROCESSOR_CID, false, NULL, sbGStreamerAudioProcessorConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kGstreamerMediacoreContacts[] = {
  { SBGSTREAMERSERVICE_CONTRACTID, &kSBGSTREAMERSERVICE_CID },
  { SB_GSTREAMERMEDIACOREFACTORY_CONTRACTID, &kSB_GSTREAMERMEDIACOREFACTORY_CID },
  { SB_GSTREAMER_MEDIAINSPECTOR_CONTRACTID, &kSB_GSTREAMER_MEDIAINSPECTOR_CID },
  { SB_GSTREAMER_MEDIACONTAINER_CONTRACTID, &kSB_GSTREAMER_MEDIACONTAINER_CID },
  { SB_GSTREAMER_METADATA_HANDLER_CONTRACTID, &kSB_GSTREAMER_METADATA_HANDLER_CID },
  { SB_GSTREAMERRTPSTREAMER_CONTRACTID, &kSB_GSTREAMERRTPSTREAMER_CID },
  { SB_GSTREAMER_TRANSCODE_CONTRACTID, &kSB_GSTREAMER_TRANSCODE_CID },
  { SB_GSTREAMER_VIDEO_TRANSCODE_CONTRACTID, &kSB_GSTREAMER_VIDEO_TRANSCODE_CID },
  { SB_GSTREAMER_TRANSCODE_AUDIO_CONFIGURATOR_CONTRACTID, &kSB_GSTREAMER_TRANSCODE_AUDIO_CONFIGURATOR_CID },
  { SB_GSTREAMER_TRANSCODE_DEVICE_CONFIGURATOR_CONTRACTID, &kSB_GSTREAMER_TRANSCODE_DEVICE_CONFIGURATOR_CID },
  { SB_GSTREAMER_AUDIO_PROCESSOR_CONTRACTID, &kSB_GSTREAMER_AUDIO_PROCESSOR_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kGstreamerMediacoreCategories[] = {
  { NULL }
};

static const mozilla::Module kGstreamerMediacoreModule = {
  mozilla::Module::kVersion,
  kGstreamerMediacoreCIDs,
  kGstreamerMediacoreContacts,
  kGstreamerMediacoreCategories
};

NSMODULE_DEFN(sbGstreamerMediacoreModule) = &kGstreamerMediacoreModule;
