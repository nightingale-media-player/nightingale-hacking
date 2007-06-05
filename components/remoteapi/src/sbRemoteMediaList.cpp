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

#include "sbRemoteMediaList.h"
#include "sbRemoteLibrary.h"
#include "sbRemoteWrappingSimpleEnumerator.h"

#include <sbClassInfoUtils.h>
#include <sbIRemoteMediaList.h>
#include <sbIMediaItem.h>
#include <sbIMediaList.h>
#include <sbIMediaListView.h>
#include <sbIMediaListViewTreeView.h>
#include <sbIWrappedMediaItem.h>
#include <sbIWrappedMediaList.h>

#include <nsComponentManagerUtils.h>
#include <nsICategoryManager.h>
#include <nsIProgrammingLanguage.h>
#include <nsIScriptNameSpaceManager.h>
#include <nsIScriptSecurityManager.h>
#include <nsITreeSelection.h>
#include <nsITreeView.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>
#include <prlog.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteMediaList:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gRemoteMediaListLog = nsnull;
#define LOG(args)   if (gRemoteMediaListLog) PR_LOG(gRemoteMediaListLog, PR_LOG_WARN, args)
#else
#define LOG(args)   /* nothing */
#endif

// prefixes for theses strings can be:
// metadata:
// library:
// classinfo:
// controls:
// binding:
// internal:
const static char* sPublicWProperties[] =
{
  // sbIMediaItem
  "library:mediaCreated",
  "library:mediaUpdated",
  "library:contentSrc",
  "library:contentLength",
  "library:contentType",

  // sbIMediaList
  "library:name"
};

const static char* sPublicRProperties[] =
{
  // sbILibraryResource
  "library:guid",
  "library:created",
  "library:updated",

  // sbIMediaItem
  // XXXsteve I've omitted library since we don't want the user to get back
  // to the original library
  "library:isMutable",
  "library:mediaCreated",
  "library:mediaUpdated",
  "library:contentLength",
  "library:contentType",

  // sbIMediaList
  "library:name",
  "library:type",
  "library:length",
  "library:isEmpty",

  // sbIRemoteMediaList
  "library:selection",

  // nsIClassInfo
  "classinfo:classDescription",
  "classinfo:contractID",
  "classinfo:classID",
  "classinfo:implementationLanguage",
  "classinfo:flags"
};

const static char* sPublicMethods[] =
{ 
  // sbILibraryResource
  // XXXsteve I am omitting the write/writeThrough/writePending stuff since
  // it is easy for the user to mess it up.  We will automatically write the
  // properties the users set from the remote api (hopefully these methods will
  // go away soon)
  "library:getProperty",
  "library:setProperty",
  "library:equals",

  // sbIMediaItem
  // XXXsteve None of the media item methods are suitable for the remote api

  // sbIMediaList
  // XXXsteve: I've omitted createView, listeners, and batching
  "library:getItemByGuid",
  "library:getItemByIndex",
  "library:enumerateAllItems",
  "library:enumerateItemsByProperty",
  "library:indexOf",
  "library:lastIndexOf",
  "library:contains",
  "library:add",
  "library:addAll",
  "library:remove",
  "library:removeByIndex",
  "library:clear",
  "library:getDistinctValuesForProperty",

  // sbIRemoteMediaList
  "library:ensureColumVisible",
  "library:getView",
  "library:setSelectionByIndex"
};

NS_IMPL_ISUPPORTS8(sbRemoteMediaList,
                   nsIClassInfo,
                   nsISecurityCheckedComponent,
                   sbISecurityAggregator,
                   sbIRemoteMediaList,
                   sbIMediaList,
                   sbIWrappedMediaList,
                   sbIMediaItem,
                   sbILibraryResource)

NS_IMPL_CI_INTERFACE_GETTER6( sbRemoteMediaList,
                              sbISecurityAggregator,
                              sbIRemoteMediaList,
                              sbIMediaList,
                              sbIMediaItem,
                              sbILibraryResource,
                              nsISecurityCheckedComponent )

SB_IMPL_CLASSINFO_INTERFACES_ONLY(sbRemoteMediaList)

