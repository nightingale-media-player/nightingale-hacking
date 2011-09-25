/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#include <nsIGenericFactory.h>
#include "sbMediaExportITunesAgentService.h"
#include "sbIMediaExportAgentService.h"


NS_GENERIC_FACTORY_CONSTRUCTOR(sbMediaExportITunesAgentService)


#define SB_MEDIAEXPORTAGENTSERVICE_CID                    \
{ 0x625f3a7c, 0x9f5c, 0x4d43, { 0x82, 0xc5, 0xdb, 0x30, 0xe6, 0x7b, 0x60, 0x25 } }


static nsModuleComponentInfo sbMediaExportITunesAgent[] =
{
  {
    SB_MEDIAEXPORTAGENTSERVICE_CLASSNAME,
    SB_MEDIAEXPORTAGENTSERVICE_CID,
    SB_MEDIAEXPORTAGENTSERVICE_CONTRACTID,
    sbMediaExportITunesAgentServiceConstructor
  },
};

NS_IMPL_NSGETMODULE(NightingaleMediaExportITunesAgentComponent, 
                    sbMediaExportITunesAgent)


