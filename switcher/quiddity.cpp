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

/**
 * The Quiddity class
 */

#include "quiddity.h"
#include "quiddity-manager-impl.h"


namespace switcher
{
  std::map<std::pair <std::string,std::string>, Signal::ptr> Quiddity::signals_;

  Quiddity::Quiddity ()
  {
    gobject_.reset (new GObjectWrapper ());
    gobject_->property_set_default_user_data (this);
    properties_description_.reset (new JSONBuilder());
    methods_description_.reset (new JSONBuilder());
    signals_description_.reset (new JSONBuilder());
  }
  
  Quiddity::~Quiddity () 
  {}

  std::string
  Quiddity::get_name()
  {
    return std::string (name_);
  }

  std::string
  Quiddity::get_nick_name()
  {
    return std::string (nick_name_);
  }

  bool
  Quiddity::set_name(std::string name)
  {
    name_ = name;
    if (nick_name_.empty ()) 
      nick_name_ = name;
    return true;
  }

  bool
  Quiddity::set_nick_name(std::string nick_name)
  {
    nick_name_ = nick_name;
    return true;
  }
  
  
  bool 
  Quiddity::register_signal_gobject (std::string signal_name,
				     GObject *object, 
				     std::string gobject_signal_name)
  {
    std::pair <std::string,std::string> sig_pair = std::make_pair (get_documentation().get_class_name (),
    								   signal_name);
    
    if (signals_.find(sig_pair) != signals_.end())
      {
    	g_warning ("signals: registering name %s already exists",signal_name.c_str());
    	return false;
      }
    
    Signal::ptr signal;
    signal.reset (new Signal ());
    if (!signal->set_gobject_signame (object, gobject_signal_name))
      return false;
    signals_[sig_pair] = signal; 
    g_debug ("signal %s registered with name %s", 
      	     gobject_signal_name.c_str (),
      	     signal_name.c_str ());
    return true;
  }

  // bool 
  // Quiddity::register_signal_gobject_by_id (std::string signal_name,
  // 					   GObject *object, 
  // 					   guint gobject_signal_id)
  // {
  //   std::pair <std::string,std::string> sig_pair = std::make_pair (get_documentation().get_class_name (),
  // 								   signal_name);
    
  //   if (signals_.find(sig_pair) != signals_.end())
  //     {
  // 	g_warning ("signals: registering name %s already exists",signal_name.c_str());
  // 	return false;
  //     }

  //   Signal::ptr signal (new Signal ());
  //   if (!signal->set_gobject_sigid (object, gobject_signal_id))
  //     return false;
  //   signals_[sig_pair] = signal; 
  //   g_debug ("signal %s registered", 
  // 	     signal_name.c_str ());
  //   return true;
  // }

  bool 
  Quiddity::make_custom_signal (const std::string signal_name, //the name to give
				GType return_type,
				guint n_params, //number of params
				GType *param_types)
  {
    std::pair <std::string,std::string> sig_pair = std::make_pair (get_documentation().get_class_name (),
								   signal_name);
    if (signals_.find(sig_pair) != signals_.end())
      {
    	g_warning ("signals: name %s already exists, cannot create a new signal",signal_name.c_str());
    	return false;
      }
    guint id = GObjectWrapper::make_signal (return_type,
    					    n_params,
    					    param_types); 

    if (id == 0)
      {
    	g_warning ("custom signal %s not created because of a type issue");
      }

    Signal::ptr signal (new Signal ());
    if (!signal->set_gobject_sigid (gobject_->get_gobject (), id))
        return false;
    signals_[sig_pair] = signal; 
    g_debug ("signal %s registered", 
     	     signal_name.c_str ());
    return true;
  }


  bool 
  Quiddity::set_signal_description (const std::string signal_name,
				    const std::string short_description,
				    const std::vector<std::pair<std::string,std::string> > arg_description)
  {

    std::pair <std::string,std::string> sig_pair = std::make_pair (get_documentation().get_class_name (),
								   signal_name);
    if (signals_.find(sig_pair) == signals_.end())
      {
	g_error ("cannot set description of a not existing signal");
	return false;
      }
    signals_[sig_pair]->set_description (signal_name, short_description, arg_description);
    return true;
  }


