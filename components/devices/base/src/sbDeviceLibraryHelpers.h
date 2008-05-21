/* vim: set sw=2 :miv */
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

/**
 * \file Listener helpers for sbDeviceLibrary
 */

#include <sbIMediaListListener.h>

class sbILibrary;

/**
 * Listens to item update events to propagate into a second library
 * The target library must register this listener before going away.
 */
class sbLibraryItemUpdateListener : public sbIMediaListListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTLISTENER
  
  sbLibraryItemUpdateListener(sbILibrary* aTargetLibrary);
  
protected:
  /**
   * The target library holds a reference to us (to unregister), so it is
   * safe to not own a reference
   */
  sbILibrary* mTargetLibrary;
};
