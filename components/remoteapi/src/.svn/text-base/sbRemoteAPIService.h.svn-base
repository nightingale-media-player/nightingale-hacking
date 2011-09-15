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

#ifndef __SB_REMOTEAPI_SERVICE_H__
#define __SB_REMOTEAPI_SERVICE_H__

#include "sbIRemoteAPIService.h"

#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsIURI.h"
#include "nsStringGlue.h"
#include "nsCOMArray.h"

#include "sbIDataRemote.h"

#define SONGBIRD_REMOTEAPI_SERVICE_CONTRACTID           \
  "@songbirdnest.com/remoteapi/remoteapiservice;1"
#define SONGBIRD_REMOTEAPI_SERVICE_CLASSNAME            \
  "Songbird RemoteAPI State Service"
#define SONGBIRD_REMOTEAPI_SERVICE_CID                  \
{ /* 8E380CB6-CFBC-4d80-9E4C-E035253243C9 */            \
  0x8e380cb6,                                           \
  0xcfbc,                                               \
  0x4d80,                                               \
  {0x9e, 0x4c, 0xe0, 0x35, 0x25, 0x32, 0x43, 0xc9}      \
}

class sbRemoteAPIService : public sbIRemoteAPIService,
                           public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_SBIREMOTEAPISERVICE

  sbRemoteAPIService();
  
public:
  NS_IMETHOD Init();

private:
  ~sbRemoteAPIService();

protected:
  nsCOMPtr<nsIURI> mPlaybackControllerURI;

  // list of data remotes we're observing
  nsCOMArray<sbIDataRemote> mDataRemotes;
};

#endif // __SB_REMOTEAPI_SERVICE_H__
