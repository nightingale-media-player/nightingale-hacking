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

#ifndef __SBLOCALDATABASESMARTMEDIALIST_H__
#define __SBLOCALDATABASESMARTMEDIALIST_H__

#include <sbILocalDatabaseSmartMediaList.h>

#include <nsWeakReference.h>
#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include <prlock.h>

#include <sbIMediaItem.h>
#include <sbIMediaList.h>


/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_SBIMEDIALIST_ALL_BUT_TYPE(_to) \
  NS_IMETHOD GetName(nsAString & aName) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetName(aName); } \
  NS_IMETHOD SetName(const nsAString & aName) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetName(aName); } \
  NS_IMETHOD GetLength(PRUint32 *aLength) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetLength(aLength); } \
  NS_IMETHOD GetItemByGuid(const nsAString & aGuid, sbIMediaItem **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetItemByGuid(aGuid, _retval); } \
  NS_IMETHOD GetItemByIndex(PRUint32 aIndex, sbIMediaItem **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetItemByIndex(aIndex, _retval); } \
  NS_IMETHOD EnumerateAllItems(sbIMediaListEnumerationListener *aEnumerationListener, PRUint16 aEnumerationType) { return !_to ? NS_ERROR_NULL_POINTER : _to->EnumerateAllItems(aEnumerationListener, aEnumerationType); } \
  NS_IMETHOD EnumerateItemsByProperty(const nsAString & aPropertyName, const nsAString & aPropertyValue, sbIMediaListEnumerationListener *aEnumerationListener, PRUint16 aEnumerationType) { return !_to ? NS_ERROR_NULL_POINTER : _to->EnumerateItemsByProperty(aPropertyName, aPropertyValue, aEnumerationListener, aEnumerationType); } \
  NS_IMETHOD EnumerateItemsByProperties(sbIPropertyArray *aProperties, sbIMediaListEnumerationListener *aEnumerationListener, PRUint16 aEnumerationType) { return !_to ? NS_ERROR_NULL_POINTER : _to->EnumerateItemsByProperties(aProperties, aEnumerationListener, aEnumerationType); } \
  NS_IMETHOD IndexOf(sbIMediaItem *aMediaItem, PRUint32 aStartFrom, PRUint32 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->IndexOf(aMediaItem, aStartFrom, _retval); } \
  NS_IMETHOD LastIndexOf(sbIMediaItem *aMediaItem, PRUint32 aStartFrom, PRUint32 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->LastIndexOf(aMediaItem, aStartFrom, _retval); } \
  NS_IMETHOD Contains(sbIMediaItem *aMediaItem, PRBool *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Contains(aMediaItem, _retval); } \
  NS_IMETHOD GetIsEmpty(PRBool *aIsEmpty) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetIsEmpty(aIsEmpty); } \
  NS_IMETHOD Add(sbIMediaItem *aMediaItem) { return !_to ? NS_ERROR_NULL_POINTER : _to->Add(aMediaItem); } \
  NS_IMETHOD AddAll(sbIMediaList *aMediaList) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddAll(aMediaList); } \
  NS_IMETHOD AddSome(nsISimpleEnumerator *aMediaItems) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddSome(aMediaItems); } \
  NS_IMETHOD Remove(sbIMediaItem *aMediaItem) { return !_to ? NS_ERROR_NULL_POINTER : _to->Remove(aMediaItem); } \
  NS_IMETHOD RemoveByIndex(PRUint32 aIndex) { return !_to ? NS_ERROR_NULL_POINTER : _to->RemoveByIndex(aIndex); } \
  NS_IMETHOD RemoveSome(nsISimpleEnumerator *aMediaItems) { return !_to ? NS_ERROR_NULL_POINTER : _to->RemoveSome(aMediaItems); } \
  NS_IMETHOD Clear(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->Clear(); } \
  NS_IMETHOD AddListener(sbIMediaListListener *aListener) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddListener(aListener); } \
  NS_IMETHOD RemoveListener(sbIMediaListListener *aListener) { return !_to ? NS_ERROR_NULL_POINTER : _to->RemoveListener(aListener); } \
  NS_IMETHOD CreateView(sbIMediaListView **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->CreateView(_retval); } \
  NS_IMETHOD BeginUpdateBatch(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->BeginUpdateBatch(); } \
  NS_IMETHOD EndUpdateBatch(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->EndUpdateBatch(); } 

class sbLocalDatabaseSmartMediaListCondition : public sbILocalDatabaseSmartMediaListCondition
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILOCALDATABASESMARTMEDIALISTCONDITION

  sbLocalDatabaseSmartMediaListCondition();
  ~sbLocalDatabaseSmartMediaListCondition();

protected:

};

class sbLocalDatabaseSmartMediaList : public sbILocalDatabaseSmartMediaList,
                                      public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILOCALDATABASESMARTMEDIALIST

  
  NS_FORWARD_SAFE_SBIMEDIAITEM(mItem)
  NS_FORWARD_SAFE_SBILIBRARYRESOURCE(mItem)

  /* Forward all media list functions to mList except for the
     type getter */
  NS_FORWARD_SAFE_SBIMEDIALIST_ALL_BUT_TYPE(mList)
  NS_IMETHOD GetType(nsAString& aType);

  sbLocalDatabaseSmartMediaList();
  ~sbLocalDatabaseSmartMediaList();

protected:
  PRLock* mInnerLock;

  nsCOMPtr<sbIMediaItem> mItem;
  nsCOMPtr<sbIMediaList> mList;

};

#endif /* __SBLOCALDATABASESMARTMEDIALIST_H__ */
