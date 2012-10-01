/* vim: set sw=2 :miv */
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
* \file  sbBaseMediacoreModule.cpp
* \brief Songbird Mediacore Base Component Factory and Main Entry Point.
*/

#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsICategoryManager.h>
#include <nsIGenericFactory.h>

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

static nsModuleComponentInfo sbBaseMediacoreComponents[] =
{
  {
    SB_MEDIACORE_CAPABILITIES_CLASSNAME,
    SB_MEDIACORE_CAPABILITIES_CID,
    SB_MEDIACORE_CAPABILITIES_CONTRACTID,
    sbMediacoreCapabilitiesConstructor
  },
  {
    SB_MEDIACORE_EQUALIZER_BAND_CLASSNAME,
    SB_MEDIACORE_EQUALIZER_BAND_CID,
    SB_MEDIACORE_EQUALIZER_BAND_CONTRACTID,
    sbMediacoreEqualizerBandConstructor
  },
  {
    SB_MEDIACORE_EVENT_CLASSNAME,
    SB_MEDIACORE_EVENT_CID,
    SB_MEDIACORE_EVENT_CONTRACTID,
    sbMediacoreEventConstructor
  },
  {
    SB_MEDIACOREFACTORYWRAPPER_CLASSNAME,
    SB_MEDIACOREFACTORYWRAPPER_CID,
    SB_MEDIACOREFACTORYWRAPPER_CONTRACTID,
    sbMediacoreFactoryWrapperConstructor
  },
  {
    SB_MEDIACOREWRAPPER_CLASSNAME,
    SB_MEDIACOREWRAPPER_CID,
    SB_MEDIACOREWRAPPER_CONTRACTID,
    sbMediacoreWrapperConstructor
  },
  {
    SB_MEDIAFORMATCONTAINER_CLASSNAME,
    SB_MEDIAFORMATCONTAINER_CID,
    SB_MEDIAFORMATCONTAINER_CONTRACTID,
    sbMediaFormatContainerConstructor
  },
  {
    SB_MEDIAFORMATVIDEO_CLASSNAME,
    SB_MEDIAFORMATVIDEO_CID,
    SB_MEDIAFORMATVIDEO_CONTRACTID,
    sbMediaFormatVideoConstructor
  },
  {
    SB_MEDIAFORMATAUDIO_CLASSNAME,
    SB_MEDIAFORMATAUDIO_CID,
    SB_MEDIAFORMATAUDIO_CONTRACTID,
    sbMediaFormatAudioConstructor
  },
  {
    SB_MEDIAFORMAT_CLASSNAME,
    SB_MEDIAFORMAT_CID,
    SB_MEDIAFORMAT_CONTRACTID,
    sbMediaFormatConstructor
  },
};

NS_IMPL_NSGETMODULE(SongbirdBaseMediacore, sbBaseMediacoreComponents)
