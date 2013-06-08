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

#include "sbGStreamerPipeline.h"

#include <sbIGStreamerService.h>

#include <nsIClassInfoImpl.h>
#include <sbClassInfoUtils.h>
#include <nsIInterfaceRequestorUtils.h>
#include <nsIRunnable.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>
#include <prlog.h>
#include <mozilla/Monitor.h>

#include <sbIMediacoreError.h>
#include <sbMediacoreError.h>
#include <sbProxiedComponentManager.h>

/**
 * To log this class, set the following environment variable in a debug build:
 *  NSPR_LOG_MODULES=sbGStreamerPipeline:5 (or :3 for LOG messages only)
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gGStreamerPipeline = PR_NewLogModule("sbGStreamerPipeline");
#define LOG(args) PR_LOG(gGStreamerPipeline, PR_LOG_WARNING, args)
#define TRACE(args) PR_LOG(gGStreamerPipeline, PR_LOG_DEBUG, args)
#else /* PR_LOGGING */
#define LOG(args)   /* nothing */
#define TRACE(args) /* nothing */
#endif /* PR_LOGGING */

// XXX CID for sbGStreamerPipeline?
//NS_IMPL_CLASSINFO(sbGStreamerPipeline, NULL, nsIClassInfo::THREADSAFE, sbGStreamerPipeline);
NS_IMPL_CLASSINFO(sbGStreamerPipeline, NULL, nsIClassInfo::THREADSAFE, SB_GSTREAMER_PIPELINE_CID);

NS_IMPL_ISUPPORTS2_CI(sbGStreamerPipeline, sbIMediacoreEventTarget, nsIClassInfo);

NS_IMPL_THREADSAFE_CI(sbGStreamerPipeline)

sbGStreamerPipeline::sbGStreamerPipeline() :
  mPipeline(NULL),
  mMonitor(NULL),
  mPipelineOp(GStreamer::OP_UNKNOWN)
{
  TRACE(("sbGStreamerPipeline[0x%.8x] - Constructed", this));

  mBaseEventTarget = new sbBaseMediacoreEventTarget(this);
}

sbGStreamerPipeline::~sbGStreamerPipeline()
{
  TRACE(("sbGStreamerPipeline[0x%.8x] - Destructed", this));

  DestroyPipeline();
}

