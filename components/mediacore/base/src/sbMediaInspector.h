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

#ifndef SBMEDIACOREFORMATINSPECTOR_H_
#define SBMEDIACOREFORMATINSPECTOR_H_

// Mozilla includes
#include <nsCOMArray.h>
#include <nsCOMPtr.h>
#include <nsIVariant.h>
#include <nsIWritablePropertyBag.h>
#include <nsStringAPI.h>

// Songbird includes
#include <sbFraction.h>
#include <sbIMediaInspector.h>

#include <nsIWritablePropertyBag.h>

class sbMediaFormatContainer : public sbIMediaFormatContainer
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIAFORMATCONTAINER

  sbMediaFormatContainer(nsAString const & aContainerType = nsString());

  void SetContainerType(nsAString const & aContainerType)
  {
    mContainerType = aContainerType;
  }

  nsresult AddProperty(nsAString const & aKey, nsIVariant * aValue)
  {
    return mProperties->SetProperty(aKey, aValue);
  }
private:
  ~sbMediaFormatContainer();

  nsString mContainerType;
  nsCOMPtr<nsIWritablePropertyBag> mProperties;
};

class sbMediaFormatVideo : public sbIMediaFormatVideo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIAFORMATVIDEO

  sbMediaFormatVideo();

  void SetVideoType(nsAString const & aVideoType)
  {
    mVideoType = aVideoType;
  }

  void SetVideoDimensions(PRInt32 aWidth, PRInt32 aHeight)
  {
    mVideoWidth = aWidth;
    mVideoHeight = aHeight;
  }

  void SetVideoPAR(sbFraction const & aVideoPar)
  {
    mVideoPAR = aVideoPar;
  }

  /**
   * Sets the video frame rate for the
   */
  void SetVideoFrameRate(sbFraction const & aVideoFrameRate)
  {
    mVideoFrameRate = aVideoFrameRate;
  }

  /**
   * Sets the bit rate for the video format
   * \param aBitRate The bit rate to be set
   */
  void SetBitRate(PRInt32 const & aBitRate)
  {
    mBitRate = aBitRate;
  }

  /**
   * Adds a property to the list of properties. If the property exists, the
   * existing value is replaced by the new value.
   * \param aKey The key for the property
   * \param aVar The value of the property
   */
  nsresult SetProperty(nsAString const & aKey, nsIVariant * aVar)
  {
    nsresult rv;

    rv = mProperties->SetProperty(aKey, aVar);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }
private:
  ~sbMediaFormatVideo();

  nsString mVideoType;
  PRInt32 mVideoWidth;
  PRInt32 mVideoHeight;
  sbFraction mVideoPAR;
  sbFraction mVideoFrameRate;
  PRInt32 mBitRate;
  nsCOMPtr<nsIWritablePropertyBag> mProperties;
};

class sbMediaFormatAudio : public sbIMediaFormatAudio
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIAFORMATAUDIO

  sbMediaFormatAudio();

private:
  ~sbMediaFormatAudio();

  nsString mAudioType;
  PRInt32 mBitRate;
  PRInt32 mSampleRate;
  PRInt32 mChannels;
  nsCOMPtr<nsIWritablePropertyBag> mProperties;
};

class sbMediaFormat : public sbIMediaFormat
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIAFORMAT

  sbMediaFormat(sbIMediaFormatContainer * aContainer = nsnull,
                sbIMediaFormatVideo * aVideoStream = nsnull);

  /**
   * Sets the container for the media format
   */
  void SetContainer(sbIMediaFormatContainer * aContainer)
  {
    mContainer = aContainer;
  }

  /**
   * Sets the format for the video stream for the media format
   */
  void SetVideoStream(sbIMediaFormatVideo * aVideoStream)
  {
    mVideoStream = aVideoStream;
  }

  /**
   * Sets the format for the audio stream for the media format
   */
  void SetAudioStream(sbIMediaFormatAudio * aAudioStream)
  {
    mAudioStream = aAudioStream;
  }
private:
  ~sbMediaFormat();

  nsCOMPtr<sbIMediaFormatContainer> mContainer;
  nsCOMPtr<sbIMediaFormatVideo> mVideoStream;
  nsCOMPtr<sbIMediaFormatAudio> mAudioStream;
};

/**
 * This is the implementation of the \see sbIMediacoreMediaInspector service
 */
class sbMediaInspector : public sbIMediaInspector
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIAINSPECTOR

  sbMediaInspector();

private:
  ~sbMediaInspector();

protected:
  /* additional members */
};

#endif /* SBMEDIACOREFORMATINSPECTOR_H_ */
