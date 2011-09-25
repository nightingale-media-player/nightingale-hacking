/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

#ifndef __SB_PROXIEDSERVICES_H__
#define __SB_PROXIEDSERVICES_H__

#include "sbIProxiedServices.h"

#include <nsCOMPtr.h>
#include <nsIUTF8ConverterService.h>
#include <nsIObserver.h>

class sbProxiedServices : public sbIProxiedServices,
                          public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIPROXIEDSERVICES
  NS_DECL_NSIOBSERVER

  nsresult Init();

private:
  nsCOMPtr<nsIUTF8ConverterService> mUTF8ConverterService;
};

#define NIGHTINGALE_PROXIEDSERVICES_CLASSNAME "sbProxiedServices"
#define NIGHTINGALE_PROXIEDSERVICES_CID \
  /* {5237d83e-dc33-40fb-a12d-caee39fba6a0} */ \
  { 0x5237d83e, 0xdc33, 0x40fb, \
    { 0xa1, 0x2d, 0xca, 0xee, 0x39, 0xfb, 0xa6, 0xa0 } \
  }
#define NIGHTINGALE_PROXIEDSERVICES_CONTRACTID \
  "@getnightingale.com/moz/proxied-services;1"

#endif // __SB_PROXIEDSERVICES_H__
