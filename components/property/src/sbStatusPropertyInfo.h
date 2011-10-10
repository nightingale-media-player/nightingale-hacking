/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.getnightingale.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END NIGHTINGALE GPL
 */

#ifndef __SBSTATUSPROPERTYINFO_H__
#define __SBSTATUSPROPERTYINFO_H__

// Local includes
#include "sbImmutablePropertyInfo.h"

// Mozilla includes
#include <nsCOMPtr.h>
#include <nsStringGlue.h>

// Nightingale includes
#include <sbITreeViewPropertyInfo.h>

class sbStatusPropertyInfo : public sbImmutablePropertyInfo,
                             public sbITreeViewPropertyInfo
{
public:

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_SBITREEVIEWPROPERTYINFO

  sbStatusPropertyInfo(const nsAString& aPropertyID,
                       const nsAString& aDisplayName,
                       const nsAString& aLocalizationKey,
                       const nsAString& aLabel,
                       const nsAString& aCompletedLabel,
                       const nsAString& aFailedLabel,
                       const PRBool aRemoteReadable,
                       const PRBool aRemoteWritable,
                       const PRBool aUserViewable,
                       const PRBool aUserEditable);
  virtual ~sbStatusPropertyInfo() {}

  NS_IMETHOD Format(const nsAString& aValue, nsAString& _retval);

  nsresult Init();

private:

  nsString mLabel;
  nsString mCompletedLabel;
  nsString mFailedLabel;
};

#endif /* __SBSTATUSPROPERTYINFO_H__ */

