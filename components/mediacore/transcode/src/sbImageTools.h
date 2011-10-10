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

#ifndef __SBIMAGETOOLS_H__
#define __SBIMAGETOOLS_H__

#include <imgIContainer.h>
#include <nsIInputStream.h>
#include <nsStringAPI.h>


/* Replacements for imgTools with bugs fixed. Should later become a real
   component.

   For now, this exists because imgTools::DecodeImageData() fails on GIF images
   (because the GIF decoder returns NS_ERROR_NOT_IMPLEMENTED from ::Flush()).
 */
class sbImageTools
{
public:
  static nsresult DecodeImageData(nsIInputStream* aInStr,
                                  const nsACString& aMimeType,
                                  imgIContainer **aContainer);
};

#endif /*__SBIMAGETOOLS_H__*/

