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

#include "sbGStreamerSimple.h"
#include "sbGStreamerErrorEvent.h"

#include <sbIGStreamerService.h>
#include <sbIGStreamerEvent.h>
#include <sbIGStreamerEventListener.h>

#include <nsIInterfaceRequestorUtils.h>
#include <nsIBaseWindow.h>
#include <nsIBoxObject.h>
#include <nsIBrowserDOMWindow.h>
#include <nsIDocShellTreeItem.h>
#include <nsIDocShellTreeOwner.h>
#include <nsIDocument.h>
#include <nsIDOMAbstractView.h>
#include <nsIDOMChromeWindow.h>
#include <nsIDOMDocument.h>
#include <nsIDOMDocumentView.h>
#include <nsIDOMEventTarget.h>
#include <nsIDOMEvent.h>
#include <nsIDOMEventListener.h>
#include <nsIDOMWindow.h>
#include <nsIDOMWindowInternal.h>
#include <nsIDOMXULElement.h>
#include <nsIIOService.h>
#include <nsIPrefBranch.h>
#include <nsIPromptService.h>
#include <nsIProxyObjectManager.h>
#include <nsIRunnable.h>
#include <nsIScriptGlobalObject.h>
#include <nsIWebNavigation.h>
#include <nsIWidget.h>
#include <nsIWindowMediator.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>
#include <nsThreadUtils.h>
#include <prlog.h>

#ifdef MOZ_WIDGET_GTK2
#include "sbGStreamerPlatformGDK.h"
#endif

#ifdef XP_WIN
#include "sbGStreamerPlatformWin32.h"
#endif

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=gstreamer:5
 */
#ifdef PR_LOGGING
extern PRLogModuleInfo* gGStreamerLog;
#endif /* PR_LOGGING */

#define TRACE(args) PR_LOG(gGStreamerLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gGStreamerLog, PR_LOG_WARN, args)

class sbGstMessageEvent : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  explicit sbGstMessageEvent(GstMessage *msg, sbGStreamerSimple *gsts) :
      mGST(gsts)
  {
    gst_message_ref(msg);
    mMessage = msg;
  }

  ~sbGstMessageEvent() {
    gst_message_unref(mMessage);
  }

  NS_IMETHOD Run()
  {
    mGST->HandleMessage(mMessage);
    return NS_OK;
  }

private:
  GstMessage *mMessage;
  sbGStreamerSimple *mGST;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(sbGstMessageEvent,
                              nsIRunnable)

NS_IMPL_THREADSAFE_ISUPPORTS3(sbGStreamerSimple,
                              sbIGStreamerSimple,
                              nsIDOMEventListener,
                              nsITimerCallback)

// sync-message handler. Handles messages on streaming threads that must be
// acted on synchronously (currently, just prepare-xwindow-id)
/* static */ void
sbGStreamerSimple::syncHandler(GstBus* bus, GstMessage* message, gpointer data)
{
  sbGStreamerSimple* gsts = static_cast<sbGStreamerSimple*>(data);

  GstMessageType msg_type;
  msg_type = GST_MESSAGE_TYPE(message);

  switch (msg_type) {
    case GST_MESSAGE_ELEMENT:
      if (gst_structure_has_name(message->structure, "prepare-xwindow-id"))
      {
        gsts->PrepareVideoWindow(message);
        return;
      }
      // Fall through for other ELEMENT messages.
    default:
      nsCOMPtr<nsIRunnable> event = new sbGstMessageEvent(message, gsts);
      NS_DispatchToMainThread(event);
      break;
  }
}

/* static */ void
sbGStreamerSimple::videoCapsSetHelper(GObject* obj, GParamSpec* pspec, 
        sbGStreamerSimple* gsts)
{
  GstPad *pad = GST_PAD(obj);
  GstCaps *caps = gst_pad_get_negotiated_caps(pad);

  if (caps) {
    gsts->OnVideoCapsSet(caps);
    gst_caps_unref (caps);
  }
}

/* static */ void
sbGStreamerSimple::streamInfoSetHelper(GObject* obj, GParamSpec* pspec, 
        sbGStreamerSimple* gsts)
{
  gsts->OnStreamInfoSet();
}

