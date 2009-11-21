/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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
* \file  sbBaseMediacoreModule.cpp
* \brief Songbird Mediacore Base Component Factory and Main Entry Point.
*/

#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsICategoryManager.h>
#include <nsIGenericFactory.h>

#include "sbMediacoreEqualizerBand.h"
#include "sbMediacoreEvent.h"
#include "sbMediaInspector.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbMediacoreEqualizerBand);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMediacoreEvent);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMediaInspector);

static nsModuleComponentInfo sbBaseMediacoreComponents[] =
{
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
    SB_MEDIAINSPECTOR_CLASSNAME,
    SB_MEDIAINSPECTOR_CID,
    SB_MEDIAINSPECTOR_CONTRACTID,
    sbMediaInspectorConstructor
  }
};

NS_IMPL_NSGETMODULE(SongbirdBaseMediacore, sbBaseMediacoreComponents)
