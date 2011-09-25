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

#ifndef _SB_GSTREAMER_TRANSCODE_H_
#define _SB_GSTREAMER_TRANSCODE_H_

#include <nsCOMPtr.h>
#include <nsCOMArray.h>
#include <nsITimer.h>
#include <nsTArray.h>
#include <nsStringGlue.h>

#include <gst/gst.h>

#include "sbITranscodeJob.h"
#include "sbITranscodeProfile.h"
#include "sbIJobProgress.h"
#include "sbIJobCancelable.h"

#include "sbGStreamerPipeline.h"

// {67623837-fea5-4005-b475-e34b738635c4}
#define SB_GSTREAMER_TRANSCODE_CID \
	{ 0x67623837, 0xfea5, 0x4005, \
	{ 0xb4, 0x75, 0xe3, 0x4b, 0x73, 0x86, 0x35, 0xc4 } }

#define SB_GSTREAMER_TRANSCODE_CONTRACTID "@getnightingale.com/Nightingale/Mediacore/Transcode/GStreamer;1"
#define SB_GSTREAMER_TRANSCODE_CLASSNAME  "GStreamerTranscode"


class sbGStreamerTranscode : public sbGStreamerPipeline,
                             public sbITranscodeJob,
                             public sbIJobProgressTime,
                             public sbIJobCancelable,
                             public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBITRANSCODEJOB
  NS_DECL_SBIJOBPROGRESS
  NS_DECL_SBIJOBPROGRESSTIME
  NS_DECL_SBIJOBCANCELABLE
  NS_DECL_NSITIMERCALLBACK

  sbGStreamerTranscode();

  NS_IMETHOD BuildPipeline();
  NS_IMETHOD PlayPipeline();
  NS_IMETHOD StopPipeline();

private:
  virtual ~sbGStreamerTranscode();

  nsresult LocalizeString(const nsAString& aName, nsAString& aValue);
  nsresult OnJobProgress();

  void HandleErrorMessage(GstMessage *message);
  void HandleEOSMessage(GstMessage *message);

  GstClockTime QueryPosition();
  GstClockTime QueryDuration();

  nsresult StartProgressReporting();
  nsresult StopProgressReporting();

  GstElement *BuildTranscodePipeline(sbITranscodeProfile *aProfile);
  nsresult BuildPipelineString(nsCString description, nsACString &pipeline);
  nsresult BuildPipelineFragmentFromProfile(sbITranscodeProfile *aProfile,
          nsACString &pipelineFragment);
  nsresult GetContainer(nsAString &container, nsIArray *properties,
          nsACString &gstMuxer);
  nsresult GetAudioCodec(nsAString &codec, nsIArray *properties,
          nsACString &gstCodec);
  nsresult AddImageToTagList(GstTagList *aTags, nsIInputStream *aStream);

  nsCOMPtr<sbIPropertyArray>              mMetadata;
  nsCOMPtr<nsIInputStream>                mImageStream;

  nsString                                mSourceURI;
  nsString                                mDestURI;
  nsCOMPtr<sbITranscodeProfile>           mProfile;

  PRUint16                                mStatus;
  nsTArray<nsString>                      mErrorMessages;

  nsCOMArray<sbIJobProgressListener>      mProgressListeners;
  nsCOMPtr<nsITimer>                      mProgressTimer;

  nsCOMPtr<nsIArray>                      mAvailableProfiles; 

protected:
  /* additional members */
};

#endif // _SB_GSTREAMER_TRANSCODE_H_
