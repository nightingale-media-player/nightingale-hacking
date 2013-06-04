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
#include <mozilla/ModuleUtils.h>

#include "sbDeviceProperties.h"
#include "sbDeviceCapabilities.h"
#include "sbDeviceCapabilitiesUtils.h"
#include "sbDeviceCapsCompatibility.h"
#include "sbDeviceLibrarySyncDiff.h"
#include "sbDeviceStatus.h"


NS_GENERIC_FACTORY_CONSTRUCTOR(sbAudioFormatType);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbVideoFormatType);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbDeviceCapabilities);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbDeviceCapabilitiesUtils);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbDeviceCapsCompatibility);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbDeviceLibrarySyncDiff);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbDeviceProperties);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbDeviceStatus);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbFormatTypeConstraint);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbImageFormatType);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbDevCapVideoStream);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbDevCapAudioStream);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbImageSize);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbDevCapRange);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbPlaylistFormatType);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbDevCapFraction);

NS_DEFINE_NAMED_CID(SONGBIRD_DEVICEPROPERTIES_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_DEVICECAPABILITIES_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_DEVICECAPABILITIESUTILS_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_DEVICECAPSCOMPATIBILITY_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_DEVICELIBRARYSYNCDIFF_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_DEVICESTATUS_CID);
NS_DEFINE_NAMED_CID(SB_IAUDIOFORMATTYPE_CID);
NS_DEFINE_NAMED_CID(SB_IVIDEOFORMATTYPE_CID);
NS_DEFINE_NAMED_CID(SB_IFORMATTYPECONSTRAINT_CID);
NS_DEFINE_NAMED_CID(SB_IIMAGEFORMATTYPE_CID);
NS_DEFINE_NAMED_CID(SB_IMAGESIZE_CID);
NS_DEFINE_NAMED_CID(SB_IDEVCAPVIDEOSTREAM_CID);
NS_DEFINE_NAMED_CID(SB_IDEVCAPAUDIOSTREAM_CID);
NS_DEFINE_NAMED_CID(SB_IDEVCAPRANGE_CID);
NS_DEFINE_NAMED_CID(SB_IPLAYLISTFORMATTYPE_CID);
NS_DEFINE_NAMED_CID(SB_IDEVCAPFRACTION_CID);

static const mozilla::Module::CIDEntry kDeviceBaseCIDs[] = {
  { &kSONGBIRD_DEVICEPROPERTIES_CID, false, NULL, sbDevicePropertiesConstructor },
  { &kSONGBIRD_DEVICECAPABILITIES_CID, false, NULL, sbDeviceCapabilitiesConstructor },
  { &kSONGBIRD_DEVICECAPABILITIESUTILS_CID, false, NULL, sbDeviceCapabilitiesUtilsConstructor },
  { &kSONGBIRD_DEVICECAPSCOMPATIBILITY_CID, false, NULL, sbDeviceCapsCompatibilityConstructor },
  { &kSONGBIRD_DEVICELIBRARYSYNCDIFF_CID, false, NULL, sbDeviceLibrarySyncDiffConstructor },
  { &kSONGBIRD_DEVICESTATUS_CID, false, NULL, sbDeviceStatusConstructor },
  { &kSB_IAUDIOFORMATTYPE_CID, false, NULL, sbAudioFormatTypeConstructor },
  { &kSB_IVIDEOFORMATTYPE_CID, false, NULL, sbVideoFormatTypeConstructor },
  { &kSB_IFORMATTYPECONSTRAINT_CID, false, NULL, sbFormatTypeConstraintConstructor },
  { &kSB_IIMAGEFORMATTYPE_CID, false, NULL, sbImageFormatTypeConstructor },
  { &kSB_IMAGESIZE_CID, false, NULL, sbImageSizeConstructor },
  { &kSB_IDEVCAPVIDEOSTREAM_CID, false, NULL, sbDevCapVideoStreamConstructor },
  { &kSB_IDEVCAPAUDIOSTREAM_CID, false, NULL, sbDevCapAudioStreamConstructor },
  { &kSB_IDEVCAPRANGE_CID, false, NULL, sbDevCapRangeConstructor },
  { &kSB_IPLAYLISTFORMATTYPE_CID, false, NULL, sbPlaylistFormatTypeConstructor },
  { &kSB_IDEVCAPFRACTION_CID, false, NULL, sbDevCapFractionConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kDeviceBaseContracts[] = {
  { SONGBIRD_DEVICEPROPERTIES_CONTRACTID, &kSONGBIRD_DEVICEPROPERTIES_CID },
  { SONGBIRD_DEVICECAPABILITIES_CONTRACTID, &kSONGBIRD_DEVICECAPABILITIES_CID },
  { SONGBIRD_DEVICECAPABILITIESUTILS_CONTRACTID, &kSONGBIRD_DEVICECAPABILITIESUTILS_CID },
  { SONGBIRD_DEVICECAPSCOMPATIBILITY_CONTRACTID, &kSONGBIRD_DEVICECAPSCOMPATIBILITY_CID },
  { SONGBIRD_DEVICELIBRARYSYNCDIFF_CONTRACTID, &kSONGBIRD_DEVICELIBRARYSYNCDIFF_CID },
  { SONGBIRD_DEVICESTATUS_CONTRACTID, &kSONGBIRD_DEVICESTATUS_CID },
  { SB_IAUDIOFORMATTYPE_CONTRACTID, &kSB_IAUDIOFORMATTYPE_CID },
  { SB_IVIDEOFORMATTYPE_CONTRACTID, &kSB_IVIDEOFORMATTYPE_CID },
  { SB_IFORMATTYPECONSTRAINT_CONTRACTID, &kSB_IFORMATTYPECONSTRAINT_CID },
  { SB_IIMAGEFORMATTYPE_CONTRACTID, &kSB_IIMAGEFORMATTYPE_CID },
  { SB_IMAGESIZE_CONTRACTID, &kSB_IMAGESIZE_CID },
  { SB_IDEVCAPVIDEOSTREAM_CONTRACTID, &kSB_IDEVCAPVIDEOSTREAM_CID },
  { SB_IDEVCAPAUDIOSTREAM_CONTRACTID, &kSB_IDEVCAPAUDIOSTREAM_CID },
  { SB_IDEVCAPRANGE_CONTRACTID, &kSB_IDEVCAPRANGE_CID },
  { SB_IPLAYLISTFORMATTYPE_CONTRACTID, &kSB_IPLAYLISTFORMATTYPE_CID },
  { SB_IDEVCAPFRACTION_CONTRACTID, &kSB_IDEVCAPFRACTION_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kDeviceBaseCategories[] = {
  { NULL }
};

static const mozilla::Module kDeviceBaseModule = {
  mozilla::Module::kVersion,
  kDeviceBaseCIDs,
  kDeviceBaseContracts,
  kDeviceBaseCategories
};

NSMODULE_DEFN(sbDeviceBaseComponents) = &kDeviceBaseModule;
