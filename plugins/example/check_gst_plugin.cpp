/*
 * This file is part of switcher-plugin-example.
 *
 * switcher-plugin-example is free software; you can redistribute it and/or
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

#include <thread>
#include <cassert>
#include "switcher/quiddity/basic-test.hpp"
#include "switcher/switcher.hpp"

int main() {
  {
    using namespace switcher;
    using namespace quiddity;
    using namespace std::chrono_literals;

    Switcher::ptr manager = Switcher::make_switcher("test_manager");
    manager->factory<MPtr(&quiddity::Factory::scan_dir)>("./");
    assert(quiddity::test::full(manager, "gst"));

    auto qrox =
        manager->quids<MPtr(&quiddity::Container::create)>("gst", "Gst(frame test)", nullptr);
    auto quid = qrox.get();
    assert(quid);

    assert(manager->quids<MPtr(&quiddity::Container::remove)>(qrox.get_id()));
  }

  return 0;
}
