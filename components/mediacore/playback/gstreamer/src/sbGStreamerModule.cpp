/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

#include <nsXPCOM.h>
#include <nsCOMPtr.h>
#include <nsICategoryManager.h>
#include <nsIGenericFactory.h>
#include <nsServiceManagerUtils.h>
#include <prlog.h>

#include "sbGStreamerService.h"
#include "sbGStreamerSimple.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbGStreamerService, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbGStreamerSimple)

static const nsModuleComponentInfo components[] =
{
  {
    SBGSTREAMERSERVICE_CLASSNAME,
    SBGSTREAMERSERVICE_CID,
    SBGSTREAMERSERVICE_CONTRACTID,
    sbGStreamerServiceConstructor
  },
  {
    SBGSTREAMERSIMPLE_CLASSNAME,
    SBGSTREAMERSIMPLE_CID,
    SBGSTREAMERSIMPLE_CONTRACTID,
    sbGStreamerSimpleConstructor
  }
};

//-----------------------------------------------------------------------------
#if defined( PR_LOGGING )
PRLogModuleInfo *gGStreamerLog;

// setup nspr logging ...
PR_STATIC_CALLBACK(nsresult)
InitGStreamer(nsIModule *self)
{
  gGStreamerLog = PR_NewLogModule("gstreamer");
  return NS_OK;
}
#else
#define InitGStreamer nsnull
#endif

NS_IMPL_NSGETMODULE_WITH_CTOR(sbGStreamerModule, components, InitGStreamer)
