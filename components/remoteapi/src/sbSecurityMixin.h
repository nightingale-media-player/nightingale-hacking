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

#ifndef __SB_SECURITY_MIXIN_H__
#define __SB_SECURITY_MIXIN_H__

#include "sbISecurityMixin.h"
#include "sbISecurityAggregator.h"

#include <nsCOMPtr.h>
#include <nsISecurityCheckedComponent.h>
#include <nsIURI.h>
#include <nsStringGlue.h>
#include <nsTArray.h>

#define SONGBIRD_SECURITYMIXIN_CONTRACTID                 \
  "@songbirdnest.com/remoteapi/security-mixin;1"
#define SONGBIRD_SECURITYMIXIN_CLASSNAME                  \
  "Songbird Remote Security Mixin"
#define SONGBIRD_SECURITYMIXIN_CID                        \
{ /* aaae98ec-386e-405e-b109-cf1a872ef6dd */              \
  0xaaae98ec,                                             \
  0x386e,                                                 \
  0x405e,                                                 \
  {0xb1, 0x09, 0xcf, 0x1a, 0x87, 0x2e, 0xf6, 0xdd}        \
}

extern char* SB_CloneAllAccess();

class sbSecurityMixin : public nsISecurityCheckedComponent,
                        public nsIClassInfo,
                        public sbISecurityMixin
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_NSISECURITYCHECKEDCOMPONENT
  NS_DECL_SBISECURITYMIXIN

  sbSecurityMixin();

protected:
  virtual ~sbSecurityMixin();

  // helpers for resolving the prefs and permissions
  PRBool GetPermission(nsIURI *aURI, const char *aType, const char *aRAPIPref);
  PRBool GetPermissionForScopedName(const nsAString &aScopedName);
  PRBool GetScopedName(nsTArray<nsCString> &aStringArray,
                       const nsAString &aMethodName,
                       nsAString &aScopedName);

  // helper function for allocating IID array 
  nsresult CopyIIDArray(PRUint32 aCount, const nsIID **aSourceArray, nsIID ***aDestArray);

  // helper function for copying over a string array
  nsresult CopyStrArray(PRUint32 aCount, const char **aSourceArray, nsTArray<nsCString> *aDestArray);

  // non-binding ref, we don't want to keep our outer from going away
  sbISecurityAggregator* mOuter;  

  // holders for the lists of approved methods, properties and interfaces
  // passed in to us by the mOuter ( in our Init() method )
  nsIID **mInterfaces;
  PRUint32 mInterfacesCount;
  nsTArray<nsCString> mMethods;
  nsTArray<nsCString> mRProperties;
  nsTArray<nsCString> mWProperties;
};

#endif // __SB_SECURITY_MIXIN_H__

