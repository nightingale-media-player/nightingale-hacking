/* GStreamer
 * Copyright (C) <2008> Pioneers of the Inevitable <songbird@songbirdnest.com>
 *
 * Authors: Michael Smith <msmith@songbirdnest.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more 
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Must be before any other mozilla headers */
#include "mozilla-config.h"

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>
#include <gst/base/gstadapter.h>

#include <nsStringAPI.h>
#include <nsNetUtil.h>
#include <nsComponentManagerUtils.h>
#include <nsIProtocolHandler.h>
#include <nsIComponentRegistrar.h>
#include <nsCOMPtr.h>
#include <nsISupportsPrimitives.h>
#include <nsIResumableChannel.h>
#include <nsIRunnable.h>
#include <nsIHttpHeaderVisitor.h>
#include <nsIHttpEventSink.h>
#include <nsThreadUtils.h>
#include <nsIWindowWatcher.h>
#include <nsIAuthPrompt.h>

#include "mozillaplugin.h"

typedef struct _GstMozillaSrc GstMozillaSrc;
typedef struct _GstMozillaSrcClass GstMozillaSrcClass;

class StreamListener;

struct _GstMozillaSrc {
  GstPushSrc element;

  gint64 content_size;

  gboolean eos;
  gboolean flushing;

  gboolean iradio_mode;            // Are we in internet radio mode?
  GstCaps *icy_caps;               // caps to use (only in iradio mode)
  gboolean is_shoutcast_server;    // Are we dealing with a shoutcast server?
  gboolean shoutcast_headers_read; // If so, have we read the response headers?

  gint64 current_position; // Current offset in the stream, in bytes.
  gboolean is_seekable;  // This is true until we know seeking is not 
                         // supported (after we've seen the response)
  gboolean is_cancelled; // TRUE if we cancelled the request (and hence
                         // shouldn't treat the request ending as EOS)

  /* URI as a UTF-8 string */
  gchar *location;
  /* URI for location */
  nsCOMPtr<nsIURI> uri;

  /* Our input channel */
  nsCOMPtr<nsIChannel> channel;
  gboolean suspended; /* Is reading from the channel currently suspended? */

  /* Simple async queue to decouple mozilla (which is reading
   * from the main thread) and the streaming thread run by basesrc.
   * We keep it limited in size by suspending the request if we get
   * too full */
  GQueue *queue;
  gint queue_size;
  GMutex *queue_lock;
  GCond *queue_cond;
};

struct _GstMozillaSrcClass {
  GstPushSrcClass parent_class;
};

GST_DEBUG_CATEGORY_STATIC (mozillasrc_debug);
#define GST_CAT_DEFAULT mozillasrc_debug

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

enum
{
  PROP_0,
  PROP_LOCATION,
  PROP_IRADIO_MODE,
};

static void gst_mozilla_src_uri_handler_init (gpointer g_iface,
    gpointer iface_data);

static void gst_mozilla_src_finalize (GObject * gobject);
static void gst_mozilla_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_mozilla_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstFlowReturn gst_mozilla_src_create (GstPushSrc * psrc,
    GstBuffer ** outbuf);
static gboolean gst_mozilla_src_start (GstBaseSrc * bsrc);
static gboolean gst_mozilla_src_stop (GstBaseSrc * bsrc);
static gboolean gst_mozilla_src_get_size (GstBaseSrc * bsrc, 
        guint64 * size);
static gboolean gst_mozilla_src_is_seekable (GstBaseSrc * bsrc);
static gboolean gst_mozilla_src_do_seek (GstBaseSrc * bsrc,
    GstSegment * segment);
static gboolean gst_mozilla_src_unlock(GstBaseSrc * psrc);
static gboolean gst_mozilla_src_unlock_stop(GstBaseSrc * psrc);

class ResumeEvent : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  explicit ResumeEvent (GstMozillaSrc *src)
  {
    mSrc = (GstMozillaSrc *)g_object_ref (src);
  }

  ~ResumeEvent() 
  {
    g_object_unref (mSrc);
  }
private:
  GstMozillaSrc *mSrc;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(ResumeEvent,
                              nsIRunnable)

NS_IMETHODIMP
ResumeEvent::Run()
{
  if (mSrc->suspended) {
    nsCOMPtr<nsIRequest> request(do_QueryInterface(mSrc->channel));
    if (request) {
      GST_DEBUG_OBJECT (mSrc, "Resuming request...");
      request->Resume();
      mSrc->suspended = FALSE;
    }
  }

  return NS_OK;
}

class StreamListener : public nsIStreamListener,
                       public nsIHttpHeaderVisitor,
                       public nsIInterfaceRequestor,
                       public nsIHttpEventSink
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIHTTPHEADERVISITOR
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSIHTTPEVENTSINK
 
  StreamListener(GstMozillaSrc *aSrc);
  virtual ~StreamListener();

