/*
 * Copyright (C) 2012-2013 Nicolas Bouillot (http://www.nicolasbouillot.net)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 */

#include "shmdata/base-reader.h"

struct shmdata_base_reader_
{
  //pipeline elements
  GstElement *bin_;
  GstElement *source_;
  GstElement *deserializer_;
  GstElement *typefind_;
  GstElement *sink_;
  GstPad *sink_pad_;
  GstPad *src_pad_;
  GstCaps *caps_;
  //monitoring the shm file
  GFile *shmfile_;
  GFileMonitor *dirMonitor_;
  char *socketName_;
  //user callback
  void (*on_first_data_) (shmdata_base_reader_t *, void *);
  void *on_first_data_userData_;
  void (*on_have_type_) (shmdata_base_reader_t *, GstCaps*, void *);
  void *on_have_type_userData_;
  //sync_handler
  gboolean install_sync_handler_;
  //state
  gboolean attached_;
  //state boolean
  gboolean initialized_;	//the shared video has been attached once
  gboolean do_absolute_;
  gboolean timereset_;
  GstClockTime timeshift_;
};

//FIXME this should be part of the library
void
shmdata_base_reader_unlink_pad (GstPad * pad)
{
  g_print ("base_reader_unlink_pad\n");
  GstPad *peer;
  if ((peer = gst_pad_get_peer (pad))) {
    if (gst_pad_get_direction (pad) == GST_PAD_SRC)
      gst_pad_unlink (pad, peer);
    else
      gst_pad_unlink (peer, pad);
    //checking if the pad has been requested and releasing it needed 
    GstPadTemplate *pad_templ = gst_pad_get_pad_template (peer);//check if this must be unrefed for GST 1
    if (GST_PAD_TEMPLATE_PRESENCE (pad_templ) == GST_PAD_REQUEST)
      gst_element_release_request_pad (gst_pad_get_parent_element(peer), peer);
    gst_object_unref (peer);
  }
}

//FIXME this should be part of the library
void
shmdata_base_reader_clean_element (GstElement *element)
{
  g_print ("base_reader_clean element\n");
  if (element != NULL && GST_IS_ELEMENT (element))
    {
      GstIterator *pad_iter;
      pad_iter = gst_element_iterate_pads (element);
      gst_iterator_foreach (pad_iter, (GFunc) shmdata_base_reader_unlink_pad, element);
      gst_iterator_free (pad_iter);
      if (GST_STATE_TARGET (element) != GST_STATE_NULL)
	if (GST_STATE_CHANGE_ASYNC == gst_element_set_state (element, GST_STATE_NULL))
	  gst_element_get_state (element, NULL, NULL, GST_CLOCK_TIME_NONE);//warning this may be blocking
      if (GST_IS_BIN (gst_element_get_parent (element)))
	gst_bin_remove (GST_BIN (gst_element_get_parent (element)), element);
    }
}



gboolean
shmdata_base_reader_clean_source (gpointer user_data)
{
  shmdata_base_reader_t *context = (shmdata_base_reader_t *) user_data;

  g_print ("base_reader_clean source\n");

  g_debug ("shmdata_base_reader_clean_source");

  shmdata_base_reader_clean_element (context->typefind_);
  shmdata_base_reader_clean_element (context->deserializer_);
  shmdata_base_reader_clean_element (context->source_);

  g_debug ("shmdata_base_reader_clean_source done");
  
  /* if (GST_IS_ELEMENT (context->typefind_))    */
  /*     gst_element_set_state (context->typefind_, GST_STATE_NULL);    */
  
  /*  if (GST_IS_ELEMENT (context->deserializer_))  */
  /*      gst_element_set_state (context->deserializer_, GST_STATE_NULL);  */

  /* if (GST_IS_ELEMENT (context->source_))  */
  /*   if (GST_STATE (context->source_) == GST_STATE_PLAYING) */
  /*     gst_element_set_state (context->source_, GST_STATE_NULL);    */
  
  /* if (GST_IS_BIN (context->bin_)  */
  /*     && GST_IS_ELEMENT (context->typefind_) */
  /*     && GST_ELEMENT_PARENT (context->typefind_) == context->bin_) */
  /*   gst_bin_remove (GST_BIN (context->bin_), context->typefind_); */
  
  /* if (GST_IS_BIN (context->bin_)  */
  /*     && GST_IS_ELEMENT (context->deserializer_) */
  /*     && GST_ELEMENT_PARENT (context->deserializer_) == context->bin_) */
  /*   gst_bin_remove (GST_BIN (context->bin_), context->deserializer_); */

  /* if (GST_IS_BIN (context->bin_)  */
  /*     && GST_IS_ELEMENT (context->source_) */
  /*     && GST_ELEMENT_PARENT (context->source_) == context->bin_) */
  /*     gst_bin_remove (GST_BIN (context->bin_), context->source_); */

  return FALSE;
}

