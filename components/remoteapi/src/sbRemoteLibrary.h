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

#ifndef __SB_REMOTE_LIBRARY_H__
#define __SB_REMOTE_LIBRARY_H__

#include "sbRemoteMediaItem.h"
#include "sbRemoteMediaList.h"

#include <sbIRemotePlayer.h>
#include <sbILibrary.h>
#include <sbISecurityMixin.h>
#include <sbISecurityAggregator.h>
#include <sbIMediaListView.h>

#include <nsIFile.h>
#include <nsISecurityCheckedComponent.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <nsCOMPtr.h>
#include <nsAutoPtr.h>

#define SONGBIRD_REMOTELIBRARY_CONTRACTID               \
  "@songbirdnest.com/remoteapi/remotelibrary;1"
#define SONGBIRD_REMOTELIBRARY_CLASSNAME                \
  "Songbird Remote Library"
#define SONGBIRD_REMOTELIBRARY_CID                      \
{ /* e2d511c5-be47-4140-b48e-c011e471e1a2 */            \
  0xe2d511c5,                                            \
  0xbe47,                                                \
  0x4140,                                                \
  {0xb4, 0x8e, 0xc0, 0x11, 0xe4, 0x71, 0xe1, 0xa2}       \
}

class sbRemoteLibrary : public nsIClassInfo,
                        public nsISecurityCheckedComponent,
                        public sbISecurityAggregator,
                        public sbIRemoteLibrary
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_NSISECURITYCHECKEDCOMPONENT
  NS_DECL_SBISECURITYAGGREGATOR
  NS_DECL_SBIREMOTELIBRARY
  NS_DECL_SBIREMOTEMEDIALIST

  sbRemoteLibrary();
  nsresult Init();

protected:
  virtual ~sbRemoteLibrary();
  nsCOMPtr<sbILibrary> mLibrary;

  // Gets the GUID for the built in libraries: main, web, download
  nsresult GetLibraryGUID( const nsAString &aLibraryID,
                           nsAString &aLibraryGUID );
  // fetches the URI from the security mixin
  nsresult GetURI( nsIURI **aSiteURI );
  // validates aPath against aSiteURI, setting aPath if empty
  nsresult CheckPath( nsAString &aPath, nsIURI *aSiteURI );
  // validates aDomain against aSiteURI, setting aDomain if empty
  nsresult CheckDomain( nsAString &aDomain, nsIURI *aSiteURI );
  // builds a path to the db file for the passed in domain and path
  nsresult GetSiteLibraryFile( const nsAString &aDomain,
                               const nsAString &aPath,
                               nsIFile **aSiteDBFile );

  // SecurityCheckedComponent stuff
  nsCOMPtr<nsISecurityCheckedComponent> mSecurityMixin;
  nsTArray<nsString> mPublicMethods;
  nsTArray<nsString> mPublicRProperties;  // Readable Properties
  nsTArray<nsString> mPublicWProperties;  // Writeable Properties

#ifdef DEBUG
  // Only set in debug builds - used for validating library creation
  nsString mFilename;
#endif
};

static nsresult
SB_WrapMediaItem(sbIMediaItem* aMediaItem,
                 sbIMediaItem** aRemoteMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aRemoteMediaItem);

  nsresult rv;

  nsCOMPtr<sbIMediaList> mediaList = do_QueryInterface(aMediaItem, &rv);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<sbIMediaListView> mediaListView;
    rv = mediaList->CreateView(getter_AddRefs(mediaListView));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<sbRemoteMediaList> remoteMediaList(
      new sbRemoteMediaList(mediaList, mediaListView));
    NS_ENSURE_TRUE(remoteMediaList, NS_ERROR_OUT_OF_MEMORY);

    rv = remoteMediaList->Init();
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ADDREF(*aRemoteMediaItem = remoteMediaList.forget());
  }
  else {
    nsAutoPtr<sbRemoteMediaItem> remoteMediaItem(
      new sbRemoteMediaItem(aMediaItem));
    NS_ENSURE_TRUE(remoteMediaItem, NS_ERROR_OUT_OF_MEMORY);

    rv = remoteMediaItem->Init();
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ADDREF(*aRemoteMediaItem = remoteMediaItem.forget());
  }

  return NS_OK;
}

static nsresult
SB_WrapMediaList(sbIMediaList* aMediaList,
                 sbIMediaList** aRemoteMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aRemoteMediaList);

  nsresult rv;

  nsCOMPtr<sbIMediaListView> mediaListView;
  rv = aMediaList->CreateView(getter_AddRefs(mediaListView));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoPtr<sbRemoteMediaList> remoteMediaList(
    new sbRemoteMediaList(aMediaList, mediaListView));
  NS_ENSURE_TRUE(remoteMediaList, NS_ERROR_OUT_OF_MEMORY);

  rv = remoteMediaList->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aRemoteMediaList = remoteMediaList.forget());

  return NS_OK;
}

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

#endif // __SB_REMOTE_LIBRARY_H__
