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

#ifndef __SB_SCRIPTABLE_FILTER_ITEMS_H__
#define __SB_SCRIPTABLE_FILTER_ITEMS_H__

#include "sbXPCScriptableStub.h"

#include <nsAutoPtr.h>
#include <nsCOMArray.h>
#include <nsCOMPtr.h>
#include <nsIClassInfo.h>
#include <nsISecurityCheckedComponent.h>
#include <nsISimpleEnumerator.h>
#include <nsVoidArray.h>

#include <set>

class sbIFilterableMediaListView;
class sbIMediaItem;
class sbRemotePlayer;

/**
 * The final results of filtering via sbScriptableListFilter
 * Acts like a collection of media items
 */
class sbScriptableFilterItems : public nsISecurityCheckedComponent,
                                public nsISimpleEnumerator,
                                public sbXPCScriptableStub
{
// this is used for QI, to ensure that we indeed have an instance of this class
// {E26A6BC3-7FFE-44f5-8348-D499F2A6CD12}
#define SB_SCRIPTABLE_FILETER_ITEMS_CID \
  { 0xe26a6bc3, 0x7ffe, 0x44f5, \
    { 0x83, 0x48, 0xd4, 0x99, 0xf2, 0xa6, 0xcd, 0x12 } }
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(SB_SCRIPTABLE_FILETER_ITEMS_CID)
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_NSISECURITYCHECKEDCOMPONENT
  NS_DECL_NSISIMPLEENUMERATOR

  /* nsIXPCScriptable */
  NS_IMETHOD GetClassName(char * *aClassName);
  NS_IMETHOD GetScriptableFlags(PRUint32 *aScriptableFlags);
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext * cx, JSObject * obj, jsval id, jsval * vp, PRBool *_retval);
  NS_IMETHOD NewEnumerate(nsIXPConnectWrappedNative *wrapper, JSContext * cx, JSObject * obj, PRUint32 enum_op, jsval * statep, jsid *idp, PRBool *_retval);
  NS_IMETHOD NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext * cx, JSObject * obj, jsval id, PRUint32 flags, JSObject * *objp, PRBool *_retval);
  NS_IMETHOD Equality( nsIXPConnectWrappedNative *wrapper,
                       JSContext * cx,
                       JSObject * obj,
                       jsval val,
                       PRBool *_retval );

  sbScriptableFilterItems( sbIFilterableMediaListView *aFilterList,
                           sbRemotePlayer *aPlayer );
  sbScriptableFilterItems( const nsCOMArray<sbIMediaItem>& aItems,
                           sbRemotePlayer *aPlayer );

private:
  ~sbScriptableFilterItems();

protected:
  // iterator over the enumerator and fill out the list
  nsresult ReadEnumerator();

protected:
  nsCOMPtr<sbIFilterableMediaListView> mListView;
  nsCOMArray<sbIMediaItem> mItems;
  PRBool mHasItems;
  nsRefPtr<sbRemotePlayer> mPlayer;
  PRUint32 mEnumerationIndex;
};

#endif // __SB_SCRIPTABLE_FILTER_ITEMS_H__
