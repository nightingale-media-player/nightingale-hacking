/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is XPCOM Array implementation.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corp.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Alec Flett <alecf@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#ifndef nsArray_h__
#define nsArray_h__

#include "nsIMutableArray.h"
#include "sbCOMArray.h"
#include "nsCOMPtr.h"
#include <mozilla/Mutex.h>

#define SB_THREADSAFE_ARRAY_CLASSNAME \
  "threadsafe nsIArray implementation"

// {14FA3C6C-868D-445d-A3B5-D18B77BF8B36}
#define SB_THREADSAFE_ARRAY_CID \
{ 0x14fa3c6c, 0x868d, 0x445d, \
  { 0xa3, 0xb5, 0xd1, 0x8b, 0x77, 0xbf, 0x8b, 0x36 } }

#define SB_THREADSAFE_ARRAY_CONTRACTID \
  "@songbirdnest.com/moz/xpcom/threadsafe-array;1"

// threadsafe (i.e. spuriously locked) version of nsArray, which is an
// adapter class to map nsIArray->nsCOMArray
// do NOT declare this as a stack or member variable, use
// nsCOMArray instead!
class sbArray : public nsIMutableArray
{
public:
    sbArray();
    sbArray(const sbCOMArray_base& aBaseArray);
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIARRAY
    NS_DECL_NSIMUTABLEARRAY

private:
    ~sbArray();

    sbCOMArray_base mArray;
    mozilla::Mutex mLock;
};

#endif
