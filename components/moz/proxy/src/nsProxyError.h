/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef nsProxyError_h__
#define nsProxyError_h__


#include <nsError.h>

/* For COM compatibility reasons, we want to use exact error code numbers
   for NS_ERROR_PROXY_INVALID_IN_PARAMETER and NS_ERROR_PROXY_INVALID_OUT_PARAMETER.
   The first matches:

     #define RPC_E_INVALID_PARAMETER          _HRESULT_TYPEDEF_(0x80010010L)

   Errors returning this mean that the xpcom proxy code could not create a proxy for
   one of the in paramaters.

   Because of this, we are ignoring the convention if using a base and offset for
   error numbers.

*/

/* Returned when a proxy could not be create a proxy for one of the IN parameters
   This is returned only when the "real" method has NOT been invoked.
*/

#define NS_ERROR_PROXY_INVALID_IN_PARAMETER        ((nsresult) 0x80010010L)

/* Returned when a proxy could not be create a proxy for one of the OUT parameters
   This is returned only when the "real" method has ALREADY been invoked.
*/

#define NS_ERROR_PROXY_INVALID_OUT_PARAMETER       ((nsresult) 0x80010011L)
#endif
