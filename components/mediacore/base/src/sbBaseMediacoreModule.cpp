/* vim: set sw=2 :miv */
/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale media player.
 *
 * Copyright(c) 2013
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

/**
* \file  sbBaseMediacoreModule.cpp
* \brief Songbird Mediacore Base Component Factory and Main Entry Point.
*/

#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsICategoryManager.h>
#include <mozilla/ModuleUtils.h>

#include "sbMediacoreCapabilities.h"
#include "sbMediacoreEqualizerBand.h"
#include "sbMediacoreEvent.h"
#include "sbMediacoreFactoryWrapper.h"
#include "sbMediacoreWrapper.h"
#include "sbMediaInspector.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbMediacoreCapabilities, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMediacoreEqualizerBand);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMediacoreEvent);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbMediacoreFactoryWrapper, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbMediacoreWrapper, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMediaFormatContainer);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMediaFormatVideo);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMediaFormatAudio);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMediaFormat);

NS_DEFINE_NAMED_CID(SB_MEDIACORE_CAPABILITIES_CID);
NS_DEFINE_NAMED_CID(SB_MEDIACORE_EQUALIZER_BAND_CID);
NS_DEFINE_NAMED_CID(SB_MEDIACORE_EVENT_CID);
NS_DEFINE_NAMED_CID(SB_MEDIACOREFACTORYWRAPPER_CID);
NS_DEFINE_NAMED_CID(SB_MEDIACOREWRAPPER_CID);
NS_DEFINE_NAMED_CID(SB_MEDIAFORMATCONTAINER_CID);
NS_DEFINE_NAMED_CID(SB_MEDIAFORMATVIDEO_CID);
NS_DEFINE_NAMED_CID(SB_MEDIAFORMATAUDIO_CID);
NS_DEFINE_NAMED_CID(SB_MEDIAFORMAT_CID);

static const mozilla::Module::CIDEntry kBaseMediacoreComponentsCIDs[] = {
  { &kSB_MEDIACORE_CAPABILITIES_CID, false, NULL, sbMediacoreCapabilitiesConstructor },
  { &kSB_MEDIACORE_EQUALIZER_BAND_CID, false, NULL, sbMediacoreEqualizerBandConstructor },
  { &kSB_MEDIACORE_EVENT_CID, false, NULL, sbMediacoreEventConstructor },
  { &kSB_MEDIACOREFACTORYWRAPPER_CID, false, NULL, sbMediacoreFactoryWrapperConstructor },
  { &kSB_MEDIACOREWRAPPER_CID, false, NULL, sbMediacoreWrapperConstructor },
  { &kSB_MEDIAFORMATCONTAINER_CID, false, NULL, sbMediaFormatContainerConstructor },
  { &kSB_MEDIAFORMATVIDEO_CID, false, NULL, sbMediaFormatVideoConstructor },
  { &kSB_MEDIAFORMATAUDIO_CID, false, NULL, sbMediaFormatAudioConstructor },
  { &kSB_MEDIAFORMAT_CID, false, NULL, sbMediaFormatConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kBaseMediacoreComponentsContracts[] = {
  { SB_MEDIACORE_CAPABILITIES_CONTRACTID, &kSB_MEDIACORE_CAPABILITIES_CID },
  { SB_MEDIACORE_EQUALIZER_BAND_CONTRACTID, &kSB_MEDIACORE_EQUALIZER_BAND_CID },
  { SB_MEDIACORE_EVENT_CONTRACTID, &kSB_MEDIACORE_EVENT_CID },
  { SB_MEDIACOREFACTORYWRAPPER_CONTRACTID, &kSB_MEDIACOREFACTORYWRAPPER_CID },
  { SB_MEDIACOREWRAPPER_CONTRACTID, &kSB_MEDIACOREWRAPPER_CID },
  { SB_MEDIAFORMATCONTAINER_CONTRACTID, &kSB_MEDIAFORMATCONTAINER_CID },
  { SB_MEDIAFORMATVIDEO_CONTRACTID, &kSB_MEDIAFORMATVIDEO_CID },
  { SB_MEDIAFORMATAUDIO_CONTRACTID, &kSB_MEDIAFORMATAUDIO_CID },
  { SB_MEDIAFORMAT_CONTRACTID, &kSB_MEDIAFORMAT_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kBaseMediacoreComponentsCategories[] = {
  { NULL }
};

static const mozilla::Module kBaseMediacoreComponentsModule = {
  mozilla::Module::kVersion,
  kBaseMediacoreComponentsCIDs,
  kBaseMediacoreComponentsContracts,
  kBaseMediacoreComponentsCategories
};

NSMODULE_DEFN(sbBaseMediacoreComponents) = &kBaseMediacoreComponentsModule;