  bool 
  Quiddity::register_property_by_pspec (GObject *object, 
					GParamSpec *pspec, 
					std::string name_to_give)
  {

    Property::ptr prop (new Property ());
    prop->set_gobject_pspec (object, pspec);

    if (properties_.find(name_to_give) == properties_.end())
      {
	properties_[name_to_give] = prop; 
	return true;
      }
    else 
      {
	g_warning ("registering name %s already exists",name_to_give.c_str());
	return false;
      }
    
  }
  
  bool
  Quiddity::register_property (GObject *object, 
			       std::string gobject_property_name, 
			       std::string name_to_give)
  {
    GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS(object), gobject_property_name.c_str());
    if (pspec == NULL)
      {
	g_error ("property not found %s", gobject_property_name.c_str());
	return false;
      }
    
    return register_property_by_pspec (object, pspec, name_to_give);
  }
  


  //return -1 if method not found
  //TODO implement get method and let the manager to call invoke, get_num_args etc...
  int 
  Quiddity::method_get_num_value_args (std::string function_name)
  {
    if (methods_.find( function_name ) == methods_.end())
      {
	g_debug ("Quiddity::method_get_num_value_args error: method %s not found",function_name.c_str());
	return -1;
      }
    else 
      return (int)methods_[function_name]->get_num_of_value_args(); 
  }

  bool 
  Quiddity::invoke_method (std::string function_name, 
			   std::vector<std::string> args)
  {
    if (methods_.find( function_name ) == methods_.end())
      {
	g_error ("Quiddity::invoke_method error: method %s not found",function_name.c_str());
	return false;
      }
    else 
      {
	return methods_[function_name]->invoke (args); 
      }
 }
  

  bool
  Quiddity::register_method (std::string method_name, void *method, std::vector<GType> arg_types, gpointer user_data)
  {
    if (method == NULL)
      {
	g_error ("fail registering %s (method is NULL)",method_name.c_str());
	return false;
      }
    
    Method::ptr meth (new Method ());
    meth->set_method (method, arg_types, user_data);

    if (methods_.find( method_name ) == methods_.end())
      {
	methods_[method_name] = meth;
	return true;
      }
    else 
      {
	g_error ("registering name %s already exists",method_name.c_str());
	return false;
      }
  }

  bool 
  Quiddity::set_method_description (const std::string method_name,
				    const std::string short_description,
				    const std::vector<std::pair<std::string,std::string> > arg_description)
  {
    if (methods_.find( method_name ) == methods_.end())
      {
	g_error ("cannot set description of a not existing method");
	return false;
      }
    methods_[method_name]->set_description (method_name, short_description, arg_description);
    return true;
  }

  std::string 
  Quiddity::get_methods_description ()
  {
    
    methods_description_->reset();
    methods_description_->begin_object ();
    methods_description_->set_member_name ("methods");
    methods_description_->begin_array ();
    for(std::map<std::string, Method::ptr>::iterator it = methods_.begin(); it != methods_.end(); ++it) 
	methods_description_->add_node_value (it->second->get_json_root_node ());
    methods_description_->end_array ();
    methods_description_->end_object ();
    return methods_description_->get_string (true);
  }

  std::string 
  Quiddity::get_method_description (std::string method_name)
  {
    if (methods_.find( method_name ) == methods_.end())
      return "";
    
    Method::ptr meth = methods_[method_name];
    return meth->get_description ();
  }

  std::string 
  Quiddity::get_properties_description ()
  {
    properties_description_->reset();
    properties_description_->begin_object ();
    properties_description_->set_member_name ("properties");
    properties_description_->begin_array ();

    for(std::map<std::string, Property::ptr>::iterator it = properties_.begin(); it != properties_.end(); ++it) 
      {
	properties_description_->begin_object ();
	properties_description_->add_string_member ("name",it->first.c_str ());
	JsonNode *root_node = it->second->get_json_root_node ();
	properties_description_->add_JsonNode_member ("description", root_node);
	properties_description_->end_object ();
      }

    properties_description_->end_array ();
    properties_description_->end_object ();
    
    return properties_description_->get_string (true);
  }

  std::string 
  Quiddity::get_property_description (std::string name)
  {
    if (properties_.find( name ) == properties_.end())
      return "";
    
    Property::ptr prop = properties_[name];
    return prop->get_description ();
  }

  bool 
  Quiddity::set_property (std::string name, std::string value)
  {
    if (properties_.find( name ) == properties_.end())
      return false;

    Property::ptr prop = properties_[name];
    prop->set (value);
    return true;
  }

  std::string 
  Quiddity::get_property (std::string name)
  {
    if (properties_.find( name ) == properties_.end())
      return "property not found";

    Property::ptr prop = properties_[name];
    return prop->get ();
  }

  bool 
  Quiddity::subscribe_property (std::string name, 
				Property::Callback cb,
				void *user_data)
  {
    if (properties_.find( name ) == properties_.end())
      return false;

    Property::ptr prop = properties_[name];
    return prop->subscribe (cb, user_data);
  }

  bool 
  Quiddity::unsubscribe_property (std::string name,
				  Property::Callback cb,
				  void *user_data)
  {
    if (properties_.find (name) == properties_.end())
      return false;

    Property::ptr prop = properties_[name];
    return prop->unsubscribe (cb, user_data);
  }

  bool 
  Quiddity::subscribe_signal (std::string signal_name, 
			      Signal::OnEmittedCallback cb,
			      void *user_data)
  {
    std::pair <std::string,std::string> sig_pair = std::make_pair (get_documentation().get_class_name (),
								   signal_name);
    
    if (signals_.find(sig_pair) == signals_.end())
      {
	g_warning ("Quiddity::subscribe_signal, signal %s not found", signal_name.c_str ());
	return false;
      }
    Signal::ptr sig = signals_[sig_pair];
    return sig->subscribe (cb, user_data);
  }

  bool 
  Quiddity::unsubscribe_signal (std::string signal_name,
				Signal::OnEmittedCallback cb,
				void *user_data)
  {
    std::pair <std::string,std::string> sig_pair = std::make_pair (get_documentation().get_class_name (),
								   signal_name);

    if (signals_.find (sig_pair) == signals_.end())
      return false;

    Signal::ptr signal = signals_[sig_pair];
    return signal->unsubscribe (cb, user_data);
  }

  void
  Quiddity::signal_emit (const std::string signal_name, 
			 ...)
  {
    std::pair <std::string,std::string> sig_pair = std::make_pair (get_documentation().get_class_name (),
								   signal_name);
    
    if (signals_.find (sig_pair) == signals_.end())
	return;
    Signal::ptr signal = signals_[sig_pair];
    va_list var_args;
    va_start (var_args, signal_name);
    // va_list va_cp;
    // va_copy (va_cp, var_args);
    // signal->signal_emit (signal_name.c_str (), va_cp); 
    signal->signal_emit (signal_name.c_str (), var_args); 
    va_end (var_args);
  }

  std::string 
  Quiddity::get_signals_description ()
  {
    // properties_description_->reset();
    // properties_description_->begin_object ();
    // properties_description_->set_member_name ("properties");
    // properties_description_->begin_array ();

    // for(std::map<std::string, Property::ptr>::iterator it = properties_.begin(); it != properties_.end(); ++it) 
    //   {
    // 	properties_description_->begin_object ();
    // 	properties_description_->add_string_member ("name",it->first.c_str ());
    // 	JsonNode *root_node = it->second->get_json_root_node ();
    // 	properties_description_->add_JsonNode_member ("description", root_node);
    // 	properties_description_->end_object ();
    //   }

    // properties_description_->end_array ();
    // properties_description_->end_object ();
    
    //return properties_description_->get_string (true);
    return "{ \"what\" : \"to be implemented\"}}";
  }

  std::string 
  Quiddity::get_signal_description (std::string signal_name)
  {
    // if (properties_.find( name ) == properties_.end())
    //   return "";
    
    // Property::ptr prop = properties_[name];
    // return prop->get_description ();
    return "{\"what\": \"to be implemented\"}";
  }


  std::string
  Quiddity::make_file_name (std::string suffix)
  {
    std::string connector_name;
    QuiddityManager_Impl::ptr manager = manager_impl_.lock ();
    if ( (bool)manager)
      connector_name.append ("/tmp/switcher_"+manager->get_name ()+"_"+nick_name_+"_"+suffix);
    else
      connector_name.append ("/tmp/switcher__"+nick_name_+"_"+ suffix); 

    return connector_name;
  }

  std::string
  Quiddity::get_socket_name_prefix ()
  {
    return "switcher_";
  }

  std::string
  Quiddity::get_socket_dir ()
  {
    return "/tmp";
  }


  void
  Quiddity::set_manager_impl (QuiddityManager_Impl::ptr manager_impl)
  {
    manager_impl_ = manager_impl;
  }

}
