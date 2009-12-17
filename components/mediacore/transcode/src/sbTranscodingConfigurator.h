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
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */


#ifndef __SB_TRANSCODINGCONFIGURATOR_H__
#define __SB_TRANSCODINGCONFIGURATOR_H__

// Songbird Includes
#include <sbIMediaInspector.h>
#include <sbITranscodingConfigurator.h>

// Mozilla Includes
#include <nsCOMPtr.h>
#include <nsIPropertyBag.h>
#include <nsStringAPI.h>

class nsIWritablePropertyBag2;

#define SONGBIRD_TRANSCODINGCONFIGURATOR_CONTRACTID        \
  "@songbirdnest.com/Songbird/Mediacore/TranscodingConfigurator;1"
#define SONGBIRD_TRANSCODINGCONFIGURATOR_CLASSNAME         \
  "Songbird Transcoding Configurator Interface"
#define SONGBIRD_TRANSCODINGCONFIGURATOR_CID               \
  { /* f40ea6ad-4a39-464f-9e94-9f240e0a05ee */             \
    0xf40ea6ad,                                            \
    0x4a39,                                                \
    0x464f,                                                \
    { 0x9e, 0x94, 0x9f, 0x24, 0x0e, 0x0a, 0x05, 0xee }     \
  }

/*
 */
class sbTranscodingConfigurator : public sbITranscodingConfigurator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBITRANSCODINGCONFIGURATOR

  sbTranscodingConfigurator();

protected:
  virtual ~sbTranscodingConfigurator();

protected:
  // Have we configurated? Some functions will throw NS_ERROR_ALREADY_INITIALIZED
  // based on this. This will be set to true when configurate has been called.
  PRBool                              isConfigurated;
  // Store the input format we will use to configurate.
  nsCOMPtr<sbIMediaFormat>            mInputFormat;
  // String values of encoders and muxer
  nsString                            mMuxer;
  nsString                            mVideoEncoder;
  nsString                            mAudioEncoder;
  // The basic video format for data that is not specific to a particular codec.
  nsCOMPtr<sbIMediaFormatVideo>       mVideoFormat;
  // The basic audio format for data that is not specific to a particular codec.
  nsCOMPtr<sbIMediaFormatAudio>       mAudioFormat;
  // The video properties to set to encode a file as defined by configurate.
  nsCOMPtr<nsIWritablePropertyBag2>   mVideoEncoderProperties;
  // The audio properties to set to encode a file as defined by configurate.
  nsCOMPtr<nsIWritablePropertyBag2>   mAudioEncoderProperties;
};

#endif /*__SB_TRANSCODINGCONFIGURATOR_H__*/
