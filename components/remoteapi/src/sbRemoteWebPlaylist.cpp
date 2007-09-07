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

#include "sbRemoteAPIUtils.h"
#include "sbRemoteWebPlaylist.h"
#include "sbRemoteWrappingSimpleEnumerator.h"

#include <sbClassInfoUtils.h>
#include <sbIPropertyManager.h>
#include <sbIRemoteMediaList.h>
#include <sbIMediaListView.h>
#include <sbIMediaListViewTreeView.h>
#include <sbPropertiesCID.h>
#include <sbIPropertyBuilder.h>

#include <nsAutoPtr.h>
#include <nsIDOMElement.h>
#include <nsITreeSelection.h>
#include <nsITreeView.h>
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
  "site:mediaList"
};

const static char* sPublicRProperties[] =
{
  // sbIRemoteWebPlaylist
  "site:selection",
  "site:mediaList",

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
  "site:addColumn",
  "site:setSelectionByIndex",
  "site:showColumnBefore",

  // sbIPlaylistWidget
  "site:showColumn",
  "site:hideColumn",
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

sbRemoteWebPlaylist::sbRemoteWebPlaylist( sbIPlaylistWidget *aWebPlaylistWidget,
                                          sbITabBrowserTab *aTabBrowserTab ) :
  mInitialized(PR_FALSE),
  mPlaylistWidget(aWebPlaylistWidget),
  mOwnerTab(aTabBrowserTab)
{
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

    rv = webMediaList->CreateView( getter_AddRefs(mediaListView) );
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
  return SB_WrapMediaList( mediaListView, aMediaList );
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

  // get to the right view interface
  nsCOMPtr<nsITreeView> treeView;
  rv = mediaListView->GetTreeView( getter_AddRefs(treeView) );
  nsCOMPtr<sbIMediaListViewTreeView> view = do_QueryInterface( treeView, &rv );
  NS_ENSURE_SUCCESS( rv, rv );

  // get enumeration of selected items
  nsCOMPtr<nsISimpleEnumerator> selection;
  rv = view->GetSelectedMediaItems( getter_AddRefs(selection) );
  NS_ENSURE_SUCCESS( rv, rv );

  // wrap it so it's safe
  nsRefPtr<sbRemoteWrappingSimpleEnumerator> wrapped(
                             new sbRemoteWrappingSimpleEnumerator(selection) );
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

  nsCOMPtr<nsITreeView> treeView ( do_QueryInterface( mediaListView, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );

  nsCOMPtr<nsITreeSelection> treeSelection;
  rv = treeView->GetSelection( getter_AddRefs(treeSelection) );
  NS_ENSURE_SUCCESS( rv, rv );

  PRBool isSelected;
  rv = treeSelection->IsSelected( aIndex, &isSelected );
  NS_ENSURE_SUCCESS( rv, rv );

  if ( isSelected != aSelected ) {
    rv = treeSelection->ToggleSelect(aIndex);
    NS_ENSURE_SUCCESS( rv, rv );
  }

  return NS_OK;
}

NS_IMETHODIMP
sbRemoteWebPlaylist::AddColumn( const nsAString& aColumnType,
                                const nsAString& aColumnName,
                                const nsAString& aDisplayName,
                                const nsAString& aBeforeColumnName,
                                PRBool aHidden,
                                PRBool aUserViewable,
                                PRBool aUserEditable,
                                PRBool aHasNullSort,
                                PRUint32 aNullSort,
                                PRUint32 aWidth )
{
  LOG(( "sbRemoteWebPlaylist::AddColumn(%s, %s, %s, %s)",
        NS_LossyConvertUTF16toASCII(aColumnType).get(),
        NS_LossyConvertUTF16toASCII(aColumnName).get(),
        NS_LossyConvertUTF16toASCII(aDisplayName).get(),
        NS_LossyConvertUTF16toASCII(aBeforeColumnName).get() ));

  NS_ASSERTION( mPlaylistWidget, "Playlist widget not set" );

  nsresult rv;
  nsCOMPtr<sbIPropertyManager> propMngr( 
                         do_GetService( SB_PROPERTYMANAGER_CONTRACTID, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );

  PRBool hasProp;
  nsCOMPtr<sbIPropertyInfo> info;
  propMngr->HasProperty( aColumnName, &hasProp );

  // NOTE: if you add more column types here, please be sure to update the
  // documentation in ../public/sbIRemoteWebPlaylist.idl

  if (!hasProp) {
    // property doesn't exist, add it.

    // Figure out which type of property we want, special types get a
    // display type set on them, the rest are just text.

    if (aColumnType.EqualsLiteral("text") ||
        aColumnType.EqualsLiteral("datetime") ||
        aColumnType.EqualsLiteral("uri") ||
        aColumnType.EqualsLiteral("number")) {
      if ( aColumnType.EqualsLiteral("text") ){
        info = do_CreateInstance( SB_TEXTPROPERTYINFO_CONTRACTID, &rv );
      } else if ( aColumnType.EqualsLiteral("datetime") ) {
        info = do_CreateInstance( SB_DATETIMEPROPERTYINFO_CONTRACTID, &rv );
      } else if ( aColumnType.EqualsLiteral("uri") ) {
        info = do_CreateInstance( SB_URIPROPERTYINFO_CONTRACTID, &rv );
      } else if ( aColumnType.EqualsLiteral("number") ) {
        info = do_CreateInstance( SB_NUMBERPROPERTYINFO_CONTRACTID, &rv );
      }
      NS_ENSURE_SUCCESS( rv, rv );

      if (info) {
        // set the data on the propinfo
        rv = info->SetName(aColumnName);
        NS_ENSURE_SUCCESS( rv, rv );
        rv = info->SetDisplayName(aDisplayName);
        NS_ENSURE_SUCCESS( rv, rv );
        rv = info->SetUserViewable(aUserViewable);
        NS_ENSURE_SUCCESS( rv, rv );
        rv = info->SetUserEditable(aUserEditable);
        NS_ENSURE_SUCCESS( rv, rv );
        if (aHasNullSort)
          info->SetNullSort(aNullSort);
      }
    }
    else {

      if (aColumnType.EqualsLiteral("button")) {
        nsCOMPtr<sbISimpleButtonPropertyBuilder> builder =
          do_CreateInstance(SB_SIMPLEBUTTONPROPERTYBUILDER_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = builder->SetPropertyName(aColumnName);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = builder->SetDisplayName(aDisplayName);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = builder->Get(getter_AddRefs(info));
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else if (aColumnType.EqualsLiteral("image")) {
        nsCOMPtr<sbIImagePropertyBuilder> builder =
          do_CreateInstance(SB_IMAGEPROPERTYBUILDER_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = builder->SetPropertyName(aColumnName);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = builder->SetDisplayName(aDisplayName);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = builder->Get(getter_AddRefs(info));
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else if (aColumnType.EqualsLiteral("downloadbutton")) {
        nsCOMPtr<sbIDownloadButtonPropertyBuilder> builder =
          do_CreateInstance(SB_DOWNLOADBUTTONPROPERTYBUILDER_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = builder->SetPropertyName(aColumnName);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = builder->SetDisplayName(aDisplayName);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = builder->Get(getter_AddRefs(info));
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        // bad type, return an error - Should we just create text by default?
        return NS_ERROR_FAILURE;
      }

    }

    // add to the property manager
    rv = propMngr->AddPropertyInfo(info);
    NS_ENSURE_SUCCESS( rv, rv );

    rv = InsertColumn( info,
                       aHidden,
                       aWidth,
                       NS_LITERAL_STRING(""),
                       aBeforeColumnName );
    NS_ENSURE_SUCCESS( rv, rv );

  } else {
    // column already exists in the system, if not hidden, show it
    if (!aHidden) {
      ShowColumnBefore( aColumnName, aBeforeColumnName );
    }
  }

  return rv;
}

NS_IMETHODIMP
sbRemoteWebPlaylist::ShowColumnBefore( const nsAString& aColumnName,
                                       const nsAString& aBeforeColumnName )
{
  LOG(( "sbRemoteWebPlaylist::ShowColumnBefore(%s, %s)",
        NS_LossyConvertUTF16toASCII(aColumnName).get(),
        NS_LossyConvertUTF16toASCII(aBeforeColumnName).get() ));

  // put the column in the right spot
  nsresult rv = MoveColumnBefore( aColumnName, aBeforeColumnName);
  NS_ENSURE_SUCCESS( rv, rv );

  // show the column
  return ShowColumn( aColumnName );
}

