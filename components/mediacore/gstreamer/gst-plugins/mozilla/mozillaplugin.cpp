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
#include "mozillaplugin.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "mozillasrc", GST_RANK_PRIMARY+1,
      GST_TYPE_MOZILLA_SRC))
    return FALSE;

  if (!gst_element_register (plugin, "mozillasink", GST_RANK_NONE,
      GST_TYPE_MOZILLA_SINK))
    return FALSE;

  return TRUE;
}

extern "C" {
  GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "mozilla",
    "Mozilla source and sink",
    plugin_init,
    GST_MOZ_VERSION,
    GST_MOZ_LICENSE,
    GST_MOZ_PACKAGE,
    GST_MOZ_ORIGIN
  )
}
