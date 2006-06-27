/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 POTI, Inc.
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
* \file  MetadataHandlerWMAComponent.cpp
* \brief Songbird WMA Metadata Handler Component Factory and Main Entry Point.
*/

#include "nsIGenericFactory.h"
#include "MetadataHandlerWMA.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbMetadataHandlerWMA)

static nsModuleComponentInfo sbMetadataHandlerWMAComponent[] =
{

  {
    SONGBIRD_METADATAHANDLERWMA_CLASSNAME,
    SONGBIRD_METADATAHANDLERWMA_CID,
    SONGBIRD_METADATAHANDLERWMA_CONTRACTID,
    sbMetadataHandlerWMAConstructor
  },

};

NS_IMPL_NSGETMODULE("SongbirdMetadataHandlerWMAComponent", sbMetadataHandlerWMAComponent)
