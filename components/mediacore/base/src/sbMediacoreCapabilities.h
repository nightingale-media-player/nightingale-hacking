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
 * \file  sbMediacoreCapabilities.h
 * \brief Songbird Mediacore Capabilities Definition.
 */

#ifndef __SB_MEDIACORECAPABILITIES_H__
#define __SB_MEDIACORECAPABILITIES_H__

#include <sbIMediacoreCapabilities.h>

#include <mozilla/Mutex.h>
#include <nsStringGlue.h>
#include <nsTArray.h>

#include <sbTArrayStringEnumerator.h>

class sbMediacoreCapabilities : public sbIMediacoreCapabilities
{
  typedef nsTArray<nsString> sbStringArray;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIACORECAPABILITIES

  sbMediacoreCapabilities();

  nsresult Init();

  nsresult SetAudioExtensions(
    const sbStringArray &aAudioExtensions);
  nsresult SetVideoExtensions(
    const sbStringArray &aVideoExtensions);
  nsresult SetImageExtensions(
    const sbStringArray &aImageExtensions);

  nsresult SetSupportsAudioPlayback(PRBool aSupportsAudioPlayback);
  nsresult SetSupportsVideoPlayback(PRBool aSupportsVideoPlayback);
  nsresult SetSupportsImagePlayback(PRBool aSupportsImagePlayback);

  nsresult SetSupportsAudioTranscode(PRBool aSupportsAudioTranscode);
  nsresult SetSupportsVideoTranscode(PRBool aSupportsVideoTranscode);
  nsresult SetSupportsImageTranscode(PRBool aSupportsImageTranscode);

protected:
  virtual ~sbMediacoreCapabilities();

  mozilla::Mutex mLock;

  PRBool mSupportsAudioPlayback;
  PRBool mSupportsVideoPlayback;
  PRBool mSupportsImagePlayback;

  PRBool mSupportsAudioTranscode;
  PRBool mSupportsVideoTranscode;
  PRBool mSupportsImageTranscode;

  sbStringArray mAudioExtensions;
  sbStringArray mVideoExtensions;
  sbStringArray mImageExtensions;
};

#endif /* __SB_MEDIACORECAPABILITIES_H__ */
