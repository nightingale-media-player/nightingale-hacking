/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
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
 *=END SONGBIRD GPL
 */

#ifndef __SBIMAGELABELLINKPROPERTYBUILDER_H__
#define __SBIMAGELABELLINKPROPERTYBUILDER_H__

#include "sbImageLinkPropertyInfo.h"

#include <sbIPropertyManager.h>
#include <sbIPropertyBuilder.h>

#include <nsClassHashtable.h>
#include <nsCOMPtr.h>
#include <nsStringGlue.h>

#include "sbAbstractPropertyBuilder.h"

class sbImageLabelLinkPropertyBuilder : public sbAbstractPropertyBuilder,
                                        public sbIImageLabelLinkPropertyBuilder
{
  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_SBIPROPERTYBUILDER_NO_GET(sbAbstractPropertyBuilder::)
  NS_DECL_SBIIMAGELABELLINKPROPERTYBUILDER

public:
  sbImageLabelLinkPropertyBuilder();
  virtual ~sbImageLabelLinkPropertyBuilder();

public:
  nsresult Init();
  
  /* partial implementation of sbIPropertyBuilder */
  NS_IMETHOD Get(sbIPropertyInfo** _retval);
private:
  nsClassHashtable<nsCStringHashKey, nsCString> *mImages;
  nsClassHashtable<nsCStringHashKey, nsString> *mLabels;
};

#endif /* __SBIMAGELABELLINKPROPERTYBUILDER_H__ */

