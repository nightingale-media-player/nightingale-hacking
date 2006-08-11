#include "sbGStreamerSimple.h"
#include "nsIInterfaceRequestorUtils.h"

#include "nsIWebNavigation.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentView.h"
#include "nsIDOMXULElement.h"
#include "nsIDOMAbstractView.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIBaseWindow.h"
#include "nsIWidget.h"
#include "nsIBoxObject.h"
#include "nsString.h"
#include "prlog.h"

#if defined( PR_LOGGING )
extern PRLogModuleInfo* gGStreamerLog;
#define LOG(args) PR_LOG(gGStreamerLog, PR_LOG_DEBUG, args)
#else
#define LOG(args)
#endif

NS_IMPL_ISUPPORTS1(sbGStreamerSimple, sbIGStreamerSimple)

static GstBusSyncReply
syncHandlerHelper(GstBus* bus, GstMessage* message, gpointer data)
{
  sbGStreamerSimple* gsts = NS_STATIC_CAST(sbGStreamerSimple*, data);
  return gsts->SyncHandler(bus, message);
}

sbGStreamerSimple::sbGStreamerSimple() :
  mInitialized(PR_FALSE),
  mPlay(NULL),
  mBus(NULL),
  mVideoSink(NULL),
  mGdkWin(NULL),
  mIsAtEndOfStream(PR_TRUE)
{
}

sbGStreamerSimple::~sbGStreamerSimple()
{
  if(mPlay != NULL && GST_IS_ELEMENT(mPlay)) {
    gst_element_set_state(mPlay, GST_STATE_NULL);
    gst_object_unref(mPlay);
    mPlay = NULL;
  }

  // Do i need to close the video window?
}

