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

#ifndef __SB_REMOTE_MEDIALISTBASE_H__
#define __SB_REMOTE_MEDIALISTBASE_H__

#include "sbRemoteAPI.h"
#include "sbRemoteForwardingMacros.h"
#include <sbILibraryResource.h>
#include "sbXPCScriptableStub.h"

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
                              public sbIWrappedMediaList,
                              public sbXPCScriptableStub
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBISECURITYAGGREGATOR
  NS_DECL_SBIREMOTEMEDIALIST

  NS_FORWARD_SAFE_SBILIBRARYRESOURCE(mRemLibraryResource)
  NS_FORWARD_SAFE_SBIMEDIAITEM(mMediaItem)
  NS_FORWARD_SAFE_SBIMEDIALIST_SIMPLE_ARGUMENTS(mMediaList)
  NS_FORWARD_SAFE_NSISECURITYCHECKEDCOMPONENT(mSecurityMixin)

  using sbXPCScriptableStub::GetProperty;
  using sbXPCScriptableStub::SetProperty;

  // nsIXPCScriptable
  NS_IMETHOD GetClassName(char **aClassName);
  NS_IMETHOD GetScriptableFlags(PRUint32 *aScriptableFlags);
  NS_IMETHOD NewResolve( nsIXPConnectWrappedNative *wrapper,
                         JSContext *cx,
                         JSObject *obj,
                         jsval id,
                         PRUint32 flags,
                         JSObject **objp,
                         bool *_retval );

  // sbIMediaList
  NS_IMETHOD GetItemByGuid(const nsAString& aGuid, sbIMediaItem** _retval);
  NS_IMETHOD GetItemByIndex(PRUint32 aIndex, sbIMediaItem** _retval);
  NS_IMETHOD GetItemCountByProperty(const nsAString & aPropertyID,
                                    const nsAString & aPropertyValue,
                                    PRUint32 *_retval);
  NS_IMETHOD GetListContentType(PRUint16 * _retval);
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
  NS_IMETHOD Contains(sbIMediaItem* aMediaItem, bool* _retval);
  NS_IMETHOD Add(sbIMediaItem* aMediaItem);
  NS_IMETHOD AddItem(sbIMediaItem * aMediaItem, sbIMediaItem ** aNewItem);
  NS_IMETHOD AddAll(sbIMediaList *aMediaList);
  NS_IMETHOD AddSome(nsISimpleEnumerator* aMediaItems);
  NS_IMETHOD AddMediaItems(nsISimpleEnumerator* aMediaItems, sbIAddMediaItemsListener* aListener, bool aAsync);
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

  // Helper method to do the throwing for AddHelper
  static nsresult ThrowJSException( JSContext *cx,
                                    const nsACString &aExceptionMsg );

  static JSBool AddHelper( JSContext *cx,
                           JSObject *obj,
                           uintN argc,
                           jsval *argv,
                           jsval *rval );

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

