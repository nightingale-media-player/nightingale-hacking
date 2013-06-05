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

#ifndef __SB_REMOTE_MEDIALIST_H__
#define __SB_REMOTE_MEDIALIST_H__

#include "sbRemoteAPI.h"
#include "sbRemoteMediaListBase.h"

#include <sbIMediaList.h>
#include <sbIMediaListView.h>

#include <nsIClassInfo.h>

#define SB_REMOMEDIALIST_CID                            \
{ /* 7bd2a7dd-aa83-4f2e-97d4-db7a612e18da */            \
  0x7bd2a7dd,                                           \
  0xaa83,                                               \
  0x4f2e,                                               \
  {0x97, 0xd4, 0xdb, 0x7a, 0x61, 0x2e, 0x18, 0xda}      \
}                                                       \



class sbRemotePlayer;

class sbRemoteMediaList : public sbRemoteMediaListBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSICLASSINFO
  SB_DECL_SECURITYCHECKEDCOMP_INIT

  sbRemoteMediaList( sbRemotePlayer* aRemotePlayer,
                     sbIMediaList* aMediaList,
                     sbIMediaListView* aMediaListView );

protected:
  virtual ~sbRemoteMediaList();

};

#endif // __SB_REMOTE_MEDIALIST_H__

