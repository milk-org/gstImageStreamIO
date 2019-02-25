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

#ifndef _GST_GSTIMAGESTREAMIOSRC_H_
#define _GST_GSTIMAGESTREAMIOSRC_H_

#include "ImageStruct.h"

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>

#include <gst/video/video.h>
#include <gst/video/gstvideopool.h>

G_BEGIN_DECLS

#define GST_TYPE_GSTIMAGESTREAMIOSRC   (gst_imageStreamIOsrc_get_type())
#define GST_GSTIMAGESTREAMIOSRC(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_GSTIMAGESTREAMIOSRC,GstImageStreamIOsrc))
#define GST_GSTIMAGESTREAMIOSRC_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_GSTIMAGESTREAMIOSRC,GstImageStreamIOsrcClass))
#define GST_IS_RAWSHMSRC(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_GSTIMAGESTREAMIOSRC))
#define GST_IS_RAWSHMSRC_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_GSTIMAGESTREAMIOSRC))

typedef struct _GstImageStreamIOsrc GstImageStreamIOsrc;
typedef struct _GstImageStreamIOsrcClass GstImageStreamIOsrcClass;

struct _GstImageStreamIOsrc
{
	GstPushSrc element;

	char shmpathname[64];
	gchar  *shmaddr;
	gint shmtype;
	gint shmsize;
	IMAGE *image;
	gint sem_num;

	GstBufferPool *pool;

	GstVideoInfo vinfo;
	/* private */

	/* running time and frames for current caps */
	GstClockTime running_time;            /* total running time */

	/* previous caps running time and frames */
	GstClockTime accum_rtime;              /* accumulated running_time */


	gint height;
	gint width;
	gint framerate;
};

struct _GstImageStreamIOsrcClass
{
	GstPushSrcClass parent_class;
};

GType gst_imageStreamIOsrc_get_type (void);

gboolean register_imageStreamIOsrc(GstPlugin *plugin);

G_END_DECLS

#endif
