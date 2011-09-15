/* vim: set sw=2 :miv */
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

#ifndef SBDEVICECAPSCOMPATIBILITY_H_
#define SBDEVICECAPSCOMPATIBILITY_H_

#include <nsCOMPtr.h>
#include <nsStringAPI.h>

#include <sbIDeviceCapsCompatibility.h>

class sbIDevCapVideoStream;
class sbIDevCapAudioStream;
class sbIMediaFormatVideo;
class sbIMediaFormatAudio;
class sbIDeviceCapabilities;

/**
 * This class compares the medie format from media inspector and device video
 * format from device capabilities.
 */
class sbDeviceCapsCompatibility : public sbIDeviceCapsCompatibility
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICECAPSCOMPATIBILITY

  sbDeviceCapsCompatibility();

private:
  /**
   * Cleanup
   */
  virtual ~sbDeviceCapsCompatibility();

  /**
   * To compare the audio format between media file and device capabilities.
   * \param aCompatible The flag on whether it's compatible.
   */
  nsresult CompareAudioFormat(PRBool* aCompatible);

  /**
   * To compare the video format between media file and device capabilities.
   * \param aCompatible The flag on whether it's compatible.
   */
  nsresult CompareVideoFormat(PRBool* aCompatible);

  /**
   * To compare the image format between media file and device capabilities.
   * \param aCompatible The flag on whether it's compatible.
   */
  nsresult CompareImageFormat(PRBool* aCompatible);

  /**
   * To compare the video stream between media file and device capabilities.
   * \param aVideoStream video stream of the device supported formats.
   * \param aCompatible The flag on whether it's compatible.
   */
  nsresult CompareVideoStream(sbIDevCapVideoStream* aVideoStream,
                              PRBool* aCompatible);

  /**
   * To compare the audio stream between media file and device capabilities.
   * \param aAudioStream audio stream of the device supported formats.
   * \param aCompatible The flag on whether it's compatible.
   */
  nsresult CompareAudioStream(sbIDevCapAudioStream* aAudioStream,
                              PRBool* aCompatible);

  /**
   * To compare the width and height of video stream between media file and
   * device capabilities.
   * \param aVideoStream video stream of the device supported formats.
   * \param aCompatible The flag on whether it's compatible.
   */
  nsresult CompareVideoWidthAndHeight(sbIDevCapVideoStream* aVideoStream,
                                      PRBool* aCompatible);

  /**
   * To compare the bit rate of video stream between media file and device
   * capabilities.
   * \param aVideoStream video stream of the device supported formats.
   * \param aCompatible The flag on whether it's compatible.
   */
  nsresult CompareVideoBitRate(sbIDevCapVideoStream* aVideoStream,
                               PRBool* aCompatible);

  /**
   * To compare the PAR (Pixel aspect ratio) of video stream between media
   * file and device capabilities.
   * \param aVideoStream video stream of the device supported formats.
   * \param aCompatible The flag on whether it's compatible.
   */
  nsresult CompareVideoPAR(sbIDevCapVideoStream* aVideoStream,
                           PRBool* aCompatible);

  /**
   * To compare the frame rate of video stream between media file and device
   * capabilities.
   * \param aVideoStream video stream of the device supported formats.
   * \param aCompatible The flag on whether it's compatible.
   */
  nsresult CompareVideoFrameRate(sbIDevCapVideoStream* aVideoStream,
                                 PRBool* aCompatible);

  /**
   * To compare the bit rate of audio stream between media file and device
   * capabilities.
   * \param aAudioStream audio stream of the device supported formats.
   * \param aCompatible The flag on whether it's compatible.
   */
  nsresult CompareAudioBitRate(sbIDevCapAudioStream* aAudioStream,
                               PRBool* aCompatible);

  /**
   * To compare the sample rate of audio stream between media file and device
   * capabilities.
   * \param aAudioStream audio stream of the device supported formats.
   * \param aCompatible The flag on whether it's compatible.
   */
  nsresult CompareAudioSampleRate(sbIDevCapAudioStream* aAudioStream,
                                  PRBool* aCompatible);

  /**
   * To compare the channels of audio stream between media file and device
   * capabilities.
   * \param aAudioStream audio stream of the device supported formats.
   * \param aCompatible The flag on whether it's compatible.
   */
  nsresult CompareAudioChannels(sbIDevCapAudioStream* aAudioStream,
                                PRBool* aCompatible);

  // Device capabilities
  nsCOMPtr<sbIDeviceCapabilities> mDeviceCapabilities;

  // Media format from media inspector
  nsCOMPtr<sbIMediaFormat> mMediaFormat;
  nsCOMPtr<sbIMediaFormatVideo> mMediaVideoStream;
  nsCOMPtr<sbIMediaFormatAudio> mMediaAudioStream;

  // container type
  nsString mMediaContainerType;

  // Video properties from video media stream
  nsString mMediaVideoType;
  PRInt32  mMediaVideoWidth;
  PRInt32  mMediaVideoHeight;
  PRInt32  mMediaVideoBitRate;
  PRInt32  mMediaVideoSampleRate;
  PRUint32 mMediaVideoPARNumerator;
  PRUint32 mMediaVideoPARDenominator;
  PRUint32 mMediaVideoFRNumerator;
  PRUint32 mMediaVideoFRDenominator;

  // Audio properties from audio media stream
  nsString mMediaAudioType;
  PRInt32  mMediaAudioBitRate;
  PRInt32  mMediaAudioSampleRate;
  PRInt32  mMediaAudioChannels;

  // Content type in the form of sbIDeviceCapabilities::CONTENT_* for the
  // media item to transfer
  PRUint32 mContentType;
};

#endif /* SBDEVICECAPSCOMPATIBILITY_H_ */