void
shmdata_base_reader_on_type_found (GstElement* typefind, 
				   guint probability, 
				   GstCaps *caps, 
				   gpointer user_data)
{
  g_print ("base_reader_on_type_found\n");

  shmdata_base_reader_t * reader = (shmdata_base_reader_t *) user_data; 
  reader->caps_ = caps;
  if (reader->on_have_type_ != NULL)
    reader->on_have_type_ (reader,
			   reader->caps_,
			   reader->on_have_type_userData_);
  g_debug ("new caps for base reader: %s",gst_caps_to_string (reader->caps_));
}

GstCaps *
shmdata_base_reader_get_caps (shmdata_base_reader_t *reader)
{
  g_print ("base_reader_get_caps\n");

  return reader->caps_;
}

gboolean
shmdata_base_reader_reset_time (GstPad * pad,
				GstMiniObject * mini_obj, gpointer user_data)
{

  shmdata_base_reader_t *context = (shmdata_base_reader_t *) user_data;
  if (GST_IS_EVENT (mini_obj))
    {
      g_debug ("EVENT %s", GST_EVENT_TYPE_NAME (GST_EVENT_CAST(mini_obj)));
    }
  else if (GST_IS_BUFFER (mini_obj))
    {
      GstBuffer *buffer = GST_BUFFER_CAST (mini_obj);
      /* g_debug ("shmdata writer data frame (%p), data size %d, timestamp %llu, caps %s", */
      /* 	       GST_BUFFER_DATA (buffer), GST_BUFFER_SIZE (buffer), */
      /* 	       GST_TIME_AS_MSECONDS (GST_BUFFER_TIMESTAMP (buffer)), */
      /* 	       gst_caps_to_string (GST_BUFFER_CAPS (buffer))); */
      if (context->timereset_)
	{
	  context->timeshift_ = GST_BUFFER_TIMESTAMP (buffer);
	  context->timereset_ = FALSE;
	}
      GST_BUFFER_TIMESTAMP (buffer) =
	GST_BUFFER_TIMESTAMP (buffer) - context->timeshift_;
    }
  else if (GST_IS_MESSAGE (mini_obj))
    {
    }

  return TRUE;
}

gboolean
shmdata_base_reader_attach (shmdata_base_reader_t *reader)
{
  g_print ("base_reader_reader_attach\n");

  if (reader->attached_)
    return FALSE;
  
  reader->attached_ = TRUE;
  reader->source_ = gst_element_factory_make ("shmsrc", NULL);
  reader->deserializer_ = gst_element_factory_make ("gdpdepay", NULL);
  GstPad *pad = gst_element_get_pad(reader->deserializer_, "src");
  gst_pad_add_data_probe (pad,
              G_CALLBACK (shmdata_base_reader_reset_time),
              reader);

  reader->typefind_ = gst_element_factory_make ("typefind", NULL);
  if (reader->typefind_ == NULL)
    g_critical ("no typefind !");
  g_signal_connect (reader->typefind_, "have-type", G_CALLBACK (shmdata_base_reader_on_type_found), reader);

  if (reader->do_absolute_)
    reader->timereset_ = FALSE;

  if (!reader->source_)
    {
      g_critical ("Reader: \"shmsrc\" element could not be created, consider installing libshmdata.");
      return FALSE;
    }
  if (!reader->deserializer_)
    {
      g_critical ("Reader: \"gdpdepay\" element could not be created.");
      return FALSE;
    }
  if (!reader->typefind_)
    {
      g_critical ("Reader: \"typefind\" element could not be created.");
      return FALSE;
    }

  g_object_set_data (G_OBJECT (reader->source_), 
		     "shmdata_base_reader",
		     (gpointer)reader);

  g_object_set_data (G_OBJECT (reader->deserializer_), 
		     "shmdata_base_reader",
		     (gpointer)reader);

  g_object_set (G_OBJECT (reader->source_), "socket-path",
		reader->socketName_, NULL);

  gst_bin_add_many (GST_BIN (reader->bin_),
		    reader->source_, reader->deserializer_, reader->typefind_, NULL);

  reader->src_pad_ = gst_element_get_static_pad (reader->typefind_,
						     "src");
  reader->sink_pad_ = gst_element_get_compatible_pad (reader->sink_,
						     reader->src_pad_,
						     GST_PAD_CAPS
						     (reader->src_pad_));

  gst_element_link_many (reader->source_, 
			 reader->deserializer_,
			 reader->typefind_,
			 NULL);
  gst_pad_link (reader->src_pad_, reader->sink_pad_);

  gst_element_set_state (reader->typefind_,GST_STATE_TARGET(reader->bin_));
  gst_element_set_state (reader->deserializer_,GST_STATE_TARGET(reader->bin_));
  gst_element_set_state (reader->source_,GST_STATE_TARGET(reader->bin_));

  return FALSE; //for g_idle_add
}

