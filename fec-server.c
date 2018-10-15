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

/*FEC encoder */
static GstElement * make_fec_encoder (GstElement *element, guint  session,  gpointer  fec_perc)
{

  gint64 *fec_per = fec_perc;
  GstElement *fecenc = gst_element_factory_make ("rtpulpfecenc", "rtpfecelement_encoder");

  g_object_set (G_OBJECT (fecenc), "pt" ,100, NULL);
  g_object_set (G_OBJECT (fecenc), "multipacket" , TRUE, NULL);
  g_object_set (G_OBJECT (fecenc), "percentage" , *fec_per , NULL);

  return (fecenc);
}

int
main (int   argc,
      char *argv[])
{
  GMainLoop *loop;

  GstElement *pipeline, *source, *encoder, *queue, *pay, *bin, *sink;
  GstBus *bus;
  guint bus_watch_id;

  /* Initialisation */
  gst_init (&argc, &argv);

  loop = g_main_loop_new (NULL, FALSE);

  /* Check input arguments */
  if (argc != 2) {
    g_printerr ("Usage: %s <FEC percentage> \n", argv[0]);
    return -1;
  }

  /* Create gstreamer elements */
  pipeline = gst_pipeline_new ("fec-player");
  source = gst_element_factory_make ("videotestsrc", "test-source");
  encoder = gst_element_factory_make ("x264enc", "h264-encoder");
  queue = gst_element_factory_make ("queue", "queue");
  pay = gst_element_factory_make ("rtph264pay", "rtp-payloader");
  bin = gst_element_factory_make ("rtpbin", "rtp-bin");
  sink = gst_element_factory_make ("udpsink", "udp-sink");

  if (!pipeline || !source || !encoder || !queue || !pay || !bin || !sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  /* we add all elements into the pipeline */
  gst_bin_add_many (GST_BIN (pipeline), source, encoder, queue, pay, bin, sink, NULL);

  /* Connect FEC encoder to rtpbin */
  gint64 fec_perc = g_ascii_strtoll (argv[1] , NULL , 0) ; /* Setting fec percentage */
  g_signal_connect (bin, "request-fec-encoder", G_CALLBACK (make_fec_encoder), &fec_perc);

  /* we link the elements together */
  gst_element_link_many (source, encoder, queue, pay, bin, sink, NULL);

  /* we set the properties of the pipeline elements */
  g_object_set (G_OBJECT (pay), "name", "pay0", "pt", 96, NULL);
  g_object_set (G_OBJECT (sink), "host" , "127.0.0.1", "port", 5004, NULL);

  /* we add a message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  /* Set the pipeline to "playing" state*/
  g_print ("Now playing: %s\n", argv[1]);
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
