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

#include <sbClassInfoUtils.h>
#include "sbRemoteLibrary.h"
#include "sbRemoteWrappingStringEnumerator.h"

#include <nsComponentManagerUtils.h>
#include <nsIProgrammingLanguage.h>
#include <nsMemory.h>

const static char* sPublicWProperties[] =
{
  // Need this empty string to make VC happy
  ""
};

const static char* sPublicRProperties[] =
{
  // Need this empty string to make VC happy
  ""
};

const static char* sPublicMethods[] =
{
  // helper for helper classes like this
  // nsIStringEnumerator
  "helper:hasMore",
  "helper:getNext"
};

NS_IMPL_ISUPPORTS4( sbRemoteWrappingStringEnumerator,
                    nsIClassInfo,
                    nsISecurityCheckedComponent,
                    sbISecurityAggregator,
                    nsIStringEnumerator )

NS_IMPL_CI_INTERFACE_GETTER3( sbRemoteWrappingStringEnumerator,
                              nsISecurityCheckedComponent,
                              nsIStringEnumerator,
                              sbISecurityAggregator )

SB_IMPL_CLASSINFO_INTERFACES_ONLY(sbRemoteWrappingStringEnumerator)

SB_IMPL_SECURITYCHECKEDCOMP_INIT(sbRemoteWrappingStringEnumerator)

