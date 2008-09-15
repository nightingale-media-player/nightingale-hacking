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

#include <sbIGStreamerService.h>

#include "sbGStreamerMediacoreUtils.h"

#ifdef MOZ_WIDGET_GTK2
#include "sbGStreamerPlatformGDK.h"
#endif

#ifdef XP_WIN
#include "sbGStreamerPlatformWin32.h"
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

NS_IMPL_ISUPPORTS_INHERITED6(sbGStreamerMediacore,
                             sbBaseMediacore,
                             sbIMediacore,
                             sbIMediacorePlaybackControl,
                             sbIMediacoreVolumeControl,
                             sbIMediacoreVotingParticipant,
                             sbIGStreamerMediacore,
                             nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER5(sbGStreamerMediacore,
                             sbIMediacore,
                             sbIMediacorePlaybackControl,
                             sbIMediacoreVolumeControl,
                             sbIMediacoreVotingParticipant,
                             sbIGStreamerMediacore)

NS_DECL_CLASSINFO(sbGStreamerMediacore)
NS_IMPL_THREADSAFE_CI(sbGStreamerMediacore)

sbGStreamerMediacore::sbGStreamerMediacore() :
    mVideoEnabled(PR_FALSE),
    mPipeline(nsnull),
    mPlatformInterface(nsnull)
{
}

sbGStreamerMediacore::~sbGStreamerMediacore()
{
}

nsresult
sbGStreamerMediacore::Init() 
{
  nsresult rv = sbBaseMediacore::InitBaseMediacore();
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

/*virtual */ nsresult 
sbGStreamerMediacore::DestroyPipeline()
{
  if (mPipeline) {
    gst_element_set_state (mPipeline, GST_STATE_NULL);
    gst_object_unref (mPipeline);

    mPipeline = nsnull;
  }

  return NS_OK;
}

/*virtual */ nsresult 
sbGStreamerMediacore::CreatePlaybackPipeline()
{
  nsresult rv = DestroyPipeline();
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

  gst_bus_add_signal_watch (bus);
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

void sbGStreamerMediacore::HandleMessage (GstMessage *message)
{
  GstMessageType msg_type;
  msg_type = GST_MESSAGE_TYPE(message);

  switch (msg_type) {
    case GST_MESSAGE_ERROR:
    case GST_MESSAGE_EOS:
      /* TODO: Once we have events hooked up, this should just fire an event
       * and we'll shut down only once told to */
      gst_element_set_state (mPipeline, GST_STATE_NULL);
      break;
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
sbGStreamerMediacore::OnSetCurrentSequence(sbIMediacoreSequence *aCurrentSequence)
{
  // XXXAus: Implement this when implementing the default sequencer!
  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult 
sbGStreamerMediacore::OnShutdown()
{
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

  rv = CreatePlaybackPipeline();
  NS_ENSURE_SUCCESS (rv,rv);

  rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  /* Set the URI to play */
  g_object_set (G_OBJECT (mPipeline), "uri", spec.get(), NULL);

  return NS_OK;
}

/*virtual*/ nsresult 
sbGStreamerMediacore::OnSetPosition(PRUint64 aPosition)
{
  // Incoming position is in milliseconds, convert to GstClockTime (nanoseconds)
  GstClockTime position = aPosition * GST_MSECOND;
  gboolean ret;

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
  NS_ENSURE_STATE(mPipeline);

  /* We have no explicit mute control, so just set the volume to zero, but
   * don't update our internal mVolume value */
  g_object_set(mPipeline, "volume", 0.0, NULL);

  return NS_OK;
}

/*virtual*/ nsresult 
sbGStreamerMediacore::OnSetVolume(double aVolume)
{
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
