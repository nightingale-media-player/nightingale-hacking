/* vim: set sw=2 :miv */
/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

#ifndef SBTRANSCODEPROFILE_H_
#define SBTRANSCODEPROFILE_H_

#include <nsIArray.h>
#include <sbITranscodeProfile.h>

#include <nsCOMPtr.h>
#include <nsStringAPI.h>

#include <map>

/**
 * Basic implementation of a transcoding profile \see sbITranscodeProfile for
 * more information
 */
class sbTranscodeProfile : public sbITranscodeEncoderProfile
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBITRANSCODEPROFILE
  NS_DECL_SBITRANSCODEENCODERPROFILE

  sbTranscodeProfile();

private:
  ~sbTranscodeProfile();

public:
  /**
   * add a priority point
   */
  nsresult AddPriority(double aQuality, PRUint32 aPriority);
  /**
   * add an audio bitrate point
   */
  nsresult AddAudioBitrate(double aQuality, double aBitrate);
  /**
   * add a video bits-per-pixel point
   */
  nsresult AddVideoBPP(double aQuality, double aBPP);

private:
  nsString mId;
  PRUint32 mPriority;
  nsString mDescription;
  PRUint32 mType;
  nsString mContainerFormat;
  nsCString mFileExtension;
  nsString mAudioCodec;
  nsString mVideoCodec;
  nsCOMPtr<nsIArray> mContainerProperties;
  nsCOMPtr<nsIArray> mAudioProperties;
  nsCOMPtr<nsIArray> mVideoProperties;
  nsCOMPtr<nsIArray> mContainerAttributes;
  nsCOMPtr<nsIArray> mAudioAttributes;
  nsCOMPtr<nsIArray> mVideoAttributes;
  
  /* map for the priority */
  std::map<double, PRUint32> mPriorityMap;
  /* map for audio bitrates */
  std::map<double, double> mAudioBitrateMap;
  /* map for the video bpps */
  std::map<double, double> mVideoBPPMap;
};

#endif /* SBTRANSCODEPROFILE_H_ */
