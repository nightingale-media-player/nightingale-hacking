/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

#include "nsIGenericFactory.h"

#include "sbPropertyArray.h"
#include "sbPropertyFactory.h"
#include "sbPropertyManager.h"

#include "sbDatetimePropertyInfo.h"
#include "sbNumberPropertyInfo.h"
#include "sbProgressPropertyInfo.h"
#include "sbTextPropertyInfo.h"
#include "sbURIPropertyInfo.h"
#include "sbButtonPropertyInfo.h"
#include "sbBooleanPropertyInfo.h"

#include "sbPropertiesCID.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbPropertyArray, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbPropertyFactory);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbPropertyManager, Init);

NS_GENERIC_FACTORY_CONSTRUCTOR(sbPropertyOperator);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbDatetimePropertyInfo);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbNumberPropertyInfo);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbProgressPropertyInfo);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbTextPropertyInfo);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbURIPropertyInfo);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbButtonPropertyInfo);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbBooleanPropertyInfo, Init);

static const nsModuleComponentInfo components[] =
{
	{
    SB_MUTABLEPROPERTYARRAY_DESCRIPTION,
    SB_MUTABLEPROPERTYARRAY_CID,
    SB_MUTABLEPROPERTYARRAY_CONTRACTID,
    sbPropertyArrayConstructor
	},  
	{
    SB_PROPERTYFACTORY_DESCRIPTION,
    SB_PROPERTYFACTORY_CID,
    SB_PROPERTYFACTORY_CONTRACTID,
    sbPropertyFactoryConstructor
	},
  {
    SB_PROPERTYMANAGER_DESCRIPTION,
    SB_PROPERTYMANAGER_CID,
    SB_PROPERTYMANAGER_CONTRACTID,
    sbPropertyManagerConstructor,
    sbPropertyManager::RegisterSelf
  },
  {
    SB_PROPERTYOPERATOR_DESCRIPTION,
    SB_PROPERTYOPERATOR_CID,
    SB_PROPERTYOPERATOR_CONTRACTID,
    sbPropertyOperatorConstructor
  },
  {
    SB_DATETIMEPROPERTYINFO_DESCRIPTION,
    SB_DATETIMEPROPERTYINFO_CID,
    SB_DATETIMEPROPERTYINFO_CONTRACTID,
    sbDatetimePropertyInfoConstructor
  },
  {
    SB_NUMBERPROPERTYINFO_DESCRIPTION,
    SB_NUMBERPROPERTYINFO_CID,
    SB_NUMBERPROPERTYINFO_CONTRACTID,
    sbNumberPropertyInfoConstructor
  },
  {
    SB_TEXTPROPERTYINFO_DESCRIPTION,
    SB_TEXTPROPERTYINFO_CID,
    SB_TEXTPROPERTYINFO_CONTRACTID,
    sbTextPropertyInfoConstructor
  },
  {
    SB_PROGRESSPROPERTYINFO_DESCRIPTION,
    SB_PROGRESSPROPERTYINFO_CID,
    SB_PROGRESSPROPERTYINFO_CONTRACTID,
    sbProgressPropertyInfoConstructor
  },
  {
    SB_URIPROPERTYINFO_DESCRIPTION,
    SB_URIPROPERTYINFO_CID,
    SB_URIPROPERTYINFO_CONTRACTID,
    sbURIPropertyInfoConstructor
  },
  {
    SB_BUTTONPROPERTYINFO_DESCRIPTION,
    SB_BUTTONPROPERTYINFO_CID,
    SB_BUTTONPROPERTYINFO_CONTRACTID,
    sbButtonPropertyInfoConstructor
  },
  {
    SB_BOOLEANPROPERTYINFO_DESCRIPTION,
    SB_BOOLEANPROPERTYINFO_CID,
    SB_BOOLEANPROPERTYINFO_CONTRACTID,
    sbBooleanPropertyInfoConstructor
  },
};

NS_IMPL_NSGETMODULE(Songbird Properties Module, components)
