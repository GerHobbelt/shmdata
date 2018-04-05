/*
 * This file is part of switcher-pjsip.
 *
 * libswitcher is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#undef NDEBUG  // get assert in release mode

#include <cassert>
#include <vector>
#include "switcher/quiddity-basic-test.hpp"

int main() {
  {
    using namespace switcher;
    switcher::Switcher::ptr manager = Switcher::make_switcher("siptest");

    manager->factory<MPtr(&quid::Factory::scan_dir)>("./");

    assert(test::full(manager, "sip"));

    for (auto& it : manager->quids<MPtr(&quid::Container::get_names)>()) {
      manager->quids<MPtr(&quid::Container::remove)>(
          manager->quids<MPtr(&quid::Container::get_id)>(it));
    }
    manager->conf<MPtr(&Configuration::from_file)>("./config.json");
    assert(manager->quids<MPtr(&quid::Container::create)>("sip", "test"));
  }  // end of scope is releasing the manager
  return 0;
}