void
shmdata_base_reader_file_system_monitor_change (GFileMonitor *monitor,
						GFile *file,
						GFile *other_file,
						GFileMonitorEvent type,
						gpointer user_data)
{
  g_print ("base_reader_file_monitor_change\n");

  char *filename = g_file_get_path (file);
  shmdata_base_reader_t *context = (shmdata_base_reader_t *) user_data;

  switch (type)
    {
    case G_FILE_MONITOR_EVENT_CREATED:
      if (g_file_equal (file, context->shmfile_))
	{
	  if (!context->initialized_)
	    {
	      context->initialized_ = TRUE;
	      context->on_first_data_ (context,
				       context->on_first_data_userData_);
	    }
	  shmdata_base_reader_attach (context);
	}
      break;
      /* case G_FILE_MONITOR_EVENT_DELETED: */
      /* break; */
      /* case G_FILE_MONITOR_EVENT_CHANGED: */
      /*        g_print ("G_FILE_MONITOR_EVENT_CHANGED\n"); */
      /*        break; */
      /* case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED: */
      /*        g_print ("G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED\n"); */
      /*        break; */
      /* case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT: */
      /*        g_print ("G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT\n"); */
      /*        break; */
      /* case G_FILE_MONITOR_EVENT_PRE_UNMOUNT: */
      /*        g_print ("G_FILE_MONITOR_EVENT_PRE_UNMOUNT\n"); */
      /*        break; */
      /* case G_FILE_MONITOR_EVENT_UNMOUNTED: */
      /*        g_print ("G_FILE_MONITOR_EVENT_UNMOUNTED\n"); */
      /*        break; */
    default:
      break;
    }
  g_free (filename);
}

void
shmdata_base_reader_detach (shmdata_base_reader_t * reader)
{
  g_print ("base_reader_reader_detach\n");

  if (reader != NULL)
    {
      reader->attached_ = FALSE;
      shmdata_base_reader_clean_source (reader);
    }
}

gboolean
shmdata_base_reader_recover_from_deserializer_error (shmdata_base_reader_t * reader)
{
  g_print ("base_reader_recover_error\n");

  shmdata_base_reader_detach (reader);

  shmdata_base_reader_attach (reader);
  /* gst_object_unref (reader->deserializer_); */
  /* gst_object_unref (reader->bin_); */

  return FALSE; //for g_idle_add use
}

gboolean
shmdata_base_reader_process_error (shmdata_base_reader_t * reader, GstMessage *msg)
{
  
  g_print ("base_reader_proess_error\n");

  switch (GST_MESSAGE_TYPE (msg))
    {
    case GST_MESSAGE_ERROR:
      {
	gchar *debug;
	GError *error;
	gst_message_parse_error (msg, &error, &debug);
	g_print ("error::::: %s (%s)\n",error->message, GST_OBJECT_NAME (msg->src));
	g_free (debug);
	if (g_strcmp0
	    (GST_ELEMENT_NAME (reader->source_),
	     GST_OBJECT_NAME (msg->src)) == 0)
	  {
	    if (error->code == GST_RESOURCE_ERROR_READ)
	      shmdata_base_reader_detach (reader);
	    g_error_free (error);
	    return TRUE;
	  }

	if (g_strcmp0
	    (GST_ELEMENT_NAME (reader->deserializer_),
	     GST_OBJECT_NAME (msg->src)) == 0)
	  {
	    if (GST_IS_BIN (reader->bin_) && GST_IS_ELEMENT (reader->deserializer_))
	      {
		gst_object_ref (reader->deserializer_);
		gst_bin_remove (GST_BIN (reader->bin_), reader->deserializer_);
	      }
	    
	    if (GST_IS_BIN (reader->bin_) && GST_IS_ELEMENT (reader->source_))
	      {
		gst_object_ref (reader->source_);
		gst_bin_remove (GST_BIN (reader->bin_), reader->source_);
	      }

	    g_idle_add ((GSourceFunc)shmdata_base_reader_recover_from_deserializer_error, (gpointer)reader);
	    g_error_free (error);
	    return TRUE; 
	  }
	g_error_free (error);
	break;
      }
    default:
      break;
    }
  return FALSE;
}

