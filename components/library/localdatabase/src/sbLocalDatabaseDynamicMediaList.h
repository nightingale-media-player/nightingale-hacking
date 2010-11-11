/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#ifndef __SB_LOCAL_DATABASE_DYNAMIC_MEDIALIST_H__
#define __SB_LOCAL_DATABASE_DYNAMIC_MEDIALIST_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird dynamic media list defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbLocalDatabaseDynamicMediaList.h
 * \brief Songbird Local Database Dynamic Media List Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird dynamic media list imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbILocalDatabaseMediaItem.h>
#include <sbIMediaList.h>
#include <sbIDynamicMediaList.h>
#include <sbWeakReference.h>

// Mozilla imports.
#include <nsIClassInfo.h>
#include <nsStringGlue.h>

//------------------------------------------------------------------------------
//
// Songbird dynamic media list macros.
//
//------------------------------------------------------------------------------

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_SBIMEDIALIST_ALL_BUT_TYPE(_to) \
  NS_IMETHOD GetName(nsAString & aName) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetName(aName); } \
  NS_IMETHOD SetName(const nsAString & aName) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetName(aName); } \
  NS_IMETHOD GetLength(PRUint32 *aLength) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetLength(aLength); } \
  NS_IMETHOD GetItemByGuid(const nsAString & aGuid, sbIMediaItem **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetItemByGuid(aGuid, _retval); } \
  NS_IMETHOD GetItemByIndex(PRUint32 aIndex, sbIMediaItem **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetItemByIndex(aIndex, _retval); } \
  NS_IMETHOD GetListContentType(PRUint16* _retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetListContentType(_retval); } \
  NS_IMETHOD EnumerateAllItems(sbIMediaListEnumerationListener *aEnumerationListener, PRUint16 aEnumerationType) { return !_to ? NS_ERROR_NULL_POINTER : _to->EnumerateAllItems(aEnumerationListener, aEnumerationType); } \
  NS_IMETHOD EnumerateItemsByProperty(const nsAString & aPropertyID, const nsAString & aPropertyValue, sbIMediaListEnumerationListener *aEnumerationListener, PRUint16 aEnumerationType) { return !_to ? NS_ERROR_NULL_POINTER : _to->EnumerateItemsByProperty(aPropertyID, aPropertyValue, aEnumerationListener, aEnumerationType); } \
  NS_IMETHOD EnumerateItemsByProperties(sbIPropertyArray *aProperties, sbIMediaListEnumerationListener *aEnumerationListener, PRUint16 aEnumerationType) { return !_to ? NS_ERROR_NULL_POINTER : _to->EnumerateItemsByProperties(aProperties, aEnumerationListener, aEnumerationType); } \
  NS_IMETHOD GetItemsByProperty(const nsAString & aPropertyID, const nsAString & aPropertyValue, nsIArray **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetItemsByProperty(aPropertyID, aPropertyValue, _retval); } \
  NS_IMETHOD GetItemCountByProperty(const nsAString & aPropertyID, const nsAString & aPropertyValue, PRUint32 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetItemCountByProperty(aPropertyID, aPropertyValue, _retval); } \
  NS_IMETHOD GetItemsByProperties(sbIPropertyArray *aProperties, nsIArray **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetItemsByProperties(aProperties, _retval); } \
  NS_IMETHOD IndexOf(sbIMediaItem *aMediaItem, PRUint32 aStartFrom, PRUint32 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->IndexOf(aMediaItem, aStartFrom, _retval); } \
  NS_IMETHOD LastIndexOf(sbIMediaItem *aMediaItem, PRUint32 aStartFrom, PRUint32 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->LastIndexOf(aMediaItem, aStartFrom, _retval); } \
  NS_IMETHOD Contains(sbIMediaItem *aMediaItem, PRBool *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Contains(aMediaItem, _retval); } \
  NS_IMETHOD GetIsEmpty(PRBool *aIsEmpty) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetIsEmpty(aIsEmpty); } \
  NS_IMETHOD GetUserEditableContent(PRBool *aUserEditableContent) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetUserEditableContent(aUserEditableContent); } \
  NS_IMETHOD Add(sbIMediaItem *aMediaItem) { return !_to ? NS_ERROR_NULL_POINTER : _to->Add(aMediaItem); } \
  NS_IMETHOD AddItem(sbIMediaItem *aMediaItem, sbIMediaItem ** aNewMediaItem) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddItem(aMediaItem, aNewMediaItem); } \
  NS_IMETHOD AddAll(sbIMediaList *aMediaList) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddAll(aMediaList); } \
  NS_IMETHOD AddSome(nsISimpleEnumerator *aMediaItems) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddSome(aMediaItems); } \
  NS_IMETHOD AddSomeAsync(nsISimpleEnumerator *aMediaItems, sbIMediaListAsyncListener *aListener) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddSomeAsync(aMediaItems, aListener); } \
  NS_IMETHOD Remove(sbIMediaItem *aMediaItem) { return !_to ? NS_ERROR_NULL_POINTER : _to->Remove(aMediaItem); } \
  NS_IMETHOD RemoveByIndex(PRUint32 aIndex) { return !_to ? NS_ERROR_NULL_POINTER : _to->RemoveByIndex(aIndex); } \
  NS_IMETHOD RemoveSome(nsISimpleEnumerator *aMediaItems) { return !_to ? NS_ERROR_NULL_POINTER : _to->RemoveSome(aMediaItems); } \
  NS_IMETHOD Clear(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->Clear(); } \
  NS_IMETHOD AddListener(sbIMediaListListener *aListener, PRBool aOwnsWeak, PRUint32 aFlags, sbIPropertyArray *aPropertyFilter) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddListener(aListener, aOwnsWeak, aFlags, aPropertyFilter); } \
  NS_IMETHOD RemoveListener(sbIMediaListListener *aListener) { return !_to ? NS_ERROR_NULL_POINTER : _to->RemoveListener(aListener); } \
  NS_IMETHOD CreateView(sbIMediaListViewState* aState, sbIMediaListView **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->CreateView(aState, _retval); } \
  NS_IMETHOD GetDistinctValuesForProperty(const nsAString& aPropertyID, nsIStringEnumerator** _retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetDistinctValuesForProperty(aPropertyID, _retval); } \
  NS_IMETHOD RunInBatchMode(sbIMediaListBatchCallback *aCallback, nsISupports *aUserData) { return !_to ? NS_ERROR_NULL_POINTER : _to->RunInBatchMode(aCallback, aUserData); }

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define SB_FORWARD_SAFE_SBILOCALDATABASEMEDIAITEM(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetMediaItemId(PRUint32 *aMediaItemId) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetMediaItemId(aMediaItemId); } \
  NS_SCRIPTABLE NS_IMETHOD GetPropertyBag(sbILocalDatabaseResourcePropertyBag * *aPropertyBag) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetPropertyBag(aPropertyBag); } \
  NS_SCRIPTABLE NS_IMETHOD SetPropertyBag(sbILocalDatabaseResourcePropertyBag * aPropertyBag) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetPropertyBag(aPropertyBag); } \
  NS_IMETHOD_(void) SetSuppressNotifications(PRBool aSuppress) { if (_to) _to->SetSuppressNotifications(aSuppress); }


