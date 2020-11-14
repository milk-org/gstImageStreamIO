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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <fcntl.h>
#include <glib/gstdio.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

#include "ImageStreamIO.h"
#include "ImageStruct.h"
#include "gstImageStreamIOsrc.h"
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

GST_DEBUG_CATEGORY_STATIC(gst_imageStreamIOsrc_debug_category);
#define GST_CAT_DEFAULT gst_imageStreamIOsrc_debug_category

/* prototypes */
static void gst_imageStreamIOsrc_set_property(GObject *object,
                                              guint property_id,
                                              const GValue *value,
                                              GParamSpec *pspec);
static void gst_imageStreamIOsrc_get_property(GObject *object,
                                              guint property_id, GValue *value,
                                              GParamSpec *pspec);
static void gst_imageStreamIOsrc_finalize(GObject *object);

static gboolean gst_imageStreamIOsrc_negotiate(GstBaseSrc *src);
static GstCaps *gst_imageStreamIOsrc_fixate(GstBaseSrc *src, GstCaps *caps);
static gboolean gst_imageStreamIOsrc_decide_allocation(GstBaseSrc *src,
                                                       GstQuery *query);
static gboolean gst_imageStreamIOsrc_start(GstBaseSrc *src);
static gboolean gst_imageStreamIOsrc_stop(GstBaseSrc *src);
static void gst_imageStreamIOsrc_get_times(GstBaseSrc *src, GstBuffer *buffer,
                                           GstClockTime *start,
                                           GstClockTime *end);
static GstFlowReturn gst_imageStreamIOsrc_create(GstPushSrc *src,
                                                 GstBuffer **buf);

enum { PROP_0, PROP_SHM_NAME, PROP_SHM_MIN, PROP_SHM_MAX, N_PROPERTIES };
static GParamSpec *obj_properties[N_PROPERTIES] = {
    NULL,
};

/* pad templates */
static GstStaticPadTemplate gst_imageStreamIOsrc_template =
    GST_STATIC_PAD_TEMPLATE(
        "src", GST_PAD_SRC, GST_PAD_ALWAYS,
        GST_STATIC_CAPS(
            "video/x-raw,format=(string) GRAY16_LE,framerate = 25/1;"));

#define gst_imageStreamIOsrc_parent_class parent_class

/* class initialization */
G_DEFINE_TYPE_WITH_CODE(
    GstImageStreamIOsrc, gst_imageStreamIOsrc, GST_TYPE_PUSH_SRC,
    GST_DEBUG_CATEGORY_INIT(gst_imageStreamIOsrc_debug_category,
                            "imageStreamIOsrc", 0,
                            "debug category for imageStreamIOsrc element"));