GstBusSyncReply
shmdata_base_reader_message_handler (GstBus * bus,
				     GstMessage * msg, gpointer user_data)
{
    
    g_print ("base_reader_message handler\n");

  /* if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_EOS) */
  /*   { */
  /*     g_print ("message %s from %s\n",GST_MESSAGE_TYPE_NAME(msg),GST_MESSAGE_SRC_NAME(msg)); */
  /*   } */

  shmdata_base_reader_t *reader = (shmdata_base_reader_t *) g_object_get_data (G_OBJECT (msg->src), "shmdata_base_reader");
  if ( reader != NULL)
    {
      if (shmdata_base_reader_process_error (reader, msg)) 
     	return GST_BUS_DROP; 
      else 
     	return GST_BUS_PASS; 
    }
  return GST_BUS_PASS; 
}

shmdata_base_reader_t *
shmdata_base_reader_new ()
{
  g_print ("base_reader_new\n");

  shmdata_base_reader_t *reader =
    (shmdata_base_reader_t *) g_malloc0 (sizeof (shmdata_base_reader_t));
  reader->initialized_ = FALSE;
  reader->on_first_data_ = NULL;
  reader->on_first_data_userData_ = NULL;
  reader->on_have_type_ = NULL;
  reader->on_have_type_userData_ = NULL;
  reader->bin_ = NULL;
  reader->install_sync_handler_ = TRUE;
  reader->attached_ = FALSE;
  reader->do_absolute_ = FALSE;
  reader->timereset_ = TRUE;
  reader->timeshift_ = 0;
  return reader;
}


void 
shmdata_base_reader_set_callback (shmdata_base_reader_t *reader,
				  shmdata_base_reader_on_first_data cb,
				  void *user_data)
{
  g_print ("base_reader_set_callback\n");

  reader->on_first_data_ = cb;
  reader->on_first_data_userData_ = user_data;
}

void 
shmdata_base_reader_set_on_have_type_callback (shmdata_base_reader_t *reader,
					       shmdata_base_reader_on_have_type cb,
					       void *user_data)
{
  g_print ("base_reader_set_on_have_type_callback\n");

  reader->on_have_type_ = cb;
  reader->on_have_type_userData_ = user_data;
}
void 
shmdata_base_reader_install_sync_handler (shmdata_base_reader_t *reader,
					  gboolean install)
{
  g_print ("base_reader_install_sync_handler\n");

  reader->install_sync_handler_ = install;
}

gboolean 
shmdata_base_reader_set_bin (shmdata_base_reader_t * reader, GstElement *bin)
{
  g_print ("base_reader_set_bin\n");

  if (!GST_IS_BIN (bin))
    {
      g_warning ("base reader: not a bin");
      return FALSE;
    }
  
  reader->bin_ = bin;
  return TRUE;
}

gboolean 
shmdata_base_reader_start (shmdata_base_reader_t * reader, const char *socketPath)
{
  
  g_print ("base_reader_start\n");

  if (reader->install_sync_handler_)
    {
      g_debug ("installing an sync handler");
      //looking for the bus, searching the top level
      GstElement *pipe = reader->bin_;
      
      while (pipe != NULL && !GST_IS_PIPELINE (pipe))
	pipe = GST_ELEMENT_PARENT (pipe);
      
      if( GST_IS_PIPELINE (pipe))
	{
	  GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (pipe));  
	  
	  gst_bus_set_sync_handler (bus, shmdata_base_reader_message_handler, NULL);  
	  gst_object_unref (bus);  
	}
      else
	{
	  g_warning ("no top level pipeline found when starting, cannot install sync_handler");
	  return FALSE;
	}
    }
  
  reader->socketName_ = g_strdup (socketPath);
  
  //monitoring the shared memory file
  reader->shmfile_ = g_file_new_for_commandline_arg (reader->socketName_);
  
  if (g_file_query_exists (reader->shmfile_, NULL))
    {
      g_debug ("existing shmdata, attaching (%s)", g_file_get_uri (reader->shmfile_));
      reader->initialized_ = TRUE;
      reader->on_first_data_ (reader, reader->on_first_data_userData_);
      shmdata_base_reader_attach (reader);
    }
  else
    g_debug ("monitoring %s", g_file_get_uri (reader->shmfile_));

  //GFile *dir = g_file_get_parent (reader->shmfile_);
  //g_debug ("trying to monitor directory %s",g_file_get_uri (dir));
  GError *error = NULL;
  //reader->dirMonitor_ = g_file_monitor_directory (dir,
  reader->dirMonitor_ = g_file_monitor_file (reader->shmfile_,
					     G_FILE_MONITOR_NONE,
					     NULL, &error);
  //g_object_unref (dir);
  g_debug ("enabling monitoring");
  if (reader->dirMonitor_ == NULL)
    {
      g_warning ("monitor directory failled: %s",error->message);
      g_error_free (error);
      return FALSE;
    }
  g_signal_connect (reader->dirMonitor_,
		    "changed",
		    G_CALLBACK (shmdata_base_reader_file_system_monitor_change), 
		    reader);

  g_debug ("shmdata reader started (%s)", g_file_get_uri (reader->shmfile_));
  return TRUE;
  
}



