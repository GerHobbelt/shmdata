/*
 * Copyright (C) 2012 Nicolas Bouillot (http://www.nicolasbouillot.net)
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

#include "switcher/quiddity-documentation.h"
#include "switcher/quiddity-life-manager.h"
#include "switcher/quiddity.h" 

//removing shmdata 
#include <gio/gio.h>

//the base quiddities to manage (line sorted)
#include "switcher/aac.h"
#include "switcher/audio-test-source.h"
#include "switcher/aravis-genicam.h"
#include "switcher/soap-ctrl-server.h"
#include "switcher/gconf-audio-sink.h"
#include "switcher/gconf-audio-source.h"
#include "switcher/gconf-video-sink.h"
#include "switcher/gconf-video-source.h"
#include "switcher/h264.h"
#include "switcher/http-sdp.h"
#include "switcher/pulse-sink.h"
#include "switcher/rtp-session.h"
#include "switcher/runtime.h"
#include "switcher/udpsink.h"
#include "switcher/uridecodebin.h"
#include "switcher/video-test-source.h"
#include "switcher/xvimagesink.h"

namespace switcher
{


  QuiddityLifeManager::ptr 
  QuiddityLifeManager::make_life_manager ()
  {
    QuiddityLifeManager::ptr manager(new QuiddityLifeManager);
    return manager;
  }
  
  QuiddityLifeManager::ptr 
  QuiddityLifeManager::make_life_manager (std::string name)
  {
    QuiddityLifeManager::ptr manager(new QuiddityLifeManager(name));
    return manager;
  }

  QuiddityLifeManager::QuiddityLifeManager() :
    name_ ("default")
  {
    remove_shmdata_sockets ();
    register_classes ();
    classes_doc_.reset (new JSONBuilder ());
    make_classes_doc ();
  }
  
  QuiddityLifeManager::QuiddityLifeManager(std::string name) :
    name_ (name)
  {
    remove_shmdata_sockets ();
    register_classes ();
    classes_doc_.reset (new JSONBuilder ());
    make_classes_doc ();
  }

  void
  QuiddityLifeManager::remove_shmdata_sockets ()
  {
    
    GFile *shmdata_dir = g_file_new_for_commandline_arg (Quiddity::get_socket_dir().c_str ());

    gchar *shmdata_prefix = g_strconcat (Quiddity::get_socket_name_prefix ().c_str (), 
					 name_.c_str (), 
					 "_",
					 NULL);
    
    gboolean res;
    GError *error;
    GFileEnumerator *enumerator;
    GFileInfo *info;
    GFile *descend;
    char *relative_path;
    
    error = NULL;
    enumerator =
      g_file_enumerate_children (shmdata_dir, "*",
				 G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL,
				 &error);
    if (! enumerator)
      return;
    error = NULL;
    info = g_file_enumerator_next_file (enumerator, NULL, &error);
    while ((info) && (!error))
      {
	descend = g_file_get_child (shmdata_dir, g_file_info_get_name (info));
	//g_assert (descend != NULL);
	relative_path = g_file_get_relative_path (shmdata_dir, descend);

	
	
	error = NULL;
	
	if (g_str_has_prefix (relative_path, shmdata_prefix))
	  {
	    g_warning ("deleting previous shmdata socket (%s)", g_file_get_path (descend));
	    res = g_file_delete (descend, NULL, &error);
	    if(res != TRUE)
	      g_warning ("socket cannot be deleted");
	  }
	
	g_object_unref (descend);
	error = NULL;
	info = g_file_enumerator_next_file (enumerator, NULL, &error);
      }
    if (error != NULL)
      g_debug ("error not NULL");
    
    error = NULL;
    res = g_file_enumerator_close (enumerator, NULL, &error);
    if (res != TRUE)
      g_debug ("QuiddityLifeManager: file enumerator not properly closed");
    if (error != NULL)
      g_debug ("error not NULL");
    
    g_object_unref (shmdata_dir);
    g_free (shmdata_prefix);
  }
  
  std::string
  QuiddityLifeManager::get_name ()
  {
    return name_; 
  }
  
  void
  QuiddityLifeManager::register_classes ()
  {
    //registering quiddities
    abstract_factory_.register_class<AAC> (AAC::doc_.get_class_name (), 
					   AAC::doc_.get_json_root_node ());
    abstract_factory_.register_class<AudioTestSource> (AudioTestSource::doc_.get_class_name (), 
      						       AudioTestSource::doc_.get_json_root_node ());
    abstract_factory_.register_class<AravisGenicam> (AravisGenicam::doc_.get_class_name (), 
      						     AravisGenicam::doc_.get_json_root_node ());
    abstract_factory_.register_class<SoapCtrlServer> (SoapCtrlServer::doc_.get_class_name (), 
     						      SoapCtrlServer::doc_.get_json_root_node ());
    abstract_factory_.register_class<GconfAudioSink> (GconfAudioSink::doc_.get_class_name (), 
      						      GconfAudioSink::doc_.get_json_root_node ());
    abstract_factory_.register_class<GconfAudioSource> (GconfAudioSource::doc_.get_class_name (), 
      							GconfAudioSource::doc_.get_json_root_node ());
    abstract_factory_.register_class<GconfVideoSink> (GconfVideoSink::doc_.get_class_name (), 
      						      GconfVideoSink::doc_.get_json_root_node ());
    abstract_factory_.register_class<GconfVideoSource> (GconfVideoSource::doc_.get_class_name (),
      							GconfVideoSource::doc_.get_json_root_node ());
    abstract_factory_.register_class<H264> (H264::doc_.get_class_name (), 
      					    H264::doc_.get_json_root_node ());
    abstract_factory_.register_class<HTTPSDP> (HTTPSDP::doc_.get_class_name (), 
      					       HTTPSDP::doc_.get_json_root_node ());
    abstract_factory_.register_class<PulseSink> (PulseSink::doc_.get_class_name (), 
      						 PulseSink::doc_.get_json_root_node ());
    abstract_factory_.register_class<RtpSession> (RtpSession::doc_.get_class_name (), 
      						  RtpSession::doc_.get_json_root_node ());
    abstract_factory_.register_class<Runtime> (Runtime::doc_.get_class_name (), 
      					       Runtime::doc_.get_json_root_node ());
    abstract_factory_.register_class<UDPSink> (UDPSink::doc_.get_class_name (), 
      					       UDPSink::doc_.get_json_root_node ());
    abstract_factory_.register_class<Uridecodebin> (Uridecodebin::doc_.get_class_name (), 
      						    Uridecodebin::doc_.get_json_root_node ());
    abstract_factory_.register_class<VideoTestSource> (VideoTestSource::doc_.get_class_name (),
      						       VideoTestSource::doc_.get_json_root_node ());
    abstract_factory_.register_class<Xvimagesink> (Xvimagesink::doc_.get_class_name (),
      						   Xvimagesink::doc_.get_json_root_node ());
  }

  std::vector<std::string> 
  QuiddityLifeManager::get_classes ()
  {
    //return abstract_factory_.get_classes_documentation ();
    return abstract_factory_.get_keys ();
  }

  void
  QuiddityLifeManager::make_classes_doc ()
  {
    std::vector<JSONBuilder::Node> docs = abstract_factory_.get_classes_documentation ();
    classes_doc_->reset ();
    classes_doc_->begin_object ();
    classes_doc_->set_member_name ("classes");
    classes_doc_->begin_array ();
    for(std::vector<JSONBuilder::Node>::iterator it = docs.begin(); it != docs.end(); ++it) 
      classes_doc_->add_node_value (*it);
    classes_doc_->end_array ();
    classes_doc_->end_object ();
  }

  std::string 
  QuiddityLifeManager::get_classes_doc ()
  {
    return classes_doc_->get_string ();
  }
  
  bool 
  QuiddityLifeManager::class_exists (std::string class_name)
  {
    return abstract_factory_.key_exists (class_name);
  }


  void 
  QuiddityLifeManager::init_quiddity (Quiddity::ptr quiddity)
  {
    	quiddity->set_life_manager (shared_from_this());
	 if (!quiddity->init ())
	   g_critical ("QuiddityLifeManager: intialization of %s (%s) return false",
		       quiddity->get_name ().c_str (),
		       quiddity->get_documentation ().get_class_name ().c_str ());
	quiddities_.insert (quiddity->get_name(),quiddity);
	quiddities_nick_names_.insert (quiddity->get_nick_name (),quiddity->get_name());
  }

  std::string 
  QuiddityLifeManager::create (std::string quiddity_class)
  {
     if(!class_exists (quiddity_class))
      return "";
    
     Quiddity::ptr quiddity = abstract_factory_.create (quiddity_class);
     if (quiddity.get() != NULL)
	 init_quiddity (quiddity);
     
     g_message ("(%s) quiddity %s created (%s)",
		name_.c_str(), 
		quiddity->get_nick_name ().c_str (), 
		quiddity->get_name ().c_str ());

     return quiddity->get_nick_name ();
  }

  std::string 
  QuiddityLifeManager::create (std::string quiddity_class, std::string nick_name)
  {
    if(!class_exists (quiddity_class))
      return "";

    Quiddity::ptr quiddity = abstract_factory_.create (quiddity_class);

    if (quiddity.get() != NULL)
      {
	if (!nick_name.empty () && !quiddities_nick_names_.contains (nick_name))
	  quiddity->set_nick_name (nick_name);
	else
	  g_warning ("QuiddityLifeManager::create: nick name %s not valid, using %s",
		   nick_name.c_str (),
		   quiddity->get_name().c_str ());

	init_quiddity (quiddity);

     g_message ("(%s) quiddity %s created (%s)",
		name_.c_str(), 
		quiddity->get_nick_name ().c_str (), 
		quiddity->get_name ().c_str ());

      }
    return quiddity->get_nick_name ();
  }

  std::vector<std::string> 
  QuiddityLifeManager::get_instances ()
  {
    return quiddities_nick_names_.get_keys();
  }

  Quiddity::ptr 
  QuiddityLifeManager::get_quiddity (std::string quiddity_name)
  {
     if (!exists (quiddity_name))
      {
	g_critical ("quiddity %s not found, cannot provide ptr",quiddity_name.c_str());
	Quiddity::ptr empty_quiddity_ptr;
	return empty_quiddity_ptr;
      }
    return quiddities_.lookup (quiddities_nick_names_.lookup (quiddity_name));
  }

  bool 
  QuiddityLifeManager::exists (std::string quiddity_name)
  {
    return quiddities_nick_names_.contains (quiddity_name);
  }

  bool 
  QuiddityLifeManager::remove (std::string quiddity_name)
  {
    if (exists (quiddity_name))
      quiddities_.remove (quiddities_nick_names_.lookup (quiddity_name));
    
    if (quiddities_nick_names_.remove (quiddity_name))
      {  
        g_message ("(%s) quiddity removed (%s)",name_.c_str(), quiddity_name.c_str());
	return true;
      }
    g_warning ("(%s) quiddity %s not found for removing",name_.c_str(), quiddity_name.c_str());
    return false; 
  }

  std::string 
  QuiddityLifeManager::get_properties_description (std::string quiddity_name)
  {

    if (!exists (quiddity_name))
      {
	g_warning ("quiddity %s not found, cannot get description of properties",quiddity_name.c_str());
	return "";
      }
    return (get_quiddity (quiddity_name))->get_properties_description ();
  }

  std::string 
  QuiddityLifeManager::get_property_description (std::string quiddity_name, std::string property_name)
  {

    if (!exists (quiddity_name))
      {
	g_warning ("quiddity %s not found, cannot get description of properties",quiddity_name.c_str());
	return "";
      }
    return (get_quiddity (quiddity_name))->get_property_description (property_name);
  }

  bool
  QuiddityLifeManager::set_property (std::string quiddity_name,
				 std::string property_name,
				 std::string property_value)
  {
    if (!exists (quiddity_name))
      {
	g_warning ("quiddity %s not found, cannot set property",quiddity_name.c_str());
	return false;
      }
    return (get_quiddity (quiddity_name))->set_property(property_name.c_str(),property_value.c_str());
  }

  std::string
  QuiddityLifeManager::get_property (std::string quiddity_name,
				 std::string property_name)
  {
    if (!exists (quiddity_name))
      {
	g_warning ("quiddity %s not found, cannot get property",quiddity_name.c_str());
	return "error, quiddity not found";
      }
    return (get_quiddity (quiddity_name))->get_property(property_name.c_str());
  }

  bool 
  QuiddityLifeManager::invoke (std::string quiddity_name, 
			       std::string function_name,
			       std::vector<std::string> args)
  {
    //g_debug ("QuiddityLifeManager::quiddity_invoke_method %s %s, arg size %d",quiddity_name.c_str(), function_name.c_str(), args.size ());
    
    if (!exists (quiddity_name))
      {
	g_warning ("quiddity %s not found, cannot invoke",quiddity_name.c_str());
	return false;
      }
    Quiddity::ptr quiddity = get_quiddity (quiddity_name);

    int num_val = quiddity->method_get_num_value_args(function_name);
    
    if (num_val == -1) 
      {
	g_debug ("function %s not found, cannot invoke",function_name.c_str());
	return false;
      }

    if ((int)args.size () != num_val)
      {
	g_warning ("invoking %s/%s, number of arguments does not match",quiddity_name.c_str(),function_name.c_str());
	return false;
      }

    return quiddity->invoke_method (function_name, args);
  } 

  std::string
  QuiddityLifeManager::get_methods_description (std::string quiddity_name)
  {
    if (!exists (quiddity_name))
      {
	g_warning ("quiddity %s not found, cannot get description of methods",quiddity_name.c_str());
	return "error, quiddity not found";
      }
     
    return (get_quiddity (quiddity_name))->get_methods_description ();
  }

  std::string
  QuiddityLifeManager::get_method_description (std::string quiddity_name, std::string method_name)
  {
    if (!exists (quiddity_name))
      {
	g_warning ("quiddity %s not found, cannot get description of methods",quiddity_name.c_str());
	return "error, quiddity not found";
      }
     
    return (get_quiddity (quiddity_name))->get_method_description (method_name);
  }

  
} // end of namespace
