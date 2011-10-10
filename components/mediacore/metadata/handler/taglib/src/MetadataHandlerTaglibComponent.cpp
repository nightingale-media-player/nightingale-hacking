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

/** 
* \file  MetadataHandlerTaglibComponent.cpp
* \brief Nightingale taglib Metadata Handler Component Factory and Main Entry
* Point.
*/

#include "nsIGenericFactory.h"
#include "MetadataHandlerTaglib.h"
#include "TaglibChannelFileIOManager.h"
#include "SeekableChannel.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbMetadataHandlerTaglib, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbTagLibChannelFileIOManager, FactoryInit)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbSeekableChannel)

static nsModuleComponentInfo componentInfo[] =
{
  {
    NIGHTINGALE_METADATAHANDLERTAGLIB_CLASSNAME, 
    NIGHTINGALE_METADATAHANDLERTAGLIB_CID,
    NIGHTINGALE_METADATAHANDLERTAGLIB_CONTRACTID,
    sbMetadataHandlerTaglibConstructor
  },

  {
    NIGHTINGALE_TAGLIBCHANNELFILEIOMANAGER_CLASSNAME, 
    NIGHTINGALE_TAGLIBCHANNELFILEIOMANAGER_CID,
    NIGHTINGALE_TAGLIBCHANNELFILEIOMANAGER_CONTRACTID,
    sbTagLibChannelFileIOManagerConstructor
  },

  {
    NIGHTINGALE_SEEKABLECHANNEL_CLASSNAME, 
    NIGHTINGALE_SEEKABLECHANNEL_CID,
    NIGHTINGALE_SEEKABLECHANNEL_CONTRACTID,
    sbSeekableChannelConstructor
  }
};

NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(NightingaleMetadataHandlerTaglibComponent,
                                   componentInfo,
                                   sbMetadataHandlerTaglib::ModuleConstructor,
                                   sbMetadataHandlerTaglib::ModuleDestructor)