//------------------------------------------------------------------------------
//
// Songbird dynamic media list classes.
//
//------------------------------------------------------------------------------

/**
 * This class implements a dynamic media list.
 */

class sbLocalDatabaseDynamicMediaList : public nsIClassInfo,
                                        public sbSupportsWeakReference,
                                        public sbIMediaList,
                                        public sbIDynamicMediaList,
                                        public sbILocalDatabaseMediaItem
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // Interface implementations.
  //

  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBIDYNAMICMEDIALIST
  NS_FORWARD_SAFE_SBILIBRARYRESOURCE(mBaseMediaList)
  NS_FORWARD_SAFE_SBIMEDIAITEM(mBaseMediaList)
  NS_FORWARD_SAFE_SBIMEDIALIST_ALL_BUT_TYPE(mBaseMediaList)
  SB_FORWARD_SAFE_SBILOCALDATABASEMEDIAITEM(mBaseLocalDatabaseMediaList)


  //
  // sbIMediaList interface implementation.
  //

  NS_IMETHOD GetType(nsAString& aType);


  //
  // Public services.
  //

  /**
   * Create and initialize a dynamic media list for the inner media item
   * specified by aInner.  Return the created dynamic media list in aMediaList.
   *
   * \param aInner                Media list inner media item.
   * \param aMediaList            Returned created dynamic media list.
   */
  static nsresult New(sbIMediaItem*  aInner,
                      sbIMediaList** aMediaList);

  virtual ~sbLocalDatabaseDynamicMediaList();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

  //
  // Internal fields.
  //
  //   mBaseMediaList           Base media list.
  //   mBaseLocalDatabaseMediaList
  //                            Base media list as an sbILocalDatabaseMediaItem.
  //

  nsCOMPtr<sbIMediaList>                mBaseMediaList;
  nsCOMPtr<sbILocalDatabaseMediaItem>   mBaseLocalDatabaseMediaList;


  //
  // Private services.
  //

  sbLocalDatabaseDynamicMediaList();

  nsresult Initialize(sbIMediaItem* aInner);
};

#endif // __SB_LOCAL_DATABASE_DYNAMIC_MEDIALIST_H__