NS_IMETHODIMP
sbGStreamerSimple::Init(nsIDOMXULElement* aVideoOutput)
{
  nsresult rv;

  if(mInitialized) {
    return NS_OK;
  }

  if(aVideoOutput == nsnull) {
    return NS_ERROR_INVALID_ARG;
  }

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

  nsCOMPtr<nsIBaseWindow> baseWindow(do_QueryInterface(docShellTreeOwner));
  NS_ENSURE_TRUE(docShellTreeItem, NS_NOINTERFACE);

  nsCOMPtr<nsIWidget> widget;
  rv = baseWindow->GetMainWidget(getter_AddRefs(widget));
  NS_ENSURE_SUCCESS(rv, rv);

  GdkWindow* win = GDK_WINDOW(widget->GetNativeData(NS_NATIVE_WIDGET));

  LOG(("Found native window %x", win));

  PRInt32 x, y;
  nsCOMPtr<nsIBoxObject> boxObject;
  aVideoOutput->GetBoxObject(getter_AddRefs(boxObject));
  boxObject->GetX(&x);
  boxObject->GetX(&y);

  // Create the video window
  // TODO: get window coords from element's box object
  GdkWindowAttr attributes;

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = x;
  attributes.y = y;
  attributes.width = 320;
  attributes.height = 240;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = 0;

  mGdkWin = gdk_window_new(win, &attributes, GDK_WA_X | GDK_WA_Y);
  gdk_window_show(mGdkWin);

  // Set up the playbin
  mPlay = gst_element_factory_make("playbin", "play");
  GstElement *audioSink = gst_element_factory_make("gconfaudiosink", "audio-sink");
  g_object_set(mPlay, "audio-sink", audioSink, NULL);

  // TODO: use gconf video config here
  mVideoSink = gst_element_factory_make("ximagesink", "video-sink");
  g_object_set(mPlay, "video-sink", mVideoSink, NULL);

  mBus = gst_element_get_bus(mPlay);
  gst_bus_set_sync_handler(mBus, &syncHandlerHelper, this);

  mInitialized = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::GetUri(nsAString& aUri)
{
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
  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  // XXX: What are these glib strings?
  g_object_set(G_OBJECT(mPlay), "uri", NS_ConvertUTF16toUTF8(aUri).get(), NULL);

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::GetVolume(double* aVolume)
{
  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  g_object_get(G_OBJECT(mPlay), "volume", aVolume, NULL);

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::SetVolume(double aVolume)
{
  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  g_object_set(mPlay, "volume", aVolume, NULL);

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::GetPosition(PRUint64* aPosition)
{
  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  nsresult rv;

  GstQuery *query;
  gboolean res;
  query = gst_query_new_position(GST_FORMAT_TIME);
  res = gst_element_query(mPlay, query);
  if (res) {
    gint64 position;
    gst_query_parse_position(query, NULL, &position);
    *aPosition = position;
    rv = NS_OK;
  }
  else {
    rv = NS_ERROR_FAILURE;
  }

  gst_query_unref (query);

  return rv;
}

NS_IMETHODIMP
sbGStreamerSimple::GetIsPlaying(PRBool* aIsPlaying)
{
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
sbGStreamerSimple::GetStreamLength(PRUint64* aStreamLength)
{
  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  nsresult rv;

  GstQuery *query;
  gboolean res;
  query = gst_query_new_duration(GST_FORMAT_TIME);
  res = gst_element_query(mPlay, query);
  if (res) {
    gint64 duration;
    gst_query_parse_duration(query, NULL, &duration);
    *aStreamLength = duration;
    rv = NS_OK;
  }
  else {
    rv = NS_ERROR_FAILURE;
  }

  gst_query_unref (query);

  return rv;
}

NS_IMETHODIMP
sbGStreamerSimple::GetIsAtEndOfStream(PRBool* aIsAtEndOfStream)
{
  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  *aIsAtEndOfStream = mIsAtEndOfStream;

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::Play()
{
  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  gst_element_set_state(mPlay, GST_STATE_PLAYING);
  mIsAtEndOfStream = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::Pause()
{
  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  gst_element_set_state(mPlay, GST_STATE_PAUSED);

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::Stop()
{
  if(!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  gst_element_set_state(mPlay, GST_STATE_NULL);

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerSimple::Seek(PRUint64 aTimeNanos)
{
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

GstBusSyncReply
sbGStreamerSimple::SyncHandler(GstBus* bus, GstMessage* message)
{
  GstMessageType msg_type;
  msg_type = GST_MESSAGE_TYPE(message);

  switch (msg_type) {
    case GST_MESSAGE_ERROR: {
      GError *error = NULL;
      gchar *debug = NULL;

      gst_message_parse_error(message, &error, &debug);

      LOG(("Error message: %s [%s]", GST_STR_NULL (error->message), GST_STR_NULL (debug)));

      g_free (debug);
      break;
    }
    case GST_MESSAGE_WARNING: {
      GError *error = NULL;
      gchar *debug = NULL;

      gst_message_parse_warning (message, &error, &debug);

      LOG(("Warning message: %s [%s]", GST_STR_NULL (error->message), GST_STR_NULL (debug)));

      g_warning ("%s [%s]", GST_STR_NULL (error->message), GST_STR_NULL (debug));

      g_error_free (error);
      g_free (debug);
      break;
    }

    case GST_MESSAGE_EOS: {
      mIsAtEndOfStream = PR_TRUE;
      break;
    }

    case GST_MESSAGE_STATE_CHANGED: {
      GstState old_state, new_state;
      gchar *src_name;

      gst_message_parse_state_changed (message, &old_state, &new_state, NULL);

      src_name = gst_object_get_name(message->src);
      LOG(("stage-changed: %s changed state from %s to %s", src_name,
          gst_element_state_get_name (old_state),
          gst_element_state_get_name (new_state)));
      g_free (src_name);
      break;
    }
/*
    case GST_MESSAGE_CLOCK_PROVIDE: {
      GstClock *clock;
      gboolean ready;

      gst_message_parse_clock_provide(msg, &clock, &ready);
      GstClockTime clockTime = gst_clock_get_internal_time(clock);
      LOG(("clock-provide: %d", clockTime));
      break;
    }
*/
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
      }
      break;
    }

    default:
      LOG(("Got message: %s", gst_message_type_get_name(msg_type)));
  }

  //gst_message_unref(message); XXX: Do i need to unref this?

  return GST_BUS_PASS;
}