private:
  GstMozillaSrc *mSrc;
  GstAdapter *mAdapter;
  
  void ReadHeaders (gchar *data);
  GstBuffer *ScanForHeaders(GstBuffer *buf);
};

StreamListener::StreamListener (GstMozillaSrc *aSrc) : 
    mSrc(aSrc),
    mAdapter(NULL)
{
}

StreamListener::~StreamListener ()
{
  if (mAdapter)
    g_object_unref (mAdapter);
}

NS_IMPL_THREADSAFE_ISUPPORTS5(StreamListener,
                              nsIRequestObserver,
                              nsIStreamListener,
                              nsIHttpHeaderVisitor,
                              nsIInterfaceRequestor,
                              nsIHttpEventSink)

NS_IMETHODIMP 
StreamListener::GetInterface(const nsIID &aIID, void **aResult)
{
  if (aIID.Equals(NS_GET_IID(nsIAuthPrompt))) {
    nsCOMPtr<nsIWindowWatcher> wwatch(
            do_GetService(NS_WINDOWWATCHER_CONTRACTID));
    return wwatch->GetNewAuthPrompter(NULL, (nsIAuthPrompt**)aResult);
  }
  else if (aIID.Equals(NS_GET_IID(nsIHttpEventSink))) {
    return QueryInterface(aIID, aResult);
  }

  return NS_ERROR_NO_INTERFACE;
}

NS_IMETHODIMP
StreamListener::OnRedirect(nsIHttpChannel *httpChannel, nsIChannel *newChannel)
{
  nsCOMPtr<nsIChannel> channel(do_QueryInterface(newChannel));

  GST_DEBUG_OBJECT (mSrc, "Redirecting, got new channel");

  mSrc->channel = channel;
  mSrc->suspended = FALSE;

  return NS_OK;
}

NS_IMETHODIMP
StreamListener::VisitHeader(const nsACString &header, const nsACString &value)
{
  // If we see any headers, it's not shoutcast
  mSrc->is_shoutcast_server = FALSE;

  return NS_OK;
}

