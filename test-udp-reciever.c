//TEST UDP RECIEVER, BASED ON HELLOWORLD EXAMPLE
//TO COMPILE: gcc -Wall test-udp-reciever.c -o test-udp-reciever $(pkg-config --cflags --libs gstreamer-1.0)
//TO HOST SERVER: gst-launch-1.0 videotestsrc ! x264enc ! rtph264pay config-interval=1 pt=96 ! udpsink host=127.0.0.1 port=5004
#include <gst/gst.h>
#include <glib.h>

static gboolean
bus_call (GstBus     *bus,
          GstMessage *msg,
          gpointer    data)
{
  GMainLoop *loop = (GMainLoop *) data;

  switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_EOS:
      g_print ("End of stream\n");
      g_main_loop_quit (loop);
      break;

    case GST_MESSAGE_ERROR: {
      gchar  *debug;
      GError *error;
      gst_message_parse_error (msg, &error, &debug);
      g_free (debug);
      g_printerr ("Error: %s\n", error->message);
      g_error_free (error);
      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }
  return TRUE;
}

int
main (int   argc,
      char *argv[])
{
  GMainLoop *loop;
  GstElement *pipeline, *source, *depay, *queue, *decoder, *convert, *sink;
  GstBus *bus;
  GstCaps    *caps;
  guint bus_watch_id;

  /* Initialisation */
  gst_init (&argc, &argv);
  loop = g_main_loop_new (NULL, FALSE);

  /* Create pipeline elements */
  pipeline = gst_pipeline_new ("test-udp-reciever");
  source = gst_element_factory_make ("udpsrc", "udp-source");
  depay = gst_element_factory_make ("rtph264depay", "rtph264depay");
  queue = gst_element_factory_make ("queue", "queue");
  decoder = gst_element_factory_make ("avdec_h264", "h264-decoder");
  convert = gst_element_factory_make ("videoconvert", "videoconvert");
  sink = gst_element_factory_make ("autovideosink", "autovideosink");

  if (!pipeline || !source || !depay || !queue || !decoder || !convert || !sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }
  /* we set the caps for the incoming stream */
  caps = gst_caps_new_simple("application/x-rtp", "media", G_TYPE_STRING, "video",
                             "clock-rate", G_TYPE_INT, 90000, "payload", G_TYPE_INT, 96,
                             "encoding-name", G_TYPE_STRING, "H264", NULL);

  /* we set the properties of the pipeline elements */
  g_object_set (G_OBJECT (source), "port", 5004, NULL);
  g_object_set (G_OBJECT (source), "caps", caps, NULL);
  gst_caps_unref(caps);

  /* we add a message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  /* we add all elements into the pipeline */
  gst_bin_add_many (GST_BIN (pipeline), source, depay, queue, decoder, convert, sink, NULL);

  /* we link the elements together */
  gst_element_link_many (source, depay, queue, decoder, convert, sink, NULL);

  /* Set the pipeline to "playing" state*/
  g_print ("Now streaming to localhost at port 5004\n");
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  /* Iterate */
  g_print ("Running...\n");
  g_main_loop_run (loop);

  /* Out of the main loop, clean up nicely */
  g_print ("Returned, stopping playback\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_print ("Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (pipeline));
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);

  return 0;
}
