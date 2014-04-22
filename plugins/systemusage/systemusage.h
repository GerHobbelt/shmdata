/*
 * This file is part of switcher-top.
 *
 * switcher-top is free software; you can redistribute it and/or
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

#ifndef __SWITCHER_SYSTEM_USAGE_H__
#define __SWITCHER_SYSTEM_USAGE_H__

#include "switcher/quiddity.h"
#include "switcher/startable-quiddity.h"
#include "switcher/custom-property-helper.h"

#include <memory>
#include <thread>

namespace switcher
{
  struct Cpu
  {
    long user {0};
    long nice {0};
    long system {0};
    long idle {0};
    long io {0};
    long irq {0};
    long softIrq {0};
    long steal {0};
    long guest {0};

    int totalTime {0};
  };
  
  class SystemUsage : public Quiddity
  {
  public:
    SWITCHER_DECLARE_QUIDDITY_PUBLIC_MEMBERS(SystemUsage);
    SystemUsage ();
    ~SystemUsage ();
    SystemUsage (const SystemUsage &) = delete;
    SystemUsage &operator= (const SystemUsage &) = delete;
    bool init ();

  private:
    std::shared_ptr<std::thread> pollStateThread_;
    bool running_ {false};

    data::Tree::ptr tree_;

    int cpuNbr_;
    int refreshRate_;
    std::map<std::string, Cpu> _cpus;

    void pollState();
  };
  
  SWITCHER_DECLARE_PLUGIN(SystemUsage);

}  // end of namespace

#endif // ifndef
