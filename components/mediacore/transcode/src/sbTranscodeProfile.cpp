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
NS_IMPL_THREADSAFE_ISUPPORTS1(sbTranscodeProfile, sbITranscodeProfile)

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

NS_IMETHODIMP sbTranscodeProfile::GetId(nsAString & aId)
{
  aId = mId;
  return NS_OK;
}

NS_IMETHODIMP sbTranscodeProfile::SetId(nsAString const & aId)
{
  mId = aId;
  return NS_OK;
}

NS_IMETHODIMP sbTranscodeProfile::GetPriority(PRUint32 *aPriority)
{
  NS_ENSURE_ARG_POINTER(aPriority);
  *aPriority = mPriority;
  return NS_OK;
}

NS_IMETHODIMP sbTranscodeProfile::SetPriority(PRUint32 aPriority)
{
  mPriority = aPriority;
  return NS_OK;
}

NS_IMETHODIMP sbTranscodeProfile::GetDescription(nsAString & aDescription)
{
  aDescription = mDescription;
  return NS_OK;
}

NS_IMETHODIMP sbTranscodeProfile::SetDescription(nsAString const & aDescription)
{
  mDescription = aDescription;
  return NS_OK;
}

NS_IMETHODIMP sbTranscodeProfile::GetType(PRUint32 *aType)
{
  NS_ENSURE_ARG_POINTER(aType);

  *aType = mType;
  return NS_OK;
}

NS_IMETHODIMP sbTranscodeProfile::SetType(PRUint32 aType)
{
  mType = aType;
  return NS_OK;
}

NS_IMETHODIMP sbTranscodeProfile::GetContainerFormat(nsAString & aContainerFormat)
{
  aContainerFormat = mContainerFormat;
  return NS_OK;
}

NS_IMETHODIMP sbTranscodeProfile::SetContainerFormat(nsAString const & aContainerFormat)
{
  mContainerFormat = aContainerFormat;
  return NS_OK;
}

NS_IMETHODIMP sbTranscodeProfile::GetAudioCodec(nsAString & aAudioCodec)
{
  aAudioCodec = mAudioCodec;
  return NS_OK;
}

NS_IMETHODIMP sbTranscodeProfile::SetAudioCodec(nsAString const & aAudioCodec)
{
  mAudioCodec = aAudioCodec;
  return NS_OK;
}

NS_IMETHODIMP sbTranscodeProfile::GetVideoCodec(nsAString & aVideoCodec)
{
  aVideoCodec = mVideoCodec;
  return NS_OK;
}

NS_IMETHODIMP sbTranscodeProfile::SetVideoCodec(nsAString const & aVideoCodec)
{
  mVideoCodec = aVideoCodec;
  return NS_OK;
}

NS_IMETHODIMP sbTranscodeProfile::GetAudioProperties(nsIArray * *aAudioProperties)
{
  NS_ENSURE_ARG_POINTER(aAudioProperties);

  NS_IF_ADDREF(*aAudioProperties = mAudioProperties);
  return NS_OK;
}

NS_IMETHODIMP sbTranscodeProfile::SetAudioProperties(nsIArray * aAudioProperties)
{
  mAudioProperties = aAudioProperties;
  return NS_OK;
}

NS_IMETHODIMP sbTranscodeProfile::GetVideoProperties(nsIArray * *aVideoProperties)
{
  NS_ENSURE_ARG_POINTER(aVideoProperties);

  NS_IF_ADDREF(*aVideoProperties = mVideoProperties);
  return NS_OK;
}

NS_IMETHODIMP sbTranscodeProfile::SetVideoProperties(nsIArray * aVideoProperties)
{
  mVideoProperties = aVideoProperties;
  return NS_OK;
}

NS_IMETHODIMP sbTranscodeProfile::GetContainerProperties(nsIArray * *aContainerProperties)
{
  NS_ENSURE_ARG_POINTER(aContainerProperties);

  NS_IF_ADDREF(*aContainerProperties = mContainerProperties);
  return NS_OK;
}

NS_IMETHODIMP sbTranscodeProfile::SetContainerProperties(nsIArray * aContainerProperties)
{
  mContainerProperties = aContainerProperties;
  return NS_OK;
}
