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

/** 
* \file  MediaLibraryComponent.cpp
* \brief Songbird MediaLibrary Component Factory and Main Entry Point.
*/

#include "nsIGenericFactory.h"

#include "MediaScan.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(CMediaScan)
NS_GENERIC_FACTORY_CONSTRUCTOR(CMediaScanQuery)

static nsModuleComponentInfo sbMediaLibrary[] =
{
  {
    SONGBIRD_MEDIASCAN_CLASSNAME,
    SONGBIRD_MEDIASCAN_CID,
    SONGBIRD_MEDIASCAN_CONTRACTID,
    CMediaScanConstructor
  },

  {
    SONGBIRD_MEDIASCANQUERY_CLASSNAME,
    SONGBIRD_MEDIASCANQUERY_CID,
    SONGBIRD_MEDIASCANQUERY_CONTRACTID,
    CMediaScanQueryConstructor
  },
  
};

NS_IMPL_NSGETMODULE("SongbirdMediaLibraryComponent", sbMediaLibrary)
