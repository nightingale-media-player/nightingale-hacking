/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

#include "sbGStreamerMediacore.h"

#include <nsIFile.h>
#include <nsIFileURL.h>
#include <nsIInterfaceRequestorUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsILocalFile.h>
#include <nsIURI.h>
#include <nsIURL.h>
#include <nsIRunnable.h>
#include <nsThreadUtils.h>
#include <nsCOMPtr.h>
#include <prlog.h>

#include <sbClassInfoUtils.h>
#include <sbTArrayStringEnumerator.h>
#include <sbVariantUtils.h>
#include <sbBaseMediacoreEventTarget.h>
#include <sbMediacoreError.h>

#include <sbIGStreamerService.h>

#include "sbGStreamerMediacoreUtils.h"

#ifdef MOZ_WIDGET_GTK2
#include "sbGStreamerPlatformGDK.h"
#endif

#ifdef XP_WIN
#include "sbGStreamerPlatformWin32.h"
#endif

#ifdef CreateEvent
#undef CreateEvent
#endif

/**
 * To log this class, set the following environment variable in a debug build:
 *
 *  NSPR_LOG_MODULES=sbGStreamerMediacore:5 (or :3 for LOG messages only)
 *
 */
#ifdef PR_LOGGING

static PRLogModuleInfo* gGStreamerMediacore =
  PR_NewLogModule("sbGStreamerMediacore");

#define LOG(args)                                          \
  if (gGStreamerMediacore)                             \
    PR_LOG(gGStreamerMediacore, PR_LOG_WARNING, args)

#define TRACE(args)                                        \
  if (gGStreamerMediacore)                             \
    PR_LOG(gGStreamerMediacore, PR_LOG_DEBUG, args)

#else /* PR_LOGGING */

#define LOG(args)   /* nothing */
#define TRACE(args) /* nothing */

#endif /* PR_LOGGING */