nsresult
sbGStreamerPipeline::InitGStreamer()
{
  TRACE(("sbGStreamerPipeline[0x%.8x] - Initialise", this));

  nsresult rv;

  // We need to make sure the gstreamer service component has been loaded
  // since it calls gst_init for us.
  if (!NS_IsMainThread()) {
    nsCOMPtr<sbIGStreamerService> service = 
      do_ProxiedGetService(SBGSTREAMERSERVICE_CONTRACTID, &rv);
  }
  else {
    nsCOMPtr<sbIGStreamerService> service =
      do_GetService(SBGSTREAMERSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbGStreamerPipeline::BuildPipeline()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
sbGStreamerPipeline::SetupPipeline()
{
  TRACE(("sbGStreamerPipeline[0x%.8x] - SetupPipeline", this));
  nsresult rv;
  mozilla::MonitorAutoLock mon(mMonitor);

  rv = BuildPipeline();
  NS_ENSURE_SUCCESS (rv, rv);

  // BuildPipeline is required to have created the pipeline
  NS_ENSURE_STATE (mPipeline);

  GstBus *bus = gst_element_get_bus(mPipeline);

  // We want to receive state-changed messages when shutting down, so we
  // need to turn off bus auto-flushing
  g_object_set(mPipeline, "auto-flush-bus", FALSE, NULL);

  // Handle GStreamer messages synchronously, either directly or
  // dispatching to the main thread.
  gst_bus_set_sync_handler (bus, SyncToAsyncDispatcher,
          static_cast<sbGStreamerMessageHandler*>(this));

  gst_object_unref(bus);

  // Hold a reference to ourselves while we have a pipeline
  NS_ADDREF_THIS();

  return NS_OK;
}

nsresult
sbGStreamerPipeline::DestroyPipeline()
{
  TRACE(("sbGStreamerPipeline[0x%.8x] - DestroyPipeline", this));

  nsresult rv;
  GstElement *pipeline = NULL;

  {
    mozilla::MonitorAutoLock mon(mMonitor);
    if (mPipeline)
      pipeline = (GstElement *)gst_object_ref (mPipeline);
  }

  if (pipeline) {
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);
    TRACE(("sbGStreamerPipeline[0x%.8x] - DestroyPipeline NULL state.", this));
  }

  mozilla::MonitorAutoLock mon(mMonitor);
  if (mPipeline) {
    // Give subclass a chance to do something after the pipeline has been 
    // stopped, but before we unref it
    rv = OnDestroyPipeline(mPipeline);
    NS_ENSURE_SUCCESS (rv, rv);

    gst_object_unref(mPipeline);
    TRACE(("sbGStreamerPipeline[0x%.8x] - DestroyPipeline unref", this));
    mPipeline = NULL;

    // Drop our self-reference once we no longer have a pipeline
    NS_RELEASE_THIS();
  }

  return NS_OK;
}

PRBool sbGStreamerPipeline::HandleSynchronousMessage(GstMessage *aMessage)
{
  // Subclasses will probably override this to handle some messages.
  return PR_FALSE;
}

NS_IMETHODIMP
sbGStreamerPipeline::PlayPipeline()
{
  TRACE(("sbGStreamerPipeline[0x%.8x] - PlayPipeline", this));
  GstElement *pipeline = NULL;
  nsresult rv;

  {
    mozilla::MonitorAutoLock mon(mMonitor);
    if (!mPipeline) {
      rv = SetupPipeline();
      NS_ENSURE_SUCCESS (rv, rv);
    }
    pipeline = (GstElement *)gst_object_ref (mPipeline);
  }

  gst_element_set_state(pipeline, GST_STATE_PLAYING);
  gst_object_unref (pipeline);

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerPipeline::PausePipeline()
{
  TRACE(("sbGStreamerPipeline[0x%.8x] - PausePipeline", this));
  GstElement *pipeline = NULL;
  nsresult rv;

  {
    mozilla::MonitorAutoLock mon(mMonitor);
    if (!mPipeline) {
      rv = SetupPipeline();
      NS_ENSURE_SUCCESS (rv, rv);
    }
    pipeline = (GstElement *)gst_object_ref (mPipeline);
  }

  gst_element_set_state(pipeline, GST_STATE_PAUSED);
  gst_object_unref (pipeline);

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerPipeline::StopPipeline()
{
  TRACE(("sbGStreamerPipeline[0x%.8x] - StopPipeline", this));
  GstElement *pipeline = NULL;
  nsresult rv;

  {
    mozilla::MonitorAutoLock mon(mMonitor);
    if (mPipeline)
      pipeline = (GstElement *)gst_object_ref (mPipeline);
  }

  if (pipeline) {
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);

    rv = DestroyPipeline();
    NS_ENSURE_SUCCESS (rv, rv);
  }

  return NS_OK;
}

void sbGStreamerPipeline::SetPipelineOp(GStreamer::pipelineOp_t aPipelineOp)
{
  mozilla::MonitorAutoLock mon(mMonitor);
  mPipelineOp = aPipelineOp;
  return;
}

GStreamer::pipelineOp_t sbGStreamerPipeline::GetPipelineOp()
{
  mozilla::MonitorAutoLock mon(mMonitor);
  return mPipelineOp;
}

void sbGStreamerPipeline::HandleMessage (GstMessage *message)
{
  GstMessageType msg_type;
  msg_type = GST_MESSAGE_TYPE(message);

  switch (msg_type) {
    case GST_MESSAGE_ERROR:
      HandleErrorMessage(message);
      break;
    case GST_MESSAGE_WARNING:
      HandleWarningMessage(message);
      break;
    case GST_MESSAGE_STATE_CHANGED:
      HandleStateChangeMessage(message);
      break;
    case GST_MESSAGE_EOS:
      HandleEOSMessage(message);
      break;
    default:
      LOG(("Got message: %s", gst_message_type_get_name(msg_type)));
      break;
  }
}

void sbGStreamerPipeline::HandleErrorMessage(GstMessage *message)
{
  GError *gerror = NULL;
  gchar *debug = NULL;
  nsString errormessage;
  nsCOMPtr<sbIMediacoreError> error;
  nsresult rv;

  gst_message_parse_error(message, &gerror, &debug);

  LOG(("Error message: %s [%s]", GST_STR_NULL (gerror->message), 
              GST_STR_NULL (debug)));

  GStreamer::pipelineOp_t op = GetPipelineOp();
  rv = GetMediacoreErrorFromGstError(gerror, mResourceDisplayName,
          op, getter_AddRefs(error));
  NS_ENSURE_SUCCESS(rv, /* void */);

  DispatchMediacoreEvent(sbIMediacoreEvent::ERROR_EVENT, nsnull, error);

  g_error_free (gerror);
  g_free(debug);

  rv = StopPipeline();
  NS_ENSURE_SUCCESS (rv, /* void */);
}

void sbGStreamerPipeline::HandleWarningMessage(GstMessage *message)
{
  GError *error = NULL;
  gchar *debug = NULL;

  gst_message_parse_warning(message, &error, &debug);

  LOG(("Warning message: %s [%s]", GST_STR_NULL (error->message), 
              GST_STR_NULL (debug)));

  g_warning ("%s [%s]", GST_STR_NULL (error->message), GST_STR_NULL (debug));

  g_error_free (error);
  g_free (debug);
}

void sbGStreamerPipeline::HandleEOSMessage(GstMessage *message)
{
  TRACE(("sbGStreamerPipeline[0x%.8x] - HandleEOSMessage", this));

  nsresult rv = StopPipeline();
  NS_ENSURE_SUCCESS (rv, /* void */);
}

void sbGStreamerPipeline::HandleTagMessage(GstMessage *message)
{
}

void sbGStreamerPipeline::HandleBufferingMessage(GstMessage *message)
{
}

void sbGStreamerPipeline::HandleStateChangeMessage(GstMessage *message)
{
  // Only listen to state-changed messages from top-level pipelines
  if (GST_IS_PIPELINE (message->src))
  {
    GstState oldstate, newstate, pendingstate;
    gst_message_parse_state_changed (message,
            &oldstate, &newstate, &pendingstate);

    gchar *srcname = gst_object_get_name(message->src);
    LOG(("state-changed: %s changed state from %s to %s", srcname,
        gst_element_state_get_name (oldstate),
        gst_element_state_get_name (newstate)));
    g_free (srcname);

    if (oldstate == GST_STATE_PAUSED && newstate == GST_STATE_PLAYING)
    {
      /* Start our timer */
      mTimeStarted = PR_IntervalNow();
    }
    else if (oldstate == GST_STATE_PLAYING && newstate == GST_STATE_PAUSED)
    {
      mTimeRunning += GetRunningTime();
      mTimeStarted = (PRIntervalTime)-1;
    }

    // Dispatch START, PAUSE, STOP events
    if (pendingstate == GST_STATE_VOID_PENDING) {
      if (newstate == GST_STATE_PLAYING)
        DispatchMediacoreEvent (sbIMediacoreEvent::STREAM_START);
      else if (newstate == GST_STATE_PAUSED)
        DispatchMediacoreEvent (sbIMediacoreEvent::STREAM_PAUSE);
      else if (newstate == GST_STATE_NULL)
        DispatchMediacoreEvent (sbIMediacoreEvent::STREAM_STOP);
    }
  }
}

void sbGStreamerPipeline::DispatchMediacoreEvent (unsigned long type,
        nsIVariant *aData, sbIMediacoreError *aError)
{
  nsresult rv;
  nsCOMPtr<sbIMediacoreEvent> event;
  rv = sbMediacoreEvent::CreateEvent(type,
                                     aError,
                                     aData,
                                     NULL,
                                     getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, /* void */);

  rv = DispatchEvent(event, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, /* void */);
}

GstClockTime
sbGStreamerPipeline::GetRunningTime()
{
  PRIntervalTime now = PR_IntervalNow();
  PRIntervalTime interval;

  if (mTimeStarted == (PRIntervalTime)-1)
    return mTimeRunning;

  if (now < mTimeStarted) {
    // Wraparound occurred, deal with it.
    PRInt64 realnow = (PRInt64)now + ((PRInt64)1<<32);
    interval = (PRIntervalTime)(realnow - mTimeStarted);
  }
  else {
    interval = now - mTimeStarted;
  }

  return mTimeRunning + PR_IntervalToMilliseconds (interval) * GST_MSECOND;
}

// Forwarding functions for sbIMediacoreEventTarget interface

NS_IMETHODIMP
sbGStreamerPipeline::DispatchEvent(sbIMediacoreEvent *aEvent,
                                   PRBool aAsync,
                                   PRBool* _retval)
{
  return mBaseEventTarget ?
         mBaseEventTarget->DispatchEvent(aEvent, aAsync, _retval) :
         NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
sbGStreamerPipeline::AddListener(sbIMediacoreEventListener *aListener)
{
  return mBaseEventTarget ?
         mBaseEventTarget->AddListener(aListener) :
         NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
sbGStreamerPipeline::RemoveListener(sbIMediacoreEventListener *aListener)
{
  return mBaseEventTarget ?
         mBaseEventTarget->RemoveListener(aListener) :
         NS_ERROR_NULL_POINTER;
}

