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

#include "sbDeviceProperties.h"
#include "sbDeviceCapabilities.h"
#include "sbDeviceCapabilitiesUtils.h"
#include "sbDeviceCapsCompatibility.h"
#include "sbDeviceStatus.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbAudioFormatType);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbVideoFormatType);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbDeviceCapabilities);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbDeviceCapabilitiesUtils);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbDeviceCapsCompatibility);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbDeviceProperties);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbDeviceStatus);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbFormatTypeConstraint);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbImageFormatType);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbDevCapVideoStream);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbDevCapAudioStream);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbImageSize);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbDevCapRange);

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
    SONGBIRD_DEVICECAPABILITIESUTILS_CLASSNAME,
    SONGBIRD_DEVICECAPABILITIESUTILS_CID,
    SONGBIRD_DEVICECAPABILITIESUTILS_CONTRACTID,
    sbDeviceCapabilitiesUtilsConstructor
  },

  {
    SONGBIRD_DEVICECAPSCOMPATIBILITY_DESCRIPTION,
    SONGBIRD_DEVICECAPSCOMPATIBILITY_CID,
    SONGBIRD_DEVICECAPSCOMPATIBILITY_CONTRACTID,
    sbDeviceCapsCompatibilityConstructor
  },

  {
    SONGBIRD_DEVICESTATUS_CLASSNAME,
    SONGBIRD_DEVICESTATUS_CID,
    SONGBIRD_DEVICESTATUS_CONTRACTID,
    sbDeviceStatusConstructor
  },

  {
    SB_IAUDIOFORMATTYPE_CLASSNAME,
    SB_IAUDIOFORMATTYPE_CID,
    SB_IAUDIOFORMATTYPE_CONTRACTID,
    sbAudioFormatTypeConstructor
  },

  {
    SB_IVIDEOFORMATTYPE_CLASSNAME,
    SB_IVIDEOFORMATTYPE_CID,
    SB_IVIDEOFORMATTYPE_CONTRACTID,
    sbVideoFormatTypeConstructor
  },

  {
    SB_IFORMATTYPECONSTRAINT_CLASSNAME,
    SB_IFORMATTYPECONSTRAINT_CID,
    SB_IFORMATTYPECONSTRAINT_CONTRACTID,
    sbFormatTypeConstraintConstructor
  },

  {
    SB_IIMAGEFORMATTYPE_CLASSNAME,
    SB_IIMAGEFORMATTYPE_CID,
    SB_IIMAGEFORMATTYPE_CONTRACTID,
    sbImageFormatTypeConstructor
  },

  {
    SB_IMAGESIZE_CLASSNAME,
    SB_IMAGESIZE_CID,
    SB_IMAGESIZE_CONTRACTID,
    sbImageSizeConstructor
  },

  {
    SB_IDEVCAPVIDEOSTREAM_CLASSNAME,
    SB_IDEVCAPVIDEOSTREAM_CID,
    SB_IDEVCAPVIDEOSTREAM_CONTRACTID,
    sbDevCapVideoStreamConstructor
  },

  {
    SB_IDEVCAPAUDIOSTREAM_CLASSNAME,
    SB_IDEVCAPAUDIOSTREAM_CID,
    SB_IDEVCAPAUDIOSTREAM_CONTRACTID,
    sbDevCapAudioStreamConstructor
  },

  {
    SB_IVIDEOFORMATTYPE_CLASSNAME,
    SB_IVIDEOFORMATTYPE_CID,
    SB_IVIDEOFORMATTYPE_CONTRACTID,
    sbVideoFormatTypeConstructor
  },

  {
    SB_IDEVCAPRANGE_CLASSNAME,
    SB_IDEVCAPRANGE_CID,
    SB_IDEVCAPRANGE_CONTRACTID,
    sbDevCapRangeConstructor
  },

};

NS_IMPL_NSGETMODULE(SongbirdDeviceBaseComps, sbDeviceBaseComponents)