/* static */ void
sbGStreamerSimple::currentVideoSetHelper(GObject* obj, GParamSpec* pspec, 
        sbGStreamerSimple* gsts)
{
  int current_video;
  GstPad *pad;

  /* Which video stream has been activated? */
  g_object_get(obj, "current-video", &current_video, NULL);
  NS_ASSERTION(current_video >= 0, "current video is negative");

  /* Get the video pad for this stream number */
  g_signal_emit_by_name(obj, "get-video-pad", current_video, &pad);

  if (pad) {
    GstCaps *caps;
    caps = gst_pad_get_negotiated_caps(pad);
    if (caps) {
      gsts->OnVideoCapsSet(caps);
      gst_caps_unref(caps);
    }

    g_signal_connect(pad, "notify::caps", 
            G_CALLBACK(videoCapsSetHelper), gsts);

    gst_object_unref(pad);
  }
}

sbGStreamerSimple::sbGStreamerSimple() :
  mInitialized(PR_FALSE),
  mPlay(NULL),
  mPixelAspectRatioN(1),
  mPixelAspectRatioD(1),
  mVideoWidth(0),
  mVideoHeight(0),
  mPlatformInterface(NULL),
  mIsAtEndOfStream(PR_TRUE),
  mIsPlayingVideo(PR_FALSE),
  mLastErrorCode(0),
  mBufferingPercent(0),
  mIsUsingPlaybin2(PR_FALSE),
  mLastVolume(0),
  mDomWindow(nsnull)
{
  TRACE(("sbGStreamerSimple[0x%.8x] - Constructed", this));
  mArtist.Assign(EmptyString());
  mAlbum.Assign(EmptyString());
  mTitle.Assign(EmptyString());
  mGenre.Assign(EmptyString());

}

sbGStreamerSimple::~sbGStreamerSimple()
{
  TRACE(("sbGStreamerSimple[0x%.8x] - Destructed", this));
  DestroyPlaybin();

  // Do i need to close the video window?

  mDomWindow = nsnull;
}

NS_IMETHODIMP
sbGStreamerSimple::Init(nsIDOMXULElement* aVideoOutput)
{
  NS_ENSURE_ARG_POINTER(aVideoOutput);
  TRACE(("sbGStreamerSimple[0x%.8x] - Init", this));

  nsresult rv;

  // We need to make sure the gstreamer service component has been loaded
  // since it calls gst_init for us
  nsCOMPtr<sbIGStreamerService> service =
    do_GetService(SBGSTREAMERSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if(mInitialized) {
    return NS_OK;
  }

  // Get the box object representing the actual display area for the video.
  nsCOMPtr<nsIBoxObject> boxObject;
  rv = aVideoOutput->GetBoxObject(getter_AddRefs(boxObject));
  NS_ENSURE_SUCCESS (rv, rv);

  nsCOMPtr<nsIDOMDocument> domDocument;
  rv = aVideoOutput->GetOwnerDocument(getter_AddRefs(domDocument));
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

  mDomWindow = do_QueryInterface(document->GetScriptGlobalObject());
  NS_ENSURE_TRUE(mDomWindow, NS_NOINTERFACE);

  nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(mDomWindow));
  NS_ENSURE_TRUE(target, NS_NOINTERFACE);
  target->AddEventListener(NS_LITERAL_STRING("resize"), this, PR_FALSE);
  target->AddEventListener(NS_LITERAL_STRING("unload"), this, PR_FALSE);

#if defined (MOZ_WIDGET_GTK2)
  GdkWindow *native = GDK_WINDOW(widget->GetNativeData(NS_NATIVE_WIDGET));
  LOG(("Found native window %x", native));
  mPlatformInterface = new GDKPlatformInterface(boxObject, native);
#elif defined (XP_WIN)
  HWND native = (HWND)widget->GetNativeData(NS_NATIVE_WIDGET);
  LOG(("Found native window %x", native));
  mPlatformInterface = new Win32PlatformInterface(boxObject, native);
#else
  LOG(("No video backend available for this platform"));
#endif

  rv = SetupPlaybin();
  NS_ENSURE_SUCCESS(rv, rv);

  mInitialized = PR_TRUE;

  return NS_OK;
}