NS_IMETHODIMP
StreamListener::OnStartRequest(nsIRequest *req, nsISupports *ctxt)
{
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(req));
  nsresult rv;

  if (httpChannel) {
    nsCAutoString acceptRangesHeader;
    nsCAutoString icyHeader;
    PRBool succeeded;

    if (NS_SUCCEEDED(httpChannel->GetRequestSucceeded(&succeeded)) 
            && !succeeded) 
    {
      // HTTP response is not a 2xx! Error out, then cancel the request.
      PRUint32 responsecode;
      nsCString responsetext;

      rv = httpChannel->GetResponseStatus(&responsecode);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = httpChannel->GetResponseStatusText(responsetext);
      NS_ENSURE_SUCCESS(rv, rv);

      GST_INFO_OBJECT (mSrc, "HTTP Response %d (%s)", responsecode, 
              responsetext.get());

      /* Shut down if this is our current channel (but not if we've been 
       * redirected, in which case we have a different channel now
       */
      nsCOMPtr<nsIChannel> channel(do_QueryInterface(req));
      if (mSrc->channel == channel) {
        GST_ELEMENT_ERROR (mSrc, RESOURCE, READ,
            ("Could not read from URL %s", mSrc->location), 
            ("HTTP response code %d (%s) when fetching uri %s", 
             responsecode, responsetext.get(),
             mSrc->location));

        req->Cancel(NS_BINDING_ABORTED);
      }
      return NS_OK;
    }
    
    // Unfortunately, shoutcast isn't actually HTTP compliant - it sends back
    // a non-HTTP response. Mozilla accepts this, but just gives us back the
    // data - including the headers - as part of the body.
    // We detect this as a response with zero headers, and then handle it
    // specially.
    mSrc->is_shoutcast_server = TRUE;
    rv = httpChannel->VisitResponseHeaders(this);

    GST_DEBUG_OBJECT (mSrc, "Is shoutcast: %d", mSrc->is_shoutcast_server);

    rv = httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("Accept-Ranges"),
            acceptRangesHeader);
    if (NS_FAILED (rv) || acceptRangesHeader.IsEmpty()) {
      GST_DEBUG_OBJECT (mSrc, "No Accept-Ranges header in response; "
              "setting is_seekable to false");
      mSrc->is_seekable = FALSE;
    } else {
      if (acceptRangesHeader.Find (NS_LITERAL_CSTRING ("bytes")) < 0) {
        GST_DEBUG_OBJECT (mSrc, "No 'bytes' in Accept-Ranges header; "
                "setting is_seekable to false");
        mSrc->is_seekable = FALSE;
      }
      else {
        GST_DEBUG_OBJECT (mSrc, "Accept-Ranges header includes 'bytes' field, "
                "seeking supported");
      }
    }

    // Handle the metadata interval for non-shoutcast servers that support
    // this (like icecast).
    rv = httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("icy-metaint"),
           icyHeader);
    if (NS_SUCCEEDED (rv) && !icyHeader.IsEmpty()) {
      int metadata_interval;
      GST_DEBUG_OBJECT (mSrc, "Received icy-metaint header, parsing it");

      metadata_interval = icyHeader.ToInteger(&rv);
      if (NS_FAILED (rv)) {
        GST_WARNING_OBJECT (mSrc, "Could not parse icy-metaint header");
      }
      else {
        GST_DEBUG_OBJECT (mSrc, "Using icy caps with metadata interval %d",
                metadata_interval);
        if (mSrc->icy_caps) {
          gst_caps_unref (mSrc->icy_caps);
        }
        mSrc->icy_caps = gst_caps_new_simple ("application/x-icy",
                "metadata-interval", G_TYPE_INT, metadata_interval, NULL);
      }
    }
    else {
      GST_DEBUG_OBJECT (mSrc, "No icy-metaint header found");
    }
  }

  /* Unfortunately channel->GetContentLength() is only 32 bit; this is the 
   * 64 bit variant - excessively complex... */
  nsCOMPtr<nsIPropertyBag2> properties(do_QueryInterface(mSrc->channel));
  if (properties && mSrc->content_size == -1) {
    PRInt64 length;
    nsresult rv;
    rv = properties->GetPropertyAsInt64 (NS_LITERAL_STRING ("content-length"),
            &length);

    if (NS_SUCCEEDED(rv) && length != -1) {
      mSrc->content_size = length;
      GST_DEBUG_OBJECT (mSrc, "Read content length: %" G_GINT64_FORMAT,
              mSrc->content_size);

      ((GstBaseSrc *)mSrc)->segment.duration = mSrc->content_size;
      gst_element_post_message (GST_ELEMENT (mSrc), 
              gst_message_new_duration (GST_OBJECT (mSrc), GST_FORMAT_BYTES,
              mSrc->content_size));
    }
    else {
      GST_DEBUG_OBJECT (mSrc, "No content-length found");
    }
  }
  
  GST_DEBUG_OBJECT (mSrc, "%p::StreamListener::OnStartRequest called; "
          "connection made", this);
  return NS_OK;
}

NS_IMETHODIMP
StreamListener::OnStopRequest(nsIRequest *req, nsISupports *ctxt, 
    nsresult status)
{
  GST_DEBUG_OBJECT (mSrc, "%p::StreamListener::OnStopRequest called; "
          "connection lost", this);

  /* If we failed, turn this into an element error message. Unfortunately we
     don't have much information at this point about what sort of failure it
     was, so this will have to do.
   */

  /* If we cancelled the request explicitly, we pass NS_BINDING_ABORTED as the
     status code. This should not be treated as an error from the network stack
   */
  if (status != NS_BINDING_ABORTED && NS_FAILED (status)) {
    GST_ELEMENT_ERROR (mSrc, RESOURCE, READ,
        ("Could not read from URL %s", mSrc->location), 
        ("nsresult %d", status));
  }

  if (!mSrc->is_cancelled)
  {
    GST_DEBUG_OBJECT (mSrc, "At EOS after request stopped");
    mSrc->eos = TRUE;
  }

  mSrc->is_cancelled = FALSE;

  return NS_OK;
}

void StreamListener::ReadHeaders (gchar *data)
{
  gchar **lines = g_strsplit (data, "\r\n", 0);
  gchar **line;

  line = lines;
  while (*line) {
    gchar **vals = g_strsplit_set (*line, ": ", 2);
    if (vals[0] && vals[1]) {
      // Well formed; we can process...
      gchar *header  = vals[0];
      gchar *value = vals[1];

      GST_DEBUG ("Read header: '%s' : '%s'", header, value);

      // There's actually only one header we're interested in right now...
      if (g_ascii_strcasecmp (header, "icy-metaint") == 0) {
        int metaint = g_ascii_strtoll (value, NULL, 10);

        if (metaint) {
          GST_DEBUG ("icy-metaint read: %d", metaint);
          if (mSrc->icy_caps)
            gst_caps_unref (mSrc->icy_caps);
          mSrc->icy_caps = gst_caps_new_simple ("application/x-icy",
                  "metadata-interval", G_TYPE_INT, metaint, NULL);
        }
      }
    }

    g_strfreev (vals);
    line++;
  }

  g_strfreev (lines);
}

