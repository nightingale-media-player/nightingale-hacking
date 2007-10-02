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
#include "sbRemoteWebMediaItem.h"
#include "sbRemoteWebMediaList.h"
#include "sbRemotePlayer.h"

// Figures out if the media item passed in is from the named
// library by comparing the GUIDs from the item and the named library
static nsresult
SB_IsFromLibName( sbIMediaItem *aMediaItem,
                  const nsAString &aLibName,
                  PRBool *aIsFromLib )
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aIsFromLib);

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
  nsAutoString libGuid;
  rv = sbRemoteLibraryBase::GetLibraryGUID( aLibName, libGuid );
  NS_ENSURE_SUCCESS( rv, rv );

  if ( guid == libGuid )
    *aIsFromLib = PR_TRUE;
  else
    *aIsFromLib = PR_FALSE;

  return NS_OK;
}

// given an existing list, create a view and hand back
// the wrapped list
static nsresult
SB_WrapMediaList( sbRemotePlayer *aRemotePlayer,
                  sbIMediaList *aMediaList,
                  sbIMediaList **aRemoteMediaList )
{
  NS_ENSURE_ARG_POINTER(aRemotePlayer);
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aRemoteMediaList);

  nsresult rv;

  // Create the view for the list
  nsCOMPtr<sbIMediaListView> mediaListView;
  rv = aMediaList->CreateView( nsnull, getter_AddRefs(mediaListView) );
  NS_ENSURE_SUCCESS(rv, rv);

  // Find out if this is a default or site library
  PRBool isMain;
  nsCOMPtr<sbIMediaItem> item( do_QueryInterface( aMediaList, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );
  rv = SB_IsFromLibName( item, NS_LITERAL_STRING("main"), &isMain );
  NS_ENSURE_SUCCESS( rv, rv );
  PRBool isWeb;
  rv = SB_IsFromLibName( item, NS_LITERAL_STRING("web"), &isWeb );
  NS_ENSURE_SUCCESS( rv, rv );

  nsRefPtr<sbRemoteMediaList> remoteMediaList;
  if (isMain) {
    remoteMediaList = new sbRemoteMediaList( aRemotePlayer,
                                             aMediaList,
                                             mediaListView );
  } else if (isWeb) {
    remoteMediaList = new sbRemoteWebMediaList( aRemotePlayer,
                                                aMediaList,
                                                mediaListView );
  } else {
    remoteMediaList = new sbRemoteSiteMediaList( aRemotePlayer,
                                                 aMediaList,
                                                 mediaListView );
  }
  NS_ENSURE_TRUE( remoteMediaList, NS_ERROR_OUT_OF_MEMORY );
  rv = remoteMediaList->Init();
  NS_ENSURE_SUCCESS( rv, rv );

  rv = CallQueryInterface( remoteMediaList.get(), aRemoteMediaList );
  NS_ENSURE_SUCCESS( rv, rv );

  return NS_OK;
}

// given an existing item, create either a wrapped list
// or wrapped item, handing it back as an item
static nsresult
SB_WrapMediaItem( sbRemotePlayer *aRemotePlayer,
                  sbIMediaItem *aMediaItem,
                  sbIMediaItem **aRemoteMediaItem )
{
  NS_ENSURE_ARG_POINTER(aRemotePlayer);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aRemoteMediaItem);

  nsresult rv;

  nsCOMPtr<sbIMediaList> mediaList = do_QueryInterface( aMediaItem, &rv );
  if ( NS_SUCCEEDED(rv) ) {

    nsCOMPtr<sbIMediaList> remoteMediaList;
    rv = SB_WrapMediaList( aRemotePlayer,
                           mediaList,
                           getter_AddRefs(remoteMediaList) );
    NS_ENSURE_SUCCESS( rv, rv );

    rv = CallQueryInterface( remoteMediaList.get(), aRemoteMediaItem );
    NS_ENSURE_SUCCESS( rv, rv );
  }
  else {
    // Find out if this is a default or site library
    PRBool isMain;
    rv = SB_IsFromLibName( aMediaItem, NS_LITERAL_STRING("main"), &isMain );
    NS_ENSURE_SUCCESS( rv, rv );
    PRBool isWeb;
    rv = SB_IsFromLibName( aMediaItem, NS_LITERAL_STRING("web"), &isWeb );
    NS_ENSURE_SUCCESS( rv, rv );

    nsRefPtr<sbRemoteMediaItem> remoteMediaItem;
    if (isMain) {
      remoteMediaItem = new sbRemoteMediaItem( aRemotePlayer, aMediaItem );
    } else if (isWeb) {
      remoteMediaItem = new sbRemoteWebMediaItem( aRemotePlayer, aMediaItem );
    } else {
      remoteMediaItem = new sbRemoteSiteMediaItem( aRemotePlayer, aMediaItem );
    }
    NS_ENSURE_TRUE( remoteMediaItem, NS_ERROR_OUT_OF_MEMORY );
    rv = remoteMediaItem->Init();
    NS_ENSURE_SUCCESS( rv, rv );

    rv = CallQueryInterface( remoteMediaItem.get(), aRemoteMediaItem );
    NS_ENSURE_SUCCESS( rv, rv );
  }

  return NS_OK;
}

