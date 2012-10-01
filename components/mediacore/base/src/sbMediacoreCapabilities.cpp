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
* \file  sbMediacoreCapabilities.cpp
* \brief Songbird Mediacore Capabilities Implementation.
*/
#include "sbMediacoreCapabilities.h"

#include <nsCOMPtr.h>

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbMediacoreCapabilities:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gMediacoreCapabilities = nsnull;
#define TRACE(args) PR_LOG(gMediacoreCapabilities , PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gMediacoreCapabilities , PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

NS_IMPL_THREADSAFE_ISUPPORTS1(sbMediacoreCapabilities, 
                              sbIMediacoreCapabilities)

sbMediacoreCapabilities::sbMediacoreCapabilities()
: mLock(nsnull)
, mSupportsAudioPlayback(PR_FALSE)
, mSupportsVideoPlayback(PR_FALSE)
, mSupportsImagePlayback(PR_FALSE)
, mSupportsAudioTranscode(PR_FALSE)
, mSupportsVideoTranscode(PR_FALSE)
, mSupportsImageTranscode(PR_FALSE)
{
  MOZ_COUNT_CTOR(sbMediacoreCapabilities);

#ifdef PR_LOGGING
  if (!gMediacoreCapabilities)
    gMediacoreCapabilities = PR_NewLogModule("sbMediacoreCapabilities");
#endif

  TRACE(("sbMediacoreCapabilities[0x%x] - Created", this));
}

sbMediacoreCapabilities::~sbMediacoreCapabilities()
{
  TRACE(("sbMediacoreCapabilities[0x%x] - Destroyed", this));

  MOZ_COUNT_DTOR(sbMediacoreCapabilities);

  if(mLock) {
    nsAutoLock::DestroyLock(mLock);
  }
}

nsresult 
sbMediacoreCapabilities::Init()
{
  TRACE(("sbMediacoreCapabilities[0x%x] - Init", this));

  mLock = nsAutoLock::NewLock("sbMediacoreCapabilities::mLock");
  NS_ENSURE_TRUE(mLock, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

nsresult 
sbMediacoreCapabilities::SetAudioExtensions(
  const sbStringArray &aAudioExtensions)
{
  TRACE(("sbMediacoreCapabilities[0x%x] - SetAudioExtensions", this));

  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  
  nsAutoLock lock(mLock);
  mAudioExtensions = aAudioExtensions;

  return NS_OK;
}

nsresult 
sbMediacoreCapabilities::SetVideoExtensions(
  const sbStringArray &aVideoExtensions)
{
  TRACE(("sbMediacoreCapabilities[0x%x] - SetVideoExtensions", this));

  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);

  nsAutoLock lock(mLock);
  mVideoExtensions = aVideoExtensions;

  return NS_OK;
}

nsresult  
sbMediacoreCapabilities::SetImageExtensions(
  const sbStringArray &aImageExtensions)
{
  TRACE(("sbMediacoreCapabilities[0x%x] - SetImageExtensions", this));

  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);

  nsAutoLock lock(mLock);
  mImageExtensions = aImageExtensions;

  return NS_OK;
}

nsresult  
sbMediacoreCapabilities::SetSupportsAudioPlayback(bool aSupportsAudioPlayback)
{
  TRACE(("sbMediacoreCapabilities[0x%x] - SetSupportsAudioPlayback", this));

  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);

  nsAutoLock lock(mLock);
  mSupportsAudioPlayback = aSupportsAudioPlayback;

  return NS_OK;
}

nsresult  
sbMediacoreCapabilities::SetSupportsVideoPlayback(bool aSupportsVideoPlayback)
{
  TRACE(("sbMediacoreCapabilities[0x%x] - SetSupportsVideoPlayback", this));

  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);

  nsAutoLock lock(mLock);
  mSupportsVideoPlayback = aSupportsVideoPlayback;

  return NS_OK;
}

nsresult  
sbMediacoreCapabilities::SetSupportsImagePlayback(bool aSupportsImagePlayback)
{
  TRACE(("sbMediacoreCapabilities[0x%x] - SetSupportsImagePlayback", this));

  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);

  nsAutoLock lock(mLock);
  mSupportsImagePlayback = aSupportsImagePlayback;

  return NS_OK;
}

nsresult  
sbMediacoreCapabilities::SetSupportsAudioTranscode(bool aSupportsAudioTranscode)
{  
  TRACE(("sbMediacoreCapabilities[0x%x] - SetSupportsAudioTranscode", this));

  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);

  nsAutoLock lock(mLock);
  mSupportsAudioTranscode = aSupportsAudioTranscode;

  return NS_OK;
}

nsresult  
sbMediacoreCapabilities::SetSupportsVideoTranscode(bool aSupportsVideoTranscode)
{
  TRACE(("sbMediacoreCapabilities[0x%x] - SetSupportsVideoTranscode", this));

  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);

  nsAutoLock lock(mLock);
  mSupportsVideoTranscode = aSupportsVideoTranscode;

  return NS_OK;
}