sbRemoteMediaList::sbRemoteMediaList(sbIMediaList* aMediaList,
                                     sbIMediaListView* aMediaListView) :
  mMediaList(aMediaList),
  mMediaListView(aMediaListView)
{
  NS_ASSERTION(aMediaList, "Null media list!");
  NS_ASSERTION(aMediaListView, "Null media list view!");

  mMediaItem = do_QueryInterface(mMediaList);
  NS_ASSERTION(mMediaItem, "Could not QI media list to media item");

#ifdef PR_LOGGING
  if (!gRemoteMediaListLog) {
    gRemoteMediaListLog = PR_NewLogModule("sbRemoteMediaList");
  }
  LOG(("sbRemoteMediaList::sbRemoteMediaList()"));
#endif
}

nsresult
sbRemoteMediaList::Init()
{
  LOG(("sbRemoteMediaList::Init()"));

  nsresult rv;

  nsCOMPtr<sbISecurityMixin> mixin =
    do_CreateInstance("@songbirdnest.com/remoteapi/security-mixin;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the list of IIDs to pass to the security mixin
  nsIID ** iids;
  PRUint32 iidCount;
  rv = GetInterfaces(&iidCount, &iids);
  NS_ENSURE_SUCCESS(rv, rv);

  // initialize our mixin with approved interfaces, methods, properties
  rv = mixin->Init( (sbISecurityAggregator*)this,
                    (const nsIID**)iids, iidCount,
                    sPublicMethods, NS_ARRAY_LENGTH(sPublicMethods),
                    sPublicRProperties,NS_ARRAY_LENGTH(sPublicRProperties),
                    sPublicWProperties, NS_ARRAY_LENGTH(sPublicWProperties) );
  NS_ENSURE_SUCCESS(rv, rv);

  mSecurityMixin = do_QueryInterface(mixin, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP_(already_AddRefed<sbIMediaItem>)
sbRemoteMediaList::GetMediaItem()
{
  sbIMediaItem* item = mMediaList;
  NS_ADDREF(item);
  return item;
}

NS_IMETHODIMP_(already_AddRefed<sbIMediaList>)
sbRemoteMediaList::GetMediaList()
{
  sbIMediaList* list = mMediaList;
  NS_ADDREF(list);
  return list;
}

// ---------------------------------------------------------------------------
//
//                        sbIMediaList
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbRemoteMediaList::GetItemByGuid(const nsAString& aGuid,
                                 sbIMediaItem** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIMediaItem> item;
  nsresult rv = mMediaList->GetItemByGuid(aGuid, getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  return SB_WrapMediaItem(item, _retval);
}

NS_IMETHODIMP
sbRemoteMediaList::GetItemByIndex(PRUint32 aIndex, sbIMediaItem **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIMediaItem> item;
  nsresult rv = mMediaList->GetItemByIndex(aIndex, getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  return SB_WrapMediaItem(item, _retval);
}

NS_IMETHODIMP
sbRemoteMediaList::EnumerateAllItems(sbIMediaListEnumerationListener *aEnumerationListener,
                                     PRUint16 aEnumerationType)
{
  NS_ENSURE_ARG_POINTER(aEnumerationListener);

  nsRefPtr<sbWrappingMediaListEnumerationListener> wrapper(
    new sbWrappingMediaListEnumerationListener(aEnumerationListener));
  NS_ENSURE_TRUE(wrapper, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = mMediaList->EnumerateAllItems(wrapper, aEnumerationType);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbRemoteMediaList::EnumerateItemsByProperty(const nsAString& aPropertyName,
                                            const nsAString& aPropertyValue,
                                            sbIMediaListEnumerationListener* aEnumerationListener,
                                            PRUint16 aEnumerationType)
{
  NS_ENSURE_ARG_POINTER(aEnumerationListener);

  nsRefPtr<sbWrappingMediaListEnumerationListener> wrapper(
    new sbWrappingMediaListEnumerationListener(aEnumerationListener));
  NS_ENSURE_TRUE(wrapper, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = mMediaList->EnumerateItemsByProperty(aPropertyName,
                                                     aPropertyValue,
                                                     wrapper,
                                                     aEnumerationType);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbRemoteMediaList::IndexOf(sbIMediaItem* aMediaItem,
                           PRUint32 aStartFrom,
                           PRUint32* _retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIMediaItem> mediaItem;
  nsresult rv = SB_WrapMediaItem(aMediaItem, getter_AddRefs(mediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  return mMediaList->IndexOf(mediaItem, aStartFrom, _retval);
}

NS_IMETHODIMP
sbRemoteMediaList::LastIndexOf(sbIMediaItem* aMediaItem,
                               PRUint32 aStartFrom,
                               PRUint32* _retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIMediaItem> mediaItem;
  nsresult rv = SB_WrapMediaItem(aMediaItem, getter_AddRefs(mediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  return mMediaList->LastIndexOf(mediaItem, aStartFrom, _retval);
}

NS_IMETHODIMP
sbRemoteMediaList::Contains(sbIMediaItem* aMediaItem, PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIMediaItem> mediaItem;
  nsresult rv = SB_WrapMediaItem(aMediaItem, getter_AddRefs(mediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  return mMediaList->Contains(mediaItem, _retval);
}

NS_IMETHODIMP
sbRemoteMediaList::Add(sbIMediaItem *aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);

  nsresult rv;
  nsCOMPtr<sbIWrappedMediaItem> wrappedMediaItem =
    do_QueryInterface(aMediaItem, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return mMediaList->Add(wrappedMediaItem->GetMediaItem().get());
}

NS_IMETHODIMP
sbRemoteMediaList::AddAll(sbIMediaList *aMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  nsresult rv;
  nsCOMPtr<sbIWrappedMediaList> wrappedMediaList =
    do_QueryInterface(aMediaList, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return mMediaList->AddAll(wrappedMediaList->GetMediaList().get());
}

NS_IMETHODIMP
sbRemoteMediaList::AddSome(nsISimpleEnumerator* aMediaItems)
{
  NS_ENSURE_ARG_POINTER(aMediaItems);

  nsRefPtr<sbUnwrappingSimpleEnumerator> wrapper(
    new sbUnwrappingSimpleEnumerator(aMediaItems));
  NS_ENSURE_TRUE(wrapper, NS_ERROR_OUT_OF_MEMORY);

  return mMediaList->AddSome(wrapper);
}

NS_IMETHODIMP
sbRemoteMediaList::Remove(sbIMediaItem* aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);

  nsresult rv;
  nsCOMPtr<sbIWrappedMediaItem> wrappedMediaItem =
    do_QueryInterface(aMediaItem, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return mMediaList->Remove(wrappedMediaItem->GetMediaItem().get());
}

// ---------------------------------------------------------------------------
//
//                        sbIRemoteMediaList
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbRemoteMediaList::GetSelection( nsISimpleEnumerator **aSelection )
{
  LOG(("sbRemoteMediaList::GetSelection()"));

  nsCOMPtr<nsITreeView> treeView;
  nsresult rv = mMediaListView->GetTreeView(getter_AddRefs(treeView));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaListViewTreeView> mlvtv = do_QueryInterface(treeView, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISimpleEnumerator> selection;
  rv = mlvtv->GetSelectedMediaItems(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoPtr<sbRemoteWrappingSimpleEnumerator> wrapped(
    new sbRemoteWrappingSimpleEnumerator(selection));
  NS_ENSURE_TRUE(wrapped, NS_ERROR_OUT_OF_MEMORY);

  rv = wrapped->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aSelection = wrapped.forget());
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteMediaList::EnsureColumVisible( const nsAString& aPropertyName,
                                       const nsAString& aColumnType )
{
  LOG(("sbRemoteMediaList::EnsureColumVisible()"));
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteMediaList::SetSelectionByIndex( PRUint32 aIndex, PRBool aSelected )
{
  LOG(("sbRemoteMediaList::SetSelectionByIndex()"));

  nsCOMPtr<nsITreeView> treeView;
  nsresult rv = mMediaListView->GetTreeView(getter_AddRefs(treeView));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsITreeSelection> treeSelection;
  rv = treeView->GetSelection(getter_AddRefs(treeSelection));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isSelected;
  rv = treeSelection->IsSelected(aIndex, &isSelected);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isSelected != aSelected) {
    rv = treeSelection->ToggleSelect(aIndex);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbRemoteMediaList::AddItemByURL( const nsAString &aURL )
{
  LOG(( "sbRemoteMediaList::AddItemByURL(%s)",
        NS_LossyConvertUTF16toASCII(aURL).get() ));
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteMediaList::GetView( sbIMediaListView **aView )
{
  LOG(("sbRemoteMediaList::GetView()"));
  NS_ENSURE_ARG_POINTER(aView);
  NS_ASSERTION(mMediaListView, "No View");
  NS_ADDREF( *aView = mMediaListView );
  return NS_OK;
}


// ---------------------------------------------------------------------------
//
//                    sbIMediaListEnumerationListener
//
// ---------------------------------------------------------------------------

NS_IMPL_ISUPPORTS1(sbWrappingMediaListEnumerationListener,
                   sbIMediaListEnumerationListener)

sbWrappingMediaListEnumerationListener::sbWrappingMediaListEnumerationListener(sbIMediaListEnumerationListener* aWrapped) :
  mWrapped(aWrapped)
{
  NS_ASSERTION(aWrapped, "Null wrapped enumerator!");
}

NS_IMETHODIMP
sbWrappingMediaListEnumerationListener::OnEnumerationBegin(sbIMediaList *aMediaList,
                                                           PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIMediaList> mediaList;
  nsresult rv = SB_WrapMediaList(aMediaList, getter_AddRefs(mediaList));
  NS_ENSURE_SUCCESS(rv, rv);

  return mWrapped->OnEnumerationBegin(mediaList, _retval);
}

NS_IMETHODIMP
sbWrappingMediaListEnumerationListener::OnEnumeratedItem(sbIMediaList *aMediaList,
                                                         sbIMediaItem *aMediaItem,
                                                         PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIMediaList> mediaList;
  nsresult rv = SB_WrapMediaList(aMediaList, getter_AddRefs(mediaList));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> mediaItem;
  rv = SB_WrapMediaItem(aMediaItem, getter_AddRefs(mediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  return mWrapped->OnEnumeratedItem(mediaList, mediaItem, _retval);
}

NS_IMETHODIMP
sbWrappingMediaListEnumerationListener::OnEnumerationEnd(sbIMediaList *aMediaList,
                                                         nsresult aStatusCode)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  nsCOMPtr<sbIMediaList> mediaList;
  nsresult rv = SB_WrapMediaList(aMediaList, getter_AddRefs(mediaList));
  NS_ENSURE_SUCCESS(rv, rv);

  return mWrapped->OnEnumerationEnd(mediaList, aStatusCode);;
}

// ---------------------------------------------------------------------------
//
//                          nsISimpleEnumerator
//
// ---------------------------------------------------------------------------

NS_IMPL_ISUPPORTS1(sbUnwrappingSimpleEnumerator, nsISimpleEnumerator)

sbUnwrappingSimpleEnumerator::sbUnwrappingSimpleEnumerator(nsISimpleEnumerator* aWrapped) :
  mWrapped(aWrapped)
{
  NS_ASSERTION(aWrapped, "Null wrapped enumerator!");
}

NS_IMETHODIMP
sbUnwrappingSimpleEnumerator::HasMoreElements(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  return mWrapped->HasMoreElements(_retval);
}

NS_IMETHODIMP
sbUnwrappingSimpleEnumerator::GetNext(nsISupports **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  nsCOMPtr<nsISupports> supports;
  rv = mWrapped->GetNext(getter_AddRefs(supports));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIWrappedMediaList> mediaList = do_QueryInterface(supports, &rv);
  if (NS_SUCCEEDED(rv)) {
    NS_ADDREF(*_retval = mediaList->GetMediaList().get());
    return NS_OK;
  }

  nsCOMPtr<sbIWrappedMediaItem> mediaItem = do_QueryInterface(supports, &rv);
  if (NS_SUCCEEDED(rv)) {
    NS_ADDREF(*_retval = mediaItem->GetMediaItem().get());
    return NS_OK;
  }

  return NS_ERROR_UNEXPECTED;
}

