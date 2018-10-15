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

/* rtpstorage connect */
void storage_connect (GstElement* object, GstElement* storage, guint arg1, gpointer user_data)
{
   g_object_set (G_OBJECT (storage), "size-time" , 250000000, NULL);
}

/* FEC Decoder */
static  GstElement * make_fec_decoder (GstElement* rtpbin ,guint session,gpointer user_data)
{

  GObject *internal_storage = NULL;
  GstElement *fecdec = gst_element_factory_make ("rtpulpfecdec", "rtpfecelement_decoder");
  g_signal_emit_by_name(G_OBJECT (rtpbin),"get-internal-storage",session,&internal_storage);

  g_object_set (G_OBJECT (fecdec), "pt" , 100, NULL);
  g_object_set (G_OBJECT (fecdec), "storage" ,internal_storage, NULL);

  g_object_unref (internal_storage);

  return(fecdec);
}

/* Caps Handle for rtpbin */
GstCaps * caps_handle (GstElement* object, guint ssrc,guint pt,gpointer user_data)
{
    GstCaps *rtp_caps = NULL;
    switch(pt)
    {
      case 100 : rtp_caps = gst_caps_new_simple ("application/x-rtp",
                                                 "clock-rate", G_TYPE_INT, 90000,
                                                 "media",G_TYPE_STRING,"video",
                                                 "is-fec", G_TYPE_BOOLEAN, TRUE, NULL);
                break;
      case  96 : rtp_caps = gst_caps_new_simple ("application/x-rtp",
                                                "clock-rate", G_TYPE_INT, 90000,
                                                "media",G_TYPE_STRING,"video",
                                                "encoding-name",G_TYPE_STRING,"H264", NULL);

                break;
    default :   break;

    }
    return rtp_caps;
}

int
main (int   argc,
      char *argv[])
{
  GMainLoop *loop;
  GstElement *pipeline, *source, *rtpbin, *depay, *queue, *decoder, *convert, *sink;
  GstBus *bus;
  guint bus_watch_id;

  /* Initialisation */
  gst_init (&argc, &argv);
  loop = g_main_loop_new (NULL, FALSE);

  /* Create pipeline elements */
  pipeline = gst_pipeline_new ("test-udp-reciever");
  source = gst_element_factory_make ("udpsrc", "udp-source");
  rtpbin = gst_element_factory_make ("rtpbin", "rtp-bin");
  depay = gst_element_factory_make ("rtph264depay", "rtph264depay");
  queue = gst_element_factory_make ("queue", "queue");
  decoder = gst_element_factory_make ("avdec_h264", "h264-decoder");
  convert = gst_element_factory_make ("videoconvert", "videoconvert");
  sink = gst_element_factory_make ("autovideosink", "autovideosink");

  if (!pipeline || !source || !rtpbin || !depay || !queue || !decoder || !convert || !sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  /* we set the properties of the pipeline elements */
  g_object_set (G_OBJECT (source), "port", 5004, NULL);

  /* we add a message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  /* we add all elements into the pipeline */
  gst_bin_add_many (GST_BIN (pipeline), source, rtpbin, depay, queue, decoder, convert, sink, NULL);

  /* Connect to the rtpssrc rtpptdemux rtpstorage and fecdec signal */
  g_signal_connect (rtpbin, "new-storage", G_CALLBACK (storage_connect), NULL);
  g_signal_connect (rtpbin, "request-pt-map", G_CALLBACK (caps_handle), NULL);
  g_signal_connect (rtpbin, "request-fec-decoder", G_CALLBACK (make_fec_decoder), NULL);

  /* we link the elements together */
  gst_element_link_many (source, rtpbin, depay, queue, decoder, convert, sink, NULL);

  /* Set the pipeline to "playing" state*/
  g_print ("Now playing incoming stream from localhost on port 5004\n");
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