nsresult  
sbMediacoreCapabilities::SetSupportsImageTranscode(bool aSupportsImageTranscode)
{
  TRACE(("sbMediacoreCapabilities[0x%x] - SetSupportsImageTranscode", this));

  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);

  nsAutoLock lock(mLock);
  mSupportsImageTranscode = aSupportsImageTranscode;

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreCapabilities::GetAudioExtensions(nsIStringEnumerator * *aAudioExtensions)
{
  TRACE(("sbMediacoreCapabilities[0x%x] - GetAudioExtensions", this));

  NS_ENSURE_ARG_POINTER(aAudioExtensions);
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  
  nsAutoLock lock(mLock);
  nsCOMPtr<nsIStringEnumerator> audioExtensions = 
    new sbTArrayStringEnumerator(&mAudioExtensions);
  NS_ENSURE_TRUE(audioExtensions, NS_ERROR_OUT_OF_MEMORY);

  audioExtensions.forget(aAudioExtensions);

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreCapabilities::GetVideoExtensions(nsIStringEnumerator * *aVideoExtensions)
{
  TRACE(("sbMediacoreCapabilities[0x%x] - GetVideoExtensions", this));

  NS_ENSURE_ARG_POINTER(aVideoExtensions);
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  
  nsAutoLock lock(mLock);
  nsCOMPtr<nsIStringEnumerator> videoExtensions = 
    new sbTArrayStringEnumerator(&mVideoExtensions);
  NS_ENSURE_TRUE(videoExtensions, NS_ERROR_OUT_OF_MEMORY);

  videoExtensions.forget(aVideoExtensions);

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreCapabilities::GetImageExtensions(nsIStringEnumerator * *aImageExtensions)
{
  TRACE(("sbMediacoreCapabilities[0x%x] - GetImageExtensions", this));

  NS_ENSURE_ARG_POINTER(aImageExtensions);
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  
  nsAutoLock lock(mLock);
  nsCOMPtr<nsIStringEnumerator> imageExtensions = 
    new sbTArrayStringEnumerator(&mImageExtensions);
  NS_ENSURE_TRUE(imageExtensions, NS_ERROR_OUT_OF_MEMORY);

  imageExtensions.forget(aImageExtensions);

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreCapabilities::GetSupportsAudioPlayback(bool *aSupportsAudioPlayback)
{
  TRACE(("sbMediacoreCapabilities[0x%x] - GetSupportsAudioPlayback", this));
  
  NS_ENSURE_ARG_POINTER(aSupportsAudioPlayback);
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  
  nsAutoLock lock(mLock);
  *aSupportsAudioPlayback = mSupportsAudioPlayback;

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreCapabilities::GetSupportsVideoPlayback(bool *aSupportsVideoPlayback)
{
  TRACE(("sbMediacoreCapabilities[0x%x] - GetSupportsVideoPlayback", this));

  NS_ENSURE_ARG_POINTER(aSupportsVideoPlayback);
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  
  nsAutoLock lock(mLock);
  *aSupportsVideoPlayback = mSupportsVideoPlayback;

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreCapabilities::GetSupportsImagePlayback(bool *aSupportsImagePlayback)
{
  TRACE(("sbMediacoreCapabilities[0x%x] - GetSupportsImagePlayback", this));

  NS_ENSURE_ARG_POINTER(aSupportsImagePlayback);
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  
  nsAutoLock lock(mLock);
  *aSupportsImagePlayback = mSupportsImagePlayback;

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreCapabilities::GetSupportsAudioTranscode(bool *aSupportsAudioTranscode)
{
  TRACE(("sbMediacoreCapabilities[0x%x] - GetSupportsAudioTranscode", this));

  NS_ENSURE_ARG_POINTER(aSupportsAudioTranscode);
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  
  nsAutoLock lock(mLock);
  *aSupportsAudioTranscode = mSupportsAudioTranscode;

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreCapabilities::GetSupportsVideoTranscode(bool *aSupportsVideoTranscode)
{
  TRACE(("sbMediacoreCapabilities[0x%x] - GetSupportsVideoTranscode", this));

  NS_ENSURE_ARG_POINTER(aSupportsVideoTranscode);
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  
  nsAutoLock lock(mLock);
  *aSupportsVideoTranscode = mSupportsVideoTranscode;

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreCapabilities::GetSupportsImageTranscode(bool *aSupportsImageTranscode)
{
  TRACE(("sbMediacoreCapabilities[0x%x] - GetSupportsImageTranscode", this));

  NS_ENSURE_ARG_POINTER(aSupportsImageTranscode);
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  
  nsAutoLock lock(mLock);
  *aSupportsImageTranscode = mSupportsImageTranscode;

  return NS_OK;
}
