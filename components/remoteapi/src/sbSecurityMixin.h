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

#include "sbISecurityMixin.h"
#include "sbISecurityAggregator.h"

#include <nsCOMPtr.h>
#include <nsISecurityCheckedComponent.h>
#include <nsStringGlue.h>
#include <nsTArray.h>

#define SONGBIRD_SECURITYMIXIN_CONTRACTID                \
  "@songbirdnest.com/songbird/securitymixin;1"
#define SONGBIRD_SECURITYMIXIN_CLASSNAME                 \
  "Songbird Remote Mixin Security Component"
#define SONGBIRD_SECURITYMIXIN_CID                       \
{ /* aaae98ec-386e-405e-b109-cf1a872ef6dd */              \
  0xaaae98ec,                                             \
  0x386e,                                                 \
  0x405e,                                                 \
  {0xb1, 0x09, 0xcf, 0x1a, 0x87, 0x2e, 0xf6, 0xdd}        \
}

extern char* xpc_CloneAllAccess();

class sbSecurityMixin : public nsISecurityCheckedComponent,
                        public nsIClassInfo,
                        public sbISecurityMixin
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISECURITYCHECKEDCOMPONENT
  NS_DECL_NSICLASSINFO
  NS_DECL_SBISECURITYMIXIN

  sbSecurityMixin();

protected:
  virtual ~sbSecurityMixin();

  // helper function for allocating IID array 
  nsresult CopyIIDArray(PRUint32 aCount, const nsIID **aSourceArray, nsIID ***aDestArray);
  nsresult CopyStrArray(PRUint32 aCount, const PRUnichar **aSourceArray, nsTArray<nsString> *aDestArray);

  sbISecurityAggregator* mOuter;  // non-binding ref, we don't want to keep our outer from going away
  nsIID ** mInterfaces;
  PRUint32 mInterfacesCount;
  nsTArray<nsString> mMethods;
  nsTArray<nsString> mRProperties;
  nsTArray<nsString> mWProperties;
};

