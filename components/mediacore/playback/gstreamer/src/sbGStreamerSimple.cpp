#include "sbGStreamerSimple.h"
#include "nsIInterfaceRequestorUtils.h"

#include "nsIBaseWindow.h"
#include "nsIBoxObject.h"
#include "nsIBrowserDOMWindow.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDocument.h"
#include "nsIDOMAbstractView.h"
#include "nsIDOMChromeWindow.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentView.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMWindow.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMXULElement.h"
#include "nsIIOService.h"
#include "nsIPrefBranch.h"
#include "nsIPromptService.h"
#include "nsIProxyObjectManager.h"
#include "nsIScriptGlobalObject.h"
#include "nsIWebNavigation.h"
#include "nsIWidget.h"
#include "nsIWindowMediator.h"
#include "nsServiceManagerUtils.h"
#include "nsStringGlue.h"
#include "prlog.h"

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=gstreamer:5
 */
#ifdef PR_LOGGING
extern PRLogModuleInfo* gGStreamerLog;
#endif /* PR_LOGGING */

#define TRACE(args) PR_LOG(gGStreamerLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gGStreamerLog, PR_LOG_WARN, args)

NS_IMPL_THREADSAFE_ISUPPORTS3(sbGStreamerSimple,
                              sbIGStreamerSimple,
                              nsIDOMEventListener,
                              nsITimerCallback)

static GstBusSyncReply
syncHandlerHelper(GstBus* bus, GstMessage* message, gpointer data)
{
  sbGStreamerSimple* gsts = static_cast<sbGStreamerSimple*>(data);
  return gsts->SyncHandler(bus, message);
}

static void
streamInfoSetHelper(GObject* obj, GParamSpec* pspec, sbGStreamerSimple* gsts)
{
  gsts->StreamInfoSet(obj, pspec);
}

static void
capsSetHelper(GObject* obj, GParamSpec* pspec, sbGStreamerSimple* gsts)
{
  gsts->CapsSet(obj, pspec);
}

sbGStreamerSimple::sbGStreamerSimple() :
  mInitialized(PR_FALSE),
  mPlay(NULL),
  mBus(NULL),
  mPixelAspectRatioN(1),
  mPixelAspectRatioD(1),
  mVideoWidth(0),
  mVideoHeight(0),
  mVideoSink(NULL),
  mGdkWin(NULL),
  mNativeWin(NULL),
  mGdkWinFull(NULL),
  mIsAtEndOfStream(PR_TRUE),
  mIsPlayingVideo(PR_FALSE),
  mFullscreen(PR_FALSE),
  mLastErrorCode(0),
  mBufferingPercent(0),
  mLastVolume(0),
  mVideoOutputElement(nsnull),
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
  mVideoOutputElement = nsnull;
}

