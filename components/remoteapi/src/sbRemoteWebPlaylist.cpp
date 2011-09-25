/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
 */

#include "sbRemoteAPIUtils.h"
#include "sbRemotePlayer.h"
#include "sbRemoteWebPlaylist.h"
#include "sbRemoteWrappingSimpleEnumerator.h"

#include <sbClassInfoUtils.h>
#include <sbIPropertyManager.h>
#include <sbIRemoteMediaList.h>
#include <sbIMediaListView.h>
#include <sbIMediaListViewSelection.h>
#include <sbPropertiesCID.h>
#include <sbIPropertyBuilder.h>

#include <nsAutoPtr.h>
#include <nsIDOMElement.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>
#include <prlog.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteWebPlaylist:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gRemoteWebPlaylistLog = nsnull;
#endif

#undef LOG
#define LOG(args) PR_LOG(gRemoteWebPlaylistLog, PR_LOG_WARN, args)

const static char* sPublicWProperties[] =
{
  "site:mediaList",
  "site:hidden"
};

const static char* sPublicRProperties[] =
{
  // sbIRemoteWebPlaylist
  "site:selection",
  "site:mediaList",
  "site:hidden",

  // nsIClassInfo
  "classinfo:classDescription",
  "classinfo:contractID",
  "classinfo:classID",
  "classinfo:implementationLanguage",
  "classinfo:flags"
};

const static char* sPublicMethods[] =
{ 
  // sbIRemoteWebPlaylist
  "site:appendColumn",
  "site:setSelectionByIndex",
  "site:showColumnBefore",

  // sbIPlaylistWidget
  "site:showColumn",
  "site:hideColumn",
  "site:getColumnCount",
  "site:clearColumns",
  "site:insertColumnBefore",
  "site:removeColumn",
  "site:getColumnPropertyIDByIndex",
  "site:setSortColumn",
  "internal:getView"
};

NS_IMPL_ISUPPORTS5( sbRemoteWebPlaylist,
                    nsIClassInfo,
                    nsISecurityCheckedComponent,
                    sbISecurityAggregator,
                    sbIRemoteWebPlaylist,
                    sbIPlaylistWidget )

NS_IMPL_CI_INTERFACE_GETTER4( sbRemoteWebPlaylist,
                              sbISecurityAggregator,
                              sbIRemoteWebPlaylist,
                              sbIPlaylistWidget,
                              nsISecurityCheckedComponent )

SB_IMPL_CLASSINFO_INTERFACES_ONLY( sbRemoteWebPlaylist )

SB_IMPL_SECURITYCHECKEDCOMP_INIT( sbRemoteWebPlaylist )

sbRemoteWebPlaylist::sbRemoteWebPlaylist( sbRemotePlayer *aRemotePlayer,
                                          sbIPlaylistWidget *aWebPlaylistWidget,
                                          sbITabBrowserTab *aTabBrowserTab ) :
  mInitialized(PR_FALSE),
  mRemotePlayer(aRemotePlayer),
  mPlaylistWidget(aWebPlaylistWidget),
  mOwnerTab(aTabBrowserTab)
{
  NS_ASSERTION( aRemotePlayer, "Null remote player!" );
  NS_ASSERTION( aWebPlaylistWidget, "Null playlist widget!" );
  NS_ASSERTION( aTabBrowserTab, "Null playlist widget!" );

#ifdef PR_LOGGING
  if (!gRemoteWebPlaylistLog) {
    gRemoteWebPlaylistLog = PR_NewLogModule("sbRemoteWebPlaylist");
  }
  LOG(("sbRemoteWebPlaylist::sbRemoteWebPlaylist()"));
#endif
}

// ---------------------------------------------------------------------------
//
//                          sbISecurityAggregator
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP 
sbRemoteWebPlaylist::GetRemotePlayer(sbIRemotePlayer * *aRemotePlayer)
{
  NS_ENSURE_STATE(mRemotePlayer);
  NS_ENSURE_ARG_POINTER(aRemotePlayer);

  nsresult rv;
  *aRemotePlayer = nsnull;

  nsCOMPtr<sbIRemotePlayer> remotePlayer;

  rv = mRemotePlayer->QueryInterface( NS_GET_IID( sbIRemotePlayer ), getter_AddRefs( remotePlayer ) );
  NS_ENSURE_SUCCESS( rv, rv );

  remotePlayer.swap( *aRemotePlayer );

  return NS_OK;
}

