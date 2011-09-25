/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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

#ifndef _SB_CHARSET_DETECTOR_H_
#define _SB_CHARSET_DETECTOR_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Nightingale charset detector defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbCharsetDetector.h
 * \brief Nightingale Charset Detector Definitions.
 */

//------------------------------------------------------------------------------
//
// Nightingale charset detector imported services.
//
//------------------------------------------------------------------------------

// Mozilla imports.
#include <nsCOMPtr.h>
#include <nsICharsetDetectionObserver.h>
#include <nsStringGlue.h>

// Nightingale imports.
#include <sbICharsetDetector.h>


//------------------------------------------------------------------------------
//
// Nightingale charset detector definitions.
//
//------------------------------------------------------------------------------

//
// Nightingale charset detector XPCOM component definitions.
//

#define SB_CHARSETDETECTOR_CLASSNAME "sbCharsetDetector"
#define SB_CHARSETDETECTOR_DESCRIPTION "Nightingale Charset Detector Utilities"
#define SB_CHARSETDETECTOR_CID                                                 \
{                                                                              \
  0x32f9b560,                                                                  \
  0x4830,                                                                      \
  0x11df,                                                                      \
  { 0x98, 0x79, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66 }                           \
}


//------------------------------------------------------------------------------
//
// Nightingale charset detector classes.
//
//------------------------------------------------------------------------------

/**
 * This class provides charset detect utilities.
 */

class nsICharsetDetector;

class sbCharsetDetector : public sbICharsetDetector,
                          public nsICharsetDetectionObserver
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // Implemented interfaces.
  //

  NS_DECL_ISUPPORTS
  NS_DECL_SBICHARSETDETECTOR

  //
  // Public services.
  //

  sbCharsetDetector();

  virtual ~sbCharsetDetector();

  /* nsICharsetDetectionObserver */
  NS_IMETHOD Notify(const char* aCharset, nsDetectionConfident aConf);

private:

  nsresult RunCharsetDetector(const nsACString& aStringToDetect);
 
  nsCOMPtr<nsICharsetDetector> mDetector;
  nsCString mDetectedCharset;
  nsCString mLastCharset;
  nsDetectionConfident mLastConfidence;
  PRBool mIsCharsetFound;
  PRBool mIsDone;
};


#endif /* _SB_CHARSET_DETECTOR_H_ */

