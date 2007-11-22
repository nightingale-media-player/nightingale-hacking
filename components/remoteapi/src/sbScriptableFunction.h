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

#ifndef __SB_SCRIPTABLE_FUNCTION_H__
#define __SB_SCRIPTABLE_FUNCTION_H__

#include "sbXPCScriptableStub.h"

#include <nsCOMPtr.h>
#include <nsIClassInfo.h>
#include <nsISecurityCheckedComponent.h>

/**
 * This is a stub class that takes an object and some interface, and
 * simply returns it when called as a JS function.  It exists to help preserve
 * the Songbird 0.3 API.
 */

class sbScriptableFunction : public nsISecurityCheckedComponent,
                             public sbXPCScriptableStub
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_NSISECURITYCHECKEDCOMPONENT
 
  /* nsIXPCScriptable */
  NS_IMETHOD GetClassName(char * *aClassName);
  NS_IMETHOD GetScriptableFlags(PRUint32 *aScriptableFlags);
  NS_IMETHOD Call(nsIXPConnectWrappedNative *wrapper, JSContext * cx, JSObject * obj, PRUint32 argc, jsval * argv, jsval * vp, PRBool *_retval);

  sbScriptableFunction(nsISupports *aObject, const nsIID& aIID);

private:
  ~sbScriptableFunction();

protected:
  nsCOMPtr<nsISupports> mObject;
  nsIID mIID;
};

#endif // __SB_SCRIPTABLE_FUNCTION_H__
