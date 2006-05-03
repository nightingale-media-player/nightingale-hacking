/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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
* \file  MetadataManagerComponent.cpp
* \brief Songbird Metadata Manager Component Factory and Main Entry Point.
*/

#include "nsIGenericFactory.h"
#include "MetadataManager.h"
#include "MetadataValues.h"
#include "MetadataChannel.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbMetadataManager)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMetadataValues)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMetadataChannel)

static nsModuleComponentInfo sbMetadataManagerComponent[] =
{

  {
    SONGBIRD_METADATAMANAGER_CLASSNAME,
    SONGBIRD_METADATAMANAGER_CID,
    SONGBIRD_METADATAMANAGER_CONTRACTID,
    sbMetadataManagerConstructor
  },
  {
    SONGBIRD_METADATAVALUES_CLASSNAME,
    SONGBIRD_METADATAVALUES_CID,
    SONGBIRD_METADATAVALUES_CONTRACTID,
    sbMetadataValuesConstructor
  },
  {
    SONGBIRD_METADATACHANNEL_CLASSNAME,
    SONGBIRD_METADATACHANNEL_CID,
    SONGBIRD_METADATACHANNEL_CONTRACTID,
    sbMetadataChannelConstructor
  },

};

NS_IMPL_NSGETMODULE("SongbirdMetadataManagerComponent", sbMetadataManagerComponent)