NS_IMPL_ISUPPORTS_INHERITED7(sbGStreamerMediacore,
                             sbBaseMediacore,
                             sbIMediacore,
                             sbIMediacorePlaybackControl,
                             sbIMediacoreVolumeControl,
                             sbIMediacoreVotingParticipant,
                             sbIMediacoreEventTarget,
                             sbIGStreamerMediacore,
                             nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER6(sbGStreamerMediacore,
                             sbIMediacore,
                             sbIMediacorePlaybackControl,
                             sbIMediacoreVolumeControl,
                             sbIMediacoreVotingParticipant,
                             sbIGStreamerMediacore,
                             sbIMediacoreEventTarget)

NS_DECL_CLASSINFO(sbGStreamerMediacore)
NS_IMPL_THREADSAFE_CI(sbGStreamerMediacore)

sbGStreamerMediacore::sbGStreamerMediacore() :
    mMonitor(nsnull),
    mVideoEnabled(PR_FALSE),
    mPipeline(nsnull),
    mPlatformInterface(nsnull),
    mBaseEventTarget(new sbBaseMediacoreEventTarget(this)),
    mTags(NULL),
    mProperties(nsnull)
{
  NS_WARN_IF_FALSE(mBaseEventTarget, 
          "mBaseEventTarget is null, may be out of memory");

}

sbGStreamerMediacore::~sbGStreamerMediacore()
{
  if (mTags)
    gst_tag_list_free(mTags);

  if (mMonitor)
    nsAutoMonitor::DestroyMonitor(mMonitor);
}

nsresult
sbGStreamerMediacore::Init() 
{
  nsresult rv;

  mMonitor = nsAutoMonitor::NewMonitor("sbGStreamerMediacore::mMonitor");
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_OUT_OF_MEMORY);

  rv = sbBaseMediacore::InitBaseMediacore();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbBaseMediacorePlaybackControl::InitBaseMediacorePlaybackControl();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbBaseMediacoreVolumeControl::InitBaseMediacoreVolumeControl();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// Utility methods

class sbGstMessageEvent : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  explicit sbGstMessageEvent(GstMessage *msg, sbGStreamerMediacore *core) :
      mCore(core)
  {
    gst_message_ref(msg);
    mMessage = msg;
  }

  ~sbGstMessageEvent() {
    gst_message_unref(mMessage);
  }

  NS_IMETHOD Run()
  {
    mCore->HandleMessage(mMessage);
    return NS_OK;
  }

private:
  GstMessage *mMessage;
  sbGStreamerMediacore *mCore;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(sbGstMessageEvent,
                              nsIRunnable)

/* static */ void
sbGStreamerMediacore::syncHandler(GstBus* bus, GstMessage* message, 
        gpointer data)
{
  sbGStreamerMediacore *core = static_cast<sbGStreamerMediacore*>(data);

  // Allow a sync handler to look at this first.
  // If it returns false (the default), we dispatch it asynchronously.
  PRBool handled = core->HandleSynchronousMessage(message);

  if (!handled) {
    nsCOMPtr<nsIRunnable> event = new sbGstMessageEvent(message, core);
    NS_DispatchToMainThread(event);
  }
}

/* Must be called with mMonitor held */
nsresult 
sbGStreamerMediacore::DestroyPipeline()
{
  nsAutoMonitor lock(mMonitor);

  if (mPipeline) {
    gst_element_set_state (mPipeline, GST_STATE_NULL);
    gst_object_unref (mPipeline);

    mPipeline = nsnull;
  }

  return NS_OK;
}

/* Must be called with mMonitor held */
nsresult 
sbGStreamerMediacore::CreatePlaybackPipeline()
{
  nsresult rv;

  nsAutoMonitor lock(mMonitor);

  rv = DestroyPipeline();
  NS_ENSURE_SUCCESS (rv, rv);

  mPipeline = gst_element_factory_make ("playbin2", "player");

  if (!mPipeline)
    return NS_ERROR_FAILURE;

  if (mPlatformInterface) {
    GstElement *videosink = mPlatformInterface->CreateVideoSink();
    GstElement *audiosink = mPlatformInterface->CreateAudioSink();
    g_object_set(mPipeline, "video-sink", videosink, NULL);
    g_object_set(mPipeline, "audio-sink", audiosink, NULL);
  }

  GstBus *bus = gst_element_get_bus (mPipeline);

  // We want to receive state-changed messages when shutting down, so we
  // need to turn off bus auto-flushing
  g_object_set(mPipeline, "auto-flush-bus", FALSE, NULL);

  gst_bus_enable_sync_message_emission (bus);

  // Handle GStreamer messages synchronously, either directly or
  // dispatching to the main thread.
  g_signal_connect (bus, "sync-message",
          G_CALLBACK (syncHandler), this);

  g_object_unref ((GObject *)bus);

  return NS_OK;
}

PRBool sbGStreamerMediacore::HandleSynchronousMessage(GstMessage *aMessage)
{
  /* XXX: Handle prepare-xwindow-id here */

  /* Return PR_FALSE since we haven't handled the message */
  return PR_FALSE;
}

void sbGStreamerMediacore::DispatchMediacoreEvent (unsigned long type, 
        nsIVariant *aData, sbIMediacoreError *aError)
{
  nsresult rv;
  nsCOMPtr<sbIMediacoreEvent> event;
  rv = sbMediacoreEvent::CreateEvent(type,
                                     aError,
                                     aData,
                                     this,
                                     getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, /* void */);

  rv = DispatchEvent(event, PR_FALSE, nsnull);
  NS_ENSURE_SUCCESS(rv, /* void */);
}

void sbGStreamerMediacore::HandleTagMessage(GstMessage *message)
{
  GstTagList *tag_list;
  nsresult rv;

  LOG(("Handling tag message"));
  gst_message_parse_tag(message, &tag_list);

  if (mTags) {
    GstTagList *newTags = gst_tag_list_merge (mTags, tag_list,
            GST_TAG_MERGE_REPLACE);
    gst_tag_list_free (mTags);
    mTags = newTags;
  }
  else
    mTags = gst_tag_list_copy (tag_list);

  rv = ConvertTagListToPropertyArray(mTags, getter_AddRefs(mProperties));
  gst_tag_list_free(tag_list);

  if (NS_SUCCEEDED (rv)) {
    nsCOMPtr<nsISupports> properties = do_QueryInterface(mProperties, &rv);
    NS_ENSURE_SUCCESS (rv, /* void */);
    nsCOMPtr<nsIVariant> propVariant = sbNewVariant(properties).get();
    DispatchMediacoreEvent (sbIMediacoreEvent::METADATA_CHANGE, propVariant);
  }
  else // Non-fatal, just log a message
    LOG(("Failed to convert")); 
}

void sbGStreamerMediacore::HandleStateChangedMessage(GstMessage *message)
{
  // Only listen to state-changed messages from top-level pipelines
  if (GST_IS_PIPELINE (message->src))
  {
    GstState oldstate, newstate, pendingstate;
    gst_message_parse_state_changed (message, 
            &oldstate, &newstate, &pendingstate);

    // Dispatch START, PAUSE, END (but only if it's our target state)
    if (pendingstate == GST_STATE_VOID_PENDING) {
      if (newstate == GST_STATE_PLAYING)
        DispatchMediacoreEvent (sbIMediacoreEvent::STREAM_START);
      else if (newstate == GST_STATE_PAUSED)
        DispatchMediacoreEvent (sbIMediacoreEvent::STREAM_PAUSE);
      else if (newstate == GST_STATE_NULL)
        DispatchMediacoreEvent (sbIMediacoreEvent::STREAM_END);
    }
  }
}

void sbGStreamerMediacore::HandleEOSMessage(GstMessage *message)
{
  nsAutoMonitor lock(mMonitor);

  // Shut down the pipeline. This will cause us to send a STREAM_END
  // event when we get the state-changed message to GST_STATE_NULL
  gst_element_set_state (mPipeline, GST_STATE_NULL);
}

void sbGStreamerMediacore::HandleErrorMessage(GstMessage *message)
{
  GError *gerror = NULL;
  nsString errormessage;
  nsCOMPtr<sbMediacoreError> error;
  nsCOMPtr<sbIMediacoreEvent> event;

  nsAutoMonitor lock(mMonitor);

  // Create and dispatch an error event. 
  NS_NEWXPCOM(error, sbMediacoreError);
  NS_ENSURE_TRUE(error, /* void */);

  gst_message_parse_error(message, &gerror, NULL);
  CopyUTF8toUTF16(nsDependentCString(gerror->message), errormessage);
  error->Init(0, errormessage); // XXX: Use a proper error code once they exist
  g_error_free (gerror);

  DispatchMediacoreEvent(sbIMediacoreEvent::ERROR_EVENT, nsnull, error);

  // Then, shut down the pipeline, which will cause
  // a STREAM_END event to be fired.
  gst_element_set_state (mPipeline, GST_STATE_NULL);
}

/* Dispatch messages based on type.
 * For ELEMENT messages, further introspect the exact meaning for
 * dispatch
 */
void sbGStreamerMediacore::HandleMessage (GstMessage *message)
{
  GstMessageType msg_type;
  msg_type = GST_MESSAGE_TYPE(message);

  switch (msg_type) {
    case GST_MESSAGE_STATE_CHANGED: {
      HandleStateChangedMessage(message);
      break;
    }
    case GST_MESSAGE_TAG:
      HandleTagMessage(message);
      break;
    case GST_MESSAGE_ERROR:
      HandleErrorMessage(message);
      break;
    case GST_MESSAGE_EOS: {
      HandleEOSMessage(message);
      break;
    }
    default:
      LOG(("Got message: %s", gst_message_type_get_name(msg_type)));
      break;
  }
}

//-----------------------------------------------------------------------------
// sbBaseMediacore
//-----------------------------------------------------------------------------

/*virtual*/ nsresult 
sbGStreamerMediacore::OnInitBaseMediacore()
{
  nsresult rv;

  // Ensure the service component is loaded; it initialises GStreamer for us.
  nsCOMPtr<sbIGStreamerService2> service =
    do_GetService(SBGSTREAMERSERVICE2_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/*virtual*/ nsresult 
sbGStreamerMediacore::OnGetCapabilities()
{
  // XXXAus: Implement this when implementing the default sequencer!
  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult 
sbGStreamerMediacore::OnShutdown()
{
  nsAutoMonitor lock(mMonitor);

  if (mPipeline) {
    LOG (("Destroying pipeline on shutdown"));
    DestroyPipeline();
  }

  return NS_OK;
}

//-----------------------------------------------------------------------------
// sbBaseMediacorePlaybackControl
//-----------------------------------------------------------------------------

/*virtual*/ nsresult 
sbGStreamerMediacore::OnInitBaseMediacorePlaybackControl()
{
  /* Need to create the platform interface stuff here once that's updated. */

  return NS_OK;
}

/*virtual*/ nsresult 
sbGStreamerMediacore::OnSetUri(nsIURI *aURI)
{
  nsCAutoString spec;
  nsresult rv;
  nsAutoMonitor lock(mMonitor);

  rv = CreatePlaybackPipeline();
  NS_ENSURE_SUCCESS (rv,rv);

  rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  /* Set the URI to play */
  g_object_set (G_OBJECT (mPipeline), "uri", spec.get(), NULL);

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerMediacore::GetPosition(PRUint64 *aPosition)
{
  GstQuery *query;
  gboolean res;
  nsresult rv;
  nsAutoMonitor lock(mMonitor);

  query = gst_query_new_position(GST_FORMAT_TIME);
  res = gst_element_query(mPipeline, query);

  if(res) {
    gint64 position;
    gst_query_parse_position(query, NULL, &position);
    
    /* Convert to milliseconds */
    *aPosition = position / GST_MSECOND;
    rv = NS_OK;
  }
  else
    rv = NS_ERROR_NOT_AVAILABLE;

  gst_query_unref (query);

  return rv;
}

/*virtual*/ nsresult 
sbGStreamerMediacore::OnSetPosition(PRUint64 aPosition)
{
  GstClockTime position;
  gboolean ret;
  nsAutoMonitor lock(mMonitor);

  // Incoming position is in milliseconds, convert to GstClockTime (nanoseconds)
  position = aPosition * GST_MSECOND;

  // Do a flushing keyframe seek to the requested position. This is the simplest
  // and fastest type of seek.
  ret = gst_element_seek_simple (mPipeline, GST_FORMAT_TIME, 
      (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
      position);
  
  if (!ret) {
    /* TODO: Is this appropriate for a non-fatal failure to seek? Should we
       fire an event? */
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

/*virtual*/ nsresult 
sbGStreamerMediacore::OnPlay()
{
  nsAutoMonitor lock(mMonitor);

  NS_ENSURE_STATE(mPipeline);

  GstStateChangeReturn ret;
  ret = gst_element_set_state (mPipeline, GST_STATE_PLAYING);

  /* Usually ret will be GST_STATE_CHANGE_ASYNC, but we could get a synchronous
   * error... */
  if (ret == GST_STATE_CHANGE_FAILURE)
    return NS_ERROR_FAILURE;
  return NS_OK;
}

/*virtual*/ nsresult 
sbGStreamerMediacore::OnPause()
{
  nsAutoMonitor lock(mMonitor);

  NS_ENSURE_STATE(mPipeline);

  GstStateChangeReturn ret;
  ret = gst_element_set_state (mPipeline, GST_STATE_PAUSED);

  if (ret == GST_STATE_CHANGE_FAILURE)
    return NS_ERROR_FAILURE;
  return NS_OK;
}

/*virtual*/ nsresult 
sbGStreamerMediacore::OnStop()
{
  nsAutoMonitor lock(mMonitor);

  NS_ENSURE_STATE(mPipeline);

  GstStateChangeReturn ret;
  ret = gst_element_set_state (mPipeline, GST_STATE_NULL);

  return NS_OK;
}

//-----------------------------------------------------------------------------
// sbBaseMediacoreVolumeControl
//-----------------------------------------------------------------------------

/*virtual*/ nsresult 
sbGStreamerMediacore::OnInitBaseMediacoreVolumeControl()
{
  mVolume = 1.0;
  mMute = PR_FALSE;

  return NS_OK;
}

/*virtual*/ nsresult 
sbGStreamerMediacore::OnSetMute(PRBool aMute)
{
  nsAutoMonitor lock(mMonitor);

  NS_ENSURE_STATE(mPipeline);

  /* We have no explicit mute control, so just set the volume to zero, but
   * don't update our internal mVolume value */
  g_object_set(mPipeline, "volume", 0.0, NULL);

  return NS_OK;
}

/*virtual*/ nsresult 
sbGStreamerMediacore::OnSetVolume(double aVolume)
{
  nsAutoMonitor lock(mMonitor);

  NS_ENSURE_STATE(mPipeline);

  /* Well, this is nice and easy! */
  g_object_set(mPipeline, "volume", aVolume, NULL);

  return NS_OK;
}

//-----------------------------------------------------------------------------
// sbIMediacoreVotingParticipant
//-----------------------------------------------------------------------------

NS_IMETHODIMP 
sbGStreamerMediacore::VoteWithURI(nsIURI *aURI, PRUint32 *_retval)
{
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_ARG_POINTER(_retval);

  // XXXAus: Run aURI through extension filtering first.
  //
  //         After that, that's as much as we can do, it's most likely
  //         playable.
  
  *_retval = 2000;

  return NS_OK;
}

NS_IMETHODIMP 
sbGStreamerMediacore::VoteWithChannel(nsIChannel *aChannel, PRUint32 *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

//-----------------------------------------------------------------------------
// sbIGStreamerMediacore
//-----------------------------------------------------------------------------

NS_IMETHODIMP
sbGStreamerMediacore::GetGstreamerVersion(nsAString& aGStreamerVersion)
{
  nsString versionString;

  versionString.AppendInt(GST_VERSION_MAJOR);
  versionString.AppendLiteral(".");
  versionString.AppendInt(GST_VERSION_MINOR);
  versionString.AppendLiteral(".");
  versionString.AppendInt(GST_VERSION_MICRO);
  
  aGStreamerVersion.Assign(versionString);

  return NS_OK;
}

// Forwarding functions for sbIMediacoreEventTarget interface

NS_IMETHODIMP
sbGStreamerMediacore::DispatchEvent(sbIMediacoreEvent *aEvent,
                                    PRBool aAsync,
                                    PRBool* _retval)
{
  return mBaseEventTarget ? 
         mBaseEventTarget->DispatchEvent(aEvent, aAsync, _retval) : 
         NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
sbGStreamerMediacore::AddListener(sbIMediacoreEventListener *aListener)
{
  return mBaseEventTarget ? 
         mBaseEventTarget->AddListener(aListener) : 
         NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
sbGStreamerMediacore::RemoveListener(sbIMediacoreEventListener *aListener)
{
  return mBaseEventTarget ? 
         mBaseEventTarget->RemoveListener(aListener) : 
         NS_ERROR_NULL_POINTER;
}

