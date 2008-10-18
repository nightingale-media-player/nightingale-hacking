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
#include <nsIIOService.h>
#include <nsThreadUtils.h>
#include <nsCOMPtr.h>
#include <prlog.h>

// Required to crack open the DOM XUL Element and get a native window handle.
#include <nsIBaseWindow.h>
#include <nsIBoxObject.h>
#include <nsIDocument.h>
#include <nsIDOMAbstractView.h>
#include <nsIDOMEvent.h>
#include <nsIDocShellTreeItem.h>
#include <nsIDocShellTreeOwner.h>
#include <nsIDOMDocument.h>
#include <nsIDOMDocumentView.h>
#include <nsIDOMEventTarget.h>
#include <nsIDOMXULElement.h>
#include <nsIScriptGlobalObject.h>
#include <nsIWebNavigation.h>
#include <nsIWidget.h>

#include <sbClassInfoUtils.h>
#include <sbTArrayStringEnumerator.h>
#include <sbVariantUtils.h>
#include <sbBaseMediacoreEventTarget.h>
#include <sbMediacoreError.h>
#include <sbProxiedComponentManager.h>

#include <sbIGStreamerService.h>
#include <sbIMediaItem.h>

#include "sbGStreamerMediacoreUtils.h"

#ifdef MOZ_WIDGET_GTK2
#include "sbGStreamerPlatformGDK.h"
#endif

#ifdef XP_WIN
#include "sbGStreamerPlatformWin32.h"
#endif

#ifdef XP_MACOSX
#include "sbGStreamerPlatformOSX.h"
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

NS_IMPL_THREADSAFE_ADDREF(sbGStreamerMediacore)
NS_IMPL_THREADSAFE_RELEASE(sbGStreamerMediacore)

