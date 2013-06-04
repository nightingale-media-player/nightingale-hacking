/*
 * BEGIN NIGHTINGALE GPL
 * 
 * This file is part of the Nightingale Media Player.
 *
 * Copyright(c) 2013
 * http://getnightingale.com
 * 
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the "GPL").
 * 
 * Software distributed under the License is distributed 
 * on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 * express or implied. See the GPL for the specific language 
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this 
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc., 
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * 
 * END NIGHTINGALE GPL
 */

/** 
* \file  DownloadDeviceComponent.cpp
* \brief Songbird DeviceBase Component Factory and Main Entry Point.
*/

#include <mozilla/ModuleUtils.h>
#include "DownloadDevice.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbDownloadDevice)

NS_DEFINE_NAMED_CID(SONGBIRD_DownloadDevice_CID);

static const mozilla::Module::CIDEntry kDownloadDeviceCIDs[] = {
  { &kSONGBIRD_DownloadDevice_CID, false, NULL, sbDownloadDeviceConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kDownloadDeviceContracts[] = {
  { SONGBIRD_DownloadDevice_CONTRACTID, &kSONGBIRD_DownloadDevice_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kDownloadDeviceCategories[] = {
  { NULL }
};

static const mozilla::Module kDownloadDeviceModule = {
  mozilla::Module::kVersion,
  kDownloadDeviceCIDs,
  kDownloadDeviceContracts,
  kDownloadDeviceCategories
};

NSMODULE_DEFN(sbDownloadDeviceComponents) = &kDownloadDeviceModule;
