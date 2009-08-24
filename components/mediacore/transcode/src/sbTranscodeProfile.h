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

#ifndef SBTRANSCODEPROFILE_H_
#define SBTRANSCODEPROFILE_H_

#include <nsCOMPtr.h>
#include <nsIArray.h>
#include <sbITranscodeProfile.h>
#include <nsStringAPI.h>

/**
 * Basic implementation of a transcoding profile \see sbITranscodeProfile for
 * more information
 */
class sbTranscodeProfile : public sbITranscodeProfile
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBITRANSCODEPROFILE

  sbTranscodeProfile();

private:
  ~sbTranscodeProfile();

private:
  nsString mId;
  PRUint32 mPriority;
  nsString mDescription;
  PRUint32 mType;
  nsString mContainerFormat;
  nsString mAudioCodec;
  nsString mVideoCodec;
  nsCOMPtr<nsIArray> mContainerProperties;
  nsCOMPtr<nsIArray> mAudioProperties;
  nsCOMPtr<nsIArray> mVideoProperties;
};

#endif /* SBTRANSCODEPROFILE_H_ */
