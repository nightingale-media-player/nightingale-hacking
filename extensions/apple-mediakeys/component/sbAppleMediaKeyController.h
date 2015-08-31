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

#ifndef sbAppleMediaKeyController_h_
#define sbAppleMediaKeyController_h_

#include <nsIObserver.h>
#include <nsIComponentManager.h>
#include <nsIGenericFactory.h>
#include <nsIFile.h>


//------------------------------------------------------------------------------
//
// @brief XPCOM service to listen to media key events in Mac OS X and update
//        the Songbird player as needed.
//
//------------------------------------------------------------------------------

class sbAppleMediaKeyController : public nsIObserver
{
public:
  sbAppleMediaKeyController();
  virtual ~sbAppleMediaKeyController();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  NS_IMETHOD Init();  // does this really need to be virtual

  static NS_METHOD RegisterSelf(nsIComponentManager *aCompMgr,
                                nsIFile *aPath,
                                const char *aLoaderStr,
                                const char *aType,
                                const nsModuleComponentInfo *aInfo);
};


//------------------------------------------------------------------------------
// Component Registration Stuff:

#define SONGBIRD_APPLEMEDIAKEYCONTROLLER_CONTRACTID                \
  "@songbirdnest.com/apple-media-key-controller;1"
#define SONGBIRD_APPLEMEDIAKEYCONTROLLER_CLASSNAME                 \
  "Songbird Apple Media Key Controller" 
#define SONGBIRD_APPLEMEDIAKEYCONTROLLER_CID                       \
{ /* ECA6F002-175E-4FF4-B46D-ACE45680DBC5 */              \
  0xECA6F002,                                             \
  0x175E,                                                 \
  0x4FF4,                                                 \
  {0xB4, 0x6D, 0xAC, 0xE4, 0x56, 0x80, 0xDB, 0xC5}        \
}

#endif  // sbAppleMediaKeyController_h_