GstBuffer *StreamListener::ScanForHeaders(GstBuffer *buf)
{
  const gchar *data;
  gchar *end;
  int len;

  if (!mAdapter)
    mAdapter = gst_adapter_new();
  gst_adapter_push (mAdapter, buf);

  len = gst_adapter_available (mAdapter);
  data = (gchar *) gst_adapter_map(mAdapter, len);

  end = g_strrstr_len (data, len, "\r\n\r\n");

  if (!end)
    return NULL;

  // Throw in a null terminator after the final header (overwriting the
  // second '\r' in the terminating '\r\n\r\n\'). We own the data, and
  // we're about to discard it, so this is ok...
  *(end + 2) = 0;

  GST_DEBUG ("Reading headers from '%s'", data);
  ReadHeaders ((gchar *)data);

  len = end - data + 4;
  gst_adapter_unmap(mAdapter);

  return gst_adapter_take_buffer (mAdapter, gst_adapter_available (mAdapter));
}

/* Selected very arbitrarily */
#define MAX_INTERNAL_BUFFER (8*1024)

/* Called when new data is available. All the data MUST be read.
 *
 * We read into our internal async queue, and then suspend the request
 * for a little while if the queue is now 'full'. This avoids infinite
 * memory use, which is generally considered good.
 */
NS_IMETHODIMP
StreamListener::OnDataAvailable(nsIRequest *req, nsISupports *ctxt,
                            nsIInputStream *stream,
                            PRUint32 offset, PRUint32 count)
{
  GST_DEBUG_OBJECT (mSrc, "%p::OnDataAvailable called: [count=%u, offset=%u]",
          this, count, offset);

  nsresult rv;
  PRUint32 bytesRead=0;
  int len;
  GstMapInfo info;

  GstBuffer *buf = gst_buffer_new_and_alloc (count);

  gst_buffer_map(buf, &info, GST_MAP_READ);

  rv = stream->Read((char *) info.data, count, &bytesRead);
  if (NS_FAILED(rv)) {
    GST_DEBUG_OBJECT (mSrc, "stream->Read failed with rv=%x", rv);
    return rv;
  }

  info.size = bytesRead;

  if (mSrc->is_shoutcast_server && !mSrc->shoutcast_headers_read) {
    GST_DEBUG_OBJECT (mSrc, "Scanning for headers");
    // Scan the buffer for the end of the headers. Returns a new
    // buffer if we have any non-header data.
    buf = ScanForHeaders (buf);

    if (!buf) {
      return NS_OK;
    }
    else {
      mSrc->shoutcast_headers_read = TRUE;
    }
  }

  len = info.size;

  g_mutex_lock (mSrc->queue_lock);

  mSrc->queue_size += len;
  g_queue_push_tail (mSrc->queue, buf);
  GST_DEBUG_OBJECT (mSrc, "Pushed %d byte buffer onto queue (now %d bytes)",
          len, mSrc->queue_size);

  g_cond_signal (mSrc->queue_cond);

  /* If we're reading too quickly, we'll suspend after this.
   * Suspend if:
   *   - we've gone over our buffer size
   *   - reading this much _again_ will put us over the buffer size, and we have
   *     at least half a buffer already.
   *
   * We're required to always read all the data.
   */
  if ((mSrc->queue_size > MAX_INTERNAL_BUFFER) ||
      (mSrc->queue_size + len > MAX_INTERNAL_BUFFER && 
       mSrc->queue_size > MAX_INTERNAL_BUFFER/2))
  {
    if (mSrc->suspended) {
      GST_WARNING_OBJECT (mSrc, "Trying to suspend while already suspended, "
              "unexpected!");
      goto done;
    }

    /* Then we should suspend the connection until we need more data.
     * If our channel doesn't support nsIRequest, we can't do this, but
     * that's not an error. */
    nsCOMPtr<nsIRequest> request(do_QueryInterface(mSrc->channel));
    if (request) {
      GST_DEBUG_OBJECT (mSrc, "Suspending request, reading too fast!");
      request->Suspend();
      mSrc->suspended = TRUE;
    }
  }

done:
  g_mutex_unlock (mSrc->queue_lock);

  return NS_OK;
}

#define gst_mozilla_src_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstMozillaSrc, gst_mozilla_src, GST_TYPE_PUSH_SRC, 
    G_IMPLEMENT_INTERFACE(GST_TYPE_URI_HANDLER, gst_mozilla_src_uri_handler_init););

