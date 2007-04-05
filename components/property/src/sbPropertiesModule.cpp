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
#include "sbTextPropertyInfo.h"
#include "sbURIPropertyInfo.h"

#include "sbPropertiesCID.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbPropertyArray)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbPropertyFactory);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbPropertyManager, Init);

NS_GENERIC_FACTORY_CONSTRUCTOR(sbDatetimePropertyInfo);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbNumberPropertyInfo);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbTextPropertyInfo);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbURIPropertyInfo);

static const nsModuleComponentInfo components[] =
{
	{
    SB_PROPERTYARRAY_DESCRIPTION,
    SB_PROPERTYARRAY_CID,
    SB_PROPERTYARRAY_CONTRACTID,
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
    SB_URIPROPERTYINFO_DESCRIPTION,
    SB_URIPROPERTYINFO_CID,
    SB_URIPROPERTYINFO_CONTRACTID,
    sbURIPropertyInfoConstructor
  },
};

NS_IMPL_NSGETMODULE(Songbird Properties Module, components)
