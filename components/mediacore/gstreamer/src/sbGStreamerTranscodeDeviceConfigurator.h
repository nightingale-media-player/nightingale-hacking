/* vim: set sw=2 */
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

#ifndef _SB_GSTREAMER_TRANSCODE_DEVICE_CONFIGURATOR_H_
#define _SB_GSTREAMER_TRANSCODE_DEVICE_CONFIGURATOR_H_

// {8D5D04F8-2060-4e82-A25B-C7D67A6B5292}
#define SB_GSTREAMER_TRANSCODE_DEVICE_CONFIGURATOR_CID \
    { 0x8d5d04f8, 0x2060, 0x4e82, \
    { 0xa2, 0x5b, 0xc7, 0xd6, 0x7a, 0x6b, 0x52, 0x92 } }

#define SB_GSTREAMER_TRANSCODE_DEVICE_CONFIGURATOR_CONTRACTID \
    "@songbirdnest.com/Songbird/Mediacore/Transcode/Configurator/Device/GStreamer;1"
#define SB_GSTREAMER_TRANSCODE_DEVICE_CONFIGURATOR_CLASSNAME  \
    "GStreamerTranscodeDeviceConfigurator"

#include <sbTranscodingConfigurator.h>

#include <sbITranscodingConfigurator.h>
#include <sbPIGstTranscodingConfigurator.h>

#include <nsCOMPtr.h>

#include <sbFraction.h>

class nsIArray;

class sbIDevice;
class sbITranscodeEncoderProfile;
class sbIVideoFormatType;

class sbGStreamerTranscodeDeviceConfigurator :
        public sbTranscodingConfigurator,
        public sbIDeviceTranscodingConfigurator,
        public sbPIGstTranscodingConfigurator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICETRANSCODINGCONFIGURATOR
  NS_DECL_SBPIGSTTRANSCODINGCONFIGURATOR

  // forward most of sbITranscodingConfigurator to the base class
  #define FORWARD_TO_BASE(METHOD, PROTO, ARGS) \
    NS_IMETHOD METHOD PROTO { return sbTranscodingConfigurator::METHOD(ARGS); }

  FORWARD_TO_BASE(GetInputFormat, (sbIMediaFormat * *aInputFormat), aInputFormat)
  FORWARD_TO_BASE(SetInputFormat, (sbIMediaFormat * aInputFormat), aInputFormat)
  FORWARD_TO_BASE(GetMuxer, (nsAString & aMuxer), aMuxer)
  FORWARD_TO_BASE(GetVideoEncoder, (nsAString & aVideoEncoder), aVideoEncoder)
  FORWARD_TO_BASE(GetVideoFormat, (sbIMediaFormatVideo * *aVideoFormat), aVideoFormat)
  FORWARD_TO_BASE(GetAudioEncoder, (nsAString & aAudioEncoder), aAudioEncoder)
  FORWARD_TO_BASE(GetAudioFormat, (sbIMediaFormatAudio * *aAudioFormat), aAudioFormat)
  FORWARD_TO_BASE(GetVideoEncoderProperties, (nsIPropertyBag * *aVideoEncoderProperties), aVideoEncoderProperties)
  FORWARD_TO_BASE(GetAudioEncoderProperties, (nsIPropertyBag * *aAudioEncoderProperties), aAudioEncoderProperties)
  
  #undef FORWARD_TO_BASE

  NS_IMETHOD Configurate();

  sbGStreamerTranscodeDeviceConfigurator();

protected:
  virtual ~sbGStreamerTranscodeDeviceConfigurator();

  /**
   * structure to contain a set of dimensions (width * height)
   */
  struct Dimensions {
    PRInt32 width;
    PRInt32 height;
    Dimensions() : width(PR_INT32_MIN), height(PR_INT32_MIN) {}
    Dimensions(PRInt32 w, PRInt32 h) : width(w), height(h) {}
  };
  
  /**
   * Make sure the given transcode profile does not use any gstreamer elements
   * that we do not have access to.
   *
   * @param aProfile the profile to test
   *
   * @throw NS_ERROR_NOT_AVAILABLE if the profile uses elements that are not
   * found
   */
  nsresult EnsureProfileAvailable(sbITranscodeEncoderProfile *aProfile);

  /**
   * Select the encoding profile to use
   *
   * @postcondition mSelectedProfile is the encoder profile to use
   * @postcondition mSelectedFormat is ths device format matching the profile
   */
  nsresult SelectProfile();

  /**
   * Set audio-related properties
   *
   * @precondition a profile has been selected
   * @postcondition mAudioEncoderProperties contains the audio properties desired
   */
  nsresult SetAudioProperties();

  /**
   * Determine the desired output size (ignoring bitrate constraints)
   *
   * @precondition the profile has been selected via SelectProfile()
   * @postcondition mPreferredDimensions is the desired output size
   */
  nsresult DetermineIdealOutputSize();

  /**
   * Given an input image dimensions (which provides the aspect ratio) and
   * maximum image dimensions, return a rectangle that is the largest possible
   * to fit in the second but with the aspect ratio of the first.
   * This for now will ignore all PARs.
   */
  static Dimensions GetMaximumFit(const Dimensions& aInput,
                                  const Dimensions& aMaximum);

  /**
   * Scale the video to something useful for the selected video bit rate
   * @precondition mPreferredDimensions has been set
   * @precondition mSelectedProfile has been selected
   * @precondition mSelectedFormat has been selected
   * @postcondition mVideoBitrate will be set (if it hasn't already been, or
   *                if the existing setting is too high for the device)
   * @postcondition mOutputDimensions will be the desired output dimensions
   */
  nsresult FinalizeOutputSize();

protected:
  /**
   * The quality setting to use
   */
  double mQuality;
  /**
   * The device to transcode to
   */
  nsCOMPtr<sbIDevice> mDevice;
  /**
   * A cache of the encoder profiles available
   * (should only be used via GetAvailableProfiles)
   */
  nsCOMPtr<nsIArray> mAvailableProfiles;
  /**
   * The encoder profile selected; set by SelectProfile()
   */
  nsCOMPtr<sbITranscodeEncoderProfile> mSelectedProfile;
  /**
   * The device video format that corresponds with mSelectedProfile;
   * set by SelectProfile()
   */
  nsCOMPtr<sbIVideoFormatType> mSelectedFormat;

  /**
   * The preferred output dimensions, ignoring bitrate constraints
   */
  Dimensions mPreferredDimensions;
  
  /**
   * The video bit rate we want to use
   */
  PRInt32 mVideoBitrate;
  
  /**
   * The video frame rate we want to use
   */
  sbFraction mVideoFrameRate;

  /**
   * The selected output dimensions, taking into account the bitrate constraints
   */
  Dimensions mOutputDimensions;
};

#endif // _SB_GSTREAMER_TRANSCODE_DEVICE_CONFIGURATOR_H_