static void
gst_mozilla_src_class_init (GstMozillaSrcClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GObjectClass *gobject_class;

  GstBaseSrcClass *gstbasesrc_class;
  GstPushSrcClass *gstpushsrc_class;

  gobject_class = (GObjectClass *) klass;
  gstbasesrc_class = (GstBaseSrcClass *) klass;
  gstpushsrc_class = (GstPushSrcClass *) klass;

  gobject_class->set_property = gst_mozilla_src_set_property;
  gobject_class->get_property = gst_mozilla_src_get_property;
  gobject_class->finalize = gst_mozilla_src_finalize;

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&srctemplate));

  gst_element_class_set_details_simple(element_class,
    (gchar *)"Mozilla nsIInputStream source",
    (gchar *)"Source/Network",
    (gchar *)"Receive data from a hosting mozilla "
             "application Mozilla's I/O APIs",
    (gchar *)"Pioneers of the Inevitable <songbird@songbirdnest.com");

  g_object_class_install_property
      (gobject_class, PROP_LOCATION,
      g_param_spec_string ("location", "Location",
          "Location to read from", "", (GParamFlags)G_PARAM_READWRITE));

  g_object_class_install_property
      (gobject_class, PROP_IRADIO_MODE,
      g_param_spec_boolean ("iradio-mode", "iradio-mode",
          "Enable reading of shoutcast/icecast metadata", 
          FALSE, (GParamFlags)G_PARAM_READWRITE));

  gstbasesrc_class->start = GST_DEBUG_FUNCPTR (gst_mozilla_src_start);
  gstbasesrc_class->stop = GST_DEBUG_FUNCPTR (gst_mozilla_src_stop);
  gstbasesrc_class->get_size = GST_DEBUG_FUNCPTR (gst_mozilla_src_get_size);
  gstbasesrc_class->is_seekable =
      GST_DEBUG_FUNCPTR (gst_mozilla_src_is_seekable);
  gstbasesrc_class->do_seek = GST_DEBUG_FUNCPTR (gst_mozilla_src_do_seek);
  gstbasesrc_class->unlock = GST_DEBUG_FUNCPTR (gst_mozilla_src_unlock);
  gstbasesrc_class->unlock_stop = 
      GST_DEBUG_FUNCPTR (gst_mozilla_src_unlock_stop);

  gstpushsrc_class->create = GST_DEBUG_FUNCPTR (gst_mozilla_src_create);

  GST_DEBUG_CATEGORY_INIT (mozillasrc_debug, "mozillasrc", 0,
      "Mozilla Source");
}

static void unref_buffer (gpointer data, gpointer user_data)
{
  gst_buffer_unref ((GstBuffer *)data);
}

static void
gst_mozilla_src_flush (GstMozillaSrc * src)
{
  GST_DEBUG_OBJECT (src, "Flushing input queue");

  g_mutex_lock (src->queue_lock);
  g_queue_foreach (src->queue, unref_buffer, NULL);
  /* g_queue_clear (src->queue); // glib 2.14 required */
  g_queue_free (src->queue);
  src->queue = g_queue_new ();
  g_mutex_unlock (src->queue_lock);
}

static void
gst_mozilla_src_clear (GstMozillaSrc * src)
{
  if (src->location) {
    g_free ((void *)src->location);
    src->location = NULL;
  }

  if (src->icy_caps) {
    gst_caps_unref (src->icy_caps);
    src->icy_caps = NULL;
  }

  src->content_size = -1;
  src->current_position = 0;
  src->is_seekable = TRUE; /* Only set to false once we know */

  src->is_shoutcast_server = FALSE;
  src->shoutcast_headers_read = FALSE;
}

static void
gst_mozilla_src_init (GstMozillaSrc * src)
{
  gst_mozilla_src_clear (src);

  src->queue_lock = new GMutex;
  src->queue_cond = new GCond;

  g_mutex_init(src->queue_lock);
  g_cond_init(src->queue_cond);
  src->queue = g_queue_new ();

  src->iradio_mode = FALSE;
}

