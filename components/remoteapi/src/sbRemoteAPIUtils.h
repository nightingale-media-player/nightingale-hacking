/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

#ifndef __SB_REMOTE_API_UTILS_H__
#define __SB_REMOTE_API_UTILS_H__

#include <sbILibrary.h>
#include <sbILibraryResource.h>
#include <sbIMediaItem.h>
#include <sbIMediaList.h>
#include <sbIMediaListView.h>
#include <sbIRemoteMediaList.h>

#include "sbRemoteLibraryBase.h"
#include "sbRemoteMediaItem.h"
#include "sbRemoteMediaList.h"
#include "sbRemoteSiteMediaItem.h"
#include "sbRemoteSiteMediaList.h"

// Figures out if the media item passed in is from the web or main
// libraries by comparing the GUIDs from the item and the default libraries
static nsresult
SB_IsFromDefaultLib( sbIMediaItem *aMediaItem, PRBool *aDefault )
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aDefault);

  nsCOMPtr<sbILibrary> library;
  nsresult rv = aMediaItem->GetLibrary( getter_AddRefs(library) );
  NS_ENSURE_SUCCESS( rv, rv );

  // Get the guid from the library
  nsCOMPtr<sbILibraryResource> res( do_QueryInterface( library, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );
  nsAutoString guid;
  rv = res->GetGuid(guid);
  NS_ENSURE_SUCCESS( rv, rv );

  // Get the guids for the default libs
  nsAutoString mainGuid;
  rv = sbRemoteLibraryBase::GetLibraryGUID( NS_LITERAL_STRING("main"), mainGuid );
  NS_ENSURE_SUCCESS( rv, rv );
  nsAutoString webGuid;
  rv = sbRemoteLibraryBase::GetLibraryGUID( NS_LITERAL_STRING("web"), webGuid );
  NS_ENSURE_SUCCESS( rv, rv );

  if ( guid == mainGuid || guid == webGuid )
    *aDefault = PR_TRUE;
  else
    *aDefault = PR_FALSE;

  return NS_OK;
}

// given an existing list, create a view and hand back
// the wrapped list
static nsresult
SB_WrapMediaList(sbIMediaList* aMediaList,
                 sbIMediaList** aRemoteMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aRemoteMediaList);

  nsresult rv;

  // Create the view for the list
  nsCOMPtr<sbIMediaListView> mediaListView;
  rv = aMediaList->CreateView(getter_AddRefs(mediaListView));
  NS_ENSURE_SUCCESS(rv, rv);

  // Find out if this is a default or site library
  PRBool isDefault;
  nsCOMPtr<sbIMediaItem> item( do_QueryInterface( aMediaList, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );
  rv = SB_IsFromDefaultLib( item, &isDefault );
  NS_ENSURE_SUCCESS( rv, rv );

  if (isDefault) {
    nsRefPtr<sbRemoteMediaList> remoteMediaList;
    remoteMediaList = new sbRemoteMediaList( aMediaList, mediaListView );
    NS_ENSURE_TRUE( remoteMediaList, NS_ERROR_OUT_OF_MEMORY );
    rv = remoteMediaList->Init();
    NS_ENSURE_SUCCESS( rv, rv );
    NS_ADDREF( *aRemoteMediaList = remoteMediaList );
  } else {
    nsRefPtr<sbRemoteSiteMediaList> remoteMediaList;
    remoteMediaList = new sbRemoteSiteMediaList( aMediaList, mediaListView );
    NS_ENSURE_TRUE( remoteMediaList, NS_ERROR_OUT_OF_MEMORY );
    rv = remoteMediaList->Init();
    NS_ENSURE_SUCCESS( rv, rv );
    NS_ADDREF( *aRemoteMediaList = remoteMediaList );
  }

  return NS_OK;
}

