/* GStreamer
 * Copyright (C) <2010> Pioneers of the Inevitable <songbird@songbirdnest.com>
 *
 * Authors: Steve Lloyd <slloyd@songbirdnest.com>
 *          Michael Smith <msmith@songbirdnest.com>
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
#include <gst/base/gstbasesink.h>

#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsCOMPtr.h>
#include <nsIOutputStream.h>
#include <nsIProxyObjectManager.h>

#include "mozillaplugin.h"

typedef struct _GstMozillaSink GstMozillaSink;
typedef struct _GstMozillaSinkClass GstMozillaSinkClass;

struct _GstMozillaSink {
  GstBaseSink parent;

  /* The Mozilla output stream, whose methods will always be called on the main
   * thread using a proxy */
  nsCOMPtr<nsIOutputStream> output_stream;
  nsCOMPtr<nsIOutputStream> proxied_output_stream;

  /* If true, the stream is seekable.
   * TODO: Implement ability to seek. Right now this is just set to false. */
  gboolean is_seekable;
};

struct _GstMozillaSinkClass {
  GstBaseSinkClass parent_class;
};

GST_DEBUG_CATEGORY_STATIC (mozilla_sink_debug);
#define GST_CAT_DEFAULT mozilla_sink_debug

static const GstElementDetails gst_mozilla_sink_details =
GST_ELEMENT_DETAILS ((gchar *)"Mozilla nsIOutputStream sink",
    (gchar *)"Sink",
    (gchar *)"Write data to a hosting mozilla "
             "application's output stream API",
    (gchar *)"Pioneers of the Inevitable <songbird@songbirdnest.com");

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

enum
{
  PROP_0,
  PROP_STREAM,
};

static void gst_mozilla_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_mozilla_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstFlowReturn gst_mozilla_sink_render (GstBaseSink * bsink,
    GstBuffer * buf);
static gboolean gst_mozilla_sink_start (GstBaseSink * bsink);
static gboolean gst_mozilla_sink_stop (GstBaseSink * bsink);
static gboolean gst_mozilla_sink_event (GstBaseSink * bsink, GstEvent * event);

static void gst_mozilla_sink_close_stream (GstMozillaSink * sink);
static void gst_mozilla_sink_flush_stream (GstMozillaSink * sink);

GST_BOILERPLATE (GstMozillaSink, gst_mozilla_sink, GstBaseSink,
    GST_TYPE_BASE_SINK);

static void
gst_mozilla_sink_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_template));

  gst_element_class_set_details (element_class, &gst_mozilla_sink_details);
}

static void
gst_mozilla_sink_class_init (GstMozillaSinkClass * klass)
{
  GObjectClass *gobject_class;
  GstBaseSinkClass *gstbasesink_class;

  gobject_class = (GObjectClass *) klass;
  gstbasesink_class = (GstBaseSinkClass *) klass;

  gobject_class->set_property = gst_mozilla_sink_set_property;
  gobject_class->get_property = gst_mozilla_sink_get_property;

  g_object_class_install_property
      (gobject_class, PROP_STREAM,
       g_param_spec_pointer ("stream", "Stream",
          "Mozilla output stream", (GParamFlags) G_PARAM_READWRITE));

  gstbasesink_class->render = GST_DEBUG_FUNCPTR (gst_mozilla_sink_render); 
  gstbasesink_class->start = GST_DEBUG_FUNCPTR (gst_mozilla_sink_start);
  gstbasesink_class->stop = GST_DEBUG_FUNCPTR (gst_mozilla_sink_stop);
  gstbasesink_class->event = GST_DEBUG_FUNCPTR (gst_mozilla_sink_event);

  GST_DEBUG_CATEGORY_INIT (mozilla_sink_debug, "mozillasink", 0,
      "Mozilla Sink");
}


static void
gst_mozilla_sink_init (GstMozillaSink * sink, GstMozillaSinkClass * g_class)
{
  sink->output_stream = nsnull;
  sink->proxied_output_stream = nsnull;

  sink->is_seekable = FALSE;

  /* Don't sync to clock by default */
  gst_base_sink_set_sync (GST_BASE_SINK (sink), FALSE);
}