static void
gst_mozilla_src_finalize (GObject * gobject)
{
  GstMozillaSrc *src = GST_MOZILLA_SRC (gobject);

  GST_DEBUG_OBJECT (gobject, "Finalizing mozillasrc");

  gst_mozilla_src_flush (src);
  gst_mozilla_src_clear (src);

  g_mutex_clear(src->queue_lock);
  g_cond_clear(src->queue_cond);
  g_queue_free(src->queue);

  G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void
gst_mozilla_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstMozillaSrc *src = GST_MOZILLA_SRC (object);

  switch (prop_id) {
    case PROP_LOCATION:
    {
      src->location = g_value_dup_string (value);
      break;
    }
    case PROP_IRADIO_MODE:
    {
      src->iradio_mode = g_value_get_boolean (value);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_mozilla_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstMozillaSrc *src = GST_MOZILLA_SRC (object);

  switch (prop_id) {
    case PROP_LOCATION:
    {
      g_value_set_string (value, src->location);
      break;
    }
    case PROP_IRADIO_MODE:
    {
      g_value_set_boolean (value, src->iradio_mode);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_mozilla_src_unlock(GstBaseSrc * psrc)
{
  GstMozillaSrc *src = GST_MOZILLA_SRC (psrc);

  src->flushing = TRUE;

  GST_DEBUG_OBJECT (src, "unlock");
  g_mutex_lock (src->queue_lock);
  g_cond_signal (src->queue_cond);
  g_mutex_unlock (src->queue_lock);

  return TRUE;
}

static gboolean
gst_mozilla_src_unlock_stop(GstBaseSrc * psrc)
{
  GstMozillaSrc *src = GST_MOZILLA_SRC (psrc);

  src->flushing = FALSE;

  GST_DEBUG_OBJECT (src, "unlock_stop");
  g_mutex_lock (src->queue_lock);
  g_cond_signal (src->queue_cond);
  g_mutex_unlock (src->queue_lock);

  return TRUE;
}

static GstFlowReturn
gst_mozilla_src_create (GstPushSrc * psrc, GstBuffer ** outbuf)
{
  GstMozillaSrc *src;
  GstBaseSrc *basesrc;
  GstFlowReturn ret;
  GstMapInfo info;

  src = GST_MOZILLA_SRC (psrc);
  basesrc = GST_BASE_SRC_CAST (psrc);

  if (G_UNLIKELY (src->eos)) {
    GST_DEBUG_OBJECT (src, "Create called at EOS");
    return GST_FLOW_EOS;
  }
  
  if (G_UNLIKELY (src->flushing)) {
    GST_DEBUG_OBJECT (src, "Create called while flushing");
    return GST_FLOW_FLUSHING;
  }

  /* Pop a buffer from our async queue, if possible. */
  g_mutex_lock (src->queue_lock);

  GST_DEBUG_OBJECT (src, "Queue has %d bytes", src->queue_size);

  /* If the queue is empty, wait for it to be non-empty, or for us to be 
   * otherwise interrupted */
  if (g_queue_is_empty (src->queue)) {
    GST_DEBUG_OBJECT (src, "Queue is empty; waiting");

    /* Ask mozilla to resume reading the stream */
    nsCOMPtr<nsIRunnable> resume_event = new ResumeEvent (src);
    NS_DispatchToMainThread (resume_event);

    GST_DEBUG_OBJECT (src, "Starting wait");
    g_cond_wait (src->queue_cond, src->queue_lock);
    GST_DEBUG_OBJECT (src, "Wait done, we should have a buffer now");

    /* If it's still empty, we were interrupted by something, so return a 
     * failure */
    if (g_queue_is_empty (src->queue)) {
      GST_DEBUG_OBJECT (src, "Still no buffer; bailing");
      ret = GST_FLOW_EOS;
      goto done;
    }
  }

  /* And otherwise, we're fine: pop the next buffer and return */
  *outbuf = (GstBuffer *)g_queue_pop_head (src->queue);

  gst_buffer_map(*outbuf, &info, GST_MAP_READ);
  src->queue_size -= info.size;

  GST_BUFFER_OFFSET (*outbuf) = src->current_position;
  src->current_position += info.size;

  GST_DEBUG_OBJECT(src, "Popped %lu byte buffer from queue", 
                  info.size);
  ret = GST_FLOW_OK;

done:
  g_mutex_unlock (src->queue_lock);

  return ret;
}

static void
gst_mozilla_src_cancel_request (GstMozillaSrc *src)
{
  nsCOMPtr<nsIRequest> request(do_QueryInterface(src->channel));
  if (request) {
    if (src->suspended) {
      /* Cancel doesn't work while suspended */
      GST_DEBUG_OBJECT (src, "Resuming request for cancel");
      request->Resume();
      src->suspended = FALSE;
    }

    // Set this so we can distinguish between cancelling a request,
    // and reaching EOS.
    src->is_cancelled = TRUE;

    GST_DEBUG_OBJECT (src, "Cancelling request");
    request->Cancel(NS_BINDING_ABORTED);
  }

  src->channel = 0;
}

static gboolean
gst_mozilla_src_create_request (GstMozillaSrc *src, GstSegment *segment)
{
  nsresult rv;
  nsCOMPtr<nsIStreamListener> listener = new StreamListener(src);

  rv = NS_NewChannel(getter_AddRefs(src->channel), src->uri, 
            nsnull, nsnull, nsnull, 
            nsIRequest::LOAD_BYPASS_CACHE | nsIRequest::INHIBIT_CACHING);
  if (NS_FAILED (rv)) {
    GST_WARNING_OBJECT (src, "Failed to create channel for %s", src->location);
    return FALSE;
  }

  nsCOMPtr<nsIInterfaceRequestor> requestor = do_QueryInterface (listener);
  src->channel->SetNotificationCallbacks(requestor);

  if (segment && segment->format == GST_FORMAT_BYTES && segment->start > 0) {
    nsCOMPtr<nsIResumableChannel> resumable(do_QueryInterface(src->channel));
    if (resumable) {
      GST_DEBUG_OBJECT (src, "Trying to resume at %lu bytes", segment->start);

      rv = resumable->ResumeAt(segment->start, EmptyCString());
      // If we failed, just log it and continue; it's not critical.
      if (NS_FAILED (rv))
        GST_WARNING_OBJECT (src, 
                "Failed to resume channel at non-zero offsets");
      else
        src->current_position = segment->start;
    }
  }

  nsCOMPtr<nsIHttpChannel> httpchannel(do_QueryInterface(src->channel));

  if (httpchannel) {
    NS_NAMED_LITERAL_CSTRING (useragent, "User-Agent");
    NS_NAMED_LITERAL_CSTRING (mozilla, "Mozilla");
    NS_NAMED_LITERAL_CSTRING (notmozilla, "NotMoz");
    nsCAutoString agent;

    // Set up to receive shoutcast-style metadata, if in that mode.
    if (src->iradio_mode) {
      rv = httpchannel->SetRequestHeader(
              NS_LITERAL_CSTRING("icy-metadata"),
              NS_LITERAL_CSTRING("1"),
              PR_FALSE);
      if (NS_FAILED (rv))
        GST_WARNING_OBJECT (src, 
                "Failed to set icy-metadata header on channel");
    }

    // Some servers (notably shoutcast) serve html to web browsers, and the 
    // stream to everything else. So, use a different user-agent so as not to
    // look like a web browser - mangle the existing one to not mention 
    // Mozilla (we change 'Mozilla' to 'NotMoz'.
    rv = httpchannel->GetRequestHeader (useragent, agent);

    if (NS_SUCCEEDED (rv)) {
      int offset;
      GST_DEBUG_OBJECT (src, "Default User-Agent is '%s'",
              agent.BeginReading());

      offset = agent.Find(mozilla);
      if (offset >= 0) {
        agent.Replace(offset, mozilla.Length(), notmozilla);
      }

      GST_DEBUG_OBJECT (src, "Actual User-Agent is '%s'",
              agent.BeginReading());

      rv = httpchannel->SetRequestHeader(useragent, agent, PR_FALSE);
      if (NS_FAILED (rv)) 
        GST_WARNING_OBJECT (src, "Failed to set user agent on channel");
    }
  }

  rv = src->channel->AsyncOpen(listener, nsnull);
  if (NS_FAILED (rv)) {
    GST_WARNING_OBJECT (src, "Failed to open channel for %s", src->location);
    return FALSE;
  }

  return TRUE;
}

static gboolean
gst_mozilla_src_start (GstBaseSrc * bsrc)
{
  GstMozillaSrc *src = GST_MOZILLA_SRC (bsrc);
  nsresult rv;

  if (!src->location) {
    GST_WARNING_OBJECT (src, "No location set");
    return FALSE;
  }

  rv = NS_NewURI(getter_AddRefs(src->uri), src->location);
  if (NS_FAILED (rv)) {
    GST_WARNING_OBJECT (src, "Failed to create URI from %s", src->location);
    return FALSE;
  }

  /* Send off a request! */
  if (gst_mozilla_src_create_request (src, NULL)) {
    GST_DEBUG_OBJECT (src, "Started request");
    return TRUE;
  }
  else {
    GST_ELEMENT_ERROR (src, LIBRARY, INIT,
        (NULL), ("Failed to initialise mozilla to fetch uri %s", 
        src->location));
    return FALSE;
  }
}

static gboolean
gst_mozilla_src_stop (GstBaseSrc * bsrc)
{
  GstMozillaSrc *src = GST_MOZILLA_SRC (bsrc);

  GST_INFO_OBJECT (src, "Stop(): shutting down connection");

  if (src->channel) {
    nsCOMPtr<nsIRequest> request(do_QueryInterface(src->channel));
    if (request) {
      GST_DEBUG_OBJECT (src, "Cancelling request");
      request->Cancel(NS_BINDING_ABORTED);
    }

    // Delete the underlying nsIChannel.
    src->channel = 0;
    src->uri = 0;
  }

  return TRUE;
}

static gboolean
gst_mozilla_src_get_size (GstBaseSrc * bsrc, guint64 * size)
{
  GstMozillaSrc *src;

  src = GST_MOZILLA_SRC (bsrc);

  if (src->content_size == -1)
    return FALSE;

  *size = src->content_size;

  return TRUE;
}


static gboolean
gst_mozilla_src_is_seekable (GstBaseSrc * bsrc)
{
  /* We always claim seekability, and then fail it when a seek is attempted if
   * we can't support it - we don't know at this point if it's actually 
   * seekable or not (we only know that once we've received  headers) */
  return TRUE;
}

static gboolean
gst_mozilla_src_do_seek (GstBaseSrc * bsrc, GstSegment * segment)
{
  GstMozillaSrc *src = GST_MOZILLA_SRC (bsrc);

  if (segment->start == src->current_position) {
    /* We're being asked to seek to the where we already are (this includes
     * the initial seek to zero); so just return success */
    return TRUE;
  }

  if (!src->is_seekable) {
    GST_INFO_OBJECT (src, "is_seekable FALSE; failing seek immediately");
    return FALSE;
  }

  /* Cancel current request; create a new one */
  gst_mozilla_src_cancel_request (src);

  if (gst_mozilla_src_create_request (src, segment)) {
    GST_INFO_OBJECT (src, "New request for seek initiated");
    return TRUE;
  }
  else {
    GST_WARNING_OBJECT (src, "Creating new request for seek failed");
    return FALSE;
  }
}

/* GstURIHandler Interface */
static GstURIType
gst_mozilla_src_uri_get_type(GType handler)
{
  return GST_URI_SRC;
}

static gchar **_supported_protocols = NULL;

static const gchar * const *
gst_mozilla_src_uri_get_protocols(GType handler)
{
  /* Return the cache if we've created it */
  if (_supported_protocols != NULL)
    return _supported_protocols;

  int numprotocols = 0;
  nsresult rv;
  nsCOMPtr<nsIComponentRegistrar> registrar;
  rv = NS_GetComponentRegistrar(getter_AddRefs(registrar));
  NS_ENSURE_SUCCESS(rv, NULL);

  nsCOMPtr<nsISimpleEnumerator> simpleEnumerator;
  rv = registrar->EnumerateContractIDs(getter_AddRefs(simpleEnumerator));
  NS_ENSURE_SUCCESS(rv, NULL);

  // Enumerate through the contractIDs and look for a specific prefix
  nsCOMPtr<nsISupports> element;
  PRBool more = PR_FALSE;
  while(NS_SUCCEEDED(simpleEnumerator->HasMoreElements(&more)) && more) {

    rv = simpleEnumerator->GetNext(getter_AddRefs(element));
    NS_ENSURE_SUCCESS(rv, NULL);

    nsCOMPtr<nsISupportsCString> contractString =
      do_QueryInterface(element, &rv);
    if NS_FAILED(rv) {
      NS_WARNING("QueryInterface failed");
      continue;
    }

    nsCAutoString contractID;
    rv = contractString->GetData(contractID);
    if NS_FAILED(rv) {
      NS_WARNING("GetData failed");
      continue;
    }

    NS_NAMED_LITERAL_CSTRING(prefix, NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX);

    if (!StringBeginsWith(contractID, prefix))
      continue;

    nsCAutoString scheme = contractID;
    scheme.Cut(0, prefix.Length());

    if (scheme.Equals(NS_LITERAL_CSTRING("file")))
    {
      // We don't want to handle normal files.
      GST_DEBUG ("Skipping file scheme");
      continue;
    }

    GST_DEBUG ("Adding scheme '%s'", scheme.BeginReading());

    // Now add it to our null-terminated array.
    numprotocols++;
    _supported_protocols = (gchar **)g_realloc (_supported_protocols, 
            sizeof(gchar *) * (numprotocols + 1));
    _supported_protocols[numprotocols - 1] = g_strdup (scheme.BeginReading());
    _supported_protocols[numprotocols] = NULL;
  }

  return _supported_protocols;
}

static gchar *
gst_mozilla_src_uri_get_uri (GstURIHandler * handler)
{ 
  GstMozillaSrc *src = GST_MOZILLA_SRC (handler);

  return src->location;
}

static gboolean
gst_mozilla_src_uri_set_uri(GstURIHandler * handler, 
                            const gchar * uri, 
                            GError** error)
{
  GstMozillaSrc *src = GST_MOZILLA_SRC (handler);

  GST_DEBUG_OBJECT (src, "URI set to '%s'", uri);

  src->location = g_strdup (uri);
  return TRUE;
}

static void
gst_mozilla_src_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = gst_mozilla_src_uri_get_type;
  iface->get_protocols = gst_mozilla_src_uri_get_protocols;
  iface->get_uri = gst_mozilla_src_uri_get_uri;
  iface->set_uri = gst_mozilla_src_uri_set_uri;
}
