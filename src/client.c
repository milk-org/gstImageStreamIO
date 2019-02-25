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

int main(int argc, char *argv[]) {
  GMainLoop *loop;

  GstElement *pipeline, *source, *depay, *dec, *conv, *sink;
  GstBus *bus;
  guint bus_watch_id;

  /* Initialisation */
  gst_init(&argc, &argv);

  loop = g_main_loop_new(NULL, FALSE);

  /* Check input arguments */
  if (argc != 3) {
    g_printerr("Usage: %s host port\n", argv[0]);
    return -1;
  }

  /* Create gstreamer elements */
  pipeline = gst_pipeline_new("imageStreamIO-client");
  source = gst_element_factory_make("udpsrc", "UDP-source");
  depay = gst_element_factory_make("rtpvp8depay", "depayloader");
  dec = gst_element_factory_make("decodebin", "decoder");
  conv = gst_element_factory_make("videoconvert", "converter");
  sink = gst_element_factory_make("autovideosink", "graphical-output");

  if (!pipeline || !source || !depay || !dec || !conv || !sink) {
    g_printerr("One element could not be created. Exiting.\n");
    return -1;
  }

  /* Set up the pipeline */

  /* we set the input filename to the source element */
  GstCaps *caps = gst_caps_new_simple("application/x-rtp", NULL);
  g_object_set(G_OBJECT(source), "caps", caps, NULL);
  g_object_set(G_OBJECT(source), "address", argv[1], "port", atoi(argv[2]), NULL);

  /* we add a message handler */
  bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
  bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
  gst_object_unref(bus);

  /* we add all elements into the pipeline */
  gst_bin_add_many(GST_BIN(pipeline), source, depay, dec, conv, sink, NULL);

  /* we link the elements together */
  gst_element_link_many(source, depay, dec, conv, sink, NULL);

  /* Set the pipeline to "playing" state*/
  g_print("Now playing: from %s:%s\n", argv[1], argv[2]);
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
