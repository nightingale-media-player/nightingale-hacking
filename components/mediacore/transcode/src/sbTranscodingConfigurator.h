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
  enum CONFIGURATE_STATE {
    /* The various states of configuration this configurator can be in; this
     * order must be preserved in chronological order so we can compare them
     * numerically.
     */
    CONFIGURATE_NOT_STARTED, /* determineOutputType has not finished */
    CONFIGURATE_OUTPUT_SET, /* determineOutputType finished, not configurated */
    CONFIGURATE_FINISHED /* configurate has been successfully called */
  };
  // Have we configurated?  Some of the properties below will not be ready
  // until we have determined the output type or configurated.
  CONFIGURATE_STATE                   mConfigurateState;
  // The input URI that is being transcoded
  nsCOMPtr<nsIURI>                    mInputUri;
  // The last transcode error seen
  nsCOMPtr<sbITranscodeError>         mLastError;
  // Store the input format we will use to configurate.
  nsCOMPtr<sbIMediaFormat>            mInputFormat;
  // String values of encoders and muxer
  nsString                            mMuxer;
  nsString                            mVideoEncoder;
  nsString                            mAudioEncoder;
  // The file extension to use
  nsCString                           mFileExtension;
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