// given an exiting list, create a view and hand back a wrapped
// list. This is merely just a wrapper around the other method to QI
// to a sbIRemoteMediaList
static nsresult
SB_WrapMediaList( sbRemotePlayer *aRemotePlayer,
                  sbIMediaList *aMediaList,
                  sbIRemoteMediaList **aRemoteMediaList )
{
  NS_ENSURE_ARG_POINTER(aRemotePlayer);
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aRemoteMediaList);

  nsCOMPtr<sbIMediaList> mediaList;
  nsresult rv = SB_WrapMediaList( aRemotePlayer, 
                                  aMediaList,
                                  getter_AddRefs(mediaList) );
  NS_ENSURE_SUCCESS( rv, rv );

  nsCOMPtr<sbIRemoteMediaList> remoteMediaList =
    do_QueryInterface( mediaList, &rv );
  NS_ENSURE_SUCCESS( rv, rv );

  rv = CallQueryInterface( remoteMediaList.get(), aRemoteMediaList );
  NS_ENSURE_SUCCESS( rv, rv );

  return NS_OK;
}

// given an existing view, walk back in to the list and
// hand back the wrapped list
static nsresult
SB_WrapMediaList( sbRemotePlayer *aRemotePlayer,
                  sbIMediaListView *aMediaListView,
                  sbIRemoteMediaList **aRemoteMediaList )
{
  NS_ENSURE_ARG_POINTER(aRemotePlayer);
  NS_ENSURE_ARG_POINTER(aMediaListView);
  NS_ENSURE_ARG_POINTER(aRemoteMediaList);

  // Get the list from the view passed in
  nsCOMPtr<sbIMediaList> mediaList;
  nsresult rv = aMediaListView->GetMediaList( getter_AddRefs(mediaList) );
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the library to which the list belongs
  nsCOMPtr<sbIMediaItem> item( do_QueryInterface( mediaList, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );

  // Find out if this is a default or site library
  PRBool isMain;
  rv = SB_IsFromLibName( item, NS_LITERAL_STRING("main"), &isMain );
  NS_ENSURE_SUCCESS( rv, rv );
  PRBool isWeb;
  rv = SB_IsFromLibName( item, NS_LITERAL_STRING("web"), &isWeb );
  NS_ENSURE_SUCCESS( rv, rv );

  nsRefPtr<sbRemoteMediaList> remoteMediaList;
  if (isMain) {
    remoteMediaList = new sbRemoteMediaList( aRemotePlayer,
                                             mediaList,
                                             aMediaListView );
  } else if (isWeb) {
    remoteMediaList = new sbRemoteWebMediaList( aRemotePlayer,
                                                mediaList,
                                                aMediaListView );
  } else {
    remoteMediaList = new sbRemoteSiteMediaList( aRemotePlayer,
                                                 mediaList,
                                                 aMediaListView );
  }
  NS_ENSURE_TRUE( remoteMediaList, NS_ERROR_OUT_OF_MEMORY );
  rv = remoteMediaList->Init();
  NS_ENSURE_SUCCESS( rv, rv );

  rv = CallQueryInterface( remoteMediaList.get(), aRemoteMediaList );
  NS_ENSURE_SUCCESS( rv, rv );

  return NS_OK;
}

#endif // __SB_REMOTE_API_H__

