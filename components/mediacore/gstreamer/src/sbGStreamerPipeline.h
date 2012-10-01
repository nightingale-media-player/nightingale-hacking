/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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

#ifndef _SB_GSTREAMER_PIPELINE_H_
#define _SB_GSTREAMER_PIPELINE_H_

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsCOMArray.h>
#include <nsStringGlue.h>
#include <nsIDOMEventListener.h>
#include <nsIDOMWindow.h>
#include <nsITimer.h>
#include <nsComponentManagerUtils.h>
#include <nsIStringBundle.h>
#include <nsIClassInfo.h>

#include <sbBaseMediacoreEventTarget.h>

#include <sbIGStreamerPipeline.h>
#include <sbIMediacoreEventTarget.h>

#include "sbGStreamerMediacoreUtils.h"

#include <gst/gst.h>

class sbGStreamerPipeline :
    public sbGStreamerMessageHandler,
    public sbIGStreamerPipeline,
    public sbIMediacoreEventTarget,
    public nsIClassInfo

{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBIMEDIACOREEVENTTARGET
  NS_DECL_SBIGSTREAMERPIPELINE

  sbGStreamerPipeline();
  virtual ~sbGStreamerPipeline();

  virtual void HandleMessage(GstMessage *message);
  virtual bool HandleSynchronousMessage(GstMessage *message);

  nsresult InitGStreamer();

protected:
  virtual void HandleEOSMessage(GstMessage *message);
  virtual void HandleErrorMessage(GstMessage *message);
  virtual void HandleWarningMessage(GstMessage *message);
  virtual void HandleStateChangeMessage(GstMessage *message);
  virtual void HandleTagMessage(GstMessage *message);
  virtual void HandleBufferingMessage(GstMessage *message);

  virtual nsresult OnDestroyPipeline(GstElement *pipeline) { return NS_OK; }

  virtual nsresult BuildPipeline();
  virtual nsresult SetupPipeline();
  virtual nsresult DestroyPipeline();

  void SetPipelineOp(GStreamer::pipelineOp_t aPipelineOp);
  GStreamer::pipelineOp_t GetPipelineOp();

  void DispatchMediacoreEvent (unsigned long type,
          nsIVariant *aData = NULL, sbIMediacoreError *aError = NULL);

  // Get the amount of time the pipeline has been running for.
  GstClockTime GetRunningTime();

  // The pipeline we're managing
  GstElement* mPipeline;

  // A string describing the resource (filename, URI, etc) being processed
  // by the pipeline. Only used for informational purposes (error messages, etc)
  nsString mResourceDisplayName;

  // The total time the pipeline has been running (not including the time from
  // mTimeStarted to now, if mTimeStarted != -1)
  GstClockTime mTimeRunning;

  // The last time that the pipeline was started
  PRIntervalTime mTimeStarted;

  // Protect access to the pipeline
  PRMonitor *mMonitor;

  // Pipeline Primary Operation
  GStreamer::pipelineOp_t mPipelineOp;

  nsAutoPtr<sbBaseMediacoreEventTarget> mBaseEventTarget;
};

#endif // _SB_GSTREAMER_PIPELINE_H_

