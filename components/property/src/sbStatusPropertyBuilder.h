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

#ifndef __SBCDRIPSTATUSPROPERTYBUILDER_H__
#define __SBCDRIPSTATUSPROPERTYBUILDER_H__

#include "sbAbstractPropertyBuilder.h"

#include <sbIPropertyBuilder.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>

class sbStatusPropertyBuilder : public sbAbstractPropertyBuilder,
                                public sbIStatusPropertyBuilder
{
public:

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_SBIPROPERTYBUILDER_NO_GET(sbAbstractPropertyBuilder::)
  NS_DECL_SBISTATUSPROPERTYBUILDER

  virtual ~sbStatusPropertyBuilder() {}

  NS_IMETHOD Get(sbIPropertyInfo **_retval);
private:
  nsString mLabel;
  nsString mCompletedLabel;
  nsString mFailedLabel;
  nsString mLabelKey;
  nsString mCompletedLabelKey;
  nsString mFailedLabelKey;
};

#endif /* __SBDOWNLOADBUTTONPROPERTYBUILDER_H__ */

