/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
 */

#include "sbArray.h"
#include "sbPropertyBag.h"
#include <nsIGenericFactory.h>

NS_GENERIC_FACTORY_CONSTRUCTOR(sbArray)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbPropertyBag, Init)

#define SB_PROPERTYBAG_CLASSNAME "sbPropertyBag"
#define SB_PROPERTYBAG_CID \
   {0x135c8890, 0xd9da, 0x4a1c, \
     {0xb3, 0x45, 0x3b, 0xb2, 0xfe, 0xe8, 0xfa, 0xb5}}
#define SB_PROPERTYBAG_CONTRACTID "@getnightingale.com/moz/xpcom/sbpropertybag;1"
// fill out data struct to register with component system
static const nsModuleComponentInfo components[] =
{
  {
    SB_THREADSAFE_ARRAY_CLASSNAME,
    SB_THREADSAFE_ARRAY_CID,
    SB_THREADSAFE_ARRAY_CONTRACTID,
    sbArrayConstructor
  },
  {
    SB_PROPERTYBAG_CLASSNAME,
    SB_PROPERTYBAG_CID,
    SB_PROPERTYBAG_CONTRACTID,
    sbPropertyBagConstructor
  }
};

// create the module info struct that is used to regsiter
NS_IMPL_NSGETMODULE(NightingaleXPCOMLib, components)

