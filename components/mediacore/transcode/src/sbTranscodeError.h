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

#ifndef SBTRANSCODEERROR_H_
#define SBTRANSCODEERROR_H_

#include <nsIScriptError.h>
#include <nsISupportsPrimitives.h>

#include "sbITranscodeError.h"

#include <mozilla/Mutex.h>
#include <nsCOMPtr.h>
#include <nsStringGlue.h>

class sbTranscodeError : public sbITranscodeError,
                         public nsIScriptError,
                         public nsISupportsString
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICONSOLEMESSAGE
  NS_DECL_NSISCRIPTERROR
  NS_DECL_NSISUPPORTSSTRING
  NS_DECL_NSISUPPORTSPRIMITIVE
  NS_DECL_SBITRANSCODEERROR

  sbTranscodeError();

private:
  ~sbTranscodeError();

protected:
  mozilla::Mutex mLock;
  nsString mMessageWithItem;
  nsString mMessageWithoutItem;
  nsString mDetails;
  PRUint32 mSeverity;
  nsString mSrcUri;
  nsCOMPtr<sbIMediaItem> mSrcItem;
  nsString mDestUri;
  nsCOMPtr<sbIMediaItem> mDestItem;
};

#define SONGBIRD_TRANSCODEERROR_CLASSNAME                  \
  "Songbird Transcode Error"
#define SONGBIRD_TRANSCODEERROR_CID                        \
{ /* {9B0CDBF9-E4FA-4a5e-9E8A-C87AB37AA187} */             \
  0x9b0cdbf9,                                              \
  0xe4fa,                                                  \
  0x4a5e,                                                  \
  { 0x9e, 0x8a, 0xc8, 0x7a, 0xb3, 0x7a, 0xa1, 0x87 }       \
}

#endif /* SBTRANSCODEERROR_H_ */
