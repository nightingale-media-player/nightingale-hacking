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

/** 
* \file  sbMediacoreError.cpp
* \brief Songbird Mediacore Error Implementation.
*/
#include "sbMediacoreError.h"

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbMediacoreError:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gMediacoreError = nsnull;
#define TRACE(args) PR_LOG(gMediacoreError, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gMediacoreError, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

NS_IMPL_THREADSAFE_ISUPPORTS1(sbMediacoreError, 
                              sbIMediacoreError)

sbMediacoreError::sbMediacoreError()
: mLock(nsnull)
, mCode(sbIMediacoreError::UNINITIALIZED)
{
  MOZ_COUNT_CTOR(sbMediacoreError);

#ifdef PR_LOGGING
  if (!gMediacoreError)
    gMediacoreError= PR_NewLogModule("sbMediacoreError");
#endif

  TRACE(("sbMediacoreError[0x%x] - Created", this));
}

sbMediacoreError::~sbMediacoreError()
{
  TRACE(("sbMediacoreError[0x%x] - Destroyed", this));
  MOZ_COUNT_DTOR(sbMediacoreError);
}

nsresult 
sbMediacoreError::Init(PRUint32 aCode, 
                       const nsAString &aMessage)
{
  TRACE(("sbMediacoreError[0x%x] - Init", this));

  mozilla::MutexAutoLock lock(mLock);

  mCode = aCode;
  mMessage = aMessage;

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreError::GetCode(PRUint32 *aCode)
{
  TRACE(("sbMediacoreError[0x%x] - GetCode", this));

  NS_ENSURE_ARG_POINTER(aCode);

  mozilla::MutexAutoLock lock(mLock);
  *aCode = mCode;

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreError::GetMessage(nsAString & aMessage)
{
  TRACE(("sbMediacoreError[0x%x] - GetMessage", this));

  mozilla::MutexAutoLock lock(mLock);
  aMessage = mMessage;

  return NS_OK;
}
