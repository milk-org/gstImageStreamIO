/* GStreamer
 * Copyright (C) 2018 FIXME <fixme@example.com>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA..
 */
/**
 * SECTION:element-gstImageStreamsrc
 *
 * The gstImageStreamsrc element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! gstImageStreamsrc ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <glib/gstdio.h>
#include <math.h>
#include "/home/brule/cacao/ImageStreamIO/ImageStruct.h"
#include "/home/brule/cacao/ImageStreamIO/ImageStreamIO.h"
#include "gstImageStreamsrc.h"

GST_DEBUG_CATEGORY_STATIC (gst_gstImageStreamsrc_debug_category);
#define GST_CAT_DEFAULT gst_gstImageStreamsrc_debug_category
#define DEFAULT_TIMESTAMP_OFFSET   0
#define DEFAULT_SHM_PATH			"imtest00"
#define DEFAULT_SHM_OFFSET			0
#define DEFAULT_SHM_TYPE			_DATATYPE_FLOAT

/* prototypes */
static void gst_gstImageStreamsrc_set_property (GObject * object,
		guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_gstImageStreamsrc_get_property (GObject * object,
		guint property_id, GValue * value, GParamSpec * pspec);
static void gst_gstImageStreamsrc_dispose (GObject * object);
static void gst_gstImageStreamsrc_finalize (GObject * object);

static GstCaps *gst_gstImageStreamsrc_get_caps (GstBaseSrc * src, GstCaps * filter);
static gboolean gst_gstImageStreamsrc_negotiate (GstBaseSrc * src);
static GstCaps *gst_gstImageStreamsrc_fixate (GstBaseSrc * src, GstCaps * caps);
static gboolean gst_gstImageStreamsrc_set_caps (GstBaseSrc * src, GstCaps * caps);
static gboolean gst_gstImageStreamsrc_decide_allocation (GstBaseSrc * src,
		GstQuery * query);
static gboolean gst_gstImageStreamsrc_start (GstBaseSrc * src);
static gboolean gst_gstImageStreamsrc_stop (GstBaseSrc * src);
/*static void gst_gstImageStreamsrc_get_times (GstBaseSrc * src, GstBuffer * buffer,
		GstClockTime * start, GstClockTime * end);*/
static gboolean gst_gstImageStreamsrc_get_size (GstBaseSrc * src, guint64 * size);
static gboolean gst_gstImageStreamsrc_is_seekable (GstBaseSrc * src);
static gboolean gst_gstImageStreamsrc_prepare_seek_segment (GstBaseSrc * src,
		GstEvent * seek, GstSegment * segment);
static gboolean gst_gstImageStreamsrc_do_seek (GstBaseSrc * src, GstSegment * segment);
static gboolean gst_gstImageStreamsrc_unlock (GstBaseSrc * src);
static gboolean gst_gstImageStreamsrc_unlock_stop (GstBaseSrc * src);
static gboolean gst_gstImageStreamsrc_query (GstBaseSrc * src, GstQuery * query);
static gboolean gst_gstImageStreamsrc_event (GstBaseSrc * src, GstEvent * event);
static GstFlowReturn gst_gstImageStreamsrc_create (GstPushSrc * src, GstBuffer ** buf);

enum
{
	PROP_0,
	PROP_IS_LIVE,
	PROP_SHM_AREA_NAME,
	PROP_SHM_OFFSET,
	PROP_SHM_TYPE,
	PROP_TIMESTAMP_OFFSET

};

/* pad templates */
static GstStaticPadTemplate gst_gstImageStreamsrc_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
	GST_PAD_SRC,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS ( "video/x-raw,format=(string) GRAY16_LE, width = 512,height = 512,framerate = 60/1;" )
);

#define gst_gstImageStreamsrc_parent_class parent_class

/* class initialization */
G_DEFINE_TYPE_WITH_CODE (GstImageStreamsrc, gst_gstImageStreamsrc, GST_TYPE_PUSH_SRC,
	GST_DEBUG_CATEGORY_INIT (gst_gstImageStreamsrc_debug_category, "gstImageStreamsrc", 0,
	"debug category for gstImageStreamsrc element")
);

