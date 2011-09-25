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

// local includes
#include "sbMediaInspector.h"

// Mozilla includes
#include <nsComponentManagerUtils.h>
#include <nsISimpleEnumerator.h>
#include <nsIWritablePropertyBag2.h>

#include <sbArrayUtils.h>
#include <sbMemoryUtils.h>

#define SB_PROPERTYBAG_CONTRACTID "@getnightingale.com/moz/xpcom/sbpropertybag;1"

/* Implementation file */
NS_IMPL_THREADSAFE_ISUPPORTS2(sbMediaFormatContainer,
                              sbIMediaFormatContainer,
                              sbIMediaFormatContainerMutable)

sbMediaFormatContainer::sbMediaFormatContainer(nsAString const & aContainerType) :
  mContainerType(aContainerType)
{
  mProperties = do_CreateInstance(SB_PROPERTYBAG_CONTRACTID);
}

sbMediaFormatContainer::~sbMediaFormatContainer()
{
}

/* readonly attribute AString containerType; */
NS_IMETHODIMP
sbMediaFormatContainer::GetContainerType(nsAString & aContainerType)
{
  aContainerType = mContainerType;
  return NS_OK;
}

/* readonly attribute nsIPropertyBag properties; */
NS_IMETHODIMP
sbMediaFormatContainer::GetProperties(nsIPropertyBag * *aProperties)
{
  NS_ENSURE_STATE(mProperties);

  return sbReturnCOMPtr(mProperties, aProperties);
}

/* void setContainerType (in AString aContainerType); */
NS_IMETHODIMP
sbMediaFormatContainer::SetContainerType(const nsAString & aContainerType)
{
  mContainerType = aContainerType;
  return NS_OK;
}

