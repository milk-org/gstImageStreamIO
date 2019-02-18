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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _GST_GSTIMAGESTREAMSRC_H_
#define _GST_GSTIMAGESTREAMSRC_H_

#include "/home/brule/cacao/ImageStreamIO/ImageStruct.h"

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>

#include <gst/video/video.h>
#include <gst/video/gstvideopool.h>

G_BEGIN_DECLS

#define GST_TYPE_GSTIMAGESTREAMSRC   (gst_gstImageStreamsrc_get_type())
#define GST_GSTIMAGESTREAMSRC(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_GSTIMAGESTREAMSRC,GstImageStreamsrc))
#define GST_GSTIMAGESTREAMSRC_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_GSTIMAGESTREAMSRC,GstImageStreamsrcClass))
#define GST_IS_RAWSHMSRC(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_GSTIMAGESTREAMSRC))
#define GST_IS_RAWSHMSRC_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_GSTIMAGESTREAMSRC))

typedef struct _GstImageStreamsrc GstImageStreamsrc;
typedef struct _GstImageStreamsrcClass GstImageStreamsrcClass;

struct _GstImageStreamsrc
{
	GstPushSrc element;

	char *shmpathname;
	goffset shmoffset;
	gchar  *shmaddr;
	gint shmtype;
	gint shmsize;
	IMAGE *image;

	GstCaps *fixed_caps;

	
	GstBufferPool *pool;

	GstVideoInfo vinfo;
	guint8 *frame;	

	/* private */
	/* FIXME 2.0: Change type to GstClockTime */
	gint64 timestamp_offset;              /* base offset */

	/* running time and frames for current caps */
	GstClockTime running_time;            /* total running time */
	gint64 n_frames;                      /* total frames sent */
	gint64 p_frames;                      /* previous total frames sent */
	gboolean reverse;

	/* previous caps running time and frames */
	GstClockTime accum_rtime;              /* accumulated running_time */
	gint64 accum_frames;                  /* accumulated frames */


	gint height;
	gint width;
/*
if (framerate) {
     *fps_n = gst_value_get_fraction_numerator (framerate);
     *fps_d = gst_value_get_fraction_denominator (framerate);
*/
	gint fps_n; /* numerator*/
	gint fps_d; /* denominator*/
	/*duration*/
	/*gint64 dur = gst_util_uint64_scale_int_round (bsrc->num_buffers* GST_SECOND, fps_d, fps_n);*/
	/*	void (*make_image) (GstRawshm *v, GstVideoFrame *frame);*/
};

struct _GstImageStreamsrcClass
{
	GstPushSrcClass parent_class;
};

GType gst_gstImageStreamsrc_get_type (void);

G_END_DECLS

#endif
