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

#ifndef sbMacAppDelegate_h_
#define sbMacAppDelegate_h_

#include <nsIObserver.h>
#include <nsIComponentManager.h>
#include <nsIGenericFactory.h>
#include <nsIFile.h>
#include <nsCOMPtr.h>

//
// \brief This class simply registers with the component manager
//        for startup. Once the app has started up, it will override
//        the XULRunner Mac application delegate so Nightingale can
//        handle Mac specific events like Dock creation.
//
@class SBMacAppDelegate;

class sbMacAppDelegateManager : public nsIObserver
{
public:
  sbMacAppDelegateManager();
  virtual ~sbMacAppDelegateManager();
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  NS_IMETHOD Init();

  static NS_METHOD RegisterSelf(nsIComponentManager* aCompMgr,
                                nsIFile* aPath,
                                const char* aLoaderStr,
                                const char* aType,
                                const nsModuleComponentInfo *aInfo);
private:
  SBMacAppDelegate *mDelegate;
};


// Component Registration Stuff:
#define NIGHTINGALE_MACAPPDELEGATEMANAGER_CONTRACTID                \
  "@getnightingale.com/integration/mac-app-delegate-mgr;1"
#define NIGHTINGALE_MACAPPDELEGATEMANAGER_CLASSNAME                 \
  "Nightingale Mac Application Delegate Manager" 
#define NIGHTINGALE_MACAPPDELEGATEMANAGER_CID                       \
{ /* CB64A891-2D63-4A71-850F-26201F7370D6 */              \
  0xCB64A891,                                             \
  0x2D63,                                                 \
  0x4A71,                                                 \
  {0x85, 0x0F, 0x26, 0x20, 0x1F, 0x73, 0x70, 0xD6}        \
}

#endif  // sbMacAppDelegate_h_

