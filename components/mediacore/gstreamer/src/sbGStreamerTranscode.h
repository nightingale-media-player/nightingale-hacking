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

#ifndef _SB_GSTREAMER_TRANSCODE_H_
#define _SB_GSTREAMER_TRANSCODE_H_

#include <nsCOMPtr.h>
#include <nsCOMArray.h>
#include <nsITimer.h>

#include <gst/gst.h>

#include "sbIGStreamerTranscode.h"
#include "sbIJobProgress.h"
#include "sbIJobCancelable.h"

#include "sbGStreamerPipeline.h"

class sbGStreamerTranscode : public sbGStreamerPipeline,
                             public sbIGStreamerTranscode,
                             public sbIJobProgress,
                             public sbIJobCancelable,
                             public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBIGSTREAMERTRANSCODE
  NS_DECL_SBIJOBPROGRESS
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

  nsCString BuildPipelineString();

  nsCOMPtr<sbIPropertyArray>             mMetadata;

  nsString                               mSourceURI;
  nsString                               mDestURI;
  nsString                               mPipelineDescription;

  PRUint16                               mStatus;

  nsCOMArray<sbIJobProgressListener>     mProgressListeners;
  nsCOMPtr<nsITimer>                     mProgressTimer;

protected:
  /* additional members */
};

#endif // _SB_GSTREAMER_TRANSCODE_H_
