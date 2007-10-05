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

#ifndef __SB_REMOTE_API_H__
#define __SB_REMOTE_API_H__

#include <sbISecurityAggregator.h>
#include <sbISecurityMixin.h>
#include <nsAutoPtr.h>
#include <nsMemory.h>
#include "sbSecurityMixin.h"

#ifndef LOG
#define LOG(args) /* nothing */
#endif

#define SB_DECL_SECURITYCHECKEDCOMP_INIT virtual nsresult Init();

#define SB_IMPL_SECURITYCHECKEDCOMP_INIT_CUSTOM( _class, _mixin, _mixinArgs ) \
nsresult                                                                      \
_class::Init()                                                                \
{                                                                             \
  LOG(( "%s::Init()", #_class ));                                             \
  nsresult rv;                                                                \
  nsRefPtr<_mixin> mixin = new _mixin _mixinArgs;                             \
  NS_ENSURE_TRUE( mixin, NS_ERROR_OUT_OF_MEMORY );                            \
  /* Get the list of IIDs to pass to the security mixin */                    \
  nsIID **iids;                                                               \
  PRUint32 iidCount;                                                          \
  GetInterfaces( &iidCount, &iids );                                          \
  /* initialize our mixin with approved interfaces, methods, properties */    \
  rv = mixin->Init( (sbISecurityAggregator*)this,                             \
                    ( const nsIID** )iids, iidCount,                          \
                    sPublicMethods, NS_ARRAY_LENGTH(sPublicMethods),          \
                    sPublicRProperties, NS_ARRAY_LENGTH(sPublicRProperties),  \
                    sPublicWProperties, NS_ARRAY_LENGTH(sPublicWProperties) );\
  NS_ENSURE_SUCCESS( rv, rv );                                                \
  NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(iidCount, iids);                      \
  mSecurityMixin = do_QueryInterface(                                         \
    NS_ISUPPORTS_CAST( sbISecurityMixin*, mixin ), &rv );                     \
  NS_ENSURE_SUCCESS( rv, rv );                                                \
  return NS_OK;                                                               \
}

#define SB_IMPL_SECURITYCHECKEDCOMP_INIT(_class)                              \
  SB_IMPL_SECURITYCHECKEDCOMP_INIT_CUSTOM( _class, sbSecurityMixin, () )

#endif // __SB_REMOTE_API_H__