shmdata_base_reader_t *
shmdata_base_reader_init (const char *socketName,
			  GstElement *bin,
			  void (*on_first_data) (shmdata_base_reader_t *,
						 void *), void *user_data)
{
  g_print ("base_reader_init\n");
  
  shmdata_base_reader_t *reader =
    (shmdata_base_reader_t *) g_malloc0 (sizeof (shmdata_base_reader_t));
  reader->initialized_ = FALSE;
  reader->on_first_data_ = on_first_data;
  reader->on_first_data_userData_ = user_data;
  reader->socketName_ = g_strdup (socketName);
  reader->attached_ = FALSE;
  reader->bin_ = bin;
  reader->do_absolute_ = FALSE;
  reader->timereset_ = FALSE;
  reader->timeshift_ = 0;

  //looking for the bus, searching the top level
  GstElement *pipe = reader->bin_;
  
  while (pipe != NULL && !GST_IS_PIPELINE (pipe))
      pipe = GST_ELEMENT_PARENT (pipe);

  if( GST_IS_PIPELINE (pipe))
      {
	  GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (pipe));  
	  
	  gst_bus_set_sync_handler (bus, shmdata_base_reader_message_handler, NULL);  
	  gst_object_unref (bus);  
      }
  /* else */
  /*     g_debug ("not watching the bus. Application should handle error messages (GST_RESOURCE_ERROR_READ from shmsrc) and call shmdata_base_reader_process_error.\n"); */
  
  //monitoring the shared memory file
  reader->shmfile_ = g_file_new_for_commandline_arg (reader->socketName_);
  g_debug ("monitoring %s", g_file_get_uri (reader->shmfile_));
  
  if (g_file_query_exists (reader->shmfile_, NULL))
    {
      reader->initialized_ = TRUE;
      reader->on_first_data_ (reader, reader->on_first_data_userData_);
      shmdata_base_reader_attach (reader);
    }

  GFile *dir = g_file_get_parent (reader->shmfile_);
  reader->dirMonitor_ = g_file_monitor_directory (dir,
						  G_FILE_MONITOR_NONE,
						  NULL, NULL);
  g_object_unref (dir);
  g_signal_connect (reader->dirMonitor_,
		    "changed",
		    G_CALLBACK
		    (shmdata_base_reader_file_system_monitor_change), reader);

  return reader;
}

void
shmdata_base_reader_set_sink (shmdata_base_reader_t * reader,
            			      GstElement * sink)
{
  g_print ("base_reader_set_sink\n");

  reader->sink_ = sink;
}

void
shmdata_base_reader_set_absolute_timestamp (shmdata_base_reader_t * reader,
                                            gboolean do_absolute)
{
  g_print ("base_reader_set_unlink_pad\n");

  reader->do_absolute_ = do_absolute;
}

void
shmdata_base_reader_close (shmdata_base_reader_t * reader)
{
  if (reader != NULL)
    {
      if (reader->socketName_ != NULL)
	g_free (reader->socketName_);
      shmdata_base_reader_detach (reader);
      if (reader->shmfile_ != NULL)
	g_object_unref (reader->shmfile_);
      if (reader->dirMonitor_ != NULL)
	g_object_unref (reader->dirMonitor_);
      g_free (reader);
      g_debug ("base reader closed");
    }
}



