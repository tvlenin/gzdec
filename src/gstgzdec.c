/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2023 Lenin <<user@hostname.org>>
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
 * gst-launch -v -m fakesrc ! gzdec ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>

#include <gst/base/gsttypefindhelper.h>

#include "gstgzdec.h"

#include <zlib.h>

GST_DEBUG_CATEGORY_STATIC (gst_gzdec_debug);
#define GST_CAT_DEFAULT gst_gzdec_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SILENT
};


struct _GstGzdec
{
    GstElement parent;

    GstPad *sinkpad, *srcpad;

    gboolean silent;
    gboolean ready;
    z_stream stream;
    guint64 offset;

    GstFlowReturn(* encode)(GstGzdec *, GstBuffer *, GstBuffer **);

};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

#define gst_gzdec_parent_class parent_class
G_DEFINE_TYPE (GstGzdec, gst_gzdec, GST_TYPE_ELEMENT);

GST_ELEMENT_REGISTER_DEFINE (gzdec, "gzdec", GST_RANK_NONE,
    GST_TYPE_GZDEC);

static void gst_gzdec_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_gzdec_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static GstFlowReturn zlib_encode (GstGzdec * filter,
    GstBuffer * inbuf, GstBuffer ** outbuf);
static gboolean gst_gzdec_sink_event (GstPad * pad,
    GstObject * parent, GstEvent * event);
