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

#include "sbRemotePlayerFactory.h"
#include "sbRemotePlayer.h"

#include <sbIRemotePlayer.h>
#include <nsIDOMWindow.h>
#include <nsIURI.h>

NS_IMPL_ISUPPORTS1(sbRemotePlayerFactory, sbIRemotePlayerFactory)

NS_IMETHODIMP
sbRemotePlayerFactory::CreatePrivileged(nsIURI* aCodebase,
                                        nsIDOMWindow* aWindow,
                                        sbIRemotePlayer** _retval)
{
  NS_ENSURE_ARG_POINTER(aCodebase);
  NS_ENSURE_ARG_POINTER(aWindow);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  nsRefPtr<sbRemotePlayer> player = new sbRemotePlayer();
  NS_ENSURE_TRUE(player, NS_ERROR_OUT_OF_MEMORY);

  rv = player->InitPrivileged(aCodebase, aWindow);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = player);
  return NS_OK;
}

