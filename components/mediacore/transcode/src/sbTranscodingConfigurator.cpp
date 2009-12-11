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


#include "sbTranscodingConfigurator.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(sbTranscodingConfigurator,
                              sbITranscodingConfigurator)

sbTranscodingConfigurator::sbTranscodingConfigurator()
  : isConfigurated(PR_FALSE),
    mInputFormat(nsnull),
    mMuxer(EmptyString()),
    mVideoEncoder(EmptyString()),
    mAudioEncoder(EmptyString()),
    mVideoFormat(nsnull),
    mAudioFormat(nsnull),
    mVideoEncoderProperties(nsnull),
    mAudioEncoderProperties(nsnull)
{
  // Nothing to do yet.
}

sbTranscodingConfigurator::~sbTranscodingConfigurator()
{
  // Nothing to do yet
}

/**
 * \brief Gets the input format that was set to use in the configurate function.
 * \param aInputFormat the sbIMediaFormat set previously or null if none set.
 */
NS_IMETHODIMP
sbTranscodingConfigurator::GetInputFormat(sbIMediaFormat **aInputFormat)
{
  NS_ENSURE_ARG_POINTER(aInputFormat);
  NS_IF_ADDREF(*aInputFormat = mInputFormat);
  return NS_OK;
}

/**
 * \brief Sets the input format that will be used in the configurate function.
 * \param aInputFormat the sbIMediaFormat to set for input.
 * \throws NS_ERROR_ALREADY_INITIALIZED if Configurate has already been called.
 */
NS_IMETHODIMP
sbTranscodingConfigurator::SetInputFormat(sbIMediaFormat *aInputFormat)
{
  NS_ENSURE_FALSE(!isConfigurated, NS_ERROR_ALREADY_INITIALIZED);
  NS_ENSURE_ARG(aInputFormat);
  mInputFormat = aInputFormat;
  return NS_OK;
}

/**
 * \brief Configures the settings to use in order to properly transcode the
 * media item.
 */
NS_IMETHODIMP
sbTranscodingConfigurator::Configurate(void)
{
  NS_NOTYETIMPLEMENTED("sbTranscodingConfigurator::Configurate");
  return NS_ERROR_NOT_IMPLEMENTED;
}

/**
 * \brief Returns the muxer to use.
 * \param aMuxer - Muxer to use in string form.
 * \returns string value for muxer.
 */
NS_IMETHODIMP
sbTranscodingConfigurator::GetMuxer(nsAString &aMuxer)
{
  NS_ENSURE_TRUE(isConfigurated, NS_ERROR_NOT_INITIALIZED);
  aMuxer = mMuxer;
  return NS_OK;
}

/**
 * \brief Get the video Encoder to use.
 * \param aVideoEncoder - name of video encoder.
 * \returns video encoder to use.
 */
NS_IMETHODIMP
sbTranscodingConfigurator::GetVideoEncoder(nsAString &aVideoEncoder)
{
  NS_ENSURE_TRUE(isConfigurated, NS_ERROR_NOT_INITIALIZED);
  aVideoEncoder = mVideoEncoder;
  return NS_OK;
}

/**
 * \brief Get the Video format to use.
 * \param aVideoFormat - sbIMediaFormatVideo video format to use.
 * \returns the video format to use.
 */
NS_IMETHODIMP
sbTranscodingConfigurator::GetVideoFormat(sbIMediaFormatVideo **aVideoFormat)
{
  NS_ENSURE_TRUE(isConfigurated, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aVideoFormat);
  NS_IF_ADDREF(*aVideoFormat = mVideoFormat);
  return NS_OK;
}

/**
 * \brief Get the audio encoder to use.
 * \param aAudioEncoder - name of the audio encoder.
 * \returns audio encoder to use.
 */
NS_IMETHODIMP
sbTranscodingConfigurator::GetAudioEncoder(nsAString &aAudioEncoder)
{
  NS_ENSURE_TRUE(isConfigurated, NS_ERROR_NOT_INITIALIZED);
  aAudioEncoder = mAudioEncoder;
  return NS_OK;
}

/**
 * \brief Get the Audio format to use.
 * \param aAudioFormat - sbIMediaFormatAudio audio format to use.
 * \returns the audio format to use.
 */
NS_IMETHODIMP
sbTranscodingConfigurator::GetAudioFormat(sbIMediaFormatAudio **aAudioFormat)
{
  NS_ENSURE_TRUE(isConfigurated, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aAudioFormat);
  NS_IF_ADDREF(*aAudioFormat = mAudioFormat);
  return NS_OK;
}

/**
 * \brief Get the video encoder properties.
 * \param aVideoEncoderProperties - video encoder properties in a nsIPropertyBag
 * \returns video encoder properties in a nsIPropertyBag.
 */
NS_IMETHODIMP
sbTranscodingConfigurator::GetVideoEncoderProperties(
        nsIPropertyBag **aVideoEncoderProperties)
{
  NS_ENSURE_TRUE(isConfigurated, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aVideoEncoderProperties);
  NS_IF_ADDREF(*aVideoEncoderProperties = mVideoEncoderProperties);
  return NS_OK;
}

/**
 * \brief Get the audio encoder properties.
 * \param aAudioEncoderProperties - audio encoder properties in a nsIPropertyBag
 * \returns audio encoder properties in a nsIPropertyBag.
 */
NS_IMETHODIMP
sbTranscodingConfigurator::GetAudioEncoderProperties(
        nsIPropertyBag **aAudioEncoderProperties)
{
  NS_ENSURE_TRUE(isConfigurated, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aAudioEncoderProperties);
  NS_IF_ADDREF(*aAudioEncoderProperties = mAudioEncoderProperties);
  return NS_OK;
}
