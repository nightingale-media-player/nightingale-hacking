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

#ifndef __SB_REMOTE_MEDIALIST_H__
#define __SB_REMOTE_MEDIALIST_H__

#include <sbILibraryResource.h>
#include <sbIMediaItem.h>
#include <sbIMediaList.h>
#include <sbIMediaListListener.h>
#include <sbIRemoteMediaList.h>
#include <sbISecurityAggregator.h>
#include <sbISecurityMixin.h>
#include <sbIWrappedMediaList.h>

#include <nsISimpleEnumerator.h>
#include <nsISecurityCheckedComponent.h>
#include <nsStringGlue.h>
#include <nsCOMPtr.h>

class sbIMediaListView;
class sbIMediaItem;

#define NS_FORWARD_SAFE_SBIMEDIALIST_SIMPLE_ARGUMENTS(_to) \
  NS_IMETHOD GetName(nsAString & aName) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetName(aName); } \
  NS_IMETHOD SetName(const nsAString & aName) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetName(aName); } \
  NS_IMETHOD GetType(nsAString & aType) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetType(aType); } \
  NS_IMETHOD GetLength(PRUint32 *aLength) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetLength(aLength); } \
  NS_IMETHOD EnumerateItemsByProperties(sbIPropertyArray *aProperties, sbIMediaListEnumerationListener *aEnumerationListener, PRUint16 aEnumerationType) { return !_to ? NS_ERROR_NULL_POINTER : _to->EnumerateItemsByProperties(aProperties, aEnumerationListener, aEnumerationType); } \
  NS_IMETHOD GetIsEmpty(PRBool *aIsEmpty) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetIsEmpty(aIsEmpty); } \
  NS_IMETHOD RemoveByIndex(PRUint32 aIndex) { return !_to ? NS_ERROR_NULL_POINTER : _to->RemoveByIndex(aIndex); } \
  NS_IMETHOD RemoveSome(nsISimpleEnumerator *aMediaItems) { return !_to ? NS_ERROR_NULL_POINTER : _to->RemoveSome(aMediaItems); } \
  NS_IMETHOD Clear(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->Clear(); } \
  NS_IMETHOD AddListener(sbIMediaListListener *aListener, PRBool aOwnsWeak) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddListener(aListener, aOwnsWeak); } \
  NS_IMETHOD RemoveListener(sbIMediaListListener *aListener) { return !_to ? NS_ERROR_NULL_POINTER : _to->RemoveListener(aListener); } \
  NS_IMETHOD CreateView(sbIMediaListView **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->CreateView(_retval); } \
  NS_IMETHOD BeginUpdateBatch(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->BeginUpdateBatch(); } \
  NS_IMETHOD EndUpdateBatch(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->EndUpdateBatch(); } \
  NS_IMETHOD GetDistinctValuesForProperty(const nsAString & aPropertyName, nsIStringEnumerator **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetDistinctValuesForProperty(aPropertyName, _retval); }

class sbRemoteMediaList : public nsIClassInfo,
                          public nsISecurityCheckedComponent,
                          public sbISecurityAggregator,
                          public sbIRemoteMediaList,
                          public sbIMediaList,
                          public sbIWrappedMediaList
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBISECURITYAGGREGATOR
  NS_DECL_SBIREMOTEMEDIALIST

  NS_FORWARD_SAFE_SBILIBRARYRESOURCE(mMediaItem);
  NS_FORWARD_SAFE_SBIMEDIAITEM(mMediaItem);
  NS_FORWARD_SAFE_SBIMEDIALIST_SIMPLE_ARGUMENTS(mMediaList);
  NS_FORWARD_SAFE_NSISECURITYCHECKEDCOMPONENT(mSecurityMixin)

  // sbIMediaList
  NS_IMETHOD GetItemByGuid(const nsAString& aGuid, sbIMediaItem** _retval);
  NS_IMETHOD GetItemByIndex(PRUint32 aIndex, sbIMediaItem** _retval);
  NS_IMETHOD EnumerateAllItems(sbIMediaListEnumerationListener *aEnumerationListener,
                               PRUint16 aEnumerationType);
  NS_IMETHOD EnumerateItemsByProperty(const nsAString& aPropertyName,
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

  // sbIWrappedMediaList
  NS_IMETHOD_(already_AddRefed<sbIMediaItem>) GetMediaItem();
  NS_IMETHOD_(already_AddRefed<sbIMediaList>) GetMediaList();

  sbRemoteMediaList(sbIMediaList* aMediaList,
                    sbIMediaListView* aMediaListView);

  nsresult Init();

protected:

  nsCOMPtr<nsISecurityCheckedComponent> mSecurityMixin;

  nsCOMPtr<sbIMediaList> mMediaList;
  nsCOMPtr<sbIMediaListView> mMediaListView;
  nsCOMPtr<sbIMediaItem> mMediaItem;
};

class sbWrappingMediaListEnumerationListener : public sbIMediaListEnumerationListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER

  sbWrappingMediaListEnumerationListener(sbIMediaListEnumerationListener* aWrapped);

private:
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

#endif // __SB_REMOTE_MEDIALIST_H__
