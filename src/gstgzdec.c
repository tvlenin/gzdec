/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2023 Lenin <<ttvleninn@gmail.com>>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-gzdec
 *
 * FIXME:Describe gzdec here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m filesrc location=/path/to/file ! gzdec method=zlib ! filesink location=/path/to/decompressed/file
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/gst.h>
#include "gstgzdec.h"

#include <zlib.h>
#include <bzlib.h>

GST_DEBUG_CATEGORY_STATIC(gst_gzdec_debug);
#define GST_CAT_DEFAULT gst_gzdec_debug
#define DEFAULT_DEC_SIZE 1024

enum
{
  PROP_0,
  PROP_SILENT,
  PROP_METHOD
};

struct _GstGzdec
{
  GstElement parent;

  GstPad *sinkpad, *srcpad;

  gboolean silent;
  GstDecMethod method;
  gboolean ready;
  z_stream stream;
  bz_stream bz_stream;
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE("sink",
                                                                   GST_PAD_SINK,
                                                                   GST_PAD_ALWAYS,
                                                                   GST_STATIC_CAPS("application/x-gzip"));

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE("src",
                                                                  GST_PAD_SRC,
                                                                  GST_PAD_ALWAYS,
                                                                  GST_STATIC_CAPS("ANY"));

#define gst_gzdec_parent_class parent_class
G_DEFINE_TYPE(GstGzdec, gst_gzdec, GST_TYPE_ELEMENT);

GST_ELEMENT_REGISTER_DEFINE(gzdec, "gzdec", GST_RANK_NONE,
                            GST_TYPE_GZDEC);

static void gst_gzdec_set_property(GObject *object,
                                   guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_gzdec_get_property(GObject *object,
                                   guint prop_id, GValue *value, GParamSpec *pspec);
static GstFlowReturn gst_gzdec_chain(GstPad *pad,
                                     GstObject *parent, GstBuffer *buf);

GType gst_method_get_type(void)
{
  static GType gzdec_type = 0;

  if (g_once_init_enter(&gzdec_type))
  {
    static GEnumValue gzdec_types[] = {
        {ZLIB, "ZLIB method",
         "zlib"},
        {BZLIB,
         "BZLIB method",
         "bzlib"},
        {0, NULL, NULL},
    };

    GType temp = g_enum_register_static("GstDecMethod",
                                        gzdec_types);

    g_once_init_leave(&gzdec_type, temp);
  }

  return gzdec_type;
}

/* GObject vmethod implementations */
static void
gst_gzdec_decompress_end(GstGzdec *dec)
{
  g_return_if_fail(GST_IS_GZDEC(dec));

  if (dec->ready)
  {
    GST_DEBUG_OBJECT(dec, "Finalize gzdec decompressing library");
    if (dec->method == ZLIB)
    {
      inflateEnd(&dec->stream);
      memset(&dec->stream, 0, sizeof(dec->stream));
    }
    else
    {
      BZ2_bzDecompressEnd(&dec->bz_stream);
      memset(&dec->bz_stream, 0, sizeof(dec->bz_stream));
    }
    dec->ready = FALSE;
  }
}

static void
gst_gzdec_finalize(GObject *object)
{
  GstGzdec *dec = GST_GZDEC(object);
  GST_DEBUG_OBJECT(dec, "Finalize gzdec");
  gst_gzdec_decompress_end(dec);
  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_gzdec_decompress_init(GstGzdec *dec)
{
  gint ret;
  g_return_if_fail(GST_IS_GZDEC(dec));

  gst_gzdec_decompress_end(dec);
  if (dec->method == ZLIB)
  {
    dec->stream.zalloc = Z_NULL;
    dec->stream.zfree = Z_NULL;
    dec->stream.opaque = Z_NULL;
    dec->stream.avail_in = 0;
    dec->stream.next_in = Z_NULL;
    // MAX_WBITS + 16  to deflate (RFC 1952)
    ret = inflateInit2(&dec->stream, MAX_WBITS + 16);
  }
  else
  {
    dec->bz_stream.bzalloc = NULL;
    dec->bz_stream.bzfree = NULL;
    dec->bz_stream.opaque = NULL;
    ret = BZ2_bzDecompressInit(&dec->bz_stream, 0, 0);
  }
  dec->ready = TRUE;
  return;
}

static GstStateChangeReturn
gst_gzdec_change_state(GstElement *element, GstStateChange transition)
{
  g_return_if_fail(element);
  GstGzdec *dec = GST_GZDEC(element);
  GstStateChangeReturn ret;

  GST_DEBUG_OBJECT(dec, "Changing gzdec state");
  ret = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
  if (ret != GST_STATE_CHANGE_SUCCESS)
    return ret;
  switch (transition)
  {
  case GST_STATE_CHANGE_PAUSED_TO_READY:
    gst_gzdec_decompress_init(dec);
    break;
  case GST_STATE_CHANGE_NULL_TO_READY:
    gst_gzdec_decompress_init(dec);
    break;
  default:
    break;
  }
  return ret;
}

/* initialize the gzdec's class */
static void
gst_gzdec_class_init(GstGzdecClass *klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *)klass;
  gstelement_class = (GstElementClass *)klass;

  gstelement_class->change_state = GST_DEBUG_FUNCPTR(gst_gzdec_change_state);
  gobject_class->finalize = gst_gzdec_finalize;

  gobject_class->set_property = gst_gzdec_set_property;
  gobject_class->get_property = gst_gzdec_get_property;

  g_object_class_install_property(gobject_class, PROP_SILENT,
                                  g_param_spec_boolean("silent", "Silent", "Produce verbose output ?",
                                                       FALSE, G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_METHOD,
                                  g_param_spec_enum("method",
                                                    "Method",
                                                    "Decompress method",
                                                    GST_TYPE_METHOD, ZLIB,
                                                    (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
                                                     GST_PARAM_MUTABLE_READY)));

  gst_element_class_set_details_simple(gstelement_class,
                                       "Gzip decompress",
                                       "gzdec",
                                       "Element to decompress .gz and .bz2 files", "Lenin Torres <<ttvleninn@gmail.com>>");

  gst_element_class_add_pad_template(gstelement_class,
                                     gst_static_pad_template_get(&src_factory));
  gst_element_class_add_pad_template(gstelement_class,
                                     gst_static_pad_template_get(&sink_factory));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad callback functions
 * initialize instance structure
 */
static void
gst_gzdec_init(GstGzdec *dec)
{
  
  g_return_if_fail(GST_IS_GZDEC(dec));
  dec->sinkpad = gst_pad_new_from_static_template(&sink_factory, "sink");

  gst_pad_set_chain_function(dec->sinkpad,
                             GST_DEBUG_FUNCPTR(gst_gzdec_chain));
  GST_PAD_SET_PROXY_CAPS(dec->sinkpad);
  gst_element_add_pad(GST_ELEMENT(dec), dec->sinkpad);

  dec->srcpad = gst_pad_new_from_static_template(&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS(dec->srcpad);
  gst_element_add_pad(GST_ELEMENT(dec), dec->srcpad);

  dec->silent = FALSE;
  dec->method = ZLIB;
}

static void
gst_gzdec_set_property(GObject *object, guint prop_id,
                       const GValue *value, GParamSpec *pspec)
{
  GstGzdec *dec = GST_GZDEC(object);
  switch (prop_id)
  {
  case PROP_SILENT:
    dec->silent = g_value_get_boolean(value);
    break;
  case PROP_METHOD:
    dec->method = g_value_get_enum(value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void
gst_gzdec_get_property(GObject *object, guint prop_id,
                       GValue *value, GParamSpec *pspec)
{
  GstGzdec *dec = GST_GZDEC(object);

  switch (prop_id)
  {
  case PROP_SILENT:
    g_value_set_boolean(value, dec->silent);
    break;
  case PROP_METHOD:
    g_value_set_enum(value, dec->method);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

/* GstElement vmethod implementations */
static GstFlowReturn process_buffer_zlib(GstGzdec *dec, GstBuffer *buf)
{
  g_return_if_fail(GST_IS_GZDEC(dec));

  GstFlowReturn flow = GST_FLOW_OK;
  GstBuffer *outbuf;
  gint err;

  GstMapInfo inmap = GST_MAP_INFO_INIT, outmap;

  gst_buffer_map(buf, &inmap, GST_MAP_READ);
  dec->stream.next_in = (z_const gchar *)inmap.data;
  dec->stream.avail_in = inmap.size;
  do
  {
    outbuf = gst_buffer_new_and_alloc(DEFAULT_DEC_SIZE);
    gst_buffer_map(outbuf, &outmap, GST_MAP_WRITE);
    dec->stream.next_out = (gchar *)outmap.data;
    dec->stream.avail_out = outmap.size;

    gst_buffer_unmap(outbuf, &outmap);
    err = inflate(&dec->stream, Z_NO_FLUSH);

    if (dec->stream.avail_out >= gst_buffer_get_size(outbuf))
    {
      gst_buffer_unref(outbuf);
      break;
    }

    gst_buffer_resize(outbuf, 0, gst_buffer_get_size(outbuf) - dec->stream.avail_out);
    GST_BUFFER_OFFSET(outbuf) = dec->stream.total_out - gst_buffer_get_size(outbuf);
    GST_DEBUG_OBJECT(dec, "Push data on src pad");

    /* Push data */
    flow = gst_pad_push(dec->srcpad, outbuf);
    if (flow != GST_FLOW_OK)
    {
      break;
    }
  } while (err != Z_STREAM_END);


  gst_buffer_unmap(buf, &inmap);
  gst_buffer_unref(buf);
  return flow;

}

static GstFlowReturn process_buffer_bzlib(GstGzdec *dec, GstBuffer *buf)
{
  g_return_if_fail(GST_IS_GZDEC(dec));
  GstFlowReturn flow = GST_FLOW_OK;
  GstBuffer *outbuf;
  gint err;

  GstMapInfo inmap = GST_MAP_INFO_INIT, outmap;

  gst_buffer_map(buf, &inmap, GST_MAP_READ);
  dec->bz_stream.next_in = (gchar *)inmap.data;
  dec->bz_stream.avail_in = inmap.size;

  do
  {
    /* Create the output buffer */
    outbuf = gst_buffer_new_and_alloc(DEFAULT_DEC_SIZE);

    gst_buffer_map(outbuf, &outmap, GST_MAP_WRITE);
    dec->bz_stream.next_out = (gchar *)outmap.data;
    dec->bz_stream.avail_out = outmap.size;
    err = BZ2_bzDecompress(&dec->bz_stream);
    gst_buffer_unmap(outbuf, &outmap);
    if ((err != BZ_OK) && (err != BZ_STREAM_END))
    {
      GST_ELEMENT_ERROR(dec, STREAM, DECODE, (NULL), ("Failed to decompress data"));
      gst_gzdec_decompress_init(dec);
      gst_buffer_unref(outbuf);
      flow = GST_FLOW_ERROR;
      break;
    }

    if (dec->bz_stream.avail_out >= gst_buffer_get_size(outbuf))
    {
      gst_buffer_unref(outbuf);
      break;
    }
    gst_buffer_resize(outbuf, 0, gst_buffer_get_size(outbuf) - dec->bz_stream.avail_out);
    GST_BUFFER_OFFSET(outbuf) = dec->bz_stream.total_out_lo32 - gst_buffer_get_size(outbuf);

    /* Push data */
    flow = gst_pad_push(dec->srcpad, outbuf);
    if (flow != GST_FLOW_OK)
      break;
  } while (err != BZ_STREAM_END);


  gst_buffer_unmap(buf, &inmap);
  gst_buffer_unref(buf);
  return flow;
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_gzdec_chain(GstPad *pad, GstObject *parent, GstBuffer *buf)
{
  GstFlowReturn flow = GST_FLOW_OK;
  GstGzdec *dec;

  dec = GST_GZDEC(parent);

  if (!dec->ready)
  {
    GST_ELEMENT_ERROR(dec, LIBRARY, FAILED, (NULL), ("Decompressor not ready."));
    flow = GST_FLOW_FLUSHING;
  }
  else
  {
    if (dec->method == ZLIB)
    {
      flow = process_buffer_zlib(dec, buf);
    }
    else
    {
      flow = process_buffer_bzlib(dec, buf);
    }
  }
  return flow;
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
gzdec_init(GstPlugin *gzdec)
{
  /* debug category for filtering log messages
   *
   * exchange the string 'Template gzdec' with your description
   */
  GST_DEBUG_CATEGORY_INIT(gst_gzdec_debug, "gzdec",
                          0, "Gzip decompress");

  return GST_ELEMENT_REGISTER(gzdec, gzdec);
}
/* gstreamer looks for this structure to register gzdecs
 *
 * exchange the string 'Template gzdec' with your gzdec description
 */
GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  gzdec,
                  "gzdec",
                  gzdec_init,
                  PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
