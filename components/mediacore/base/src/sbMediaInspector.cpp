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

// local includes
#include "sbMediaInspector.h"

// Mozilla includes
#include <nsComponentManagerUtils.h>
#include <nsIWritablePropertyBag2.h>
#include <sbMemoryUtils.h>

#include <sbArrayUtils.h>

#define NS_HASH_PROPERTY_BAG_CONTRACTID "@mozilla.org/hash-property-bag;1"

/* Implementation file */
NS_IMPL_ISUPPORTS1(sbMediaFormatContainer, sbIMediaFormatContainer)

sbMediaFormatContainer::sbMediaFormatContainer(nsAString const & aContainerType) :
  mContainerType(aContainerType)
{
  mProperties = do_CreateInstance(NS_HASH_PROPERTY_BAG_CONTRACTID);
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

NS_IMPL_ISUPPORTS1(sbMediaFormatVideo, sbIMediaFormatVideo)

sbMediaFormatVideo::sbMediaFormatVideo()
{
  mProperties = do_CreateInstance(NS_HASH_PROPERTY_BAG_CONTRACTID);
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

NS_IMPL_ISUPPORTS1(sbMediaFormatAudio, sbIMediaFormatAudio)

sbMediaFormatAudio::sbMediaFormatAudio()
{
  mProperties = do_CreateInstance(NS_HASH_PROPERTY_BAG_CONTRACTID);
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

NS_IMPL_ISUPPORTS1(sbMediaFormat, sbIMediaFormat)

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
  nsresult rv;

  return sbReturnCOMPtr(mAudioStream, aAudioStream);
}

NS_IMPL_ISUPPORTS1(sbMediaInspector, sbIMediaInspector)

sbMediaInspector::sbMediaInspector()
{
}

sbMediaInspector::~sbMediaInspector()
{
}

/* sbIMediaFormat inspectMedia (in sbIMediaItem aMediaItem); */
NS_IMETHODIMP
sbMediaInspector::InspectMedia(sbIMediaItem *aMediaItem, sbIMediaFormat **retval)
{

  return NS_ERROR_NOT_AVAILABLE;
}