/* void setProperties (in nsIPropertyBag aProperties); */
NS_IMETHODIMP
sbMediaFormatContainer::SetProperties(nsIPropertyBag *aProperties)
{
  // MSVC can't figure out the templates, so we have to tell the compiler
  // explicitly what types to use
  nsresult rv = CallQueryInterface<nsIPropertyBag, nsIWritablePropertyBag>
                                  (aProperties, getter_AddRefs(mProperties));
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS2(sbMediaFormatVideo,
                              sbIMediaFormatVideo,
                              sbIMediaFormatVideoMutable)

sbMediaFormatVideo::sbMediaFormatVideo() :
    mVideoWidth(0),
    mVideoHeight(0),
    mBitRate(0)
{
  mProperties = do_CreateInstance(SB_PROPERTYBAG_CONTRACTID);
}

sbMediaFormatVideo::~sbMediaFormatVideo()
{
  /* destructor code */
}

/* readonly attribute AString videoType; */
NS_IMETHODIMP
sbMediaFormatVideo::GetVideoType(nsAString & aVideoType)
{
  aVideoType = mVideoType;

  return NS_OK;
}

/* readonly attribute long videoWidth; */
NS_IMETHODIMP
sbMediaFormatVideo::GetVideoWidth(PRInt32 *aVideoWidth)
{
  NS_ENSURE_ARG_POINTER(aVideoWidth);

  *aVideoWidth = mVideoWidth;

  return NS_OK;
}

/* readonly attribute long videoHeight; */
NS_IMETHODIMP
sbMediaFormatVideo::GetVideoHeight(PRInt32 *aVideoHeight)
{
  NS_ENSURE_ARG_POINTER(aVideoHeight);

  *aVideoHeight = mVideoHeight;

  return NS_OK;
}

/* void getVideoPAR (out unsigned long aNumerator, out unsigned long aDenominator); */
NS_IMETHODIMP
sbMediaFormatVideo::GetVideoPAR(PRUint32 *aNumerator, PRUint32 *aDenominator)
{
  NS_ENSURE_ARG_POINTER(aNumerator);
  NS_ENSURE_ARG_POINTER(aDenominator);

  *aNumerator = mVideoPAR.Numerator();
  *aDenominator = mVideoPAR.Denominator();

  return NS_OK;
}

NS_IMETHODIMP
sbMediaFormatVideo::GetVideoFrameRate(PRUint32 *aNumerator, PRUint32 *aDenominator)
{
  NS_ENSURE_ARG_POINTER(aNumerator);
  NS_ENSURE_ARG_POINTER(aDenominator);

  *aNumerator = mVideoFrameRate.Numerator();
  *aDenominator = mVideoFrameRate.Denominator();

  return NS_OK;
}

NS_IMETHODIMP
sbMediaFormatVideo::GetBitRate(PRInt32 *aBitRate)
{
  NS_ENSURE_ARG_POINTER(aBitRate);

  *aBitRate = mBitRate;

  return NS_OK;
}

/* readonly attribute nsIPropertyBag properties; */
NS_IMETHODIMP
sbMediaFormatVideo::GetProperties(nsIPropertyBag * *aProperties)
{
  NS_ENSURE_STATE(mProperties);

  return sbReturnCOMPtr(mProperties, aProperties);
}

/* void setVideoType (in AString aVideoType); */
NS_IMETHODIMP
sbMediaFormatVideo::SetVideoType(const nsAString & aVideoType)
{
  mVideoType = aVideoType;
  return NS_OK;
}

/* void setVideoWidth (in long aVideoWidth); */
NS_IMETHODIMP
sbMediaFormatVideo::SetVideoWidth(PRInt32 aVideoWidth)
{
  mVideoWidth = aVideoWidth;
  return NS_OK;
}

/* void setVideoHeight (in long aVideoHeight); */
NS_IMETHODIMP
sbMediaFormatVideo::SetVideoHeight(PRInt32 aVideoHeight)
{
  mVideoHeight = aVideoHeight;
  return NS_OK;
}

/* void setVideoPAR (in unsigned long aNumerator, in unsigned long aDenominator); */
NS_IMETHODIMP
sbMediaFormatVideo::SetVideoPAR(PRUint32 aNumerator, PRUint32 aDenominator)
{
  mVideoPAR = sbFraction(aNumerator, aDenominator);
  return NS_OK;
}

/* void setVideoFrameRate (in unsigned long aNumerator, in unsigned long aDenominator); */
NS_IMETHODIMP
sbMediaFormatVideo::SetVideoFrameRate(PRUint32 aNumerator, PRUint32 aDenominator)
{
  mVideoFrameRate = sbFraction(aNumerator, aDenominator);
  return NS_OK;
}

/* void setBitRate (in long aBitRate); */
NS_IMETHODIMP
sbMediaFormatVideo::SetBitRate(PRInt32 aBitRate)
{
  mBitRate = aBitRate;
  return NS_OK;
}

/* void setProperties (in nsIPropertyBag aProperties); */
NS_IMETHODIMP
sbMediaFormatVideo::SetProperties(nsIPropertyBag *aProperties)
{
  // MSVC can't figure out the templates, so we have to tell the compiler
  // explicitly what types to use
  nsresult rv = CallQueryInterface<nsIPropertyBag, nsIWritablePropertyBag>
                                  (aProperties, getter_AddRefs(mProperties));
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS2(sbMediaFormatAudio,
                              sbIMediaFormatAudio,
                              sbIMediaFormatAudioMutable)

sbMediaFormatAudio::sbMediaFormatAudio() :
    mBitRate(0),
    mSampleRate(0),
    mChannels(0)
{
  mProperties = do_CreateInstance(SB_PROPERTYBAG_CONTRACTID);
}

sbMediaFormatAudio::~sbMediaFormatAudio()
{
  /* destructor code */
}

/* readonly attribute string audioType; */
NS_IMETHODIMP
sbMediaFormatAudio::GetAudioType(nsAString & aAudioType)
{
  aAudioType = mAudioType;

  return NS_OK;
}

/* readonly attribute long bitrate; */
NS_IMETHODIMP
sbMediaFormatAudio::GetBitRate(PRInt32 *aBitRate)
{
  NS_ENSURE_ARG_POINTER(aBitRate);

  *aBitRate = mBitRate;

  return NS_OK;
}

/* readonly attribute long sampleRate; */
NS_IMETHODIMP
sbMediaFormatAudio::GetSampleRate(PRInt32 *aSampleRate)
{
  NS_ENSURE_ARG_POINTER(aSampleRate);

  *aSampleRate = mSampleRate;

  return NS_OK;
}

/* readonly attribute long channels; */
NS_IMETHODIMP
sbMediaFormatAudio::GetChannels(PRInt32 *aChannels)
{
  NS_ENSURE_ARG_POINTER(aChannels);

  *aChannels = mChannels;

  return NS_OK;
}

/* readonly attribute nsIPropertyBag properties; */
NS_IMETHODIMP
sbMediaFormatAudio::GetProperties(nsIPropertyBag * *aProperties)
{
  NS_ENSURE_ARG_POINTER(aProperties);

  return sbReturnCOMPtr(mProperties, aProperties);
}

/* void setAudioType (in AString aAudioType); */
NS_IMETHODIMP
sbMediaFormatAudio::SetAudioType(const nsAString & aAudioType)
{
  mAudioType = aAudioType;
  return NS_OK;
}

/* void setBitRate (in long aBitRate); */
NS_IMETHODIMP
sbMediaFormatAudio::SetBitRate(PRInt32 aBitRate)
{
  mBitRate = aBitRate;
  return NS_OK;
}

/* void setSampleRate (in long aSampleRate); */
NS_IMETHODIMP
sbMediaFormatAudio::SetSampleRate(PRInt32 aSampleRate)
{
  mSampleRate = aSampleRate;
  return NS_OK;
}

/* void setChannels (in long aChannels); */
NS_IMETHODIMP
sbMediaFormatAudio::SetChannels(PRInt32 aChannels)
{
  mChannels = aChannels;
  return NS_OK;
}

/* void setProperties (in nsIPropertyBag aProperties); */
NS_IMETHODIMP sbMediaFormatAudio::SetProperties(nsIPropertyBag *aProperties)
{
  // MSVC can't figure out the templates, so we have to tell the compiler
  // explicitly what types to use
  nsresult rv = CallQueryInterface<nsIPropertyBag, nsIWritablePropertyBag>
                                  (aProperties, getter_AddRefs(mProperties));
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS2(sbMediaFormat,
                              sbIMediaFormat,
                              sbIMediaFormatMutable)

sbMediaFormat::sbMediaFormat(sbIMediaFormatContainer * aContainer,
                             sbIMediaFormatVideo * aVideoStream) :
                               mContainer(aContainer),
                               mVideoStream(aVideoStream)
{

}

sbMediaFormat::~sbMediaFormat()
{
  /* destructor code */
}

/* readonly attribute sbIMediaFormatContainer container; */
NS_IMETHODIMP
sbMediaFormat::GetContainer(sbIMediaFormatContainer * *aContainer)
{
  NS_ENSURE_STATE(mContainer);

  return sbReturnCOMPtr(mContainer, aContainer);
}

/* readonly attribute sbIMediaFormatVideo videoStream; */
NS_IMETHODIMP
sbMediaFormat::GetVideoStream(sbIMediaFormatVideo * *aVideoStream)
{
  return sbReturnCOMPtr(mVideoStream, aVideoStream);
}

/* readonly attribute nsIArray audioStream; */
NS_IMETHODIMP
sbMediaFormat::GetAudioStream(sbIMediaFormatAudio * *aAudioStream)
{
  return sbReturnCOMPtr(mAudioStream, aAudioStream);
}

/* void setContainer (in sbIMediaFormatContainer aContainer); */
NS_IMETHODIMP
sbMediaFormat::SetContainer(sbIMediaFormatContainer *aContainer)
{
  mContainer = aContainer;
  return NS_OK;
}

/* void setVideoStream (in sbIMediaFormatVideo aFormat); */
NS_IMETHODIMP
sbMediaFormat::SetVideoStream(sbIMediaFormatVideo *aFormat)
{
  mVideoStream = aFormat;
  return NS_OK;
}

/* void setAudioStream (in sbIMediaFormatAudio aFormat); */
NS_IMETHODIMP
sbMediaFormat::SetAudioStream(sbIMediaFormatAudio *aFormat)
{
  mAudioStream = aFormat;
  return NS_OK;
}

