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

#include "switcher/quiddity-manager.h"
#include <vector>
#include <string>

int
main (int argc,
      char *argv[])
{
  bool success = true;
  switcher::QuiddityManager *manager = new switcher::QuiddityManager("test_manager");  
  
  std::vector<std::string> classes = manager->get_classes ();
  
  std::vector<std::string>::iterator iter;
  
  for (iter = classes.begin(); iter != classes.end (); ++iter)
    {

      gchar **class_description = g_strsplit_set (iter->c_str (), "\"",-1);
      std::string class_name (class_description[7]);

	std::string res = manager->create(class_name, class_name);
      if (res.compare (class_name) != 0)
	{
	  g_printerr ("quiddity %s not created properly",iter->c_str ());
	  success = false;
	}
      
      if (!manager->remove (class_name))
	{
	  g_printerr ("error while removing quiddity %s",iter->c_str ());
	  success = false;
	}
    }
  
  if (success)
    return 0;
  else
    return 1;
}