NS_IMPL_QUERY_INTERFACE9_CI(sbGStreamerMediacore,
                            sbIMediacore,
                            sbIMediacorePlaybackControl,
                            sbIMediacoreVolumeControl,
                            sbIMediacoreVotingParticipant,
                            sbIMediacoreEventTarget,
                            sbIMediacoreVideoWindow,
                            sbIGStreamerMediacore,
                            nsIDOMEventListener,
                            nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER7(sbGStreamerMediacore,
                             sbIMediacore,
                             sbIMediacorePlaybackControl,
                             sbIMediacoreVideoWindow,
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
    mProperties(nsnull),
    mStopped(PR_FALSE),
    mBuffering(PR_FALSE)
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

/* static */ void
sbGStreamerMediacore::aboutToFinishHandler(GstElement *playbin, gpointer data)
{
  sbGStreamerMediacore *core = static_cast<sbGStreamerMediacore*>(data);
  core->HandleAboutToFinishSignal();
  return;
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
  mStopped = PR_FALSE;
  mBuffering = PR_FALSE;

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

  // Handle about-to-finish signal emitted by playbin2
  g_signal_connect (mPipeline, "about-to-finish", 
          G_CALLBACK (aboutToFinishHandler), this);

  return NS_OK;
}

PRBool sbGStreamerMediacore::HandleSynchronousMessage(GstMessage *aMessage)
{
  GstMessageType msg_type;
  msg_type = GST_MESSAGE_TYPE(aMessage);

  switch (msg_type) {
    case GST_MESSAGE_ELEMENT: {
      // Win32 and GDK use prepare-xwindow-id, OSX has its own private thing,
      // have-ns-view
      if (gst_structure_has_name(aMessage->structure, "prepare-xwindow-id") ||
          gst_structure_has_name(aMessage->structure, "have-ns-view"))
      {
        if(mPlatformInterface) 
        {
          DispatchMediacoreEvent(sbIMediacoreEvent::STREAM_HAS_VIDEO);
          mPlatformInterface->PrepareVideoWindow(aMessage);
        }
        return PR_TRUE;
      }
    }
  }

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

void sbGStreamerMediacore::HandleAboutToFinishSignal()
{
  LOG(("Handling about-to-finish signal"));

  nsAutoLock lock(sbBaseMediacore::mLock);
  nsCOMPtr<sbIMediacoreSequencer> sequencer = mSequencer;
  lock.unlock();

  if(!sequencer) {
    return;
  }

  nsCOMPtr<sbIMediaItem> item;
  nsresult rv = sequencer->GetNextItem(getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, /*void*/ );
  NS_ENSURE_TRUE(item, /*void*/);

  nsCOMPtr<nsIURI> uri;
  rv = item->GetContentSrc(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, /*void*/ );

  PRBool schemeIsFile = PR_FALSE;
  
  rv = uri->SchemeIs("file", &schemeIsFile);
  NS_ENSURE_SUCCESS(rv, /*void*/);

  if(schemeIsFile) {
    rv = sequencer->RequestHandleNextItem(this);
    NS_ENSURE_SUCCESS(rv, /*void*/ );

    nsCString spec;
    rv = uri->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, /*void*/);

    LOG(("Setting URI to \"%s\"", spec.get()));

    /* Set the URI to play */
    nsAutoMonitor mon(mMonitor);
    NS_ENSURE_TRUE(mPipeline, /*void*/);
    g_object_set (G_OBJECT (mPipeline), "uri", spec.get(), NULL);
  }

  return;
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

    // Dispatch START, PAUSE, STOP/END (but only if it's our target state)
    if (pendingstate == GST_STATE_VOID_PENDING) {
      if (newstate == GST_STATE_PLAYING)
        DispatchMediacoreEvent (sbIMediacoreEvent::STREAM_START);
      else if (newstate == GST_STATE_PAUSED)
        DispatchMediacoreEvent (sbIMediacoreEvent::STREAM_PAUSE);
      else if (newstate == GST_STATE_NULL)
      {
        // Distinguish between 'stopped via API' and 'stopped due to error or
        // reaching EOS'
        if (mStopped)
          DispatchMediacoreEvent (sbIMediacoreEvent::STREAM_STOP);
        else
          DispatchMediacoreEvent (sbIMediacoreEvent::STREAM_END);
      }
    }
  }
}

void sbGStreamerMediacore::HandleBufferingMessage (GstMessage *message)
{
  nsAutoMonitor lock(mMonitor);

  gint percent = 0;
  gst_message_parse_buffering (message, &percent);
  TRACE(("Buffering (%u percent done)", percent));

  /* If we receive buffering messages, go to PAUSED.
   * Then, return to PLAYING once we have 100% buffering (which will be
   * before we actually hit PAUSED)
   */
  if (mBuffering && percent >= 100 ) {
    TRACE(("Buffering complete, setting back to playing"));
    mBuffering = PR_FALSE;
    gst_element_set_state (mPipeline, GST_STATE_PLAYING);
  }
  else if (!mBuffering && percent < 100) {
    GstState cur_state;
    gst_element_get_state (mPipeline, &cur_state, NULL, 0);

    /* Only pause if we've already reached playing (this means we've underrun
     * the buffer and need to rebuffer) */
    if (cur_state == GST_STATE_PLAYING) {
      TRACE(("Buffering... setting to paused"));
      gst_element_set_state (mPipeline, GST_STATE_PAUSED);
      mBuffering = PR_TRUE;

      // And inform listeners that we've underrun */
      DispatchMediacoreEvent(sbIMediacoreEvent::BUFFER_UNDERRUN);
    }

    // Inform listeners of current progress
    double bufferingProgress = (double)percent / 100.;
    nsCOMPtr<nsIVariant> variant = sbNewVariant(bufferingProgress).get();
    DispatchMediacoreEvent(sbIMediacoreEvent::BUFFERING, variant);
  }
}

// Demuxers (such as qtdemux) send redirect messages when the media
// file itself redirects to another location. Handle these here.
void sbGStreamerMediacore::HandleRedirectMessage(GstMessage *message)
{
  const gchar *location;
  nsresult rv;
  nsCString uriString;

  location = gst_structure_get_string (message->structure, "new-location");

  if (location && *location) {
    if (strstr (location, "://") != NULL) {
      // Then we assume it's an absolute URL
      uriString = location;
    }
    else {
      TRACE (("Resolving redirect to '%s'", location));

      rv = mUri->Resolve(nsDependentCString(location), uriString);
      NS_ENSURE_SUCCESS (rv, /* void */ );
    }

    // Now create a URI from our string form.
    nsCOMPtr<nsIIOService> ioService = do_GetService(
            "@mozilla.org/network/io-service;1", &rv);
    NS_ENSURE_SUCCESS (rv, /* void */ );

    nsCOMPtr<nsIURI> finaluri;
    rv = ioService->NewURI(uriString, nsnull, nsnull,
            getter_AddRefs(finaluri));
    NS_ENSURE_SUCCESS (rv, /* void */ );

    PRBool isEqual;
    rv = finaluri->Equals(mUri, &isEqual);
    NS_ENSURE_SUCCESS (rv, /* void */ );

    // Don't loop forever redirecting to ourselves. If the URIs are the same,
    // then just ignore the redirect message.
    if (isEqual)
      return;

    // Ok, we have a new uri, and we're ready to use it... 
    rv = SetUri(finaluri);
    NS_ENSURE_SUCCESS (rv, /* void */ );

    // Inform listeners that we've switched URI
    nsCOMPtr<nsIVariant> propVariant = sbNewVariant(finaluri).get();
    DispatchMediacoreEvent (sbIMediacoreEvent::URI_CHANGE, propVariant);

    // And finally, attempt to play it
    rv = Play();
    NS_ENSURE_SUCCESS (rv, /* void */ );
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

  LOG(("Got message: %s", gst_message_type_get_name(msg_type)));

  switch (msg_type) {
    case GST_MESSAGE_STATE_CHANGED:
      HandleStateChangedMessage(message);
      break;
    case GST_MESSAGE_TAG:
      HandleTagMessage(message);
      break;
    case GST_MESSAGE_ERROR:
      HandleErrorMessage(message);
      break;
    case GST_MESSAGE_EOS:
      HandleEOSMessage(message);
      break;
    case GST_MESSAGE_BUFFERING:
      HandleBufferingMessage(message);
    case GST_MESSAGE_ELEMENT: {
      if (gst_structure_has_name (message->structure, "redirect")) {
        HandleRedirectMessage(message);
      }
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
  nsCOMPtr<sbIGStreamerService> service =
    do_GetService(SBGSTREAMERSERVICE_CONTRACTID, &rv);
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

  LOG(("Setting URI to \"%s\"", spec.get()));

  /* Set the URI to play */
  g_object_set (G_OBJECT (mPipeline), "uri", spec.get(), NULL);

  return NS_OK;
}

/*virtual*/ nsresult 
sbGStreamerMediacore::OnGetDuration(PRUint64 *aDuration)
{
  GstQuery *query;
  gboolean res;
  nsresult rv;
  nsAutoMonitor lock(mMonitor);

  query = gst_query_new_duration(GST_FORMAT_TIME);
  res = gst_element_query(mPipeline, query);

  if(res) {
    gint64 duration;
    gst_query_parse_duration(query, NULL, &duration);

    /* Convert to milliseconds */
    *aDuration = duration / GST_MSECOND;
    rv = NS_OK;
  }
  else
    rv = NS_ERROR_NOT_AVAILABLE;

  gst_query_unref (query);

  return rv;
}

/*virtual*/ nsresult
sbGStreamerMediacore::OnGetPosition(PRUint64 *aPosition)
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

  gint flags = 0x2 | 0x10; // audio | soft-volume
  if (mVideoEnabled) {
    // Enable video only if we're set up for it is turned off. Also enable
    // text (subtitles), which require a video window to display.
    flags |= 0x1 | 0x4; // video | text
  }

  g_object_set (G_OBJECT(mPipeline), "flags", flags, NULL);

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

  mStopped = PR_TRUE;
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

  if(!aMute && mMute) {
    nsAutoLock lock(sbBaseMediacoreVolumeControl::mLock);

    /* Well, this is nice and easy! */
    g_object_set(mPipeline, "volume", mVolume, NULL);
  }
  else if(aMute && !mMute){
    /* We have no explicit mute control, so just set the volume to zero, but
    * don't update our internal mVolume value */
    g_object_set(mPipeline, "volume", 0.0, NULL);
  }

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
// sbIMediacoreVideoWindow
//-----------------------------------------------------------------------------

NS_IMETHODIMP
sbGStreamerMediacore::GetFullscreen(PRBool *aFullscreen)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbGStreamerMediacore::SetFullscreen(PRBool aFullscreen)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbGStreamerMediacore::GetVideoWindow(nsIDOMXULElement **aVideoWindow)
{
  nsAutoMonitor mon(mMonitor);
  NS_IF_ADDREF(*aVideoWindow = mVideoWindow);

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerMediacore::SetVideoWindow(nsIDOMXULElement *aVideoWindow)
{
  NS_ENSURE_ARG_POINTER(aVideoWindow);
  
  nsAutoMonitor mon(mMonitor);

  // Get the box object representing the actual display area for the video.
  nsCOMPtr<nsIBoxObject> boxObject;
  nsresult rv = aVideoWindow->GetBoxObject(getter_AddRefs(boxObject));
  NS_ENSURE_SUCCESS (rv, rv);

  nsCOMPtr<nsIDOMDocument> domDocument;
  rv = aVideoWindow->GetOwnerDocument(getter_AddRefs(domDocument));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMDocumentView> domDocumentView(do_QueryInterface(domDocument));
  NS_ENSURE_TRUE(domDocumentView, NS_NOINTERFACE);

  nsCOMPtr<nsIDOMAbstractView> domAbstractView;
  rv = domDocumentView->GetDefaultView(getter_AddRefs(domAbstractView));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIWebNavigation> webNavigation(do_GetInterface(domAbstractView));
  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem(do_QueryInterface(webNavigation));
  NS_ENSURE_TRUE(docShellTreeItem, NS_NOINTERFACE);

  nsCOMPtr<nsIDocShellTreeOwner> docShellTreeOwner;
  rv = docShellTreeItem->GetTreeOwner(getter_AddRefs(docShellTreeOwner));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(docShellTreeOwner);
  NS_ENSURE_TRUE(baseWindow, NS_NOINTERFACE);

  nsCOMPtr<nsIWidget> widget;
  rv = baseWindow->GetMainWidget(getter_AddRefs(widget));
  NS_ENSURE_SUCCESS(rv, rv);

  // Attach event listeners
  nsCOMPtr<nsIDocument> document(do_QueryInterface(domDocument));
  NS_ENSURE_TRUE(document, NS_NOINTERFACE);

  mDOMWindow = do_QueryInterface(document->GetScriptGlobalObject());
  NS_ENSURE_TRUE(mDOMWindow, NS_NOINTERFACE);

  nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(mDOMWindow));
  NS_ENSURE_TRUE(target, NS_NOINTERFACE);
  target->AddEventListener(NS_LITERAL_STRING("resize"), this, PR_FALSE);
  target->AddEventListener(NS_LITERAL_STRING("unload"), this, PR_FALSE);

  nsCOMPtr<nsIThread> eventTarget;
  rv = NS_GetMainThread(getter_AddRefs(eventTarget));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIBoxObject> proxiedBoxObject;
  rv = do_GetProxyForObject(eventTarget,
                            NS_GET_IID(nsIBoxObject),
                            boxObject,
                            NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                            getter_AddRefs(proxiedBoxObject));


  mVideoEnabled = PR_TRUE;
  mVideoWindow = aVideoWindow;

#if defined (MOZ_WIDGET_GTK2)
  GdkWindow *native = GDK_WINDOW(widget->GetNativeData(NS_NATIVE_WIDGET));
  LOG(("Found native window %x", native));
  mPlatformInterface = new GDKPlatformInterface(proxiedBoxObject, native);
#elif defined (XP_WIN)
  HWND native = (HWND)widget->GetNativeData(NS_NATIVE_WIDGET);
  LOG(("Found native window %x", native));
  mPlatformInterface = new Win32PlatformInterface(proxiedBoxObject, native);
#elif defined (XP_MACOSX)
  void * native = (void *)widget->GetNativeData(NS_NATIVE_WIDGET);
  LOG(("Found native window %x", native));
  mPlatformInterface = new OSXPlatformInterface(proxiedBoxObject, native);
#else
  LOG(("No video backend available for this platform"));
  mVideoEnabled = PR_FALSE;
#endif

  return NS_OK;
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


//-----------------------------------------------------------------------------
// nsIDOMEventListener
//-----------------------------------------------------------------------------

NS_IMETHODIMP
sbGStreamerMediacore::HandleEvent(nsIDOMEvent* aEvent)
{
  nsAutoString eventType;
  aEvent->GetType(eventType);

  if(eventType.EqualsLiteral("unload")) {

    // Clean up here
    nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(mDOMWindow));
    NS_ENSURE_TRUE(target, NS_NOINTERFACE);
    target->RemoveEventListener(NS_LITERAL_STRING("resize"), this, PR_FALSE);
    target->RemoveEventListener(NS_LITERAL_STRING("unload"), this, PR_FALSE);

    mDOMWindow = nsnull;
  }
  else if(eventType.EqualsLiteral("resize") &&
          mPlatformInterface) {
    mPlatformInterface->ResizeToWindow();  
  }

  return NS_OK;
}
