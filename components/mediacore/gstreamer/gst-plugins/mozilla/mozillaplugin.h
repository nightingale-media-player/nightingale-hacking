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

#include <gst/gst.h>

#define GST_MOZ_VERSION "1.0.4" // Version of gst-plugibs-bad
#define GST_MOZ_LICENSE "GPL"
#define GST_MOZ_PACKAGE "GStreamer Mozilla Plugin for Nightingale"
#define GST_MOZ_ORIGIN "http://www.getnightingale.com"
#define PACKAGE "Nightingale"

GType gst_mozilla_src_get_type (void);
#define GST_TYPE_MOZILLA_SRC \
  (gst_mozilla_src_get_type())
#define GST_MOZILLA_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MOZILLA_SRC,GstMozillaSrc))
#define GST_MOZILLA_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_MOZILLA_SRC,GstMozillaSrcClass))
#define GST_IS_MOZILLA_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MOZILLA_SRC))
#define GST_IS_MOZILLA_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MOZILLA_SRC))

GType gst_mozilla_sink_get_type (void);
#define GST_TYPE_MOZILLA_SINK \
  (gst_mozilla_sink_get_type())
#define GST_MOZILLA_SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MOZILLA_SINK,GstMozillaSink))
#define GST_MOZILLA_SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_MOZILLA_SINK,GstMozillaSinkClass))
#define GST_IS_MOZILLA_SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MOZILLA_SINK))
#define GST_IS_MOZILLA_SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MOZILLA_SINK))
