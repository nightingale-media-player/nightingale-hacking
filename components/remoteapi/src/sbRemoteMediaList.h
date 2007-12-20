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

