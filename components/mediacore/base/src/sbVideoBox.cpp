/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 *  governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#include "sbVideoBox.h"

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbVideoBox:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo * VideoBox() {
  static PRLogModuleInfo* gVideoBox = PR_NewLogModule("sbVideoBox");
  return gVideoBox;
}

#define TRACE(args) PR_LOG(VideoBox() , PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(VideoBox() , PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

NS_IMPL_THREADSAFE_ISUPPORTS1(sbVideoBox,
                              sbIVideoBox)

sbVideoBox::sbVideoBox()
: mPARNumerator(0)
, mPARDenominator(0)
{
  TRACE(("sbVideoBox[0x%x] - Created", this));
}

sbVideoBox::~sbVideoBox()
{
  TRACE(("sbVideoBox[0x%x] - Destroyed", this));
}

nsresult
sbVideoBox::Init(PRUint32 aWidth, 
                 PRUint32 aHeight, 
                 PRUint32 aParNumerator /*= 1*/, 
                 PRUint32 aParDenominator /*= 1*/)
{
  TRACE(("sbVideoBox[0x%x] - Init", this));

  mWidth = aWidth;
  mHeight = aHeight;
  mPARNumerator = aParNumerator;
  mPARDenominator = aParDenominator;

  return NS_OK;
}

NS_IMETHODIMP
sbVideoBox::GetParNumerator(PRUint32 *aParNumerator)
{
  TRACE(("sbVideoBox[0x%x] - GetParNumerator", this));
  NS_ENSURE_ARG_POINTER(aParNumerator);

  *aParNumerator = mPARNumerator;

  return NS_OK;
}

NS_IMETHODIMP
sbVideoBox::GetParDenominator(PRUint32 *aParDenominator)
{
  TRACE(("sbVideoBox[0x%x] - GetParDenominator", this));
  NS_ENSURE_ARG_POINTER(aParDenominator);

  *aParDenominator = mPARDenominator;

  return NS_OK;
}

NS_IMETHODIMP
sbVideoBox::GetWidth(PRUint32 *aWidth)
{
  TRACE(("sbVideoBox[0x%x] - GetWidth", this));
  NS_ENSURE_ARG_POINTER(aWidth);

  *aWidth = mWidth;

  return NS_OK;
}

NS_IMETHODIMP
sbVideoBox::GetHeight(PRUint32 *aHeight)
{
  TRACE(("sbVideoBox[0x%x] - GetHeight", this));
  NS_ENSURE_ARG_POINTER(aHeight);

  *aHeight = mHeight;

  return NS_OK;
}
