/*
 * BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale Media Player.
 *
 * Copyright(c) 2013
 * http://getnightingale.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the "GPL").
 *
 * Software distributed under the License is distributed
 * on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END NIGHTINGALE GPL
 */

#ifndef __SB_SCRIPTABLE_FILTER_RESULTS_H__
#define __SB_SCRIPTABLE_FILTER_RESULTS_H__

#include "ngXPCScriptable.h"

#include <sbIScriptableFilterResult.h>

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsIClassInfo.h>
#include <nsISecurityCheckedComponent.h>

#define SB_SCRIPTABLEFILTERRESULT_CID                   \
{ /* 30944f15-3d6f-40b8-a133-52673bca2c7b */            \
  0x30944f15,                                           \
  0x3d6f,                                               \
  0x40b8,                                               \
  {0xa1, 0x33, 0x52, 0x67, 0x3b, 0xca, 0x2c, 0x7b}      \
}

class sbIFilterableMediaListView;
class sbRemotePlayer;

/**
 * The results of filtering via sbScriptableListFilter
 * Has the property .items as well as the ability to further filter
 */
class sbScriptableFilterResult : public sbIScriptableFilterResult,
                                 public nsISecurityCheckedComponent,
                                 public nsIClassInfo,
                                 public ngXPCScriptable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_NSISECURITYCHECKEDCOMPONENT
  NS_DECL_SBISCRIPTABLEFILTERRESULT

  /* nsIXPCScriptable */
  NS_IMETHOD GetClassName(char * *aClassName);
  NS_IMETHOD GetScriptableFlags(PRUint32 *aScriptableFlags);
  NS_IMETHOD Equality( nsIXPConnectWrappedNative *wrapper,
                       JSContext * cx,
                       JSObject * obj,
                       jsval val,
                       PRBool *_retval );

  sbScriptableFilterResult( sbIFilterableMediaListView *aMediaListView,
                            sbRemotePlayer* aRemotePlayer );

private:
  ~sbScriptableFilterResult();

protected:
  nsCOMPtr<sbIFilterableMediaListView> mListView;
  nsRefPtr<sbRemotePlayer> mPlayer;

};

#endif // __SB_SCRIPTABLE_FILTER_RESULTS_H__