nsresult
sbGStreamerSimple::SetupPlaybin()
{
  TRACE(("sbGStreamerSimple[0x%.8x] - SetupPlaybin", this));

  if (!mPlay) {
    if (0) {
      // playbin2 is a bit busted without a few patches, so disable this for
      // now.
      mIsUsingPlaybin2 = PR_TRUE;
      mPlay = gst_element_factory_make("playbin2", "play");
    }
    else {
      mIsUsingPlaybin2 = PR_FALSE;
      mPlay = gst_element_factory_make("playbin", "play");
    }

    if (mPlatformInterface) {
      GstElement *videosink = mPlatformInterface->CreateVideoSink();
      GstElement *audiosink = mPlatformInterface->CreateAudioSink();
      g_object_set(mPlay, "video-sink", videosink, NULL);
      g_object_set(mPlay, "audio-sink", audiosink, NULL);
    }

    GstBus *bus = gst_element_get_bus(mPlay);

    gst_bus_add_signal_watch (bus);
    gst_bus_enable_sync_message_emission (bus);

    // Handle GStreamer messages; synchronously, or dispatching to the
    // main thread.
    g_signal_connect (bus, "sync-message", 
            G_CALLBACK (syncHandler), this);

    // Get notified when the current video stream changes.
    // This will let us get info about the specific video stream being played.
    if (mIsUsingPlaybin2) {
      g_signal_connect(mPlay, "notify::current-video", 
              G_CALLBACK(currentVideoSetHelper), this);
    }
    else {
      g_signal_connect(mPlay, "notify::stream-info",
              G_CALLBACK(streamInfoSetHelper), this);
    }

    gst_object_unref(bus);

    // TODO: Hook up other audio/video signals/properties, and provide an 
    // interface for selecting which one to play.
  }

  return NS_OK;
}

