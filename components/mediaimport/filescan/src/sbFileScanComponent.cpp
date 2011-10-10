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
* \file  FileScanComponent.cpp
* \brief Nightingale FileScan Component Factory and Main Entry Point.
*/

#include "nsIGenericFactory.h"

#include "sbFileScan.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbFileScan)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbFileScanQuery)

static nsModuleComponentInfo sbFileScan[] =
{
  {
    NIGHTINGALE_FILESCAN_CLASSNAME,
    NIGHTINGALE_FILESCAN_CID,
    NIGHTINGALE_FILESCAN_CONTRACTID,
    sbFileScanConstructor
  },

  {
    NIGHTINGALE_FILESCANQUERY_CLASSNAME,
    NIGHTINGALE_FILESCANQUERY_CID,
    NIGHTINGALE_FILESCANQUERY_CONTRACTID,
    sbFileScanQueryConstructor
  },
  
};

NS_IMPL_NSGETMODULE(NightingaleFileScanComponent, sbFileScan)