// given an existing item, create either a wrapped list
// or wrapped item, handing it back as an item
static nsresult
SB_WrapMediaItem(sbIMediaItem* aMediaItem,
                 sbIMediaItem** aRemoteMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aRemoteMediaItem);

  nsresult rv;

  nsCOMPtr<sbIMediaList> mediaList = do_QueryInterface(aMediaItem, &rv);
  if (NS_SUCCEEDED(rv)) {

    nsCOMPtr<sbIMediaList> remoteMediaList;
    SB_WrapMediaList( mediaList, getter_AddRefs(remoteMediaList) );

    NS_ADDREF(*aRemoteMediaItem = remoteMediaList);
  }
  else {
    // Find out if this is a default or site library
    PRBool isDefault;
    rv = SB_IsFromDefaultLib( aMediaItem, &isDefault );
    NS_ENSURE_SUCCESS( rv, rv );

    if (isDefault) {
      nsRefPtr<sbRemoteMediaItem> remoteMediaItem;
      remoteMediaItem = new sbRemoteMediaItem(aMediaItem);
      NS_ENSURE_TRUE(remoteMediaItem, NS_ERROR_OUT_OF_MEMORY);
      rv = remoteMediaItem->Init();
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ADDREF(*aRemoteMediaItem = remoteMediaItem);
    } else {
      nsRefPtr<sbRemoteSiteMediaItem> remoteMediaItem;
      remoteMediaItem = new sbRemoteSiteMediaItem(aMediaItem);
      NS_ENSURE_TRUE(remoteMediaItem, NS_ERROR_OUT_OF_MEMORY);
      rv = remoteMediaItem->Init();
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ADDREF(*aRemoteMediaItem = remoteMediaItem);
    }
  }

  return NS_OK;
}

// given an exiting list, create a view and hand back a wrapped
// list. This is merely just a wrapper around the other method to QI
// to a sbIRemoteMediaList
static nsresult
SB_WrapMediaList(sbIMediaList* aMediaList,
                 sbIRemoteMediaList** aRemoteMediaList)
{
  nsCOMPtr<sbIMediaList> mediaList;
  nsresult rv = SB_WrapMediaList(aMediaList, getter_AddRefs(mediaList));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIRemoteMediaList> remoteMediaList =
    do_QueryInterface(mediaList, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aRemoteMediaList = remoteMediaList);
  return NS_OK;
}

// given an existing view, walk back in to the list and
// hand back the wrapped list
static nsresult
SB_WrapMediaList(sbIMediaListView* aMediaListView,
                 sbIRemoteMediaList** aRemoteMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaListView);
  NS_ENSURE_ARG_POINTER(aRemoteMediaList);

  // Get the list from the view passed in
  nsCOMPtr<sbIMediaList> mediaList;
  nsresult rv = aMediaListView->GetMediaList( getter_AddRefs(mediaList) );
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the library to which the list belongs
  nsCOMPtr<sbIMediaItem> item( do_QueryInterface( mediaList, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );

  PRBool isDefault;
  rv = SB_IsFromDefaultLib( item, &isDefault );
  NS_ENSURE_SUCCESS( rv, rv );

  if (isDefault) {
    nsRefPtr<sbRemoteMediaList> remoteMediaList;
    remoteMediaList = new sbRemoteMediaList(mediaList, aMediaListView);
    NS_ENSURE_TRUE(remoteMediaList, NS_ERROR_OUT_OF_MEMORY);
    rv = remoteMediaList->Init();
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ADDREF(*aRemoteMediaList = remoteMediaList);
  } else {
    nsRefPtr<sbRemoteSiteMediaList> remoteMediaList;
    remoteMediaList = new sbRemoteSiteMediaList(mediaList, aMediaListView);
    NS_ENSURE_TRUE(remoteMediaList, NS_ERROR_OUT_OF_MEMORY);
    rv = remoteMediaList->Init();
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ADDREF(*aRemoteMediaList = remoteMediaList);
  }

  return NS_OK;
}

#endif // __SB_REMOTE_API_H__

