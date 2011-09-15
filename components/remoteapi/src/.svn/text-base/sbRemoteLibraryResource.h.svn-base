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

#ifndef __SB_REMOTE_LIBRARY_RESOURCE_H__
#define __SB_REMOTE_LIBRARY_RESOURCE_H__

#include <sbIMediaItem.h>
#include <sbILibraryResource.h>

#include "sbRemoteAPI.h"
#include "sbRemotePlayer.h"
#include "sbRemoteForwardingMacros.h"
#include <nsCOMPtr.h>

#ifdef PR_LOGGING
extern PRLogModuleInfo *gRemoteLibResLog;
#endif

#define LOG_RES(args) PR_LOG( gRemoteLibResLog, PR_LOG_WARN, args )

class sbRemoteLibraryResource : public sbILibraryResource
{

public:
  NS_DECL_ISUPPORTS
  NS_FORWARD_SAFE_SBILIBRARYRESOURCE_NO_SETGETPROPERTY_SETPROPERTIES(mMediaItem);

  NS_IMETHOD GetProperty( const nsAString& aID, nsAString& _retval );
  NS_IMETHOD SetProperty( const nsAString& aID, const nsAString& aValue );
  NS_IMETHOD SetProperties( sbIPropertyArray *aProperties );

  sbRemoteLibraryResource( sbRemotePlayer *aRemotePlayer,
                           sbIMediaItem *aMediaItem );
  virtual ~sbRemoteLibraryResource();

protected:

  nsRefPtr<sbRemotePlayer> mRemotePlayer;
  nsCOMPtr<sbIMediaItem> mMediaItem;
};

#endif // __SB_REMOTE_LIBRARY_RESOURCE_H__

