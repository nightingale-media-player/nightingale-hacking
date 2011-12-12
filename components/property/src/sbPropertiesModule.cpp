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

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbPropertyArray, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbPropertyFactory);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbPropertyManager, Init);

NS_GENERIC_FACTORY_CONSTRUCTOR(sbPropertyOperator);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbDatetimePropertyInfo, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbDurationPropertyInfo, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbNumberPropertyInfo, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbTextPropertyInfo, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbURIPropertyInfo, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbImageLabelLinkPropertyInfo, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbBooleanPropertyInfo, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbDownloadButtonPropertyBuilder, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbStatusPropertyBuilder, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbSimpleButtonPropertyBuilder, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbImagePropertyBuilder, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbImageLabelLinkPropertyBuilder, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbRatingPropertyBuilder, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbOriginPageImagePropertyBuilder, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbStoragePropertyUnitConverter);

static const mozilla::Module::CIDEntry kSongbirdPropertiesModuleCIDs[] = {
    { &SB_MUTABLEPROPERTYARRAY_CID, false, NULL, sbPropertyArrayConstructor },
    { &SB_PROPERTYFACTORY_CID, false, NULL, sbPropertyFactoryConstructor },
    { &SB_PROPERTYMANAGER_CID, false, NULL, sbPropertyManagerConstructor },
    { &SB_PROPERTYOPERATOR_CID, false, NULL, sbPropertyOperatorConstructor },  
    { &SB_DATETIMEPROPERTYINFO_CID, false, NULL, sbDatetimePropertyInfoConstructor },  
    { &SB_DURATIONPROPERTYINFO_CID, false, NULL, sbDurationPropertyInfoConstructor }, 
    { &SB_NUMBERPROPERTYINFO_CID, false, NULL, sbNumberPropertyInfoConstructor }, 
    { &SB_TEXTPROPERTYINFO_CID, false, NULL, sbTextPropertyInfoConstructor }, 
    { &SB_URIPROPERTYINFO_CID, false, NULL, sbURIPropertyInfoConstructor },
    { &SB_BOOLEANPROPERTYINFO_CID, false, NULL, sbBooleanPropertyInfoConstructor },
    { &SB_DOWNLOADBUTTONPROPERTYBUILDER_CID, false, NULL, sbDownloadButtonPropertyBuilderConstructor },
    { &SB_STATUSPROPERTYBUILDER_CID, false, NULL, sbStatusPropertyBuilderConstructor },
    { &SB_SIMPLEBUTTONPROPERTYBUILDER_CID, false, NULL, sbSimpleButtonPropertyBuilderConstructor },
    { &SB_SBIMAGELABELLINKPROPERTYINFO_CID, false, NULL, sbImageLabelLinkPropertyInfoConstructor },
    { &SB_IMAGEPROPERTYBUILDER_CID, false, NULL, sbImagePropertyBuilderConstructor },
    { &SB_SBIMAGELABELLINKPROPERTYBUILDER_CID, false, NULL, sbImageLabelLinkPropertyBuilderConstructor },
    { &SB_RATINGPROPERTYBUILDER_CID, false, NULL, sbRatingPropertyBuilderConstructor },
    { &SB_ORIGINPAGEIMAGEPROPERTYBUILDER_CID, false, NULL, sbOriginPageImagePropertyBuilderConstructor },
    { &SB_STORAGEPROPERTYUNITCONVERTER_CID, false, NULL, sbStoragePropertyUnitConverterConstructor },
    { NULL }
};


static const mozilla::Module::ContractIDEntry kSongbirdPropertiesModuleContracts[] = {
    { SB_MUTABLEPROPERTYARRAY_CONTRACTID, &SB_MUTABLEPROPERTYARRAY_CID },
    { SB_PROPERTYFACTORY_CONTRACTID, &SB_PROPERTYFACTORY_CID },
    { SB_PROPERTYMANAGER_CONTRACTID, &SB_PROPERTYMANAGER_CID },
    { SB_PROPERTYOPERATOR_CONTRACTID, &SB_PROPERTYOPERATOR_CID },
    { SB_DATETIMEPROPERTYINFO_CONTRACTID, &SB_DATETIMEPROPERTYINFO_CID },
    { SB_DURATIONPROPERTYINFO_CONTRACTID, &SB_DURATIONPROPERTYINFO_CID },
    { SB_NUMBERPROPERTYINFO_CONTRACTID, &SB_NUMBERPROPERTYINFO_CID },
    { SB_TEXTPROPERTYINFO_CONTRACTID, &SB_TEXTPROPERTYINFO_CID },
    { SB_URIPROPERTYINFO_CONTRACTID, &SB_URIPROPERTYINFO_CID },
    { SB_BOOLEANPROPERTYINFO_CONTRACTID, &SB_BOOLEANPROPERTYINFO_CID },
    { SB_DOWNLOADBUTTONPROPERTYBUILDER_CONTRACTID, &SB_DOWNLOADBUTTONPROPERTYBUILDER_CID },
    { SB_STATUSPROPERTYBUILDER_CONTRACTID, &SB_STATUSPROPERTYBUILDER_CID },
    { SB_SIMPLEBUTTONPROPERTYBUILDER_CONTRACTID, &SB_SIMPLEBUTTONPROPERTYBUILDER_CID },
    { SB_SBIMAGELABELLINKPROPERTYINFO_CONTRACTID, &SB_SBIMAGELABELLINKPROPERTYINFO_CID },
    { SB_IMAGEPROPERTYBUILDER_CONTRACTID, &SB_IMAGEPROPERTYBUILDER_CID },
    { SB_SBIMAGELABELLINKPROPERTYBUILDER_CONTRACTID, &SB_SBIMAGELABELLINKPROPERTYBUILDER_CID },
    { SB_RATINGPROPERTYBUILDER_CONTRACTID, &SB_RATINGPROPERTYBUILDER_CID },
    { SB_ORIGINPAGEIMAGEPROPERTYBUILDER_CONTRACTID, &SB_ORIGINPAGEIMAGEPROPERTYBUILDER_CID },
    { SB_STORAGEPROPERTYUNITCONVERTER_CONTRACTID, &SB_STORAGEPROPERTYUNITCONVERTER_CID },
    { NULL }
};

static const mozilla::Module kSongbirdPropertiesModule = {
    mozilla::Module::kVersion,
    kSongbirdPropertiesModuleCIDs,
    kSongbirdPropertiesModuleContracts
};

NSMODULE_DEFN(SongbirdPropertiesModule) = &kSongbirdPropertiesModule;
