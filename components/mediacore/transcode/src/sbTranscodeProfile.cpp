/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#include "sbTranscodeProfile.h"

/* Implementation file */
NS_IMPL_THREADSAFE_ISUPPORTS2(sbTranscodeProfile,
                              sbITranscodeProfile,
                              sbITranscodeEncoderProfile)

sbTranscodeProfile::sbTranscodeProfile() :
  mPriority(0),
  mType(sbITranscodeProfile::TRANSCODE_TYPE_UNKNOWN)
{
  /* member initializers and constructor code */
}

sbTranscodeProfile::~sbTranscodeProfile()
{
  /* destructor code */
}

NS_IMETHODIMP
sbTranscodeProfile::GetId(nsAString & aId)
{
  aId = mId;
  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeProfile::SetId(nsAString const & aId)
{
  mId = aId;
  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeProfile::GetPriority(PRUint32 *aPriority)
{
  NS_ENSURE_ARG_POINTER(aPriority);
  *aPriority = mPriority;
  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeProfile::SetPriority(PRUint32 aPriority)
{
  mPriority = aPriority;
  // set a default point for sbITranscodeEncoderProfile::getPriority
  mPriorityMap[0.5] = aPriority;
  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeProfile::GetDescription(nsAString & aDescription)
{
  aDescription = mDescription;
  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeProfile::SetDescription(nsAString const & aDescription)
{
  mDescription = aDescription;
  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeProfile::GetType(PRUint32 *aType)
{
  NS_ENSURE_ARG_POINTER(aType);

  *aType = mType;
  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeProfile::SetType(PRUint32 aType)
{
  mType = aType;
  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeProfile::GetContainerFormat(nsAString & aContainerFormat)
{
  aContainerFormat = mContainerFormat;
  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeProfile::SetContainerFormat(nsAString const & aContainerFormat)
{
  mContainerFormat = aContainerFormat;
  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeProfile::GetFileExtension(nsACString & aFileExtension)
{
  aFileExtension = mFileExtension;
  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeProfile::SetFileExtension(nsACString const & aFileExtension)
{
  mFileExtension = aFileExtension;
  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeProfile::GetAudioCodec(nsAString & aAudioCodec)
{
  aAudioCodec = mAudioCodec;
  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeProfile::SetAudioCodec(nsAString const & aAudioCodec)
{
  mAudioCodec = aAudioCodec;
  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeProfile::GetVideoCodec(nsAString & aVideoCodec)
{
  aVideoCodec = mVideoCodec;
  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeProfile::SetVideoCodec(nsAString const & aVideoCodec)
{
  mVideoCodec = aVideoCodec;
  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeProfile::GetAudioProperties(nsIArray * *aAudioProperties)
{
  NS_ENSURE_ARG_POINTER(aAudioProperties);

  NS_IF_ADDREF(*aAudioProperties = mAudioProperties);
  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeProfile::SetAudioProperties(nsIArray * aAudioProperties)
{
  mAudioProperties = aAudioProperties;
  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeProfile::GetVideoProperties(nsIArray * *aVideoProperties)
{
  NS_ENSURE_ARG_POINTER(aVideoProperties);

  NS_IF_ADDREF(*aVideoProperties = mVideoProperties);
  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeProfile::SetVideoProperties(nsIArray * aVideoProperties)
{
  mVideoProperties = aVideoProperties;
  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeProfile::GetContainerProperties(nsIArray * *aContainerProperties)
{
  NS_ENSURE_ARG_POINTER(aContainerProperties);

  NS_IF_ADDREF(*aContainerProperties = mContainerProperties);
  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeProfile::SetContainerProperties(nsIArray * aContainerProperties)
{
  mContainerProperties = aContainerProperties;
  return NS_OK;
}


/***** nsITranscodeEncoderProfile implementation *****/
template<typename T>
T getInterpolatedQuality(std::map<double, T> &aMap, double aQuality)
{
  typename std::map<double, T>::const_iterator end   = aMap.end(),
                                               lower,
                                               upper = aMap.upper_bound(aQuality);

  if (aMap.empty()) {
    // completely missing :(
    return 0;
  }

  if (upper == aMap.begin()) {
    // the first point is larger than desired; just return that
    return upper->second;
  }

  lower = upper;
  --lower;
  if (upper == end) {
    // nothing is greater than desired quality, select the highest we have
    return lower->second;
  }

  double scale = (aQuality - lower->first) / (upper->first - lower->first);
  T difference = (T)(scale * (upper->second - lower->second));
  return lower->second + difference;
}

/* PRInt32 getPriority (in double aQuality); */
NS_IMETHODIMP
sbTranscodeProfile::GetPriority(double aQuality, PRUint32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = getInterpolatedQuality(mPriorityMap, aQuality);
  return NS_OK;
}

/* double getAudioBitrate (in double aQuality); */
NS_IMETHODIMP
sbTranscodeProfile::GetAudioBitrate(double aQuality, double *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = getInterpolatedQuality(mAudioBitrateMap, aQuality);
  return NS_OK;
}

/* double getVideoBitsPerPixel (in double aQuality); */
NS_IMETHODIMP
sbTranscodeProfile::GetVideoBitsPerPixel(double aQuality, double *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = getInterpolatedQuality(mVideoBPPMap, aQuality);
  return NS_OK;
}

/***** nsITranscodeEncoderProfile helpers *****/
nsresult
sbTranscodeProfile::AddPriority(double aQuality, PRUint32 aPriority)
{
  NS_ENSURE_ARG_RANGE(aQuality, 0, 1);
  mPriorityMap[aQuality] = aPriority;
  return NS_OK;
}

nsresult 
sbTranscodeProfile::AddAudioBitrate(double aQuality, double aBitrate)
{
  NS_ENSURE_ARG_RANGE(aQuality, 0, 1);
  mAudioBitrateMap[aQuality] = aBitrate;
  return NS_OK;
}

nsresult 
sbTranscodeProfile::AddVideoBPP(double aQuality, double aBPP)
{
  NS_ENSURE_ARG_RANGE(aQuality, 0, 1);
  mVideoBPPMap[aQuality] = aBPP;
  return NS_OK;
}
