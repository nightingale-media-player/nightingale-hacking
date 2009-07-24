/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#include "sbTranscodeBatchJobItem.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(sbTranscodeBatchJobItem,
                              sbITranscodeBatchJobItem)

sbTranscodeBatchJobItem::sbTranscodeBatchJobItem()
{
}

sbTranscodeBatchJobItem::~sbTranscodeBatchJobItem()
{
}

NS_IMETHODIMP
sbTranscodeBatchJobItem::GetMediaItem(sbIMediaItem **aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);

  NS_IF_ADDREF(*aMediaItem = mMediaItem);
  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeBatchJobItem::SetMediaItem(sbIMediaItem *aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);

  mMediaItem = aMediaItem;
  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeBatchJobItem::GetDestinationFile(nsIFile **aDestinationFile)
{
  NS_ENSURE_ARG_POINTER(aDestinationFile);

  NS_IF_ADDREF(*aDestinationFile = mDestinationFile);
  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeBatchJobItem::SetDestinationFile(nsIFile *aDestinationFile)
{
  NS_ENSURE_ARG_POINTER(aDestinationFile);

  mDestinationFile = aDestinationFile;
  return NS_OK;
}

