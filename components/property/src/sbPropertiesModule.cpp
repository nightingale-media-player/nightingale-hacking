/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale media player.
 *
 * Copyright(c) 2013 POTI, Inc.
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

#include <mozilla/ModuleUtils.h>

#include "sbBooleanPropertyInfo.h"
#include "sbDatetimePropertyInfo.h"
#include "sbDurationPropertyInfo.h"
#include "sbDownloadButtonPropertyBuilder.h"
#include "sbImagePropertyBuilder.h"
#include "sbImageLabelLinkPropertyBuilder.h"
#include "sbImageLabelLinkPropertyInfo.h"
#include "sbNumberPropertyInfo.h"
#include "sbPropertyArray.h"
#include "sbPropertyFactory.h"
#include "sbPropertyManager.h"
#include "sbOriginPageImagePropertyBuilder.h"
#include "sbRatingPropertyBuilder.h"
#include "sbSimpleButtonPropertyBuilder.h"
#include "sbStatusPropertyBuilder.h"
#include "sbStoragePropertyUnitConverter.h"
#include "sbTextPropertyInfo.h"
#include "sbURIPropertyInfo.h"

#include "sbPropertiesCID.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbPropertyArray, Init);
NS_DEFINE_NAMED_CID(SB_MUTABLEPROPERTYARRAY_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(sbPropertyFactory);
NS_DEFINE_NAMED_CID(SB_PROPERTYFACTORY_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbPropertyManager, Init);
NS_DEFINE_NAMED_CID(SB_PROPERTYMANAGER_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(sbPropertyOperator);
NS_DEFINE_NAMED_CID(SB_PROPERTYOPERATOR_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbDatetimePropertyInfo, Init);
NS_DEFINE_NAMED_CID(SB_DATETIMEPROPERTYINFO_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbDurationPropertyInfo, Init);
NS_DEFINE_NAMED_CID(SB_DURATIONPROPERTYINFO_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbNumberPropertyInfo, Init);
NS_DEFINE_NAMED_CID(SB_NUMBERPROPERTYINFO_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbTextPropertyInfo, Init);
NS_DEFINE_NAMED_CID(SB_TEXTPROPERTYINFO_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbURIPropertyInfo, Init);
NS_DEFINE_NAMED_CID(SB_URIPROPERTYINFO_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbImageLabelLinkPropertyInfo, Init);
NS_DEFINE_NAMED_CID(SB_SBIMAGELABELLINKPROPERTYINFO_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbBooleanPropertyInfo, Init);
NS_DEFINE_NAMED_CID(SB_BOOLEANPROPERTYINFO_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbDownloadButtonPropertyBuilder, Init);
NS_DEFINE_NAMED_CID(SB_DOWNLOADBUTTONPROPERTYBUILDER_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbStatusPropertyBuilder, Init);
NS_DEFINE_NAMED_CID(SB_STATUSPROPERTYBUILDER_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbSimpleButtonPropertyBuilder, Init);
NS_DEFINE_NAMED_CID(SB_SIMPLEBUTTONPROPERTYBUILDER_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbImagePropertyBuilder, Init);
NS_DEFINE_NAMED_CID(SB_IMAGEPROPERTYBUILDER_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbImageLabelLinkPropertyBuilder, Init);
NS_DEFINE_NAMED_CID(SB_SBIMAGELABELLINKPROPERTYBUILDER_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbRatingPropertyBuilder, Init);
NS_DEFINE_NAMED_CID(SB_RATINGPROPERTYBUILDER_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbOriginPageImagePropertyBuilder, Init);
NS_DEFINE_NAMED_CID(SB_ORIGINPAGEIMAGEPROPERTYBUILDER_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(sbStoragePropertyUnitConverter);
NS_DEFINE_NAMED_CID(SB_STORAGEPROPERTYUNITCONVERTER_CID);


static const mozilla::Module::CIDEntry kSongbirdPropertiesModuleCIDs[] = {
  { &kSB_MUTABLEPROPERTYARRAY_CID, false, NULL, sbPropertyArrayConstructor },
  { &kSB_PROPERTYFACTORY_CID, false, NULL, sbPropertyFactoryConstructor },
  { &kSB_PROPERTYMANAGER_CID, false, NULL, sbPropertyManagerConstructor },
  { &kSB_PROPERTYOPERATOR_CID, false, NULL, sbPropertyOperatorConstructor },
  { &kSB_DATETIMEPROPERTYINFO_CID, false, NULL, sbDatetimePropertyInfoConstructor },
  { &kSB_DURATIONPROPERTYINFO_CID, false, NULL, sbDurationPropertyInfoConstructor },
  { &kSB_NUMBERPROPERTYINFO_CID, false, NULL, sbNumberPropertyInfoConstructor },
  { &kSB_TEXTPROPERTYINFO_CID, false, NULL, sbTextPropertyInfoConstructor },
  { &kSB_URIPROPERTYINFO_CID, false, NULL, sbURIPropertyInfoConstructor },
  { &kSB_SBIMAGELABELLINKPROPERTYINFO_CID, false, NULL, sbImageLabelLinkPropertyInfoConstructor },
  { &kSB_BOOLEANPROPERTYINFO_CID, false, NULL, sbBooleanPropertyInfoConstructor },
  { &kSB_DOWNLOADBUTTONPROPERTYBUILDER_CID, false, NULL, sbDownloadButtonPropertyBuilderConstructor },
  { &kSB_STATUSPROPERTYBUILDER_CID, false, NULL, sbStatusPropertyBuilderConstructor },
  { &kSB_SIMPLEBUTTONPROPERTYBUILDER_CID, false, NULL, sbSimpleButtonPropertyBuilderConstructor },
  { &kSB_IMAGEPROPERTYBUILDER_CID, false, NULL, sbImagePropertyBuilderConstructor },
  { &kSB_SBIMAGELABELLINKPROPERTYBUILDER_CID, false, NULL, sbImageLabelLinkPropertyBuilderConstructor },
  { &kSB_RATINGPROPERTYBUILDER_CID, false, NULL, sbRatingPropertyBuilderConstructor },
  { &kSB_ORIGINPAGEIMAGEPROPERTYBUILDER_CID, false, NULL, sbOriginPageImagePropertyBuilderConstructor },
  { &kSB_STORAGEPROPERTYUNITCONVERTER_CID, false, NULL, sbStoragePropertyUnitConverterConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kSongbirdPropertiesModuleContracts[] = {
  { SB_MUTABLEPROPERTYARRAY_CONTRACTID, &kSB_MUTABLEPROPERTYARRAY_CID },
  { SB_PROPERTYFACTORY_CONTRACTID, &kSB_PROPERTYFACTORY_CID },
  { SB_PROPERTYMANAGER_CONTRACTID, &kSB_PROPERTYMANAGER_CID },
  { SB_PROPERTYOPERATOR_CONTRACTID, &kSB_PROPERTYOPERATOR_CID },
  { SB_DATETIMEPROPERTYINFO_CONTRACTID, &kSB_DATETIMEPROPERTYINFO_CID },
  { SB_DURATIONPROPERTYINFO_CONTRACTID, &kSB_DURATIONPROPERTYINFO_CID },
  { SB_NUMBERPROPERTYINFO_CONTRACTID, &kSB_NUMBERPROPERTYINFO_CID },
  { SB_TEXTPROPERTYINFO_CONTRACTID, &kSB_TEXTPROPERTYINFO_CID },
  { SB_URIPROPERTYINFO_CONTRACTID, &kSB_URIPROPERTYINFO_CID },
  { SB_SBIMAGELABELLINKPROPERTYINFO_CONTRACTID, &kSB_SBIMAGELABELLINKPROPERTYINFO_CID },
  { SB_BOOLEANPROPERTYINFO_CONTRACTID, &kSB_BOOLEANPROPERTYINFO_CID },
  { SB_DOWNLOADBUTTONPROPERTYBUILDER_CONTRACTID, &kSB_DOWNLOADBUTTONPROPERTYBUILDER_CID },
  { SB_STATUSPROPERTYBUILDER_CONTRACTID, &kSB_STATUSPROPERTYBUILDER_CID },
  { SB_SIMPLEBUTTONPROPERTYBUILDER_CONTRACTID, &kSB_SIMPLEBUTTONPROPERTYBUILDER_CID },
  { SB_IMAGEPROPERTYBUILDER_CONTRACTID, &kSB_IMAGEPROPERTYBUILDER_CID },
  { SB_SBIMAGELABELLINKPROPERTYBUILDER_CONTRACTID, &kSB_SBIMAGELABELLINKPROPERTYBUILDER_CID },
  { SB_RATINGPROPERTYBUILDER_CONTRACTID, &kSB_RATINGPROPERTYBUILDER_CID },
  { SB_ORIGINPAGEIMAGEPROPERTYBUILDER_CONTRACTID, &kSB_ORIGINPAGEIMAGEPROPERTYBUILDER_CID },
  { SB_STORAGEPROPERTYUNITCONVERTER_CONTRACTID, &kSB_STORAGEPROPERTYUNITCONVERTER_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kSongbirdPropertiesModuleCategories[] = {
  { NULL }
};

static const mozilla::Module kSongbirdPropertiesModule = {
  mozilla::Module::kVersion,
  kSongbirdPropertiesModuleCIDs,
  kSongbirdPropertiesModuleContracts,
  kSongbirdPropertiesModuleCategories
};

NSMODULE_DEFN(SongbirdPropertiesModule) = &kSongbirdPropertiesModule;
