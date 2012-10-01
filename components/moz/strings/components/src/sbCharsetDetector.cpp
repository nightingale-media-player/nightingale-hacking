/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird charset detect utility.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbCharsetDetector.cpp
 * \brief Songbird Charset Detector Source.
 */

//------------------------------------------------------------------------------
//
// Songbird charset detect utility imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbCharsetDetector.h"

// Mozilla imports.
#include <nsComponentManagerUtils.h>
#include <nsICharsetDetector.h>

// Songbird imports.
#include <sbStringUtils.h>

/* Windows Specific */
#if defined(XP_WIN)
  #include <windows.h>
#endif

//------------------------------------------------------------------------------
//
// Songbird charset detect utilities nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS2(sbCharsetDetector,
                              sbICharsetDetector,
                              nsICharsetDetectionObserver)


//------------------------------------------------------------------------------
//
// Songbird charset detect utilities sbICharsetDetector implementation.
//
//------------------------------------------------------------------------------

NS_IMETHODIMP
sbCharsetDetector::GetIsCharsetFound(bool *aIsCharsetFound)
{
  NS_ENSURE_ARG_POINTER(aIsCharsetFound);

  *aIsCharsetFound = mIsCharsetFound;
  return NS_OK;
}

NS_IMETHODIMP
sbCharsetDetector::Detect(const nsACString& aStringToDetect)
{
  nsresult rv;

  // Already have the answer for the charset.
  if (mIsCharsetFound)
    return NS_OK;

  if (!mDetector) {
    mDetector = do_CreateInstance(
      NS_CHARSET_DETECTOR_CONTRACTID_BASE "universal_charset_detector");

    nsCOMPtr<nsICharsetDetectionObserver> observer =
      do_QueryInterface(NS_ISUPPORTS_CAST(nsICharsetDetectionObserver*, this));
    NS_ENSURE_TRUE(observer, NS_ERROR_NO_INTERFACE);

    rv = mDetector->Init(observer);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // see if it's valid utf8; if yes, assume it _is_ indeed utf8 for now.
  // Do not set mIsCharsetFound as there could be more incoming data so that
  // we can get a better answer.
  nsDependentCString raw(aStringToDetect.BeginReading());
  if (IsLikelyUTF8(raw) && IsUTF8(raw)) {
    // this is utf8
    mDetectedCharset.AssignLiteral("UTF-8");
    return NS_OK;
  }

  // the metadata is in some 8-bit encoding; try to guess
  rv = RunCharsetDetector(aStringToDetect);
  if (NS_SUCCEEDED(rv) && !mLastCharset.IsEmpty()) {
    mDetectedCharset.Assign(mLastCharset);
    if (eSureAnswer == mLastConfidence || eBestAnswer == mLastConfidence)
      mIsCharsetFound = PR_TRUE;

    return NS_OK;
  }

#if XP_WIN
  // we have no idea what charset this is, but we know it's bad.
  // for Windows only, assume CP_ACP

  // make the call fail if it's not valid CP_ACP
  const char *str = aStringToDetect.BeginReading();
  int size = MultiByteToWideChar(CP_ACP,
                                 MB_ERR_INVALID_CHARS,
                                 str,
                                 aStringToDetect.Length(),
                                 nsnull,
                                 0);
  if (size) {
    // okay, so CP_ACP is usable
    mDetectedCharset.AssignLiteral("CP_ACP");
    return NS_OK;
  }
#endif

  // we truely know nothing
  mDetectedCharset.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbCharsetDetector::Finish(nsACString& _retval)
{
  // The charset detection is not done yet. Get the best answer so far.
  if (!mIsDone) {
    if (mDetector) {
      nsresult rv = mDetector->Done();
      NS_ENSURE_SUCCESS(rv, rv);
    }
    if (!mLastCharset.IsEmpty())
      mDetectedCharset = mLastCharset;
  }

  mLastConfidence = eNoAnswerYet;
  mIsCharsetFound = PR_FALSE;
  mIsDone = PR_FALSE;
  // sadly, the detector has no way to unregister a listener; in order to break
  // the reference cycle between this and mDetector, we need to let it go and
  // just make a new one the next time we need it.
  mDetector = nsnull;

  _retval = mDetectedCharset;

  return NS_OK;
}

//------------------------------------------------------------------------------
//
// Songbird charset detect nsICharsetDetectionObserver implementation.
//
//------------------------------------------------------------------------------

NS_IMETHODIMP
sbCharsetDetector::Notify(const char *aCharset, nsDetectionConfident aConf)
{
  mLastCharset.AssignLiteral(aCharset);
  mLastConfidence = aConf;
  return NS_OK;
}

//------------------------------------------------------------------------------
//
// Songbird charset detect utilities public services.
//
//------------------------------------------------------------------------------

/**
 * Construct a Songbird charset detect utilities object.
 */

sbCharsetDetector::sbCharsetDetector()
: mLastConfidence(eNoAnswerYet),
  mIsCharsetFound(PR_FALSE),
  mIsDone(PR_FALSE)
{
}


/**
 * Destroy a Songbird charset detect utilities object.
 */

sbCharsetDetector::~sbCharsetDetector()
{
}

//------------------------------------------------------------------------------
//
// Songbird charset detect utilities private methods.
//
//------------------------------------------------------------------------------

nsresult sbCharsetDetector::RunCharsetDetector(const nsACString& aStringToDetect)
{
  NS_ENSURE_TRUE(mDetector, NS_ERROR_NOT_INITIALIZED);
  nsresult rv = NS_OK;

  if (NS_SUCCEEDED(rv)) {
    PRUint32 chunkSize = aStringToDetect.Length();
    const char *str = aStringToDetect.BeginReading();
    rv = mDetector->DoIt(str, chunkSize, &mIsDone);
    NS_ENSURE_SUCCESS(rv, rv);
    if (mIsDone) {
      rv = mDetector->Done();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return rv;
}
