/*
 * Copyright (C) 2012-2013 Nicolas Bouillot (http://www.nicolasbouillot.net)
 *
 * This file is part of switcher.
 *
 * switcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * switcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with switcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "switcher/quiddity-manager.h"
#include <iostream>
#include <signal.h>
#include <time.h>

#include "switcher/gobject-wrapper.h"

static gchar *server_name = NULL;
static gchar *port_number = NULL;
static gboolean quiet;
static gboolean debug;
static gboolean verbose;
//static gchar **remaining_args = NULL;

static gboolean listclasses;
static gboolean classesdoc;
static gchar *classdoc = NULL;
static gchar *listpropbyclass = NULL;
static gchar *listmethodsbyclass = NULL;

static std::vector<switcher::QuiddityManager::ptr> container;

static GOptionEntry entries[] =
  {
    { "server-name", 's', 0, G_OPTION_ARG_STRING, &server_name, "server name (default is \"default\")", NULL },
    { "port-number", 'p', 0, G_OPTION_ARG_STRING, &port_number, "port number the server will bind (default is 8080)", NULL },
    { "quiet", 'q', 0, G_OPTION_ARG_NONE, &quiet, "do not display any message", NULL },
    { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "display all messages, excluding debug", NULL },
    { "debug", 'd', 0, G_OPTION_ARG_NONE, &debug, "display all messages, including debug", NULL },
    { "list-classes", 'C', 0, G_OPTION_ARG_NONE, &listclasses, "list quiddity classes", NULL },
    { "list-props-by-class", 'P', 0, G_OPTION_ARG_STRING, &listpropbyclass, "list properties of a class", NULL },
    { "list-methods-by-class", 'M', 0, G_OPTION_ARG_STRING, &listmethodsbyclass, "list methods of a class", NULL },
    { "classes-doc", NULL, 0, G_OPTION_ARG_NONE, &classesdoc, "print classes documentation, JSON-formated", NULL },
    { "class-doc", NULL, 0, G_OPTION_ARG_STRING, &classdoc, "print class documentation, JSON-formated (--class-doc class_name)", NULL },
    { NULL }
  };

  static bool
  mon_set_int (const GValue *value,
	       void *user_data)
  {
    g_print ("mon set called with %d\n", g_value_get_int (value));
  }

  static bool
  mon_get_int (GValue *value,
	       void *user_data)
  {
    g_value_set_int (value, 4);
    g_print ("mon set called with %d\n", g_value_get_int (value));
  }



void
leave (int sig)
{
  //removing reference to manager in order to delete it
  container.clear ();
  exit (sig);
}

 static void
 log_handler (const gchar *log_domain, 
 	     GLogLevelFlags log_level,
 	     const gchar *message,
 	     gpointer user_data)
 {
   //OsgReader_impl *context = static_cast<OsgReader_impl*>(user_data);
    if (quiet)
      return;

   switch (log_level) {
   case G_LOG_LEVEL_ERROR:
     g_print ("%s-error: %s\n",log_domain, message);
     break;
   case G_LOG_LEVEL_CRITICAL:
     g_print ("%s-critical: %s\n",log_domain, message);
     break;
   case G_LOG_LEVEL_WARNING:
     g_print ("%s-warning: %s\n",log_domain, message);
     break;
   case G_LOG_LEVEL_MESSAGE:
     if (debug || verbose)
       g_print ("%s-message: %s\n",log_domain, message);
     break;
   case G_LOG_LEVEL_INFO:
     if (debug || verbose)
       g_print ("%s-info: %s\n",log_domain, message);
     break;
   case G_LOG_LEVEL_DEBUG:
     if (debug)
       g_print ("%s-debug: %s\n",log_domain, message);
     break;
   default:
     g_print ("%s-unknown-level: %s\n",log_domain,message);
     break;
   }
 }

int
main (int argc,
      char *argv[])
{
  (void) signal (SIGINT, leave);

  //command line options
  GError *error = NULL;
  GOptionContext *context;
  context = g_option_context_new ("- switcher server");
  g_option_context_add_main_entries (context, entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_print ("option parsing failed: %s\n", error->message);
      exit (1);
    } 

  g_log_set_default_handler (log_handler, NULL);


  //checking if this is printing info only
  if (listclasses)
    {
      quiet=TRUE;
      switcher::QuiddityManager::ptr manager 
	= switcher::QuiddityManager::make_manager ("immpossible_name");  
      std::vector<std::string> resultlist = manager->get_classes ();
      for(uint i = 0; i < resultlist.size(); i++)
	g_print ("%s\n",resultlist[i].c_str ());
      return 0;
    }
  if (classesdoc)
    {
      quiet=TRUE;
      switcher::QuiddityManager::ptr manager 
	= switcher::QuiddityManager::make_manager ("immpossible_name");  
      g_print ("%s\n", manager->get_classes_doc ().c_str ());
      return 0;
    }
  if (classdoc != NULL)
    {
      quiet=TRUE;
      switcher::QuiddityManager::ptr manager 
	= switcher::QuiddityManager::make_manager ("immpossible_name");  
      g_print ("%s\n", manager->get_class_doc (classdoc).c_str ());
      return 0;
    }
  if (listpropbyclass != NULL)
    {
      quiet=TRUE;
      switcher::QuiddityManager::ptr manager 
   	= switcher::QuiddityManager::make_manager ("immpossible_name");  
      g_print ("%s\n", manager->get_properties_description_by_class (listpropbyclass).c_str ());
      return 0;
    }
  if (listmethodsbyclass != NULL)
    {
      quiet=TRUE;
      switcher::QuiddityManager::ptr manager 
   	= switcher::QuiddityManager::make_manager ("immpossible_name");  
      g_print ("%s\n", manager->get_methods_description_by_class (listmethodsbyclass).c_str ());
      return 0;
    }

  //running a switcher server  

  if (server_name == NULL)
    server_name = "default";
  if (port_number == NULL)
    port_number = "8080";



  {
    //using context in order to let excluse ownership of manager by the container,
    //allowing to properly call destructor when SIGINT
    switcher::QuiddityManager::ptr manager 
      = switcher::QuiddityManager::make_manager (server_name);  

     container.push_back (manager); // keep reference only in the container
     // Create a runtime (pipeline0)
     //std::string runtime = 
     manager->create ("runtime");

     std::string soap_name = manager->create ("SOAPcontrolServer", "soapserver");
     std::vector<std::string> port_arg;
     port_arg.push_back (port_number);
     manager->invoke (soap_name, "set_port", port_arg);

     //setting auto_invoke for attaching to gst pipeline "pipeline0"
     std::vector<std::string> arg;
     arg.push_back ("pipeline0");
     manager->auto_invoke ("set_runtime",arg);

       
     GParamSpec *heu1 = 
       switcher::GObjectWrapper::make_int_property ("heuuu", 
						    "ba heu c'est eux",
						    0,
						    10,
						    1,
						    (GParamFlags)G_PARAM_READWRITE,
						    &mon_set_int,
						    &mon_get_int);
     
     GParamSpec *heu2 = 
       switcher::GObjectWrapper::make_int_property ("heuuu", 
						    "AAAAAAAAba heu c'est eux",
						    0,
						    10,
						    1,
						    (GParamFlags) G_PARAM_READWRITE,
						    &mon_set_int,
						    &mon_get_int);
     
     switcher::GObjectWrapper *truc = new switcher::GObjectWrapper ();
     
     switcher::Property *proper = new switcher::Property ();
     proper->set_gobject_pspec (truc->get_gobject (),
				heu1);
     proper->set ("3");
     g_print ("get returned %s\n", proper->get ().c_str ());
     delete proper;

     switcher::Property *proper2 = new switcher::Property ();
     proper2->set_gobject_pspec (truc->get_gobject (),
				heu2);
     proper2->set ("3");
     g_print ("get returned %s\n", proper2->get ().c_str ());
     delete proper2;

     delete truc;

  }

  //waiting for end of life
  timespec delay;
  delay.tv_sec = 1;
  delay.tv_nsec = 0;
  while (1)
    nanosleep(&delay, NULL);
  
  return 0;
}