static void
gst_gstImageStreamsrc_class_init (GstImageStreamsrcClass * klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);
	GstBaseSrcClass *base_src_class = GST_BASE_SRC_CLASS (klass);
	GstPushSrcClass *push_src_class = GST_PUSH_SRC_CLASS (klass);

	/* Setting up pads and setting metadata should be moved to
	   base_class_init if you intend to subclass this class. */
	gst_element_class_add_static_pad_template (gstelement_class,
			&gst_gstImageStreamsrc_src_template);

	gst_element_class_set_static_metadata (gstelement_class,
			"FIXME Long name", "Generic", "FIXME Description",
			"FIXME <fixme@example.com>");

	gobject_class->set_property = gst_gstImageStreamsrc_set_property;
	gobject_class->get_property = gst_gstImageStreamsrc_get_property;
	gobject_class->dispose = gst_gstImageStreamsrc_dispose;
	gobject_class->finalize = gst_gstImageStreamsrc_finalize;

	g_object_class_install_property (gobject_class, PROP_SHM_AREA_NAME,
			g_param_spec_string ("shm-name", "Name of the shared memory area",
				"location of the shmfile", DEFAULT_SHM_PATH, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, PROP_SHM_OFFSET,
			g_param_spec_int ("shmoffset", "location of the payload in the shmem",
				"offset to find the data", DEFAULT_SHM_OFFSET, G_MAXINT,0, G_PARAM_READWRITE ));

	g_object_class_install_property (gobject_class, PROP_SHM_TYPE,
			g_param_spec_int ("shm-type", "type of the shared memory data",
				"type of the data", DEFAULT_SHM_TYPE, G_MAXINT,0, G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_TIMESTAMP_OFFSET,
			g_param_spec_int64 ("timestamp-offset", "Timestamp offset",
				"An offset added to timestamps set on buffers (in ns)", 0,
				(G_MAXLONG == G_MAXINT64) ? G_MAXINT64 : (G_MAXLONG * GST_SECOND - 1),
				0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	base_src_class->get_caps = GST_DEBUG_FUNCPTR (gst_gstImageStreamsrc_get_caps);
	base_src_class->negotiate = GST_DEBUG_FUNCPTR (gst_gstImageStreamsrc_negotiate);
	base_src_class->fixate = GST_DEBUG_FUNCPTR (gst_gstImageStreamsrc_fixate);
	base_src_class->set_caps = GST_DEBUG_FUNCPTR (gst_gstImageStreamsrc_set_caps);
	base_src_class->decide_allocation =
		GST_DEBUG_FUNCPTR (gst_gstImageStreamsrc_decide_allocation);
	base_src_class->start = GST_DEBUG_FUNCPTR (gst_gstImageStreamsrc_start);
	base_src_class->stop = GST_DEBUG_FUNCPTR (gst_gstImageStreamsrc_stop);
/*	base_src_class->get_times = GST_DEBUG_FUNCPTR (gst_gstImageStreamsrc_get_times);*/
	base_src_class->get_size = GST_DEBUG_FUNCPTR (gst_gstImageStreamsrc_get_size);
	base_src_class->is_seekable = GST_DEBUG_FUNCPTR (gst_gstImageStreamsrc_is_seekable);
	base_src_class->prepare_seek_segment =
		GST_DEBUG_FUNCPTR (gst_gstImageStreamsrc_prepare_seek_segment);
	base_src_class->do_seek = GST_DEBUG_FUNCPTR (gst_gstImageStreamsrc_do_seek);
	base_src_class->unlock = GST_DEBUG_FUNCPTR (gst_gstImageStreamsrc_unlock);
	base_src_class->unlock_stop = GST_DEBUG_FUNCPTR (gst_gstImageStreamsrc_unlock_stop);
	base_src_class->query = GST_DEBUG_FUNCPTR (gst_gstImageStreamsrc_query);
	base_src_class->event = GST_DEBUG_FUNCPTR (gst_gstImageStreamsrc_event);
	push_src_class->create = GST_DEBUG_FUNCPTR (gst_gstImageStreamsrc_create);
}

	static void
gst_gstImageStreamsrc_init (GstImageStreamsrc * gstImageStreamsrc)
{

	int BUFF_SIZE=512*512*sizeof(uint16_t);

	GstBaseSrc *src = GST_BASE_SRC (gstImageStreamsrc);

	gstImageStreamsrc->shmpathname=DEFAULT_SHM_PATH;
	gstImageStreamsrc->shmoffset=DEFAULT_SHM_OFFSET;
	gstImageStreamsrc->shmsize=BUFF_SIZE;
	gstImageStreamsrc->fixed_caps=NULL;
	gstImageStreamsrc->height=512;
	gstImageStreamsrc->width=512;
	gstImageStreamsrc->timestamp_offset = DEFAULT_TIMESTAMP_OFFSET;
	gstImageStreamsrc->shmtype = DEFAULT_SHM_TYPE;

	/* open mmap en imageSteamIO */
	gstImageStreamsrc->image=malloc(sizeof(IMAGE));
	ImageStreamIO_read_sharedmem_image_toIMAGE(gstImageStreamsrc->shmpathname,gstImageStreamsrc->image);

	gst_pad_use_fixed_caps (GST_BASE_SRC_PAD (gstImageStreamsrc));
	gst_base_src_set_live (src, TRUE);
	gst_base_src_set_do_timestamp(src,TRUE);
	gst_base_src_set_format (src, GST_FORMAT_TIME);

}

	void
gst_gstImageStreamsrc_set_property (GObject * object, guint property_id,
		const GValue * value, GParamSpec * pspec)
{
	GstImageStreamsrc *gstImageStreamsrc = GST_GSTIMAGESTREAMSRC (object);

	GST_DEBUG_OBJECT (gstImageStreamsrc, "set_property");

	switch (property_id) {
		case PROP_SHM_AREA_NAME:
			g_free (gstImageStreamsrc->shmpathname);
			/* check if we are currently active
			   prevent setting camera and other values to something not representing the active camera */
			/*if stream*/ 
			gstImageStreamsrc->shmpathname = g_strdup(g_value_get_string (value));
			GST_LOG_OBJECT (gstImageStreamsrc, "Set shm path name to %s", gstImageStreamsrc->shmpathname);
			break;

		case PROP_TIMESTAMP_OFFSET:
			gstImageStreamsrc->timestamp_offset = g_value_get_int64(value);
			break;

		case PROP_SHM_OFFSET:
			gstImageStreamsrc->shmoffset = g_value_get_int(value);
			break;

		case PROP_SHM_TYPE:
			gstImageStreamsrc->shmtype = g_value_get_int(value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
			break;
	}
}

void
gst_gstImageStreamsrc_get_property (GObject * object, guint property_id,
		GValue * value, GParamSpec * pspec)
{
	GstImageStreamsrc *gstImageStreamsrc = GST_GSTIMAGESTREAMSRC (object);

	GST_DEBUG_OBJECT (gstImageStreamsrc, "get_property");

	switch (property_id) {
		case PROP_SHM_AREA_NAME:
			g_value_set_string (value, gstImageStreamsrc->shmpathname);
			GST_LOG_OBJECT (gstImageStreamsrc, "Set shm path name to %s", gstImageStreamsrc->shmpathname);
			break;

		case PROP_SHM_OFFSET:
			g_value_set_int (value, gstImageStreamsrc->shmoffset);
			GST_LOG_OBJECT (gstImageStreamsrc, "Set shm offset to %lu",gstImageStreamsrc->shmoffset);
			break;

		case PROP_SHM_TYPE:
			g_value_set_int (value, gstImageStreamsrc->shmtype);
			GST_LOG_OBJECT (gstImageStreamsrc, "Set shm type name to %d", gstImageStreamsrc->shmtype);
			break;

		case PROP_TIMESTAMP_OFFSET:
			g_value_set_int64 (value, gstImageStreamsrc->timestamp_offset);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

	void
gst_gstImageStreamsrc_dispose (GObject * object)
{
	GstImageStreamsrc *gstImageStreamsrc = GST_GSTIMAGESTREAMSRC (object);

	GST_DEBUG_OBJECT (gstImageStreamsrc, "dispose");

	/* clean up as possible.  may be called multiple times */

	G_OBJECT_CLASS (gst_gstImageStreamsrc_parent_class)->dispose (object);
}

	void
gst_gstImageStreamsrc_finalize (GObject * object)
{
	GstImageStreamsrc *gstImageStreamsrc = GST_GSTIMAGESTREAMSRC (object);

	GST_DEBUG_OBJECT (gstImageStreamsrc, "finalize");

	if (gstImageStreamsrc->fixed_caps != NULL) {
		gst_caps_unref (gstImageStreamsrc->fixed_caps);
		gstImageStreamsrc->fixed_caps = NULL;
	}
	g_free(gstImageStreamsrc->frame);
	g_free(gstImageStreamsrc->image);
	/*	g_free (gstImageStreamsrc->shmpathname);*/
	gstImageStreamsrc->shmpathname = NULL;

	/* clean up object here */

	G_OBJECT_CLASS (gst_gstImageStreamsrc_parent_class)->finalize (object);
}

/* get caps from subclass */
	static GstCaps *
gst_gstImageStreamsrc_get_caps (GstBaseSrc * src, GstCaps * filter)
{
	GstImageStreamsrc *gstImageStreamsrc = GST_GSTIMAGESTREAMSRC (src);
	GstCaps *caps;

	GST_DEBUG_OBJECT (gstImageStreamsrc, "get_caps");

	if (gstImageStreamsrc->fixed_caps != NULL)
		caps = gst_caps_copy (gstImageStreamsrc->fixed_caps);
	else
		caps = gst_caps_new_simple ("video/x-raw","format",G_TYPE_STRING,"GRAY16_LE","width",G_TYPE_INT,512,"height",G_TYPE_INT,512,"framerate",GST_TYPE_FRACTION, 60, 1,NULL);

	GST_LOG_OBJECT (gstImageStreamsrc, "Available caps = %" GST_PTR_FORMAT, caps);

	return caps;
}

/* decide on caps */
	static gboolean
gst_gstImageStreamsrc_negotiate (GstBaseSrc * src)
{

	GstCaps *caps;
	GstVideoInfo vinfo;
	GstVideoFormat vformat;
	gchar *stream_id = NULL;
	GstEvent *stream_start = NULL;

	GstImageStreamsrc *gstImageStreamsrc = GST_GSTIMAGESTREAMSRC (src);

	GST_DEBUG_OBJECT (gstImageStreamsrc, "negotiate");

	stream_id = gst_pad_create_stream_id (GST_BASE_SRC_PAD (src), GST_ELEMENT (src), "RAWSHMSRC");
	stream_start = gst_event_new_stream_start (stream_id);
	g_free (stream_id);
	gst_pad_push_event (GST_BASE_SRC_PAD (src), stream_start);

	vformat = gst_video_format_from_string ("GRAY16_LE");

	gst_video_info_init (&vinfo);

	gst_video_info_set_format (&vinfo, vformat, gstImageStreamsrc->height,gstImageStreamsrc->width);

	gstImageStreamsrc->frame = g_malloc (vinfo.size);

	caps = gst_video_info_to_caps (&vinfo);

	gst_base_src_set_caps (src, caps);

	gst_caps_unref (caps);

	return TRUE;
}

/* called if, in negotiation, caps need fixating */
	static GstCaps *
gst_gstImageStreamsrc_fixate (GstBaseSrc * src, GstCaps * caps)
{
	GstImageStreamsrc *gstImageStreamsrc = GST_GSTIMAGESTREAMSRC (src);
	/*
	   GstStructure *structure;
	   caps = gst_caps_make_writable (caps);
	   structure = gst_caps_get_structure (caps, 0);
	   gst_structure_fixate_field_nearest_int (structure, "width", 512);
	   gst_structure_fixate_field_nearest_int (structure, "height", 512);
	   caps = GST_BASE_SRC_CLASS (gst_rawshm_parent_class)->fixate (src, caps);
	   GST_LOG ("caps are %" GST_PTR_FORMAT, caps);
	   return caps;
	   */
	GST_DEBUG_OBJECT (gstImageStreamsrc, "fixate");
	/*GST_BASE_SRC_CLASS(gstImageStreamsrc_parent_class)->fixate(src, caps);*/
	return NULL;
}

/* notify the subclass of new caps */
	static gboolean
gst_gstImageStreamsrc_set_caps (GstBaseSrc * src, GstCaps * caps)
{

	GstVideoInfo info;

	GstImageStreamsrc *gstImageStreamsrc = GST_GSTIMAGESTREAMSRC (src);

	GST_DEBUG_OBJECT (gstImageStreamsrc, "set_caps");
	GST_LOG_OBJECT (gstImageStreamsrc, "Requested caps = %" GST_PTR_FORMAT, caps);

	gst_video_info_init (&info);


	/* https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gst-plugins-base-libs/html/gst-plugins-base-libs-GstVideoAlignment.html#GstVideoFormatInfo*/
	info.finfo = gst_video_format_get_info (GST_VIDEO_FORMAT_GRAY16_LE);

	gstImageStreamsrc->vinfo = info;

	gstImageStreamsrc->frame = g_malloc (gstImageStreamsrc->vinfo.size);

	gstImageStreamsrc->accum_rtime += gstImageStreamsrc->running_time;
	gstImageStreamsrc->accum_frames += gstImageStreamsrc->n_frames;

	gstImageStreamsrc->running_time = 0;
	gstImageStreamsrc->n_frames = 0;

	/*start stream ?*/
	return TRUE;
}

/* setup allocation query */
	static gboolean
gst_gstImageStreamsrc_decide_allocation (GstBaseSrc * src, GstQuery * query)
{
	GstImageStreamsrc *gstImageStreamsrc = GST_GSTIMAGESTREAMSRC (src);
	GstBufferPool *pool = NULL;
	guint size, min = 1, max = 0;
	GstStructure *config;
	GstCaps *caps;
	GstVideoInfo info;
	gboolean ret;

	GST_DEBUG_OBJECT (gstImageStreamsrc, "decide_allocation");

	/* Parse an allocation query, 
	   writing the requested caps in caps and whether a pool is needed in need_pool , 
	   if the respective parameters are non-NULL.*/
	gst_query_parse_allocation (query, &caps, NULL);

	if (!caps || !gst_video_info_from_caps (&info, caps))
		return FALSE;

	/* Retrieve the number of values currently stored in the pool array of the query's structure.*/
	while (gst_query_get_n_allocation_pools (query) > 0) {

		/*Get the pool parameters in query*/
		gst_query_parse_nth_allocation_pool (query, 0, &pool, &size, &min, &max);

		/* TODO We restrict to the exact size as we don't support strides or
		 * special padding */
		if (size == info.size)
			break;

		/*Remove the allocation pool at index of the allocation pool array*/
		gst_query_remove_nth_allocation_pool (query, 0);
		gst_object_unref (pool);
		pool = NULL;
	}

	if (pool == NULL) {
		/* we did not get a pool, make one ourselves then */
		/* Create a new bufferpool that can allocate video frames */
		pool = gst_video_buffer_pool_new ();
		size = info.size;
		min = 1;
		max = 0;
		/* Set the pool parameters in query */
		gst_query_add_allocation_pool (query, pool, size, min, max);
	}

	gstImageStreamsrc->pool=pool;
	/* Get a copy of the current configuration of the pool */
	config = gst_buffer_pool_get_config (pool);

	/*Configure config with the given parameters*/
	gst_buffer_pool_config_set_params (config, caps, size, min, max);

	GST_DEBUG_OBJECT (gstImageStreamsrc, "decide_allocation size %d",size);

	/*Set the configuration of the pool.
	  If the pool is already configured, and the configuration haven't change, this function will return TRUE. 
	  If the pool is active, this method will return FALSE and active configuration will remain. 
	  Buffers allocated form this pool must be returned or else this function will do nothing and return FALSE.*/
	ret = gst_buffer_pool_set_config (pool, config);

	GST_DEBUG_OBJECT (gstImageStreamsrc, "decide_allocation change? %d",ret);

	/*ajout??*/
	gst_buffer_pool_set_active (pool, TRUE);

	gst_object_unref (pool);

	gstImageStreamsrc->accum_rtime += gstImageStreamsrc->running_time;
	gstImageStreamsrc->accum_frames += gstImageStreamsrc->n_frames;

	gstImageStreamsrc->running_time = 0;
	gstImageStreamsrc->n_frames = 0;

	return ret;
	/*	return TRUE;*/
}

/* start and stop processing, ideal for opening/closing the resource */
	static gboolean
gst_gstImageStreamsrc_start (GstBaseSrc * src)
{
	GstImageStreamsrc *gstImageStreamsrc = GST_GSTIMAGESTREAMSRC (src);

	GST_DEBUG_OBJECT (gstImageStreamsrc, "start");

	gstImageStreamsrc->running_time = 0;
	gstImageStreamsrc->n_frames = 0;
	gstImageStreamsrc->accum_frames = 0;
	gstImageStreamsrc->accum_rtime = 0;

	gst_base_src_set_live(src,TRUE);
/*	gst_video_info_init (&gstImageStreamsrc->vinfo);*/

	return TRUE;
}

	static gboolean
gst_gstImageStreamsrc_stop (GstBaseSrc * src)
{
	GstImageStreamsrc *gstImageStreamsrc = GST_GSTIMAGESTREAMSRC (src);

	GST_DEBUG_OBJECT (gstImageStreamsrc, "stop");

	return TRUE;
}


#if 0
/* given a buffer, return start and stop time when it should be pushed
 * out. The base class will sync on the clock using these times. */
	static void
gst_gstImageStreamsrc_get_times (GstBaseSrc * src, GstBuffer * buffer,
		GstClockTime * start, GstClockTime * end)
{
	GstImageStreamsrc *gstImageStreamsrc = GST_GSTIMAGESTREAMSRC (src);

	GST_DEBUG_OBJECT (gstImageStreamsrc, "get_times");

	if (buffer == NULL)
		GST_DEBUG_OBJECT (gstImageStreamsrc, "buffer is null");

	/* for live sources, sync on the timestamp of the buffer */
	if (gst_base_src_is_live (src)) {
		GstClockTime timestamp = GST_BUFFER_PTS (buffer);
/*		GstClockTime timestamp = GST_BUFFER_TIMESTAMP (buffer);*/

		GST_DEBUG_OBJECT (gstImageStreamsrc, "PTS %"GST_TIME_FORMAT,GST_TIME_ARGS (GST_BUFFER_PTS (buffer)));
		GST_DEBUG_OBJECT (gstImageStreamsrc, "timestamp %"GST_TIME_FORMAT,GST_TIME_ARGS (timestamp));

		if (GST_CLOCK_TIME_IS_VALID (timestamp)) {
			/* get duration to calculate end time */
			GstClockTime duration = GST_BUFFER_DURATION (buffer);
			GST_DEBUG_OBJECT (gstImageStreamsrc, "is valid");

			if (GST_CLOCK_TIME_IS_VALID (duration)) {
				*end = timestamp + duration;
				GST_DEBUG_OBJECT (gstImageStreamsrc, "alid");
			}
			*start = timestamp;
		
		} else {
			GST_DEBUG_OBJECT (gstImageStreamsrc, "is not valide");
			*start = -1;
			*end = -1;
		}
	}

	GST_DEBUG_OBJECT (gstImageStreamsrc, "fin get_times");

}
#endif

/* get the total size of the resource in bytes */
	static gboolean
gst_gstImageStreamsrc_get_size (GstBaseSrc * src, guint64 * size)
{
	GstImageStreamsrc *gstImageStreamsrc = GST_GSTIMAGESTREAMSRC (src);

	GST_DEBUG_OBJECT (gstImageStreamsrc, "get_size");

	return TRUE;
}

/* check if the resource is seekable */
	static gboolean
gst_gstImageStreamsrc_is_seekable (GstBaseSrc * src)
{
	GstImageStreamsrc *gstImageStreamsrc = GST_GSTIMAGESTREAMSRC (src);

	GST_DEBUG_OBJECT (gstImageStreamsrc, "is_seekable");

	return TRUE;
}

/* Prepare the segment on which to perform do_seek(), converting to the
 * current basesrc format. */
	static gboolean
gst_gstImageStreamsrc_prepare_seek_segment (GstBaseSrc * src, GstEvent * seek,
		GstSegment * segment)
{
	GstImageStreamsrc *gstImageStreamsrc = GST_GSTIMAGESTREAMSRC (src);

	GST_DEBUG_OBJECT (gstImageStreamsrc, "prepare_seek_segment");

	return TRUE;
}

/* notify subclasses of a seek */
	static gboolean
gst_gstImageStreamsrc_do_seek (GstBaseSrc * src, GstSegment * segment)
{
	GstImageStreamsrc *gstImageStreamsrc = GST_GSTIMAGESTREAMSRC (src);

	GST_DEBUG_OBJECT (gstImageStreamsrc, "do_seek");

	gstImageStreamsrc->n_frames = 0;

	gstImageStreamsrc->accum_frames = 0;
	gstImageStreamsrc->accum_rtime = 0;
	/* FIXME : Not sure what to set here */
	gstImageStreamsrc->running_time = 0;

	return TRUE;
}

/* unlock any pending access to the resource. subclasses should unlock
 * any function ASAP. */
	static gboolean
gst_gstImageStreamsrc_unlock (GstBaseSrc * src)
{
	GstImageStreamsrc *gstImageStreamsrc = GST_GSTIMAGESTREAMSRC (src);

	GST_DEBUG_OBJECT (gstImageStreamsrc, "unlock");

	return TRUE;
}

/* Clear any pending unlock request, as we succeeded in unlocking */
	static gboolean
gst_gstImageStreamsrc_unlock_stop (GstBaseSrc * src)
{
	GstImageStreamsrc *gstImageStreamsrc = GST_GSTIMAGESTREAMSRC (src);

	GST_DEBUG_OBJECT (gstImageStreamsrc, "unlock_stop");

	return TRUE;
}

/* notify subclasses of a query */
	static gboolean
gst_gstImageStreamsrc_query (GstBaseSrc * src, GstQuery * query)
{

	gboolean res = FALSE;
	GstImageStreamsrc *gstImageStreamsrc = GST_GSTIMAGESTREAMSRC (src);
	GST_DEBUG_OBJECT (gstImageStreamsrc, "query");

	switch (GST_QUERY_TYPE (query)) {
		case GST_QUERY_CONVERT:
			{
				/*   GstFormat src_fmt, dest_fmt;
					 gint64 src_val, dest_val;
					 gst_query_parse_convert (query, &src_fmt, &src_val, &dest_fmt, &dest_val);
					 res =
					 gst_video_info_convert (&src->info, src_fmt, src_val, dest_fmt,
					 &dest_val);
					 gst_query_set_convert (query, src_fmt, src_val, dest_fmt, dest_val);*/
				break;
			}
		case GST_QUERY_LATENCY:
			{
				/*      if (src->info.fps_n > 0) {
						GstClockTime latency;
						latency =
						gst_util_uint64_scale (GST_SECOND, src->info.fps_d,
						src->info.fps_n);
						gst_query_set_latency (query,
						gst_base_src_is_live (GST_BASE_SRC_CAST (src)), latency,
						GST_CLOCK_TIME_NONE);
						GST_DEBUG_OBJECT (src, "Reporting latency of %" GST_TIME_FORMAT,
						GST_TIME_ARGS (latency));
						res = TRUE;
						}*/
				break;
			}
		case GST_QUERY_DURATION:{
				/*  if (bsrc->num_buffers != -1) {
					GstFormat format;
					gst_query_parse_duration (query, &format, NULL);
					switch (format) {
					case GST_FORMAT_TIME:{
					gint64 dur = gst_util_uint64_scale_int_round (bsrc->num_buffers
				 * GST_SECOND, src->info.fps_d, src->info.fps_n);
				 res = TRUE;
				 gst_query_set_duration (query, GST_FORMAT_TIME, dur);
				 goto done;
				 }
				 case GST_FORMAT_BYTES:{
				 res = TRUE;
				 gst_query_set_duration (query, GST_FORMAT_BYTES,
				 bsrc->num_buffers * src->info.size);
				 goto done;
				 default:
				 break;
				 }
				 }
				 } */
				break;
		}

		default:
								res = GST_BASE_SRC_CLASS (parent_class)->query (src, query);
								break;
	}

	return res;
	/*	return TRUE;*/
}

/* notify subclasses of an event */
	static gboolean
gst_gstImageStreamsrc_event (GstBaseSrc * src, GstEvent * event)
{
	GstImageStreamsrc *gstImageStreamsrc = GST_GSTIMAGESTREAMSRC (src);

	GST_DEBUG_OBJECT (gstImageStreamsrc, "event");

	return TRUE;
}

/* ask the subclass to create a buffer with offset and size, the default
 * implementation will call alloc and fill. */
/*  If this method is not implemented, alloc followed by fill will be called. */
	static GstFlowReturn
gst_gstImageStreamsrc_create (GstPushSrc * src,	GstBuffer ** buf)
{
	gboolean ret;
	GstMapInfo info;
/*	GstClockTime next_time;*/
	gint bframe;
	uint16_t *map;
	GstImageStreamsrc *gstImageStreamsrc = GST_GSTIMAGESTREAMSRC (src);

	GST_DEBUG_OBJECT (gstImageStreamsrc, "create");

	ret = gst_buffer_pool_acquire_buffer (gstImageStreamsrc->pool, buf, NULL);
	if (G_UNLIKELY (ret != GST_FLOW_OK))
		return GST_FLOW_ERROR;

	bframe = gstImageStreamsrc->accum_frames ;

	ret = gst_buffer_map (*buf,&info, GST_MAP_WRITE);
	map=(uint16_t *)info.data;
/*	gst_base_src_set_do_timestamp (GST_BASE_SRC_CAST (gstImageStreamsrc),TRUE);*/
/*	GST_BUFFER_PTS (buf) = gstImageStreamsrc->accum_rtime + gstImageStreamsrc->timestamp_offset + gstImageStreamsrc->running_time;*/
/*	GST_BUFFER_DTS (buf) = GST_CLOCK_TIME_NONE;*/
	ret = gst_object_sync_values (GST_OBJECT (src), GST_BUFFER_PTS (buf));
	if (!ret)
		GST_DEBUG_OBJECT (src, "problem");

	ImageStreamIO_semwait(gstImageStreamsrc->image, 0);
	switch (gstImageStreamsrc->shmtype){
		case _DATATYPE_UINT16:
			memcpy(info.data,gstImageStreamsrc->image->array.UI16,gstImageStreamsrc->shmsize);
/*			GST_INFO_OBJECT (gstImageStreamsrc, "DATATYPE_UINT16");*/
			break;
		case _DATATYPE_FLOAT:
			for (int i=0;i<512*512;i++){
/*				for (int j=0;j<512;j++){*/
					map[i]=(uint16_t)floor(gstImageStreamsrc->image->array.F[i]);
/*				}*/
			}
				
			GST_INFO_OBJECT (gstImageStreamsrc, "info.size %lu",info.size);
			GST_INFO_OBJECT (gstImageStreamsrc, "info.maxsize %lu",info.maxsize);
			GST_INFO_OBJECT (gstImageStreamsrc, "DATATYPE_FLOAT");
			GST_INFO_OBJECT (gstImageStreamsrc, "->memsize %lu",gstImageStreamsrc->image->memsize);
			break;
	}

	gst_buffer_unmap(*buf,&info);

	GST_DEBUG_OBJECT (src, "Timestamp: %" GST_TIME_FORMAT  " + offset: %"
     	GST_TIME_FORMAT " + running time: %" GST_TIME_FORMAT,
     	GST_TIME_ARGS (GST_BUFFER_PTS (buf)),
		GST_TIME_ARGS (gstImageStreamsrc->timestamp_offset), GST_TIME_ARGS (gstImageStreamsrc->running_time));

	GST_BUFFER_TIMESTAMP (buf) = gstImageStreamsrc->timestamp_offset + gstImageStreamsrc->running_time;	

/*	GST_BUFFER_OFFSET (buf) = gstImageStreamsrc->accum_frames + gstImageStreamsrc->n_frames;*/
	gstImageStreamsrc->n_frames++;
/*	GST_BUFFER_OFFSET_END (buf) = GST_BUFFER_OFFSET (buf) + 1;*/

	gstImageStreamsrc->accum_frames=gstImageStreamsrc->image->md->cnt0;

	if ( gstImageStreamsrc->accum_frames  > bframe + 1  )
		GST_INFO_OBJECT (gstImageStreamsrc,"frame dropped");
		
	GST_INFO_OBJECT (gstImageStreamsrc,"frame number %ld", gstImageStreamsrc->accum_frames);
	GST_INFO_OBJECT (gstImageStreamsrc,"image count %lu", gstImageStreamsrc->image->md->cnt0);
/*
    if (gstImageStreamsrc->fps_n) {
    	next_time = gst_util_uint64_scale_int (gstImageStreamsrc->n_frames * GST_SECOND,
        gstImageStreamsrc->fps_d, gstImageStreamsrc->fps_n);
        GST_BUFFER_DURATION (buf) = next_time - gstImageStreamsrc->running_time;

     } else {
  		next_time = gstImageStreamsrc->timestamp_offset;
  		GST_BUFFER_DURATION (buf) = GST_CLOCK_TIME_NONE;
	}
*/

	/* running time avec framerate on livesource*/
/*	gstImageStreamsrc->running_time = next_time;*/


	gstImageStreamsrc->running_time = gst_clock_get_time (GST_ELEMENT_CLOCK (src)) - GST_ELEMENT_CAST (src)->base_time;
	
	GST_DEBUG_OBJECT (gstImageStreamsrc,"running time %"GST_TIME_FORMAT,GST_TIME_ARGS ( gstImageStreamsrc->running_time));
	GST_DEBUG_OBJECT (gstImageStreamsrc,"PTS %"GST_TIME_FORMAT,GST_TIME_ARGS (gstImageStreamsrc->timestamp_offset + gstImageStreamsrc->running_time));
/*	GST_BUFFER_PTS (buf) = GST_CLOCK_TIME_NONE;
	GST_BUFFER_DTS (buf) = GST_CLOCK_TIME_NONE;*/
/*	gst_object_sync_values (GST_OBJECT (src), GST_BUFFER_PTS (buf));*/

	return GST_FLOW_OK;
}

	static gboolean
plugin_init (GstPlugin * plugin)
{

	/* FIXME Remember to set the rank if it's an element that is meant
	   to be autoplugged by decodebin. */
	return gst_element_register (plugin, "gstImageStreamsrc", GST_RANK_NONE,
			GST_TYPE_GSTIMAGESTREAMSRC);
}

/* FIXME: these are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
#ifndef VERSION
#define VERSION "0.0.FIXME"
#endif
#ifndef PACKAGE
#define PACKAGE "FIXME_package"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "FIXME_package_name"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "http://FIXME.org/"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
		GST_VERSION_MINOR,
		gstImageStreamsrc,
		"FIXME plugin description",
		plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)
