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

#ifndef __SB_SCRIPTABLE_FILTER_H__
#define __SB_SCRIPTABLE_FILTER_H__

#include "sbXPCScriptableStub.h"

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsIClassInfo.h>
#include <nsISecurityCheckedComponent.h>
#include <nsIStringEnumerator.h>
#include <nsVoidArray.h>

class nsIStringEnumerator;
class sbIFilterableMediaListView;
class sbIProperty;
class sbRemotePlayer;

/**
 * Class reflected into JavaScript as filters on the library
 * Shows up as library.artists etc. (so the user can do library.artists["Bob"])
 */
class sbScriptableFilter : public nsISecurityCheckedComponent,
                           public nsIStringEnumerator,
                           public nsIClassInfo,
                           public sbXPCScriptableStub
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_NSISECURITYCHECKEDCOMPONENT
  NS_DECL_NSISTRINGENUMERATOR

  /* nsIXPCScriptable */
  NS_IMETHOD GetClassName(char * *aClassName);
  NS_IMETHOD GetScriptableFlags(PRUint32 *aScriptableFlags);
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext * cx, JSObject * obj, jsval id, jsval * vp, PRBool *_retval);
  NS_IMETHOD NewEnumerate(nsIXPConnectWrappedNative *wrapper, JSContext * cx, JSObject * obj, PRUint32 enum_op, jsval * statep, jsid *idp, PRBool *_retval);
  NS_IMETHOD NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext * cx, JSObject * obj, jsval id, PRUint32 flags, JSObject * *objp, PRBool *_retval);
  
  sbScriptableFilter( sbIFilterableMediaListView *aMediaListView,
                      const nsAString &aPropertyName,
                      sbRemotePlayer* aPlayer);
  
  /**
   * Add a property to this filter
   * \@see sbIPropertyArray::AppendProperty
   */
  nsresult AppendProperty( const nsAString &aName,
                           const nsAString &aValue );
  

private:
  ~sbScriptableFilter();

protected:
  friend class sbScriptableFilterResult;

protected:
  // iterator over the enumerator and fill out the list
  nsresult ReadEnumerator();

protected:
  nsCOMPtr<sbIFilterableMediaListView> mListView;
  nsString mPropertyName;
  nsStringArray mStrings;
  PRInt32 mEnumeratorIndex;
  PRBool mHasProps;
  nsRefPtr<sbRemotePlayer> mPlayer;
};

#endif // __SB_SCRIPTABLE_LIST_FILTER_H__
