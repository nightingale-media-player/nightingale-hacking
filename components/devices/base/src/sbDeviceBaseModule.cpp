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
* \file  sbDeviceManagerModule.cpp
* \brief Songbird Device Manager Component Factory and Main Entry Point.
*/

#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsIAppStartupNotifier.h>
#include <nsICategoryManager.h>
#include <nsIGenericFactory.h>

#include "sbContentTypeFormat.h"
#include "sbDeviceProperties.h"
#include "sbDeviceCapabilities.h"
#include "sbDeviceStatus.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbContentTypeFormat);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbDeviceProperties);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbDeviceCapabilities);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbDeviceStatus);

static nsModuleComponentInfo sbDeviceBaseComponents[] =
{
  {
    SONGBIRD_DEVICEPROPERTIES_CLASSNAME,
    SONGBIRD_DEVICEPROPERTIES_CID,
    SONGBIRD_DEVICEPROPERTIES_CONTRACTID,
    sbDevicePropertiesConstructor
  },

  {
    SONGBIRD_DEVICECAPABILITIES_CLASSNAME,
    SONGBIRD_DEVICECAPABILITIES_CID,
    SONGBIRD_DEVICECAPABILITIES_CONTRACTID,
    sbDeviceCapabilitiesConstructor
  },

  {
    SONGBIRD_CONTENTTYPEFORMAT_CLASSNAME,
    SONGBIRD_CONTENTTYPEFORMAT_CID,
    SONGBIRD_CONTENTTYPEFORMAT_CONTRACTID,
    sbContentTypeFormatConstructor
  },

  {
    SONGBIRD_DEVICESTATUS_CLASSNAME,
    SONGBIRD_DEVICESTATUS_CID,
    SONGBIRD_DEVICESTATUS_CONTRACTID,
    sbDeviceStatusConstructor
  }
};

NS_IMPL_NSGETMODULE(SongbirdDeviceBaseComps, sbDeviceBaseComponents)