static void
gst_mozilla_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstMozillaSink *sink = GST_MOZILLA_SINK (object);

  switch (prop_id) {
    case PROP_STREAM:
    {
      nsresult rv;

      nsCOMPtr<nsIProxyObjectManager> proxyObjectManager =
          do_GetService("@mozilla.org/xpcomproxy;1", &rv);
      if (NS_FAILED (rv)) {
        GST_WARNING_OBJECT (sink, "Failed to get Mozilla proxy object manager");
        return;
      }
      
      sink->output_stream =
          do_QueryInterface((nsIOutputStream *) g_value_get_pointer(value));
      if (!sink->output_stream) {
        GST_WARNING_OBJECT (sink, "Failed to set output stream");
        return;
      }

      /* Get main thread proxy for the output stream. All output stream methods
       * will be called using the proxy. */ 
      rv = proxyObjectManager->GetProxyForObject(
                                  NS_PROXY_TO_MAIN_THREAD,
                                  NS_GET_IID (nsIOutputStream),
                                  sink->output_stream,
                                  NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                                  getter_AddRefs(sink->proxied_output_stream));
      if (NS_FAILED (rv)) {
        GST_WARNING_OBJECT (sink, "Failed to get proxy for output stream");
        return;
      }

      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_mozilla_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstMozillaSink *sink = GST_MOZILLA_SINK (object);

  switch (prop_id) {
    case PROP_STREAM:
    {
      nsIOutputStream* rawStreamPtr = sink->output_stream.get();
      NS_IF_ADDREF (rawStreamPtr);
      g_value_set_pointer (value, rawStreamPtr);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstFlowReturn
gst_mozilla_sink_render (GstBaseSink * bsink, GstBuffer * buf)
{
  GstMozillaSink *sink = GST_MOZILLA_SINK (bsink);
  nsresult rv;
  
  if (!sink->proxied_output_stream) {
    GST_WARNING_OBJECT (sink, "Tried to render without a proxied stream");
    return GST_FLOW_UNEXPECTED;
  }

  PRUint32 bytesWritten;
  PRUint32 offset = 0;
  PRUint32 count = GST_BUFFER_SIZE (buf);
  char * data = (char *) GST_BUFFER_DATA (buf);
  GST_DEBUG_OBJECT (sink, "Writing %u byte buffer", count);

  while (offset < count) {
    rv = sink->proxied_output_stream->Write(data + offset, count - offset,
                                            &bytesWritten);
    if (NS_FAILED (rv)) {
      GST_WARNING_OBJECT (sink, "Failed to write buffer to output stream");
      return GST_FLOW_UNEXPECTED;
    }

    GST_DEBUG_OBJECT (sink, "Wrote %u bytes to output stream", bytesWritten);
    offset += bytesWritten;
  }
  
  return GST_FLOW_OK;
}

static gboolean
gst_mozilla_sink_start (GstBaseSink * bsink)
{
  GstMozillaSink *sink = GST_MOZILLA_SINK (bsink);
   
  if (!sink->output_stream || !sink->proxied_output_stream) {
    GST_WARNING_OBJECT (sink, "Tried to start with invalid output stream");
    return FALSE;
  }

  return TRUE;
}

static gboolean
gst_mozilla_sink_stop (GstBaseSink * bsink)
{
  GstMozillaSink *sink = GST_MOZILLA_SINK (bsink);

  gst_mozilla_sink_close_stream (sink);

  return TRUE;
}

static gboolean
gst_mozilla_sink_event (GstBaseSink * bsink, GstEvent * event)
{
  GstEventType type;
  GstMozillaSink *sink = GST_MOZILLA_SINK (bsink);

  type = GST_EVENT_TYPE (event);

  switch (type) {
    case GST_EVENT_EOS:
    {
      GST_DEBUG_OBJECT (sink, "Handling EOS event");
      gst_mozilla_sink_close_stream (sink);
      break;
    }
    case GST_EVENT_FLUSH_STOP:
    {
      GST_DEBUG_OBJECT (sink, "Handling flush start event");
      gst_mozilla_sink_flush_stream (sink);
      break;
    }
    default:
      break;
  }

  return TRUE;
}

static void
gst_mozilla_sink_close_stream (GstMozillaSink *sink)
{
  GST_DEBUG_OBJECT (sink, "Closing output stream");
  nsresult rv;

  rv = sink->proxied_output_stream->Close();
  if (NS_FAILED (rv)) {
    GST_WARNING_OBJECT (sink, "Failed to close output stream");
  }
}

static void
gst_mozilla_sink_flush_stream (GstMozillaSink *sink)
{
  GST_DEBUG_OBJECT (sink, "Flushing output stream");

  nsresult rv = sink->proxied_output_stream->Flush();
  if (NS_FAILED (rv)) {
    GST_WARNING_OBJECT (sink, "Failed to flush output stream");
  }
}