static void gst_imageStreamIOsrc_class_init(GstImageStreamIOsrcClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS(klass);
  GstBaseSrcClass *base_src_class = GST_BASE_SRC_CLASS(klass);
  GstPushSrcClass *push_src_class = GST_PUSH_SRC_CLASS(klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_static_pad_template(gstelement_class,
                                            &gst_imageStreamIOsrc_template);

  gst_element_class_set_static_metadata(gstelement_class,
                                        "imageStreamIO source", "Generic",
                                        "imageStreamIO source",
                                        "Julien Brulé <Julien.Brule@obspm.fr>, "
                                        "Arnaud Sevin <Arnaud.Sevin@obspm.fr>");

  obj_properties[PROP_SHM_NAME] =
      g_param_spec_string("shm-name", "Name of the shared memory",
                          "define the location of the shmfile", "imtest00",
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  obj_properties[PROP_SHM_MIN] = g_param_spec_int(
      "min", "Minimum value of the frame", "Get the minimum value of the frame",
      G_MININT, G_MAXINT, G_MININT, G_PARAM_READWRITE);

  obj_properties[PROP_SHM_MAX] = g_param_spec_int(
      "max", "Maximum value of the frame", "Get the maximum value of the frame",
      G_MININT, G_MAXINT, G_MAXINT, G_PARAM_READWRITE);

  gobject_class->set_property = gst_imageStreamIOsrc_set_property;
  gobject_class->get_property = gst_imageStreamIOsrc_get_property;
  g_object_class_install_properties(gobject_class, N_PROPERTIES,
                                    obj_properties);
  gobject_class->finalize = gst_imageStreamIOsrc_finalize;

  base_src_class->negotiate = GST_DEBUG_FUNCPTR(gst_imageStreamIOsrc_negotiate);
  base_src_class->fixate = GST_DEBUG_FUNCPTR(gst_imageStreamIOsrc_fixate);
  base_src_class->decide_allocation =
      GST_DEBUG_FUNCPTR(gst_imageStreamIOsrc_decide_allocation);
  base_src_class->start = GST_DEBUG_FUNCPTR(gst_imageStreamIOsrc_start);
  base_src_class->stop = GST_DEBUG_FUNCPTR(gst_imageStreamIOsrc_stop);
  base_src_class->get_times = GST_DEBUG_FUNCPTR(gst_imageStreamIOsrc_get_times);
  push_src_class->create = GST_DEBUG_FUNCPTR(gst_imageStreamIOsrc_create);
}

static void gst_imageStreamIOsrc_init(GstImageStreamIOsrc *imageStreamIOsrc) {
  GstBaseSrc *src = GST_BASE_SRC(imageStreamIOsrc);
  imageStreamIOsrc->image = malloc(sizeof(IMAGE));

  gst_pad_use_fixed_caps(GST_BASE_SRC_PAD(imageStreamIOsrc));
  gst_base_src_set_live(src, TRUE);
  gst_base_src_set_do_timestamp(src, TRUE);
  gst_base_src_set_format(src, GST_FORMAT_TIME);
}

void gst_imageStreamIOsrc_set_property(GObject *object, guint property_id,
                                       const GValue *value, GParamSpec *pspec) {
  GstImageStreamIOsrc *imageStreamIOsrc = GST_GSTIMAGESTREAMIOSRC(object);

  GST_DEBUG_OBJECT(imageStreamIOsrc, "set_property");

  switch (property_id) {
  case PROP_SHM_NAME:
    // g_free(imageStreamIOsrc->shmpathname);
    /* check if we are currently active
       prevent setting camera and other values to something not representing
       the active camera */
    /*if stream*/
    strcpy(imageStreamIOsrc->shmpathname, g_strdup(g_value_get_string(value)));
    fprintf(stderr, "Setting shm-name to %s\n", imageStreamIOsrc->shmpathname);
    GST_LOG_OBJECT(imageStreamIOsrc, "Set shm path name to %s",
                   imageStreamIOsrc->shmpathname);

    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

void gst_imageStreamIOsrc_get_property(GObject *object, guint property_id,
                                       GValue *value, GParamSpec *pspec) {
  GstImageStreamIOsrc *imageStreamIOsrc = GST_GSTIMAGESTREAMIOSRC(object);

  GST_DEBUG_OBJECT(imageStreamIOsrc, "get_property");

  switch (property_id) {
  case PROP_SHM_NAME:
    g_value_set_string(value, imageStreamIOsrc->shmpathname);
    GST_LOG_OBJECT(imageStreamIOsrc, "Set shm path name to %s",
                   imageStreamIOsrc->shmpathname);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

void gst_imageStreamIOsrc_finalize(GObject *object) {
  GstImageStreamIOsrc *imageStreamIOsrc = GST_GSTIMAGESTREAMIOSRC(object);

  GST_DEBUG_OBJECT(imageStreamIOsrc, "finalize");

  /* clean up object here */
  g_free(imageStreamIOsrc->image);

  G_OBJECT_CLASS(gst_imageStreamIOsrc_parent_class)->finalize(object);
}

/* decide on caps */
static gboolean gst_imageStreamIOsrc_negotiate(GstBaseSrc *src) {
  GstCaps *caps;
  GstVideoInfo vinfo;
  GstVideoFormat vformat;
  gchar *stream_id = NULL;
  GstEvent *stream_start = NULL;
  GstStructure *structure;

  GstImageStreamIOsrc *imageStreamIOsrc = GST_GSTIMAGESTREAMIOSRC(src);

  GST_DEBUG_OBJECT(imageStreamIOsrc, "negotiate");

  // stream_id = gst_pad_create_stream_id(GST_BASE_SRC_PAD(src),
  // GST_ELEMENT(src),
  //                                      "RAWSHMSRC");
  // stream_start = gst_event_new_stream_start(stream_id);
  // g_free(stream_id);
  // gst_pad_push_event(GST_BASE_SRC_PAD(src), stream_start);

  vformat = gst_video_format_from_string("GRAY16_LE");

  gst_video_info_init(&vinfo);

  gst_video_info_set_format(&vinfo, vformat, imageStreamIOsrc->height,
                            imageStreamIOsrc->width);

  caps = gst_video_info_to_caps(&vinfo);

  structure = gst_caps_get_structure(caps, 0);
  gst_structure_fixate_field_nearest_fraction(structure, "framerate",
                                              imageStreamIOsrc->framerate, 1);

  gst_base_src_set_caps(src, caps);

  gst_caps_unref(caps);

  return TRUE;
}

/* called if, in negotiation, caps need fixating */
static GstCaps *gst_imageStreamIOsrc_fixate(GstBaseSrc *src, GstCaps *caps) {
  GstImageStreamIOsrc *imageStreamIOsrc = GST_GSTIMAGESTREAMIOSRC(src);

  GST_DEBUG_OBJECT(imageStreamIOsrc, "fixate");
  GstStructure *structure;
  caps = gst_caps_make_writable(caps);
  structure = gst_caps_get_structure(caps, 0);
  gst_structure_fixate_field_nearest_int(structure, "width",
                                         imageStreamIOsrc->width);
  gst_structure_fixate_field_nearest_int(structure, "height",
                                         imageStreamIOsrc->height);
  gst_structure_fixate_field_nearest_fraction(structure, "framerate",
                                              imageStreamIOsrc->framerate, 1);
  caps = GST_BASE_SRC_CLASS(src)->fixate(src, caps);
  GST_LOG("caps are %" GST_PTR_FORMAT, caps);

  GST_BASE_SRC_CLASS(gst_imageStreamIOsrc_parent_class)->fixate(src, caps);
  return caps;
}

/* setup allocation query */
static gboolean gst_imageStreamIOsrc_decide_allocation(GstBaseSrc *src,
                                                       GstQuery *query) {
  GstImageStreamIOsrc *imageStreamIOsrc = GST_GSTIMAGESTREAMIOSRC(src);
  GstBufferPool *pool = NULL;
  guint size, min = 1, max = 0;
  GstStructure *config;
  GstCaps *caps;
  GstVideoInfo info;
  gboolean ret;

  GST_DEBUG_OBJECT(imageStreamIOsrc, "decide_allocation");

  /* Parse an allocation query,
     writing the requested caps in caps and whether a pool is needed in
     need_pool , if the respective parameters are non-NULL.*/
  gst_query_parse_allocation(query, &caps, NULL);

  if (!caps || !gst_video_info_from_caps(&info, caps))
    return FALSE;

  /* Retrieve the number of values currently stored in the pool array of the
   * query's structure.*/
  while (gst_query_get_n_allocation_pools(query) > 0) {
    /*Get the pool parameters in query*/
    gst_query_parse_nth_allocation_pool(query, 0, &pool, &size, &min, &max);

    /* TODO We restrict to the exact size as we don't support strides or
     * special padding */
    if (size == info.size)
      break;

    /*Remove the allocation pool at index of the allocation pool array*/
    gst_query_remove_nth_allocation_pool(query, 0);
    gst_object_unref(pool);
    pool = NULL;
  }

  if (pool == NULL) {
    /* we did not get a pool, make one ourselves then */
    /* Create a new bufferpool that can allocate video frames */
    pool = gst_video_buffer_pool_new();
    size = info.size;
    min = 1;
    max = 0;
    /* Set the pool parameters in query */
    gst_query_add_allocation_pool(query, pool, size, min, max);
  }

  imageStreamIOsrc->pool = pool;
  /* Get a copy of the current configuration of the pool */
  config = gst_buffer_pool_get_config(pool);

  /*Configure config with the given parameters*/
  gst_buffer_pool_config_set_params(config, caps, size, min, max);

  // GST_INFO_OBJECT(imageStreamIOsrc, "decide_allocation size %d", size);

  /*Set the configuration of the pool.
    If the pool is already configured, and the configuration haven't change,
    this function will return TRUE. If the pool is active, this method will
    return FALSE and active configuration will remain. Buffers allocated form
    this pool must be returned or else this function will do nothing and return
    FALSE.*/
  ret = gst_buffer_pool_set_config(pool, config);

  // GST_INFO_OBJECT(imageStreamIOsrc, "decide_allocation change? %d", ret);

  /*ajout??*/
  gst_buffer_pool_set_active(pool, TRUE);

  gst_object_unref(pool);

  imageStreamIOsrc->accum_rtime += imageStreamIOsrc->running_time;

  imageStreamIOsrc->running_time = 0;

  return ret;
  /*	return TRUE;*/
}

/* start and stop processing, ideal for opening/closing the resource */
static gboolean gst_imageStreamIOsrc_start(GstBaseSrc *src) {
  GstImageStreamIOsrc *imageStreamIOsrc = GST_GSTIMAGESTREAMIOSRC(src);

  GST_DEBUG_OBJECT(imageStreamIOsrc, "start");

    /* open mmap en imageSteamIO */
    ImageStreamIO_read_sharedmem_image_toIMAGE(imageStreamIOsrc->shmpathname,
                                               imageStreamIOsrc->image);

    imageStreamIOsrc->sem_num =
        ImageStreamIO_getsemwaitindex(imageStreamIOsrc->image, 0);

    int imid = 0;
    int imcnt0 = imageStreamIOsrc->image->md->cnt0;

    int imXsize = imageStreamIOsrc->image->md->size[0];
    int imYsize = imageStreamIOsrc->image->md->size[1];
    if (imageStreamIOsrc->image->md->naxis == 2) {
        imid = 0;
    } else if (imageStreamIOsrc->image->md->naxis == 3) {
      imid = imageStreamIOsrc->image->md->cnt1;
    } else {
      GST_DEBUG_OBJECT(imageStreamIOsrc, "wrong dimensions");
      return FALSE;
    }
    int imSize = imXsize * imYsize;

    imageStreamIOsrc->height = imXsize;
    imageStreamIOsrc->width = imYsize;
    imageStreamIOsrc->framerate = 25;
    imageStreamIOsrc->shmtype = imageStreamIOsrc->image->md->datatype;
    imageStreamIOsrc->shmsize =
        imSize * ImageStreamIO_typesize(imageStreamIOsrc->shmtype);

  imageStreamIOsrc->height = imXsize;
  imageStreamIOsrc->width = imYsize;
  imageStreamIOsrc->framerate = 25;
  imageStreamIOsrc->shmtype = imageStreamIOsrc->image->md[0].datatype;
  imageStreamIOsrc->shmsize =
      imSize * ImageStreamIO_typesize(imageStreamIOsrc->shmtype);

  imageStreamIOsrc->running_time = 0;
  imageStreamIOsrc->accum_rtime = 0;

  gst_base_src_set_live(src, TRUE);
  /*	gst_video_info_init (&imageStreamIOsrc->vinfo);*/

  return TRUE;
}

static gboolean gst_imageStreamIOsrc_stop(GstBaseSrc *src) {
  GstImageStreamIOsrc *imageStreamIOsrc = GST_GSTIMAGESTREAMIOSRC(src);

  GST_DEBUG_OBJECT(imageStreamIOsrc, "stop");

  return TRUE;
}

/* given a buffer, return start and stop time when it should be pushed
 * out. The base class will sync on the clock using these times. */
static void gst_imageStreamIOsrc_get_times(GstBaseSrc *src, GstBuffer *buffer,
                                           GstClockTime *start,
                                           GstClockTime *end) {
  GstImageStreamIOsrc *imageStreamIOsrc = GST_GSTIMAGESTREAMIOSRC(src);

  GST_DEBUG_OBJECT(imageStreamIOsrc, "get_times");

  if (buffer == NULL)
    GST_DEBUG_OBJECT(imageStreamIOsrc, "buffer is null");

  /* for live sources, sync on the timestamp of the buffer */
  if (gst_base_src_is_live(src)) {
    GstClockTime timestamp, duration;

    /* first sync on DTS, else use PTS */
    timestamp = GST_BUFFER_DTS(buffer);
    if (!GST_CLOCK_TIME_IS_VALID(timestamp))
      timestamp = GST_BUFFER_PTS(buffer);

    if (GST_CLOCK_TIME_IS_VALID(timestamp)) {
      /* get duration to calculate end time */
      duration = GST_BUFFER_DURATION(buffer);
      if (GST_CLOCK_TIME_IS_VALID(duration)) {
        *end = timestamp + duration;
      }
      *start = timestamp;
    }
  } else {
    *start = -1;
    *end = -1;
  }
}

/* ask the subclass to create a buffer with offset and size, the default
 * implementation will call alloc and fill. */
/*  If this method is not implemented, alloc followed by fill will be called. */
static GstFlowReturn gst_imageStreamIOsrc_create(GstPushSrc *src,
                                                 GstBuffer **buf) {
  gboolean ret;
  GstMapInfo info;
  GstClockTime next_time;
  void *lastBuffer;
  uint16_t *map;
  GstImageStreamIOsrc *imageStreamIOsrc = GST_GSTIMAGESTREAMIOSRC(src);
  float *f_map;

  GST_DEBUG_OBJECT(imageStreamIOsrc, "create");

  ret = gst_buffer_pool_acquire_buffer(imageStreamIOsrc->pool, buf, NULL);
  if (G_UNLIKELY(ret != GST_FLOW_OK))
    return GST_FLOW_ERROR;

  ret = gst_buffer_map(*buf, &info, GST_MAP_WRITE);
  map = (uint16_t *)info.data;
  gst_base_src_set_do_timestamp(GST_BASE_SRC_CAST(imageStreamIOsrc), TRUE);
  GST_BUFFER_PTS(buf) =
      imageStreamIOsrc->accum_rtime + imageStreamIOsrc->running_time;
  GST_BUFFER_DTS(buf) = GST_CLOCK_TIME_NONE;
  ret = gst_object_sync_values(GST_OBJECT(src), GST_BUFFER_PTS(buf));
  if (!ret)
    GST_DEBUG_OBJECT(src, "problem");

  ImageStreamIO_semwait(imageStreamIOsrc->image, imageStreamIOsrc->sem_num);
  const int64_t nb_index =  (imageStreamIOsrc->image->md->naxis == 3 ? imageStreamIOsrc->image->md->size[2] : 1);
  const int64_t read_index = (imageStreamIOsrc->image->md->cnt1 + 1) % nb_index;
  ImageStreamIO_readBufferAt(imageStreamIOsrc->image, read_index, &lastBuffer);

  double max, min, value;
  switch (imageStreamIOsrc->shmtype) {
  case _DATATYPE_UINT16:
    GST_INFO_OBJECT (imageStreamIOsrc, "DATATYPE_UINT16");
    if (imageStreamIOsrc->image->md->location == -1) {
      GST_INFO_OBJECT (imageStreamIOsrc, "doing memcpy directly");
      memcpy(map, lastBuffer, imageStreamIOsrc->shmsize);
    } else {
#ifdef HAVE_CUDA
      GST_INFO_OBJECT (imageStreamIOsrc, "doing cudaMemcpy from GPU%d directly", imageStreamIOsrc->image->md->location);
      cudaSetDevice(imageStreamIOsrc->image->md->location);
      if(cudaMemcpy(map, lastBuffer, imageStreamIOsrc->shmsize, cudaMemcpyDeviceToHost) != cudaSuccess){
        fprintf(stderr, "cudaMemcpy failled");
        return GST_FLOW_ERROR;
      }
      cudaDeviceSynchronize();
#else
      fprintf(stderr, "not compiled with USE_CUDA flag");
      return GST_FLOW_ERROR;
#endif
    }
    max=map[0];
    min=map[0];
    for (int i = 1; i < imageStreamIOsrc->height * imageStreamIOsrc->width;
          i++) {
      value = (map[i]);
      max = (value>max?value:max);
      min = (value<min?value:min);
    }
    GST_INFO_OBJECT (imageStreamIOsrc, "min %d ;  max %d", min, max);

    for (int i = 0; i < imageStreamIOsrc->height * imageStreamIOsrc->width;
          i++) {
      map[i] = (uint16_t)floor(((map[i])-min)/(max-min)*65535);
    }


    break;
  case _DATATYPE_FLOAT:
    GST_INFO_OBJECT(imageStreamIOsrc, "info.size %lu", info.size);
    GST_INFO_OBJECT(imageStreamIOsrc, "info.maxsize %lu", info.maxsize);
    GST_INFO_OBJECT(imageStreamIOsrc, "DATATYPE_FLOAT");
    GST_INFO_OBJECT(imageStreamIOsrc, "->memsize %lu",
                    imageStreamIOsrc->image->memsize);
    if (imageStreamIOsrc->image->md->location == -1) {
      GST_INFO_OBJECT (imageStreamIOsrc, "using imageStreamIO SHM pointer");
      f_map = (float*)lastBuffer;
    } else {
#ifdef HAVE_CUDA
      f_map = (float*)malloc(imageStreamIOsrc->shmsize);
      GST_INFO_OBJECT (imageStreamIOsrc, "using temporary buffer for doing cudaMemcpy from GPU%d", imageStreamIOsrc->image->md->location);
      cudaSetDevice(imageStreamIOsrc->image->md->location);
      if(cudaMemcpy(f_map, lastBuffer, imageStreamIOsrc->shmsize, cudaMemcpyDeviceToHost) != cudaSuccess){
        fprintf(stderr, "cudaMemcpy failled");
        return GST_FLOW_ERROR;
      }
      cudaDeviceSynchronize();
#else
      fprintf(stderr, "not compiled with USE_CUDA flag");
      return GST_FLOW_ERROR;
#endif
    }

    max=f_map[0];
    min=f_map[0];
    for (int i = 1; i < imageStreamIOsrc->height * imageStreamIOsrc->width;
          i++) {
      value = (f_map[i]);
      max = (value>max?value:max);
      min = (value<min?value:min);
    }
    GST_INFO_OBJECT (imageStreamIOsrc, "min %d ;  max %d", min, max);

    for (int i = 0; i < imageStreamIOsrc->height * imageStreamIOsrc->width;
          i++) {
      map[i] = (uint16_t)floor(((f_map[i])-min)/(max-min)*65535);
    }

    if (imageStreamIOsrc->image->md->location > -1) {
#ifdef HAVE_CUDA
      free(f_map);
#else
      fprintf(stderr, "not compiled with USE_CUDA flag");
      return GST_FLOW_ERROR;
#endif
    }
    break;
    default:
    GST_INFO_OBJECT (imageStreamIOsrc, "DATATYPE not managed");
    break;
  }

  gst_buffer_unmap(*buf, &info);

  GST_BUFFER_TIMESTAMP(buf) = imageStreamIOsrc->running_time;

  GST_INFO_OBJECT(imageStreamIOsrc, "image count %lu",
                  imageStreamIOsrc->image->md->cnt0);

  if (imageStreamIOsrc->framerate) {
    next_time =
        gst_util_uint64_scale_int(GST_SECOND, imageStreamIOsrc->framerate, 1);
    GST_BUFFER_DURATION(buf) = next_time - imageStreamIOsrc->running_time;

  } else {
    next_time = 0;
    GST_BUFFER_DURATION(buf) = GST_CLOCK_TIME_NONE;
  }

  /* running time avec framerate on livesource*/
  imageStreamIOsrc->running_time = next_time;

  imageStreamIOsrc->running_time = gst_clock_get_time(GST_ELEMENT_CLOCK(src)) -
                                   GST_ELEMENT_CAST(src)->base_time;

  GST_INFO_OBJECT(imageStreamIOsrc, "running time %" GST_TIME_FORMAT,
                  GST_TIME_ARGS(imageStreamIOsrc->running_time));
  // GST_BUFFER_PTS(buf) = GST_CLOCK_TIME_NONE;
  // gst_object_sync_values(GST_OBJECT(src), GST_BUFFER_PTS(buf));

  return GST_FLOW_OK;
}

gboolean register_imageStreamIOsrc(GstPlugin *plugin) {
  /* FIXME Remember to set the rank if it's an element that is meant
     to be autoplugged by decodebin. */
  return gst_element_register(plugin, "imageStreamIOsrc", GST_RANK_NONE,
                              GST_TYPE_GSTIMAGESTREAMIOSRC);
}
