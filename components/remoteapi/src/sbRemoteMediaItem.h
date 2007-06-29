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

#ifndef __SB_REMOTE_MEDIAITEM_H__
#define __SB_REMOTE_MEDIAITEM_H__

#include <sbIMediaItem.h>
#include <sbILibraryResource.h>
#include <sbISecurityMixin.h>
#include <sbISecurityAggregator.h>
#include <sbIWrappedMediaItem.h>

#include <nsISecurityCheckedComponent.h>
#include <nsStringGlue.h>
#include <nsCOMPtr.h>

#define NS_FORWARD_SAFE_SBILIBRARYRESOURCE_NO_SETPROPERTY(_to) \
  NS_IMETHOD GetGuid(nsAString & aGuid) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetGuid(aGuid); } \
  NS_IMETHOD GetCreated(PRInt64 *aCreated) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetCreated(aCreated); } \
  NS_IMETHOD GetUpdated(PRInt64 *aUpdated) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetUpdated(aUpdated); } \
  NS_IMETHOD GetWriteThrough(PRBool *aWriteThrough) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetWriteThrough(aWriteThrough); } \
  NS_IMETHOD SetWriteThrough(PRBool aWriteThrough) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetWriteThrough(aWriteThrough); } \
  NS_IMETHOD GetWritePending(PRBool *aWritePending) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetWritePending(aWritePending); } \
  NS_IMETHOD GetPropertyNames(nsIStringEnumerator ** _retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetPropertyNames(_retval); } \
  NS_IMETHOD GetProperty(const nsAString & aName, nsAString & _retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetProperty(aName, _retval); } \
  NS_IMETHOD GetProperties(sbIPropertyArray * aPropertyNames, sbIPropertyArray ** _retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetProperties(aPropertyNames, _retval); } \
  NS_IMETHOD SetProperties(sbIPropertyArray * aProperties) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetProperties(aProperties); } \
  NS_IMETHOD Write(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->Write(); } \
  NS_IMETHOD Equals(sbILibraryResource *aOtherLibraryResource, PRBool *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Equals(aOtherLibraryResource, _retval); }

class sbRemoteMediaItem : public nsIClassInfo,
                          public nsISecurityCheckedComponent,
                          public sbISecurityAggregator,
                          public sbIMediaItem,
                          public sbIWrappedMediaItem
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBISECURITYAGGREGATOR

  NS_FORWARD_SAFE_SBIMEDIAITEM(mMediaItem);
  NS_FORWARD_SAFE_SBILIBRARYRESOURCE_NO_SETPROPERTY(mMediaItem);
  NS_FORWARD_SAFE_NSISECURITYCHECKEDCOMPONENT(mSecurityMixin);

  NS_IMETHOD SetProperty(const nsAString& aName, const nsAString& aValue);

  // sbIWrappedMediaItem interface
  NS_IMETHOD_(already_AddRefed<sbIMediaItem>) GetMediaItem();

  sbRemoteMediaItem(sbIMediaItem* aMediaItem);

  nsresult Init();

protected:

  nsCOMPtr<nsISecurityCheckedComponent> mSecurityMixin;

  nsCOMPtr<sbIMediaItem> mMediaItem;
};

#endif // __SB_REMOTE_MEDIAITEM_H__
