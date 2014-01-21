/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
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

/*
 * Written by Logan F. Smyth Š 2009
 * http://logansmyth.com
 * me@logansmyth.com
 */


#include "nsIGenericFactory.h"
#include "ngDbusConnection.h"


NS_GENERIC_FACTORY_CONSTRUCTOR(ngDbusConnection)


static nsModuleComponentInfo components[] =
{
	{
		"Dbus Protocol plugin", 
		{ 0x939381dc, 0x1747, 0x4d3e, { 0xb2, 0xf7, 0x50, 0x53, 0xe5, 0xfb, 0x7d, 0xa4 } },
		"@getnightingale.com/Nightingale/DbusConnection;1",
		ngDbusConnectionConstructor,
	}
};

NS_IMPL_NSGETMODULE("ngDbusConnectionModule", components)

