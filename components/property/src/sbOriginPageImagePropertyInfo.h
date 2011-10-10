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

#ifndef __SBORIGINPAGEIMAGEPROPERTYINFO_H__
#define __SBORIGINPAGEIMAGEPROPERTYINFO_H__

#include "sbImageLinkPropertyInfo.h"

#include <sbIPropertyManager.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>

#include <nsIFaviconService.h>

class sbOriginPageImagePropertyInfo : public sbImageLinkPropertyInfo
{
public:

  sbOriginPageImagePropertyInfo(const nsAString& aPropertyID,
                                const nsAString& aDisplayName,
                                const nsAString& aLocalizationKey,
                                const PRBool aRemoteReadable,
                                const PRBool aRemoteWritable,
                                const PRBool aUserViewable,
                                const PRBool aUserEditable);

  NS_IMETHOD GetCellProperties(const nsAString& aValue, nsAString& _retval);
  NS_IMETHOD GetImageSrc(const nsAString& aValue, nsAString& _retval);
  NS_IMETHOD GetPreventNavigation(const nsAString& aImageValue,
                                  const nsAString& aUrlValue,
                                  PRBool *_retval);

  nsresult Init();
  virtual ~sbOriginPageImagePropertyInfo() {}

protected:

  nsCOMPtr<nsIFaviconService> mFaviconService;
};

#endif /* __SBIMAGEPROPERTYINFO_H__ */

