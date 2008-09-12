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

#ifndef sbTestDummyMediacoreManager_h
#define sbTestDummyMediacoreManager_h

#include <nsAutoPtr.h>

#include <sbIMediacoreEventTarget.h>
#include <sbIMediacoreEventListener.h>

class sbBaseMediacoreEventTarget;
class sbIMediacoreEventTarget;

class sbTestDummyMediacoreManager : public sbIMediacoreEventTarget
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIACOREEVENTTARGET
  sbTestDummyMediacoreManager();
  virtual ~sbTestDummyMediacoreManager();
private:
  nsAutoPtr<sbBaseMediacoreEventTarget> mBaseEventTarget;
};

#define SB_TEST_DUMMY_MEDIACORE_MANAGER_DESCRIPTION              \
  "Songbird Mediacore Dummy Mediacore Manager"
#define SB_TEST_DUMMY_MEDIACORE_MANAGER_CONTRACTID               \
  "@songbirdnest.com/mediacore/sbTestDummyMediacoreManager;1"
#define SB_TEST_DUMMY_MEDIACORE_MANAGER_CLASSNAME                \
  "sbTestDummyMediacoreManager"

#define SB_TEST_DUMMY_MEDIACORE_MANAGER_CID                      \
{ /* 00831a9c-1d9d-4515-8620-5ef8c85355a4 */               \
  0x00831a9c,                                              \
  0x1d9d,                                                  \
  0x4515,                                                  \
  { 0x86, 0x20, 0x5e, 0xf8, 0xc8, 0x53, 0x55, 0xa4 }       \
}

#endif /* sbTestDummyMediacoreManager_h */
