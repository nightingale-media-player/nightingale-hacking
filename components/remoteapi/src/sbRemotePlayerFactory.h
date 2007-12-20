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

#ifndef __SB_REMOTE_PLAYER_FACTORY_H__
#define __SB_REMOTE_PLAYER_FACTORY_H__

#include <sbIRemotePlayerFactory.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>

#define SONGBIRD_REMOTEPLAYERFACTORY_CONTRACTID         \
  "@songbirdnest.com/remoteapi/remoteplayer-factory;1"
#define SONGBIRD_REMOTEPLAYERFACTORY_CLASSNAME          \
  "Songbird Remote Player Factory"
#define SONGBIRD_REMOTEPLAYERFACTORY_CID                \
{ /* df5f2b39-66b1-44ff-bb03-6bb7636d593b */            \
  0xdf5f2b39,                                           \
  0x66b1,                                               \
  0x44ff,                                               \
  {0xbb, 0x03, 0x6b, 0xb7, 0x63, 0x6d, 0x59, 0x3b}      \
}

class sbRemotePlayerFactory : public sbIRemotePlayerFactory
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIREMOTEPLAYERFACTORY
};

#endif /* __SB_REMOTE_PLAYER_FACTORY_H__ */

