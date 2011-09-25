/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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

#ifndef __SBDUMMYPLAYLISTPROPERTYINFO_H__
#define __SBDUMMYPLAYLISTPROPERTYINFO_H__

#include "sbDummyPropertyInfo.h"

#include <sbIPropertyManager.h>
#include <sbITreeViewPropertyInfo.h>
#include <sbIClickablePropertyInfo.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>

class sbDummyPlaylistPropertyInfo : public sbDummyPropertyInfo
{
public:

  NS_DECL_ISUPPORTS_INHERITED

  sbDummyPlaylistPropertyInfo();
  virtual ~sbDummyPlaylistPropertyInfo() {}

  virtual nsresult Init();
};

#endif /* __SBDUMMYPLAYLISTPROPERTYINFO_H__ */