nsresult
sbGStreamerSimple::DestroyPlaybin()
{
  TRACE(("sbGStreamerSimple[0x%.8x] - DestroyPlaybin", this));

  if (mPlay && GST_IS_ELEMENT(mPlay)) {
    gst_element_set_state(mPlay, GST_STATE_NULL);
    gst_object_unref(mPlay);
    mPlay = NULL;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::GetUri(nsAString& aUri)
{
  TRACE(("sbGStreamerSimple[0x%.8x] - GetUri", this));

  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  GValue value = { 0, };
  g_value_init(&value, G_TYPE_STRING);
  g_object_get_property(G_OBJECT(mPlay), "uri", &value);

  nsCAutoString uri;
  uri.Assign(g_value_get_string(&value));

  g_value_unset(&value);

  CopyUTF8toUTF16(uri, aUri);

  return NS_OK;
}
NS_IMETHODIMP
sbGStreamerSimple::SetUri(const nsAString& aUri)
{
  TRACE(("sbGStreamerSimple[0x%.8x] - SetUri(%s)", this,
         NS_LossyConvertUTF16toASCII(aUri).get()));

  nsresult rv;

  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  /*
   * Mercilessly clobber the playbin and recreate it
   */
  rv = RestartPlaybin();
  NS_ENSURE_SUCCESS(rv, rv);

  // XXX: What are these glib strings?
  g_object_set(G_OBJECT(mPlay), "uri", NS_ConvertUTF16toUTF8(aUri).get(), NULL);

  mArtist.Assign(EmptyString());
  mAlbum.Assign(EmptyString());
  mTitle.Assign(EmptyString());
  mGenre.Assign(EmptyString());

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::GetVolume(double* aVolume)
{
  NS_ENSURE_ARG_POINTER(aVolume);
  TRACE(("sbGStreamerSimple[0x%.8x] - GetVolume", this));

  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  g_object_get(G_OBJECT(mPlay), "volume", aVolume, NULL);

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::GetFullscreen(PRBool* aFullscreen)
{
  NS_ENSURE_ARG_POINTER(aFullscreen);
  TRACE(("sbGStreamerSimple[0x%.8x] - GetFullscreen", this));

  if (mPlatformInterface) {
    bool fullscreen = mPlatformInterface->GetFullscreen();

    *aFullscreen = fullscreen;
  }
  else {
    // Without a video manager we don't support fullscreen mode
    *aFullscreen = PR_FALSE;
  } 

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::SetFullscreen(PRBool aFullscreen)
{
  TRACE(("sbGStreamerSimple[0x%.8x] - SetFullscreen", this));
  if (mPlatformInterface) {
    mPlatformInterface->SetFullscreen(aFullscreen);
    // Make sure it's drawn in the right location after this.
    Resize();

    return NS_OK;
  }

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbGStreamerSimple::SetVolume(double aVolume)
{
  TRACE(("sbGStreamerSimple[0x%.8x] - SetVolume", this));

  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  g_object_set(mPlay, "volume", aVolume, NULL);
  mLastVolume = aVolume;

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::GetPosition(PRUint64* aPosition)
{
  NS_ENSURE_ARG_POINTER(aPosition);
  TRACE(("sbGStreamerSimple[0x%.8x] - GetPosition", this));

  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  nsresult rv;

  GstQuery *query;
  gboolean res;
  query = gst_query_new_position(GST_FORMAT_TIME);
  res = gst_element_query(mPlay, query);
  if(res) {
    gint64 position;
    gst_query_parse_position(query, NULL, &position);
    *aPosition = position;
    rv = NS_OK;
  }
  else {
    rv = NS_ERROR_NOT_AVAILABLE;
  }

  gst_query_unref (query);

  return rv;
}

NS_IMETHODIMP
sbGStreamerSimple::GetIsPlaying(PRBool* aIsPlaying)
{
  NS_ENSURE_ARG_POINTER(aIsPlaying);
  TRACE(("sbGStreamerSimple[0x%.8x] - GetIsPlaying", this));

  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  GstState cur, pending;

  gst_element_get_state(mPlay, &cur, &pending, 0);

  if(cur == GST_STATE_PLAYING || pending == GST_STATE_PLAYING) {
    *aIsPlaying = PR_TRUE;
  }
  else {
    *aIsPlaying = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::GetIsPlayingVideo(PRBool* aIsPlayingVideo)
{
  NS_ENSURE_ARG_POINTER(aIsPlayingVideo);
  TRACE(("sbGStreamerSimple[0x%.8x] - GetIsPlayingVideo", this));

  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  *aIsPlayingVideo = mIsPlayingVideo;

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::GetIsPaused(PRBool* aIsPaused)
{
  NS_ENSURE_ARG_POINTER(aIsPaused);
  TRACE(("sbGStreamerSimple[0x%.8x] - GetIsPaused", this));

  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  GstState cur, pending;

  gst_element_get_state(mPlay, &cur, &pending, 0);

  if(cur == GST_STATE_PAUSED || pending == GST_STATE_PAUSED) {
    *aIsPaused = PR_TRUE;
  }
  else {
    *aIsPaused = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::GetBufferingPercent(PRUint16* aBufferingPercent)
{
  NS_ENSURE_ARG_POINTER(aBufferingPercent);
  TRACE(("sbGStreamerSimple[0x%.8x] - aBufferingPercent", this));

  *aBufferingPercent = mBufferingPercent;

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::GetStreamLength(PRUint64* aStreamLength)
{
  NS_ENSURE_ARG_POINTER(aStreamLength);
  TRACE(("sbGStreamerSimple[0x%.8x] - GetStreamLength", this));

  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  nsresult rv;

  GstQuery *query;
  gboolean res;
  query = gst_query_new_duration(GST_FORMAT_TIME);
  res = gst_element_query(mPlay, query);
  if(res) {
    gint64 duration;
    gst_query_parse_duration(query, NULL, &duration);
    *aStreamLength = duration;
    rv = NS_OK;
  }
  else {
    rv = NS_ERROR_NOT_AVAILABLE;
  }

  gst_query_unref (query);

  return rv;
}

NS_IMETHODIMP
sbGStreamerSimple::GetIsAtEndOfStream(PRBool* aIsAtEndOfStream)
{
  NS_ENSURE_ARG_POINTER(aIsAtEndOfStream);
  TRACE(("sbGStreamerSimple[0x%.8x] - GetIsAtEndOfStream", this));

  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  *aIsAtEndOfStream = mIsAtEndOfStream;

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::GetLastErrorCode(PRInt32* aLastErrorCode)
{
  NS_ENSURE_ARG_POINTER(aLastErrorCode);
  TRACE(("sbGStreamerSimple[0x%.8x] - GetLastErrorCode", this));

  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  *aLastErrorCode = mLastErrorCode;

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::GetArtist(nsAString& aArtist)
{
  TRACE(("sbGStreamerSimple[0x%.8x] - GetArtist", this));

  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  aArtist.Assign(mArtist);

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::GetAlbum(nsAString& aAlbum)
{
  TRACE(("sbGStreamerSimple[0x%.8x] - GetAlbum", this));

  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  aAlbum.Assign(mAlbum);

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::GetTitle(nsAString& aTitle)
{
  TRACE(("sbGStreamerSimple[0x%.8x] - GetTitle", this));

  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  aTitle.Assign(mTitle);

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::GetGenre(nsAString& aGenre)
{
  TRACE(("sbGStreamerSimple[0x%.8x] - GetGenre", this));

  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  aGenre.Assign(mGenre);

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::Play()
{
  TRACE(("sbGStreamerSimple[0x%.8x] - Play", this));

  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  // If we were paused, don't reset the is playing video state
  PRBool isPaused = PR_FALSE;
  GetIsPaused(&isPaused);
  if(!isPaused) {
    mIsPlayingVideo = PR_FALSE;
  }

  gst_element_set_state(mPlay, GST_STATE_PLAYING);
  mIsAtEndOfStream = PR_FALSE;
  mLastErrorCode = 0;

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::Pause()
{
  TRACE(("sbGStreamerSimple[0x%.8x] - Pause", this));

  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  gst_element_set_state(mPlay, GST_STATE_PAUSED);

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::Stop()
{
  TRACE(("sbGStreamerSimple[0x%.8x] - Stop", this));

  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  gst_element_set_state(mPlay, GST_STATE_NULL);
  mIsAtEndOfStream = PR_TRUE;
  mIsPlayingVideo = PR_FALSE;

  SetFullscreen(FALSE);

  mLastErrorCode = 0;
  mBufferingPercent = 0;

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::Seek(PRUint64 aTimeNanos)
{
  TRACE(("sbGStreamerSimple[0x%.8x] - Seek", this));

  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  gst_element_seek(mPlay, 1.0,
      GST_FORMAT_TIME,
      GstSeekFlags(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
      GST_SEEK_TYPE_SET, aTimeNanos,
      GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);

  return NS_OK;
}

nsresult
sbGStreamerSimple::RestartPlaybin()
{
  TRACE(("sbGStreamerSimple[0x%.8x] - RestartPlaybin", this));

  nsresult rv;

  LOG(("Restarting playbin..."));

  /*
   * Destroy and recreate the playbin, then restore the volume
   */

  rv = DestroyPlaybin();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetupPlaybin();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetVolume(mLastVolume);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbGStreamerSimple::Notify(nsITimer *aTimer)
{
  NS_ENSURE_ARG_POINTER(aTimer);

  // Not used, currently.

  return NS_OK;
}

// nsIDOMEventListener
NS_IMETHODIMP
sbGStreamerSimple::HandleEvent(nsIDOMEvent* aEvent)
{
  nsAutoString eventType;
  aEvent->GetType(eventType);

  if(eventType.EqualsLiteral("unload")) {

    // Clean up here
    nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(mDomWindow));
    NS_ENSURE_TRUE(target, NS_NOINTERFACE);
    target->RemoveEventListener(NS_LITERAL_STRING("resize"), this, PR_FALSE);
    target->RemoveEventListener(NS_LITERAL_STRING("unload"), this, PR_FALSE);

    mDomWindow = nsnull;
    mInitialized = PR_FALSE;

    return NS_OK;
  }
  else {
    // All the other events we're listening for we need to call Resize()
    // for, so we don't bother checking the event type
    return Resize();
  }

}

nsresult
sbGStreamerSimple::Resize()
{
  if (mPlatformInterface) {
    mPlatformInterface->ResizeToWindow();
    return NS_OK;
  }
  else
    return NS_ERROR_NOT_IMPLEMENTED;
}

// Set our video window. Called when the video sink requires a window to draw
// on.
// Note: Called from a GStreamer streaming thread.
void
sbGStreamerSimple::PrepareVideoWindow(GstMessage *msg)
{
  if (mPlatformInterface) {
    // TODO: Figure out an appropriately generic interface here; is this 
    // sufficient?
    mPlatformInterface->PrepareVideoWindow();
  }

  mIsPlayingVideo = PR_TRUE;
}

// Callback for async messages (all normal GstBus messages */
void sbGStreamerSimple::HandleMessage (GstMessage *message)
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
    case GST_MESSAGE_BUFFERING:
      HandleBufferingMessage(message);
      break;
    case GST_MESSAGE_EOS:
      HandleEOSMessage(message);
      break;
    case GST_MESSAGE_TAG:
      HandleTagMessage(message);
      break;
    default:
      LOG(("Got message: %s", gst_message_type_get_name(msg_type)));
      break;
  }
}

void sbGStreamerSimple::HandleErrorMessage(GstMessage *message)
{
  GError *error = NULL;
  gchar *debug = NULL;

  gst_message_parse_error(message, &error, &debug);

  LOG(("Error message: %s [%s]", GST_STR_NULL (error->message), 
              GST_STR_NULL (debug)));

  g_free(debug);

  mBufferingPercent = 0;
  mIsPlayingVideo = PR_FALSE;
  SetFullscreen (PR_FALSE);

  nsCOMPtr<sbIGStreamerEvent> event = new sbGStreamerErrorEvent(error);

  for (PRInt32 i = 0; i < mListeners.Count(); i++) {
    LOG(("Dispatching error event to listener"));
    nsresult rv = mListeners[i]->OnGStreamerEvent (event);
    if (NS_FAILED (rv)) {
      NS_WARNING("GStreamer event listener returned error");
    }
  }

  // Set this _after_ the listeners are done, not before. Allows the
  // listeners to do useful things before we try to move onto the next
  // URL.
  mIsAtEndOfStream = PR_TRUE;
}

void sbGStreamerSimple::HandleWarningMessage(GstMessage *message)
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

void sbGStreamerSimple::HandleEOSMessage(GstMessage *message)
{
  mIsAtEndOfStream = PR_TRUE;
  mIsPlayingVideo = PR_FALSE;
  mBufferingPercent = 0;
  SetFullscreen(PR_FALSE);
}

void sbGStreamerSimple::HandleStateChangeMessage(GstMessage *message)
{
  GstState old_state, new_state;
  gchar *src_name;

  gst_message_parse_state_changed(message, &old_state, &new_state, NULL);

  src_name = gst_object_get_name(message->src);
  LOG(("stage-changed: %s changed state from %s to %s", src_name,
      gst_element_state_get_name (old_state),
      gst_element_state_get_name (new_state)));
  g_free (src_name);
}

/* TODO: We should use more of the available tag types */
void sbGStreamerSimple::HandleTagMessage(GstMessage *message)
{
  GstTagList *tag_list;
  gchar *value = NULL;

  gst_message_parse_tag(message, &tag_list);

  if(gst_tag_list_get_string(tag_list, GST_TAG_ARTIST, &value)) {
    CopyUTF8toUTF16(nsDependentCString(value), mArtist);
    g_free(value);
  }

  if(gst_tag_list_get_string(tag_list, GST_TAG_ALBUM, &value)) {
    CopyUTF8toUTF16(nsDependentCString(value), mAlbum);
    g_free(value);
  }

  if(gst_tag_list_get_string(tag_list, GST_TAG_TITLE, &value)) {
    CopyUTF8toUTF16(nsDependentCString(value), mTitle);
    g_free(value);
  }

  if(gst_tag_list_get_string(tag_list, GST_TAG_GENRE, &value)) {
    CopyUTF8toUTF16(nsDependentCString(value), mGenre);
    g_free(value);
  }

  gst_tag_list_free(tag_list);
}

/* TODO: We should pause when we receive a buffering message for a percentage
 * less than 100, and go to playing when we receive a 100% message,
 * so that we can re-buffer appropriately if we underrun
 */
void sbGStreamerSimple::HandleBufferingMessage(GstMessage *message)
{
  gint percent = 0;
  gst_structure_get_int(message->structure, "buffer-percent", &percent);
  mBufferingPercent = percent;
  TRACE(("Buffering (%u percent done)", percent));
}

void
sbGStreamerSimple::OnStreamInfoSet()
{
  GList *streaminfo = NULL;
  GstPad *videopad = NULL;
 
  g_object_get (mPlay, "stream-info", &streaminfo, NULL);
  streaminfo = g_list_copy(streaminfo);
  g_list_foreach (streaminfo, (GFunc) g_object_ref, NULL);
  for( ; streaminfo != NULL; streaminfo = streaminfo->next) {
    GObject *info = (GObject*) streaminfo->data;
    gint type;
    GParamSpec *pspec;
    GEnumValue *val;
    if(!info) {
      continue;
    }

    g_object_get (info, "type", &type, NULL);
    pspec = g_object_class_find_property(G_OBJECT_GET_CLASS (info), "type");
    val = g_enum_get_value(G_PARAM_SPEC_ENUM (pspec)->enum_class, type);

    if(!g_strcasecmp(val->value_nick, "video")) {
      if (!videopad) {
        g_object_get(info, "object", &videopad, NULL);
      }
    }
  }

  if(videopad) {
    GstCaps *caps;

    caps = gst_pad_get_negotiated_caps(videopad);
    if (caps) {
      OnVideoCapsSet (caps);
      gst_caps_unref(caps);
    }

    g_signal_connect(videopad, "notify::caps", 
            G_CALLBACK(videoCapsSetHelper), this);
  }
  g_list_foreach(streaminfo, (GFunc) g_object_unref, NULL);
  g_list_free(streaminfo);
}

/* TODO: Add any neccessary locking; this is called from a streaming thread */
void
sbGStreamerSimple::OnVideoCapsSet(GstCaps *caps)
{
  GstStructure *s;

  s = gst_caps_get_structure(caps, 0);
  if(s) {
    gst_structure_get_int(s, "width", &mVideoWidth);
    gst_structure_get_int(s, "height", &mVideoHeight);

    /* pixel-aspect-ratio is optional */
    const GValue* par = gst_structure_get_value(s, "pixel-aspect-ratio");
    if (par) {
      mPixelAspectRatioN = gst_value_get_fraction_numerator(par);
      mPixelAspectRatioD = gst_value_get_fraction_denominator(par);
    }
    else {
      /* PAR not set; default to square pixels */
      mPixelAspectRatioN = mPixelAspectRatioD = 1;
    }

    if (mPlatformInterface) {
      int num = mVideoWidth * mPixelAspectRatioN;
      int denom = mVideoHeight * mPixelAspectRatioD;
      mPlatformInterface->SetDisplayAspectRatio(num, denom);
    }
  }
}

NS_IMETHODIMP sbGStreamerSimple::AddEventListener(sbIGStreamerEventListener *aListener)
{
  PRInt32 index = mListeners.IndexOf(aListener);
  if (index >= 0) {
    // Listener has already been added, can't re-add.
    return NS_ERROR_FAILURE;
  }

  PRBool added = mListeners.AppendObject(aListener);
  if (added)
    return NS_OK;
  else
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP sbGStreamerSimple::RemoveEventListener(sbIGStreamerEventListener *aListener)
{
  PRInt32 index = mListeners.IndexOf(aListener);
  if(index < 0) {
    // No such listener.
    return NS_ERROR_FAILURE;
  }

  PRBool removed = mListeners.RemoveObjectAt(index);
  if (removed)
    return NS_OK;
  else
    return NS_ERROR_FAILURE;
}