NS_IMETHODIMP
sbGStreamerSimple::Init(nsIDOMXULElement* aVideoOutput)
{
  NS_ENSURE_ARG_POINTER(aVideoOutput);
  TRACE(("sbGStreamerSimple[0x%.8x] - Init", this));

  nsresult rv;

  if(mInitialized) {
    return NS_OK;
  }

  nsCOMPtr<nsIProxyObjectManager> proxyObjMgr = 
        do_GetService("@mozilla.org/xpcomproxy;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsITimer> timer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = proxyObjMgr->GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                            NS_GET_IID(nsITimer),
                            timer,
                            NS_PROXY_ASYNC,
                            getter_AddRefs(mCursorIntervalTimer));
  NS_ENSURE_SUCCESS(rv, rv);

  mVideoOutputElement = aVideoOutput;

  // Get a handle to the xul element's native window
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

  mNativeWin = GDK_WINDOW(widget->GetNativeData(NS_NATIVE_WIDGET));

  LOG(("Found native window %x", mNativeWin));

  // Create the video window
  GdkWindowAttr attributes;

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = 0;
  attributes.y = 0;
  attributes.width = 0;
  attributes.height = 0;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = 0;

  mGdkWin = gdk_window_new(mNativeWin, &attributes, GDK_WA_X | GDK_WA_Y);
  mRedrawCursor = false;
  mOldCursorX = -1;
  mOldCursorY = -1;
  mDelayHide = 10;
  gdk_window_show(mGdkWin);

  rv = SetupPlaybin();
  NS_ENSURE_SUCCESS(rv, rv);

  mInitialized = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::SetupPlaybin()
{
  TRACE(("sbGStreamerSimple[0x%.8x] - SetupPlaybin", this));

  if (!mPlay) {
    mPlay = gst_element_factory_make("playbin", "play");
    GstElement *audioSink = gst_element_factory_make("gconfaudiosink",
                                                     "audio-sink");
    g_object_set(mPlay, "audio-sink", audioSink, NULL);
 
    mVideoSink = gst_element_factory_make("gconfvideosink", "video-sink");
    if (!mVideoSink) {
        mVideoSink = gst_element_factory_make("ximagesink", "video-sink");
    }

    g_object_set(mPlay, "video-sink", mVideoSink, NULL);

    mBus = gst_element_get_bus(mPlay);
    gst_bus_set_sync_handler(mBus, &syncHandlerHelper, this);

    // This signal lets us get info about the stream
    g_signal_connect(mPlay, "notify::stream-info",
      G_CALLBACK(streamInfoSetHelper), this);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::DestroyPlaybin()
{
  TRACE(("sbGStreamerSimple[0x%.8x] - DestroyPlaybin", this));

  if (mBus) {
    gst_bus_set_flushing(mBus, TRUE);
    mBus = NULL;
  }

  if (mPlay  && GST_IS_ELEMENT(mPlay)) {
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
  TRACE(("sbGStreamerSimple[0x%.8x] - SetUri", this));

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

void
sbGStreamerSimple::ReparentToRootWin(sbGStreamerSimple* gsts)
{
  NS_ASSERTION(gsts, "gsts is null");

  GdkScreen *fullScreen = NULL;
  GdkWindowAttr attributes;
  gint fullWidth, fullHeight;
  attributes.window_type = GDK_WINDOW_TOPLEVEL;
  attributes.x = 0;
  attributes.y = 0;
  attributes.width = 0;
  attributes.height = 0;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = 0;

  gsts->mGdkWinFull = gdk_window_new(NULL, &attributes, GDK_WA_X | GDK_WA_Y);
  gdk_window_show(gsts->mGdkWinFull);
  gdk_window_reparent(gsts->mGdkWin, gsts->mGdkWinFull, 0, 0);
  gdk_window_fullscreen(gsts->mGdkWinFull);

  // might be needed for fallback - 
  //fullScreen = gdk_display_get_screen(gdk_x11_lookup_xdisplay(
  //  GDK_WINDOW_XDISPLAY(gsts->mGdkWinFull)), 0);
  fullScreen = gdk_screen_get_default();

  fullWidth = gdk_screen_get_width(fullScreen);
  fullHeight = gdk_screen_get_height(fullScreen);
  gdk_window_move_resize(gsts->mGdkWin, 0, 0, fullWidth, fullHeight);
  gsts->Resize();
  gsts->mFullscreen = PR_TRUE;
  return;
}

void
sbGStreamerSimple::ReparentToChromeWin(sbGStreamerSimple* gsts)
{
  NS_ASSERTION(gsts, "gsts is null");

  gdk_window_unfullscreen(gsts->mGdkWin);
  gdk_window_reparent(gsts->mGdkWin, gsts->mNativeWin, 0, 0);
  gsts->Resize();
  gdk_window_destroy(gsts->mGdkWinFull);
  gsts->mGdkWinFull = NULL;
  return;
}

NS_IMETHODIMP
sbGStreamerSimple::GetFullscreen(PRBool* aFullscreen)
{
  NS_ENSURE_ARG_POINTER(aFullscreen);
  TRACE(("sbGStreamerSimple[0x%.8x] - GetFullscreen", this));

  *aFullscreen = mFullscreen;

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::SetFullscreen(PRBool aFullscreen)
{
  TRACE(("sbGStreamerSimple[0x%.8x] - SetFullscreen", this));

  mFullscreen = aFullscreen;
  if(!mFullscreen && mGdkWinFull != NULL) ReparentToChromeWin(this);

  return NS_OK;
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
  if(mFullscreen && mGdkWinFull != NULL) {
    ReparentToChromeWin(this);
  }
  mCursorIntervalTimer->Cancel();
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

NS_IMETHODIMP
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

bool 
sbGStreamerSimple::SetInvisibleCursor(sbGStreamerSimple* gsts)
{
  guint32 data = 0;
  GdkPixmap* pixmap = gdk_bitmap_create_from_data(NULL, (gchar*)&data, 1, 1);
  GdkColor color = { 0, 0, 0, 0 };
  GdkCursor* cursor = gdk_cursor_new_from_pixmap(pixmap, 
          pixmap, &color, &color, 0, 0);
  gdk_pixmap_unref(pixmap);
  gdk_window_set_cursor(gsts->mGdkWin, cursor);
  gdk_window_set_cursor(gsts->mNativeWin, cursor);
  if(gsts->mGdkWinFull != NULL )
    gdk_window_set_cursor(gsts->mGdkWinFull, cursor);
  gdk_cursor_unref(cursor);
  return true;
}

bool 
sbGStreamerSimple::SetDefaultCursor(sbGStreamerSimple* gsts) 
{
  gdk_window_set_cursor(gsts->mGdkWin, NULL);
  gdk_window_set_cursor(gsts->mNativeWin, NULL);
  if(gsts->mGdkWinFull != NULL )
    gdk_window_set_cursor(gsts->mGdkWinFull, NULL);
  return false;
}

//timeout function for determining when to hide/show mouse hide/show controls
NS_IMETHODIMP 
sbGStreamerSimple::Notify(nsITimer *aTimer)
{
  NS_ENSURE_ARG_POINTER(aTimer);

  GdkDisplay *gdkDisplay;
  GdkWindow *gdkWin = this->mGdkWin;
  gint newCursorX=-1;
  gint newCursorY=-1;

  if (gdkWin == NULL) 
    return NS_OK;

  gdkDisplay = gdk_x11_lookup_xdisplay(GDK_WINDOW_XDISPLAY(gdkWin));

  gdk_display_get_pointer(gdkDisplay, NULL, &newCursorX, &newCursorY, NULL);
  if (newCursorX != this->mOldCursorX || 
      newCursorY != this->mOldCursorY) {
    // redraw cursor if mouse is invisible and the mouse has moved
    if (this->mRedrawCursor) {
      this->mRedrawCursor = SetDefaultCursor(this);
      if(this->mFullscreen) ReparentToChromeWin(this);
    }
    this->mDelayHide = 10;
    this->mOldCursorX = newCursorX;
    this->mOldCursorY = newCursorY;
  }
  else {
    if (this->mDelayHide > 0) {
      this->mDelayHide-=1;
    }
    else {
      // otherwise hide the cursor in the video window
      this->mRedrawCursor = SetInvisibleCursor(this);
      if (this->mGdkWinFull == NULL && this->mFullscreen) {
        ReparentToRootWin(this);
      }
    }
  }
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
    mVideoOutputElement = nsnull;

    mInitialized = PR_FALSE;

    return NS_OK;
  }
  else {
    return Resize();
  }

}

NS_IMETHODIMP
sbGStreamerSimple::Resize()
{
  PRInt32 x, y, width, height;
  nsCOMPtr<nsIBoxObject> boxObject;
  mVideoOutputElement->GetBoxObject(getter_AddRefs(boxObject));
  boxObject->GetX(&x);
  boxObject->GetY(&y);
  boxObject->GetWidth(&width);
  boxObject->GetHeight(&height);

  PRInt32 newX, newY, newWidth, newHeight;

  // If we have not received a video size yet, size the video window to the
  // size of the video output element
  if(mVideoWidth == 0 && mVideoHeight == 0) {
    gdk_window_move_resize(mGdkWin, x, y, width, height);
  }
  else {
    if(mVideoWidth > 0 && mVideoHeight > 0) {

      float ratioWidth  = (float) width  / (float) mVideoWidth;
      float ratioHeight = (float) height / (float) mVideoHeight;
      if(ratioWidth < ratioHeight) {
        newWidth  = PRInt32(mVideoWidth  * ratioWidth);
        newHeight = PRInt32(mVideoHeight * ratioWidth);
        newX = x;
        newY = ((height - newHeight) / 2) + y;
      }
      else {
        newWidth  = PRInt32(mVideoWidth  * ratioHeight);
        newHeight = PRInt32(mVideoHeight * ratioHeight);
        newX = ((width - newWidth) / 2) + x;
        newY = y;
      }

      gdk_window_move_resize(mGdkWin, newX, newY, newWidth, newHeight);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::CreateBundle(const char *aURLSpec,
                                nsIStringBundle **_retval)
{
  NS_ASSERTION(aURLSpec, "aURLSpec is null");
  NS_ASSERTION(_retval, "_retval is null");
  *_retval = nsnull;

  nsresult rv;
  nsCOMPtr<nsIStringBundleService> stringBundleService =
    do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIStringBundle> stringBundle;
  rv = stringBundleService->CreateBundle(aURLSpec,
                                         getter_AddRefs(stringBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = stringBundle);
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::GetStringFromName(nsIStringBundle *aBundle,
                                     const nsAString & aName,
                                     nsAString & _retval)
{
  NS_ASSERTION(aBundle, "aBundle is null");

  nsAutoString value;
  nsresult rv = aBundle->GetStringFromName(aName.BeginReading(),
                                           getter_Copies(value));
  if (NS_SUCCEEDED(rv)) {
    _retval.Assign(value);
  }
  else {
    _retval.Truncate();
  }

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::ShowHelperPage(void)
{
  nsresult rv;
  PRBool skipNotify = PR_TRUE;
  nsCOMPtr<nsIPrefBranch> prefService =
         do_GetService( "@mozilla.org/preferences-service;1", &rv );
  NS_ENSURE_SUCCESS( rv, rv);
  prefService->GetBoolPref( "songbird.skipGStreamerHelp", &skipNotify );

  // Abort asking the user if the pref says to skip
  if (skipNotify == PR_TRUE) {
    return NS_OK;
  }

  nsCOMPtr<nsIPromptService> promptService =
    do_GetService("@mozilla.org/embedcomp/prompt-service;1", &rv);
  NS_ENSURE_SUCCESS( rv, rv);

  // If possible we want the alert to pop off of the 
  // main window and not the cheezy video window
  nsCOMPtr<nsIWindowMediator> windowMediator =
    do_GetService("@mozilla.org/appshell/window-mediator;1", &rv);
  NS_ENSURE_SUCCESS( rv, rv);
  nsCOMPtr<nsIDOMWindowInternal> mainWindow;  
  windowMediator->GetMostRecentWindow(NS_LITERAL_STRING("Songbird:Main").get(),
                                      getter_AddRefs(mainWindow));

  // Get the localized strings using the helper functions above
  nsCOMPtr<nsIStringBundle> stringBundle;
  rv = CreateBundle("chrome://songbird/locale/songbird.properties",
                    getter_AddRefs(stringBundle));
  NS_ENSURE_SUCCESS( rv, rv);

  nsAutoString windowTitle;
  nsAutoString windowText;
  nsAutoString helpURL;
  nsAutoString checkLabel;
  rv = GetStringFromName(stringBundle,
                         NS_LITERAL_STRING("mediacore.gstreamer.plugin.missing.title"),
                         windowTitle);
  NS_ENSURE_SUCCESS( rv, rv);
  rv = GetStringFromName(stringBundle,
                         NS_LITERAL_STRING("mediacore.gstreamer.plugin.missing.text"),
                         windowText);
  NS_ENSURE_SUCCESS( rv, rv);
  rv = GetStringFromName(stringBundle,
                         NS_LITERAL_STRING("mediacore.gstreamer.plugin.helpUrl"),
                         helpURL);
  NS_ENSURE_SUCCESS( rv, rv);
  rv = GetStringFromName(stringBundle,
                         NS_LITERAL_STRING("mediacorecheck.dialog.skipCheckboxLabel"),
                         checkLabel);
  NS_ENSURE_SUCCESS( rv, rv);

  // Ask the user if they would like to learn how to install the plug-ins
  // Get flags to indicate yes/no option dialog
  // Note: buttons need to be os dependent for position. (since this is currently
  //       linux only we can assume Yes/No)
  PRUint32 flags;
  flags = nsIPromptService::BUTTON_TITLE_YES * nsIPromptService::BUTTON_POS_0 +
          nsIPromptService::BUTTON_TITLE_NO * nsIPromptService::BUTTON_POS_1;

  PRInt32 promptResult = 1;
  PRBool checkState = PR_FALSE;
  rv = promptService->ConfirmEx(mainWindow, // Window to bind to
        windowTitle.get(),                  // Title of window
        windowText.get(),                   // Text message
        flags, NULL, NULL, NULL,            // button flags and titles (use default)
        checkLabel.get(),                   // Check box label
        &checkState,                        // State of checkbox
        &promptResult);                     // Button Clicked
  NS_ENSURE_SUCCESS( rv, rv);

  // Set our pref to what the check box state is
  prefService->SetBoolPref( "songbird.skipGStreamerHelp", checkState );
  
  if (promptResult == 0) { // They clicked Yes
    // Load up the url to show the user how to configure
    // First we need to get the url from preferences
    nsCOMPtr<nsIIOService> ioService =
      do_GetService("@mozilla.org/network/io-service;1", &rv);
    NS_ENSURE_SUCCESS( rv, rv);

    nsCOMPtr<nsIURI> uri;
    NS_ConvertUTF16toUTF8 cstrURL(helpURL);
    rv = ioService->NewURI(
      cstrURL,
      NULL, NULL, getter_AddRefs(uri));
    NS_ENSURE_SUCCESS( rv, rv);
    
    // Get the browser context so we can open a new tab with OpenURI,
    // this is very long and confusing since we are not in the dom.
    nsCOMPtr<nsIBrowserDOMWindow> bwin;
    
    nsCOMPtr<nsIWebNavigation> navNav(do_GetInterface(mainWindow, &rv));
    NS_ENSURE_SUCCESS( rv, rv);
    nsCOMPtr<nsIDocShellTreeItem> navItem(do_QueryInterface(navNav, &rv));
    NS_ENSURE_SUCCESS( rv, rv);
    if (navItem) {
      nsCOMPtr<nsIDocShellTreeItem> rootItem;
      rv = navItem->GetRootTreeItem(getter_AddRefs(rootItem));
      NS_ENSURE_SUCCESS( rv, rv);
      nsCOMPtr<nsIDOMWindow> rootWin(do_GetInterface(rootItem, &rv));
      NS_ENSURE_SUCCESS( rv, rv);
      nsCOMPtr<nsIDOMChromeWindow> chromeWin(do_QueryInterface(rootWin, &rv));
      NS_ENSURE_SUCCESS( rv, rv);
      if (chromeWin) {
        rv = chromeWin->GetBrowserDOMWindow(getter_AddRefs(bwin));
        NS_ENSURE_SUCCESS( rv, rv);
      }
    }
    
    if (bwin && uri) {
      nsCOMPtr<nsIDOMWindow> container;
      rv = bwin->OpenURI(uri, 0,
                         nsIBrowserDOMWindow::OPEN_NEWTAB,
                         nsIBrowserDOMWindow::OPEN_EXTERNAL,
                         getter_AddRefs(container));
      NS_ENSURE_SUCCESS( rv, rv);
    }
  }
  
  return NS_OK;
}

// Callbacks
GstBusSyncReply
sbGStreamerSimple::SyncHandler(GstBus* bus, GstMessage* message)
{
  GstMessageType msg_type;
  msg_type = GST_MESSAGE_TYPE(message);

  switch (msg_type) {
    case GST_MESSAGE_ERROR: {
      GError *error = NULL;
      gchar *debug = NULL;
      PRBool showMessage = false;

      gst_message_parse_error(message, &error, &debug);

      LOG(("Error message: %s [%s]", GST_STR_NULL (error->message), GST_STR_NULL (debug)));

      g_free (debug);

      showMessage = ( (mLastErrorCode == 0) && (error->code != 0) );
      mLastErrorCode = error->code;
      mIsAtEndOfStream = PR_TRUE;
      mBufferingPercent = 0;
      mIsPlayingVideo = PR_FALSE;
      if(mFullscreen && mGdkWinFull != NULL) {
        ReparentToChromeWin(this);
      }
      mCursorIntervalTimer->Cancel();

      if (showMessage) {
        // If we fail to show the dialog to the user then we just return
        // the GST_BUS_PASS success code so that the GST stuff will keep
        // going and not worry about the dialog this time.
        nsresult rv;
        nsCOMPtr<nsIProxyObjectManager> proxyObjMgr =
          do_GetService("@mozilla.org/xpcomproxy;1", &rv);
        NS_ENSURE_SUCCESS(rv, GST_BUS_PASS);
  
        nsCOMPtr<sbIGStreamerSimple> proxy;
        rv = proxyObjMgr->GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                                            NS_GET_IID(sbIGStreamerSimple),
                                            NS_ISUPPORTS_CAST(sbIGStreamerSimple*, this),
                                            NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                                            getter_AddRefs(proxy));
        NS_ENSURE_SUCCESS(rv, GST_BUS_PASS); 
  
        rv = proxy->ShowHelperPage();
        NS_ENSURE_SUCCESS(rv, GST_BUS_PASS);
      }
      break;
    }
    case GST_MESSAGE_WARNING: {
      GError *error = NULL;
      gchar *debug = NULL;

      gst_message_parse_warning(message, &error, &debug);

      LOG(("Warning message: %s [%s]", GST_STR_NULL (error->message), GST_STR_NULL (debug)));

      g_warning ("%s [%s]", GST_STR_NULL (error->message), GST_STR_NULL (debug));

      g_error_free (error);
      g_free (debug);
      break;
    }

    case GST_MESSAGE_EOS: {
      mIsAtEndOfStream = PR_TRUE;
      mIsPlayingVideo = PR_FALSE;
      mBufferingPercent = 0;
      mCursorIntervalTimer->Cancel();
      if(mFullscreen && mGdkWinFull != NULL) {

        nsresult rv;
        nsCOMPtr<nsIProxyObjectManager> proxyObjMgr =
          do_GetService("@mozilla.org/xpcomproxy;1", &rv);
        NS_ENSURE_SUCCESS(rv, GST_BUS_PASS);

        nsCOMPtr<sbIGStreamerSimple> proxy;
        rv = proxyObjMgr->GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                                            NS_GET_IID(sbIGStreamerSimple),
                                            NS_ISUPPORTS_CAST(sbIGStreamerSimple*, this),
                                            NS_PROXY_ASYNC,
                                            getter_AddRefs(proxy));
        NS_ENSURE_SUCCESS(rv, GST_BUS_PASS); 

        proxy->SetFullscreen(PR_FALSE);
      }
      break;
    }

    case GST_MESSAGE_STATE_CHANGED: {
      GstState old_state, new_state;
      gchar *src_name;

      gst_message_parse_state_changed(message, &old_state, &new_state, NULL);

      src_name = gst_object_get_name(message->src);
      LOG(("stage-changed: %s changed state from %s to %s", src_name,
          gst_element_state_get_name (old_state),
          gst_element_state_get_name (new_state)));
      g_free (src_name);
      break;
    }

    case GST_MESSAGE_ELEMENT: {
      if(gst_structure_has_name(message->structure, "prepare-xwindow-id") && mVideoSink != NULL) {
        GstElement *element = NULL;
        GstXOverlay *xoverlay = NULL;

        if (GST_IS_BIN (mVideoSink)) {
          element = gst_bin_get_by_interface (GST_BIN (mVideoSink),
                                              GST_TYPE_X_OVERLAY);
        }
        else {
          element = mVideoSink;
        }

        if (GST_IS_X_OVERLAY (element)) {
          xoverlay = GST_X_OVERLAY (element);
        }
        else {
          xoverlay = NULL;
        }

        XID window = GDK_WINDOW_XWINDOW(mGdkWin);
        gst_x_overlay_set_xwindow_id(xoverlay, window);
        LOG(("Set xoverlay %d to windowid %d\n", xoverlay, window));

        mCursorIntervalTimer->InitWithCallback(this,
          300, nsITimer::TYPE_REPEATING_SLACK);

        mIsPlayingVideo = PR_TRUE;
      }
      break;
    }

    case GST_MESSAGE_TAG: {
      GstTagList *tag_list;
      gchar *value = NULL;
      nsCAutoString temp;

      gst_message_parse_tag(message, &tag_list);

      if(gst_tag_list_get_string(tag_list, GST_TAG_ARTIST, &value)) {
        temp.Assign(value);
        mArtist.Assign(NS_ConvertUTF8toUTF16(temp).get());
        g_free(value);
      }

      if(gst_tag_list_get_string(tag_list, GST_TAG_ALBUM, &value)) {
        temp.Assign(value);
        mAlbum.Assign(NS_ConvertUTF8toUTF16(temp).get());
        g_free(value);
      }

      if(gst_tag_list_get_string(tag_list, GST_TAG_TITLE, &value)) {
        temp.Assign(value);
        mTitle.Assign(NS_ConvertUTF8toUTF16(temp).get());
        g_free(value);
      }

      if(gst_tag_list_get_string(tag_list, GST_TAG_GENRE, &value)) {
        temp.Assign(value);
        mGenre.Assign(NS_ConvertUTF8toUTF16(temp).get());
        g_free(value);
      }

      gst_tag_list_free(tag_list);
      break;
    }

    case GST_MESSAGE_BUFFERING: {
      gint percent = 0;
      gst_structure_get_int (message->structure, "buffer-percent", &percent);
      mBufferingPercent = percent;
      TRACE(("Buffering (%u percent done)", percent));
      break;
    }

    default:
      LOG(("Got message: %s", gst_message_type_get_name(msg_type)));
  }

  //gst_message_unref(message); XXX: Do i need to unref this?

  return GST_BUS_PASS;
}

void
sbGStreamerSimple::StreamInfoSet(GObject* obj, GParamSpec* pspec)
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
    pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (info), "type");
    val = g_enum_get_value (G_PARAM_SPEC_ENUM (pspec)->enum_class, type);

    if(!g_strcasecmp (val->value_nick, "video")) {
      if (!videopad) {
        g_object_get(info, "object", &videopad, NULL);
      }
    }
  }

  if(videopad) {
    GstCaps *caps;

    if((caps = gst_pad_get_negotiated_caps(videopad))) {
      CapsSet(G_OBJECT(videopad), NULL);
      gst_caps_unref(caps);
    }
    g_signal_connect(videopad, "notify::caps", G_CALLBACK(capsSetHelper), this);
  }

  g_list_foreach (streaminfo, (GFunc) g_object_unref, NULL);
  g_list_free (streaminfo);
}

void
sbGStreamerSimple::CapsSet(GObject* obj, GParamSpec* pspec)
{
  GstPad *pad = GST_PAD(obj);
  GstStructure *s;
  GstCaps *caps;

  if(!(caps = gst_pad_get_negotiated_caps(pad))) {
    return;
  }

  s = gst_caps_get_structure(caps, 0);
  if(s) {
    gst_structure_get_int(s, "width", &mVideoWidth);
    gst_structure_get_int(s, "height", &mVideoHeight);

/* This is unused, and causing asserts
    const GValue* par = gst_structure_get_value(s, "pixel-aspect-ratio");
    mPixelAspectRatioN = gst_value_get_fraction_numerator(par);
    mPixelAspectRatioD = gst_value_get_fraction_denominator(par);
*/
  }

  gst_caps_unref(caps);
}
