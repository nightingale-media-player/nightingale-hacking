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

#ifndef __SB_REMOTE_MEDIALISTBASE_H__
#define __SB_REMOTE_MEDIALISTBASE_H__

#include "sbRemoteAPI.h"
#include "sbRemoteForwardingMacros.h"
#include <sbILibraryResource.h>

#include <sbIMediaItem.h>
#include <sbIMediaList.h>
#include <sbIMediaListListener.h>
#include <sbIRemoteMediaList.h>
#include <sbISecurityAggregator.h>
#include <sbISecurityMixin.h>
#include <sbIWrappedMediaList.h>

#include <nsIClassInfo.h>
#include <nsISimpleEnumerator.h>
#include <nsISecurityCheckedComponent.h>
#include <nsStringGlue.h>
#include <nsCOMPtr.h>

#ifdef PR_LOGGING
extern PRLogModuleInfo *gRemoteMediaListLog;
#endif

#define LOG_LIST(args) PR_LOG(gRemoteMediaListLog, PR_LOG_WARN, args)

class sbILibrary;
class sbIMediaListView;
class sbRemotePlayer;

// derived classes need to implement nsIClassInfo
class sbRemoteMediaListBase : public sbIMediaList,
                              public nsIClassInfo,
                              public nsISecurityCheckedComponent,
                              public sbISecurityAggregator,
                              public sbIRemoteMediaList,
                              public sbIWrappedMediaList
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBISECURITYAGGREGATOR
  NS_DECL_SBIREMOTEMEDIALIST

  NS_FORWARD_SAFE_SBILIBRARYRESOURCE(mRemLibraryResource)
  NS_FORWARD_SAFE_SBIMEDIAITEM(mMediaItem)
  NS_FORWARD_SAFE_SBIMEDIALIST_SIMPLE_ARGUMENTS(mMediaList)
  NS_FORWARD_SAFE_NSISECURITYCHECKEDCOMPONENT(mSecurityMixin)

  // sbIMediaList
  NS_IMETHOD GetItemByGuid(const nsAString& aGuid, sbIMediaItem** _retval);
  NS_IMETHOD GetItemByIndex(PRUint32 aIndex, sbIMediaItem** _retval);
  NS_IMETHOD EnumerateAllItems(sbIMediaListEnumerationListener *aEnumerationListener,
                               PRUint16 aEnumerationType);
  NS_IMETHOD EnumerateItemsByProperty(const nsAString& aPropertyID,
                                      const nsAString& aPropertyValue,
                                      sbIMediaListEnumerationListener* aEnumerationListener,
                                      PRUint16 aEnumerationType);
  NS_IMETHOD IndexOf(sbIMediaItem* aMediaItem,
                     PRUint32 aStartFrom,
                     PRUint32* _retval);
  NS_IMETHOD LastIndexOf(sbIMediaItem* aMediaItem,
                         PRUint32 aStartFrom,
                         PRUint32* _retval);
  NS_IMETHOD Contains(sbIMediaItem* aMediaItem, PRBool* _retval);
  NS_IMETHOD Add(sbIMediaItem* aMediaItem);
  NS_IMETHOD AddAll(sbIMediaList *aMediaList);
  NS_IMETHOD AddSome(nsISimpleEnumerator* aMediaItems);
  NS_IMETHOD Remove(sbIMediaItem* aMediaItem);
  NS_IMETHOD GetDistinctValuesForProperty(const nsAString &aPropertyID,
                                          nsIStringEnumerator **_retval);

  // sbIWrappedMediaList
  virtual already_AddRefed<sbIMediaItem> GetMediaItem();
  virtual already_AddRefed<sbIMediaList> GetMediaList();

  sbRemoteMediaListBase(sbRemotePlayer* aRemotePlayer,
                        sbIMediaList* aMediaList,
                        sbIMediaListView* aMediaListView);

  // implemented in derived classes
  virtual nsresult Init() = 0;

protected:
  virtual ~sbRemoteMediaListBase();

  nsCOMPtr<nsISecurityCheckedComponent> mSecurityMixin;

  nsRefPtr<sbRemotePlayer> mRemotePlayer;
  nsCOMPtr<sbIMediaList> mMediaList;
  nsCOMPtr<sbIMediaListView> mMediaListView;
  nsCOMPtr<sbIMediaItem> mMediaItem;
  nsCOMPtr<sbILibrary> mLibrary;
  nsCOMPtr<sbILibraryResource> mRemLibraryResource;
};

class sbMediaListEnumerationListenerWrapper : public sbIMediaListEnumerationListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER

  sbMediaListEnumerationListenerWrapper(sbRemotePlayer* aRemotePlayer,
                                        sbIMediaListEnumerationListener* aWrapped);

private:
  nsRefPtr<sbRemotePlayer> mRemotePlayer;
  nsCOMPtr<sbIMediaListEnumerationListener> mWrapped;
};

class sbUnwrappingSimpleEnumerator : public nsISimpleEnumerator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISIMPLEENUMERATOR

  sbUnwrappingSimpleEnumerator(nsISimpleEnumerator* aWrapped);

private:
  nsCOMPtr<nsISimpleEnumerator> mWrapped;
};

#endif // __SB_REMOTE_MEDIALISTBASE_H__

