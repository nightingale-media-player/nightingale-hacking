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

#ifndef __SB_VIDEOBOX_H__
#define __SB_VIDEOBOX_H__

#include <sbIVideoBox.h>
#include <nsAutoLock.h>

class sbVideoBox : public sbIVideoBox
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIVIDEOBOX

  sbVideoBox();

  nsresult Init(PRUint32 aWidth, 
                PRUint32 aHeight, 
                PRUint32 aParNumerator = 1, 
                PRUint32 aParDenominator = 1);

protected:
  virtual ~sbVideoBox();

  PRUint32 mPARNumerator;
  PRUint32 mPARDenominator;

  PRUint32 mWidth;
  PRUint32 mHeight;
};

#endif /* __SB_VIDEOBOX_H__ */
