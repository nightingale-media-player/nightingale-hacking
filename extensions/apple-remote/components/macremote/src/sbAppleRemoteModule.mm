/*
//
// BEGIN NIGHTINGALE GPL
// 
// This file is part of the Nightingale Media Player.
//
// Copyright(c) 2014
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

#include "nsIGenericFactory.h"
#include "sbAppleRemoteService.h"


// 
// Componet Defines
//
/* AFCF7797-8923-4033-A431-359A7C7334F4 */
#define SB_MACREMOTECONTROLSERVICE_CID \
{ 0xAFCF7797, 0x8923, 0x4033, \
{ 0xA4, 0x31, 0x35, 0x9A, 0x7C, 0x73, 0x34, 0xF4 } }

#define SB_MACREMOTECONTROLSERVICE_CONTRACTID \ 
"@songbirdnest.com/mac-remote-service;1"


NS_GENERIC_FACTORY_CONSTRUCTOR(sbAppleRemoteService)

static const nsModuleComponentInfo components[] =
{
  {
    "Songbird Mac Remote Control Service",
    SB_MACREMOTECONTROLSERVICE_CID,
    SB_MACREMOTECONTROLSERVICE_CONTRACTID,
    sbAppleRemoteServiceConstructor
  }
};

NS_IMPL_NSGETMODULE(sbMacRemoteControllerModule, components)
