#include <glib.h>
#include <gst/gst.h>

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
  GMainLoop *loop = (GMainLoop *)data;

  switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_EOS:
      g_print("End of stream\n");
      g_main_loop_quit(loop);
      break;

    case GST_MESSAGE_ERROR: {
      gchar *debug;
      GError *error;

      gst_message_parse_error(msg, &error, &debug);
      g_free(debug);

      g_printerr("Error: %s\n", error->message);
      g_error_free(error);

      g_main_loop_quit(loop);
      break;
    }
    default:
      break;
  }

  return TRUE;
}

static void on_pad_added(GstElement *element, GstPad *pad, gpointer data) {
  GstPad *sinkpad;
  GstElement *decoder = (GstElement *)data;

  /* We can now link this pad with the vorbis-decoder sink pad */
  g_print("Dynamic pad created, linking demuxer/decoder\n");

  sinkpad = gst_element_get_static_pad(decoder, "sink");

  gst_pad_link(pad, sinkpad);

  gst_object_unref(sinkpad);
}

int main(int argc, char *argv[]) {
  GMainLoop *loop;

  GstElement *pipeline, *source, *conv, *enc, *pay, *sink;
  GstBus *bus;
  guint bus_watch_id;

  /* Initialisation */
  gst_init(&argc, &argv);

  loop = g_main_loop_new(NULL, FALSE);

  /* Check input arguments */
  if (argc != 4) {
    g_printerr("Usage: %s img_name host port\n", argv[0]);
    return -1;
  }

  /* Create gstreamer elements */
  pipeline = gst_pipeline_new("imageStreamIO-streamer");
  source = gst_element_factory_make("imageStreamIOsrc", "SHM-source");
  conv = gst_element_factory_make("videoconvert", "converter");
  enc = gst_element_factory_make("vp8enc", "encoder");
  pay = gst_element_factory_make("rtpvp8pay", "payloader");
  sink = gst_element_factory_make("udpsink", "udp-output");

  if (!pipeline || !source || !conv || !enc || !pay || !sink) {
    g_printerr("One element could not be created. Exiting.\n");
    return -1;
  }

  /* Set up the pipeline */

  /* we set the input filename to the source element */
  g_object_set(G_OBJECT(source), "shm-name", argv[1], NULL);
  g_object_set(G_OBJECT(sink), "host", argv[2], "port", atoi(argv[3]), NULL);

  /* we add a message handler */
  bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
  bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
  gst_object_unref(bus);

  /* we add all elements into the pipeline */
  gst_bin_add_many(GST_BIN(pipeline), source, conv, enc, pay, sink,
                   NULL);

  /* we link the elements together */
  gst_element_link_many(source, conv, enc, pay, sink, NULL);

  /* Set the pipeline to "playing" state*/
  g_print("Now playing: %s on %s:%s\n", argv[1], argv[2], argv[3]);
  gst_element_set_state(pipeline, GST_STATE_PLAYING);

  /* Iterate */
  g_print("Running...\n");
  g_main_loop_run(loop);

  /* Out of the main loop, clean up nicely */
  g_print("Returned, stopping playback\n");
  gst_element_set_state(pipeline, GST_STATE_NULL);

  g_print("Deleting pipeline\n");
  gst_object_unref(GST_OBJECT(pipeline));
  g_source_remove(bus_watch_id);
  g_main_loop_unref(loop);

  return 0;
}