static GstFlowReturn gst_gzdec_chain (GstPad * pad,
    GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */


    static void
gst_gzdec_decompress_end (GstGzdec * dec)
{
    g_return_if_fail (GST_IS_GZDEC (dec));

    if (dec->ready)
    {
        GST_DEBUG_OBJECT (dec, "Finalize gzdec decompressing feature");
        (void)inflateEnd(&dec->stream);
        memset (&dec->stream, 0, sizeof (dec->stream));
        dec->ready = FALSE;
    }
}

    static void
gst_gzdec_finalize (GObject * object)
{

    GstGzdec *dec = GST_GZDEC (object);
    GST_DEBUG_OBJECT (dec, "Finalize gzdec");
    gst_gzdec_decompress_end (dec);

    G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void gst_gzdec_decompress_init (GstGzdec *dec){
  gint ret;
    g_print ("Decompress init\n");

  g_return_if_fail (GST_IS_GZDEC (dec));
      gst_gzdec_decompress_end (dec);

  /* allocate inflate state */
  dec->stream.zalloc = Z_NULL;
  dec->stream.zfree = Z_NULL;
  dec->stream.opaque = Z_NULL;
  dec->stream.avail_in = 0;
  dec->stream.next_in = Z_NULL;

  ret = inflateInit2(&dec->stream, 32); ///////////////////////////////TODO
  dec->ready = TRUE;
  dec->offset = 0;



  return;
}

    static GstStateChangeReturn
gst_gzdec_change_state (GstElement * element, GstStateChange transition)
{
    GstGzdec *dec = GST_GZDEC (element);
    GstStateChangeReturn ret;
        g_print ("Changing state\n");

    GST_DEBUG_OBJECT (dec, "Changing gzdec state");
    ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
    if (ret != GST_STATE_CHANGE_SUCCESS)
        return ret;

    switch (transition) {
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            g_print ("PAUSED TO READY.\n");

            gst_gzdec_decompress_init (dec);
            break;
        default:
            break;
    }
    return ret;
}


/* initialize the gzdec's class */
static void
gst_gzdec_class_init (GstGzdecClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_gzdec_change_state);
  gobject_class->finalize = gst_gzdec_finalize;

  gobject_class->set_property = gst_gzdec_set_property;
  gobject_class->get_property = gst_gzdec_get_property;

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));

  gst_element_class_set_details_simple (gstelement_class,
      "gzdec",
      "FIXME:Generic",
      "FIXME:Generic Template Element", "Lenin Torres <<ttvleninn@gmail.com>>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));
      
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad callback functions
 * initialize instance structure
 */
static void
gst_gzdec_init (GstGzdec * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad,
      GST_DEBUG_FUNCPTR (gst_gzdec_sink_event));
  gst_pad_set_chain_function (filter->sinkpad,
      GST_DEBUG_FUNCPTR (gst_gzdec_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);
  filter->encode       = zlib_encode;
  gst_gzdec_decompress_init (filter);

  filter->silent = FALSE;
  
}

static void
gst_gzdec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstGzdec *filter = GST_GZDEC (object);

  switch (prop_id) {
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_gzdec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstGzdec *filter = GST_GZDEC (object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */

/* this function handles sink events */
static gboolean
gst_gzdec_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstGzdec *filter;
  gboolean ret;

  filter = GST_GZDEC (parent);

  GST_LOG_OBJECT (filter, "Received %s event: %" GST_PTR_FORMAT,
      GST_EVENT_TYPE_NAME (event), event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;

      gst_event_parse_caps (event, &caps);
      /* do something with the caps */

      /* and forward */
      ret = gst_pad_event_default (pad, parent, event);
      break;
    }
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  return ret;
}

static GstFlowReturn
zlib_encode (GstGzdec * filter,
    GstBuffer * inbuf, GstBuffer ** outbuf)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstMemory * memory_block;
  GstMapInfo map_info_in, map_info_out;
  int zlib_status = Z_OK;
    
  if (! gst_buffer_map(inbuf, &map_info_in, GST_MAP_READ))
    return GST_FLOW_ERROR;

  filter->stream.avail_in = map_info_in.size;
  filter->stream.next_in  = map_info_in.data;

  *outbuf = gst_buffer_new();
  if (outbuf == NULL)
    goto no_buffer;
  
  GST_BUFFER_OFFSET (*outbuf) = filter->stream.total_out;
  gst_buffer_copy_into(inbuf, *outbuf, GST_BUFFER_COPY_METADATA, 0, -1);
  g_print("New Buffer\n");
  while (filter->stream.avail_in > 0 && zlib_status != Z_STREAM_END) {
    g_print("Available = %d \n",filter->stream.avail_in );
    memory_block = gst_allocator_alloc(NULL, 4096, NULL);

    if (! gst_memory_map(memory_block, &map_info_out, GST_MAP_WRITE))
      goto map_error;

    filter->stream.avail_out = map_info_out.size;
    filter->stream.next_out  = map_info_out.data;

    zlib_status = inflate(&filter->stream, Z_NO_FLUSH);
    switch (zlib_status) {
    case Z_OK:
    case Z_STREAM_END:
    case Z_BUF_ERROR:
      break;
    default:
      inflateEnd(&filter->stream);
      goto decompress_error;
    }

    gst_memory_unmap(memory_block, &map_info_out);
    gst_memory_resize(memory_block, 0, 4096 - filter->stream.avail_out);
    
    gst_buffer_append_memory(*outbuf, memory_block);
  }

  gst_buffer_unmap(inbuf, &map_info_in);

  if (zlib_status == Z_STREAM_END)
    return GST_FLOW_EOS;
  
  return GST_FLOW_OK;

 no_buffer:
  {
    gst_buffer_unmap(inbuf, &map_info_in);
    GST_WARNING_OBJECT (filter, "could not allocate buffer");
    return ret;
  }

 map_error:
  {
    gst_buffer_unmap(inbuf, &map_info_in);
    GST_WARNING_OBJECT (filter, "could not map memory object");
    return ret;
  }

 decompress_error:
  {
    gst_memory_unmap(memory_block, &map_info_out);
    gst_buffer_unmap(inbuf, &map_info_in);
    GST_WARNING_OBJECT (filter, "could not decompress stream: ZLIB_ERROR(%d)",
         zlib_status);
    return ret;
  }
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_gzdec_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstFlowReturn flow = GST_FLOW_OK;
  GstBuffer *out;
  GstGzdec *dec;
  int err = Z_OK;

  dec = GST_GZDEC (parent);

  if (dec->silent == FALSE)
    g_print ("I'm plugged, chain .\n");
  GstMapInfo inmap = GST_MAP_INFO_INIT, outmap;

  if (!dec->ready){
    GST_ELEMENT_ERROR (dec, LIBRARY, FAILED, (NULL), ("Decompressor not ready."));
    flow = GST_FLOW_FLUSHING;
  } else {

    gst_buffer_map (buf, &inmap, GST_MAP_READ);
    dec->stream.next_in = (z_const Bytef *) inmap.data;
    dec->stream.avail_in = inmap.size;
    g_print("Size of in = %d \n", dec->stream.avail_in);
    GST_DEBUG_OBJECT (dec, "Input buffer size : dec->stream.avail_in = %d", dec->stream.avail_in);
        do
        {
            g_print("DO \n");

            guint have;
            //out = gst_buffer_new_and_alloc (dec->offset ? 1024*256: 1024*256);
            out = gst_buffer_new_and_alloc (10);
            gst_buffer_map (out, &outmap, GST_MAP_WRITE);
            dec->stream.next_out = (Bytef *) outmap.data;
            dec->stream.avail_out = outmap.size;

            gst_buffer_unmap (out, &outmap);
             err = inflate (&dec->stream, Z_NO_FLUSH);
        switch (err)
        {
            case Z_OK:
                g_print( "decodes correctley Z_OK [%s] \n", dec->stream.msg);
                break;
            case Z_STREAM_END:
                g_print( "decoder ended Z_STREAM_END [%s] \n", dec->stream.msg);
                break;
            case Z_BUF_ERROR:
                g_print( "decoder error: Z_BUF_ERROR [%s] \n", dec->stream.msg);
                break;
            default:
                g_print( "decoder error: unknow code (%d) [%s] \n", err, dec->stream.msg);
                break;
        }

            g_print("Avail_out = %d \n Size_out = %ld\n",dec->stream.avail_out , gst_buffer_get_size (out) );
            if (dec->stream.avail_out >= gst_buffer_get_size (out))
                {
                    gst_buffer_unref (out);
                    break;
                }

            gst_buffer_resize (out, 0, gst_buffer_get_size (out) - dec->stream.avail_out);
            GST_BUFFER_OFFSET (out) = dec->stream.total_out - gst_buffer_get_size (out);


            GST_DEBUG_OBJECT (dec, "Push data on src pad");
            have = gst_buffer_get_size (out);

            flow = gst_pad_push (dec->srcpad, out);
            if (flow != GST_FLOW_OK)
            {
                break;
            } 
            dec->offset += have;
        } while (err != Z_STREAM_END);
    }

    gst_buffer_unmap (buf, &inmap);
    gst_buffer_unref (buf);
    g_print("Buffer Sent\n\n");
    return flow; 

}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
gzdec_init (GstPlugin * gzdec)
{
  /* debug category for filtering log messages
   *
   * exchange the string 'Template gzdec' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_gzdec_debug, "gzdec",
      0, "Template gzdec");

  return GST_ELEMENT_REGISTER (gzdec, gzdec);
}

/* PACKAGE: this is usually set by meson depending on some _INIT macro
 * in meson.build and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use meson to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirstgzdec"
#endif

/* gstreamer looks for this structure to register gzdecs
 *
 * exchange the string 'Template gzdec' with your gzdec description
 */
GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    gzdec,
    "gzdec",
    gzdec_init,
    PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)