/* vim: set sw=2 */
/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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

#ifndef _SB_GSTREAMER_TRANSCODE_AUDIO_CONFIGURATOR_H_
#define _SB_GSTREAMER_TRANSCODE_AUDIO_CONFIGURATOR_H_

// {b61e78ec-9aa1-4505-b16c-e33819bca705}
#define SB_GSTREAMER_TRANSCODE_AUDIO_CONFIGURATOR_CID \
    { 0xb61e78ec, 0x9aa1, 0x4505, \
    { 0xb12, 0x6c, 0xe3, 0x38, 0x19, 0xbc, 0xa7, 0x05 } }

#define SB_GSTREAMER_TRANSCODE_AUDIO_CONFIGURATOR_CONTRACTID \
    "@getnightingale.com/Nightingale/Mediacore/Transcode/Configurator/Audio/GStreamer;1"
#define SB_GSTREAMER_TRANSCODE_AUDIO_CONFIGURATOR_CLASSNAME  \
    "GStreamerTranscodeAudioConfigurator"

#include <sbTranscodingConfigurator.h>

#include <sbITranscodingConfigurator.h>
#include <sbITranscodeProfile.h>
#include <sbPIGstTranscodingConfigurator.h>

#include <nsCOMPtr.h>
#include <nsDataHashtable.h>

class nsIArray;
class nsIWritablePropertyBag;

class sbIDevice;
class sbITranscodeEncoderProfile;
class sbIAudioFormatType;

class sbGStreamerTranscodeAudioConfigurator :
        public sbTranscodingConfigurator,
        public sbIDeviceTranscodingConfigurator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICETRANSCODINGCONFIGURATOR

  // forward most of sbITranscodingConfigurator to the base class
  #define FORWARD_TO_BASE(METHOD, PROTO, ARGS) \
    NS_IMETHOD METHOD PROTO { return sbTranscodingConfigurator::METHOD(ARGS); }

  FORWARD_TO_BASE(GetInputUri, (nsIURI ** aUri), aUri)
  FORWARD_TO_BASE(SetInputUri, (nsIURI * aUri), aUri)
  FORWARD_TO_BASE(GetInputFormat, (sbIMediaFormat * *aInputFormat), aInputFormat)
  FORWARD_TO_BASE(SetInputFormat, (sbIMediaFormat * aInputFormat), aInputFormat)
  FORWARD_TO_BASE(GetLastError, (sbITranscodeError * *aLastError), aLastError)
  FORWARD_TO_BASE(GetMuxer, (nsAString & aMuxer), aMuxer)
  FORWARD_TO_BASE(GetUseMuxer, (PRBool *aUseMuxer), aUseMuxer)
  FORWARD_TO_BASE(GetFileExtension, (nsACString & aFileExtension), aFileExtension)
  FORWARD_TO_BASE(GetVideoEncoder, (nsAString & aVideoEncoder), aVideoEncoder)
  FORWARD_TO_BASE(GetUseVideoEncoder, (PRBool *aUseVideoEncoder), aUseVideoEncoder)
  FORWARD_TO_BASE(GetVideoFormat, (sbIMediaFormatVideo * *aVideoFormat), aVideoFormat)
  FORWARD_TO_BASE(GetAudioEncoder, (nsAString & aAudioEncoder), aAudioEncoder)
  FORWARD_TO_BASE(GetUseAudioEncoder, (PRBool *aUseAudioEncoder), aUseAudioEncoder)
  FORWARD_TO_BASE(GetAudioFormat, (sbIMediaFormatAudio * *aAudioFormat), aAudioFormat)
  FORWARD_TO_BASE(GetVideoEncoderProperties, (nsIPropertyBag * *aVideoEncoderProperties), aVideoEncoderProperties)
  FORWARD_TO_BASE(GetAudioEncoderProperties, (nsIPropertyBag * *aAudioEncoderProperties), aAudioEncoderProperties)
  
  #undef FORWARD_TO_BASE

  /*
   * re-implement determineOutputType() and configurate() to have the
   * configuration logic specific to gstreamer-based configuration for devices
   */
  NS_IMETHOD DetermineOutputType();
  NS_IMETHOD Configurate();

  NS_IMETHOD GetAvailableProfiles(nsIArray **aProfiles);

  sbGStreamerTranscodeAudioConfigurator();

protected:
  virtual ~sbGStreamerTranscodeAudioConfigurator();

  /**
   * Make sure the given transcode profile does not use any gstreamer elements
   * that we do not have access to.
   *
   * @param aProfile the profile to test
   *
   * @throw NS_ERROR_NOT_AVAILABLE if the profile uses elements that are not
   * found
   */
  nsresult EnsureProfileAvailable(sbITranscodeProfile *aProfile);

  nsresult CheckProfileSupportedByDevice(sbITranscodeProfile *aProfile,
                                         sbIAudioFormatType **aFormat);

  /**
   * Select the encoding profile to use
   *
   * @precondition the device has been set
   * @precondition the quality has been set
   * @postcondition mSelectedProfile is the encoder profile to use
   * @postcondition mSelectedFormat is ths device format matching the profile
   * @postcondition mMuxer, mVideoEncoder, mAudioEncoder are the required
   *                element names
   */
  nsresult SelectProfile();

  /* Select the audio format (sample rate, channels, etc) to encode to */
  nsresult SelectOutputAudioFormat();

  /**
   * Set audio-related properties
   *
   * @precondition a profile has been selected
   * @postcondition mAudioEncoderProperties contains the audio properties desired
   */
  nsresult SetAudioProperties();

  /**
   * Copy properties, either audio or video.
   * @param aSrcProps the properties to copy from
   * @param aDstBag the property bag to output
   * @param aIsVideo true if this is for video, false for audio
   */
  nsresult CopyPropertiesIntoBag(nsIArray * aSrcProps,
                                 nsIWritablePropertyBag * aDstBag,
                                 PRBool aIsVideo);

  /* For each property in the property array, get a value from the device
   * preferences and apply it to the property.
   */
  nsresult ApplyPreferencesToPropertyArray(sbIDevice *aDevice,
                                           nsIArray  *aPropertyArray,
                                           nsString aPrefNameBase);

protected:
  /**
   * The mapping of profile to gstreamer element names (muxer + a/v encoder)
   */
  struct EncoderProfileData {
    nsCString muxer;
    nsCString audioEncoder;
    nsCString videoEncoder;
  };
  nsDataHashtable<nsISupportsHashKey, EncoderProfileData> mElementNames;
  
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
  nsCOMPtr<sbITranscodeProfile> mSelectedProfile;
  /**
   * The device audio format that corresponds with mSelectedProfile;
   * set by SelectProfile()
   */
  nsCOMPtr<sbIAudioFormatType> mSelectedFormat;

  /** True if we selected our profile based on something set in the device
   *  preferences; in such a case we should also look for property values from
   *  preferences.
   */
  PRBool mProfileFromPrefs;

  /** As above, but for whether we chose the profile from a global preference
   *  (not per-device).
   */
  PRBool mProfileFromGlobalPrefs;
};

#endif // _SB_GSTREAMER_TRANSCODE_AUDIO_CONFIGURATOR_H_
