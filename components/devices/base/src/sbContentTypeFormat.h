/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

#ifndef __SBCONTENTTYPEFORMAT_H__
#define __SBCONTENTTYPEFORMAT_H__

#include "sbIContentTypeFormat.h"

#include <nsTArray.h>

struct PRLock;

class sbContentTypeFormat : public sbIContentTypeFormat
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBICONTENTTYPEFORMAT

  sbContentTypeFormat();

private:
  ~sbContentTypeFormat();

protected:
  /* additional members */
  FourCC mContainerFormat;
  nsTArray<FourCC> mEncodingFormats;
  nsTArray<FourCC> mDecodingFormats;
  friend class FourCCEnumerator;
  
  PRBool mHasInitialized;
  PRLock* mInitLock;
};

#define SONGBIRD_CONTENTTYPEFORMAT_DESCRIPTION                \
  "Songbird Content Type Format Component"
#define SONGBIRD_CONTENTTYPEFORMAT_CONTRACTID                 \
  "@songbirdnest.com/Songbird/Device/ContentTypeFormat;1"
#define SONGBIRD_CONTENTTYPEFORMAT_CLASSNAME                  \
  "Songbird Content Type Format"
#define SONGBIRD_CONTENTTYPEFORMAT_CID                        \
  /* {E4AD189F-F5B1-4500-8FC0-DAE48E93E497} */                \
  { 0xe4ad189f, 0xf5b1, 0x4500,                               \
    { 0x8f, 0xc0, 0xda, 0xe4, 0x8e, 0x93, 0xe4, 0x97 } }

#endif /* __SBCONTENTTYPEFORMAT_H__ */