// ---------------------------------------------------------------------------
//
//                        sbIRemoteWebPlaylist
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbRemoteWebPlaylist::SetMediaList( sbIRemoteMediaList *aMediaList )
{
  LOG(("sbRemoteWebPlaylist::SetMediaList()"));
  NS_ENSURE_ARG_POINTER(aMediaList);

  nsresult rv;
  nsCOMPtr<sbIMediaListView> mediaListView;
  rv = aMediaList->GetView( getter_AddRefs(mediaListView) );
  if (!mediaListView) {
    // Create a view from the RemoteMediaList passed in
    nsCOMPtr<sbIMediaList> webMediaList = do_QueryInterface(aMediaList);
    NS_ENSURE_TRUE( webMediaList, NS_ERROR_INVALID_ARG );

    rv = webMediaList->CreateView( nsnull, getter_AddRefs(mediaListView) );
    NS_ENSURE_TRUE( mediaListView, NS_ERROR_FAILURE );
  }

  rv = mOwnerTab->DisableScan();
  NS_ENSURE_SUCCESS( rv, rv );

  rv = mOwnerTab->ClearOuterPlaylist();
  NS_ENSURE_SUCCESS( rv, rv );

  // Bind the view to the playlist widget
  rv = Bind( mediaListView, nsnull, PR_FALSE, PR_FALSE );
  NS_ENSURE_SUCCESS( rv, rv );

  nsCOMPtr<nsIDOMElement> playlist ( do_QueryInterface( mPlaylistWidget,
                                                        &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );

  //mOwnerTab->SetOuterPlaylistShowing(PR_TRUE);
  mOwnerTab->ShowOuterPlaylist();
  //rv = playlist->SetAttribute( NS_LITERAL_STRING("hidden"),
                               //NS_LITERAL_STRING("false") );
  NS_ENSURE_SUCCESS( rv, rv );

  // Update the commands because the widget has been reset and
  // needs to know the useDefaultCommands setting
  RescanCommands();

  return NS_OK;
}

NS_IMETHODIMP
sbRemoteWebPlaylist::GetMediaList( sbIRemoteMediaList **aMediaList )
{
  LOG(("sbRemoteWebPlaylist::GetMediaList()"));
  NS_ENSURE_ARG_POINTER(aMediaList);

  // get the view from the web playlist widget
  nsresult rv;
  nsCOMPtr<sbIMediaListView> mediaListView;
  rv = GetListView( getter_AddRefs(mediaListView) );
  NS_ENSURE_SUCCESS( rv, rv );

  // the getter call can succeed and return null, better check it
  if (!mediaListView) {
    LOG(("sbRemoteWebPlaylist::GetMediaList() - NO MEDIA LIST VIEW" ));
    // return error condition so a webpage can catch it.
    return NS_ERROR_FAILURE;
  }

  // create wrapped medialist, this addrefs
  return SB_WrapMediaList( mRemotePlayer, mediaListView, aMediaList );
}

NS_IMETHODIMP
sbRemoteWebPlaylist::GetSelection( nsISimpleEnumerator **aSelection )
{
  LOG(("sbRemoteWebPlaylist::GetSelection()"));

  NS_ASSERTION( mPlaylistWidget, "Playlist widget not set" );

  // Get the current view
  nsCOMPtr<sbIMediaListView> mediaListView;
  nsresult rv = GetListView( getter_AddRefs(mediaListView) );
  NS_ENSURE_SUCCESS( rv, rv );

  nsCOMPtr<sbIMediaListViewSelection> viewSelection;
  mediaListView->GetSelection( getter_AddRefs(viewSelection) );
  NS_ENSURE_TRUE( viewSelection, NS_ERROR_UNEXPECTED );

  // get enumeration of selected items
  nsCOMPtr<nsISimpleEnumerator> selection;
  rv = viewSelection->GetSelectedIndexedMediaItems( getter_AddRefs(selection) );
  NS_ENSURE_SUCCESS( rv, rv );

  // wrap it so it's safe
  nsRefPtr<sbRemoteWrappingSimpleEnumerator> wrapped(
                             new sbRemoteWrappingSimpleEnumerator(mRemotePlayer, selection) );
  NS_ENSURE_TRUE( wrapped, NS_ERROR_OUT_OF_MEMORY );

  rv = wrapped->Init();
  NS_ENSURE_SUCCESS( rv, rv );

  NS_ADDREF( *aSelection = wrapped );
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteWebPlaylist::SetSelectionByIndex( PRUint32 aIndex, PRBool aSelected )
{
  LOG(("sbRemoteWebPlaylist::SetSelectionByIndex()"));

  NS_ASSERTION( mPlaylistWidget, "Playlist widget not set" );

  nsCOMPtr<sbIMediaListView> mediaListView;
  nsresult rv = GetListView( getter_AddRefs(mediaListView) );
  NS_ENSURE_SUCCESS( rv, rv );

  nsCOMPtr<sbIMediaListViewSelection> viewSelection;
  mediaListView->GetSelection( getter_AddRefs(viewSelection) );
  NS_ENSURE_TRUE( viewSelection, NS_ERROR_UNEXPECTED );

  if (aSelected) {
    rv = viewSelection->Select(aIndex);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = viewSelection->Clear(aIndex);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbRemoteWebPlaylist::GetPlaylistWidget( sbIPlaylistWidget **aWebPlaylist )
{
  LOG(("sbRemoteWebPlaylist::SetSelectionByIndex()"));
  NS_ENSURE_ARG_POINTER(aWebPlaylist);
  NS_IF_ADDREF( *aWebPlaylist = mPlaylistWidget );
  return NS_OK;
}

nsresult
sbRemoteWebPlaylist::SetHidden(PRBool aHide)
{
  LOG(("sbRemoteWebPlaylist::SetHidden()"));
  
  nsresult rv = NS_OK;

  if (aHide) {
    rv = mOwnerTab->HideOuterPlaylist();
  } else {
    rv = mOwnerTab->ShowOuterPlaylist();
  }

  return rv;
}

nsresult
sbRemoteWebPlaylist::GetHidden(PRBool* aHidden)
{
  NS_ENSURE_ARG_POINTER(aHidden);
  LOG(("sbRemoteWebPlaylist::GetHidden()"));

  nsresult rv = NS_OK;
  PRBool showing;
  rv = mOwnerTab->GetOuterPlaylistShowing(&showing);
  NS_ENSURE_SUCCESS(rv, rv);

  *aHidden = !showing;

  return rv;
}
